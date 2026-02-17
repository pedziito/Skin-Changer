/**
 * ACE Engine — Input System
 * Unified keyboard, mouse, and gamepad input abstraction.
 * Tracks current and previous frame state for pressed/released detection.
 * Platform-agnostic — feed events from Win32, SDL, GLFW, etc.
 */

#pragma once

#include "../core/types.h"
#include "../core/event.h"
#include <array>
#include <bitset>
#include <string>

namespace ace {

// ============================================================================
// KEY CODES — Platform-agnostic key identifiers
// ============================================================================
enum class Key : i32 {
    None = 0,
    // Letters
    A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G',
    H = 'H', I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N',
    O = 'O', P = 'P', Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U',
    V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z',
    // Numbers
    Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
    Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',
    // Function keys
    F1 = 256, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    // Modifiers
    LeftShift = 300, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
    // Navigation
    Escape = 350, Enter, Tab, Backspace, Delete, Insert,
    Home, End, PageUp, PageDown,
    Left, Right, Up, Down,
    // Special
    Space = 400, CapsLock, NumLock, ScrollLock, PrintScreen, Pause,
    // Punctuation
    Comma = 450, Period, Slash, Semicolon, Apostrophe,
    LeftBracket, RightBracket, Backslash, GraveAccent, Minus, Equals,

    MAX_KEYS = 512
};

enum class MouseButton : i32 {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    MAX_BUTTONS = 8
};

// ============================================================================
// INPUT STATE — Current frame snapshot
// ============================================================================
struct InputState {
    // Keyboard
    std::bitset<static_cast<size_t>(Key::MAX_KEYS)> keysDown;
    std::bitset<static_cast<size_t>(Key::MAX_KEYS)> keysPrev;

    // Mouse
    std::bitset<static_cast<size_t>(MouseButton::MAX_BUTTONS)> mouseDown;
    std::bitset<static_cast<size_t>(MouseButton::MAX_BUTTONS)> mousePrev;
    Vec2 mousePos;
    Vec2 mouseDelta;
    f32  scrollDelta{0};

    // Text input buffer
    std::string textInput;

    // Modifiers
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
};

// ============================================================================
// INPUT SYSTEM — Main input manager
// ============================================================================
class InputSystem {
public:
    InputSystem() = default;

    // --- Query (call during frame) ---

    bool IsKeyDown(Key key) const {
        return _state.keysDown[static_cast<size_t>(key)];
    }

    bool IsKeyPressed(Key key) const {
        size_t idx = static_cast<size_t>(key);
        return _state.keysDown[idx] && !_state.keysPrev[idx];
    }

    bool IsKeyReleased(Key key) const {
        size_t idx = static_cast<size_t>(key);
        return !_state.keysDown[idx] && _state.keysPrev[idx];
    }

    bool IsMouseDown(MouseButton btn) const {
        return _state.mouseDown[static_cast<size_t>(btn)];
    }

    bool IsMousePressed(MouseButton btn) const {
        size_t idx = static_cast<size_t>(btn);
        return _state.mouseDown[idx] && !_state.mousePrev[idx];
    }

    bool IsMouseReleased(MouseButton btn) const {
        size_t idx = static_cast<size_t>(btn);
        return !_state.mouseDown[idx] && _state.mousePrev[idx];
    }

    bool IsMouseDoubleClick(MouseButton btn) const {
        return IsMousePressed(btn) && _doubleClickDetected[static_cast<size_t>(btn)];
    }

    Vec2 MousePos()   const { return _state.mousePos; }
    Vec2 MouseDelta() const { return _state.mouseDelta; }
    f32  ScrollDelta() const { return _state.scrollDelta; }

    bool Shift() const { return _state.shift; }
    bool Ctrl()  const { return _state.ctrl; }
    bool Alt()   const { return _state.alt; }

    std::string_view TextInput() const { return _state.textInput; }

    const InputState& GetState() const { return _state; }

    // --- Feed events (call from platform layer) ---

    void OnKeyDown(Key key) {
        _state.keysDown.set(static_cast<size_t>(key));
        UpdateModifiers(key, true);
    }

    void OnKeyUp(Key key) {
        _state.keysDown.reset(static_cast<size_t>(key));
        UpdateModifiers(key, false);
    }

    void OnMouseMove(f32 x, f32 y) {
        Vec2 newPos{x, y};
        _state.mouseDelta = newPos - _state.mousePos;
        _state.mousePos = newPos;
    }

    void OnMouseDown(MouseButton btn) {
        size_t idx = static_cast<size_t>(btn);
        _state.mouseDown.set(idx);

        // Double-click detection
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - _lastClickTime[idx]).count();
        _doubleClickDetected[idx] = (elapsed < 400); // 400ms threshold
        _lastClickTime[idx] = now;
    }

    void OnMouseUp(MouseButton btn) {
        _state.mouseDown.reset(static_cast<size_t>(btn));
    }

    void OnMouseScroll(f32 delta) {
        _state.scrollDelta += delta;
    }

    void OnTextInput(char c) {
        _state.textInput += c;
    }

    void OnTextInputUTF32(u32 codepoint) {
        // Simple ASCII passthrough, extend for full UTF-8
        if (codepoint < 128) _state.textInput += static_cast<char>(codepoint);
    }

    // --- Frame management ---

    void BeginFrame() {
        _state.keysPrev = _state.keysDown;
        _state.mousePrev = _state.mouseDown;
        _state.mouseDelta = {};
        _state.scrollDelta = 0;
        _state.textInput.clear();
        std::fill(std::begin(_doubleClickDetected), std::end(_doubleClickDetected), false);
    }

    /**
     * Emit events to an EventBus (optional integration).
     */
    void EmitEvents(EventBus& bus) const {
        // Could iterate changed keys and emit KeyEvent, MouseMoveEvent, etc.
        // Left as extension point for per-project needs.
    }

private:
    void UpdateModifiers(Key key, bool down) {
        if (key == Key::LeftShift || key == Key::RightShift) _state.shift = down;
        if (key == Key::LeftCtrl || key == Key::RightCtrl)   _state.ctrl = down;
        if (key == Key::LeftAlt || key == Key::RightAlt)     _state.alt = down;
    }

    InputState _state;

    // Double-click tracking
    std::array<std::chrono::steady_clock::time_point,
               static_cast<size_t>(MouseButton::MAX_BUTTONS)> _lastClickTime;
    std::array<bool, static_cast<size_t>(MouseButton::MAX_BUTTONS)> _doubleClickDetected{};
};

} // namespace ace
