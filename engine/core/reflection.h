/**
 * ACE Engine — Compile-Time Reflection System
 * Macro-based property registration with TypeDescriptor.
 * Supports property iteration, name → offset mapping, and serialization hints.
 *
 * Usage:
 *   struct PlayerSettings {
 *       float health = 100.0f;
 *       int   ammo   = 30;
 *       bool  godMode = false;
 *       std::string name = "Player";
 *
 *       ACE_REFLECT(PlayerSettings,
 *           ACE_FIELD(health, "Player health", ace::PropFlags::Slider, 0.0f, 200.0f),
 *           ACE_FIELD(ammo,   "Ammunition count", ace::PropFlags::DragInt),
 *           ACE_FIELD(godMode, "Invincibility toggle"),
 *           ACE_FIELD(name,   "Player name")
 *       )
 *   };
 */

#pragma once

#include "types.h"
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <optional>

namespace ace {

// ============================================================================
// PROPERTY FLAGS — Hint how to render in inspector
// ============================================================================
enum class PropFlags : u32 {
    None       = 0,
    ReadOnly   = 1 << 0,
    Hidden     = 1 << 1,
    Slider     = 1 << 2,
    DragInt    = 1 << 3,
    DragFloat  = 1 << 4,
    ColorPick  = 1 << 5,
    TextInput  = 1 << 6,
    Checkbox   = 1 << 7,
    Dropdown   = 1 << 8,
    Header     = 1 << 9,
    Separator  = 1 << 10,
};

constexpr PropFlags operator|(PropFlags a, PropFlags b) {
    return static_cast<PropFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}
constexpr bool HasFlag(PropFlags val, PropFlags flag) {
    return (static_cast<u32>(val) & static_cast<u32>(flag)) != 0;
}

// ============================================================================
// PROPERTY TYPE ENUM — Runtime type discriminator
// ============================================================================
enum class PropType : u8 {
    Unknown,
    Bool,
    Int,
    Float,
    Double,
    String,
    Vec2,
    Vec3,
    Vec4,
    Color,
    Enum,
    Custom,
};

// ============================================================================
// TYPE DEDUCTION
// ============================================================================
template<typename T> constexpr PropType DeducePropType() {
    if constexpr (std::is_same_v<T, bool>)          return PropType::Bool;
    else if constexpr (std::is_integral_v<T>)       return PropType::Int;
    else if constexpr (std::is_same_v<T, float>)    return PropType::Float;
    else if constexpr (std::is_same_v<T, double>)   return PropType::Double;
    else if constexpr (std::is_same_v<T, std::string>) return PropType::String;
    else if constexpr (std::is_same_v<T, Vec2>)     return PropType::Vec2;
    else if constexpr (std::is_same_v<T, Vec3>)     return PropType::Vec3;
    else if constexpr (std::is_same_v<T, Vec4>)     return PropType::Vec4;
    else if constexpr (std::is_same_v<T, Color>)    return PropType::Color;
    else if constexpr (std::is_enum_v<T>)           return PropType::Enum;
    else                                            return PropType::Custom;
}

// ============================================================================
// PROPERTY INFO — One reflected field
// ============================================================================
struct PropertyInfo {
    std::string_view name;
    std::string_view description;
    PropType         type;
    PropFlags        flags;
    size_t           offset;
    size_t           size;
    f32              rangeMin{0};
    f32              rangeMax{1};

    // Type-erased getter / setter
    using Getter = std::function<void(const void* obj, void* out)>;
    using Setter = std::function<void(void* obj, const void* in)>;
    Getter getter;
    Setter setter;

    template<typename T>
    const T& GetValue(const void* obj) const {
        return *reinterpret_cast<const T*>(static_cast<const u8*>(obj) + offset);
    }

    template<typename T>
    void SetValue(void* obj, const T& val) const {
        *reinterpret_cast<T*>(static_cast<u8*>(obj) + offset) = val;
    }
};

// ============================================================================
// TYPE DESCRIPTOR — Complete reflection info for a type
// ============================================================================
struct TypeDescriptor {
    std::string_view            name;
    TypeID                      id{0};
    size_t                      size{0};
    std::vector<PropertyInfo>   properties;

    const PropertyInfo* FindProperty(std::string_view propName) const {
        for (auto& p : properties)
            if (p.name == propName) return &p;
        return nullptr;
    }
};

// ============================================================================
// TYPE REGISTRY — Global singleton for reflected types
// ============================================================================
class TypeRegistry {
public:
    static TypeRegistry& Instance() {
        static TypeRegistry inst;
        return inst;
    }

    void Register(TypeDescriptor desc) {
        _types[desc.id] = std::move(desc);
    }

    const TypeDescriptor* Find(TypeID id) const {
        auto it = _types.find(id);
        return it != _types.end() ? &it->second : nullptr;
    }

    const TypeDescriptor* FindByName(std::string_view name) const {
        for (auto& [_, desc] : _types)
            if (desc.name == name) return &desc;
        return nullptr;
    }

    template<typename F>
    void ForEach(F&& fn) const {
        for (auto& [_, desc] : _types) fn(desc);
    }

private:
    TypeRegistry() = default;
    std::unordered_map<TypeID, TypeDescriptor> _types;
};

// ============================================================================
// AUTO-REGISTRAR — Static initialization trick
// ============================================================================
struct TypeAutoRegistrar {
    TypeAutoRegistrar(TypeDescriptor desc) {
        TypeRegistry::Instance().Register(std::move(desc));
    }
};

// ============================================================================
// REFLECTION HELPERS — Lambda-free for MSVC compatibility
// ============================================================================
template<typename T>
inline PropertyInfo MakePropertyImpl(
    std::string_view name, std::string_view desc,
    PropFlags flags, size_t offset, size_t sz,
    f32 rangeMin, f32 rangeMax)
{
    PropertyInfo pi{};
    pi.name = name;
    pi.description = desc;
    pi.type = DeducePropType<T>();
    pi.flags = flags;
    pi.offset = offset;
    pi.size = sz;
    pi.rangeMin = rangeMin;
    pi.rangeMax = rangeMax;
    return pi;
}

inline std::vector<PropertyInfo> MakePropertyList(
    std::initializer_list<PropertyInfo> list)
{
    return std::vector<PropertyInfo>(list);
}

// ============================================================================
// REFLECTION MACROS
// ============================================================================

/**
 * ACE_FIELD(member, description, [flags], [min], [max])
 * Creates a PropertyInfo entry for a struct member.
 * Uses a template helper function instead of immediately-invoked lambdas
 * to avoid MSVC C2106 bugs with IIFEs inside braced initializer lists.
 */
#define ACE_FIELD_5(member, desc, flags, min_val, max_val) \
    ace::MakePropertyImpl<decltype(Self::member)>( \
        #member, desc, flags, \
        offsetof(Self, member), sizeof(decltype(Self::member)), \
        min_val, max_val)

#define ACE_FIELD_3(member, desc, flags) ACE_FIELD_5(member, desc, flags, 0.0f, 1.0f)
#define ACE_FIELD_2(member, desc)        ACE_FIELD_5(member, desc, ace::PropFlags::None, 0.0f, 1.0f)
#define ACE_FIELD_1(member)              ACE_FIELD_2(member, "")

// Overload selector (ACE_EXPAND works around MSVC legacy preprocessor __VA_ARGS__ bug)
#define ACE_EXPAND_ARGS(...) __VA_ARGS__
#define ACE_GET_FIELD_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME
#define ACE_FIELD(...) ACE_EXPAND_ARGS(ACE_GET_FIELD_MACRO(__VA_ARGS__, ACE_FIELD_5, \
    ACE_FIELD_UNUSED, ACE_FIELD_3, ACE_FIELD_2, ACE_FIELD_1)(__VA_ARGS__))

/**
 * ACE_REFLECT(StructName, properties...)
 * Place inside your struct to generate a static TypeDescriptor and auto-register it.
 * Uses MakePropertyList() so __VA_ARGS__ expands inside a function call
 * rather than a braced initializer list (avoids MSVC preprocessor bugs).
 */
#define ACE_REFLECT(TypeName, ...) \
    using Self = TypeName; \
    static const ace::TypeDescriptor& GetTypeDescriptor() { \
        static ace::TypeDescriptor td; \
        static bool _ace_init = false; \
        if (!_ace_init) { \
            _ace_init = true; \
            td.name = #TypeName; \
            td.id = ace::Hash(#TypeName); \
            td.size = sizeof(TypeName); \
            td.properties = ace::MakePropertyList({ __VA_ARGS__ }); \
        } \
        return td; \
    } \
    static inline ace::TypeAutoRegistrar _ace_reg_##TypeName{ TypeName::GetTypeDescriptor() };

} // namespace ace
