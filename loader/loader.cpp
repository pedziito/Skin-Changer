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

static HWND g_hwnd    = nullptr;
static bool g_running = true;
static int  g_width   = 480;
static int  g_height  = 420;
static constexpr int CORNER = 16;

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
    // Backgrounds
    constexpr Color Bg       {8,   8,   14,  255};
    constexpr Color Card     {16,  16,  24,  245};
    constexpr Color CardHi   {22,  22,  34,  255};
    constexpr Color Surface  {20,  20,  30,  255};
    constexpr Color Input    {14,  14,  22,  255};
    constexpr Color InputFoc {18,  18,  28,  255};

    // Accent gradient endpoints
    constexpr Color Accent1  {79,  110, 255, 255};   // electric blue
    constexpr Color Accent2  {160, 80,  255, 255};   // vivid purple
    constexpr Color AccentDim{79,  110, 255, 60};

    // Status
    constexpr Color Green    {56,  230, 130, 255};
    constexpr Color Red      {255, 90,  90,  255};
    constexpr Color Yellow   {255, 210, 60,  255};
    constexpr Color Orange   {255, 150, 50,  255};

    // Text
    constexpr Color T1       {240, 240, 255, 255};
    constexpr Color T2       {140, 145, 170, 255};
    constexpr Color T3       {70,  72,  95,  255};

    // Borders
    constexpr Color Border   {35,  35,  55,  255};
    constexpr Color BorderLt {50,  50,  75,  180};
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

// CS2 Menu — Skins tab state
enum SkinViewState { 
    SKIN_VIEW_GRID = 0,      // Main grid with + slots
    SKIN_VIEW_WEAPONS = 1,   // Weapon picker popup (Glock, USP, etc.)
    SKIN_VIEW_SKINS = 2      // Skin list for selected weapon
};
static int g_skinView = SKIN_VIEW_GRID;
static int g_gridSlots[12] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; // weapon DB index per slot, -1 = empty
static int g_gridSkinChoice[12] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; // skin index per slot, -1 = none
static int g_activeSlot = -1;      // Which slot we're editing
static int g_selectedWeapon = -1;  // Selected weapon DB index in weapon picker
static int g_selectedSkin = -1;    // Selected skin in skin list
static int g_weaponScrollOffset = 0; // Scroll offset in weapon picker
static int g_skinScrollOffset = 0;   // Scroll offset in skin picker
static const char* g_weaponCategories[] = {"Pistols", "Rifles", "SMGs", "Heavy", "Knives", "Gloves"};
static int g_weaponCatFilter = -1;    // -1 = all, 0-5 = category index

// CS2 Menu — Configs tab state
static int g_configSelected = 0;
static char g_configName[64] = "";
static const char* g_configSlots[] = {"Config 1", "Config 2", "Config 3", "Config 4", "Config 5"};
static bool g_configSaved[5] = {false, false, false, false, false};

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

        // Need CS2 window AND at least 25 seconds (15s load + 10s buffer)
        if (windowReady && g_injTimer > 25.0f) {
            g_cs2Hwnd = ed.result;

            // Save original loader position
            GetWindowRect(g_hwnd, &g_origWindowRect);

            // Show transparent overlay centered on CS2 — no background,
            // only the AC logo, progress bar & text float on top of CS2
            int panelW = 400, panelH = 180;
            RECT cr; GetWindowRect(g_cs2Hwnd, &cr);
            int cs2w = cr.right - cr.left;
            int cs2h = cr.bottom - cr.top;
            int ovX = cr.left + (cs2w - panelW) / 2;
            int ovY = cr.top  + (cs2h - panelH) / 2;

            // Make window layered for color-key transparency
            // NOTE: Do NOT use WS_EX_TRANSPARENT — it prevents DX11 rendering
            LONG exStyle = GetWindowLongA(g_hwnd, GWL_EXSTYLE);
            SetWindowLongA(g_hwnd, GWL_EXSTYLE, (exStyle | WS_EX_LAYERED) & ~WS_EX_TRANSPARENT);
            SetLayeredWindowAttributes(g_hwnd, RGB(1, 0, 1), 0, LWA_COLORKEY);
            SetWindowRgn(g_hwnd, nullptr, TRUE); // Remove rounded region for overlay
            SetWindowPos(g_hwnd, HWND_TOPMOST, ovX, ovY, panelW, panelH, SWP_SHOWWINDOW);
            g_width = panelW; g_height = panelH;
            g_backend.Resize((u32)panelW, (u32)panelH);
            SetForegroundWindow(g_hwnd);
            Log("Overlay: transparent %dx%d centered on CS2", panelW, panelH);

            g_hasInjected = true;
            g_injPhase = INJ_OVERLAY;
            g_injTimer = 0;
            g_injProgress = 0;
        } else if (g_injTimer > 90.0f) {
            // Timeout waiting for CS2 window
            g_toastMsg = "Timeout: CS2 hovedmenu ikke fundet"; g_toastCol = P::Red; g_toastTimer = 4;
            g_injecting = false;
            g_injPhase = INJ_IDLE;
        }
        break;
    }

    case INJ_OVERLAY:
        // Beep once when overlay starts
        if (g_injProgress == 0.0f && g_injTimer < 0.1f) {
            Beep(800, 200);
        }
        // Transparent overlay: AC logo + progress bar floating on CS2
        // Animate progress 0% → 100% over ~4 seconds, then inject
        g_injProgress += g_dt * 25.0f; // ~4s to 100%
        if (g_injProgress >= 100.0f) {
            g_injProgress = 100.0f;
            g_injPhase = INJ_INJECTING;
            g_injTimer = 0;
        }
        break;

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
            g_toastMsg = "Injected successfully!"; g_toastCol = P::Green; g_injected = true;
            Log("Injection successful into PID %lu", pid);
            Beep(1200, 150); // Success beep
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

        // Always transition to CS2 menu (788x638, starts hidden — toggle with Insert)
        {
            int menuW = 788, menuH = 638;
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
        g_cs2MenuVisible = false;
        ShowWindow(g_hwnd, SW_HIDE); // Start hidden — press Insert to show
        g_injPhase = INJ_CS2_MENU;
        g_showPopup = false;
        g_injecting = false;
        break;
    }

    default: break;
    }
}

// --- Draw injection overlay ---
// No background — WS_EX_LAYERED + LWA_COLORKEY makes magenta(1,0,1) transparent
// Only the logo, text, and progress bar are visible, floating on CS2
static void DrawInjectionOverlay(DrawList& dl, f32 W, f32 H) {
    if (g_injPhase == INJ_OVERLAY || g_injPhase == INJ_INJECTING) {
        // NO background — clear color (1,0,1) is the transparency key

        // AC logo centered — fully transparent background, no backdrop
        // Logo PNG has hard alpha edges (no semi-transparent pixels)
        TextureHandle overlayLogo = (g_acGlowLogo != INVALID_TEXTURE) ? g_acGlowLogo : g_cs2Logo;
        if (overlayLogo != INVALID_TEXTURE) {
            f32 logoSz = 80;
            f32 lx = (W - logoSz) * 0.5f;
            f32 ly = 16;
            dl.AddTexturedRect(Rect{lx, ly, logoSz, logoSz},
                               overlayLogo, Color{255,255,255,255});
        }

        // Progress text — flat, clean, no shadow/outline
        char progText[64];
        snprintf(progText, 64, "Injecterer Inventory Changer (%.0f%%)", g_injProgress);
        Vec2 pts = Measure(progText, g_fontSm);
        f32 ptX = (W - pts.x) * 0.5f;
        f32 ptY = 108;
        Text(dl, ptX, ptY, Color{220, 225, 245, 255}, progText, g_fontSm);

        // Progress bar — dark-to-light blue gradient matching the AC logo
        f32 barW = W * 0.6f;
        f32 barH = 6;
        f32 barX = (W - barW) * 0.5f;
        f32 barY = ptY + 22;
        f32 fillW = barW * (g_injProgress / 100.0f);
        if (fillW > 1.0f) {
            // Gradient: dark navy → medium blue matching logo colors
            Color darkNavy{13, 43, 107, 255};
            Color medBlue{21, 101, 192, 255};
            Color c1 = Mix(darkNavy, medBlue, g_injProgress / 100.0f);
            dl.AddFilledRoundRect(Rect{barX, barY, fillW, barH}, c1, 3.0f, 8);
        }
    }
}

// Forward declaration for DrawToast (used in DrawCS2Menu)
static void DrawToast(DrawList& dl, f32 W, f32 H);

// ============================================================================
// ICON DRAWING — Paintbrush and Server icons matching reference SVGs
// ============================================================================

// Paintbrush icon — diagonal brush stroke matching reference image
static void DrawPaintbrushIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.5f;
    // Brush handle (thick diagonal line, bottom-right to center)
    dl.AddLine(Vec2{cx + h * 0.6f, cy + h * 0.65f},
               Vec2{cx + h * 0.05f, cy + h * 0.0f}, c, 3.0f);
    // Bristle shaft (thinner, center to top-left)
    dl.AddLine(Vec2{cx + h * 0.05f, cy + h * 0.0f},
               Vec2{cx - h * 0.55f, cy - h * 0.6f}, c, 2.2f);
    // Brush tip (pointed, top-left)
    dl.AddLine(Vec2{cx - h * 0.55f, cy - h * 0.6f},
               Vec2{cx - h * 0.7f, cy - h * 0.75f}, c, 1.5f);
    // Ferrule (metal band between handle and bristles)
    dl.AddLine(Vec2{cx + h * 0.15f, cy + h * 0.1f},
               Vec2{cx - h * 0.05f, cy - h * 0.1f}, c, 4.0f);
    // Brush head curve (left side arc)
    dl.AddLine(Vec2{cx - h * 0.1f, cy - h * 0.15f},
               Vec2{cx - h * 0.45f, cy - h * 0.5f}, c, 2.8f);
    // Brush head curve (right edge)
    dl.AddLine(Vec2{cx + h * 0.0f, cy - h * 0.1f},
               Vec2{cx - h * 0.35f, cy - h * 0.55f}, c, 1.8f);
    // Tip detail
    dl.AddTriangle(
        Vec2{cx - h * 0.55f, cy - h * 0.55f},
        Vec2{cx - h * 0.7f, cy - h * 0.75f},
        Vec2{cx - h * 0.4f, cy - h * 0.65f}, c);
}

// Server/stack icon — 3 stacked boxes with circles and lines matching reference
static void DrawServerStackIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.5f;
    f32 bw = h * 0.9f;   // half-width of each box
    f32 bh = h * 0.26f;  // half-height of each box
    f32 gap = 2.0f;
    f32 cr = 3.0f;        // corner radius

    for (int i = 0; i < 3; i++) {
        f32 by = cy - h * 0.6f + i * (bh * 2.0f + gap);
        Rect box{cx - bw, by, bw * 2.0f, bh * 2.0f};

        // Box fill (subtle)
        dl.AddFilledRoundRect(box, Color{c.r, c.g, c.b, (u8)(c.a / 6)}, cr, 4);
        // Box border
        dl.AddLine(Vec2{box.x + cr, box.y}, Vec2{box.x + box.w - cr, box.y}, c, 1.5f);
        dl.AddLine(Vec2{box.x + box.w, box.y + cr}, Vec2{box.x + box.w, box.y + box.h - cr}, c, 1.5f);
        dl.AddLine(Vec2{box.x + box.w - cr, box.y + box.h}, Vec2{box.x + cr, box.y + box.h}, c, 1.5f);
        dl.AddLine(Vec2{box.x, box.y + box.h - cr}, Vec2{box.x, box.y + cr}, c, 1.5f);

        // LED circle (left side)
        f32 ledX = box.x + bw * 0.35f;
        f32 ledY = by + bh;
        dl.AddCircle(Vec2{ledX, ledY}, 3.0f, c, 10);
        // Inner LED dot
        dl.AddCircle(Vec2{ledX, ledY}, 1.5f, c, 8);

        // Activity line (right side, horizontal bar)
        f32 lineX1 = cx + bw * 0.15f;
        f32 lineX2 = cx + bw * 0.65f;
        dl.AddLine(Vec2{lineX1, ledY}, Vec2{lineX2, ledY}, c, 1.8f);
    }
}

// Plus (+) icon for empty grid slots
static void DrawPlusIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.35f;
    dl.AddLine(Vec2{cx - h, cy}, Vec2{cx + h, cy}, c, 2.0f);
    dl.AddLine(Vec2{cx, cy - h}, Vec2{cx, cy + h}, c, 2.0f);
}

// Profile/User icon (circle with head silhouette)
static void DrawProfileIcon(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.5f;
    // Head circle
    dl.AddCircle(Vec2{cx, cy - h * 0.25f}, h * 0.32f, c, 12);
    // Shoulders arc (bezier-ish with lines)
    dl.AddLine(Vec2{cx - h * 0.55f, cy + h * 0.55f}, Vec2{cx - h * 0.3f, cy + h * 0.2f}, c, 1.5f);
    dl.AddLine(Vec2{cx - h * 0.3f, cy + h * 0.2f}, Vec2{cx, cy + h * 0.12f}, c, 1.5f);
    dl.AddLine(Vec2{cx, cy + h * 0.12f}, Vec2{cx + h * 0.3f, cy + h * 0.2f}, c, 1.5f);
    dl.AddLine(Vec2{cx + h * 0.3f, cy + h * 0.2f}, Vec2{cx + h * 0.55f, cy + h * 0.55f}, c, 1.5f);
}

// Back arrow icon
static void DrawBackArrow(DrawList& dl, f32 cx, f32 cy, f32 sz, Color c) {
    f32 h = sz * 0.35f;
    dl.AddLine(Vec2{cx + h, cy - h * 0.6f}, Vec2{cx - h * 0.3f, cy}, c, 2.0f);
    dl.AddLine(Vec2{cx - h * 0.3f, cy}, Vec2{cx + h, cy + h * 0.6f}, c, 2.0f);
    dl.AddLine(Vec2{cx - h * 0.3f, cy}, Vec2{cx + h * 1.2f, cy}, c, 2.0f);
}

// ============================================================================
// CS2 IN-GAME MENU — redesigned layout
// Top: AC logo center, Skins+Configs tabs left, Profile right
// Content: Grid system with weapon/skin selection
// ============================================================================
static void DrawCS2Menu(DrawList& dl, f32 W, f32 H) {
    f32 a = g_menuOpenAnim;
    if (a < 0.001f) return;
    u8 ga = (u8)(a * 255.0f);

    f32 ease = 1.0f - (1.0f - a) * (1.0f - a);
    f32 sc = 0.8f + 0.2f * ease;

    f32 sW = W * sc, sH = H * sc;
    f32 ox = (W - sW) * 0.5f;
    f32 oy = (H - sH) * 0.5f;

    // Dark gradient background
    dl.AddGradientRect(Rect{ox, oy, sW, sH},
        Color{18, 14, 32, ga}, Color{18, 14, 32, ga},
        Color{12, 10, 28, ga}, Color{12, 10, 28, ga});

    // Subtle animated orbs
    f32 t = g_time;
    auto drawOrb = [&](f32 cx, f32 cy, Color base, f32 maxR) {
        for (int i = 4; i >= 1; i--) {
            f32 r = maxR * (f32)i / 2.5f;
            u8 ca = (u8)((f32)(2 * (5 - i)) * a);
            dl.AddFilledRoundRect(Rect{cx - r, cy - r, r * 2, r * 2},
                                  Color{base.r, base.g, base.b, ca}, r, 20);
        }
    };
    drawOrb(ox + sW * 0.2f + sinf(t*0.4f)*sW*0.04f, oy + sH*0.5f, Color{50,100,255,255}, 60*sc);
    drawOrb(ox + sW * 0.8f + cosf(t*0.3f)*sW*0.04f, oy + sH*0.6f, Color{80,150,255,255}, 50*sc);

    // ===== TOP BAR =====
    f32 topH = 48.0f * sc;
    f32 topY = oy;

    // Top bar background
    dl.AddGradientRect(Rect{ox, topY, sW, topH},
        Color{24, 20, 42, ga}, Color{24, 20, 42, ga},
        Color{20, 16, 36, ga}, Color{20, 16, 36, ga});

    // Bottom separator
    dl.AddGradientRect(Rect{ox, topY + topH - 1, sW, 1},
        Color{50, 80, 200, (u8)(20*a)}, Color{80, 120, 255, (u8)(40*a)},
        Color{80, 120, 255, (u8)(40*a)}, Color{50, 80, 200, (u8)(20*a)});

    // --- LEFT SIDE: Skins + Configs tabs ---
    struct TabDef { const char* label; int id; };
    TabDef tabs[] = { {"Skins", TAB_SKINS}, {"Configs", TAB_CONFIGS} };

    f32 tabX = ox + 14.0f * sc;
    f32 tabW = 100.0f * sc;
    f32 tabGap = 4.0f * sc;
    f32 tabH = 32.0f * sc;
    f32 tabY = topY + (topH - tabH) * 0.5f;

    for (int i = 0; i < 2; i++) {
        f32 tx = tabX + i * (tabW + tabGap);
        bool active = (g_cs2Tab == tabs[i].id);
        u32 tid = Hash(tabs[i].label) + 9000;
        bool hov = Hit(tx, tabY, tabW, tabH);
        f32 ta = Anim(tid, active ? 1.0f : (hov ? 0.4f : 0.0f));

        // Tab background
        if (ta > 0.01f) {
            Color bg{55, 90, 220, (u8)(22 * ta * a)};
            dl.AddFilledRoundRect(Rect{tx, tabY, tabW, tabH}, bg, 6*sc, 8);
        }

        // Active indicator
        if (active) {
            f32 indW = tabW * 0.5f;
            dl.AddFilledRoundRect(Rect{tx + (tabW-indW)*0.5f, topY + topH - 3*sc, indW, 2.5f*sc},
                                  Color{80, 140, 255, ga}, 1, 4);
        }

        // Icon
        f32 iconSz = 15.0f * sc;
        f32 iconCX = tx + 18.0f * sc;
        f32 iconCY = tabY + tabH * 0.5f;
        Color ic = active ? Color{120, 170, 255, ga}
                          : Color{130, 135, 160, (u8)(ga * (0.55f + 0.45f*ta))};

        if (tabs[i].id == TAB_SKINS)   DrawPaintbrushIcon(dl, iconCX, iconCY, iconSz, ic);
        else                           DrawServerStackIcon(dl, iconCX, iconCY, iconSz, ic);

        // Label
        Color lc = active ? Color{215, 225, 255, ga}
                          : Color{130, 135, 160, (u8)(ga * (0.55f + 0.45f*ta))};
        Vec2 ls = Measure(tabs[i].label, g_font);
        Text(dl, tx + 34*sc, tabY + (tabH - ls.y)*0.5f, lc, tabs[i].label, g_font);

        if (hov && g_input.IsMousePressed(MouseButton::Left))
            g_cs2Tab = tabs[i].id;
    }

    // --- CENTER: AC Logo (slightly larger) ---
    TextureHandle logo = (g_acGlowLogo != INVALID_TEXTURE) ? g_acGlowLogo : g_cs2Logo;
    if (logo != INVALID_TEXTURE) {
        f32 logoSz = 36.0f * sc;
        f32 logoX = ox + (sW - logoSz) * 0.5f;
        f32 logoY = topY + (topH - logoSz) * 0.5f;
        dl.AddTexturedRect(Rect{logoX, logoY, logoSz, logoSz}, logo, Color{255,255,255,ga});
    }

    // --- RIGHT SIDE: Profile ---
    {
        f32 profX = ox + sW - 140*sc;
        f32 profY = topY + (topH - 28*sc) * 0.5f;
        f32 profW = 126*sc;
        f32 profH = 28*sc;

        // Profile background pill
        u32 profId = Hash("_profile_pill");
        bool profHov = Hit(profX, profY, profW, profH);
        f32 profA = Anim(profId, profHov ? 0.5f : 0.0f);
        dl.AddFilledRoundRect(Rect{profX, profY, profW, profH},
                              Color{255,255,255, (u8)(5*a + 8*profA*a)}, 14*sc, 10);

        // Profile icon
        DrawProfileIcon(dl, profX + 16*sc, profY + profH*0.5f, 16*sc,
                         Color{140, 165, 220, ga});

        // Username (from login)
        const char* uname = (strlen(g_loginUser) > 0) ? g_loginUser : "User";
        Vec2 us = Measure(uname, g_fontSm);
        f32 maxTW = profW - 36*sc;
        // Truncate display if too long
        char truncName[20];
        strncpy(truncName, uname, 16); truncName[16] = '\0';
        if (us.x > maxTW && strlen(uname) > 12) {
            strncpy(truncName, uname, 12); truncName[12] = '.'; truncName[13] = '.'; truncName[14] = '\0';
        }
        Vec2 ts = Measure(truncName, g_fontSm);
        Text(dl, profX + 30*sc, profY + (profH - ts.y)*0.5f,
             Color{180, 190, 220, ga}, truncName, g_fontSm);
    }

    // ===== CONTENT AREA =====
    f32 cY = topY + topH + 6*sc;
    f32 cH = sH - topH - 12*sc;
    f32 cX = ox + 10*sc;
    f32 cW = sW - 20*sc;

    // ===========================================================
    // SKINS TAB
    // ===========================================================
    if (g_cs2Tab == TAB_SKINS) {

        if (g_skinView == SKIN_VIEW_GRID) {
            // ===== WEAPON GRID — 4 columns x 3 rows of slots =====
            int cols = 4, rows = 3;
            f32 pad = 8*sc;
            f32 cellW = (cW - (cols-1)*pad) / cols;
            f32 cellH = (cH - (rows-1)*pad - 4*sc) / rows;

            for (int row = 0; row < rows; row++) {
                for (int col = 0; col < cols; col++) {
                    int idx = row * cols + col;
                    f32 cx2 = cX + col*(cellW+pad);
                    f32 cy2 = cY + row*(cellH+pad) + 2*sc;

                    int wpnIdx = g_gridSlots[idx];
                    int skinIdx = g_gridSkinChoice[idx];

                    u32 cellId = Hash("grid") + idx * 31;
                    bool cellHov = Hit(cx2, cy2, cellW, cellH);
                    f32 cellA = Anim(cellId, cellHov ? 1.0f : 0.0f);

                    // Cell background
                    Color cellBg{30, 26, 50, ga};
                    if (wpnIdx >= 0 && skinIdx >= 0) {
                        // Has a skin applied — show rarity tint
                        Color rc = RarityColor(g_weaponDB[wpnIdx].skins[skinIdx].rarity);
                        cellBg = Color{(u8)(rc.r/5), (u8)(rc.g/5), (u8)(rc.b/5), ga};
                    }
                    dl.AddFilledRoundRect(Rect{cx2, cy2, cellW, cellH}, cellBg, 8*sc, 10);

                    // Hover glow
                    if (cellA > 0.01f) {
                        dl.AddFilledRoundRect(Rect{cx2, cy2, cellW, cellH},
                            Color{80,130,255,(u8)(12*cellA*a)}, 8*sc, 10);
                    }

                    // Border
                    Color borderC{50, 48, 72, ga};
                    if (wpnIdx >= 0 && skinIdx >= 0) {
                        Color rc2 = RarityColor(g_weaponDB[wpnIdx].skins[skinIdx].rarity);
                        borderC = Color{rc2.r, rc2.g, rc2.b, (u8)(ga * 0.4f)};
                    }
                    dl.AddFilledRoundRect(Rect{cx2-1, cy2-1, cellW+2, cellH+2}, borderC, 9*sc, 10);
                    dl.AddFilledRoundRect(Rect{cx2, cy2, cellW, cellH}, cellBg, 8*sc, 10);

                    if (wpnIdx < 0) {
                        // Empty slot — draw "+"
                        Color plusC{100, 105, 140, (u8)(ga * (0.4f + 0.4f*cellA))};
                        DrawPlusIcon(dl, cx2 + cellW*0.5f, cy2 + cellH*0.5f, 20*sc, plusC);
                    } else {
                        // Filled slot — weapon name + skin name
                        const auto& wpn = g_weaponDB[wpnIdx];

                        // Weapon name (top)
                        Vec2 wns = Measure(wpn.name, g_fontSm);
                        Text(dl, cx2 + (cellW-wns.x)*0.5f, cy2 + 6*sc,
                             Color{200, 210, 240, ga}, wpn.name, g_fontSm);

                        if (skinIdx >= 0 && skinIdx < wpn.skinCount) {
                            const auto& sk = wpn.skins[skinIdx];

                            // Rarity bar at bottom
                            Color rc3 = RarityColor(sk.rarity);
                            dl.AddFilledRoundRect(
                                Rect{cx2 + 4*sc, cy2 + cellH - 6*sc, cellW - 8*sc, 3*sc},
                                Color{rc3.r, rc3.g, rc3.b, (u8)(ga*0.7f)}, 1.5f, 4);

                            // Skin name (centered)
                            Vec2 sns = Measure(sk.name, g_font);
                            Text(dl, cx2 + (cellW-sns.x)*0.5f, cy2 + cellH*0.4f,
                                 Color{rc3.r, rc3.g, rc3.b, ga}, sk.name, g_font);

                            // Rarity text (small)
                            const char* rn = RarityName(sk.rarity);
                            Vec2 rns = Measure(rn, g_fontSm);
                            Text(dl, cx2 + (cellW-rns.x)*0.5f, cy2 + cellH*0.65f,
                                 Color{rc3.r, rc3.g, rc3.b, (u8)(ga*0.6f)}, rn, g_fontSm);
                        } else {
                            // Default skin
                            Vec2 ds = Measure("Default", g_font);
                            Text(dl, cx2 + (cellW-ds.x)*0.5f, cy2 + cellH*0.4f,
                                 Color{120,125,150,ga}, "Default", g_font);
                        }
                    }

                    // Click → open weapon picker
                    if (cellHov && g_input.IsMousePressed(MouseButton::Left)) {
                        g_activeSlot = idx;
                        g_skinView = SKIN_VIEW_WEAPONS;
                        g_weaponScrollOffset = 0;
                        g_weaponCatFilter = -1;
                    }
                }
            }

        } else if (g_skinView == SKIN_VIEW_WEAPONS) {
            // ===== WEAPON PICKER =====
            // Back button
            {
                f32 bx = cX, by2 = cY + 2*sc, bw = 70*sc, bh = 26*sc;
                u32 bid = Hash("_wpn_back");
                bool bh2 = Hit(bx, by2, bw, bh);
                f32 ba = Anim(bid, bh2 ? 1.0f : 0.0f);
                dl.AddFilledRoundRect(Rect{bx, by2, bw, bh},
                    Color{255,255,255,(u8)(5*a + 10*ba*a)}, 4*sc, 6);
                DrawBackArrow(dl, bx + 14*sc, by2 + bh*0.5f, 12*sc,
                              Color{160,170,210,(u8)(ga*(0.6f+0.4f*ba))});
                Vec2 bs = Measure("Back", g_fontSm);
                Text(dl, bx + 28*sc, by2 + (bh-bs.y)*0.5f,
                     Color{160,170,210,(u8)(ga*(0.6f+0.4f*ba))}, "Back", g_fontSm);
                if (bh2 && g_input.IsMousePressed(MouseButton::Left)) {
                    g_skinView = SKIN_VIEW_GRID;
                }
            }

            // Title
            Vec2 ts = Measure("Select Weapon", g_fontMd);
            Text(dl, cX + (cW-ts.x)*0.5f, cY + 4*sc, Color{200,210,240,ga}, "Select Weapon", g_fontMd);

            // Category filter pills
            f32 pillY = cY + 28*sc;
            f32 pillH = 22*sc;
            f32 px = cX + 4*sc;
            // "All" pill
            {
                const char* lbl = "All";
                Vec2 ps = Measure(lbl, g_fontSm);
                f32 pw = ps.x + 14*sc;
                bool isAct = (g_weaponCatFilter == -1);
                u32 pid = Hash("cat_all") + 4000;
                bool ph = Hit(px, pillY, pw, pillH);
                f32 pa = Anim(pid, isAct ? 1.0f : (ph ? 0.4f : 0.0f));
                dl.AddFilledRoundRect(Rect{px, pillY, pw, pillH},
                    Color{60,100,255,(u8)(isAct ? 35*a : 8*pa*a)}, 4*sc, 6);
                Text(dl, px + (pw-ps.x)*0.5f, pillY + (pillH-ps.y)*0.5f,
                     Color{(u8)(isAct?180:130), (u8)(isAct?210:135), (u8)(isAct?255:160), ga},
                     lbl, g_fontSm);
                if (ph && g_input.IsMousePressed(MouseButton::Left))
                    g_weaponCatFilter = -1;
                px += pw + 3*sc;
            }
            for (int ci = 0; ci < 6; ci++) {
                const char* lbl = g_weaponCategories[ci];
                Vec2 ps = Measure(lbl, g_fontSm);
                f32 pw = ps.x + 14*sc;
                bool isAct = (g_weaponCatFilter == ci);
                u32 pid = Hash(lbl) + 4100;
                bool ph = Hit(px, pillY, pw, pillH);
                f32 pa = Anim(pid, isAct ? 1.0f : (ph ? 0.4f : 0.0f));
                dl.AddFilledRoundRect(Rect{px, pillY, pw, pillH},
                    Color{60,100,255,(u8)(isAct ? 35*a : 8*pa*a)}, 4*sc, 6);
                Text(dl, px + (pw-ps.x)*0.5f, pillY + (pillH-ps.y)*0.5f,
                     Color{(u8)(isAct?180:130), (u8)(isAct?210:135), (u8)(isAct?255:160), ga},
                     lbl, g_fontSm);
                if (ph && g_input.IsMousePressed(MouseButton::Left))
                    g_weaponCatFilter = ci;
                px += pw + 3*sc;
            }

            // Weapon list (scrollable)
            f32 listY = pillY + pillH + 8*sc;
            f32 listH = cY + cH - listY - 4*sc;
            f32 itemH = 34*sc;
            f32 itemPad = 3*sc;

            int drawn = 0;
            for (int wi = 0; wi < g_weaponDBCount; wi++) {
                const auto& w = g_weaponDB[wi];
                // Category filter
                if (g_weaponCatFilter >= 0) {
                    const char* cat = g_weaponCategories[g_weaponCatFilter];
                    // Compare category strings
                    bool match = false;
                    for (int k = 0; cat[k] && w.category[k]; k++) {
                        if (cat[k] != w.category[k]) { match = false; break; }
                        if (cat[k+1] == '\0' && w.category[k+1] == '\0') match = true;
                    }
                    if (cat[0] == w.category[0] && cat[1] == w.category[1] && cat[2] == w.category[2])
                        match = true; // Quick prefix match for 3+ chars
                    else match = false;
                    // Proper comparison
                    bool eq = true;
                    for (int k = 0; ; k++) {
                        if (cat[k] != w.category[k]) { eq = false; break; }
                        if (cat[k] == '\0') break;
                    }
                    if (!eq) continue;
                }

                int visualIdx = drawn - g_weaponScrollOffset;
                drawn++;
                if (visualIdx < 0) continue;

                f32 iy = listY + visualIdx * (itemH + itemPad);
                if (iy + itemH > cY + cH) break;

                u32 wid = Hash(w.name) + 6000;
                bool wh = Hit(cX, iy, cW, itemH);
                f32 wa = Anim(wid, wh ? 0.6f : 0.0f);

                dl.AddFilledRoundRect(Rect{cX, iy, cW, itemH},
                    Color{255,255,255, (u8)(4*wa*a)}, 6*sc, 8);

                // Category badge
                Vec2 cs2 = Measure(w.category, g_fontSm);
                f32 badgeW = cs2.x + 10*sc;
                dl.AddFilledRoundRect(Rect{cX + 8*sc, iy + (itemH-18*sc)*0.5f, badgeW, 18*sc},
                    Color{60,80,180,(u8)(20*a)}, 3*sc, 6);
                Text(dl, cX + 8*sc + 5*sc, iy + (itemH-cs2.y)*0.5f,
                     Color{120,140,200,ga}, w.category, g_fontSm);

                // Weapon name
                Vec2 wns = Measure(w.name, g_font);
                Text(dl, cX + badgeW + 20*sc, iy + (itemH-wns.y)*0.5f,
                     Color{(u8)(180+40*wa), (u8)(190+30*wa), (u8)(230+20*wa), ga}, w.name, g_font);

                // Skin count
                char countBuf[16];
                snprintf(countBuf, 16, "%d skins", w.skinCount);
                Vec2 cnts = Measure(countBuf, g_fontSm);
                Text(dl, cX + cW - cnts.x - 10*sc, iy + (itemH-cnts.y)*0.5f,
                     Color{110,115,140,ga}, countBuf, g_fontSm);

                if (wh && g_input.IsMousePressed(MouseButton::Left)) {
                    g_selectedWeapon = wi;
                    g_selectedSkin = -1;
                    g_skinScrollOffset = 0;
                    g_skinView = SKIN_VIEW_SKINS;
                }
            }

            // Scroll with mouse wheel
            f32 wd = g_input.ScrollDelta();
            if (wd != 0.0f && Hit(cX, listY, cW, listH)) {
                g_weaponScrollOffset -= (int)wd;
                if (g_weaponScrollOffset < 0) g_weaponScrollOffset = 0;
                int maxScroll = drawn - (int)(listH / (itemH+itemPad));
                if (maxScroll < 0) maxScroll = 0;
                if (g_weaponScrollOffset > maxScroll) g_weaponScrollOffset = maxScroll;
            }

        } else if (g_skinView == SKIN_VIEW_SKINS && g_selectedWeapon >= 0) {
            // ===== SKIN PICKER FOR SELECTED WEAPON =====
            const auto& wpn = g_weaponDB[g_selectedWeapon];

            // Back button
            {
                f32 bx = cX, by2 = cY + 2*sc, bw = 70*sc, bh = 26*sc;
                u32 bid = Hash("_skin_back");
                bool bh2 = Hit(bx, by2, bw, bh);
                f32 ba = Anim(bid, bh2 ? 1.0f : 0.0f);
                dl.AddFilledRoundRect(Rect{bx, by2, bw, bh},
                    Color{255,255,255,(u8)(5*a + 10*ba*a)}, 4*sc, 6);
                DrawBackArrow(dl, bx + 14*sc, by2 + bh*0.5f, 12*sc,
                              Color{160,170,210,(u8)(ga*(0.6f+0.4f*ba))});
                Vec2 bs = Measure("Back", g_fontSm);
                Text(dl, bx + 28*sc, by2 + (bh-bs.y)*0.5f,
                     Color{160,170,210,(u8)(ga*(0.6f+0.4f*ba))}, "Back", g_fontSm);
                if (bh2 && g_input.IsMousePressed(MouseButton::Left)) {
                    g_skinView = SKIN_VIEW_WEAPONS;
                }
            }

            // Title: weapon name
            Vec2 wts = Measure(wpn.name, g_fontMd);
            Text(dl, cX + (cW-wts.x)*0.5f, cY + 4*sc, Color{200,210,240,ga}, wpn.name, g_fontMd);

            // Skin list
            f32 listY = cY + 32*sc;
            f32 listH = cY + cH - listY - 44*sc; // leave room for Add button
            f32 itemH = 42*sc;
            f32 itemPad = 3*sc;

            for (int si = 0; si < wpn.skinCount; si++) {
                int vi = si - g_skinScrollOffset;
                if (vi < 0) continue;

                f32 iy = listY + vi * (itemH + itemPad);
                if (iy + itemH > listY + listH) break;

                const auto& sk = wpn.skins[si];
                bool isSel = (g_selectedSkin == si);
                Color rc = RarityColor(sk.rarity);

                u32 sid = Hash(sk.name) + 3000 + g_selectedWeapon * 100;
                bool sh = Hit(cX, iy, cW, itemH);
                f32 sa = Anim(sid, isSel ? 1.0f : (sh ? 0.5f : 0.0f));

                // Background
                Color ibg = isSel ? Color{rc.r, rc.g, rc.b, (u8)(25*a)}
                                  : Color{255,255,255, (u8)(4*sa*a)};
                dl.AddFilledRoundRect(Rect{cX, iy, cW, itemH}, ibg, 6*sc, 8);

                // Left rarity bar
                dl.AddFilledRoundRect(Rect{cX, iy + 3*sc, 3*sc, itemH - 6*sc},
                    Color{rc.r, rc.g, rc.b, (u8)(ga * (isSel ? 0.8f : 0.3f))}, 1.5f, 4);

                // Skin name
                Vec2 sns = Measure(sk.name, g_font);
                Text(dl, cX + 14*sc, iy + 4*sc,
                     Color{(u8)(180+40*sa), (u8)(190+30*sa), (u8)(230+20*sa), ga},
                     sk.name, g_font);

                // Rarity text + paint kit
                char rarBuf[32];
                snprintf(rarBuf, 32, "%s", RarityName(sk.rarity));
                Text(dl, cX + 14*sc, iy + 22*sc,
                     Color{rc.r, rc.g, rc.b, (u8)(ga*0.7f)}, rarBuf, g_fontSm);

                // Paint kit ID (right side)
                char pkBuf[16];
                snprintf(pkBuf, 16, "#%d", sk.paintKit);
                Vec2 pks = Measure(pkBuf, g_fontSm);
                Text(dl, cX + cW - pks.x - 10*sc, iy + (itemH-pks.y)*0.5f,
                     Color{100,105,130,ga}, pkBuf, g_fontSm);

                // Rarity dot
                dl.AddCircle(Vec2{cX + cW - pks.x - 22*sc, iy + itemH*0.5f}, 4*sc,
                             Color{rc.r, rc.g, rc.b, ga}, 10);

                // Selection
                if (sh && g_input.IsMousePressed(MouseButton::Left))
                    g_selectedSkin = si;
            }

            // Scroll
            f32 sd = g_input.ScrollDelta();
            if (sd != 0.0f && Hit(cX, listY, cW, listH)) {
                g_skinScrollOffset -= (int)sd;
                if (g_skinScrollOffset < 0) g_skinScrollOffset = 0;
                int maxS = wpn.skinCount - (int)(listH / (itemH+itemPad));
                if (maxS < 0) maxS = 0;
                if (g_skinScrollOffset > maxS) g_skinScrollOffset = maxS;
            }

            // ===== ADD TO INVENTORY BUTTON =====
            {
                f32 btnW = 160*sc;
                f32 btnH = 34*sc;
                f32 btnX = cX + (cW - btnW) * 0.5f;
                f32 btnY = cY + cH - btnH - 4*sc;
                bool canAdd = (g_selectedSkin >= 0);

                u32 addId = Hash("_add_inv");
                bool addHov = canAdd && Hit(btnX, btnY, btnW, btnH);
                f32 addA = Anim(addId, addHov ? 1.0f : 0.0f);

                // Glow
                if (addA > 0.01f && canAdd) {
                    for (int gi = 3; gi >= 1; gi--) {
                        f32 sp = (f32)gi * 2.0f;
                        dl.AddFilledRoundRect(Rect{btnX-sp, btnY-sp, btnW+sp*2, btnH+sp*2},
                            Color{60,180,120,(u8)(6*addA*a*(4-gi))}, 10+sp, 10);
                    }
                }

                Color btnBg = canAdd ? Color{40, 150, 90, (u8)((50 + 30*addA) * a)}
                                     : Color{40, 42, 55, (u8)(20*a)};
                dl.AddFilledRoundRect(Rect{btnX, btnY, btnW, btnH}, btnBg, 8*sc, 10);

                Color btnTc = canAdd ? Color{140, 240, 180, ga}
                                     : Color{90, 95, 120, ga};
                TextCenter(dl, Rect{btnX, btnY, btnW, btnH}, btnTc, "Add to Inventory", g_font);

                if (addHov && g_input.IsMousePressed(MouseButton::Left) && canAdd) {
                    // Apply skin to slot
                    if (g_activeSlot >= 0 && g_activeSlot < 12) {
                        g_gridSlots[g_activeSlot] = g_selectedWeapon;
                        g_gridSkinChoice[g_activeSlot] = g_selectedSkin;
                    }
                    // Return to grid
                    g_skinView = SKIN_VIEW_GRID;
                }
            }
        }

    // ===========================================================
    // CONFIGS TAB
    // ===========================================================
    } else if (g_cs2Tab == TAB_CONFIGS) {
        f32 listY = cY + 4*sc;
        f32 itemH = 40*sc;
        f32 itemPad = 4*sc;
        f32 listW = cW * 0.85f;
        f32 listX = cX + (cW - listW) * 0.5f;

        Vec2 hdrSz = Measure("Configuration Slots", g_fontMd);
        Text(dl, listX, listY, Color{200,210,240,ga}, "Configuration Slots", g_fontMd);
        listY += hdrSz.y + 10*sc;

        for (int i = 0; i < 5; i++) {
            f32 iy = listY + i * (itemH + itemPad);
            bool isSel = (g_configSelected == i);
            u32 cfgId = Hash(g_configSlots[i]) + 8000;
            bool cfgHov = Hit(listX, iy, listW, itemH);
            f32 cfgA = Anim(cfgId, isSel ? 1.0f : (cfgHov ? 0.5f : 0.0f));

            Color slotBg = isSel ? Color{60,100,255,(u8)(25*a)}
                                 : Color{255,255,255,(u8)(4*cfgA*a)};
            dl.AddFilledRoundRect(Rect{listX, iy, listW, itemH}, slotBg, 6*sc, 8);

            if (isSel) {
                dl.AddFilledRoundRect(Rect{listX, iy+4*sc, 3*sc, itemH-8*sc},
                    Color{80,140,255,ga}, 1.5f, 4);
            }

            Color slotNameC = isSel ? Color{220,230,255,ga}
                                    : Color{160,165,190,(u8)(ga*(0.65f+0.35f*cfgA))};
            Text(dl, listX+14*sc, iy+(itemH*0.5f-6*sc), slotNameC, g_configSlots[i], g_font);

            const char* status = g_configSaved[i] ? "Saved" : "Empty";
            Color stC = g_configSaved[i] ? Color{120,220,160,ga} : Color{120,125,150,ga};
            Vec2 stSz = Measure(status, g_fontSm);
            Text(dl, listX+listW-stSz.x-12*sc, iy+(itemH-stSz.y)*0.5f, stC, status, g_fontSm);

            if (cfgHov && g_input.IsMousePressed(MouseButton::Left))
                g_configSelected = i;
        }

        // Action buttons
        f32 btnY = listY + 5*(itemH+itemPad) + 10*sc;
        f32 btnW = 90*sc, btnH = 32*sc, btnPad = 8*sc;
        f32 btnRowW = btnW*3 + btnPad*2;
        f32 btnStartX = listX + (listW - btnRowW) * 0.5f;

        // Save
        {
            Rect r{btnStartX, btnY, btnW, btnH};
            u32 id = Hash("cfg_save");
            bool hov = Hit(r.x, r.y, r.w, r.h);
            f32 ba = Anim(id, hov ? 1.0f : 0.0f);
            dl.AddFilledRoundRect(r, Color{40,120,80,(u8)((30+25*ba)*a)}, 6, 8);
            TextCenter(dl, r, Color{140,230,170,ga}, "Save", g_font);
            if (hov && g_input.IsMousePressed(MouseButton::Left))
                g_configSaved[g_configSelected] = true;
        }
        // Load
        {
            Rect r{btnStartX+btnW+btnPad, btnY, btnW, btnH};
            u32 id = Hash("cfg_load");
            bool hov = Hit(r.x, r.y, r.w, r.h);
            f32 ba = Anim(id, hov ? 1.0f : 0.0f);
            bool can = g_configSaved[g_configSelected];
            dl.AddFilledRoundRect(r, can ? Color{40,80,160,(u8)((30+25*ba)*a)}
                                        : Color{40,42,55,(u8)(20*a)}, 6, 8);
            TextCenter(dl, r, can ? Color{140,180,255,ga} : Color{100,105,130,ga}, "Load", g_font);
        }
        // Delete
        {
            Rect r{btnStartX+2*(btnW+btnPad), btnY, btnW, btnH};
            u32 id = Hash("cfg_del");
            bool hov = Hit(r.x, r.y, r.w, r.h);
            f32 ba = Anim(id, hov ? 1.0f : 0.0f);
            bool can = g_configSaved[g_configSelected];
            dl.AddFilledRoundRect(r, can ? Color{160,50,50,(u8)((30+25*ba)*a)}
                                        : Color{40,42,55,(u8)(20*a)}, 6, 8);
            TextCenter(dl, r, can ? Color{255,140,140,ga} : Color{100,105,130,ga}, "Delete", g_font);
            if (hov && g_input.IsMousePressed(MouseButton::Left) && can)
                g_configSaved[g_configSelected] = false;
        }
    }
}

// ============================================================================
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
                              Color{79, 110, 255, a}, r, 24);
        dl.AddFilledRoundRect(Rect{orbX2 - r, orbY2 - r, r*2, r*2},
                              Color{160, 80, 255, a}, r, 24);
    }
}

// --- Title bar ---
static void DrawTitleBar(DrawList& dl, f32 W) {
    // Subtle top bar bg
    dl.AddGradientRect(Rect{0, 0, W, 48},
        Color{12, 12, 20, 220}, Color{12, 12, 20, 220},
        Color{12, 12, 20, 120}, Color{12, 12, 20, 120});

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
               Color{160, 165, 190, (u8)(150 + 105 * minA)}, 1.5f);
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
        Color{79, 110, 255, 0},  Color{79, 110, 255, 40},
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
        Color{35,35,55,0}, Color{35,35,55,0}, P::Border, P::Border);
    Text(dl, cx + cw * 0.42f + 8, divY - 6, P::T3, "or", g_fontSm);
    dl.AddGradientRect(Rect{cx + cw * 0.58f, divY, cw * 0.42f, 1},
        P::Border, P::Border, Color{35,35,55,0}, Color{35,35,55,0});

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
        Color xcc = Fade(Color{160, 165, 190, (u8)(140 + 115 * xbA)}, a);
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
    dl.AddFilledRect(Rect{0, 0, sideW, H}, Color{12, 12, 20, 240});
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
                              Color{45, 48, 65, 255}, avSz * 0.5f, 16);
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
                   Color{160, 165, 190, (u8)(130 + 125 * minA)}, 1.5f);
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
            {"Game Version", "Latest",      Color{100, 160, 255, 255}},
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

    // Injection overlay (on top of popup)
    DrawInjectionOverlay(dl, (f32)g_width, (f32)g_height);

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

    g_fontSm = TryFont(bodyFonts,  4, 13.0f);
    g_font   = TryFont(boldFonts,  4, 15.0f);
    g_fontMd = TryFont(boldFonts,  4, 18.0f);
    g_fontLg = TryFont(heavyFonts, 4, 26.0f);
    g_fontXl = TryFont(heavyFonts, 4, 34.0f);

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

        // Set clear color — magenta key during overlay (transparent), normal bg otherwise
        if (g_injPhase == INJ_OVERLAY || g_injPhase == INJ_INJECTING)
            g_backend.SetClearColor(Color{1, 0, 1, 0}); // Magenta = transparent via LWA_COLORKEY
        else
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
        } else if (g_injPhase == INJ_OVERLAY || g_injPhase == INJ_INJECTING) {
            // Transparent overlay phase — AC logo + progress bar floating on CS2
            DrawInjectionOverlay(dl, W, H);
            DrawToast(dl, W, H);
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
