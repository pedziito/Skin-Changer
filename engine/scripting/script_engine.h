/**
 * ACE Engine — Scripting Engine
 *
 * Embeddable Lua/Python scripting interface with sandboxing,
 * binding helpers, coroutine support, and error reporting.
 */

#pragma once
#include "../core/types.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>

namespace ace {

// ============================================================================
// SCRIPT VALUE (generic typed value for script <-> C++ interchange)
// ============================================================================
struct ScriptValue {
    enum class Type : u8 { Nil, Bool, Int, Float, String, Table, Function, UserData };

    Type type = Type::Nil;
    std::variant<
        std::monostate,
        bool,
        i64,
        f64,
        std::string,
        std::vector<std::pair<ScriptValue, ScriptValue>>,
        std::function<ScriptValue(const std::vector<ScriptValue>&)>,
        void*
    > data;

    ScriptValue() = default;
    ScriptValue(bool v)         : type(Type::Bool),   data(v) {}
    ScriptValue(i32 v)          : type(Type::Int),    data(static_cast<i64>(v)) {}
    ScriptValue(i64 v)          : type(Type::Int),    data(v) {}
    ScriptValue(f32 v)          : type(Type::Float),  data(static_cast<f64>(v)) {}
    ScriptValue(f64 v)          : type(Type::Float),  data(v) {}
    ScriptValue(const char* v)  : type(Type::String), data(std::string(v)) {}
    ScriptValue(std::string v)  : type(Type::String), data(std::move(v)) {}

    bool        AsBool()   const { return std::get<bool>(data); }
    i64         AsInt()    const { return std::get<i64>(data); }
    f64         AsFloat()  const { return std::get<f64>(data); }
    std::string AsString() const { return std::get<std::string>(data); }
    bool        IsNil()    const { return type == Type::Nil; }

    static ScriptValue Nil() { return {}; }
};

// ============================================================================
// SCRIPT ERROR
// ============================================================================
struct ScriptError {
    std::string message;
    std::string source;     // Script file/chunk name
    i32         line = -1;
    std::string stackTrace;
};

// ============================================================================
// SCRIPT LANGUAGE
// ============================================================================
enum class ScriptLanguage : u8 {
    Lua,
    Python,
    JavaScript // Future
};

// ============================================================================
// SCRIPT FUNCTION BINDING
// ============================================================================
using ScriptFunction = std::function<ScriptValue(const std::vector<ScriptValue>&)>;

struct ScriptBinding {
    std::string   name;
    ScriptFunction function;
    std::string   doc;           // Documentation string
    i32           minArgs = 0;
    i32           maxArgs = -1;  // -1 = unlimited
};

struct ScriptModule {
    std::string name;
    std::vector<ScriptBinding> functions;
    std::unordered_map<std::string, ScriptValue> constants;
};

// ============================================================================
// ISCRIPT ENGINE (abstract interface)
// ============================================================================
class IScriptEngine {
public:
    virtual ~IScriptEngine() = default;

    virtual ScriptLanguage GetLanguage() const = 0;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    // Execute code
    virtual bool Execute(const std::string& code, const std::string& sourceName = "<script>") = 0;
    virtual bool ExecuteFile(const std::string& filePath) = 0;

    // Global variables
    virtual void SetGlobal(const std::string& name, ScriptValue value) = 0;
    virtual ScriptValue GetGlobal(const std::string& name) = 0;

    // Function calls
    virtual ScriptValue CallFunction(const std::string& name,
                                      const std::vector<ScriptValue>& args = {}) = 0;

    // Module registration
    virtual void RegisterModule(const ScriptModule& module) = 0;
    virtual void RegisterFunction(const std::string& name, ScriptFunction func,
                                   const std::string& doc = "") = 0;

    // Error handling
    virtual bool HasError() const = 0;
    virtual ScriptError GetLastError() const = 0;
    virtual void ClearError() = 0;

    // Callbacks
    std::function<void(const ScriptError&)> onError;
    std::function<void(const std::string&)> onPrint; // Script print() output
};

// ============================================================================
// LUA SCRIPT ENGINE
// ============================================================================
class LuaEngine : public IScriptEngine {
public:
    LuaEngine() = default;
    ~LuaEngine() override { Shutdown(); }

    ScriptLanguage GetLanguage() const override { return ScriptLanguage::Lua; }
    bool Initialize() override;
    void Shutdown() override;

    bool Execute(const std::string& code, const std::string& sourceName) override;
    bool ExecuteFile(const std::string& filePath) override;

    void SetGlobal(const std::string& name, ScriptValue value) override;
    ScriptValue GetGlobal(const std::string& name) override;

    ScriptValue CallFunction(const std::string& name,
                              const std::vector<ScriptValue>& args) override;

    void RegisterModule(const ScriptModule& module) override;
    void RegisterFunction(const std::string& name, ScriptFunction func,
                           const std::string& doc) override;

    bool HasError() const override { return _hasError; }
    ScriptError GetLastError() const override { return _lastError; }
    void ClearError() override { _hasError = false; _lastError = {}; }

    // Lua-specific
    void SetSandboxed(bool v) { _sandboxed = v; }
    void SetMemoryLimit(size_t bytes) { _memLimit = bytes; }
    size_t GetMemoryUsage() const { return _memUsage; }

private:
    void* _luaState = nullptr; // lua_State*
    bool  _initialized = false;
    bool  _sandboxed = true;
    bool  _hasError = false;
    ScriptError _lastError;
    size_t _memLimit = 64 * 1024 * 1024; // 64 MB
    size_t _memUsage = 0;

    std::unordered_map<std::string, ScriptFunction> _registeredFunctions;
    std::vector<ScriptModule> _registeredModules;

    void SetupSandbox();
    void ReportError(const std::string& msg, const std::string& source = "", i32 line = -1);
};

// ============================================================================
// PYTHON SCRIPT ENGINE
// ============================================================================
class PythonEngine : public IScriptEngine {
public:
    PythonEngine() = default;
    ~PythonEngine() override { Shutdown(); }

    ScriptLanguage GetLanguage() const override { return ScriptLanguage::Python; }
    bool Initialize() override;
    void Shutdown() override;

    bool Execute(const std::string& code, const std::string& sourceName) override;
    bool ExecuteFile(const std::string& filePath) override;

    void SetGlobal(const std::string& name, ScriptValue value) override;
    ScriptValue GetGlobal(const std::string& name) override;

    ScriptValue CallFunction(const std::string& name,
                              const std::vector<ScriptValue>& args) override;

    void RegisterModule(const ScriptModule& module) override;
    void RegisterFunction(const std::string& name, ScriptFunction func,
                           const std::string& doc) override;

    bool HasError() const override { return _hasError; }
    ScriptError GetLastError() const override { return _lastError; }
    void ClearError() override { _hasError = false; _lastError = {}; }

private:
    bool _initialized = false;
    bool _hasError = false;
    ScriptError _lastError;

    std::unordered_map<std::string, ScriptFunction> _registeredFunctions;
    std::vector<ScriptModule> _registeredModules;

    void ReportError(const std::string& msg, const std::string& source = "", i32 line = -1);
};

// ============================================================================
// SCRIPT MANAGER (manages multiple engines, dispatches by language)
// ============================================================================
class ScriptManager {
public:
    static ScriptManager& Instance() {
        static ScriptManager s;
        return s;
    }

    bool Initialize(ScriptLanguage defaultLang = ScriptLanguage::Lua);
    void Shutdown();

    IScriptEngine* GetEngine(ScriptLanguage lang);
    IScriptEngine* GetDefaultEngine() { return _defaultEngine; }

    // Convenience: register a C++ function in all engines
    void RegisterGlobal(const std::string& name, ScriptFunction func, const std::string& doc = "");
    void RegisterModule(const ScriptModule& module);

    // Execute in default engine
    bool Execute(const std::string& code) {
        return _defaultEngine ? _defaultEngine->Execute(code) : false;
    }

    // Auto-detect language by file extension and execute
    bool ExecuteFile(const std::string& filePath);

    // Create ACE engine bindings (UI, input, render, etc.)
    void RegisterACEBindings();

private:
    ScriptManager() = default;
    std::unordered_map<ScriptLanguage, std::unique_ptr<IScriptEngine>> _engines;
    IScriptEngine* _defaultEngine = nullptr;
};

} // namespace ace
