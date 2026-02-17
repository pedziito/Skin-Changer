/**
 * ACE Engine — Hot Reload System
 *
 * File watching, DLL/SO module loading, resource hot-reload,
 * and live script re-execution.
 */

#pragma once
#include "../core/types.h"
#include "../core/event.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>

namespace ace {

// ============================================================================
// FILE WATCHER
// ============================================================================

enum class FileAction : u8 {
    Created,
    Modified,
    Deleted,
    Renamed
};

struct FileChangeEvent {
    std::string path;
    std::string oldPath;    // For rename events
    FileAction  action;
    std::chrono::system_clock::time_point timestamp;
};

class FileWatcher {
public:
    FileWatcher() = default;
    ~FileWatcher() { Stop(); }

    // Add a directory to watch (recursive)
    void Watch(const std::string& directory, bool recursive = true);
    void Unwatch(const std::string& directory);

    // Add file extension filter (e.g., ".lua", ".json", ".hlsl")
    void AddFilter(const std::string& extension);
    void ClearFilters();

    // Polling interval
    void SetInterval(std::chrono::milliseconds ms) { _pollInterval = ms; }

    // Start/stop background watching thread
    void Start();
    void Stop();
    bool IsRunning() const { return _running; }

    // Poll for changes (call from main thread to get events)
    std::vector<FileChangeEvent> PollChanges();

    // Callback (called from watcher thread — use with caution)
    std::function<void(const FileChangeEvent&)> onFileChanged;

private:
    struct WatchEntry {
        std::string directory;
        bool recursive;
        std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps;
    };

    std::vector<WatchEntry> _entries;
    std::vector<std::string> _filters; // Extension filters
    std::chrono::milliseconds _pollInterval{200};

    std::atomic<bool> _running{false};
    std::thread _thread;
    std::mutex _mutex;
    std::vector<FileChangeEvent> _pendingChanges;

    void WatchLoop();
    void ScanDirectory(WatchEntry& entry);
    bool PassesFilter(const std::string& path) const;
};

// ============================================================================
// MODULE LOADER (DLL/SO hot reloading)
// ============================================================================

using ModuleHandle = void*;

struct ModuleInfo {
    std::string     name;
    std::string     path;
    std::string     tempPath;   // Copy for hot-reload (avoids file lock)
    ModuleHandle    handle = nullptr;
    u64             loadTime = 0;
    u32             version = 0;
};

// Function signature for module entry points
using ModuleInitFn    = void(*)();
using ModuleShutdownFn = void(*)();
using ModuleReloadFn  = void(*)(void* context);

class ModuleLoader {
public:
    ModuleLoader() = default;
    ~ModuleLoader();

    // Load a DLL/SO module
    ModuleHandle Load(const std::string& path, const std::string& name = "");
    void Unload(const std::string& name);
    void UnloadAll();

    // Reload (unload + load new copy)
    bool Reload(const std::string& name);
    void ReloadAll();

    // Get function pointer from module
    template<typename T>
    T GetFunction(const std::string& moduleName, const std::string& funcName) {
        auto* info = GetModuleInfo(moduleName);
        if (!info || !info->handle) return nullptr;
        return reinterpret_cast<T>(GetSymbol(info->handle, funcName));
    }

    // Module info
    ModuleInfo* GetModuleInfo(const std::string& name);
    const std::vector<ModuleInfo>& GetModules() const { return _modules; }

    // Callbacks
    std::function<void(const std::string&)> onModuleLoaded;
    std::function<void(const std::string&)> onModuleUnloaded;
    std::function<void(const std::string&)> onModuleReloaded;
    std::function<void(const std::string&, const std::string&)> onModuleError;

private:
    std::vector<ModuleInfo> _modules;

    ModuleHandle LoadDynamic(const std::string& path);
    void UnloadDynamic(ModuleHandle handle);
    void* GetSymbol(ModuleHandle handle, const std::string& name);
    std::string MakeTempCopy(const std::string& path);
};

// ============================================================================
// HOT RELOAD MANAGER (orchestrates file watching + module/resource reloading)
// ============================================================================

enum class ReloadableType : u8 {
    Script,
    Shader,
    Texture,
    Font,
    Config,    // JSON/XML config files
    Module,    // DLL/SO
    Layout     // UI layout files
};

struct ReloadableAsset {
    u32             id;
    std::string     path;
    ReloadableType  type;
    u64             lastModified;
    bool            autoReload;
    std::function<void(const std::string&)> onReload;     // Called when reloaded
    std::function<void(const std::string&)> onReloadError;
};

class HotReloadManager {
public:
    static HotReloadManager& Instance() {
        static HotReloadManager s;
        return s;
    }

    void Initialize();
    void Shutdown();

    // Register assets for hot-reloading
    u32 RegisterAsset(const std::string& path, ReloadableType type,
                      std::function<void(const std::string&)> onReload,
                      bool autoReload = true);

    void UnregisterAsset(u32 id);

    // Watch directories
    void WatchDirectory(const std::string& dir, bool recursive = true);
    void AddFilter(const std::string& ext);

    // Manual reload
    void ReloadAsset(u32 id);
    void ReloadAll();

    // Call from main loop
    void Update();

    // Module support
    ModuleLoader& GetModuleLoader() { return _moduleLoader; }

    // Stats
    u32 GetReloadCount() const { return _reloadCount; }
    u32 GetAssetCount() const { return (u32)_assets.size(); }

    // Callbacks
    std::function<void(const std::string&, ReloadableType)> onAssetReloaded;
    std::function<void(const std::string&, const std::string&)> onReloadError;

private:
    HotReloadManager() = default;

    FileWatcher  _watcher;
    ModuleLoader _moduleLoader;

    std::vector<ReloadableAsset> _assets;
    u32 _nextAssetId = 1;
    u32 _reloadCount = 0;

    // Debounce: avoid reloading multiple times for rapid saves
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> _lastReloadTime;
    std::chrono::milliseconds _debounceTime{300};

    void HandleFileChange(const FileChangeEvent& event);
    ReloadableType DetectType(const std::string& path) const;
};

} // namespace ace
