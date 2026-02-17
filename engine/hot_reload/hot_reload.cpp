/**
 * ACE Engine — Hot Reload Implementation
 */

#include "hot_reload.h"
#include <algorithm>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace ace {

// ============================================================================
// FILE WATCHER
// ============================================================================

void FileWatcher::Watch(const std::string& directory, bool recursive) {
    std::lock_guard lock(_mutex);

    WatchEntry entry;
    entry.directory = directory;
    entry.recursive = recursive;
    ScanDirectory(entry); // Initial snapshot
    _entries.push_back(std::move(entry));
}

void FileWatcher::Unwatch(const std::string& directory) {
    std::lock_guard lock(_mutex);
    _entries.erase(std::remove_if(_entries.begin(), _entries.end(),
        [&](const WatchEntry& e) { return e.directory == directory; }),
        _entries.end());
}

void FileWatcher::AddFilter(const std::string& extension) {
    _filters.push_back(extension);
}

void FileWatcher::ClearFilters() {
    _filters.clear();
}

void FileWatcher::Start() {
    if (_running) return;
    _running = true;
    _thread = std::thread(&FileWatcher::WatchLoop, this);
}

void FileWatcher::Stop() {
    _running = false;
    if (_thread.joinable()) _thread.join();
}

std::vector<FileChangeEvent> FileWatcher::PollChanges() {
    std::lock_guard lock(_mutex);
    std::vector<FileChangeEvent> changes;
    std::swap(changes, _pendingChanges);
    return changes;
}

void FileWatcher::WatchLoop() {
    while (_running) {
        {
            std::lock_guard lock(_mutex);
            for (auto& entry : _entries) {
                auto oldTimestamps = entry.fileTimestamps;

                try {
                    namespace fs = std::filesystem;
                    std::unordered_map<std::string, fs::file_time_type> currentFiles;

                    auto dirIt = entry.recursive
                        ? fs::recursive_directory_iterator(entry.directory,
                              fs::directory_options::skip_permission_denied)
                        : fs::recursive_directory_iterator(); // placeholder

                    if (entry.recursive) {
                        for (auto& p : fs::recursive_directory_iterator(
                                entry.directory, fs::directory_options::skip_permission_denied)) {
                            if (!p.is_regular_file()) continue;
                            std::string path = p.path().string();
                            if (!PassesFilter(path)) continue;
                            currentFiles[path] = p.last_write_time();
                        }
                    } else {
                        for (auto& p : fs::directory_iterator(entry.directory)) {
                            if (!p.is_regular_file()) continue;
                            std::string path = p.path().string();
                            if (!PassesFilter(path)) continue;
                            currentFiles[path] = p.last_write_time();
                        }
                    }

                    // Check for new/modified files
                    for (auto& [path, time] : currentFiles) {
                        auto it = oldTimestamps.find(path);
                        if (it == oldTimestamps.end()) {
                            // New file
                            FileChangeEvent evt;
                            evt.path = path;
                            evt.action = FileAction::Created;
                            evt.timestamp = std::chrono::system_clock::now();
                            _pendingChanges.push_back(evt);
                            if (onFileChanged) onFileChanged(evt);
                        } else if (it->second != time) {
                            // Modified
                            FileChangeEvent evt;
                            evt.path = path;
                            evt.action = FileAction::Modified;
                            evt.timestamp = std::chrono::system_clock::now();
                            _pendingChanges.push_back(evt);
                            if (onFileChanged) onFileChanged(evt);
                        }
                    }

                    // Check for deleted files
                    for (auto& [path, time] : oldTimestamps) {
                        if (currentFiles.find(path) == currentFiles.end()) {
                            FileChangeEvent evt;
                            evt.path = path;
                            evt.action = FileAction::Deleted;
                            evt.timestamp = std::chrono::system_clock::now();
                            _pendingChanges.push_back(evt);
                            if (onFileChanged) onFileChanged(evt);
                        }
                    }

                    entry.fileTimestamps = std::move(currentFiles);
                } catch (const std::exception&) {
                    // Directory may have been removed
                }
            }
        }

        std::this_thread::sleep_for(_pollInterval);
    }
}

void FileWatcher::ScanDirectory(WatchEntry& entry) {
    namespace fs = std::filesystem;
    try {
        if (entry.recursive) {
            for (auto& p : fs::recursive_directory_iterator(
                    entry.directory, fs::directory_options::skip_permission_denied)) {
                if (!p.is_regular_file()) continue;
                std::string path = p.path().string();
                if (!PassesFilter(path)) continue;
                entry.fileTimestamps[path] = p.last_write_time();
            }
        } else {
            for (auto& p : fs::directory_iterator(entry.directory)) {
                if (!p.is_regular_file()) continue;
                std::string path = p.path().string();
                if (!PassesFilter(path)) continue;
                entry.fileTimestamps[path] = p.last_write_time();
            }
        }
    } catch (const std::exception&) {}
}

bool FileWatcher::PassesFilter(const std::string& path) const {
    if (_filters.empty()) return true;
    namespace fs = std::filesystem;
    auto ext = fs::path(path).extension().string();
    for (auto& f : _filters) {
        if (ext == f) return true;
    }
    return false;
}

// ============================================================================
// MODULE LOADER
// ============================================================================

ModuleLoader::~ModuleLoader() {
    UnloadAll();
}

ModuleHandle ModuleLoader::Load(const std::string& path, const std::string& name) {
    std::string moduleName = name.empty() ? std::filesystem::path(path).stem().string() : name;

    // Create a temporary copy to avoid file locking issues
    std::string tempPath = MakeTempCopy(path);
    if (tempPath.empty()) {
        if (onModuleError) onModuleError(moduleName, "Failed to create temp copy: " + path);
        return nullptr;
    }

    ModuleHandle handle = LoadDynamic(tempPath);
    if (!handle) {
        if (onModuleError) onModuleError(moduleName, "Failed to load module: " + path);
        return nullptr;
    }

    ModuleInfo info;
    info.name = moduleName;
    info.path = path;
    info.tempPath = tempPath;
    info.handle = handle;
    info.loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    info.version = 1;
    _modules.push_back(info);

    // Call init function if exists
    auto initFn = GetFunction<ModuleInitFn>(moduleName, "ModuleInit");
    if (initFn) initFn();

    if (onModuleLoaded) onModuleLoaded(moduleName);
    return handle;
}

void ModuleLoader::Unload(const std::string& name) {
    auto it = std::find_if(_modules.begin(), _modules.end(),
        [&](const ModuleInfo& m) { return m.name == name; });
    if (it == _modules.end()) return;

    // Call shutdown function if exists
    auto shutdownFn = GetFunction<ModuleShutdownFn>(name, "ModuleShutdown");
    if (shutdownFn) shutdownFn();

    UnloadDynamic(it->handle);

    // Clean up temp file
    if (!it->tempPath.empty()) {
        try { std::filesystem::remove(it->tempPath); } catch (...) {}
    }

    if (onModuleUnloaded) onModuleUnloaded(name);
    _modules.erase(it);
}

void ModuleLoader::UnloadAll() {
    while (!_modules.empty()) {
        Unload(_modules.back().name);
    }
}

bool ModuleLoader::Reload(const std::string& name) {
    auto* info = GetModuleInfo(name);
    if (!info) return false;

    std::string originalPath = info->path;
    u32 version = info->version;

    // Call reload callback before unload
    auto reloadFn = GetFunction<ModuleReloadFn>(name, "ModuleBeforeReload");
    if (reloadFn) reloadFn(nullptr);

    Unload(name);

    auto handle = Load(originalPath, name);
    if (!handle) return false;

    auto* newInfo = GetModuleInfo(name);
    if (newInfo) {
        newInfo->version = version + 1;

        auto afterReloadFn = GetFunction<ModuleReloadFn>(name, "ModuleAfterReload");
        if (afterReloadFn) afterReloadFn(nullptr);
    }

    if (onModuleReloaded) onModuleReloaded(name);
    return true;
}

void ModuleLoader::ReloadAll() {
    std::vector<std::string> names;
    for (auto& m : _modules) names.push_back(m.name);
    for (auto& name : names) Reload(name);
}

ModuleInfo* ModuleLoader::GetModuleInfo(const std::string& name) {
    for (auto& m : _modules) if (m.name == name) return &m;
    return nullptr;
}

ModuleHandle ModuleLoader::LoadDynamic(const std::string& path) {
#ifdef _WIN32
    return (ModuleHandle)LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void ModuleLoader::UnloadDynamic(ModuleHandle handle) {
    if (!handle) return;
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

void* ModuleLoader::GetSymbol(ModuleHandle handle, const std::string& name) {
    if (!handle) return nullptr;
#ifdef _WIN32
    return (void*)GetProcAddress((HMODULE)handle, name.c_str());
#else
    return dlsym(handle, name.c_str());
#endif
}

std::string ModuleLoader::MakeTempCopy(const std::string& path) {
    namespace fs = std::filesystem;
    try {
        auto stem = fs::path(path).stem().string();
        auto ext = fs::path(path).extension().string();
        auto tempDir = fs::temp_directory_path();
        auto tempPath = tempDir / (stem + "_hotreload_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ext);

        fs::copy_file(path, tempPath, fs::copy_options::overwrite_existing);
        return tempPath.string();
    } catch (const std::exception&) {
        return "";
    }
}

// ============================================================================
// HOT RELOAD MANAGER
// ============================================================================

void HotReloadManager::Initialize() {
    _watcher.Start();
}

void HotReloadManager::Shutdown() {
    _watcher.Stop();
    _moduleLoader.UnloadAll();
    _assets.clear();
}

u32 HotReloadManager::RegisterAsset(const std::string& path, ReloadableType type,
                                      std::function<void(const std::string&)> onReload,
                                      bool autoReload) {
    ReloadableAsset asset;
    asset.id = _nextAssetId++;
    asset.path = path;
    asset.type = type;
    asset.autoReload = autoReload;
    asset.onReload = std::move(onReload);
    asset.lastModified = 0;

    try {
        auto lwt = std::filesystem::last_write_time(path);
        asset.lastModified = std::chrono::duration_cast<std::chrono::milliseconds>(
            lwt.time_since_epoch()).count();
    } catch (...) {}

    _assets.push_back(std::move(asset));
    return _assets.back().id;
}

void HotReloadManager::UnregisterAsset(u32 id) {
    _assets.erase(std::remove_if(_assets.begin(), _assets.end(),
        [id](const ReloadableAsset& a) { return a.id == id; }), _assets.end());
}

void HotReloadManager::WatchDirectory(const std::string& dir, bool recursive) {
    _watcher.Watch(dir, recursive);
}

void HotReloadManager::AddFilter(const std::string& ext) {
    _watcher.AddFilter(ext);
}

void HotReloadManager::ReloadAsset(u32 id) {
    for (auto& asset : _assets) {
        if (asset.id == id) {
            try {
                if (asset.onReload) asset.onReload(asset.path);
                _reloadCount++;
                if (onAssetReloaded) onAssetReloaded(asset.path, asset.type);
            } catch (const std::exception& e) {
                if (asset.onReloadError) asset.onReloadError(e.what());
                if (onReloadError) onReloadError(asset.path, e.what());
            }
            return;
        }
    }
}

void HotReloadManager::ReloadAll() {
    for (auto& asset : _assets) {
        if (asset.onReload) {
            try {
                asset.onReload(asset.path);
                _reloadCount++;
            } catch (...) {}
        }
    }
}

void HotReloadManager::Update() {
    auto changes = _watcher.PollChanges();
    for (auto& change : changes) {
        HandleFileChange(change);
    }
}

void HotReloadManager::HandleFileChange(const FileChangeEvent& event) {
    if (event.action == FileAction::Deleted) return;

    auto now = std::chrono::steady_clock::now();

    // Debounce
    auto it = _lastReloadTime.find(event.path);
    if (it != _lastReloadTime.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (elapsed < _debounceTime) return;
    }
    _lastReloadTime[event.path] = now;

    // Find matching assets
    for (auto& asset : _assets) {
        if (asset.path == event.path && asset.autoReload) {
            try {
                if (asset.onReload) asset.onReload(asset.path);
                _reloadCount++;
                if (onAssetReloaded) onAssetReloaded(asset.path, asset.type);
            } catch (const std::exception& e) {
                if (asset.onReloadError) asset.onReloadError(e.what());
                if (onReloadError) onReloadError(asset.path, e.what());
            }
        }
    }

    // Check if it's a module
    namespace fs = std::filesystem;
    auto ext = fs::path(event.path).extension().string();
    if (ext == ".dll" || ext == ".so" || ext == ".dylib") {
        auto stem = fs::path(event.path).stem().string();
        auto* info = _moduleLoader.GetModuleInfo(stem);
        if (info) {
            _moduleLoader.Reload(stem);
        }
    }
}

ReloadableType HotReloadManager::DetectType(const std::string& path) const {
    namespace fs = std::filesystem;
    auto ext = fs::path(path).extension().string();

    if (ext == ".lua" || ext == ".py" || ext == ".js") return ReloadableType::Script;
    if (ext == ".hlsl" || ext == ".glsl" || ext == ".vert" || ext == ".frag") return ReloadableType::Shader;
    if (ext == ".png" || ext == ".jpg" || ext == ".bmp" || ext == ".tga") return ReloadableType::Texture;
    if (ext == ".ttf" || ext == ".otf") return ReloadableType::Font;
    if (ext == ".json" || ext == ".xml" || ext == ".yaml") return ReloadableType::Config;
    if (ext == ".dll" || ext == ".so" || ext == ".dylib") return ReloadableType::Module;
    if (ext == ".layout" || ext == ".ui") return ReloadableType::Layout;

    return ReloadableType::Config; // Default
}

} // namespace ace
