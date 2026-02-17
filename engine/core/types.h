/**
 * ACE Engine — Core Types
 * Fundamental math, color, rect, hash, and string utilities.
 * Zero-allocation, constexpr-heavy, SIMD-friendly layout.
 *
 * Copyright (c) 2026 ACE Engine Contributors
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>
#include <string_view>
#include <functional>
#include <type_traits>
#include <array>
#include <span>
#include <concepts>
#include <atomic>
#include <cassert>

namespace ace {

// ============================================================================
// NUMERIC ALIASES
// ============================================================================
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

// ============================================================================
// COMPILE-TIME HASH (FNV-1a)
// ============================================================================
namespace detail {
    constexpr u32 FNV_OFFSET = 0x811c9dc5u;
    constexpr u32 FNV_PRIME  = 0x01000193u;
}

using WidgetID = u32;
using TypeID   = u32;
using ThemeID  = u32;

constexpr u32 Hash(const char* str, u32 h = detail::FNV_OFFSET) {
    return (*str == 0) ? h : Hash(str + 1, (h ^ static_cast<u32>(*str)) * detail::FNV_PRIME);
}

inline u32 HashRuntime(std::string_view sv) {
    u32 h = detail::FNV_OFFSET;
    for (char c : sv) h = (h ^ static_cast<u32>(c)) * detail::FNV_PRIME;
    return h;
}

// ============================================================================
// MATH TYPES — Aligned, constexpr, operator-rich
// ============================================================================
struct Vec2 {
    f32 x{}, y{};

    constexpr Vec2() = default;
    constexpr Vec2(f32 x, f32 y) : x(x), y(y) {}
    explicit constexpr Vec2(f32 s) : x(s), y(s) {}

    constexpr Vec2 operator+(Vec2 b) const { return {x + b.x, y + b.y}; }
    constexpr Vec2 operator-(Vec2 b) const { return {x - b.x, y - b.y}; }
    constexpr Vec2 operator*(f32 s) const  { return {x * s, y * s}; }
    constexpr Vec2 operator*(Vec2 b) const { return {x * b.x, y * b.y}; }
    constexpr Vec2 operator/(f32 s) const  { return {x / s, y / s}; }

    constexpr Vec2& operator+=(Vec2 b) { x += b.x; y += b.y; return *this; }
    constexpr Vec2& operator-=(Vec2 b) { x -= b.x; y -= b.y; return *this; }
    constexpr Vec2& operator*=(f32 s)  { x *= s; y *= s; return *this; }

    constexpr bool operator==(Vec2 b) const { return x == b.x && y == b.y; }
    constexpr bool operator!=(Vec2 b) const { return !(*this == b); }

    constexpr f32  Dot(Vec2 b)   const { return x * b.x + y * b.y; }
    constexpr f32  Cross(Vec2 b) const { return x * b.y - y * b.x; }
    constexpr f32  LenSq()       const { return x * x + y * y; }
    f32            Len()         const { return std::sqrt(LenSq()); }
    Vec2           Normalized()  const { f32 l = Len(); return l > 0 ? *this / l : Vec2{}; }

    constexpr Vec2 Perpendicular() const { return {-y, x}; }
    static constexpr Vec2 Lerp(Vec2 a, Vec2 b, f32 t) { return a + (b - a) * t; }
    static Vec2 Min(Vec2 a, Vec2 b) { return {std::min(a.x, b.x), std::min(a.y, b.y)}; }
    static Vec2 Max(Vec2 a, Vec2 b) { return {std::max(a.x, b.x), std::max(a.y, b.y)}; }
};

struct Vec3 {
    f32 x{}, y{}, z{};
    constexpr Vec3() = default;
    constexpr Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
    constexpr Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    constexpr Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    constexpr Vec3 operator*(f32 s)  const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(f32 s)  const { return {x / s, y / s, z / s}; }
    constexpr f32  Dot(Vec3 b) const { return x * b.x + y * b.y + z * b.z; }
    constexpr f32  LenSq()    const { return x * x + y * y + z * z; }
    f32            Len()      const { return std::sqrt(LenSq()); }
    Vec3           Normalized() const { f32 l = Len(); return l > 0 ? *this / l : Vec3{}; }
    constexpr Vec3 Cross(Vec3 b) const {
        return {y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x};
    }
    static constexpr Vec3 Lerp(Vec3 a, Vec3 b, f32 t) { return a + (b - a) * t; }
};

struct Vec4 {
    f32 x{}, y{}, z{}, w{};
    constexpr Vec4() = default;
    constexpr Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    constexpr Vec4 operator+(Vec4 b) const { return {x + b.x, y + b.y, z + b.z, w + b.w}; }
    constexpr Vec4 operator-(Vec4 b) const { return {x - b.x, y - b.y, z - b.z, w - b.w}; }
    constexpr Vec4 operator*(f32 s) const  { return {x * s, y * s, z * s, w * s}; }
    static constexpr Vec4 Lerp(Vec4 a, Vec4 b, f32 t) { return a + (b - a) * t; }
};

struct Mat4 {
    f32 m[4][4]{};

    static constexpr Mat4 Identity() {
        Mat4 r{};
        r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
        return r;
    }

    static Mat4 Ortho(f32 l, f32 r, f32 t, f32 b, f32 zn = 0.0f, f32 zf = 1.0f) {
        Mat4 o{};
        o.m[0][0] = 2.0f / (r - l);
        o.m[1][1] = 2.0f / (t - b);
        o.m[2][2] = 1.0f / (zf - zn);
        o.m[3][0] = (r + l) / (l - r);
        o.m[3][1] = (t + b) / (b - t);
        o.m[3][2] = zn / (zn - zf);
        o.m[3][3] = 1.0f;
        return o;
    }
};

// ============================================================================
// RECT
// ============================================================================
struct Rect {
    f32 x{}, y{}, w{}, h{};

    constexpr Rect() = default;
    constexpr Rect(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), w(w), h(h) {}

    constexpr f32  Left()   const { return x; }
    constexpr f32  Top()    const { return y; }
    constexpr f32  Right()  const { return x + w; }
    constexpr f32  Bottom() const { return y + h; }
    constexpr Vec2 Pos()    const { return {x, y}; }
    constexpr Vec2 Size()   const { return {w, h}; }
    constexpr Vec2 Center() const { return {x + w * 0.5f, y + h * 0.5f}; }

    constexpr bool Contains(Vec2 p) const {
        return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
    }

    constexpr bool Overlaps(Rect b) const {
        return x < b.x + b.w && x + w > b.x && y < b.y + b.h && y + h > b.y;
    }

    constexpr Rect Expand(f32 amount) const {
        return {x - amount, y - amount, w + amount * 2, h + amount * 2};
    }

    constexpr Rect Shrink(f32 amount) const { return Expand(-amount); }

    static constexpr Rect FromMinMax(Vec2 mn, Vec2 mx) {
        return {mn.x, mn.y, mx.x - mn.x, mx.y - mn.y};
    }

    static Rect Intersection(Rect a, Rect b) {
        f32 x1 = std::max(a.x, b.x);
        f32 y1 = std::max(a.y, b.y);
        f32 x2 = std::min(a.Right(), b.Right());
        f32 y2 = std::min(a.Bottom(), b.Bottom());
        if (x2 <= x1 || y2 <= y1) return {};
        return {x1, y1, x2 - x1, y2 - y1};
    }
};

// ============================================================================
// COLOR (RGBA8, packed u32 ABGR)
// ============================================================================
struct Color {
    u8 r{}, g{}, b{}, a{255};

    constexpr Color() = default;
    constexpr Color(u8 r, u8 g, u8 b, u8 a = 255) : r(r), g(g), b(b), a(a) {}

    constexpr u32 PackABGR() const {
        return (u32(a) << 24) | (u32(b) << 16) | (u32(g) << 8) | u32(r);
    }

    static constexpr Color FromABGR(u32 packed) {
        return {
            u8(packed & 0xFF),
            u8((packed >> 8) & 0xFF),
            u8((packed >> 16) & 0xFF),
            u8((packed >> 24) & 0xFF)
        };
    }

    static constexpr Color FromFloat(f32 r, f32 g, f32 b, f32 a = 1.0f) {
        return {
            u8(r * 255.0f + 0.5f), u8(g * 255.0f + 0.5f),
            u8(b * 255.0f + 0.5f), u8(a * 255.0f + 0.5f)
        };
    }

    constexpr Vec4 ToFloat() const {
        return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }

    static Color Lerp(Color a, Color b, f32 t) {
        return {
            u8(a.r + (b.r - a.r) * t), u8(a.g + (b.g - a.g) * t),
            u8(a.b + (b.b - a.b) * t), u8(a.a + (b.a - a.a) * t)
        };
    }

    constexpr Color WithAlpha(u8 newA) const { return {r, g, b, newA}; }

    // Named colors
    static constexpr Color White()       { return {255, 255, 255, 255}; }
    static constexpr Color Black()       { return {0, 0, 0, 255}; }
    static constexpr Color Transparent() { return {0, 0, 0, 0}; }
    static constexpr Color Red()         { return {248, 113, 113, 255}; }
    static constexpr Color Green()       { return {74, 222, 128, 255}; }
    static constexpr Color Blue()        { return {59, 130, 246, 255}; }
    static constexpr Color Yellow()      { return {250, 204, 21, 255}; }
    static constexpr Color Purple()      { return {139, 92, 246, 255}; }
    static constexpr Color Cyan()        { return {34, 211, 238, 255}; }
};

// ============================================================================
// EDGE INSETS (padding / margins)
// ============================================================================
struct EdgeInsets {
    f32 top{}, right{}, bottom{}, left{};

    constexpr EdgeInsets() = default;
    constexpr EdgeInsets(f32 all) : top(all), right(all), bottom(all), left(all) {}
    constexpr EdgeInsets(f32 v, f32 h) : top(v), right(h), bottom(v), left(h) {}
    constexpr EdgeInsets(f32 t, f32 r, f32 b, f32 l) : top(t), right(r), bottom(b), left(l) {}

    constexpr f32 Horizontal() const { return left + right; }
    constexpr f32 Vertical()   const { return top + bottom; }
};

// ============================================================================
// CONCEPTS
// ============================================================================
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

template<typename T>
concept StringLike = std::is_convertible_v<T, std::string_view>;

// ============================================================================
// UTILITY
// ============================================================================
namespace math {
    constexpr f32 PI  = 3.14159265358979323846f;
    constexpr f32 TAU = PI * 2.0f;

    constexpr f32 Clamp(f32 v, f32 lo, f32 hi) { return v < lo ? lo : (v > hi ? hi : v); }
    constexpr f32 Saturate(f32 v) { return Clamp(v, 0.0f, 1.0f); }
    constexpr f32 Lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
    constexpr f32 InvLerp(f32 a, f32 b, f32 v) { return (b - a) != 0 ? (v - a) / (b - a) : 0; }
    constexpr f32 Remap(f32 v, f32 fromA, f32 fromB, f32 toA, f32 toB) {
        return Lerp(toA, toB, InvLerp(fromA, fromB, v));
    }
    constexpr f32 Deg2Rad(f32 d) { return d * PI / 180.0f; }
    constexpr f32 Rad2Deg(f32 r) { return r * 180.0f / PI; }

    inline f32 SmoothDamp(f32 current, f32 target, f32 speed, f32 dt) {
        return current + (target - current) * (1.0f - std::exp(-speed * dt));
    }
}

} // namespace ace
