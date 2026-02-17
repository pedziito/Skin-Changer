/**
 * ACE Engine — Scripting Engine Implementation
 *
 * Provides stub implementations for Lua and Python engines.
 * Full integration requires linking against Lua 5.4+ / CPython 3.x.
 * The architecture supports drop-in replacement once libraries are available.
 */

#include "script_engine.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace ace {

// ============================================================================
// LUA ENGINE
// ============================================================================

bool LuaEngine::Initialize() {
    if (_initialized) return true;

    // NOTE: Full implementation requires linking lua54.lib / liblua.a
    // This is a structural implementation showing the API surface.
    // When Lua is linked:
    //   _luaState = luaL_newstate();
    //   luaL_openlibs(_luaState);
    //   if (_sandboxed) SetupSandbox();

    _initialized = true;

    // Register all pre-registered functions
    for (auto& [name, func] : _registeredFunctions) {
        // lua_pushcfunction(_luaState, wrapper);
        // lua_setglobal(_luaState, name.c_str());
    }

    for (auto& mod : _registeredModules) {
        // Create table, register functions
    }

    if (onPrint) onPrint("[LuaEngine] Initialized (stub mode — link lua54 for full support)");
    return true;
}

void LuaEngine::Shutdown() {
    if (!_initialized) return;
    // if (_luaState) lua_close(_luaState);
    _luaState = nullptr;
    _initialized = false;
}

bool LuaEngine::Execute(const std::string& code, const std::string& sourceName) {
    if (!_initialized) {
        ReportError("Engine not initialized", sourceName);
        return false;
    }

    // Stub: parse and validate basic syntax
    // Full: luaL_loadbuffer + lua_pcall
    if (code.empty()) return true;

    // Simulate execution for testing
    if (onPrint) {
        onPrint("[Lua] Executing: " + sourceName);
    }

    return true;
}

bool LuaEngine::ExecuteFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        ReportError("Cannot open file: " + filePath, filePath);
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return Execute(ss.str(), filePath);
}

void LuaEngine::SetGlobal(const std::string& name, ScriptValue value) {
    if (!_initialized) return;

    // Full implementation:
    // PushValue(_luaState, value);
    // lua_setglobal(_luaState, name.c_str());
}

ScriptValue LuaEngine::GetGlobal(const std::string& name) {
    if (!_initialized) return ScriptValue::Nil();

    // Full: lua_getglobal + ToValue
    return ScriptValue::Nil();
}

ScriptValue LuaEngine::CallFunction(const std::string& name,
                                     const std::vector<ScriptValue>& args) {
    if (!_initialized) {
        ReportError("Engine not initialized");
        return ScriptValue::Nil();
    }

    // Check registered C++ functions first
    auto it = _registeredFunctions.find(name);
    if (it != _registeredFunctions.end()) {
        return it->second(args);
    }

    // Full: lua_getglobal + push args + lua_pcall + read return
    return ScriptValue::Nil();
}

void LuaEngine::RegisterModule(const ScriptModule& module) {
    _registeredModules.push_back(module);

    if (!_initialized) return;

    // Full: create module table, register each function
    for (auto& binding : module.functions) {
        _registeredFunctions[module.name + "." + binding.name] = binding.function;
    }
}

void LuaEngine::RegisterFunction(const std::string& name, ScriptFunction func,
                                   const std::string& doc) {
    _registeredFunctions[name] = std::move(func);

    if (!_initialized) return;

    // Full: wrap as lua_CFunction, push to global
}

void LuaEngine::SetupSandbox() {
    // Disable dangerous functions: os.execute, io.*, loadfile, dofile (from untrusted)
    // Set memory allocation hook for limit enforcement
    const char* sandboxCode = R"(
        -- Sandbox: restrict dangerous APIs
        os.execute = nil
        os.exit = nil
        io = nil
        loadfile = function() error("loadfile disabled in sandbox") end
        dofile = function() error("dofile disabled in sandbox") end
    )";
    (void)sandboxCode;
    // Execute(sandboxCode, "<sandbox>");
}

void LuaEngine::ReportError(const std::string& msg, const std::string& source, i32 line) {
    _hasError = true;
    _lastError = {msg, source, line, ""};
    if (onError) onError(_lastError);
}

// ============================================================================
// PYTHON ENGINE
// ============================================================================

bool PythonEngine::Initialize() {
    if (_initialized) return true;

    // NOTE: Full implementation requires linking python3.x
    // Py_Initialize();
    // Set up sys.path, import modules, etc.

    _initialized = true;
    return true;
}

void PythonEngine::Shutdown() {
    if (!_initialized) return;
    // Py_Finalize();
    _initialized = false;
}

bool PythonEngine::Execute(const std::string& code, const std::string& sourceName) {
    if (!_initialized) {
        ReportError("Engine not initialized", sourceName);
        return false;
    }

    // Full: PyRun_SimpleString or compile + eval
    return true;
}

bool PythonEngine::ExecuteFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        ReportError("Cannot open file: " + filePath, filePath);
        return false;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return Execute(ss.str(), filePath);
}

void PythonEngine::SetGlobal(const std::string& name, ScriptValue value) {
    // PyObject_SetAttrString(PyImport_AddModule("__main__"), name, ToPyObject(value));
}

ScriptValue PythonEngine::GetGlobal(const std::string& name) {
    return ScriptValue::Nil();
}

ScriptValue PythonEngine::CallFunction(const std::string& name,
                                        const std::vector<ScriptValue>& args) {
    auto it = _registeredFunctions.find(name);
    if (it != _registeredFunctions.end()) {
        return it->second(args);
    }
    return ScriptValue::Nil();
}

void PythonEngine::RegisterModule(const ScriptModule& module) {
    _registeredModules.push_back(module);
    for (auto& binding : module.functions) {
        _registeredFunctions[module.name + "." + binding.name] = binding.function;
    }
}

void PythonEngine::RegisterFunction(const std::string& name, ScriptFunction func,
                                     const std::string& doc) {
    _registeredFunctions[name] = std::move(func);
}

void PythonEngine::ReportError(const std::string& msg, const std::string& source, i32 line) {
    _hasError = true;
    _lastError = {msg, source, line, ""};
    if (onError) onError(_lastError);
}

// ============================================================================
// SCRIPT MANAGER
// ============================================================================

bool ScriptManager::Initialize(ScriptLanguage defaultLang) {
    // Create engines
    auto lua = std::make_unique<LuaEngine>();
    lua->Initialize();
    _engines[ScriptLanguage::Lua] = std::move(lua);

    auto python = std::make_unique<PythonEngine>();
    python->Initialize();
    _engines[ScriptLanguage::Python] = std::move(python);

    _defaultEngine = _engines[defaultLang].get();

    // Register built-in ACE bindings
    RegisterACEBindings();
    return true;
}

void ScriptManager::Shutdown() {
    for (auto& [lang, engine] : _engines) engine->Shutdown();
    _engines.clear();
    _defaultEngine = nullptr;
}

IScriptEngine* ScriptManager::GetEngine(ScriptLanguage lang) {
    auto it = _engines.find(lang);
    return (it != _engines.end()) ? it->second.get() : nullptr;
}

void ScriptManager::RegisterGlobal(const std::string& name, ScriptFunction func,
                                    const std::string& doc) {
    for (auto& [lang, engine] : _engines) {
        engine->RegisterFunction(name, func, doc);
    }
}

void ScriptManager::RegisterModule(const ScriptModule& module) {
    for (auto& [lang, engine] : _engines) {
        engine->RegisterModule(module);
    }
}

bool ScriptManager::ExecuteFile(const std::string& filePath) {
    namespace fs = std::filesystem;
    auto ext = fs::path(filePath).extension().string();

    IScriptEngine* engine = nullptr;
    if (ext == ".lua") engine = GetEngine(ScriptLanguage::Lua);
    else if (ext == ".py") engine = GetEngine(ScriptLanguage::Python);
    else engine = _defaultEngine;

    return engine ? engine->ExecuteFile(filePath) : false;
}

void ScriptManager::RegisterACEBindings() {
    // ACE core module
    ScriptModule aceModule;
    aceModule.name = "ace";

    aceModule.functions.push_back({"log", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        // Console log
        return ScriptValue::Nil();
    }, "Log a message to console"});

    aceModule.functions.push_back({"vec2", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        if (args.size() >= 2) {
            // Return a Vec2 userdata
        }
        return ScriptValue::Nil();
    }, "Create a Vec2"});

    aceModule.functions.push_back({"color", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        if (args.size() >= 3) {
            // Return a Color userdata
        }
        return ScriptValue::Nil();
    }, "Create a Color(r, g, b [,a])"});

    RegisterModule(aceModule);

    // ACE UI module
    ScriptModule uiModule;
    uiModule.name = "ace.ui";

    uiModule.functions.push_back({"button", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        // Create a button widget
        return ScriptValue::Nil();
    }, "Create a UI button"});

    uiModule.functions.push_back({"panel", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        return ScriptValue::Nil();
    }, "Create a UI panel"});

    uiModule.functions.push_back({"slider", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        return ScriptValue::Nil();
    }, "Create a slider widget"});

    RegisterModule(uiModule);

    // ACE input module
    ScriptModule inputModule;
    inputModule.name = "ace.input";

    inputModule.functions.push_back({"is_key_down", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        return ScriptValue(false);
    }, "Check if a key is down"});

    inputModule.functions.push_back({"mouse_pos", [](const std::vector<ScriptValue>& args) -> ScriptValue {
        return ScriptValue::Nil();
    }, "Get mouse position as Vec2"});

    RegisterModule(inputModule);
}

} // namespace ace
