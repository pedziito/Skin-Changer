/**
 * ACE Engine — Event System
 * Type-safe event bus with delegate subscriptions.
 * Supports scoped listeners, priority ordering, and event consumption.
 *
 * Usage:
 *   struct ClickEvent { Vec2 pos; int button; };
 *
 *   EventBus bus;
 *   auto id = bus.Subscribe<ClickEvent>([](const ClickEvent& e) {
 *       printf("Click at %.0f, %.0f\n", e.pos.x, e.pos.y);
 *   });
 *   bus.Emit(ClickEvent{{100, 200}, 0});
 *   bus.Unsubscribe<ClickEvent>(id);
 */

#pragma once

#include "types.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <algorithm>
#include <mutex>
#include <memory>
#include <any>

namespace ace {

// ============================================================================
// DELEGATE — Lightweight callable wrapper
// ============================================================================
using ListenerID = u64;

namespace detail {
    inline std::atomic<ListenerID> g_nextListenerID{1};

    struct ListenerBase {
        ListenerID id;
        i32        priority;

        virtual ~ListenerBase() = default;
        virtual void Invoke(const void* eventData) = 0;
    };

    template<typename Event>
    struct TypedListener : ListenerBase {
        std::function<void(const Event&)> callback;

        void Invoke(const void* eventData) override {
            callback(*static_cast<const Event*>(eventData));
        }
    };

    struct ChannelBase {
        virtual ~ChannelBase() = default;
        virtual void RemoveListener(ListenerID id) = 0;
    };

    template<typename Event>
    struct Channel : ChannelBase {
        std::vector<std::unique_ptr<TypedListener<Event>>> listeners;
        bool dirty = false;

        void Sort() {
            if (!dirty) return;
            std::sort(listeners.begin(), listeners.end(),
                [](const auto& a, const auto& b) { return a->priority > b->priority; });
            dirty = false;
        }

        void RemoveListener(ListenerID id) override {
            listeners.erase(
                std::remove_if(listeners.begin(), listeners.end(),
                    [id](const auto& l) { return l->id == id; }),
                listeners.end());
        }
    };
}

// ============================================================================
// EVENT BUS — Central pub/sub dispatcher
// ============================================================================
class EventBus {
public:
    /**
     * Subscribe to an event type with optional priority (higher = called first).
     * Returns a ListenerID for later unsubscription.
     */
    template<typename Event>
    ListenerID Subscribe(std::function<void(const Event&)> callback, i32 priority = 0) {
        auto& channel = GetOrCreateChannel<Event>();
        auto listener = std::make_unique<detail::TypedListener<Event>>();
        listener->id = detail::g_nextListenerID.fetch_add(1);
        listener->priority = priority;
        listener->callback = std::move(callback);
        ListenerID id = listener->id;
        channel.listeners.push_back(std::move(listener));
        channel.dirty = true;
        return id;
    }

    /**
     * Unsubscribe a specific listener by ID.
     */
    template<typename Event>
    void Unsubscribe(ListenerID id) {
        auto idx = std::type_index(typeid(Event));
        auto it = _channels.find(idx);
        if (it != _channels.end()) it->second->RemoveListener(id);
    }

    /**
     * Convenience: unsubscribe from all channels (slower, scans all).
     */
    void UnsubscribeAll(ListenerID id) {
        for (auto& [_, channel] : _channels)
            channel->RemoveListener(id);
    }

    /**
     * Emit an event to all subscribers on that channel.
     */
    template<typename Event>
    void Emit(const Event& event) {
        auto idx = std::type_index(typeid(Event));
        auto it = _channels.find(idx);
        if (it == _channels.end()) return;

        auto& channel = static_cast<detail::Channel<Event>&>(*it->second);
        channel.Sort();

        for (auto& listener : channel.listeners)
            listener->Invoke(&event);
    }

    /**
     * Emit and allow consumers to stop propagation (returns true if consumed).
     */
    template<typename Event>
    bool EmitConsumable(const Event& event) {
        auto idx = std::type_index(typeid(Event));
        auto it = _channels.find(idx);
        if (it == _channels.end()) return false;

        auto& channel = static_cast<detail::Channel<Event>&>(*it->second);
        channel.Sort();

        for (auto& listener : channel.listeners) {
            listener->Invoke(&event);
            if constexpr (requires { event.consumed; }) {
                if (event.consumed) return true;
            }
        }
        return false;
    }

    void Clear() { _channels.clear(); }

private:
    template<typename Event>
    detail::Channel<Event>& GetOrCreateChannel() {
        auto idx = std::type_index(typeid(Event));
        auto it = _channels.find(idx);
        if (it == _channels.end()) {
            auto channel = std::make_unique<detail::Channel<Event>>();
            auto& ref = *channel;
            _channels[idx] = std::move(channel);
            return ref;
        }
        return static_cast<detail::Channel<Event>&>(*it->second);
    }

    std::unordered_map<std::type_index, std::unique_ptr<detail::ChannelBase>> _channels;
};

// ============================================================================
// SCOPED LISTENER — RAII auto-unsubscribe
// ============================================================================
class ScopedListener {
public:
    ScopedListener() = default;

    template<typename Event>
    ScopedListener(EventBus& bus, std::function<void(const Event&)> cb, i32 priority = 0)
        : _bus(&bus)
    {
        _id = bus.Subscribe<Event>(std::move(cb), priority);
    }

    ~ScopedListener() { Unbind(); }

    ScopedListener(ScopedListener&& o) noexcept : _bus(o._bus), _id(o._id) {
        o._bus = nullptr; o._id = 0;
    }
    ScopedListener& operator=(ScopedListener&& o) noexcept {
        Unbind();
        _bus = o._bus; _id = o._id;
        o._bus = nullptr; o._id = 0;
        return *this;
    }

    ScopedListener(const ScopedListener&) = delete;
    ScopedListener& operator=(const ScopedListener&) = delete;

    void Unbind() {
        if (_bus && _id) { _bus->UnsubscribeAll(_id); _bus = nullptr; _id = 0; }
    }

private:
    EventBus*  _bus{nullptr};
    ListenerID _id{0};
};

// ============================================================================
// COMMON ENGINE EVENTS
// ============================================================================
struct WindowResizeEvent { i32 width; i32 height; };
struct WindowCloseEvent  {};
struct FrameBeginEvent   { f64 deltaTime; u64 frameNumber; };
struct FrameEndEvent     {};

struct KeyEvent {
    i32  keyCode;
    bool pressed;
    bool repeat;
    bool shift, ctrl, alt;
    mutable bool consumed{false};
};

struct MouseMoveEvent {
    Vec2 position;
    Vec2 delta;
    mutable bool consumed{false};
};

struct MouseButtonEvent {
    i32  button;
    bool pressed;
    Vec2 position;
    mutable bool consumed{false};
};

struct MouseScrollEvent {
    f32  delta;
    Vec2 position;
    mutable bool consumed{false};
};

struct TextInputEvent {
    u32  codepoint;
    mutable bool consumed{false};
};

} // namespace ace
