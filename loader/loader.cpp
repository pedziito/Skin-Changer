/*
 * AC Loader — Premium DX11 Launcher v4.0
 * Built on ACE Engine v2.0 — zero ImGui dependency.
 * 
 * Design: Floating glass-card UI, gradient mesh background,
 *         blue-purple accent, vertical dashboard layout.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <Shlobj.h>
#include <shellapi.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#endif

#include "../engine/ace_engine_v2.h"

// stb_image for decoding embedded PNG
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "../vendor/stb/stb_image.h"

// Embedded CS2 logo (PNG → C array, compiled into binary)
#include "../assets/cs2_logo_data.h"

// Embedded AC glow logo for injection overlay
#include "../assets/ac_glow_logo_data.h"

// New AC purple sunset logo (rounded)
#include "../assets/ac_logo_data.h"

#include "resource.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include "../vendor/json/json.hpp"

#ifdef _WIN32

using namespace ace;

// ============================================================================
// DEBUG LOG — writes to ac_loader.log next to the exe
// ============================================================================
static FILE* g_logFile = nullptr;

static void LogInit() {
    char p[MAX_PATH]; GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    size_t i = s.find_last_of('\\');
    std::string dir = (i != std::string::npos) ? s.substr(0, i + 1) : "";
    g_logFile = fopen((dir + "ac_loader.log").c_str(), "w");
}
static void Log(const char* fmt, ...) {
    if (!g_logFile) return;
    va_list args; va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    fprintf(g_logFile, "\n");
    fflush(g_logFile);
    va_end(args);
}

// ============================================================================
// GLOBALS
// ============================================================================
static DX11Backend g_backend;
static FontAtlas   g_fontAtlas;
static InputSystem g_input;

static u32  g_fontSm   = 0;   // 12pt
static u32  g_font     = 0;   // 14pt
static u32  g_fontMd   = 0;   // 17pt
static u32  g_fontLg   = 0;   // 24pt
static u32  g_fontXl   = 0;   // 32pt

static TextureHandle g_cs2Logo = INVALID_TEXTURE;  // CS2 logo texture
static TextureHandle g_acGlowLogo = INVALID_TEXTURE; // AC glow logo for injection overlay
static TextureHandle g_acLogo = INVALID_TEXTURE;     // New purple sunset AC logo

static HWND g_hwnd    = nullptr;
static bool g_running = true;
static int  g_width   = 480;
static int  g_height  = 420;
static constexpr int CORNER = 16;

// DPI scaling — detected at startup, all UI scaled proportionally
static f32 g_dpiScale = 1.0f;
static f32 DPI(f32 v) { return v * g_dpiScale; }

// Sound feedback — injection only (beep on overlay start + success/fail already handled)

// ============================================================================
// TIMING & ANIMATION
// ============================================================================
static f32 g_dt   = 0.016f;
static f32 g_time = 0.0f;
static std::unordered_map<u32, f32> g_anims;

static f32 Anim(u32 id, f32 target, f32 speed = 12.0f) {
    auto& v = g_anims[id];
    v += (target - v) * (std::min)(1.0f, speed * g_dt);
    if (std::abs(v - target) < 0.001f) v = target;
    return v;
}

// ============================================================================
// HELPERS
// ============================================================================
static bool Hit(f32 x, f32 y, f32 w, f32 h) {
    Vec2 m = g_input.MousePos();
    return m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
}

static Color Fade(Color c, f32 a) {
    return Color{c.r, c.g, c.b, u8(a * 255.0f + 0.5f)};
}
static Color Mix(Color a, Color b, f32 t) {
    return Color{u8(a.r + (b.r - a.r) * t), u8(a.g + (b.g - a.g) * t),
                 u8(a.b + (b.b - a.b) * t), u8(a.a + (b.a - a.a) * t)};
}

static void Text(DrawList& dl, f32 x, f32 y, Color c, const char* t, u32 fid = 0) {
    g_fontAtlas.RenderText(dl, fid ? fid : g_font, Vec2{x, y}, t, c);
}
static Vec2 Measure(const char* t, u32 fid = 0) {
    return g_fontAtlas.MeasureText(fid ? fid : g_font, t);
}
static void TextCenter(DrawList& dl, Rect r, Color c, const char* t, u32 fid = 0) {
    Vec2 sz = Measure(t, fid);
    Text(dl, r.x + (r.w - sz.x) * 0.5f, r.y + (r.h - sz.y) * 0.5f, c, t, fid);
}

// ============================================================================
// PALETTE — deep dark with blue/purple accents
// ============================================================================
namespace P {
    // Backgrounds — deep purple/indigo from sunset image
    constexpr Color Bg       {10,  8,   20,  255};
    constexpr Color Card     {18,  14,  30,  245};
    constexpr Color CardHi   {26,  20,  42,  255};
    constexpr Color Surface  {22,  18,  36,  255};
    constexpr Color Input    {14,  10,  24,  255};
    constexpr Color InputFoc {20,  16,  34,  255};

    // Accent gradient endpoints — purple/lavender from sunset
    constexpr Color Accent1  {140, 100, 220, 255};   // lavender purple
    constexpr Color Accent2  {180, 80,  200, 255};   // pink-purple
    constexpr Color AccentDim{140, 100, 220, 60};

    // Status
    constexpr Color Green    {56,  230, 130, 255};
    constexpr Color Red      {255, 90,  90,  255};
    constexpr Color Yellow   {255, 210, 60,  255};
    constexpr Color Orange   {255, 150, 50,  255};

    // Text — soft lavender/white
    constexpr Color T1       {235, 225, 250, 255};
    constexpr Color T2       {150, 140, 180, 255};
    constexpr Color T3       {80,  70,  110, 255};

    // Borders
    constexpr Color Border   {40,  30,  60,  255};
    constexpr Color BorderLt {55,  45,  80,  180};
}

// ============================================================================
// APP STATE
// ============================================================================
enum Screen { LOGIN, SIGNUP, DASHBOARD };
static Screen g_screen = LOGIN;

static char g_loginUser[64]   = "";
static char g_loginPass[64]   = "";
static char g_signupUser[64]  = "";
static char g_signupPass[64]  = "";
static char g_signupPass2[64] = "";
static char g_signupKey[64]   = "";
static std::string g_authError;
static f32 g_authErrorTimer = 0;
static std::string g_loggedUser;
static u32 g_focusedField = 0;

// Subscription
struct SubInfo {
    std::string game;
    std::string expiry;
    int         daysLeft;
    bool        active;
};
static SubInfo g_sub{};
static bool g_hasSub = false;

// Injection
static bool g_injecting = false;
static bool g_injected  = false;
static std::string g_toastMsg;
static f32 g_toastTimer = 0;
static Color g_toastCol = P::Green;

// Dashboard sidebar nav
static int g_navIndex = 0; // 0=Home, 1=Website, 2=Support, 3=Market

// CS2 detail popup
static bool g_showPopup = false;
static f32  g_popupAnim = 0.0f;

// Remember Me
static bool g_rememberMe = false;
static char g_savedUser[64] = "";
static char g_savedPass[64] = "";

// Injection flow state
enum InjectionPhase {
    INJ_IDLE,
    INJ_CLOSING_STEAM,
    INJ_STARTING_STEAM,
    INJ_WAIT_CS2,          // waiting for cs2.exe process
    INJ_WAIT_CS2_READY,    // cs2.exe found, waiting for main menu (window visible)
    INJ_OVERLAY,           // fullscreen overlay with AC logo + progress
    INJ_INJECTING,         // actually injecting DLL
    INJ_DONE,
    INJ_CS2_MENU           // in-game menu
};
static InjectionPhase g_injPhase = INJ_IDLE;
static f32  g_injProgress = 0.0f;
static f32  g_injTimer    = 0.0f;
static bool g_steamWasRunning = false;
static HWND g_cs2Hwnd = nullptr; // CS2 window handle for centering
static DWORD g_cs2Pid = 0;       // CS2 process ID
static RECT g_origWindowRect = {}; // Original loader window position before fullscreen overlay
static bool g_cs2MenuVisible = false; // Toggle with Insert key
static bool g_hasInjected = false;   // True after first injection — overlay never shows again
static f32  g_menuOpenAnim = 0.0f;   // 0→1 scale+opacity animation for CS2 menu (Insert toggle)

// CS2 Menu — tab navigation
enum CS2Tab { TAB_SKINS = 0, TAB_CONFIGS = 1 };
static int g_cs2Tab = TAB_SKINS;

// ============================================================================
// EMBEDDED SKIN DATABASE — full CS2 weapon + skin data with rarity
// Rarity: 1=Consumer(grey), 2=Industrial(light blue), 3=Mil-Spec(blue),
//         4=Restricted(purple), 5=Classified(pink), 6=Covert(red), 7=Contraband(gold)
// ============================================================================
struct MenuSkinInfo {
    const char* name;
    int paintKit;
    int rarity; // 1-7
};

struct MenuWeaponInfo {
    const char* name;
    int id;
    const char* category;
    MenuSkinInfo skins[24];
    int skinCount;
};

// Rarity colors for UI
static Color RarityColor(int rarity) {
    switch (rarity) {
        case 1: return Color{176, 176, 176, 255}; // Consumer — grey
        case 2: return Color{94, 152, 217, 255};   // Industrial — light blue
        case 3: return Color{75, 105, 255, 255};   // Mil-Spec — blue
        case 4: return Color{136, 71, 255, 255};   // Restricted — purple
        case 5: return Color{211, 44, 230, 255};   // Classified — pink
        case 6: return Color{235, 75, 75, 255};    // Covert — red
        case 7: return Color{228, 174, 57, 255};   // Contraband — gold
        default: return Color{150, 150, 150, 255};
    }
}

static const char* RarityName(int rarity) {
    switch (rarity) {
        case 1: return "Consumer";   case 2: return "Industrial";
        case 3: return "Mil-Spec";   case 4: return "Restricted";
        case 5: return "Classified"; case 6: return "Covert";
        case 7: return "Contraband"; default: return "Unknown";
    }
}

static const MenuWeaponInfo g_weaponDB[] = {
    // --- PISTOLS ---
    {"Glock-18", 4, "Pistols", {
        {"Fade", 38, 6}, {"Water Elemental", 410, 5}, {"Wasteland Rebel", 511, 5},
        {"Gamma Doppler", 568, 6}, {"Twilight Galaxy", 638, 5}, {"Bullet Queen", 773, 5},
        {"Moonrise", 713, 4}, {"Neo-Noir", 770, 4}, {"Vogue", 818, 4},
        {"Dragon Tattoo", 46, 4}, {"Reactor", 209, 4}, {"Candy Apple", 3, 3},
        {"Grinder", 349, 3}, {"Steel Disruption", 541, 3}, {"Sand Dune", 208, 1}
    }, 15},
    {"USP-S", 61, "Pistols", {
        {"Kill Confirmed", 504, 6}, {"Printstream", 1017, 6}, {"The Traitor", 757, 6},
        {"Orion", 313, 5}, {"Neo-Noir", 769, 5}, {"Road Rash", 490, 5},
        {"Ticket to Hell", 814, 5}, {"Cortex", 730, 4}, {"Caiman", 423, 4},
        {"Overgrowth", 338, 4}, {"Whiteout", 217, 4}, {"Blueprint", 657, 3},
        {"Serum", 265, 3}, {"Stainless", 218, 3}, {"Guardian", 290, 3}
    }, 15},
    {"Desert Eagle", 1, "Pistols", {
        {"Blaze", 37, 6}, {"Code Red", 661, 6}, {"Printstream", 1018, 6},
        {"Kumicho Dragon", 520, 5}, {"Mecha Industries", 641, 5}, {"Trigger Discipline", 808, 5},
        {"Heirloom", 175, 5}, {"Golden Koi", 62, 4}, {"Sunset Storm", 61, 4},
        {"Ocean Drive", 760, 4}, {"Fennec Fox", 688, 4}, {"Crimson Web", 44, 4},
        {"Hypnotic", 28, 4}, {"Conspiracy", 351, 3}, {"Oxide Blaze", 571, 3}
    }, 15},
    {"P250", 36, "Pistols", {
        {"See Ya Later", 660, 6}, {"Asiimov", 551, 5}, {"Muertos", 478, 4},
        {"Cartel", 452, 4}, {"Supernova", 413, 4}, {"Mehndi", 407, 4},
        {"Vino Primo", 401, 3}, {"Steel Disruption", 542, 3}, {"Splash", 5, 2}
    }, 9},
    {"Five-SeveN", 3, "Pistols", {
        {"Hyper Beast", 520, 6}, {"Angry Mob", 799, 5}, {"Monkey Business", 476, 4},
        {"Retrobution", 700, 4}, {"Copper Galaxy", 327, 4}, {"Neon Kimono", 458, 4},
        {"Flame Test", 361, 3}, {"Triumvirate", 310, 3}, {"Orange Peel", 100, 2}
    }, 9},
    {"Tec-9", 30, "Pistols", {
        {"Fuel Injector", 653, 6}, {"Decimator", 668, 5}, {"Re-Entry", 618, 4},
        {"Avalanche", 447, 4}, {"Titanium Bit", 602, 3}, {"Blue Titanium", 362, 3}
    }, 6},
    {"Dual Berettas", 63, "Pistols", {
        {"Twin Turbo", 770, 5}, {"Cobra Strike", 649, 4}, {"Royal Consorts", 544, 4},
        {"Hemoglobin", 293, 3}, {"Urban Shock", 252, 3}, {"Shred", 530, 2}
    }, 6},
    {"CZ75-Auto", 63, "Pistols", {
        {"Victoria", 467, 5}, {"Xiangliu", 680, 4}, {"Red Astor", 571, 4},
        {"Emerald", 362, 4}, {"Polymer", 396, 3}, {"Tuxedo", 297, 3}
    }, 6},
    {"R8 Revolver", 64, "Pistols", {
        {"Fade", 38, 6}, {"Amber Fade", 618, 4}, {"Grip", 620, 3},
        {"Bone Mask", 619, 3}, {"Crimson Web", 44, 4}, {"Llama Cannon", 680, 4}
    }, 6},

    // --- RIFLES ---
    {"AK-47", 7, "Rifles", {
        {"Wild Lotus", 856, 7}, {"Fire Serpent", 180, 6}, {"Fuel Injector", 653, 6},
        {"Neon Revolution", 656, 6}, {"Bloodsport", 796, 6}, {"The Empress", 754, 6},
        {"Vulcan", 349, 5}, {"Asiimov", 524, 5}, {"Neon Rider", 645, 5},
        {"Phantom Disruptor", 801, 5}, {"Redline", 282, 4}, {"Frontside Misty", 490, 4},
        {"Aquamarine Revenge", 475, 4}, {"Point Disarray", 506, 4}, {"Slate", 818, 3},
        {"Blue Laminate", 40, 3}, {"Elite Build", 429, 3}, {"Rat Rod", 525, 3},
        {"Safari Mesh", 72, 1}, {"Predator", 12, 1}
    }, 20},
    {"M4A4", 16, "Rifles", {
        {"Howl", 309, 7}, {"Asiimov", 255, 6}, {"Poseidon", 359, 6},
        {"The Emperor", 749, 6}, {"Neo-Noir", 770, 5}, {"Desolate Space", 648, 5},
        {"Buzz Kill", 659, 5}, {"Royal Paladin", 517, 5}, {"Hellfire", 731, 5},
        {"In Living Color", 809, 5}, {"Dragon King", 400, 4}, {"Evil Daimyo", 468, 4},
        {"Cyber Security", 326, 3}, {"Faded Zebra", 176, 2}, {"Urban DDPAT", 17, 2}
    }, 15},
    {"M4A1-S", 60, "Rifles", {
        {"Printstream", 1016, 6}, {"Chantico's Fire", 639, 6},
        {"Hyper Beast", 519, 5}, {"Golden Coil", 632, 5}, {"Mecha Industries", 641, 5},
        {"Decimator", 568, 5}, {"Nightmare", 773, 4}, {"Leaded Glass", 733, 4},
        {"Atomic Alloy", 397, 4}, {"Guardian", 290, 3}, {"Bright Water", 77, 3},
        {"Nitro", 254, 3}, {"Flashback", 568, 3}, {"Boreal Forest", 23, 2}
    }, 14},
    {"AWP", 9, "Rifles", {
        {"Dragon Lore", 344, 7}, {"Gungnir", 856, 7}, {"Fade", 38, 6},
        {"Lightning Strike", 141, 6}, {"Asiimov", 279, 5}, {"Hyper Beast", 520, 5},
        {"Containment Breach", 808, 5}, {"Fever Dream", 640, 5},
        {"Redline", 282, 4}, {"Chromatic Aberration", 665, 4}, {"Atheris", 771, 4},
        {"PAW", 680, 4}, {"Electric Hive", 65, 4}, {"Elite Build", 471, 3},
        {"Pit Viper", 213, 3}, {"Safari Mesh", 72, 1}
    }, 16},
    {"SG 553", 40, "Rifles", {
        {"Integrale", 792, 5}, {"Cyrex", 487, 4}, {"Pulse", 63, 3},
        {"Phantom", 371, 3}, {"Aerial", 669, 4}, {"Tiger Moth", 707, 4}
    }, 6},
    {"AUG", 8, "Rifles", {
        {"Akihabara Accept", 551, 6}, {"Chameleon", 705, 5}, {"Syd Mead", 707, 4},
        {"Stymphalian", 536, 4}, {"Fleet Flock", 693, 4}, {"Midnight Lily", 509, 4}
    }, 6},
    {"FAMAS", 10, "Rifles", {
        {"Mecha Industries", 641, 5}, {"Roll Cage", 691, 4}, {"Djinn", 501, 4},
        {"Neural Net", 521, 3}, {"Afterimage", 412, 4}, {"Decommissioned", 608, 4}
    }, 6},
    {"Galil AR", 13, "Rifles", {
        {"Chatterbox", 460, 5}, {"Cerberus", 476, 4}, {"Firefight", 553, 4},
        {"Rocket Pop", 564, 4}, {"Stone Cold", 530, 3}, {"Sage Spray", 42, 2}
    }, 6},

    // --- SMGs ---
    {"MP9", 34, "SMGs", {
        {"Hydra", 695, 5}, {"Starlight Protector", 733, 4},
        {"Rose Iron", 634, 3}, {"Bioleak", 524, 3}, {"Dark Age", 371, 2}
    }, 5},
    {"MAC-10", 17, "SMGs", {
        {"Neon Rider", 645, 5}, {"Disco Tech", 759, 4}, {"Stalker", 521, 3},
        {"Heat", 467, 4}, {"Pipe Down", 680, 4}, {"Silver", 300, 2}
    }, 6},
    {"UMP-45", 24, "SMGs", {
        {"Primal Saber", 617, 5}, {"Momentum", 656, 4}, {"Briefing", 455, 3},
        {"Plastique", 547, 3}, {"Corporal", 341, 3}, {"Caramel", 10, 1}
    }, 6},
    {"MP7", 33, "SMGs", {
        {"Nemesis", 395, 5}, {"Fade", 38, 4}, {"Bloodsport", 607, 4},
        {"Impire", 470, 3}, {"Akoben", 655, 4}, {"Special Delivery", 432, 3}
    }, 6},
    {"P90", 19, "SMGs", {
        {"Asiimov", 399, 5}, {"Death by Kitty", 144, 5}, {"Shapewood", 536, 4},
        {"Trigon", 254, 4}, {"Module", 534, 3}, {"Elite Build", 471, 3}
    }, 6},
    {"PP-Bizon", 26, "SMGs", {
        {"Judgement of Anubis", 665, 5}, {"Fuel Rod", 517, 3},
        {"Blue Streak", 416, 3}, {"Night Ops", 223, 2}, {"Sand Dashed", 29, 1}
    }, 5},

    // --- HEAVY ---
    {"Nova", 35, "Heavy", {
        {"Hyper Beast", 520, 5}, {"Antique", 302, 4}, {"Bloomstick", 654, 4},
        {"Toy Soldier", 717, 4}, {"Koi", 68, 3}, {"Sand Dune", 208, 1}
    }, 6},
    {"XM1014", 25, "Heavy", {
        {"Ziggy", 681, 5}, {"Incinegator", 645, 4}, {"Tranquility", 523, 4},
        {"Teclu Burner", 533, 4}, {"Slipstream", 374, 3}, {"Blue Steel", 31, 2}
    }, 6},
    {"MAG-7", 27, "Heavy", {
        {"Justice", 554, 5}, {"SWAG-7", 661, 4}, {"Praetorian", 531, 4},
        {"Heat", 467, 4}, {"Metallic DDPAT", 200, 3}, {"Sand Dune", 208, 1}
    }, 6},
    {"Negev", 28, "Heavy", {
        {"Mjolnir", 689, 6}, {"Power Loader", 682, 5}, {"Dazzle", 471, 3},
        {"Bratatat", 534, 3}, {"Desert Strike", 371, 3}, {"Army Sheen", 298, 2}
    }, 6},
    {"M249", 14, "Heavy", {
        {"Emerald Poison Dart", 680, 5}, {"Spectre", 453, 4},
        {"System Lock", 522, 4}, {"Downtown", 468, 3}, {"Gator Mesh", 75, 2}
    }, 5},

    // --- KNIVES ---
    {"Karambit", 507, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Marble Fade", 413, 6}, {"Gamma Doppler", 568, 6}, {"Autotronic", 619, 6},
        {"Lore", 344, 6}, {"Crimson Web", 44, 5}, {"Slaughter", 59, 5},
        {"Case Hardened", 44, 5}, {"Blue Steel", 31, 4}, {"Night", 100, 4},
        {"Vanilla", 0, 4}, {"Stained", 43, 4}, {"Urban Masked", 105, 3},
        {"Boreal Forest", 23, 3}, {"Scorched", 182, 3}, {"Safari Mesh", 72, 3}
    }, 18},
    {"Butterfly Knife", 515, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Marble Fade", 413, 6}, {"Gamma Doppler", 568, 6}, {"Lore", 344, 6},
        {"Slaughter", 59, 5}, {"Crimson Web", 44, 5}, {"Case Hardened", 44, 5},
        {"Blue Steel", 31, 4}, {"Night", 100, 4}, {"Vanilla", 0, 4},
        {"Boreal Forest", 23, 3}, {"Scorched", 182, 3}, {"Safari Mesh", 72, 3}
    }, 15},
    {"Bayonet", 500, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Marble Fade", 413, 6}, {"Lore", 344, 6}, {"Autotronic", 619, 6},
        {"Slaughter", 59, 5}, {"Crimson Web", 44, 5}, {"Case Hardened", 44, 5},
        {"Blue Steel", 31, 4}, {"Vanilla", 0, 4}, {"Boreal Forest", 23, 3}
    }, 12},
    {"M9 Bayonet", 508, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Marble Fade", 413, 6}, {"Gamma Doppler", 568, 6}, {"Lore", 344, 6},
        {"Crimson Web", 44, 5}, {"Slaughter", 59, 5}, {"Case Hardened", 44, 5},
        {"Blue Steel", 31, 4}, {"Vanilla", 0, 4}, {"Night", 100, 4}
    }, 12},
    {"Flip Knife", 505, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Marble Fade", 413, 6}, {"Lore", 344, 6}, {"Autotronic", 619, 6},
        {"Slaughter", 59, 5}, {"Crimson Web", 44, 5}, {"Vanilla", 0, 4},
        {"Boreal Forest", 23, 3}, {"Scorched", 182, 3}
    }, 11},
    {"Huntsman Knife", 509, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Lore", 344, 6}, {"Slaughter", 59, 5}, {"Crimson Web", 44, 5},
        {"Case Hardened", 44, 5}, {"Blue Steel", 31, 4}, {"Vanilla", 0, 4}
    }, 9},
    {"Gut Knife", 506, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Crimson Web", 44, 5}, {"Slaughter", 59, 5}, {"Vanilla", 0, 4},
        {"Night", 100, 4}, {"Boreal Forest", 23, 3}
    }, 8},
    {"Falchion Knife", 512, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Lore", 344, 6},
        {"Crimson Web", 44, 5}, {"Slaughter", 59, 5}, {"Case Hardened", 44, 5},
        {"Vanilla", 0, 4}, {"Boreal Forest", 23, 3}
    }, 8},
    {"Shadow Daggers", 516, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Crimson Web", 44, 5},
        {"Slaughter", 59, 5}, {"Case Hardened", 44, 5}, {"Vanilla", 0, 4},
        {"Blue Steel", 31, 4}, {"Night", 100, 4}
    }, 8},
    {"Bowie Knife", 514, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Crimson Web", 44, 5},
        {"Slaughter", 59, 5}, {"Vanilla", 0, 4}, {"Blue Steel", 31, 4}
    }, 6},
    {"Navaja Knife", 520, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Crimson Web", 44, 5}, {"Vanilla", 0, 4}, {"Boreal Forest", 23, 3}
    }, 6},
    {"Stiletto Knife", 522, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Crimson Web", 44, 5}, {"Vanilla", 0, 4}, {"Blue Steel", 31, 4}
    }, 6},
    {"Talon Knife", 523, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Marble Fade", 413, 6}, {"Crimson Web", 44, 5}, {"Vanilla", 0, 4}
    }, 6},
    {"Ursus Knife", 519, "Knives", {
        {"Fade", 38, 6}, {"Doppler", 418, 6}, {"Tiger Tooth", 409, 6},
        {"Crimson Web", 44, 5}, {"Vanilla", 0, 4}, {"Night", 100, 4}
    }, 6},
    {"Classic Knife", 503, "Knives", {
        {"Fade", 38, 6}, {"Crimson Web", 44, 5}, {"Slaughter", 59, 5},
        {"Case Hardened", 44, 5}, {"Vanilla", 0, 4}, {"Blue Steel", 31, 4}
    }, 6},
    {"Paracord Knife", 517, "Knives", {
        {"Fade", 38, 6}, {"Crimson Web", 44, 5}, {"Slaughter", 59, 5},
        {"Vanilla", 0, 4}, {"Boreal Forest", 23, 3}, {"Stained", 43, 4}
    }, 6},
    {"Survival Knife", 518, "Knives", {
        {"Fade", 38, 6}, {"Crimson Web", 44, 5}, {"Slaughter", 59, 5},
        {"Vanilla", 0, 4}, {"Night", 100, 4}, {"Boreal Forest", 23, 3}
    }, 6},
    {"Nomad Knife", 521, "Knives", {
        {"Fade", 38, 6}, {"Crimson Web", 44, 5}, {"Slaughter", 59, 5},
        {"Vanilla", 0, 4}, {"Blue Steel", 31, 4}, {"Boreal Forest", 23, 3}
    }, 6},
    {"Skeleton Knife", 525, "Knives", {
        {"Fade", 38, 6}, {"Crimson Web", 44, 5}, {"Slaughter", 59, 5},
        {"Case Hardened", 44, 5}, {"Vanilla", 0, 4}, {"Stained", 43, 4}
    }, 6},

    // --- GLOVES ---
    {"Sport Gloves", 5030, "Gloves", {
        {"Pandora's Box", 10006, 6}, {"Hedge Maze", 10007, 6}, {"Superconductor", 10008, 6},
        {"Arid", 10009, 5}, {"Vice", 10036, 6}, {"Omega", 10037, 5},
        {"Amphibious", 10038, 5}, {"Bronze Morph", 10039, 5}, {"Slingshot", 10085, 5},
        {"Scarlet Shamagh", 10086, 5}
    }, 10},
    {"Driver Gloves", 5031, "Gloves", {
        {"Crimson Weave", 10010, 6}, {"Imperial Plaid", 10027, 5},
        {"Lunar Weave", 10011, 5}, {"Diamondback", 10012, 5},
        {"King Snake", 10040, 5}, {"Queen Jaguar", 10087, 5},
        {"Overtake", 10041, 5}, {"Racing Green", 10042, 5}
    }, 8},
    {"Hand Wraps", 5032, "Gloves", {
        {"Cobalt Skulls", 10013, 6}, {"Overprint", 10043, 5},
        {"Slaughter", 10014, 5}, {"Leather", 10015, 5},
        {"Duct Tape", 10016, 5}, {"Arboreal", 10017, 5},
        {"Constrictor", 10044, 5}, {"Desert Shamagh", 10045, 5}
    }, 8},
    {"Moto Gloves", 5033, "Gloves", {
        {"Spearmint", 10018, 6}, {"Cool Mint", 10019, 5},
        {"POW!", 10020, 5}, {"Boom!", 10021, 5},
        {"Eclipse", 10022, 5}, {"Polygon", 10046, 5},
        {"Transport", 10047, 5}, {"Turtle", 10048, 5}
    }, 8},
    {"Specialist Gloves", 5034, "Gloves", {
        {"Crimson Kimono", 10023, 6}, {"Emerald Web", 10024, 6},
        {"Foundation", 10025, 5}, {"Mogul", 10026, 5},
        {"Fade", 10028, 6}, {"Marble Fade", 10049, 6},
        {"Forest DDPAT", 10029, 5}, {"Buckshot", 10050, 5}
    }, 8},
    {"Hydra Gloves", 5035, "Gloves", {
        {"Emerald", 10030, 6}, {"Case Hardened", 10031, 5},
        {"Rattler", 10032, 5}, {"Mangrove", 10033, 5}
    }, 4},
    {"Broken Fang Gloves", 5036, "Gloves", {
        {"Jade", 10088, 6}, {"Yellow-banded", 10089, 5},
        {"Unhinged", 10090, 5}, {"Needle Point", 10091, 5}
    }, 4},
};

static const int g_weaponDBCount = sizeof(g_weaponDB) / sizeof(g_weaponDB[0]);

// CS2 Menu — Skin changer only (no sidebar nav needed)

// CS2 Menu — Inventory view states
enum InvViewState {
    INV_VIEW_MAIN = 0,       // Main inventory with added items + "+" button
    INV_VIEW_CATEGORIES = 1, // Category grid (Pistol, Rifle, etc.)
    INV_VIEW_WEAPONS = 2,    // Weapon grid for selected category
    INV_VIEW_SKINS = 3       // Skin grid for selected weapon + detail panel
};
static int g_invView = INV_VIEW_MAIN;

// Inventory items
struct InvItem {
    int weaponIdx;   // index into g_weaponDB
    int skinIdx;     // index into weapon's skins array
    float wear;      // 0.0-1.0
    bool statTrak;
    char customName[32];
    bool applied;    // whether it's been applied to CS2
};
static std::vector<InvItem> g_invItems;

// Category system
struct CategoryDef {
    const char* name;
    const char* filterKey; // matches weapon category in DB
};
static const CategoryDef g_categories[] = {
    {"Pistol",  "Pistols"},
    {"Rifle",   "Rifles"},
    {"Heavy",   "Heavy"},
    {"SMG",     "SMGs"},
    {"Knife",   "Knives"},
    {"Glove",   "Gloves"},
};
static const int g_categoryCount = 6;
static int g_selectedCategory = -1;    // index into g_categories
static int g_selectedWeapon = -1;      // index into g_weaponDB
static int g_selectedSkin = -1;        // index in weapon's skin list
static int g_weaponScrollY = 0;
static int g_skinScrollY = 0;
static float g_detailWear = 0.0f;      // Wear slider value
static bool g_detailStatTrak = false;
static char g_detailCustomName[32] = "";

// Sidebar tab
static int g_sideTab = 1; // 0=Profile, 1=Items, 2=Configs

// Skin image texture cache
static std::unordered_map<std::string, TextureHandle> g_skinTexCache;
static std::unordered_map<std::string, bool> g_skinTexFailed; // mark failed loads
static std::unordered_map<std::string, std::string> g_skinImageUrls; // weapon|skin → image URL
static bool g_skinApiLoaded = false;

// Forward declaration — defined later in file
static std::string GetBasePath();

// Skin cache directory in %APPDATA%
static std::string GetSkinCacheDir() {
    char ad[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, ad))) {
        std::string d = std::string(ad) + "\\AC_Changer\\skin_cache";
        CreateDirectoryA((std::string(ad) + "\\AC_Changer").c_str(), nullptr);
        CreateDirectoryA(d.c_str(), nullptr);
        return d + "\\";
    }
    return "";
}

// Download + parse CSGO-API skins.json for Steam CDN image URLs
static void EnsureSkinApiLoaded() {
    if (g_skinApiLoaded) return;
    g_skinApiLoaded = true;
    std::string cd = GetSkinCacheDir();
    if (cd.empty()) return;
    std::string jp = cd + "skins_api.json";
    if (GetFileAttributesA(jp.c_str()) == INVALID_FILE_ATTRIBUTES) {
        URLDownloadToFileA(nullptr,
            "https://raw.githubusercontent.com/ByMykel/CSGO-API/main/public/api/en/skins.json",
            jp.c_str(), 0, nullptr);
    }
    FILE* jf = fopen(jp.c_str(), "rb");
    if (!jf) return;
    fseek(jf, 0, SEEK_END); long jsz = ftell(jf); fseek(jf, 0, SEEK_SET);
    if (jsz <= 0 || jsz > 50*1024*1024) { fclose(jf); return; }
    std::string jbuf(jsz, '\0');
    fread(&jbuf[0], 1, jsz, jf); fclose(jf);
    try {
        auto arr = nlohmann::json::parse(jbuf);
        for (auto& s : arr) {
            std::string wn, sn, img;
            if (s.contains("weapon") && s["weapon"].contains("name"))
                wn = s["weapon"]["name"].get<std::string>();
            if (s.contains("pattern") && s["pattern"].contains("name"))
                sn = s["pattern"]["name"].get<std::string>();
            if (s.contains("image"))
                img = s["image"].get<std::string>();
            if (!wn.empty() && !sn.empty() && !img.empty())
                g_skinImageUrls[wn + "|" + sn] = img;
        }
    } catch (...) {}
}

static TextureHandle LoadSkinImage(const char* weaponName, const char* skinName) {
    // Build key: WeaponName_SkinName (spaces → underscores, remove apostrophes)
    std::string key = weaponName;
    key += "_";
    key += skinName;
    for (auto& c : key) { if (c == ' ') c = '_'; }
    std::string clean;
    for (char c : key) { if (c != '\'') clean += c; }
    key = clean;

    // Check caches
    auto it = g_skinTexCache.find(key);
    if (it != g_skinTexCache.end()) return it->second;
    if (g_skinTexFailed.count(key)) return INVALID_TEXTURE;

    std::string path;

    // 1. Try local assets/skins/ relative to exe
    {
        std::string lp = GetBasePath() + "assets\\skins\\" + key + ".png";
        FILE* tf = fopen(lp.c_str(), "rb");
        if (tf) { fclose(tf); path = lp; }
    }

    // 2. Try cached download in %APPDATA%
    if (path.empty()) {
        std::string cd = GetSkinCacheDir();
        if (!cd.empty()) {
            std::string cp = cd + key + ".png";
            FILE* tf = fopen(cp.c_str(), "rb");
            if (tf) { fclose(tf); path = cp; }
        }
    }

    // 3. Download from Steam CDN via CSGO-API mapping
    if (path.empty()) {
        EnsureSkinApiLoaded();
        auto urlIt = g_skinImageUrls.find(std::string(weaponName) + "|" + skinName);
        if (urlIt != g_skinImageUrls.end()) {
            std::string cd = GetSkinCacheDir();
            if (!cd.empty()) {
                std::string cp = cd + key + ".png";
                HRESULT hr = URLDownloadToFileA(nullptr, urlIt->second.c_str(),
                    cp.c_str(), 0, nullptr);
                if (SUCCEEDED(hr)) path = cp;
            }
        }
    }

    if (path.empty()) { g_skinTexFailed[key] = true; return INVALID_TEXTURE; }

    // Load image file
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { g_skinTexFailed[key] = true; return INVALID_TEXTURE; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 4*1024*1024) { fclose(f); g_skinTexFailed[key] = true; return INVALID_TEXTURE; }
    std::vector<unsigned char> buf(sz);
    fread(buf.data(), 1, sz, f); fclose(f);

    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = stbi_load_from_memory(buf.data(), (int)sz, &w, &h, &ch, 4);
    if (!pixels || w <= 0 || h <= 0) { g_skinTexFailed[key] = true; return INVALID_TEXTURE; }

    TextureDesc td{}; td.width = (u32)w; td.height = (u32)h; td.channels = 4; td.data = pixels;
    TextureHandle tex = g_backend.CreateTexture(td);
    stbi_image_free(pixels);
    if (tex != INVALID_TEXTURE) g_skinTexCache[key] = tex;
    else g_skinTexFailed[key] = true;
    return tex;
}

// Write equipped skins to .acpreset file so the DLL can apply them
static void WriteEquippedConfig() {
    // Get %APPDATA%\AC_Changer\presets\ path
    char appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
        std::string dir = std::string(appData) + "\\AC_Changer\\presets";
        CreateDirectoryA((std::string(appData) + "\\AC_Changer").c_str(), nullptr);
        CreateDirectoryA(dir.c_str(), nullptr);
        std::string path = dir + "\\active.acpreset";
        FILE* f = fopen(path.c_str(), "w");
        if (f) {
            for (auto& item : g_invItems) {
                if (item.weaponIdx < 0 || item.weaponIdx >= g_weaponDBCount) continue;
                const auto& wpn = g_weaponDB[item.weaponIdx];
                if (item.skinIdx < 0 || item.skinIdx >= wpn.skinCount) continue;
                const auto& sk = wpn.skins[item.skinIdx];
                fprintf(f, "%s|%s|%d|%d|0|%.4f|%d\n",
                    wpn.name, sk.name, wpn.id, sk.paintKit,
                    item.wear, item.statTrak ? 0 : -1);
            }
            fclose(f);
        }
    }
}

// CS2 Menu — Configs tab state
static int g_configSelected = 0;
static char g_configName[64] = "";
static const char* g_configSlots[] = {"Config 1", "Config 2", "Config 3", "Config 4", "Config 5"};
static bool g_configSaved[5] = {false, false, false, false, false};

// (World/View/Profile/Misc settings removed — skin changer only)

// ============================================================================
// USER DATABASE
// ============================================================================
static std::string GetBasePath() {
    char p[MAX_PATH]; GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    size_t i = s.find_last_of('\\');
    return (i != std::string::npos ? s.substr(0, i + 1) : "");
}
static std::string GetLicensePath() { return GetBasePath() + "licenses.dat"; }
static std::string GetKeysPath()    { return GetBasePath() + "license_keys.dat"; }
static std::string GetRememberPath(){ return GetBasePath() + "remember.dat"; }

static void SaveRememberMe() {
    std::ofstream f(GetRememberPath());
    if (g_rememberMe && strlen(g_loginUser) > 0) {
        f << g_loginUser << "\n" << g_loginPass << "\n";
    }
}
static void LoadRememberMe() {
    std::ifstream f(GetRememberPath());
    if (!f.is_open()) return;
    std::string user, pass;
    if (std::getline(f, user) && std::getline(f, pass) && !user.empty()) {
        strncpy(g_savedUser, user.c_str(), 63); g_savedUser[63] = '\0';
        strncpy(g_savedPass, pass.c_str(), 63); g_savedPass[63] = '\0';
        strncpy(g_loginUser, user.c_str(), 63); g_loginUser[63] = '\0';
        strncpy(g_loginPass, pass.c_str(), 63); g_loginPass[63] = '\0';
        g_rememberMe = true;
    }
}
static void ClearRememberMe() {
    DeleteFileA(GetRememberPath().c_str());
    g_rememberMe = false;
    memset(g_savedUser, 0, 64);
    memset(g_savedPass, 0, 64);
}

static std::string HashPw(const std::string& pw) {
    uint32_t h = 0x811c9dc5;
    for (char c : pw) { h ^= (uint8_t)c; h *= 0x01000193; }
    char b[16]; snprintf(b, 16, "%08X", h); return b;
}

// ============================================================================
// LICENSE KEY SYSTEM
// ============================================================================
struct LicenseKey { std::string key; int days; bool used; std::string usedBy; };
static std::vector<LicenseKey> g_keys;
static std::string g_testKey; // displayed after first-run seed

static void LoadKeys() {
    g_keys.clear();
    std::ifstream f(GetKeysPath());
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line); LicenseKey k;
        ss >> k.key >> k.days >> k.used >> k.usedBy;
        if (!k.key.empty()) g_keys.push_back(k);
    }
}
static void SaveKeys() {
    std::ofstream f(GetKeysPath());
    for (auto& k : g_keys)
        f << k.key << " " << k.days << " " << k.used << " "
          << (k.usedBy.empty() ? "-" : k.usedBy) << "\n";
}

static std::string GenerateKeyString() {
    char k[48]; snprintf(k, 48, "AC-%04X-%04X-%04X-%04X",
        rand()&0xFFFF, rand()&0xFFFF, rand()&0xFFFF, rand()&0xFFFF); return k;
}

static void SeedTestKeys() {
    // If no keys exist, create a default test key
    if (g_keys.empty()) {
        LicenseKey tk;
        tk.key  = "AC-TEST-2026-FREE-DEMO";
        tk.days = 30;
        tk.used = false;
        tk.usedBy = "-";
        g_keys.push_back(tk);

        // Generate a few more random keys
        for (int i = 0; i < 3; i++) {
            LicenseKey rk;
            rk.key  = GenerateKeyString();
            rk.days = 30;
            rk.used = false;
            rk.usedBy = "-";
            g_keys.push_back(rk);
        }
        SaveKeys();
        g_testKey = "AC-TEST-2026-FREE-DEMO";
        Log("Seeded test key: %s", g_testKey.c_str());
    }
}

static bool ValidateKey(const char* inputKey, int& outDays) {
    for (auto& k : g_keys) {
        if (k.key == inputKey) {
            if (k.used) return false; // already redeemed
            outDays = k.days;
            return true;
        }
    }
    return false;
}

static void RedeemKey(const char* inputKey, const char* user) {
    for (auto& k : g_keys) {
        if (k.key == inputKey) {
            k.used = true;
            k.usedBy = user;
            break;
        }
    }
    SaveKeys();
}

// ============================================================================
// USER DATABASE
// ============================================================================
struct UserRec { std::string user, hash, key; time_t created, expiry; bool active; };
static std::vector<UserRec> g_users;

static void LoadUsers() {
    g_users.clear();
    std::ifstream f(GetLicensePath());
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line); UserRec u;
        ss >> u.user >> u.hash >> u.key >> u.created >> u.expiry >> u.active;
        if (!u.user.empty()) g_users.push_back(u);
    }
}
static void SaveUsers() {
    std::ofstream f(GetLicensePath());
    for (auto& u : g_users)
        f << u.user << " " << u.hash << " " << u.key
          << " " << u.created << " " << u.expiry << " " << u.active << "\n";
}

static bool DoSignup(const char* user, const char* pass, const char* licenseKey) {
    if (strlen(user) < 3) { g_authError = "Username: min 3 chars"; g_authErrorTimer = 3; return false; }
    if (strlen(pass) < 4) { g_authError = "Password: min 4 chars"; g_authErrorTimer = 3; return false; }
    if (strlen(licenseKey) < 4) { g_authError = "Enter a valid license key"; g_authErrorTimer = 3; return false; }
    for (auto& u : g_users)
        if (u.user == user) { g_authError = "Username taken"; g_authErrorTimer = 3; return false; }

    // Validate license key
    int days = 0;
    if (!ValidateKey(licenseKey, days)) {
        g_authError = "Invalid or used license key"; g_authErrorTimer = 3; return false;
    }

    // Create user and redeem key
    UserRec u;
    u.user = user;
    u.hash = HashPw(pass);
    u.key  = licenseKey;
    u.created = time(nullptr);
    u.expiry  = u.created + days * 86400;
    u.active  = true;
    g_users.push_back(u);
    SaveUsers();
    RedeemKey(licenseKey, user);
    return true;
}

static bool DoLogin(const char* user, const char* pass) {
    std::string h = HashPw(pass);
    for (auto& u : g_users) {
        if (u.user == user && u.hash == h) {
            if (!u.active) { g_authError = "Account deactivated"; g_authErrorTimer = 3; return false; }
            if (time(nullptr) > u.expiry) { g_authError = "Subscription expired"; g_authErrorTimer = 3; return false; }
            g_loggedUser = user;
            int dl = (int)((u.expiry - time(nullptr)) / 86400);
            char buf[64]; snprintf(buf, 64, "%d days remaining", dl > 0 ? dl : 0);
            g_sub = {"Counter-Strike 2", buf, dl > 0 ? dl : 0, true};
            g_hasSub = true;
            return true;
        }
    }
    g_authError = "Invalid credentials"; g_authErrorTimer = 3; return false;
}

// ============================================================================
// INJECTION
// ============================================================================
static DWORD FindProc(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do { if (_stricmp(pe.szExeFile, name) == 0) { CloseHandle(snap); return pe.th32ProcessID; }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap); return 0;
}
static bool Inject(DWORD pid, const char* dll) {
    HANDLE hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hp) return false;
    size_t len = strlen(dll) + 1;
    LPVOID rm = VirtualAllocEx(hp, nullptr, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!rm) { CloseHandle(hp); return false; }
    if (!WriteProcessMemory(hp, rm, dll, len, nullptr)) {
        VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp); return false;
    }
    LPVOID ll = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!ll) { VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp); return false; }
    HANDLE ht = CreateRemoteThread(hp, nullptr, 0, (LPTHREAD_START_ROUTINE)ll, rm, 0, nullptr);
    if (!ht) { VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp); return false; }
    WaitForSingleObject(ht, 10000);
    DWORD ec = 0; GetExitCodeThread(ht, &ec);
    CloseHandle(ht); VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp);
    return ec != 0;
}
// ============================================================================
// STEAM INTEGRATION
// ============================================================================
static bool IsSteamRunning() {
    return FindProc("steam.exe") != 0;
}
static void CloseSteam() {
    // Graceful shutdown via Steam command
    ShellExecuteA(nullptr, "open", "taskkill", "/IM steam.exe /F", nullptr, SW_HIDE);
    Log("Steam: sent close signal");
}
static void StartSteam() {
    // Try default Steam path
    const char* steamPaths[] = {
        "C:\\Program Files (x86)\\Steam\\steam.exe",
        "C:\\Program Files\\Steam\\steam.exe",
        "D:\\Steam\\steam.exe",
    };
    for (auto* path : steamPaths) {
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
            ShellExecuteA(nullptr, "open", path, nullptr, nullptr, SW_SHOW);
            Log("Steam: started from %s", path);
            return;
        }
    }
    // Fallback: try steam:// protocol
    ShellExecuteA(nullptr, "open", "steam://open/main", nullptr, nullptr, SW_SHOW);
    Log("Steam: started via protocol");
}

// ============================================================================
// INJECTION WITH STEAM/CS2 FLOW
// ============================================================================
static void DoInject() {
    if (g_injPhase != INJ_IDLE) return;
    g_injecting = true;
    g_injProgress = 0;
    g_injTimer = 0;

    // Step 1: Check/close Steam
    if (IsSteamRunning()) {
        g_steamWasRunning = true;
        g_injPhase = INJ_CLOSING_STEAM;
        CloseSteam();
        g_toastMsg = "Closing Steam..."; g_toastCol = P::Yellow; g_toastTimer = 3;
    } else {
        g_steamWasRunning = false;
        g_injPhase = INJ_STARTING_STEAM;
        StartSteam();
        g_toastMsg = "Starting Steam..."; g_toastCol = P::Yellow; g_toastTimer = 3;
    }
}

static void UpdateInjectionFlow() {
    if (g_injPhase == INJ_IDLE || g_injPhase == INJ_DONE || g_injPhase == INJ_CS2_MENU) return;
    g_injTimer += g_dt;

    switch (g_injPhase) {
    case INJ_CLOSING_STEAM:
        if (!IsSteamRunning() || g_injTimer > 5.0f) {
            g_injPhase = INJ_STARTING_STEAM;
            g_injTimer = 0;
            StartSteam();
            g_toastMsg = "Starting Steam..."; g_toastCol = P::Yellow; g_toastTimer = 3;
        }
        break;

    case INJ_STARTING_STEAM:
        if (g_injTimer > 3.0f) {
            // Auto-launch CS2 via Steam
            ShellExecuteA(nullptr, "open", "steam://rungameid/730", nullptr, nullptr, SW_SHOW);
            Log("CS2: launching via steam://rungameid/730");
            g_toastMsg = "Starting CS2..."; g_toastCol = P::Yellow; g_toastTimer = 5;
            g_injPhase = INJ_WAIT_CS2;
            g_injTimer = 0;
        }
        break;

    case INJ_WAIT_CS2: {
        DWORD pid = FindProc("cs2.exe");
        if (pid != 0) {
            g_cs2Pid = pid;
            g_injPhase = INJ_WAIT_CS2_READY;
            g_injTimer = 0;
            g_toastMsg = "CS2 fundet — venter p\xc3\xa5 hovedmenu..."; g_toastCol = P::Yellow; g_toastTimer = 10;
            Log("CS2 process found (PID %lu), waiting for main menu...", pid);
        } else if (g_injTimer > 120.0f) {
            g_toastMsg = "Timeout: CS2 ikke fundet"; g_toastCol = P::Red; g_toastTimer = 4;
            g_injecting = false;
            g_injPhase = INJ_IDLE;
        }
        break;
    }

    case INJ_WAIT_CS2_READY: {
        // Wait for CS2 window to appear and be visible (main menu loaded)
        // CS2 takes ~15-30s to reach main menu after process starts
        g_cs2Hwnd = nullptr;
        struct EnumData { DWORD pid; HWND result; };
        EnumData ed{g_cs2Pid, nullptr};
        EnumWindows([](HWND hw, LPARAM lp) -> BOOL {
            auto* d = (EnumData*)lp;
            DWORD wp = 0; GetWindowThreadProcessId(hw, &wp);
            if (wp == d->pid && IsWindowVisible(hw)) {
                char cls[64]; GetClassNameA(hw, cls, 64);
                if (strstr(cls, "SDL") || GetWindow(hw, GW_OWNER) == nullptr) {
                    RECT r; GetWindowRect(hw, &r);
                    if ((r.right - r.left) > 800 && (r.bottom - r.top) > 600) {
                        d->result = hw;
                        return FALSE;
                    }
                }
            }
            return TRUE;
        }, (LPARAM)&ed);

        bool windowReady = (ed.result != nullptr);

        // Need CS2 window AND at least 20 seconds — then inject directly (no overlay)
        if (windowReady && g_injTimer > 20.0f) {
            g_cs2Hwnd = ed.result;
            GetWindowRect(g_hwnd, &g_origWindowRect);

            g_hasInjected = true;
            g_injPhase = INJ_INJECTING;
            g_injTimer = 0;
            Log("CS2 ready after 20s — injecting directly (no overlay)");
        } else if (g_injTimer > 90.0f) {
            // Timeout waiting for CS2 window
            g_toastMsg = "Timeout: CS2 hovedmenu ikke fundet"; g_toastCol = P::Red; g_toastTimer = 4;
            g_injecting = false;
            g_injPhase = INJ_IDLE;
        }
        break;
    }

    case INJ_INJECTING: {
        // Actually inject the DLL
        DWORD pid = FindProc("cs2.exe");
        if (!pid) {
            g_toastMsg = "CS2 lukkede uventet"; g_toastCol = P::Red; g_toastTimer = 4;
            g_injPhase = INJ_IDLE; g_injecting = false; return;
        }

        HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDR_SKIN_DLL), RT_RCDATA);
        if (!hRes) { g_toastMsg = "DLL resource not found"; g_toastCol = P::Red; g_toastTimer = 4; g_injPhase = INJ_IDLE; g_injecting = false; return; }
        HGLOBAL hGlob = LoadResource(nullptr, hRes);
        DWORD dwSize = SizeofResource(nullptr, hRes);
        const void* pData = LockResource(hGlob);
        if (!pData || dwSize == 0) { g_toastMsg = "Invalid DLL resource"; g_toastCol = P::Red; g_toastTimer = 4; g_injPhase = INJ_IDLE; g_injecting = false; return; }

        char tempDir[MAX_PATH];
        GetTempPathA(MAX_PATH, tempDir);
        std::string dllPath = std::string(tempDir) + "skin_changer.dll";
        HANDLE hFile = CreateFileA(dllPath.c_str(), GENERIC_WRITE, 0, nullptr,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            g_toastMsg = "Failed to extract DLL"; g_toastCol = P::Red; g_toastTimer = 4; g_injPhase = INJ_IDLE; g_injecting = false; return;
        }
        DWORD written = 0;
        WriteFile(hFile, pData, dwSize, &written, nullptr);
        CloseHandle(hFile);

        if (Inject(pid, dllPath.c_str())) {
            g_toastMsg = "Injected!"; g_toastCol = P::Green; g_injected = true;
            Log("Injection successful into PID %lu", pid);
            Beep(1200, 200); // Success beep — the ONLY sound feedback
        } else {
            g_toastMsg = "Injection fejlede"; g_toastCol = P::Orange;
            Log("Injection FAILED into PID %lu (GetLastError=%lu)", pid, GetLastError());
            Beep(400, 300); // Error beep
        }
        g_toastTimer = 4;

        // Ensure WS_EX_LAYERED + WS_EX_TRANSPARENT are cleared (clean window)
        {
            LONG exStyle = GetWindowLongA(g_hwnd, GWL_EXSTYLE);
            exStyle &= ~(WS_EX_LAYERED | WS_EX_TRANSPARENT);
            SetWindowLongA(g_hwnd, GWL_EXSTYLE, exStyle);
        }

        // Transition to CS2 menu — VISIBLE immediately after beep
        {
            int menuW = (int)(788 * g_dpiScale), menuH = (int)(638 * g_dpiScale);
            int mx, my;
            if (g_cs2Hwnd) {
                RECT cs2r; GetWindowRect(g_cs2Hwnd, &cs2r);
                int cs2w = cs2r.right - cs2r.left;
                int cs2h = cs2r.bottom - cs2r.top;
                mx = cs2r.left + (cs2w - menuW) / 2;
                my = cs2r.top  + (cs2h - menuH) / 2;
            } else {
                mx = (GetSystemMetrics(SM_CXSCREEN) - menuW) / 2;
                my = (GetSystemMetrics(SM_CYSCREEN) - menuH) / 2;
            }
            SetWindowPos(g_hwnd, HWND_TOPMOST, mx, my, menuW, menuH, SWP_SHOWWINDOW);
            g_width = menuW; g_height = menuH;
            g_backend.Resize((u32)menuW, (u32)menuH);
            HRGN rgn2 = CreateRoundRectRgn(0, 0, menuW+1, menuH+1, CORNER*2, CORNER*2);
            SetWindowRgn(g_hwnd, rgn2, TRUE);
            Log("CS2 menu: %dx%d at (%d,%d)", menuW, menuH, mx, my);
        }
        // Auto-open menu right after beep
        g_cs2MenuVisible = true;
        g_menuOpenAnim = 0.0f; // Start animation from 0 → 1
        ShowWindow(g_hwnd, SW_SHOWNOACTIVATE);
        g_injPhase = INJ_CS2_MENU;
        g_showPopup = false;
        g_injecting = false;
        break;
    }

    default: break;
    }
}

// Forward declaration for DrawToast (used in DrawCS2Menu)
static void DrawToast(DrawList& dl, f32 W, f32 H);

// ============================================================================
// SIDEBAR ICONS — clean minimal line icons for LunaR-style sidebar
// ============================================================================

// Pencil/edit icon for Inventory Changer
static void DrawEditIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.42f;
    dl.AddLine(Vec2{cx - h*0.6f, cy + h*0.6f}, Vec2{cx + h*0.4f, cy - h*0.4f}, c, 2.0f);
    dl.AddLine(Vec2{cx + h*0.4f, cy - h*0.4f}, Vec2{cx + h*0.65f, cy - h*0.65f}, c, 2.0f);
    dl.AddLine(Vec2{cx - h*0.6f, cy + h*0.6f}, Vec2{cx - h*0.8f, cy + h*0.8f}, c, 1.5f);
    dl.AddLine(Vec2{cx - h*0.8f, cy + h*0.85f}, Vec2{cx + h*0.2f, cy + h*0.85f}, c, 1.5f);
}

// Globe/world icon
static void DrawWorldIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 r = sz * 0.38f;
    dl.AddCircle(Vec2{cx, cy}, r, c, 24);
    dl.AddLine(Vec2{cx - r, cy}, Vec2{cx + r, cy}, c, 1.5f); // equator
    dl.AddLine(Vec2{cx, cy - r}, Vec2{cx, cy + r}, c, 1.5f); // meridian
    // Curved meridian
    dl.AddLine(Vec2{cx - r*0.5f, cy - r*0.85f}, Vec2{cx - r*0.6f, cy}, c, 1.2f);
    dl.AddLine(Vec2{cx - r*0.6f, cy}, Vec2{cx - r*0.5f, cy + r*0.85f}, c, 1.2f);
}

// Eye/view icon
static void DrawViewIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.35f;
    // Eye shape
    dl.AddLine(Vec2{cx - h, cy}, Vec2{cx - h*0.4f, cy - h*0.5f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.4f, cy - h*0.5f}, Vec2{cx + h*0.4f, cy - h*0.5f}, c, 1.8f);
    dl.AddLine(Vec2{cx + h*0.4f, cy - h*0.5f}, Vec2{cx + h, cy}, c, 1.8f);
    dl.AddLine(Vec2{cx + h, cy}, Vec2{cx + h*0.4f, cy + h*0.5f}, c, 1.8f);
    dl.AddLine(Vec2{cx + h*0.4f, cy + h*0.5f}, Vec2{cx - h*0.4f, cy + h*0.5f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.4f, cy + h*0.5f}, Vec2{cx - h, cy}, c, 1.8f);
    dl.AddCircle(Vec2{cx, cy}, h*0.25f, c, 12);
}

// Profile/User icon
static void DrawProfileIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.5f;
    dl.AddCircle(Vec2{cx, cy - h * 0.25f}, h * 0.32f, c, 16);
    dl.AddLine(Vec2{cx - h*0.55f, cy + h*0.55f}, Vec2{cx - h*0.3f, cy + h*0.2f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.3f, cy + h*0.2f}, Vec2{cx, cy + h*0.12f}, c, 1.8f);
    dl.AddLine(Vec2{cx, cy + h*0.12f}, Vec2{cx + h*0.3f, cy + h*0.2f}, c, 1.8f);
    dl.AddLine(Vec2{cx + h*0.3f, cy + h*0.2f}, Vec2{cx + h*0.55f, cy + h*0.55f}, c, 1.8f);
}

// Chat/misc icon
static void DrawMiscIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.38f;
    // Chat bubble
    dl.AddLine(Vec2{cx - h, cy - h*0.5f}, Vec2{cx + h, cy - h*0.5f}, c, 1.8f);
    dl.AddLine(Vec2{cx + h, cy - h*0.5f}, Vec2{cx + h, cy + h*0.3f}, c, 1.8f);
    dl.AddLine(Vec2{cx + h, cy + h*0.3f}, Vec2{cx - h*0.2f, cy + h*0.3f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.2f, cy + h*0.3f}, Vec2{cx - h*0.5f, cy + h*0.7f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.5f, cy + h*0.7f}, Vec2{cx - h*0.5f, cy + h*0.3f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.5f, cy + h*0.3f}, Vec2{cx - h, cy + h*0.3f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h, cy + h*0.3f}, Vec2{cx - h, cy - h*0.5f}, c, 1.8f);
}

// Config/gear icon
static void DrawConfigIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 r = sz * 0.35f;
    dl.AddCircle(Vec2{cx, cy}, r * 0.45f, c, 12);
    for (int i = 0; i < 6; i++) {
        f32 angle = (f32)i * 3.14159f / 3.0f;
        f32 x1 = cx + cosf(angle) * r * 0.7f;
        f32 y1 = cy + sinf(angle) * r * 0.7f;
        f32 x2 = cx + cosf(angle) * r;
        f32 y2 = cy + sinf(angle) * r;
        dl.AddLine(Vec2{x1, y1}, Vec2{x2, y2}, c, 2.5f);
    }
}

// Marketplace/cart icon
static void DrawMarketIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.38f;
    dl.AddLine(Vec2{cx - h, cy - h*0.5f}, Vec2{cx - h*0.6f, cy - h*0.5f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.6f, cy - h*0.5f}, Vec2{cx - h*0.3f, cy + h*0.3f}, c, 1.8f);
    dl.AddLine(Vec2{cx - h*0.3f, cy + h*0.3f}, Vec2{cx + h*0.7f, cy + h*0.3f}, c, 1.8f);
    dl.AddLine(Vec2{cx + h*0.7f, cy + h*0.3f}, Vec2{cx + h*0.85f, cy - h*0.3f}, c, 1.8f);
    dl.AddLine(Vec2{cx + h*0.85f, cy - h*0.3f}, Vec2{cx - h*0.45f, cy - h*0.3f}, c, 1.8f);
    dl.AddCircle(Vec2{cx - h*0.15f, cy + h*0.6f}, h*0.13f, c, 10);
    dl.AddCircle(Vec2{cx + h*0.5f, cy + h*0.6f}, h*0.13f, c, 10);
}

// Plus (+) icon for empty grid slots
static void DrawPlusIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.35f;
    dl.AddLine(Vec2{cx - h, cy}, Vec2{cx + h, cy}, c, 2.5f);
    dl.AddLine(Vec2{cx, cy - h}, Vec2{cx, cy + h}, c, 2.5f);
}

// Back arrow (< BACK)
static void DrawBackArrow(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.35f;
    dl.AddLine(Vec2{cx + h*0.3f, cy - h*0.6f}, Vec2{cx - h*0.5f, cy}, c, 2.5f);
    dl.AddLine(Vec2{cx - h*0.5f, cy}, Vec2{cx + h*0.3f, cy + h*0.6f}, c, 2.5f);
}

// Helper: compare category strings
static bool CatMatch(const char* a, const char* b) {
    for (int k = 0; ; k++) {
        if (a[k] != b[k]) return false;
        if (a[k] == '\0') return true;
    }
}

// ============================================================================
// REUSABLE UI WIDGETS — Toggle switch, slider, dropdown, section header
// ============================================================================

// Toggle switch: returns new state. Renders at (x, y) with width tW, height tH
static bool UIToggle(DrawList& dl, u32 uid, f32 x, f32 y, f32 sc, u8 ga, bool value) {
    f32 tW = 30*sc, tH = 16*sc;
    bool hov = Hit(x, y - 2*sc, tW, tH + 4*sc);
    f32 animVal = Anim(uid, value ? 1.0f : 0.0f, 10.0f);

    Color bg = Mix(Color{45, 35, 65, ga}, Color{90, 50, 140, ga}, animVal);
    dl.AddFilledRoundRect(Rect{x, y, tW, tH}, bg, tH*0.5f, 10);
    f32 knobX = x + 2*sc + animVal * (tW - tH);
    dl.AddFilledRoundRect(Rect{knobX, y + 2*sc, tH - 4*sc, tH - 4*sc},
        Color{230, 220, 250, ga}, (tH-4*sc)*0.5f, 10);

    if (hov && g_input.IsMousePressed(MouseButton::Left))
        return !value;
    return value;
}

// Setting row: label on left, toggle on right. Returns new toggle state.
static bool UISettingToggle(DrawList& dl, const char* label, bool value,
                            f32 x, f32 y, f32 w, f32 sc, u8 ga, u32 uid) {
    Text(dl, x, y + 2*sc, Color{200, 185, 230, ga}, label, g_fontSm);
    bool newVal = UIToggle(dl, uid, x + w - 36*sc, y, sc, ga, value);
    return newVal;
}

// Section header with divider line
static f32 UISectionHeader(DrawList& dl, const char* title, f32 x, f32 y, f32 w, f32 sc, u8 ga) {
    Text(dl, x, y, Color{160, 130, 220, ga}, title, g_font);
    dl.AddFilledRect(Rect{x, y + 18*sc, w, 1}, Color{50, 35, 75, (u8)(ga*0.4f)});
    return y + 26*sc;
}

// Int slider: renders slider, returns new value
static int UISlider(DrawList& dl, u32 uid, f32 x, f32 y, f32 w, f32 sc, u8 ga,
                    int value, int minV, int maxV) {
    f32 slH = 6*sc;
    f32 norm = (f32)(value - minV) / (f32)(maxV - minV);
    f32 fillW = w * norm;

    dl.AddFilledRoundRect(Rect{x, y, w, slH}, Color{38, 28, 58, ga}, 3*sc, 6);
    if (fillW > 1)
        dl.AddFilledRoundRect(Rect{x, y, fillW, slH}, Color{160, 110, 220, ga}, 3*sc, 6);
    dl.AddCircle(Vec2{x + fillW, y + slH*0.5f}, 5*sc, Color{180, 140, 240, ga}, 10);

    if (Hit(x, y - 4*sc, w, slH + 8*sc) && g_input.IsMousePressed(MouseButton::Left)) {
        f32 mx = g_input.MousePos().x;
        f32 t = (mx - x) / w;
        if (t < 0) t = 0; if (t > 1) t = 1;
        return minV + (int)(t * (maxV - minV));
    }
    return value;
}

// Dropdown / cycle selector: click to cycle through options
static int UIDropdown(DrawList& dl, u32 uid, f32 x, f32 y, f32 w, f32 h, f32 sc, u8 ga,
                      const char** options, int count, int selected) {
    bool hov = Hit(x, y, w, h);
    f32 hovA = Anim(uid, hov ? 1.0f : 0.0f);

    dl.AddFilledRoundRect(Rect{x, y, w, h},
        Color{26, 18, 44, ga}, 4*sc, 6);
    dl.AddFilledRoundRect(Rect{x, y, w, h},
        Color{50, 35, 75, (u8)(ga * (0.3f + 0.3f*hovA))}, 4*sc, 6);

    const char* label = (selected >= 0 && selected < count) ? options[selected] : "???";
    Vec2 ls = Measure(label, g_fontSm);
    Text(dl, x + 8*sc, y + (h-ls.y)*0.5f, Color{200, 185, 230, ga}, label, g_fontSm);

    // Small arrow indicator
    f32 ax = x + w - 14*sc, ay = y + h*0.5f;
    dl.AddLine(Vec2{ax - 3*sc, ay - 2*sc}, Vec2{ax, ay + 2*sc}, Color{140, 120, 180, ga}, 1.5f);
    dl.AddLine(Vec2{ax, ay + 2*sc}, Vec2{ax + 3*sc, ay - 2*sc}, Color{140, 120, 180, ga}, 1.5f);

    if (hov && g_input.IsMousePressed(MouseButton::Left))
        return (selected + 1) % count;
    return selected;
}

// ============================================================================
// DRAW CS2 MENU — Matches reference UI: sidebar + grid skin cards
// ============================================================================

// Sidebar icons (small, icon-only style)
static void DrawSidebarProfileIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 r = sz * 0.28f;
    dl.AddCircle(Vec2{cx, cy - r*0.6f}, r*0.55f, c, 12);
    dl.AddLine(Vec2{cx - r, cy + r*0.5f}, Vec2{cx - r*0.6f, cy + r*0.1f}, c, 1.6f);
    dl.AddLine(Vec2{cx - r*0.6f, cy + r*0.1f}, Vec2{cx, cy}, c, 1.6f);
    dl.AddLine(Vec2{cx, cy}, Vec2{cx + r*0.6f, cy + r*0.1f}, c, 1.6f);
    dl.AddLine(Vec2{cx + r*0.6f, cy + r*0.1f}, Vec2{cx + r, cy + r*0.5f}, c, 1.6f);
}

static void DrawSidebarItemsIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.32f;
    // Calendar/items grid icon
    dl.AddLine(Vec2{cx-h, cy-h}, Vec2{cx+h, cy-h}, c, 1.6f);
    dl.AddLine(Vec2{cx+h, cy-h}, Vec2{cx+h, cy+h}, c, 1.6f);
    dl.AddLine(Vec2{cx+h, cy+h}, Vec2{cx-h, cy+h}, c, 1.6f);
    dl.AddLine(Vec2{cx-h, cy+h}, Vec2{cx-h, cy-h}, c, 1.6f);
    dl.AddLine(Vec2{cx-h, cy-h*0.3f}, Vec2{cx+h, cy-h*0.3f}, c, 1.2f);
    dl.AddLine(Vec2{cx, cy-h}, Vec2{cx, cy+h}, c, 1.2f);
}

static void DrawSidebarConfigIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 r = sz * 0.30f;
    dl.AddCircle(Vec2{cx, cy}, r*0.45f, c, 12);
    for (int i = 0; i < 6; i++) {
        f32 ang = (f32)i * 3.14159f / 3.0f;
        dl.AddLine(Vec2{cx+cosf(ang)*r*0.65f, cy+sinf(ang)*r*0.65f},
                   Vec2{cx+cosf(ang)*r, cy+sinf(ang)*r}, c, 2.0f);
    }
}

static void DrawSearchIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 r = sz * 0.28f;
    dl.AddCircle(Vec2{cx - r*0.15f, cy - r*0.15f}, r*0.6f, c, 12);
    dl.AddLine(Vec2{cx + r*0.25f, cy + r*0.25f}, Vec2{cx + r*0.7f, cy + r*0.7f}, c, 2.0f);
}

static void DrawCS2Menu(DrawList& dl, f32 W, f32 H) {
    f32 a = g_menuOpenAnim;
    if (a < 0.001f) return;
    u8 ga = (u8)(a * 255.0f);

    f32 ease = 1.0f - (1.0f - a) * (1.0f - a);
    f32 sc = 0.8f + 0.2f * ease;
    f32 sW = W * sc, sH = H * sc;
    f32 ox = (W - sW) * 0.5f, oy = (H - sH) * 0.5f;

    // ===== MAIN BACKGROUND — very dark =====
    dl.AddFilledRoundRect(Rect{ox, oy, sW, sH}, Color{18, 18, 26, ga}, 8*sc, 12);

    // ===== LEFT SIDEBAR — narrow 62px =====
    f32 sbW = 62.0f * sc;
    dl.AddFilledRoundRect(Rect{ox, oy, sbW, sH}, Color{22, 22, 32, ga}, 8*sc, 12);
    dl.AddFilledRect(Rect{ox + sbW - 1, oy + 8*sc, 1, sH - 16*sc}, Color{40, 40, 55, (u8)(ga*0.5f)});
    // fill right side of sidebar to sharp edge
    dl.AddFilledRect(Rect{ox + sbW*0.5f, oy, sbW*0.5f, sH}, Color{22, 22, 32, ga});

    // Logo — double chevron ≪
    {
        f32 lx = ox + sbW*0.5f, ly = oy + 22*sc;
        Color lc = Color{100, 120, 220, ga};
        // first chevron
        dl.AddLine(Vec2{lx - 6*sc, ly - 6*sc}, Vec2{lx - 1*sc, ly}, lc, 2.2f);
        dl.AddLine(Vec2{lx - 1*sc, ly}, Vec2{lx - 6*sc, ly + 6*sc}, lc, 2.2f);
        // second chevron
        dl.AddLine(Vec2{lx + 1*sc, ly - 6*sc}, Vec2{lx + 6*sc, ly}, lc, 2.2f);
        dl.AddLine(Vec2{lx + 6*sc, ly}, Vec2{lx + 1*sc, ly + 6*sc}, lc, 2.2f);
    }

    // Sidebar tabs: Profile (0), Items (1), Configs (2)
    struct SideTabDef { const char* label; int id; void(*icon)(DrawList&,f32,f32,f32,Color); };
    SideTabDef sideTabs[] = {
        {"Profile", 0, DrawSidebarProfileIcon},
        {"Items",   1, DrawSidebarItemsIcon},
        {"Configs", 2, DrawSidebarConfigIcon},
    };

    f32 tabY = oy + 58*sc;
    for (int ti = 0; ti < 3; ti++) {
        f32 tabH = 44*sc;
        bool active = (g_sideTab == sideTabs[ti].id);
        u32 tid = Hash(sideTabs[ti].label) + 9900;
        bool tHov = Hit(ox, tabY, sbW, tabH);
        f32 tAn = Anim(tid, active ? 1.0f : (tHov ? 0.5f : 0.0f));

        // Active left accent bar (pink/purple)
        if (active) {
            dl.AddFilledRect(Rect{ox, tabY + 6*sc, 3*sc, tabH - 12*sc},
                Color{200, 80, 200, ga});
        }

        // Icon
        Color ic = active ? Color{200, 100, 220, ga} : Color{100, 100, 130, (u8)(ga*(0.5f+0.5f*tAn))};
        if (sideTabs[ti].icon)
            sideTabs[ti].icon(dl, ox + sbW*0.5f, tabY + 16*sc, 22*sc, ic);

        // Label below icon
        Color lc = active ? Color{200, 100, 220, ga} : Color{90, 90, 110, (u8)(ga*(0.5f+0.5f*tAn))};
        Vec2 ls = Measure(sideTabs[ti].label, g_fontSm);
        Text(dl, ox + (sbW - ls.x)*0.5f, tabY + 30*sc, lc, sideTabs[ti].label, g_fontSm);

        if (tHov && g_input.IsMousePressed(MouseButton::Left)) {
            g_sideTab = sideTabs[ti].id;
            if (sideTabs[ti].id == 1) g_invView = INV_VIEW_MAIN;
        }
        tabY += tabH + 2*sc;
    }

    // User avatar at bottom
    {
        f32 avY = oy + sH - 42*sc;
        f32 avR = 14*sc;
        dl.AddFilledRoundRect(Rect{ox + (sbW-avR*2)*0.5f, avY, avR*2, avR*2},
            Color{55, 55, 80, ga}, avR, 12);
        const char* uInit = "U";
        if (strlen(g_loginUser) > 0) {
            static char initBuf[2] = {0};
            initBuf[0] = g_loginUser[0];
            if (initBuf[0] >= 'a' && initBuf[0] <= 'z') initBuf[0] -= 32;
            uInit = initBuf;
        }
        Vec2 us = Measure(uInit, g_fontSm);
        Text(dl, ox + (sbW - us.x)*0.5f, avY + (avR*2 - us.y)*0.5f,
             Color{180, 180, 200, ga}, uInit, g_fontSm);
    }

    // ===== CONTENT AREA =====
    f32 cX = ox + sbW + 10*sc;
    f32 cY = oy + 10*sc;
    f32 cW = sW - sbW - 20*sc;
    f32 cH = sH - 20*sc;

    // ===================================================================
    // TAB: ITEMS (skin changer)
    // ===================================================================
    if (g_sideTab == 1) {

        // ================= VIEW: MAIN INVENTORY =================
        if (g_invView == INV_VIEW_MAIN) {
            // Top bar: back arrow + search icon
            {
                // Back arrow (just a "←" that does nothing on main view, visual only)
                f32 bSz = 24*sc;
                dl.AddLine(Vec2{cX + 8*sc, cY + bSz*0.5f}, Vec2{cX + 2*sc, cY + bSz*0.5f},
                    Color{140, 140, 170, ga}, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f - 4*sc},
                    Color{140, 140, 170, ga}, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f + 4*sc},
                    Color{140, 140, 170, ga}, 2.0f);

                // Horizontal line under back arrow area
                dl.AddFilledRect(Rect{cX, cY + bSz + 2*sc, cW, 1},
                    Color{50, 50, 70, (u8)(ga*0.5f)});

                // Search icon (top right)
                f32 srX = cX + cW - 26*sc, srY = cY + 2*sc;
                f32 srW = 22*sc, srH = 20*sc;
                dl.AddFilledRoundRect(Rect{srX, srY, srW, srH},
                    Color{35, 35, 50, ga}, 4*sc, 6);
                DrawSearchIcon(dl, srX + srW*0.5f, srY + srH*0.5f, 18*sc, Color{120, 120, 150, ga});
            }

            // Content border box
            f32 gridY = cY + 32*sc;
            f32 gridH = cH - 38*sc;

            // Inner content border (subtle purple outline like screenshot)
            dl.AddFilledRoundRect(Rect{cX, gridY, cW, gridH},
                Color{28, 28, 42, ga}, 6*sc, 8);
            // top edge accent
            dl.AddFilledRect(Rect{cX + 6*sc, gridY, cW - 12*sc, 1},
                Color{70, 50, 120, (u8)(ga*0.5f)});

            f32 innerPad = 10*sc;
            f32 innerX = cX + innerPad;
            f32 innerY = gridY + innerPad;
            f32 innerW = cW - innerPad*2;
            f32 innerH = gridH - innerPad*2;

            // Grid: 5 columns of skin cards
            int cols = 5;
            f32 gap = 8*sc;
            f32 cardW = (innerW - (cols-1)*gap) / cols;
            f32 cardH = cardW * 0.95f; // slightly taller than wide

            // "+" add skin card at first position
            {
                u32 plusId = Hash("_inv_plus2");
                bool plusHov = Hit(innerX, innerY, cardW, cardH);
                f32 plusAn = Anim(plusId, plusHov ? 1.0f : 0.0f);

                dl.AddFilledRoundRect(Rect{innerX, innerY, cardW, cardH},
                    Color{32, 32, 48, ga}, 6*sc, 8);
                if (plusAn > 0.01f)
                    dl.AddFilledRoundRect(Rect{innerX, innerY, cardW, cardH},
                        Color{80, 50, 140, (u8)(12*plusAn*a)}, 6*sc, 8);

                // Dashed border
                Color bc = Color{70, 50, 110, (u8)(ga*(0.4f+0.4f*plusAn))};
                dl.AddFilledRoundRect(Rect{innerX+1, innerY+1, cardW-2, cardH-2}, bc, 6*sc, 8);
                dl.AddFilledRoundRect(Rect{innerX+2, innerY+2, cardW-4, cardH-4},
                    Color{32, 32, 48, ga}, 5*sc, 8);

                DrawPlusIcon(dl, innerX + cardW*0.5f, innerY + cardH*0.4f, 28*sc,
                    Color{110, 80, 180, (u8)(ga*(0.5f+0.5f*plusAn))});
                Vec2 addTs = Measure("Add Skin", g_fontSm);
                Text(dl, innerX + (cardW-addTs.x)*0.5f, innerY + cardH*0.65f,
                     Color{100, 80, 160, (u8)(ga*(0.5f+0.5f*plusAn))}, "Add Skin", g_fontSm);

                if (plusHov && g_input.IsMousePressed(MouseButton::Left)) {
                    g_invView = INV_VIEW_CATEGORIES;
                    g_selectedCategory = -1;
                }
            }

            // Equipped skin cards
            int removeIdx = -1;
            for (int ii = 0; ii < (int)g_invItems.size(); ii++) {
                int slot = ii + 1;
                int col = slot % cols, row = slot / cols;
                f32 ix = innerX + col*(cardW+gap);
                f32 iy = innerY + row*(cardH+gap);
                if (iy + cardH > innerY + innerH) break;

                const auto& item = g_invItems[ii];
                if (item.weaponIdx < 0 || item.weaponIdx >= g_weaponDBCount) continue;
                const auto& wpn = g_weaponDB[item.weaponIdx];
                if (item.skinIdx < 0 || item.skinIdx >= wpn.skinCount) continue;
                const auto& sk = wpn.skins[item.skinIdx];
                Color rc = RarityColor(sk.rarity);

                u32 iid = Hash("inv_item2") + ii*37;
                bool iHov = Hit(ix, iy, cardW, cardH);
                f32 iAn = Anim(iid, iHov ? 1.0f : 0.0f);

                // Card background — dark with slight rarity tint
                dl.AddFilledRoundRect(Rect{ix, iy, cardW, cardH},
                    Color{(u8)(28 + rc.r/20), (u8)(28 + rc.g/20), (u8)(36 + rc.b/20), ga}, 6*sc, 8);

                // Hover glow
                if (iAn > 0.01f)
                    dl.AddFilledRoundRect(Rect{ix, iy, cardW, cardH},
                        Color{rc.r, rc.g, rc.b, (u8)(12*iAn*a)}, 6*sc, 8);

                // Rarity bar at bottom
                dl.AddFilledRect(Rect{ix + 2*sc, iy + cardH - 3*sc, cardW - 4*sc, 3*sc},
                    Color{rc.r, rc.g, rc.b, (u8)(ga*0.8f)});

                // Skin image (large, centered in upper 65% of card)
                TextureHandle skinTex = LoadSkinImage(wpn.name, sk.name);
                f32 imgAreaH = cardH * 0.60f;
                if (skinTex != INVALID_TEXTURE) {
                    f32 imgH = imgAreaH - 4*sc;
                    f32 imgW = imgH * 1.33f;
                    if (imgW > cardW - 8*sc) { imgW = cardW - 8*sc; imgH = imgW / 1.33f; }
                    dl.AddTexturedRect(Rect{ix + (cardW-imgW)*0.5f, iy + 6*sc, imgW, imgH},
                        skinTex, Color{255,255,255,ga});
                } else {
                    // Placeholder text
                    Vec2 phs = Measure(wpn.name, g_fontSm);
                    Text(dl, ix + (cardW-phs.x)*0.5f, iy + imgAreaH*0.4f,
                         Color{70, 65, 95, ga}, wpn.name, g_fontSm);
                }

                // Weapon name (white/light)
                f32 textY = iy + imgAreaH + 4*sc;
                Vec2 wns = Measure(wpn.name, g_fontSm);
                f32 labelX = ix + 6*sc;
                Text(dl, labelX, textY, Color{210, 210, 230, ga}, wpn.name, g_fontSm);

                // Skin name in rarity color (below weapon name, smaller/italic feel)
                char dn[22]; strncpy(dn, sk.name, 19); dn[19] = '\0';
                if (strlen(sk.name) > 19) { dn[17] = '.'; dn[18] = '.'; dn[19] = '\0'; }
                Text(dl, labelX, textY + 13*sc, Color{rc.r, rc.g, rc.b, (u8)(ga*0.85f)}, dn, g_fontSm);

                // Hover X to remove
                if (iAn > 0.3f) {
                    f32 xSz = 7*sc, xCx = ix + cardW - 12*sc, xCy = iy + 10*sc;
                    dl.AddLine(Vec2{xCx-xSz*0.5f,xCy-xSz*0.5f}, Vec2{xCx+xSz*0.5f,xCy+xSz*0.5f},
                        Color{255,100,100,(u8)(ga*iAn)}, 2.0f);
                    dl.AddLine(Vec2{xCx+xSz*0.5f,xCy-xSz*0.5f}, Vec2{xCx-xSz*0.5f,xCy+xSz*0.5f},
                        Color{255,100,100,(u8)(ga*iAn)}, 2.0f);
                }

                if (iHov && g_input.IsMousePressed(MouseButton::Left)) removeIdx = ii;
            }

            if (removeIdx >= 0 && removeIdx < (int)g_invItems.size()) {
                g_invItems.erase(g_invItems.begin() + removeIdx);
                WriteEquippedConfig();
            }

        // ================= VIEW: CATEGORIES =================
        } else if (g_invView == INV_VIEW_CATEGORIES) {
            // Back button
            {
                f32 bSz = 24*sc;
                u32 bid = Hash("_cat_back2");
                bool bH2 = Hit(cX, cY, 40*sc, bSz);
                f32 ba = Anim(bid, bH2 ? 1.0f : 0.0f);
                Color ac = Color{140, 140, 170, (u8)(ga*(0.6f+0.4f*ba))};
                dl.AddLine(Vec2{cX + 12*sc, cY + bSz*0.5f}, Vec2{cX + 2*sc, cY + bSz*0.5f}, ac, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f - 4*sc}, ac, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f + 4*sc}, ac, 2.0f);
                if (bH2 && g_input.IsMousePressed(MouseButton::Left)) g_invView = INV_VIEW_MAIN;
            }

            dl.AddFilledRect(Rect{cX, cY + 26*sc, cW, 1}, Color{50, 50, 70, (u8)(ga*0.5f)});

            f32 gridY = cY + 32*sc;
            f32 innerPad = 10*sc;
            dl.AddFilledRoundRect(Rect{cX, gridY, cW, cH - 38*sc}, Color{28, 28, 42, ga}, 6*sc, 8);
            dl.AddFilledRect(Rect{cX + 6*sc, gridY, cW - 12*sc, 1}, Color{70, 50, 120, (u8)(ga*0.5f)});

            int cols = 3;
            f32 gap = 10*sc;
            f32 cellW = (cW - innerPad*2 - (cols-1)*gap) / cols;
            f32 cellH = cellW * 0.55f;
            f32 catX = cX + innerPad, catY = gridY + innerPad;

            for (int ci = 0; ci < g_categoryCount; ci++) {
                int col = ci % cols, row = ci / cols;
                f32 cx2 = catX + col*(cellW+gap), cy2 = catY + row*(cellH+gap);

                u32 cid = Hash(g_categories[ci].name) + 5500;
                bool cHov = Hit(cx2, cy2, cellW, cellH);
                f32 cAn = Anim(cid, cHov ? 1.0f : 0.0f);

                dl.AddFilledRoundRect(Rect{cx2, cy2, cellW, cellH}, Color{32, 32, 48, ga}, 6*sc, 8);
                if (cAn > 0.01f)
                    dl.AddFilledRoundRect(Rect{cx2, cy2, cellW, cellH},
                        Color{100, 60, 180, (u8)(15*cAn*a)}, 6*sc, 8);

                // Preview image from first weapon's first skin
                for (int wi = 0; wi < g_weaponDBCount; wi++) {
                    if (CatMatch(g_weaponDB[wi].category, g_categories[ci].filterKey) && g_weaponDB[wi].skinCount > 0) {
                        TextureHandle tex = LoadSkinImage(g_weaponDB[wi].name, g_weaponDB[wi].skins[0].name);
                        if (tex != INVALID_TEXTURE) {
                            f32 iH = cellH - 26*sc, iW = iH * 1.5f;
                            if (iW > cellW - 12*sc) { iW = cellW - 12*sc; iH = iW / 1.5f; }
                            dl.AddTexturedRect(Rect{cx2+(cellW-iW)*0.5f, cy2+4*sc, iW, iH},
                                tex, Color{255,255,255,ga});
                        }
                        break;
                    }
                }

                Vec2 ls = Measure(g_categories[ci].name, g_font);
                Text(dl, cx2 + (cellW-ls.x)*0.5f, cy2 + cellH - ls.y - 6*sc,
                     Color{(u8)(160+60*cAn),(u8)(160+50*cAn),220,ga}, g_categories[ci].name, g_font);

                int wCount = 0;
                for (int wi = 0; wi < g_weaponDBCount; wi++)
                    if (CatMatch(g_weaponDB[wi].category, g_categories[ci].filterKey)) wCount++;
                char wcBuf[16]; snprintf(wcBuf, 16, "%d weapons", wCount);
                Vec2 wcs = Measure(wcBuf, g_fontSm);
                Text(dl, cx2 + (cellW-wcs.x)*0.5f, cy2 + cellH - 6*sc - ls.y - wcs.y,
                     Color{90, 85, 130, ga}, wcBuf, g_fontSm);

                if (cHov && g_input.IsMousePressed(MouseButton::Left)) {
                    g_selectedCategory = ci;
                    g_invView = INV_VIEW_WEAPONS;
                    g_weaponScrollY = 0;
                }
            }

        // ================= VIEW: WEAPONS IN CATEGORY =================
        } else if (g_invView == INV_VIEW_WEAPONS && g_selectedCategory >= 0) {
            const char* catKey = g_categories[g_selectedCategory].filterKey;

            // Back
            {
                f32 bSz = 24*sc;
                u32 bid = Hash("_wpn_back2");
                bool bH2 = Hit(cX, cY, 40*sc, bSz);
                f32 ba = Anim(bid, bH2 ? 1.0f : 0.0f);
                Color ac = Color{140, 140, 170, (u8)(ga*(0.6f+0.4f*ba))};
                dl.AddLine(Vec2{cX + 12*sc, cY + bSz*0.5f}, Vec2{cX + 2*sc, cY + bSz*0.5f}, ac, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f - 4*sc}, ac, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f + 4*sc}, ac, 2.0f);
                if (bH2 && g_input.IsMousePressed(MouseButton::Left)) g_invView = INV_VIEW_CATEGORIES;
            }

            Text(dl, cX + 50*sc, cY + 4*sc, Color{200, 200, 220, ga},
                 g_categories[g_selectedCategory].name, g_font);
            dl.AddFilledRect(Rect{cX, cY + 26*sc, cW, 1}, Color{50, 50, 70, (u8)(ga*0.5f)});

            f32 gridY = cY + 32*sc;
            f32 innerPad = 10*sc;
            dl.AddFilledRoundRect(Rect{cX, gridY, cW, cH - 38*sc}, Color{28, 28, 42, ga}, 6*sc, 8);
            dl.AddFilledRect(Rect{cX + 6*sc, gridY, cW - 12*sc, 1}, Color{70, 50, 120, (u8)(ga*0.5f)});

            int cols = 5;
            f32 gap = 8*sc;
            f32 cellW = (cW - innerPad*2 - (cols-1)*gap) / cols;
            f32 cellH = cellW * 0.85f;

            int widx = 0;
            for (int wi = 0; wi < g_weaponDBCount; wi++) {
                if (!CatMatch(g_weaponDB[wi].category, catKey)) continue;
                int col = widx % cols, row = widx / cols;
                f32 wx = cX + innerPad + col*(cellW+gap);
                f32 wy = gridY + innerPad + row*(cellH+gap);
                if (wy + cellH > gridY + cH - 38*sc - innerPad) { widx++; continue; }

                u32 wid = Hash(g_weaponDB[wi].name) + 6600;
                bool wHov = Hit(wx, wy, cellW, cellH);
                f32 wAn = Anim(wid, wHov ? 1.0f : 0.0f);

                dl.AddFilledRoundRect(Rect{wx, wy, cellW, cellH}, Color{32, 32, 48, ga}, 6*sc, 8);
                if (wAn > 0.01f)
                    dl.AddFilledRoundRect(Rect{wx, wy, cellW, cellH},
                        Color{80, 50, 140, (u8)(12*wAn*a)}, 6*sc, 8);

                // First skin preview
                if (g_weaponDB[wi].skinCount > 0) {
                    TextureHandle tex = LoadSkinImage(g_weaponDB[wi].name, g_weaponDB[wi].skins[0].name);
                    if (tex != INVALID_TEXTURE) {
                        f32 iH = cellH - 26*sc, iW = iH * 1.33f;
                        if (iW > cellW - 6*sc) { iW = cellW - 6*sc; iH = iW / 1.33f; }
                        dl.AddTexturedRect(Rect{wx+(cellW-iW)*0.5f, wy+3*sc, iW, iH},
                            tex, Color{255,255,255,ga});
                    }
                }

                Vec2 ns = Measure(g_weaponDB[wi].name, g_fontSm);
                Text(dl, wx + (cellW-ns.x)*0.5f, wy + cellH - ns.y - 4*sc,
                     Color{200, 200, 220, ga}, g_weaponDB[wi].name, g_fontSm);

                // Count badge
                char scBuf[8]; snprintf(scBuf, 8, "%d", g_weaponDB[wi].skinCount);
                Vec2 scs = Measure(scBuf, g_fontSm);
                dl.AddFilledRoundRect(Rect{wx + cellW - scs.x - 10*sc, wy + 3*sc, scs.x + 6*sc, scs.y + 2*sc},
                    Color{80, 50, 150, (u8)(ga*0.5f)}, 4*sc, 6);
                Text(dl, wx + cellW - scs.x - 7*sc, wy + 4*sc, Color{200, 200, 220, ga}, scBuf, g_fontSm);

                if (wHov && g_input.IsMousePressed(MouseButton::Left)) {
                    g_selectedWeapon = wi;
                    g_invView = INV_VIEW_SKINS;
                    g_selectedSkin = -1;
                    g_skinScrollY = 0;
                }
                widx++;
            }

        // ================= VIEW: SKINS (grid + detail) =================
        } else if (g_invView == INV_VIEW_SKINS && g_selectedWeapon >= 0 && g_selectedWeapon < g_weaponDBCount) {
            const auto& wpn = g_weaponDB[g_selectedWeapon];

            // Back
            {
                f32 bSz = 24*sc;
                u32 bid = Hash("_sk_back2");
                bool bH2 = Hit(cX, cY, 40*sc, bSz);
                f32 ba = Anim(bid, bH2 ? 1.0f : 0.0f);
                Color ac = Color{140, 140, 170, (u8)(ga*(0.6f+0.4f*ba))};
                dl.AddLine(Vec2{cX + 12*sc, cY + bSz*0.5f}, Vec2{cX + 2*sc, cY + bSz*0.5f}, ac, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f - 4*sc}, ac, 2.0f);
                dl.AddLine(Vec2{cX + 2*sc, cY + bSz*0.5f}, Vec2{cX + 7*sc, cY + bSz*0.5f + 4*sc}, ac, 2.0f);
                if (bH2 && g_input.IsMousePressed(MouseButton::Left)) g_invView = INV_VIEW_WEAPONS;
            }

            Text(dl, cX + 50*sc, cY + 4*sc, Color{220, 220, 240, ga}, wpn.name, g_font);
            char skCountBuf[24]; snprintf(skCountBuf, 24, "%d skins", wpn.skinCount);
            Vec2 scs2 = Measure(skCountBuf, g_fontSm);
            Text(dl, cX + cW - scs2.x, cY + 7*sc, Color{100, 100, 130, ga}, skCountBuf, g_fontSm);
            dl.AddFilledRect(Rect{cX, cY + 26*sc, cW, 1}, Color{50, 50, 70, (u8)(ga*0.5f)});

            // Grid left (62%) + detail panel right (38%)
            f32 gridY = cY + 32*sc;
            f32 detailW = cW * 0.36f;
            f32 gridW = cW - detailW - 10*sc;
            f32 gridH = cH - 38*sc;
            f32 innerPad = 6*sc;

            dl.AddFilledRoundRect(Rect{cX, gridY, gridW, gridH}, Color{28, 28, 42, ga}, 6*sc, 8);

            int cols = 3;
            f32 gap = 6*sc;
            f32 cellW = (gridW - innerPad*2 - (cols-1)*gap) / cols;
            f32 cellH = cellW * 0.72f;

            for (int si = 0; si < wpn.skinCount; si++) {
                const auto& sk = wpn.skins[si];
                int col = si % cols, row = si / cols;
                f32 sx = cX + innerPad + col*(cellW+gap);
                f32 sy2 = gridY + innerPad + row*(cellH+gap);
                if (sy2 + cellH > gridY + gridH - innerPad) break;

                bool selected = (g_selectedSkin == si);
                Color rc = RarityColor(sk.rarity);

                u32 sid = Hash(sk.name) + g_selectedWeapon * 100 + si;
                bool sHov = Hit(sx, sy2, cellW, cellH);
                f32 sAn = Anim(sid, selected ? 0.8f : (sHov ? 0.5f : 0.0f));

                // Card
                dl.AddFilledRoundRect(Rect{sx, sy2, cellW, cellH},
                    Color{(u8)(32+8*sAn), (u8)(32+6*sAn), (u8)(48+12*sAn), ga}, 5*sc, 8);

                // Rarity bar at bottom
                dl.AddFilledRect(Rect{sx+2*sc, sy2+cellH-3*sc, cellW-4*sc, 3*sc},
                    Color{rc.r, rc.g, rc.b, (u8)(ga*0.8f)});

                // Selected highlight
                if (selected)
                    dl.AddFilledRoundRect(Rect{sx-1, sy2-1, cellW+2, cellH+2},
                        Color{120, 70, 200, (u8)(ga*0.35f)}, 6*sc, 8);

                // Skin image
                TextureHandle skinTex = LoadSkinImage(wpn.name, sk.name);
                if (skinTex != INVALID_TEXTURE) {
                    f32 iH = cellH - 22*sc, iW = iH * 1.33f;
                    if (iW > cellW - 6*sc) { iW = cellW - 6*sc; iH = iW / 1.33f; }
                    dl.AddTexturedRect(Rect{sx+(cellW-iW)*0.5f, sy2+2*sc, iW, iH},
                        skinTex, Color{255,255,255,ga});
                }

                // Truncated name
                char dn[18]; strncpy(dn, sk.name, 15); dn[15] = '\0';
                if (strlen(sk.name) > 15) { dn[13] = '.'; dn[14] = '.'; dn[15] = '\0'; }
                Vec2 ns2 = Measure(dn, g_fontSm);
                Text(dl, sx + (cellW-ns2.x)*0.5f, sy2 + cellH - ns2.y - 5*sc,
                     Color{rc.r, rc.g, rc.b, ga}, dn, g_fontSm);

                if (sHov && g_input.IsMousePressed(MouseButton::Left)) {
                    g_selectedSkin = si;
                    g_detailWear = 0.0001f;
                    g_detailStatTrak = false;
                }
            }

            // ===== DETAIL PANEL (right) =====
            f32 dpX = cX + gridW + 10*sc;
            f32 dpY = gridY;
            f32 dpH = gridH;

            dl.AddFilledRoundRect(Rect{dpX, dpY, detailW, dpH}, Color{28, 28, 42, ga}, 6*sc, 8);

            if (g_selectedSkin >= 0 && g_selectedSkin < wpn.skinCount) {
                const auto& sk = wpn.skins[g_selectedSkin];
                Color rc = RarityColor(sk.rarity);

                // Large preview
                TextureHandle skinTex = LoadSkinImage(wpn.name, sk.name);
                f32 prevW = detailW - 16*sc, prevH = prevW * 0.75f;
                if (skinTex != INVALID_TEXTURE) {
                    dl.AddTexturedRect(Rect{dpX + 8*sc, dpY + 8*sc, prevW, prevH},
                        skinTex, Color{255,255,255,ga});
                } else {
                    dl.AddFilledRoundRect(Rect{dpX + 8*sc, dpY + 8*sc, prevW, prevH},
                        Color{35, 35, 50, ga}, 4*sc, 6);
                    Vec2 nimg = Measure("No Image", g_fontSm);
                    Text(dl, dpX + 8*sc + (prevW-nimg.x)*0.5f, dpY + 8*sc + (prevH-nimg.y)*0.5f,
                         Color{70, 65, 95, ga}, "No Image", g_fontSm);
                }

                f32 infoY = dpY + prevH + 18*sc;
                Text(dl, dpX + 10*sc, infoY, Color{180, 180, 200, ga}, wpn.name, g_fontSm);
                Text(dl, dpX + 10*sc, infoY + 14*sc, Color{rc.r,rc.g,rc.b,ga}, sk.name, g_font);
                Text(dl, dpX + 10*sc, infoY + 34*sc, rc, RarityName(sk.rarity), g_fontSm);

                char pkBuf[32]; snprintf(pkBuf, 32, "Paint Kit #%d", sk.paintKit);
                Text(dl, dpX + 10*sc, infoY + 48*sc, Color{100, 100, 130, ga}, pkBuf, g_fontSm);

                // Wear slider
                f32 wearY = infoY + 66*sc;
                Text(dl, dpX + 10*sc, wearY, Color{160, 160, 190, ga}, "Wear", g_fontSm);
                char wearBuf[16]; snprintf(wearBuf, 16, "%.4f", g_detailWear);
                Vec2 ws = Measure(wearBuf, g_fontSm);
                Text(dl, dpX + detailW - ws.x - 10*sc, wearY, Color{160, 120, 220, ga}, wearBuf, g_fontSm);
                {
                    f32 slX = dpX + 10*sc, slY = wearY + 16*sc;
                    f32 slW = detailW - 20*sc, slH2 = 5*sc;
                    f32 fillW = slW * g_detailWear;
                    dl.AddFilledRoundRect(Rect{slX, slY, slW, slH2}, Color{40, 40, 58, ga}, 3*sc, 6);
                    if (fillW > 1)
                        dl.AddFilledRoundRect(Rect{slX, slY, fillW, slH2}, Color{120, 70, 200, ga}, 3*sc, 6);
                    dl.AddFilledRoundRect(Rect{slX + fillW - 4*sc, slY - 2*sc, 8*sc, slH2 + 4*sc},
                        Color{160, 120, 220, ga}, 4*sc, 8);
                    if (Hit(slX, slY - 6*sc, slW, slH2 + 12*sc) && g_input.IsMousePressed(MouseButton::Left)) {
                        f32 t = (g_input.MousePos().x - slX) / slW;
                        if (t < 0) t = 0; if (t > 1) t = 1;
                        g_detailWear = t;
                    }
                }

                // StatTrak toggle
                f32 stY = wearY + 34*sc;
                g_detailStatTrak = UISettingToggle(dl, "StatTrak", g_detailStatTrak,
                    dpX + 10*sc, stY, detailW - 20*sc, sc, ga, Hash("_det_st2"));

                // ADD TO INVENTORY button
                f32 btnW = detailW - 20*sc, btnH = 30*sc;
                f32 btnX = dpX + 10*sc, btnY = dpY + dpH - btnH - 10*sc;
                u32 addId = Hash("_add_inv2");
                bool addHov = Hit(btnX, btnY, btnW, btnH);
                f32 addAn = Anim(addId, addHov ? 1.0f : 0.0f);

                dl.AddFilledRoundRect(Rect{btnX, btnY, btnW, btnH},
                    Color{90, 50, 160, (u8)((170 + 70*addAn)*a)}, 6*sc, 8);
                TextCenter(dl, Rect{btnX, btnY, btnW, btnH},
                    Color{255,255,255,ga}, "ADD TO INVENTORY", g_fontSm);

                if (addHov && g_input.IsMousePressed(MouseButton::Left)) {
                    InvItem item;
                    item.weaponIdx = g_selectedWeapon;
                    item.skinIdx = g_selectedSkin;
                    item.wear = g_detailWear;
                    item.statTrak = g_detailStatTrak;
                    item.customName[0] = '\0';
                    item.applied = false;
                    g_invItems.push_back(item);
                    WriteEquippedConfig();
                    g_toastMsg = "Skin added!"; g_toastCol = P::Green; g_toastTimer = 2;
                }
            } else {
                TextCenter(dl, Rect{dpX, dpY + dpH*0.4f, detailW, 20*sc},
                    Color{80, 80, 110, ga}, "Select a skin", g_fontSm);
            }
        }

    // ===================================================================
    // TAB: PROFILE
    // ===================================================================
    } else if (g_sideTab == 0) {
        Text(dl, cX + 10*sc, cY + 10*sc, Color{200, 200, 220, ga}, "Profile", g_fontMd);
        dl.AddFilledRect(Rect{cX, cY + 36*sc, cW, 1}, Color{50, 50, 70, (u8)(ga*0.5f)});

        f32 py = cY + 50*sc;
        const char* uname = (strlen(g_loginUser) > 0) ? g_loginUser : "User";
        Text(dl, cX + 10*sc, py, Color{180, 180, 200, ga}, "Username:", g_fontSm);
        Text(dl, cX + 90*sc, py, Color{220, 220, 240, ga}, uname, g_fontSm);
        py += 22*sc;
        char statBuf[64];
        snprintf(statBuf, 64, "Equipped: %d skins", (int)g_invItems.size());
        Text(dl, cX + 10*sc, py, Color{180, 180, 200, ga}, statBuf, g_fontSm);
        py += 22*sc;
        snprintf(statBuf, 64, "Weapons in DB: %d", g_weaponDBCount);
        Text(dl, cX + 10*sc, py, Color{180, 180, 200, ga}, statBuf, g_fontSm);
        py += 22*sc;
        snprintf(statBuf, 64, "Cached images: %d", (int)g_skinTexCache.size());
        Text(dl, cX + 10*sc, py, Color{180, 180, 200, ga}, statBuf, g_fontSm);

    // ===================================================================
    // TAB: CONFIGS
    // ===================================================================
    } else if (g_sideTab == 2) {
        Text(dl, cX + 10*sc, cY + 10*sc, Color{200, 200, 220, ga}, "Configs", g_fontMd);
        dl.AddFilledRect(Rect{cX, cY + 36*sc, cW, 1}, Color{50, 50, 70, (u8)(ga*0.5f)});

        f32 btnW = 140*sc, btnH = 30*sc;
        f32 btnX = cX + 10*sc, btnY = cY + 50*sc;

        // Save config button
        {
            u32 savId = Hash("_cfg_save2");
            bool savHov = Hit(btnX, btnY, btnW, btnH);
            f32 savAn = Anim(savId, savHov ? 1.0f : 0.0f);
            dl.AddFilledRoundRect(Rect{btnX, btnY, btnW, btnH},
                Color{50, 140, 50, (u8)((150+80*savAn)*a)}, 6*sc, 8);
            TextCenter(dl, Rect{btnX, btnY, btnW, btnH},
                Color{255,255,255,ga}, "Save Config", g_fontSm);
            if (savHov && g_input.IsMousePressed(MouseButton::Left)) {
                WriteEquippedConfig();
                g_toastMsg = "Config saved!"; g_toastCol = P::Green; g_toastTimer = 2;
            }
        }
        btnY += 40*sc;
        Text(dl, cX + 10*sc, btnY, Color{110, 110, 140, ga},
             "Configs auto-save to active.acpreset", g_fontSm);
        btnY += 16*sc;
        Text(dl, cX + 10*sc, btnY, Color{110, 110, 140, ga},
             "DLL reloads every 2 seconds", g_fontSm);
    }

    // Toast overlay
    DrawToast(dl, W, H);
}

// WNDPROC
// ============================================================================
static bool g_dragging = false;
static POINT g_dragPt = {};

static LRESULT CALLBACK WndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); g_running = false; return 0;
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) {
            g_width = LOWORD(lp); g_height = HIWORD(lp);
            if (g_width > 0 && g_height > 0) g_backend.Resize((u32)g_width, (u32)g_height);
            HRGN rgn = CreateRoundRectRgn(0, 0, g_width+1, g_height+1, CORNER*2, CORNER*2);
            SetWindowRgn(hw, rgn, TRUE);
        }
        return 0;
    case WM_MOUSEMOVE: {
        f32 x = (f32)(short)LOWORD(lp), y = (f32)(short)HIWORD(lp);
        g_input.OnMouseMove(x, y);
        if (g_dragging) {
            POINT pt; GetCursorPos(&pt);
            SetWindowPos(hw, nullptr, pt.x - g_dragPt.x, pt.y - g_dragPt.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        g_input.OnMouseDown(MouseButton::Left);
        POINT pt = {(SHORT)LOWORD(lp), (SHORT)HIWORD(lp)};
        bool canDrag = false;
        if (g_injPhase == INJ_CS2_MENU) {
            canDrag = (pt.y < 36 && pt.x < g_width - 60);
        } else if (g_screen == DASHBOARD) {
            canDrag = (pt.y < 40 && pt.x > 90 && pt.x < g_width - 60);
        } else {
            canDrag = (pt.y < 48 && pt.x < g_width - 90);
        }
        if (canDrag) { g_dragging = true; g_dragPt = pt; SetCapture(hw); }
        return 0;
    }
    case WM_LBUTTONUP:   g_input.OnMouseUp(MouseButton::Left); if (g_dragging) { g_dragging = false; ReleaseCapture(); } return 0;
    case WM_RBUTTONDOWN: g_input.OnMouseDown(MouseButton::Right); return 0;
    case WM_RBUTTONUP:   g_input.OnMouseUp(MouseButton::Right); return 0;
    case WM_MOUSEWHEEL:  g_input.OnMouseScroll((f32)GET_WHEEL_DELTA_WPARAM(wp) / 120.0f); return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        // Map Win32 VK codes → ACE Key enum
        Key k = Key::None;
        int vk = (int)wp;
        if (vk == VK_BACK)    k = Key::Backspace;
        else if (vk == VK_RETURN) k = Key::Enter;
        else if (vk == VK_TAB)    k = Key::Tab;
        else if (vk == VK_ESCAPE) k = Key::Escape;
        else if (vk == VK_DELETE) k = Key::Delete;
        else if (vk == VK_LEFT)   k = Key::Left;
        else if (vk == VK_RIGHT)  k = Key::Right;
        else if (vk == VK_UP)     k = Key::Up;
        else if (vk == VK_DOWN)   k = Key::Down;
        else if (vk == VK_HOME)   k = Key::Home;
        else if (vk == VK_END)    k = Key::End;
        else if (vk == VK_SHIFT)  k = Key::LeftShift;
        else if (vk == VK_CONTROL) k = Key::LeftCtrl;
        // INSERT handled in render loop with debounce — not here (avoids double-toggle race)
        else k = (Key)vk;
        g_input.OnKeyDown(k);
        return 0;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        Key k = Key::None;
        int vk = (int)wp;
        if (vk == VK_BACK)    k = Key::Backspace;
        else if (vk == VK_RETURN) k = Key::Enter;
        else if (vk == VK_TAB)    k = Key::Tab;
        else if (vk == VK_ESCAPE) k = Key::Escape;
        else if (vk == VK_DELETE) k = Key::Delete;
        else if (vk == VK_LEFT)   k = Key::Left;
        else if (vk == VK_RIGHT)  k = Key::Right;
        else if (vk == VK_UP)     k = Key::Up;
        else if (vk == VK_DOWN)   k = Key::Down;
        else if (vk == VK_HOME)   k = Key::Home;
        else if (vk == VK_END)    k = Key::End;
        else if (vk == VK_SHIFT)  k = Key::LeftShift;
        else if (vk == VK_CONTROL) k = Key::LeftCtrl;
        else k = (Key)vk;
        g_input.OnKeyUp(k);
        return 0;
    }
    case WM_CHAR:        if (wp >= 32 && wp < 127) g_input.OnTextInput((char)wp); return 0;
    }
    return DefWindowProcA(hw, msg, wp, lp);
}

// ============================================================================
// DRAWING COMPONENTS
// ============================================================================

// --- Background gradient mesh ---
static void DrawBg(DrawList& dl, f32 W, f32 H) {
    dl.AddFilledRect(Rect{0, 0, W, H}, P::Bg);

    // Animated accent orbs — soft blurred glow
    f32 t = g_time * 0.3f;
    f32 orbX1 = W * 0.2f + sinf(t * 0.7f) * 40.0f;
    f32 orbY1 = H * 0.25f + cosf(t * 0.5f) * 30.0f;
    f32 orbX2 = W * 0.8f + cosf(t * 0.6f) * 35.0f;
    f32 orbY2 = H * 0.7f + sinf(t * 0.8f) * 25.0f;

    f32 orbR = 120.0f;
    for (int i = 6; i >= 1; i--) {
        f32 r = orbR * (f32)i / 3.0f;
        u8 a = (u8)(4.0f * (7 - i));
        dl.AddFilledRoundRect(Rect{orbX1 - r, orbY1 - r, r*2, r*2},
                              Color{140, 80, 220, a}, r, 24);
        dl.AddFilledRoundRect(Rect{orbX2 - r, orbY2 - r, r*2, r*2},
                              Color{160, 80, 255, a}, r, 24);
    }
}

// --- Title bar ---
static void DrawTitleBar(DrawList& dl, f32 W) {
    // Subtle top bar bg
    dl.AddGradientRect(Rect{0, 0, W, 48},
        Color{14, 10, 24, 220}, Color{14, 10, 24, 220},
        Color{14, 10, 24, 120}, Color{14, 10, 24, 120});

    // AC branding
    Vec2 ls = Measure("AC", g_fontMd);
    Text(dl, 20, (48 - ls.y) * 0.5f, P::Accent1, "AC", g_fontMd);
    Vec2 vs = Measure("LOADER", g_fontSm);
    Text(dl, 20 + ls.x + 8, (48 - vs.y) * 0.5f, P::T3, "LOADER", g_fontSm);

    // Minimize button
    f32 btnSz = 12.0f;
    f32 minX = W - 68, btnY = 18;
    u32 minId = Hash("_min");
    bool minH = Hit(minX - 6, btnY - 6, btnSz + 12, btnSz + 12);
    f32 minA = Anim(minId, minH ? 1.0f : 0.0f);
    dl.AddFilledRoundRect(Rect{minX - 8, btnY - 8, btnSz + 16, btnSz + 16},
                          Fade(Color{255,255,255,255}, 0.04f * minA), 6, 8);
    dl.AddLine(Vec2{minX, btnY + btnSz*0.5f}, Vec2{minX + btnSz, btnY + btnSz*0.5f},
               Color{170, 155, 200, (u8)(150 + 105 * minA)}, 1.5f);
    if (minH && g_input.IsMousePressed(MouseButton::Left)) ShowWindow(g_hwnd, SW_MINIMIZE);

    // Close button
    f32 clsX = W - 36;
    u32 clsId = Hash("_cls");
    bool clsH = Hit(clsX - 6, btnY - 6, btnSz + 12, btnSz + 12);
    f32 clsA = Anim(clsId, clsH ? 1.0f : 0.0f);
    dl.AddFilledRoundRect(Rect{clsX - 8, btnY - 8, btnSz + 16, btnSz + 16},
                          Fade(P::Red, 0.15f * clsA), 6, 8);
    f32 cx = clsX + btnSz*0.5f, cy = btnY + btnSz*0.5f, s = 4.0f;
    Color xc{160, 165, 190, (u8)(150 + 105 * clsA)};
    dl.AddLine(Vec2{cx-s, cy-s}, Vec2{cx+s, cy+s}, xc, 1.5f);
    dl.AddLine(Vec2{cx+s, cy-s}, Vec2{cx-s, cy+s}, xc, 1.5f);
    if (clsH && g_input.IsMousePressed(MouseButton::Left)) g_running = false;

    // Bottom separator — accent gradient line
    dl.AddGradientRect(Rect{0, 47, W, 1},
        Color{140, 80, 220, 0},  Color{140, 80, 220, 40},
        Color{160, 80, 255, 40}, Color{160, 80, 255, 0});
}

// --- Gradient accent button ---
static bool GradientButton(DrawList& dl, const char* label, Rect r, bool enabled = true) {
    u32 id = Hash(label);
    bool hov = enabled && Hit(r.x, r.y, r.w, r.h);
    bool click = hov && g_input.IsMousePressed(MouseButton::Left);
    f32 a = Anim(id, hov ? 1.0f : 0.0f);

    // Glow
    if (a > 0.01f) {
        for (int i = 4; i >= 1; i--) {
            f32 spread = (f32)i * 2.0f;
            Color gc = Fade(P::Accent1, 0.04f * a * (5.0f - i));
            dl.AddFilledRoundRect(Rect{r.x - spread, r.y - spread,
                                        r.w + spread*2, r.h + spread*2},
                                  gc, 12.0f + spread, 12);
        }
    }

    // Solid rounded rect with mixed accent
    f32 op = enabled ? (0.85f + 0.15f * a) : 0.35f;
    Color c1 = Fade(P::Accent1, op);
    Color c2 = Fade(P::Accent2, op);
    Color mixed = Mix(c1, c2, 0.5f);
    dl.AddFilledRoundRect(r, mixed, 10.0f, 12);

    // Top shine
    if (a > 0.01f) {
        dl.AddFilledRoundRect(Rect{r.x + 2, r.y + 2, r.w - 4, r.h * 0.35f},
                              Fade(Color{255,255,255,255}, 0.07f * a), 8.0f, 8);
    }

    Color tc = enabled ? Color{255,255,255,255} : Color{200,200,210,140};
    TextCenter(dl, r, tc, label, g_font);
    return click && enabled;
}

// --- Input field ---
static void InputField(DrawList& dl, const char* label, char* buf, int sz,
                        Rect r, bool password = false,
                        const char* nextFieldLabel = nullptr) {
    u32 id = Hash(label);
    bool focused = g_focusedField == id;
    bool hov = Hit(r.x, r.y, r.w, r.h);
    f32 a = Anim(id, focused ? 1.0f : (hov ? 0.4f : 0.0f));

    // Focus glow
    if (a > 0.3f) {
        Color gc = Fade(P::Accent1, 0.08f * a);
        dl.AddFilledRoundRect(Rect{r.x - 3, r.y - 3, r.w + 6, r.h + 6}, gc, 13.0f, 12);
    }

    // Border
    Color bc = Mix(P::Border, P::Accent1, a * 0.6f);
    dl.AddFilledRoundRect(Rect{r.x - 1, r.y - 1, r.w + 2, r.h + 2}, bc, 11.0f, 12);

    // Background
    Color bg = focused ? P::InputFoc : P::Input;
    dl.AddFilledRoundRect(r, bg, 10.0f, 12);

    // Click to focus
    if (hov && g_input.IsMousePressed(MouseButton::Left))
        g_focusedField = id;

    // Text input
    if (focused) {
        int len = (int)strlen(buf);
        std::string_view inp = g_input.TextInput();
        for (char c : inp)
            if (c >= 32 && c < 127 && len < sz - 1) { buf[len++] = c; buf[len] = '\0'; }
        if (g_input.IsKeyPressed(Key::Backspace) && len > 0) buf[len - 1] = '\0';

        // Ctrl+V paste from clipboard
        if (g_input.Ctrl() && g_input.IsKeyPressed((Key)'V')) {
            if (OpenClipboard(nullptr)) {
                HANDLE hData = GetClipboardData(CF_TEXT);
                if (hData) {
                    const char* clip = (const char*)GlobalLock(hData);
                    if (clip) {
                        len = (int)strlen(buf);
                        for (int i = 0; clip[i] && len < sz - 1; i++) {
                            if (clip[i] >= 32 && clip[i] < 127) { buf[len++] = clip[i]; buf[len] = '\0'; }
                        }
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
        }

        // Tab → focus next field
        if (g_input.IsKeyPressed(Key::Tab) && nextFieldLabel) {
            g_focusedField = Hash(nextFieldLabel);
        }
    }

    // Measure line height
    f32 lh = 14.0f;
    const FontInstance* fi = g_fontAtlas.GetFont(g_font);
    if (fi) lh = fi->lineHeight;

    int slen = (int)strlen(buf);
    f32 textY = r.y + (r.h - lh) * 0.5f;

    if (slen > 0) {
        std::string display = password ? std::string(slen, '*') : std::string(buf);
        Text(dl, r.x + 16, textY, P::T1, display.c_str());
        if (focused) {
            f32 blink = fmodf(g_time * 2.0f, 2.0f);
            if (blink < 1.0f) {
                Vec2 tw = Measure(display.c_str());
                dl.AddFilledRect(Rect{r.x + 16 + tw.x + 2, r.y + 10, 1.5f, r.h - 20}, P::Accent1);
            }
        }
    } else {
        Text(dl, r.x + 16, textY, P::T3, label);
        if (focused) {
            f32 blink = fmodf(g_time * 2.0f, 2.0f);
            if (blink < 1.0f)
                dl.AddFilledRect(Rect{r.x + 16, r.y + 10, 1.5f, r.h - 20}, P::Accent1);
        }
    }
}

// --- Icon drawing helpers (drawn with lines/shapes) ---

// Globe icon — circle with horizontal/vertical lines
static void DrawIconGlobe(DrawList& dl, f32 cx, f32 cy, f32 r, Color c) {
    // Outer circle
    dl.AddFilledRoundRect(Rect{cx - r, cy - r, r*2, r*2}, Fade(c, 0.0f), r, 16);
    // Ring (approximated with thick lines)
    int segs = 20;
    for (int i = 0; i < segs; i++) {
        f32 a1 = (f32)i / segs * 6.2832f;
        f32 a2 = (f32)(i+1) / segs * 6.2832f;
        dl.AddLine(Vec2{cx + cosf(a1)*r, cy + sinf(a1)*r},
                   Vec2{cx + cosf(a2)*r, cy + sinf(a2)*r}, c, 1.5f);
    }
    // Horizontal line (equator)
    dl.AddLine(Vec2{cx - r, cy}, Vec2{cx + r, cy}, c, 1.0f);
    // Vertical meridian
    for (int i = 0; i < segs; i++) {
        f32 a1 = (f32)i / segs * 6.2832f;
        f32 a2 = (f32)(i+1) / segs * 6.2832f;
        dl.AddLine(Vec2{cx + cosf(a1)*r*0.5f, cy + sinf(a1)*r},
                   Vec2{cx + cosf(a2)*r*0.5f, cy + sinf(a2)*r}, c, 1.0f);
    }
}

// Shopping cart icon
static void DrawIconCart(DrawList& dl, f32 x, f32 y, f32 sz, Color c) {
    f32 s = sz * 0.5f;
    // Handle
    dl.AddLine(Vec2{x - s*0.8f, y - s*0.7f}, Vec2{x - s*0.4f, y - s*0.7f}, c, 1.5f);
    // Cart body
    dl.AddLine(Vec2{x - s*0.4f, y - s*0.7f}, Vec2{x - s*0.2f, y + s*0.3f}, c, 1.5f);
    dl.AddLine(Vec2{x - s*0.2f, y + s*0.3f}, Vec2{x + s*0.6f, y + s*0.3f}, c, 1.5f);
    dl.AddLine(Vec2{x + s*0.6f, y + s*0.3f}, Vec2{x + s*0.7f, y - s*0.5f}, c, 1.5f);
    dl.AddLine(Vec2{x + s*0.7f, y - s*0.5f}, Vec2{x - s*0.3f, y - s*0.5f}, c, 1.5f);
    // Wheels
    dl.AddFilledRoundRect(Rect{x - s*0.1f, y + s*0.5f, s*0.3f, s*0.3f}, c, s*0.15f, 8);
    dl.AddFilledRoundRect(Rect{x + s*0.35f, y + s*0.5f, s*0.3f, s*0.3f}, c, s*0.15f, 8);
}

// Person with headphones icon
static void DrawIconSupport(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 r = sz * 0.5f;
    // Head (circle)
    f32 headR = r * 0.42f;
    int segs = 16;
    for (int i = 0; i < segs; i++) {
        f32 a1 = (f32)i / segs * 6.2832f;
        f32 a2 = (f32)(i+1) / segs * 6.2832f;
        dl.AddLine(Vec2{cx + cosf(a1)*headR, cy - r*0.15f + sinf(a1)*headR},
                   Vec2{cx + cosf(a2)*headR, cy - r*0.15f + sinf(a2)*headR}, c, 1.5f);
    }
    // Body/shoulders (arc at bottom)
    f32 bodyR = r * 0.75f;
    for (int i = 0; i < segs/2; i++) {
        f32 a1 = 3.14159f + (f32)i / (segs/2) * 3.14159f;
        f32 a2 = 3.14159f + (f32)(i+1) / (segs/2) * 3.14159f;
        dl.AddLine(Vec2{cx + cosf(a1)*bodyR, cy + r*0.75f + sinf(a1)*bodyR*0.5f},
                   Vec2{cx + cosf(a2)*bodyR, cy + r*0.75f + sinf(a2)*bodyR*0.5f}, c, 1.5f);
    }
    // Headphone band (arc over head)
    f32 bandR = headR * 1.3f;
    for (int i = 0; i < segs/2; i++) {
        f32 a1 = 3.14159f + (f32)i / (segs/2) * 3.14159f;
        f32 a2 = 3.14159f + (f32)(i+1) / (segs/2) * 3.14159f;
        dl.AddLine(Vec2{cx + cosf(a1)*bandR, cy - r*0.15f + sinf(a1)*bandR},
                   Vec2{cx + cosf(a2)*bandR, cy - r*0.15f + sinf(a2)*bandR}, c, 2.0f);
    }
    // Earpieces (small filled rects on sides)
    dl.AddFilledRoundRect(Rect{cx - bandR - 2, cy - r*0.3f, 5, r*0.4f}, c, 2.0f, 4);
    dl.AddFilledRoundRect(Rect{cx + bandR - 3, cy - r*0.3f, 5, r*0.4f}, c, 2.0f, 4);
    // Mic (small line from right earpiece)
    dl.AddLine(Vec2{cx + bandR, cy + r*0.05f}, Vec2{cx + bandR - 2, cy + r*0.35f}, c, 1.5f);
    dl.AddFilledRoundRect(Rect{cx + bandR - 5, cy + r*0.32f, 6, 4}, c, 2.0f, 4);
}

// --- Glass card container ---
static void GlassCard(DrawList& dl, Rect r) {
    // Shadow layers
    for (int i = 4; i >= 1; i--) {
        f32 sp = (f32)i * 3.0f;
        dl.AddFilledRoundRect(Rect{r.x - sp + 2, r.y - sp + 4, r.w + sp*2 - 4, r.h + sp*2},
                              Fade(Color{0,0,0,255}, 0.06f * (5 - i)), 14.0f + sp, 12);
    }

    // Border
    dl.AddFilledRoundRect(Rect{r.x - 1, r.y - 1, r.w + 2, r.h + 2}, P::Border, 15.0f, 12);

    // Card bg
    dl.AddFilledRoundRect(r, P::Card, 14.0f, 12);

    // Top edge accent
    dl.AddGradientRect(Rect{r.x + 14, r.y, r.w - 28, 1},
        Fade(P::Accent1, 0.0f), Fade(P::Accent1, 0.3f),
        Fade(P::Accent2, 0.3f), Fade(P::Accent2, 0.0f));
}

// --- Status pill ---
static void Pill(DrawList& dl, f32 x, f32 y, const char* label, const char* value, Color accent) {
    Vec2 ls = Measure(label, g_fontSm);
    Vec2 vs = Measure(value, g_fontSm);
    f32 w = (std::max)(ls.x, vs.x) + 24;
    f32 h = 48.0f;

    dl.AddFilledRoundRect(Rect{x, y, w, h}, P::Surface, 10.0f, 10);
    // Accent top bar
    dl.AddFilledRoundRect(Rect{x + 8, y + 4, w - 16, 2}, Fade(accent, 0.5f), 1.0f, 4);

    Text(dl, x + 12, y + 10, P::T3, label, g_fontSm);
    Text(dl, x + 12, y + 27, accent, value, g_fontSm);
}

// --- Toast notification ---
static void DrawToast(DrawList& dl, f32 W, f32 H) {
    if (g_toastTimer <= 0) return;
    g_toastTimer -= g_dt;
    f32 a = g_toastTimer > 0.5f ? 1.0f : g_toastTimer / 0.5f;
    if (a <= 0.01f) return;

    f32 slideY = (1.0f - a) * 20.0f;
    Vec2 ts = Measure(g_toastMsg.c_str());
    f32 tw = ts.x + 40;
    f32 th = 38;
    f32 tx = (W - tw) * 0.5f;
    f32 ty = H - 60 + slideY;

    dl.AddFilledRoundRect(Rect{tx - 2, ty + 2, tw + 4, th + 2},
                          Fade(Color{0,0,0,255}, 0.3f * a), 10.0f, 12);
    dl.AddFilledRoundRect(Rect{tx, ty, tw, th}, Fade(P::Card, a), 10.0f, 12);
    dl.AddFilledRoundRect(Rect{tx + 4, ty + 8, 3, th - 16}, Fade(g_toastCol, a), 1.5f, 4);
    TextCenter(dl, Rect{tx, ty, tw, th}, Fade(P::T1, a), g_toastMsg.c_str());
}

// ============================================================================
// SCREEN: LOGIN
// ============================================================================
static void ScreenLogin(DrawList& dl, f32 W, f32 H) {
    DrawBg(dl, W, H);
    DrawTitleBar(dl, W);

    // Centered glass card — fits 380px window
    f32 cardW = (std::min)(W - 20.0f, 356.0f);
    f32 cardH = 340;
    f32 cardX = (W - cardW) * 0.5f;
    f32 cardY = 60;
    GlassCard(dl, Rect{cardX, cardY, cardW, cardH});

    f32 cx = cardX + 24;
    f32 cw = cardW - 48;
    f32 cy = cardY + 22;

    // Title
    Vec2 titleSz = Measure("Welcome back", g_fontLg);
    Text(dl, cardX + (cardW - titleSz.x) * 0.5f, cy, P::T1, "Welcome back", g_fontLg);
    cy += titleSz.y + 2;

    Vec2 subSz = Measure("Sign in to continue", g_fontSm);
    Text(dl, cardX + (cardW - subSz.x) * 0.5f, cy, P::T2, "Sign in to continue", g_fontSm);
    cy += subSz.y + 20;

    // Fields
    InputField(dl, "Username", g_loginUser, sizeof(g_loginUser),
               Rect{cx, cy, cw, 40}, false, "Password");
    cy += 50;

    InputField(dl, "Password", g_loginPass, sizeof(g_loginPass),
               Rect{cx, cy, cw, 40}, true, "Username");
    cy += 46;

    // Remember Me checkbox
    {
        u32 cbId = Hash("_remember_me");
        f32 cbSz = 16;
        bool cbH = Hit(cx, cy, cbSz + 90, cbSz + 4);
        f32 cbA = Anim(cbId, cbH ? 1.0f : 0.0f);
        if (cbH && g_input.IsMousePressed(MouseButton::Left))
            g_rememberMe = !g_rememberMe;

        // Checkbox border
        Color cbc = Mix(P::Border, P::Accent1, g_rememberMe ? 0.8f : cbA * 0.4f);
        dl.AddFilledRoundRect(Rect{cx, cy, cbSz, cbSz}, cbc, 3.0f, 6);
        dl.AddFilledRoundRect(Rect{cx + 1, cy + 1, cbSz - 2, cbSz - 2},
                              g_rememberMe ? P::Accent1 : P::Input, 2.0f, 6);
        // Checkmark
        if (g_rememberMe) {
            f32 mx = cx + 3, my = cy + 4;
            dl.AddLine(Vec2{mx, my + 4}, Vec2{mx + 3, my + 7}, Color{255,255,255,255}, 2.0f);
            dl.AddLine(Vec2{mx + 3, my + 7}, Vec2{mx + 9, my + 1}, Color{255,255,255,255}, 2.0f);
        }
        Text(dl, cx + cbSz + 8, cy + 1, Mix(P::T2, P::T1, cbA), "Remember me", g_fontSm);
    }
    cy += 24;

    // Error
    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= g_dt;
        f32 ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        Vec2 es = Measure(g_authError.c_str(), g_fontSm);
        Text(dl, cardX + (cardW - es.x) * 0.5f, cy, Fade(P::Red, ea), g_authError.c_str(), g_fontSm);
    }
    cy += 16;

    // Sign in button
    if (GradientButton(dl, "SIGN IN", Rect{cx, cy, cw, 42})) {
        if (DoLogin(g_loginUser, g_loginPass)) {
            g_screen = DASHBOARD; g_navIndex = 0;
            if (g_rememberMe) SaveRememberMe(); else ClearRememberMe();
        }
    }
    cy += 52;

    // Enter key shortcut
    if (g_input.IsKeyPressed(Key::Enter) && strlen(g_loginUser) > 0 && strlen(g_loginPass) > 0) {
        if (DoLogin(g_loginUser, g_loginPass)) {
            g_screen = DASHBOARD; g_navIndex = 0;
            if (g_rememberMe) SaveRememberMe(); else ClearRememberMe();
        }
    }

    // Divider
    f32 divY = cy;
    dl.AddGradientRect(Rect{cx, divY, cw * 0.42f, 1},
        Color{45,35,65,0}, Color{45,35,65,0}, P::Border, P::Border);
    Text(dl, cx + cw * 0.42f + 8, divY - 6, P::T3, "or", g_fontSm);
    dl.AddGradientRect(Rect{cx + cw * 0.58f, divY, cw * 0.42f, 1},
        P::Border, P::Border, Color{45,35,65,0}, Color{45,35,65,0});

    cy += 18;

    // Create account link
    const char* caText = "Create new account";
    Vec2 caTs = Measure(caText, g_fontSm);
    f32 caX = cardX + (cardW - caTs.x) * 0.5f;
    u32 caId = Hash("_create_link");
    bool caH = Hit(caX, cy, caTs.x, caTs.y + 4);
    f32 caA = Anim(caId, caH ? 1.0f : 0.0f);
    Text(dl, caX, cy, Mix(P::T2, P::Accent1, 0.3f + 0.7f * caA), caText, g_fontSm);
    if (caH && g_input.IsMousePressed(MouseButton::Left)) {
        g_screen = SIGNUP; g_authError.clear(); g_authErrorTimer = 0;
        memset(g_signupKey, 0, 64); memset(g_signupUser, 0, 64); memset(g_signupPass, 0, 64); memset(g_signupPass2, 0, 64);
    }

    // Footer
    Vec2 fts = Measure("AC Loader v4.0", g_fontSm);
    Text(dl, (W - fts.x) * 0.5f, H - 22, P::T3, "AC Loader v4.0", g_fontSm);

    DrawToast(dl, W, H);
}

// ============================================================================
// SCREEN: SIGNUP
// ============================================================================
static void ScreenSignup(DrawList& dl, f32 W, f32 H) {
    DrawBg(dl, W, H);
    DrawTitleBar(dl, W);

    f32 cardW = (std::min)(W - 20.0f, 356.0f);
    f32 cardH = 420;
    f32 cardX = (W - cardW) * 0.5f, cardY = 48;
    GlassCard(dl, Rect{cardX, cardY, cardW, cardH});

    f32 cx = cardX + 24, cw = cardW - 48;
    f32 cy = cardY + 20;

    Vec2 ts = Measure("Create account", g_fontLg);
    Text(dl, cardX + (cardW - ts.x) * 0.5f, cy, P::T1, "Create account", g_fontLg);
    cy += ts.y + 2;

    Vec2 ss = Measure("Enter your license key to register", g_fontSm);
    Text(dl, cardX + (cardW - ss.x) * 0.5f, cy, P::T2, "Enter your license key to register", g_fontSm);
    cy += ss.y + 14;

    InputField(dl, "License Key", g_signupKey, sizeof(g_signupKey), Rect{cx, cy, cw, 40}, false, "Username");
    cy += 46;
    InputField(dl, "Username", g_signupUser, sizeof(g_signupUser), Rect{cx, cy, cw, 40}, false, "Password");
    cy += 46;
    InputField(dl, "Password", g_signupPass, sizeof(g_signupPass), Rect{cx, cy, cw, 40}, true, "Confirm password");
    cy += 46;
    InputField(dl, "Confirm password", g_signupPass2, sizeof(g_signupPass2), Rect{cx, cy, cw, 40}, true, "License Key");
    cy += 44;

    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= g_dt;
        f32 ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        Vec2 es = Measure(g_authError.c_str(), g_fontSm);
        Text(dl, cardX + (cardW - es.x) * 0.5f, cy, Fade(P::Red, ea), g_authError.c_str(), g_fontSm);
    }
    cy += 16;

    if (GradientButton(dl, "ACTIVATE & REGISTER", Rect{cx, cy, cw, 40})) {
        if (strcmp(g_signupPass, g_signupPass2) != 0) { g_authError = "Passwords don't match"; g_authErrorTimer = 3; }
        else if (DoSignup(g_signupUser, g_signupPass, g_signupKey)) {
            if (DoLogin(g_signupUser, g_signupPass)) { g_screen = DASHBOARD; g_navIndex = 0; }
        }
    }
    if (g_input.IsKeyPressed(Key::Enter) && strlen(g_signupUser) > 0 && strlen(g_signupPass) > 0) {
        if (strcmp(g_signupPass, g_signupPass2) != 0) { g_authError = "Passwords don't match"; g_authErrorTimer = 3; }
        else if (DoSignup(g_signupUser, g_signupPass, g_signupKey)) {
            if (DoLogin(g_signupUser, g_signupPass)) { g_screen = DASHBOARD; g_navIndex = 0; }
        }
    }
    cy += 48;

    // Back link
    const char* bl = "Already have an account? Sign in";
    Vec2 bls = Measure(bl, g_fontSm);
    f32 blX = cardX + (cardW - bls.x) * 0.5f;
    u32 blId = Hash("_back_link");
    bool blH = Hit(blX, cy, bls.x, bls.y + 4);
    f32 blA = Anim(blId, blH ? 1.0f : 0.0f);
    Text(dl, blX, cy, Mix(P::T2, P::Accent1, 0.3f + 0.7f * blA), bl, g_fontSm);
    if (blH && g_input.IsMousePressed(MouseButton::Left)) {
        g_screen = LOGIN; g_authError.clear(); g_authErrorTimer = 0;
    }

    Vec2 fts = Measure("AC Loader v4.0", g_fontSm);
    Text(dl, (W - fts.x) * 0.5f, H - 22, P::T3, "AC Loader v4.0", g_fontSm);
    DrawToast(dl, W, H);
}

// ============================================================================
// CS2 DETAIL POPUP (modal overlay)
// ============================================================================
static void DrawPopup(DrawList& dl, f32 W, f32 H) {
    g_popupAnim += (g_showPopup ? 1.0f - g_popupAnim : -g_popupAnim) * (std::min)(1.0f, 14.0f * g_dt);
    if (g_popupAnim < 0.01f) { g_popupAnim = 0; return; }
    f32 a = g_popupAnim;

    // Dim overlay — click outside to close
    dl.AddFilledRect(Rect{0, 0, W, H}, Fade(Color{0,0,0,255}, 0.55f * a));
    f32 pw = W - 40, ph = H - 60;
    f32 px = (W - pw) * 0.5f;
    f32 py = 30 + (1.0f - a) * 20.0f;
    if (g_input.IsMousePressed(MouseButton::Left) && !Hit(px, py, pw, ph))
        g_showPopup = false;

    // Shadow
    dl.AddFilledRoundRect(Rect{px + 3, py + 5, pw - 6, ph},
                          Fade(Color{0,0,0,255}, 0.4f * a), 14.0f, 12);
    // Border
    dl.AddFilledRoundRect(Rect{px - 1, py - 1, pw + 2, ph + 2},
                          Fade(P::Border, a), 15.0f, 12);
    // Background
    dl.AddFilledRoundRect(Rect{px, py, pw, ph}, Fade(P::Card, a), 14.0f, 12);

    // Top accent line
    dl.AddGradientRect(Rect{px + 16, py, pw - 32, 2},
        Fade(P::Accent1, 0.0f), Fade(P::Accent1, 0.6f * a),
        Fade(P::Accent2, 0.6f * a), Fade(P::Accent2, 0.0f));

    f32 cx = px + 16, cw = pw - 32;
    f32 cy = py + 14;

    // ===== HEADER: Icon + Title + Close =====
    {
        // CS2 icon (real logo image, rounded)
        f32 iconSz = 36;
        if (g_cs2Logo != INVALID_TEXTURE) {
            dl.AddTexturedRect(Rect{cx, cy, iconSz, iconSz},
                               g_cs2Logo, Fade(Color{255,255,255,255}, a));
        } else {
            dl.AddFilledRoundRect(Rect{cx, cy, iconSz, iconSz},
                                  Fade(Color{220, 160, 40, 255}, a), 8.0f, 10);
            TextCenter(dl, Rect{cx, cy, iconSz, iconSz},
                       Fade(Color{255,255,255,240}, a), "CS2", g_fontSm);
        }

        // Title
        Text(dl, cx + iconSz + 10, cy + 4, Fade(P::T1, a), "Counter-Strike 2", g_fontMd);

        // Close button (X) — top right
        f32 xbX = px + pw - 28, xbY = py + 12;
        u32 xbId = Hash("_popup_close");
        bool xbH = Hit(xbX - 6, xbY - 6, 24, 24);
        f32 xbA = Anim(xbId, xbH ? 1.0f : 0.0f);
        f32 s = 5.0f;
        Color xcc = Fade(Color{170, 155, 200, (u8)(140 + 115 * xbA)}, a);
        dl.AddLine(Vec2{xbX, xbY}, Vec2{xbX + s*2, xbY + s*2}, xcc, 1.5f);
        dl.AddLine(Vec2{xbX + s*2, xbY}, Vec2{xbX, xbY + s*2}, xcc, 1.5f);
        if (xbH && g_input.IsMousePressed(MouseButton::Left)) g_showPopup = false;
    }
    cy += 48;

    // Divider
    dl.AddFilledRect(Rect{cx, cy, cw, 1}, Fade(P::Border, a));
    cy += 12;

    // ===== LEFT COLUMN: Info rows  |  RIGHT COLUMN: Changelog =====
    f32 leftW = cw * 0.42f;
    f32 rightX = cx + leftW + 10;
    f32 infoY = cy;

    // Info rows (left side)
    struct InfoRow { const char* label; const char* value; };
    char expBuf[32]; snprintf(expBuf, 32, "%d days", g_sub.daysLeft);
    InfoRow rows[] = {
        {"Release",  "25.03.2023 17:00"},
        {"Updated",  "27.03.2023 20:57"},
        {"Expires",  expBuf},
        {"Version",  "1"},
    };
    for (auto& row : rows) {
        Text(dl, cx + 4, infoY, Fade(P::Accent1, a), "|", g_fontSm);
        Text(dl, cx + 14, infoY, Fade(P::T2, a), row.label, g_fontSm);
        Vec2 vs = Measure(row.value, g_fontSm);
        Text(dl, cx + leftW - vs.x, infoY, Fade(P::T1, a), row.value, g_fontSm);
        infoY += 20;
    }

    // Changelog (right side)
    f32 clY = cy;
    const char* changelog[] = {
        "- Inventory Changer til CS2",
    };
    for (auto* entry : changelog) {
        Text(dl, rightX, clY, Fade(P::T2, a), entry, g_fontSm);
        clY += 16;
    }

    // ===== LAUNCH BUTTON (bottom) =====
    f32 btnY = py + ph - 48;
    {
        const char* btnText;
        bool enabled;
        if (g_injected) { btnText = "LAUNCHED"; enabled = false; }
        else if (g_injPhase == INJ_WAIT_CS2) { btnText = "STARTER CS2..."; enabled = false; }
        else if (g_injPhase == INJ_WAIT_CS2_READY) { btnText = "VENTER P\xc3\x85 MENU..."; enabled = false; }
        else if (g_injecting) { btnText = "INJECTING..."; enabled = false; }
        else { btnText = "LAUNCH"; enabled = true; }

        u32 lbId = Hash("_popup_launch");
        bool lbH = enabled && Hit(cx + cw * 0.25f, btnY, cw * 0.5f, 36);
        bool lbC = lbH && g_input.IsMousePressed(MouseButton::Left);
        f32 lbA = Anim(lbId, lbH ? 1.0f : 0.0f);

        f32 op = enabled ? (0.85f + 0.15f * lbA) : 0.3f;
        Color bc = Fade(Mix(P::Accent1, P::Accent2, 0.4f), op * a);
        Rect btnR{cx + cw * 0.25f, btnY, cw * 0.5f, 36};
        dl.AddFilledRoundRect(btnR, bc, 8.0f, 10);

        // Play triangle icon
        f32 triX = btnR.x + btnR.w * 0.5f - 4;
        f32 triY2 = btnR.y + btnR.h * 0.5f;
        Color tc = enabled ? Fade(Color{255,255,255,255}, a) : Fade(Color{200,200,210,140}, a);
        dl.AddLine(Vec2{triX - 4, triY2 - 6}, Vec2{triX + 6, triY2}, tc, 2.0f);
        dl.AddLine(Vec2{triX + 6, triY2}, Vec2{triX - 4, triY2 + 6}, tc, 2.0f);
        dl.AddLine(Vec2{triX - 4, triY2 + 6}, Vec2{triX - 4, triY2 - 6}, tc, 2.0f);

        if (lbC && enabled) DoInject();
    }
}

// ============================================================================
// SCREEN: DASHBOARD — Sidebar + Content + Popup
// ============================================================================
static void ScreenDashboard(DrawList& dl, f32 W, f32 H) {
    DrawBg(dl, W, H);

    // ===== SIDEBAR (left 90px) =====
    f32 sideW = 90;
    dl.AddFilledRect(Rect{0, 0, sideW, H}, Color{14, 10, 24, 240});
    dl.AddFilledRect(Rect{sideW - 1, 0, 1, H}, P::Border);

    // Logo — "AC" with 3D shadow
    Vec2 logoSz = Measure("AC", g_fontLg);
    f32 logoX = (sideW - logoSz.x) * 0.5f;
    f32 logoY = 18;
    // Shadow layers for 3D depth
    Text(dl, logoX + 2, logoY + 3, Color{0, 0, 0, 80}, "AC", g_fontLg);
    Text(dl, logoX + 1, logoY + 2, Color{40, 20, 80, 120}, "AC", g_fontLg);
    Text(dl, logoX + 0.5f, logoY + 1, Color{80, 50, 140, 160}, "AC", g_fontLg);
    Text(dl, logoX, logoY, P::Accent1, "AC", g_fontLg);

    // Navigation items with icons
    f32 navY = 76;
    // --- Website (globe icon) ---
    {
        u32 nid = Hash("Website");
        f32 nY = navY;
        bool nH = Hit(0, nY, sideW, 36);
        f32 nA = Anim(nid, nH ? 1.0f : 0.0f);
        Color nc = Mix(P::T2, P::T1, nA);
        DrawIconGlobe(dl, sideW * 0.5f, nY + 10, 7.0f, nc);
        Vec2 lblSz = Measure("Website", g_fontSm);
        Text(dl, (sideW - lblSz.x) * 0.5f, nY + 22, nc, "Website", g_fontSm);
        if (nH && g_input.IsMousePressed(MouseButton::Left))
            ShellExecuteA(nullptr, "open", "https://github.com/pedziito/Skin-Changer", nullptr, nullptr, SW_SHOW);
    }
    // --- Support (person with headphones) ---
    {
        u32 nid = Hash("Support");
        f32 nY = navY + 42;
        bool nH = Hit(0, nY, sideW, 36);
        f32 nA = Anim(nid, nH ? 1.0f : 0.0f);
        Color nc = Mix(P::T2, P::T1, nA);
        DrawIconSupport(dl, sideW * 0.5f, nY + 10, 18.0f, nc);
        Vec2 lblSz = Measure("Support", g_fontSm);
        Text(dl, (sideW - lblSz.x) * 0.5f, nY + 22, nc, "Support", g_fontSm);
        if (nH && g_input.IsMousePressed(MouseButton::Left))
            ShellExecuteA(nullptr, "open", "https://github.com/pedziito/Skin-Changer/issues", nullptr, nullptr, SW_SHOW);
    }
    // --- Market (shopping cart) ---
    {
        u32 nid = Hash("Market");
        f32 nY = navY + 84;
        bool nH = Hit(0, nY, sideW, 36);
        f32 nA = Anim(nid, nH ? 1.0f : 0.0f);
        Color nc = Mix(P::T2, P::T1, nA);
        DrawIconCart(dl, sideW * 0.5f, nY + 10, 20.0f, nc);
        Vec2 lblSz = Measure("Market", g_fontSm);
        Text(dl, (sideW - lblSz.x) * 0.5f, nY + 22, nc, "Market", g_fontSm);
        if (nH && g_input.IsMousePressed(MouseButton::Left))
            ShellExecuteA(nullptr, "open", "https://csfloat.com", nullptr, nullptr, SW_SHOW);
    }

    // User avatar + name + logout — bottom of sidebar
    {
        f32 avSz = 30;
        f32 avX = 10;
        f32 avY = H - 64;
        // Avatar circle
        dl.AddFilledRoundRect(Rect{avX, avY, avSz, avSz},
                              Color{55, 40, 80, 255}, avSz * 0.5f, 16);
        if (!g_loggedUser.empty()) {
            char ini[2] = {(char)toupper(g_loggedUser[0]), '\0'};
            Vec2 iSz = Measure(ini, g_font);
            Text(dl, avX + (avSz - iSz.x) * 0.5f, avY + (avSz - iSz.y) * 0.5f,
                 P::T1, ini, g_font);
        }
        // Username — next to avatar
        Text(dl, avX + avSz + 6, avY + (avSz - 12) * 0.5f, P::T2, g_loggedUser.c_str(), g_fontSm);

        // Logout — below avatar
        u32 loId = Hash("_sidebar_logout");
        f32 loY = avY + avSz + 4;
        Vec2 loSz = Measure("Logout", g_fontSm);
        f32 loX = avX;
        bool loH = Hit(loX - 2, loY - 2, loSz.x + 8, loSz.y + 4);
        f32 loA = Anim(loId, loH ? 1.0f : 0.0f);
        Text(dl, loX, loY, Mix(P::T3, P::Red, loA * 0.7f), "Logout", g_fontSm);
        if (loH && g_input.IsMousePressed(MouseButton::Left)) {
            g_screen = LOGIN; g_loggedUser.clear(); g_hasSub = false;
            g_injected = false; g_injecting = false;
            g_showPopup = false; g_injPhase = INJ_IDLE;
            memset(g_loginPass, 0, 64);
            ClearRememberMe();
        }
    }

    // ===== CONTENT AREA (right side) =====
    f32 cX = sideW + 16;
    f32 cW = W - sideW - 32;

    // Min/Close buttons — right-aligned
    {
        f32 btnSz = 10.0f, btnY = 14;
        f32 minX = W - 52;
        u32 minId = Hash("_min");
        bool minH = Hit(minX - 4, btnY - 4, btnSz + 8, btnSz + 8);
        f32 minA = Anim(minId, minH ? 1.0f : 0.0f);
        dl.AddLine(Vec2{minX, btnY + btnSz*0.5f}, Vec2{minX + btnSz, btnY + btnSz*0.5f},
                   Color{170, 155, 200, (u8)(130 + 125 * minA)}, 1.5f);
        if (minH && g_input.IsMousePressed(MouseButton::Left)) ShowWindow(g_hwnd, SW_MINIMIZE);

        f32 clsX = W - 30;
        u32 clsId = Hash("_cls");
        bool clsH = Hit(clsX - 4, btnY - 4, btnSz + 8, btnSz + 8);
        f32 clsA = Anim(clsId, clsH ? 1.0f : 0.0f);
        f32 cx2 = clsX + btnSz*0.5f, cy2 = btnY + btnSz*0.5f, s = 4.0f;
        Color xc2{160, 165, 190, (u8)(130 + 125 * clsA)};
        dl.AddLine(Vec2{cx2-s, cy2-s}, Vec2{cx2+s, cy2+s}, xc2, 1.5f);
        dl.AddLine(Vec2{cx2+s, cy2-s}, Vec2{cx2-s, cy2+s}, xc2, 1.5f);
        if (clsH && g_input.IsMousePressed(MouseButton::Left)) g_running = false;
    }

    f32 y = 18;

    // Title — "Subscription" (bold)
    Text(dl, cX, y, P::T1, "Subscription", g_fontMd);
    y += 26;

    // Subtitle
    Text(dl, cX, y, P::T3, "Available subscriptions", g_fontSm);
    y += 28;

    // ============================================================
    // CS2 SUBSCRIPTION CARD — icon on RIGHT like reference image
    // ============================================================
    {
        f32 cardH = 64;
        Rect cardR{cX, y, cW, cardH};

        u32 cid = Hash("_cs2_card");
        bool cH = Hit(cardR.x, cardR.y, cardR.w, cardR.h);
        f32 cA = Anim(cid, cH ? 1.0f : 0.0f);

        // Shadow
        dl.AddFilledRoundRect(Rect{cardR.x + 2, cardR.y + 2, cardR.w - 4, cardR.h},
                              Fade(Color{0,0,0,255}, 0.15f), 10.0f, 10);
        // Border
        Color borderCol = Mix(P::Border, P::Accent1, cA * 0.4f);
        dl.AddFilledRoundRect(Rect{cardR.x - 1, cardR.y - 1, cardR.w + 2, cardR.h + 2},
                              borderCol, 11.0f, 10);
        // Card bg
        dl.AddFilledRoundRect(cardR, Mix(P::Card, P::CardHi, cA * 0.3f), 10.0f, 10);

        // Text — left side
        f32 txX = cardR.x + 14;
        Text(dl, txX, cardR.y + 14, P::T1, "Counter-Strike 2", g_font);

        char expStr[48];
        snprintf(expStr, 48, "Expires in %d days", g_sub.daysLeft);
        Text(dl, txX, cardR.y + 34, P::T3, expStr, g_fontSm);

        // Icon — right side (CS2 logo image, rounded)
        f32 icoSz = 36;
        f32 icoX = cardR.Right() - icoSz - 10;
        f32 icoY = cardR.y + (cardH - icoSz) * 0.5f;
        if (g_cs2Logo != INVALID_TEXTURE) {
            dl.AddTexturedRect(Rect{icoX, icoY, icoSz, icoSz},
                               g_cs2Logo, Color{255,255,255,255});
        } else {
            dl.AddFilledRoundRect(Rect{icoX, icoY, icoSz, icoSz},
                                  Color{220, 160, 40, 255}, 8.0f, 10);
            TextCenter(dl, Rect{icoX, icoY, icoSz, icoSz},
                       Color{255,255,255,240}, "CS2", g_fontSm);
        }

        // Click → open popup
        if (cH && g_input.IsMousePressed(MouseButton::Left))
            g_showPopup = true;

        y += cardH + 10;
    }

    // ============================================================
    // STATUS INFO — Detection status, game version etc.
    // ============================================================
    {
        y += 4;
        struct StatusItem { const char* label; const char* value; Color color; };
        StatusItem items[] = {
            {"Status",       "Undetected",  Color{80, 200, 120, 255}},
            {"Game Version", "Latest",      Color{160, 120, 230, 255}},
            {"Last Updated", __DATE__,       P::T2},
        };
        for (auto& item : items) {
            Text(dl, cX, y, P::T3, item.label, g_fontSm);
            Vec2 vSz = Measure(item.value, g_fontSm);
            Text(dl, cX + cW - vSz.x, y, item.color, item.value, g_fontSm);
            y += 18;
        }
    }

    DrawToast(dl, W, H);

    // Popup overlay (drawn on top of everything)
    if (g_showPopup || g_popupAnim > 0.01f)
        DrawPopup(dl, W, H);

    // Toast on very top so errors/success always visible
    if (g_showPopup || g_injPhase != INJ_IDLE)
        DrawToast(dl, W, H);
}

// ============================================================================
// ENTRY POINT
// ============================================================================
static int LoaderMain(HINSTANCE hInstance);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    LogInit();
    Log("=== AC Loader starting ===");
    __try {
        return LoaderMain(hInstance);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("CRASH: unhandled exception 0x%08X", GetExceptionCode());
        MessageBoxA(nullptr,
            "AC Loader crashed unexpectedly.\n\n"
            "Check ac_loader.log for details.\n"
            "Try running as Administrator.",
            "AC Loader - Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}

static int LoaderMain(HINSTANCE hInstance) {
    Log("Step 1: Init");
    srand((unsigned)time(nullptr));

    // DPI awareness — Per-Monitor V2 for sharp rendering on high-DPI displays
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    {
        HDC hdc = GetDC(nullptr);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        g_dpiScale = (f32)dpiX / 96.0f;
        if (g_dpiScale < 1.0f) g_dpiScale = 1.0f;
        if (g_dpiScale > 3.0f) g_dpiScale = 3.0f;
        Log("DPI: %d (scale=%.2f)", dpiX, g_dpiScale);
    }

    // Scale initial window size for DPI
    g_width  = (int)(480 * g_dpiScale);
    g_height = (int)(420 * g_dpiScale);

    LoadUsers();
    LoadKeys();
    SeedTestKeys();
    LoadRememberMe();
    Log("Step 2: Users loaded (%d), Keys loaded (%d)", (int)g_users.size(), (int)g_keys.size());

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ACLoaderV4";
    RegisterClassExA(&wc);
    Log("Step 3: Window class registered");

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    g_hwnd = CreateWindowExA(WS_EX_APPWINDOW, wc.lpszClassName, "AC Loader",
        WS_POPUP | WS_VISIBLE,
        (sw - g_width) / 2, (sh - g_height) / 2, g_width, g_height,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hwnd) {
        Log("FAIL: CreateWindowExA error=%lu", GetLastError());
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_OK); return 1;
    }
    Log("Step 4: Window created %dx%d", g_width, g_height);

    HRGN rgn = CreateRoundRectRgn(0, 0, g_width+1, g_height+1, CORNER*2, CORNER*2);
    SetWindowRgn(g_hwnd, rgn, TRUE);

    Log("Step 5: Initializing DX11...");
    if (!g_backend.Initialize(g_hwnd, (u32)g_width, (u32)g_height)) {
        Log("FAIL: DX11 init failed");
        MessageBoxA(nullptr, "DX11 init failed.\n\nCheck ac_loader.log", "Error", MB_OK);
        return 1;
    }
    Log("Step 6: DX11 OK");

    // Load embedded CS2 logo
    {
        int imgW = 0, imgH = 0, imgC = 0;
        unsigned char* pixels = stbi_load_from_memory(
            g_cs2LogoPng, (int)g_cs2LogoPngSize, &imgW, &imgH, &imgC, 4);
        if (pixels && imgW > 0 && imgH > 0) {
            TextureDesc td{};
            td.width = (u32)imgW;
            td.height = (u32)imgH;
            td.channels = 4;
            td.data = pixels;
            g_cs2Logo = g_backend.CreateTexture(td);
            stbi_image_free(pixels);
            Log("CS2 logo loaded: %dx%d handle=%llu", imgW, imgH, (unsigned long long)g_cs2Logo);
        } else {
            Log("WARN: CS2 logo decode failed");
        }
    }

    // Load embedded AC glow logo
    {
        int imgW = 0, imgH = 0, imgC = 0;
        unsigned char* pixels = stbi_load_from_memory(
            g_acGlowPng, (int)g_acGlowPngSize, &imgW, &imgH, &imgC, 4);
        if (pixels && imgW > 0 && imgH > 0) {
            TextureDesc td{};
            td.width = (u32)imgW;
            td.height = (u32)imgH;
            td.channels = 4;
            td.data = pixels;
            g_acGlowLogo = g_backend.CreateTexture(td);
            stbi_image_free(pixels);
            Log("AC glow logo loaded: %dx%d handle=%llu", imgW, imgH, (unsigned long long)g_acGlowLogo);
        } else {
            Log("WARN: AC glow logo decode failed");
        }
    }

    // Load new purple sunset AC logo (rounded)
    {
        int imgW = 0, imgH = 0, imgC = 0;
        unsigned char* pixels = stbi_load_from_memory(
            g_acLogoPng, (int)g_acLogoPngSize, &imgW, &imgH, &imgC, 4);
        if (pixels && imgW > 0 && imgH > 0) {
            TextureDesc td{};
            td.width = (u32)imgW;
            td.height = (u32)imgH;
            td.channels = 4;
            td.data = pixels;
            g_acLogo = g_backend.CreateTexture(td);
            stbi_image_free(pixels);
            Log("AC logo loaded: %dx%d handle=%llu", imgW, imgH, (unsigned long long)g_acLogo);
        } else {
            Log("WARN: AC logo decode failed");
        }
    }

    // Load fonts — bold/heavy weights for premium feel
    auto TryFont = [](const char* paths[], int count, f32 size) -> u32 {
        for (int i = 0; i < count; i++) {
            u32 id = g_fontAtlas.AddFont(paths[i], size, &g_backend);
            if (id != 0) return id;
        }
        return 0;
    };

    // Heavy weight for headings (Black > Bold > Regular fallback)
    const char* heavyFonts[] = {
        "C:\\Windows\\Fonts\\seguibl.ttf",   // Segoe UI Black
        "C:\\Windows\\Fonts\\segoeuib.ttf",  // Segoe UI Bold
        "C:\\Windows\\Fonts\\arialbd.ttf",   // Arial Bold
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    // Bold for body/buttons
    const char* boldFonts[] = {
        "C:\\Windows\\Fonts\\segoeuib.ttf",  // Segoe UI Bold
        "C:\\Windows\\Fonts\\seguisb.ttf",   // Segoe UI Semibold
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    // Semibold for small/body text
    const char* bodyFonts[] = {
        "C:\\Windows\\Fonts\\seguisb.ttf",   // Segoe UI Semibold
        "C:\\Windows\\Fonts\\segoeui.ttf",   // Segoe UI Regular
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
    };

    g_fontSm = TryFont(bodyFonts,  4, 14.0f * g_dpiScale);
    g_font   = TryFont(boldFonts,  4, 16.0f * g_dpiScale);
    g_fontMd = TryFont(boldFonts,  4, 19.0f * g_dpiScale);
    g_fontLg = TryFont(heavyFonts, 4, 28.0f * g_dpiScale);
    g_fontXl = TryFont(heavyFonts, 4, 36.0f * g_dpiScale);

    bool loaded = (g_fontSm && g_font && g_fontMd && g_fontLg && g_fontXl);
    if (!loaded) {
        Log("FAIL: No system font found");
        MessageBoxA(nullptr, "No system font found.\nCheck ac_loader.log", "Error", MB_OK);
        g_backend.Shutdown(); return 1;
    }
    Log("Step 7: Fonts loaded (bold/heavy weights)");

    // Auto-login if Remember Me was saved
    if (g_rememberMe && strlen(g_savedUser) > 0 && strlen(g_savedPass) > 0) {
        if (DoLogin(g_savedUser, g_savedPass)) {
            g_screen = DASHBOARD; g_navIndex = 0;
            Log("Auto-login: %s", g_savedUser);
        }
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);
    Log("Step 8: Window shown — entering render loop");

    LARGE_INTEGER freq, last;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);

    MSG msg = {};
    while (g_running) {
        // BeginFrame FIRST — then pump messages so input arrives fresh for this frame
        g_input.BeginFrame();

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }
        if (!g_running) break;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        g_dt = (f32)(now.QuadPart - last.QuadPart) / (f32)freq.QuadPart;
        if (g_dt > 0.1f) g_dt = 0.016f;
        g_time += g_dt;
        last = now;

        g_backend.SetClearColor(P::Bg);

        g_backend.BeginFrame();

        // Update injection flow (Steam/CS2 state machine)
        UpdateInjectionFlow();

        // CS2 menu visibility: debounced Insert toggle, hide when CS2 not focused
        // Also close loader if CS2 has exited
        static bool s_pendingShow = false; // Defer ShowWindow until after frame is drawn
        if (g_injPhase == INJ_CS2_MENU) {
            if (FindProc("cs2.exe") == 0) {
                g_running = false;
                break;
            }

            // Debounced Insert key polling (200ms cooldown prevents flicker)
            static bool s_insertWasDown = false;
            static LARGE_INTEGER s_lastToggleTime = {};
            bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
            if (insertDown && !s_insertWasDown) {
                LARGE_INTEGER now2; QueryPerformanceCounter(&now2);
                double msSinceToggle = (double)(now2.QuadPart - s_lastToggleTime.QuadPart)
                                       / (double)freq.QuadPart * 1000.0;
                if (msSinceToggle > 200.0) { // 200ms debounce
                    g_cs2MenuVisible = !g_cs2MenuVisible;
                    s_lastToggleTime = now2;
                    if (!g_cs2MenuVisible) {
                        // Hiding: immediate, reset animation
                        g_menuOpenAnim = 0.0f;
                        ShowWindow(g_hwnd, SW_HIDE);
                    } else {
                        // Showing: reset anim to 0, defer show until frame drawn
                        g_menuOpenAnim = 0.0f;
                        s_pendingShow = true;
                    }
                }
            }
            s_insertWasDown = insertDown;

            if (!g_cs2MenuVisible) {
                // Menu hidden — sleep but still cycle swapchain to prevent ghost frames
                if (IsWindowVisible(g_hwnd)) ShowWindow(g_hwnd, SW_HIDE);
                Sleep(50);
                g_backend.EndFrame();
                g_backend.Present(); // Keep swapchain buffers clean
                continue;
            }
            // Menu visible — check CS2 focus (auto-hide when CS2 not foreground)
            if (g_cs2Hwnd) {
                HWND fg = GetForegroundWindow();
                bool cs2Active = (fg == g_cs2Hwnd || fg == g_hwnd);
                if (!cs2Active) {
                    if (IsWindowVisible(g_hwnd)) ShowWindow(g_hwnd, SW_HIDE);
                    Sleep(50);
                    g_backend.EndFrame();
                    g_backend.Present(); // Keep swapchain buffers clean
                    continue;
                } else if (!IsWindowVisible(g_hwnd) && !s_pendingShow) {
                    // CS2 regained focus — defer show until after draw
                    s_pendingShow = true;
                }
            }
        }

        DrawList dl;
        f32 W = (f32)g_width, H = (f32)g_height;

        // CS2 in-game menu takes over the entire window after injection
        if (g_injPhase == INJ_CS2_MENU) {
            // Drive scale+opacity animation: 0→1 over ~200ms (ease-out)
            if (g_cs2MenuVisible) {
                g_menuOpenAnim += g_dt * 5.0f; // ~200ms to reach 1.0
                if (g_menuOpenAnim > 1.0f) g_menuOpenAnim = 1.0f;
            }
            DrawCS2Menu(dl, W, H);
        } else {
            switch (g_screen) {
            case LOGIN:     ScreenLogin(dl, W, H);     break;
            case SIGNUP:    ScreenSignup(dl, W, H);     break;
            case DASHBOARD: ScreenDashboard(dl, W, H);  break;
            }
        }

        Vec2 vp = g_backend.GetViewportSize();
        g_backend.RenderDrawList(dl, (u32)vp.x, (u32)vp.y);
        g_backend.EndFrame();
        g_backend.Present();

        // Deferred ShowWindow — only show AFTER frame is fully drawn & presented
        // This eliminates the "ghost frame" (stale content flash) on menu toggle
        if (s_pendingShow) {
            s_pendingShow = false;
            ShowWindow(g_hwnd, SW_SHOWNOACTIVATE);
            SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    g_backend.Shutdown();
    DestroyWindow(g_hwnd);
    UnregisterClassA(wc.lpszClassName, hInstance);
    Log("Shutdown complete");
    if (g_logFile) fclose(g_logFile);
    return 0;
}

#else
int main() {
    printf("AC Loader requires Windows (DX11).\n");
    return 0;
}
#endif
