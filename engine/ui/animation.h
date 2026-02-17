/**
 * ACE Engine — Animation System
 * Declarative animation with easing curves, keyframes, and springs.
 * Integrates with widget properties for smooth transitions.
 */

#pragma once

#include "../core/types.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace ace {

// ============================================================================
// EASING FUNCTIONS
// ============================================================================
namespace easing {
    inline f32 Linear(f32 t) { return t; }

    inline f32 InQuad(f32 t) { return t * t; }
    inline f32 OutQuad(f32 t) { return t * (2 - t); }
    inline f32 InOutQuad(f32 t) { return t < 0.5f ? 2*t*t : -1 + (4-2*t)*t; }

    inline f32 InCubic(f32 t) { return t*t*t; }
    inline f32 OutCubic(f32 t) { f32 u = t-1; return u*u*u + 1; }
    inline f32 InOutCubic(f32 t) {
        return t < 0.5f ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2) + 1;
    }

    inline f32 InExpo(f32 t) { return t == 0 ? 0 : std::pow(2, 10*(t-1)); }
    inline f32 OutExpo(f32 t) { return t == 1 ? 1 : 1 - std::pow(2, -10*t); }
    inline f32 InOutExpo(f32 t) {
        if (t == 0 || t == 1) return t;
        return t < 0.5f
            ? std::pow(2, 20*t - 10) * 0.5f
            : (2 - std::pow(2, -20*t + 10)) * 0.5f;
    }

    inline f32 InElastic(f32 t) {
        if (t == 0 || t == 1) return t;
        return -std::pow(2, 10*t - 10) * std::sin((t*10 - 10.75f) * (math::TAU / 3));
    }
    inline f32 OutElastic(f32 t) {
        if (t == 0 || t == 1) return t;
        return std::pow(2, -10*t) * std::sin((t*10 - 0.75f) * (math::TAU / 3)) + 1;
    }

    inline f32 InBack(f32 t) {
        constexpr f32 c = 1.70158f;
        return (c + 1) * t*t*t - c * t*t;
    }
    inline f32 OutBack(f32 t) {
        constexpr f32 c = 1.70158f;
        f32 u = t - 1;
        return 1 + (c + 1) * u*u*u + c * u*u;
    }

    inline f32 OutBounce(f32 t) {
        if (t < 1/2.75f)      return 7.5625f*t*t;
        if (t < 2/2.75f)      { t -= 1.5f/2.75f; return 7.5625f*t*t + 0.75f; }
        if (t < 2.5f/2.75f)   { t -= 2.25f/2.75f; return 7.5625f*t*t + 0.9375f; }
        t -= 2.625f/2.75f;    return 7.5625f*t*t + 0.984375f;
    }
    inline f32 InBounce(f32 t) { return 1 - OutBounce(1 - t); }

    using EasingFn = f32(*)(f32);
}

// ============================================================================
// ANIMATION — Single value transition
// ============================================================================
struct Animation {
    using UpdateFn = std::function<void(f32 value)>;
    using CompleteFn = std::function<void()>;

    u32                 id{0};
    f32                 from{0};
    f32                 to{1};
    f32                 duration{0.3f};   // seconds
    f32                 delay{0};         // seconds
    f32                 elapsed{0};
    easing::EasingFn    easingFn{easing::OutCubic};
    UpdateFn            onUpdate;
    CompleteFn          onComplete;
    bool                loop{false};
    bool                pingPong{false};
    bool                finished{false};
    bool                paused{false};
    bool                reverse{false};

    f32 CurrentValue() const {
        if (duration <= 0) return to;
        f32 t = math::Saturate((elapsed - delay) / duration);
        if (reverse) t = 1.0f - t;
        f32 eased = easingFn ? easingFn(t) : t;
        return math::Lerp(from, to, eased);
    }

    bool Update(f32 dt) {
        if (finished || paused) return finished;

        elapsed += dt;
        f32 t = (elapsed - delay) / duration;

        if (t >= 1.0f) {
            if (pingPong) {
                reverse = !reverse;
                elapsed = delay;
                if (!loop && reverse == false) {
                    finished = true;
                }
            } else if (loop) {
                elapsed = delay;
            } else {
                finished = true;
            }
        }

        if (onUpdate) onUpdate(CurrentValue());
        if (finished && onComplete) onComplete();
        return finished;
    }
};

// ============================================================================
// VEC2 ANIMATION
// ============================================================================
struct Vec2Animation {
    Animation xAnim;
    Animation yAnim;

    Vec2 CurrentValue() const { return {xAnim.CurrentValue(), yAnim.CurrentValue()}; }

    bool Update(f32 dt) {
        bool a = xAnim.Update(dt);
        bool b = yAnim.Update(dt);
        return a && b;
    }
};

// ============================================================================
// COLOR ANIMATION
// ============================================================================
struct ColorAnimation {
    Color  from;
    Color  to;
    f32    duration{0.3f};
    f32    elapsed{0};
    easing::EasingFn easingFn{easing::OutCubic};
    bool   finished{false};

    Color CurrentValue() const {
        f32 t = math::Saturate(elapsed / duration);
        f32 eased = easingFn ? easingFn(t) : t;
        return Color::Lerp(from, to, eased);
    }

    bool Update(f32 dt) {
        elapsed += dt;
        if (elapsed >= duration) finished = true;
        return finished;
    }
};

// ============================================================================
// SPRING ANIMATION — Physics-based motion
// ============================================================================
struct SpringAnimation {
    f32 value{0};
    f32 target{0};
    f32 velocity{0};
    f32 stiffness{170.0f};
    f32 damping{26.0f};
    f32 mass{1.0f};
    f32 restThreshold{0.001f};

    bool IsAtRest() const {
        return std::abs(value - target) < restThreshold &&
               std::abs(velocity) < restThreshold;
    }

    void Update(f32 dt) {
        f32 springForce = -stiffness * (value - target);
        f32 dampForce = -damping * velocity;
        f32 acceleration = (springForce + dampForce) / mass;
        velocity += acceleration * dt;
        value += velocity * dt;
    }
};

// ============================================================================
// ANIMATION SYSTEM — Manages all active animations
// ============================================================================
class AnimationSystem {
public:
    u32 Play(Animation anim) {
        anim.id = _nextId++;
        _animations.push_back(std::move(anim));
        return _animations.back().id;
    }

    void Stop(u32 id) {
        for (auto& a : _animations)
            if (a.id == id) { a.finished = true; break; }
    }

    void Pause(u32 id) {
        for (auto& a : _animations)
            if (a.id == id) { a.paused = true; break; }
    }

    void Resume(u32 id) {
        for (auto& a : _animations)
            if (a.id == id) { a.paused = false; break; }
    }

    void StopAll() {
        for (auto& a : _animations) a.finished = true;
    }

    void Update(f32 dt) {
        // Update all animations
        for (auto& anim : _animations) {
            anim.Update(dt);
        }

        // Remove finished
        _animations.erase(
            std::remove_if(_animations.begin(), _animations.end(),
                [](const Animation& a) { return a.finished; }),
            _animations.end());
    }

    bool HasActive() const { return !_animations.empty(); }
    size_t ActiveCount() const { return _animations.size(); }

    // Convenience: animate a float
    u32 AnimateFloat(f32 from, f32 to, f32 duration,
                     std::function<void(f32)> update,
                     easing::EasingFn ease = easing::OutCubic,
                     std::function<void()> complete = nullptr) {
        Animation a{};
        a.from = from;
        a.to = to;
        a.duration = duration;
        a.easingFn = ease;
        a.onUpdate = std::move(update);
        a.onComplete = std::move(complete);
        return Play(std::move(a));
    }

private:
    std::vector<Animation> _animations;
    u32 _nextId{1};
};

} // namespace ace
