/**
 * ACE Engine — Theme Engine
 * Runtime theme management with property-level style tokens.
 * Supports multiple themes, live switching, and inheritance.
 */

#pragma once

#include "../core/types.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace ace {

// ============================================================================
// THEME TOKENS — Named style values
// ============================================================================
enum class ThemeToken : u32 {
    // Backgrounds
    BgPrimary,
    BgSecondary,
    BgTertiary,
    BgSurface,
    BgSurfaceHover,
    BgSurfaceActive,
    BgOverlay,
    BgHeader,

    // Accent
    AccentPrimary,
    AccentSecondary,
    AccentHover,
    AccentActive,

    // Text
    TextPrimary,
    TextSecondary,
    TextDisabled,
    TextAccent,
    TextError,
    TextSuccess,

    // Border
    BorderPrimary,
    BorderSecondary,
    BorderFocused,

    // Input
    InputBg,
    InputBgHover,
    InputBgFocused,
    InputBorder,

    // Scrollbar
    ScrollbarTrack,
    ScrollbarThumb,
    ScrollbarThumbHover,

    // Node editor
    NodeBg,
    NodeHeader,
    NodeBorder,
    NodePin,
    NodeLink,

    // Misc
    Shadow,
    TooltipBg,
    TooltipText,

    MAX_TOKENS
};

// ============================================================================
// THEME METRICS — Named dimension/spacing values
// ============================================================================
enum class ThemeMetric : u32 {
    BorderRadius,
    BorderWidth,
    Padding,
    PaddingSmall,
    PaddingLarge,
    Spacing,
    SpacingSmall,
    SpacingLarge,
    FontSizeSmall,
    FontSizeNormal,
    FontSizeLarge,
    FontSizeTitle,
    WidgetHeight,
    HeaderHeight,
    ScrollbarWidth,
    TabHeight,
    SplitterSize,

    MAX_METRICS
};

// ============================================================================
// THEME — A complete set of colors and metrics
// ============================================================================
struct Theme {
    std::string name;
    ThemeID     id{0};

    Color colors[static_cast<size_t>(ThemeToken::MAX_TOKENS)];
    f32   metrics[static_cast<size_t>(ThemeMetric::MAX_METRICS)];

    Color  GetColor(ThemeToken token) const { return colors[static_cast<size_t>(token)]; }
    f32    GetMetric(ThemeMetric m)   const { return metrics[static_cast<size_t>(m)]; }
    void   SetColor(ThemeToken token, Color c) { colors[static_cast<size_t>(token)] = c; }
    void   SetMetric(ThemeMetric m, f32 v)     { metrics[static_cast<size_t>(m)] = v; }
};

// ============================================================================
// BUILT-IN THEMES
// ============================================================================
namespace themes {

inline Theme CreateDark() {
    Theme t;
    t.name = "Dark";
    t.id = Hash("Dark");

    // Backgrounds
    t.SetColor(ThemeToken::BgPrimary,       {24, 24, 37});
    t.SetColor(ThemeToken::BgSecondary,     {30, 30, 46});
    t.SetColor(ThemeToken::BgTertiary,      {36, 36, 54});
    t.SetColor(ThemeToken::BgSurface,       {40, 40, 60});
    t.SetColor(ThemeToken::BgSurfaceHover,  {50, 50, 72});
    t.SetColor(ThemeToken::BgSurfaceActive, {55, 55, 80});
    t.SetColor(ThemeToken::BgOverlay,       {0, 0, 0, 180});
    t.SetColor(ThemeToken::BgHeader,        {28, 28, 42});

    // Accent
    t.SetColor(ThemeToken::AccentPrimary,   {99, 102, 241});
    t.SetColor(ThemeToken::AccentSecondary, {139, 92, 246});
    t.SetColor(ThemeToken::AccentHover,     {129, 132, 255});
    t.SetColor(ThemeToken::AccentActive,    {79, 82, 221});

    // Text
    t.SetColor(ThemeToken::TextPrimary,     {205, 214, 244});
    t.SetColor(ThemeToken::TextSecondary,   {147, 153, 178});
    t.SetColor(ThemeToken::TextDisabled,    {88, 91, 112});
    t.SetColor(ThemeToken::TextAccent,      {137, 180, 250});
    t.SetColor(ThemeToken::TextError,       {243, 139, 168});
    t.SetColor(ThemeToken::TextSuccess,     {166, 227, 161});

    // Border
    t.SetColor(ThemeToken::BorderPrimary,   {55, 55, 80});
    t.SetColor(ThemeToken::BorderSecondary, {45, 45, 65});
    t.SetColor(ThemeToken::BorderFocused,   {99, 102, 241});

    // Input
    t.SetColor(ThemeToken::InputBg,         {30, 30, 46});
    t.SetColor(ThemeToken::InputBgHover,    {36, 36, 54});
    t.SetColor(ThemeToken::InputBgFocused,  {40, 40, 62});
    t.SetColor(ThemeToken::InputBorder,     {55, 55, 80});

    // Scrollbar
    t.SetColor(ThemeToken::ScrollbarTrack, {30, 30, 46});
    t.SetColor(ThemeToken::ScrollbarThumb, {55, 55, 80});
    t.SetColor(ThemeToken::ScrollbarThumbHover, {75, 75, 100});

    // Node editor
    t.SetColor(ThemeToken::NodeBg,          {35, 35, 52});
    t.SetColor(ThemeToken::NodeHeader,      {45, 45, 65});
    t.SetColor(ThemeToken::NodeBorder,      {60, 60, 85});
    t.SetColor(ThemeToken::NodePin,         {99, 102, 241});
    t.SetColor(ThemeToken::NodeLink,        {139, 92, 246});

    // Misc
    t.SetColor(ThemeToken::Shadow,          {0, 0, 0, 120});
    t.SetColor(ThemeToken::TooltipBg,       {50, 50, 72, 240});
    t.SetColor(ThemeToken::TooltipText,     {205, 214, 244});

    // Metrics
    t.SetMetric(ThemeMetric::BorderRadius,    6.0f);
    t.SetMetric(ThemeMetric::BorderWidth,     1.0f);
    t.SetMetric(ThemeMetric::Padding,         8.0f);
    t.SetMetric(ThemeMetric::PaddingSmall,    4.0f);
    t.SetMetric(ThemeMetric::PaddingLarge,    16.0f);
    t.SetMetric(ThemeMetric::Spacing,         8.0f);
    t.SetMetric(ThemeMetric::SpacingSmall,    4.0f);
    t.SetMetric(ThemeMetric::SpacingLarge,    16.0f);
    t.SetMetric(ThemeMetric::FontSizeSmall,   12.0f);
    t.SetMetric(ThemeMetric::FontSizeNormal,  14.0f);
    t.SetMetric(ThemeMetric::FontSizeLarge,   18.0f);
    t.SetMetric(ThemeMetric::FontSizeTitle,   24.0f);
    t.SetMetric(ThemeMetric::WidgetHeight,    28.0f);
    t.SetMetric(ThemeMetric::HeaderHeight,    32.0f);
    t.SetMetric(ThemeMetric::ScrollbarWidth,  8.0f);
    t.SetMetric(ThemeMetric::TabHeight,       30.0f);
    t.SetMetric(ThemeMetric::SplitterSize,    4.0f);

    return t;
}

inline Theme CreateLight() {
    Theme t;
    t.name = "Light";
    t.id = Hash("Light");

    t.SetColor(ThemeToken::BgPrimary,       {239, 241, 245});
    t.SetColor(ThemeToken::BgSecondary,     {230, 233, 239});
    t.SetColor(ThemeToken::BgTertiary,      {220, 224, 232});
    t.SetColor(ThemeToken::BgSurface,       {255, 255, 255});
    t.SetColor(ThemeToken::BgSurfaceHover,  {242, 244, 248});
    t.SetColor(ThemeToken::BgSurfaceActive, {225, 228, 236});
    t.SetColor(ThemeToken::BgOverlay,       {0, 0, 0, 80});
    t.SetColor(ThemeToken::BgHeader,        {230, 233, 239});

    t.SetColor(ThemeToken::AccentPrimary,   {79, 70, 229});
    t.SetColor(ThemeToken::AccentSecondary, {124, 58, 237});
    t.SetColor(ThemeToken::AccentHover,     {99, 90, 249});
    t.SetColor(ThemeToken::AccentActive,    {59, 50, 199});

    t.SetColor(ThemeToken::TextPrimary,     {28, 28, 36});
    t.SetColor(ThemeToken::TextSecondary,   {92, 95, 119});
    t.SetColor(ThemeToken::TextDisabled,    {156, 160, 176});
    t.SetColor(ThemeToken::TextAccent,      {30, 102, 245});
    t.SetColor(ThemeToken::TextError,       {210, 15, 57});
    t.SetColor(ThemeToken::TextSuccess,     {64, 160, 43});

    t.SetColor(ThemeToken::BorderPrimary,   {204, 208, 218});
    t.SetColor(ThemeToken::BorderSecondary, {220, 224, 232});
    t.SetColor(ThemeToken::BorderFocused,   {79, 70, 229});

    t.SetColor(ThemeToken::InputBg,         {255, 255, 255});
    t.SetColor(ThemeToken::InputBgHover,    {248, 249, 252});
    t.SetColor(ThemeToken::InputBgFocused,  {255, 255, 255});
    t.SetColor(ThemeToken::InputBorder,     {204, 208, 218});

    t.SetColor(ThemeToken::ScrollbarTrack,  {239, 241, 245});
    t.SetColor(ThemeToken::ScrollbarThumb,  {188, 192, 204});
    t.SetColor(ThemeToken::ScrollbarThumbHover, {156, 160, 176});

    t.SetColor(ThemeToken::NodeBg,          {255, 255, 255});
    t.SetColor(ThemeToken::NodeHeader,      {239, 241, 245});
    t.SetColor(ThemeToken::NodeBorder,      {204, 208, 218});
    t.SetColor(ThemeToken::NodePin,         {79, 70, 229});
    t.SetColor(ThemeToken::NodeLink,        {124, 58, 237});

    t.SetColor(ThemeToken::Shadow,          {0, 0, 0, 30});
    t.SetColor(ThemeToken::TooltipBg,       {28, 28, 36, 230});
    t.SetColor(ThemeToken::TooltipText,     {255, 255, 255});

    // Same metrics as dark
    t.SetMetric(ThemeMetric::BorderRadius,    6.0f);
    t.SetMetric(ThemeMetric::BorderWidth,     1.0f);
    t.SetMetric(ThemeMetric::Padding,         8.0f);
    t.SetMetric(ThemeMetric::PaddingSmall,    4.0f);
    t.SetMetric(ThemeMetric::PaddingLarge,    16.0f);
    t.SetMetric(ThemeMetric::Spacing,         8.0f);
    t.SetMetric(ThemeMetric::SpacingSmall,    4.0f);
    t.SetMetric(ThemeMetric::SpacingLarge,    16.0f);
    t.SetMetric(ThemeMetric::FontSizeSmall,   12.0f);
    t.SetMetric(ThemeMetric::FontSizeNormal,  14.0f);
    t.SetMetric(ThemeMetric::FontSizeLarge,   18.0f);
    t.SetMetric(ThemeMetric::FontSizeTitle,   24.0f);
    t.SetMetric(ThemeMetric::WidgetHeight,    28.0f);
    t.SetMetric(ThemeMetric::HeaderHeight,    32.0f);
    t.SetMetric(ThemeMetric::ScrollbarWidth,  8.0f);
    t.SetMetric(ThemeMetric::TabHeight,       30.0f);
    t.SetMetric(ThemeMetric::SplitterSize,    4.0f);

    return t;
}

inline Theme CreateCyberpunk() {
    Theme t;
    t.name = "Cyberpunk";
    t.id = Hash("Cyberpunk");

    t.SetColor(ThemeToken::BgPrimary,       {10, 10, 18});
    t.SetColor(ThemeToken::BgSecondary,     {15, 15, 28});
    t.SetColor(ThemeToken::BgTertiary,      {20, 20, 38});
    t.SetColor(ThemeToken::BgSurface,       {18, 18, 35});
    t.SetColor(ThemeToken::BgSurfaceHover,  {28, 28, 50});
    t.SetColor(ThemeToken::BgSurfaceActive, {35, 35, 62});
    t.SetColor(ThemeToken::BgOverlay,       {0, 0, 0, 200});
    t.SetColor(ThemeToken::BgHeader,        {12, 12, 24});

    t.SetColor(ThemeToken::AccentPrimary,   {0, 255, 170});
    t.SetColor(ThemeToken::AccentSecondary, {255, 0, 128});
    t.SetColor(ThemeToken::AccentHover,     {0, 255, 200});
    t.SetColor(ThemeToken::AccentActive,    {0, 200, 130});

    t.SetColor(ThemeToken::TextPrimary,     {220, 240, 255});
    t.SetColor(ThemeToken::TextSecondary,   {120, 140, 170});
    t.SetColor(ThemeToken::TextDisabled,    {60, 70, 90});
    t.SetColor(ThemeToken::TextAccent,      {0, 255, 170});
    t.SetColor(ThemeToken::TextError,       {255, 60, 90});
    t.SetColor(ThemeToken::TextSuccess,     {0, 255, 170});

    t.SetColor(ThemeToken::BorderPrimary,   {0, 255, 170, 80});
    t.SetColor(ThemeToken::BorderSecondary, {255, 0, 128, 60});
    t.SetColor(ThemeToken::BorderFocused,   {0, 255, 170});

    t.SetColor(ThemeToken::InputBg,         {12, 12, 25});
    t.SetColor(ThemeToken::InputBgHover,    {18, 18, 35});
    t.SetColor(ThemeToken::InputBgFocused,  {15, 15, 30});
    t.SetColor(ThemeToken::InputBorder,     {0, 255, 170, 60});

    t.SetColor(ThemeToken::ScrollbarTrack,  {12, 12, 25});
    t.SetColor(ThemeToken::ScrollbarThumb,  {0, 255, 170, 80});
    t.SetColor(ThemeToken::ScrollbarThumbHover, {0, 255, 170, 140});

    t.SetColor(ThemeToken::NodeBg,          {15, 15, 30});
    t.SetColor(ThemeToken::NodeHeader,      {20, 20, 40});
    t.SetColor(ThemeToken::NodeBorder,      {0, 255, 170, 60});
    t.SetColor(ThemeToken::NodePin,         {0, 255, 170});
    t.SetColor(ThemeToken::NodeLink,        {255, 0, 128});

    t.SetColor(ThemeToken::Shadow,          {0, 0, 0, 180});
    t.SetColor(ThemeToken::TooltipBg,       {15, 15, 30, 240});
    t.SetColor(ThemeToken::TooltipText,     {0, 255, 170});

    // Metrics — sharper, more angular
    t.SetMetric(ThemeMetric::BorderRadius,    2.0f);
    t.SetMetric(ThemeMetric::BorderWidth,     1.5f);
    t.SetMetric(ThemeMetric::Padding,         8.0f);
    t.SetMetric(ThemeMetric::PaddingSmall,    4.0f);
    t.SetMetric(ThemeMetric::PaddingLarge,    16.0f);
    t.SetMetric(ThemeMetric::Spacing,         6.0f);
    t.SetMetric(ThemeMetric::SpacingSmall,    3.0f);
    t.SetMetric(ThemeMetric::SpacingLarge,    12.0f);
    t.SetMetric(ThemeMetric::FontSizeSmall,   11.0f);
    t.SetMetric(ThemeMetric::FontSizeNormal,  13.0f);
    t.SetMetric(ThemeMetric::FontSizeLarge,   17.0f);
    t.SetMetric(ThemeMetric::FontSizeTitle,   22.0f);
    t.SetMetric(ThemeMetric::WidgetHeight,    26.0f);
    t.SetMetric(ThemeMetric::HeaderHeight,    30.0f);
    t.SetMetric(ThemeMetric::ScrollbarWidth,  6.0f);
    t.SetMetric(ThemeMetric::TabHeight,       28.0f);
    t.SetMetric(ThemeMetric::SplitterSize,    3.0f);

    return t;
}

} // namespace themes

// ============================================================================
// THEME ENGINE — Manages themes and provides style lookups
// ============================================================================
class ThemeEngine {
public:
    ThemeEngine() {
        // Register built-in themes
        auto dark = themes::CreateDark();
        auto light = themes::CreateLight();
        auto cyber = themes::CreateCyberpunk();
        _activeThemeId = dark.id;
        _themes[dark.id] = std::move(dark);
        _themes[light.id] = std::move(light);
        _themes[cyber.id] = std::move(cyber);
    }

    void RegisterTheme(Theme theme) {
        _themes[theme.id] = std::move(theme);
    }

    bool SetActiveTheme(ThemeID id) {
        if (_themes.find(id) == _themes.end()) return false;
        _activeThemeId = id;
        if (_onThemeChange) _onThemeChange(id);
        return true;
    }

    bool SetActiveTheme(std::string_view name) {
        for (auto& [id, t] : _themes)
            if (t.name == name) return SetActiveTheme(id);
        return false;
    }

    const Theme& GetActiveTheme() const { return _themes.at(_activeThemeId); }
    ThemeID      GetActiveThemeId() const { return _activeThemeId; }

    Color GetColor(ThemeToken token)  const { return GetActiveTheme().GetColor(token); }
    f32   GetMetric(ThemeMetric m)    const { return GetActiveTheme().GetMetric(m); }

    void OnThemeChange(std::function<void(ThemeID)> cb) { _onThemeChange = std::move(cb); }

    // Animated theme transition
    Color GetAnimatedColor(ThemeToken token, f32 /*t*/) const {
        return GetColor(token); // Extend with interpolation between themes
    }

    std::vector<std::string> GetThemeNames() const {
        std::vector<std::string> names;
        for (auto& [_, t] : _themes) names.push_back(t.name);
        return names;
    }

private:
    std::unordered_map<ThemeID, Theme> _themes;
    ThemeID _activeThemeId{0};
    std::function<void(ThemeID)> _onThemeChange;
};

} // namespace ace
