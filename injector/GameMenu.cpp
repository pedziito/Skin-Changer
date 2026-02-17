// ============================================================================
//  CS2 In-Game Overlay  –  NEVERLOSE Style
//  INSERT toggles (only when CS2 is foreground window)
//  Grid skin browser  ·  float slider  ·  preset save/load
// ============================================================================
#include "GameMenu.h"
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>
#include <fstream>
#include <sstream>
#include <direct.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")

// ============================================================================
//  Debug logger
// ============================================================================
static void DbgLog(const char* fmt, ...) {
    FILE* f = nullptr;
    fopen_s(&f, "ac_debug.log", "a");
    if (!f) return;
    time_t now = time(nullptr);
    struct tm t; localtime_s(&t, &now);
    fprintf(f, "[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);
    va_list ap; va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f, "\n");
    fclose(f);
}

static OverlayWindow* g_pOverlay = nullptr;

// ============================================================================
//  Rarity color for a skin based on its index in the weapon's list
//  (simulates rarity tiers – first few = covert, etc.)
// ============================================================================
static COLORREF RarityColor(int idx) {
    if (idx < 2)  return NL_RARITY_COVERT;
    if (idx < 5)  return NL_RARITY_CLASSIFIED;
    if (idx < 10) return NL_RARITY_RESTRICTED;
    if (idx < 18) return NL_RARITY_MILSPEC;
    return NL_RARITY_INDUSTRIAL;
}

static const char* WearName(float f) {
    if (f <= 0.07f) return "Factory New";
    if (f <= 0.15f) return "Minimal Wear";
    if (f <= 0.38f) return "Field-Tested";
    if (f <= 0.45f) return "Well-Worn";
    return "Battle-Scarred";
}

// ============================================================================
//  GameMenu
// ============================================================================
GameMenu::GameMenu()
    : m_visible(false), m_page(PAGE_INVENTORY), m_view(INV_EQUIPPED)
    , m_scroll(0), m_mouseX(0), m_mouseY(0), m_menuX(0), m_menuY(0)
    , m_selWeapon(-1), m_selSkin(-1), m_detailFloat(0.01f)
    , m_gN(0), m_sliderDrag(false), m_remN(0), m_pN(0)
    , m_fTitle(0), m_fBody(0), m_fSmall(0), m_fBold(0)
    , m_fLogo(0), m_fGrid(0), m_fIcon(0) {
    memset(m_gR, 0, sizeof(m_gR));
    memset(m_sideR, 0, sizeof(m_sideR));
    memset(&m_addBtn, 0, sizeof(Rect));
    memset(&m_backBtn, 0, sizeof(Rect));
    memset(&m_applyBtn, 0, sizeof(Rect));
    memset(&m_sliderR, 0, sizeof(Rect));
    memset(&m_saveBtn, 0, sizeof(Rect));
    memset(&m_applyGameBtn, 0, sizeof(Rect));
}

GameMenu::~GameMenu() {
    HFONT* fonts[] = {&m_fTitle,&m_fBody,&m_fSmall,&m_fBold,&m_fLogo,&m_fGrid,&m_fIcon};
    for (auto* p : fonts) if (*p) { DeleteObject(*p); *p = nullptr; }
}

bool GameMenu::Initialize(HMODULE) {
    m_fTitle = CreateFontA(20,0,0,0,FW_BOLD,    0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
    m_fBody  = CreateFontA(14,0,0,0,FW_NORMAL,  0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
    m_fSmall = CreateFontA(12,0,0,0,FW_NORMAL,  0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
    m_fBold  = CreateFontA(14,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
    m_fLogo  = CreateFontA(18,0,0,0,FW_BOLD,    0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
    m_fGrid  = CreateFontA(12,0,0,0,FW_NORMAL,  0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI");
    m_fIcon  = CreateFontA(24,0,0,0,FW_NORMAL,  0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,"Segoe UI Emoji");
    BuildDatabase();
    ScanPresets();
    return true;
}

void GameMenu::Shutdown() { m_visible = false; }
void GameMenu::Toggle()   { m_visible = !m_visible; if (m_visible) m_scroll = 0; }

// ============================================================================
//  BuildDatabase  –  comprehensive CS2 weapons + paint-kit IDs
// ============================================================================
void GameMenu::BuildDatabase() {
    // ── Pistols ──
    m_weapons.push_back({"Glock-18", 4, "Pistols", {
        {"Fade",38},{"Gamma Doppler",568},{"Wasteland Rebel",511},{"Water Elemental",410},
        {"Twilight Galaxy",697},{"Bullet Queen",981},{"Vogue",978},{"Neo-Noir",769},
        {"Moonrise",837},{"Off World",850},{"Royal Legion",532},{"Grinder",241},
        {"Steel Disruption",843},{"Oxide Blaze",685},{"Sacrifice",906},{"Franklin",595},
        {"Synth Leaf",897},{"Snack Attack",872},{"Clear Polymer",855},{"Nuclear Garden",683},
        {"High Beam",665},{"Weasel",530},{"Sand Dune",208},{"Pink DDPAT",0}
    }});
    m_weapons.push_back({"USP-S", 61, "Pistols", {
        {"Kill Confirmed",504},{"Neo-Noir",769},{"Printstream",1017},{"Caiman",625},
        {"Cortex",839},{"Monster Mashup",868},{"Blueprint",826},{"Orion",313},
        {"The Traitor",729},{"Check Engine",816},{"Flashback",599},{"Serum",506},
        {"Cyrex",497},{"Guardian",290},{"Stainless",295},{"Ticket to Hell",0},
        {"Whiteout",0},{"Road Rash",653},{"Dark Water",187},{"Overgrowth",494},
        {"Night Ops",236},{"Target Acquired",0},{"Purple DDPAT",0},{"Pathfinder",0},
        {"Ancient Visions",0},{"Black Lotus",0}
    }});
    m_weapons.push_back({"P250", 36, "Pistols", {
        {"Asiimov",551},{"See Ya Later",503},{"Vino Primo",518},{"Nevermore",538},
        {"Undertow",187},{"Muertos",496},{"Mehndi",414},{"Supernova",483},
        {"Nuclear Threat",252},{"Cartel",495},{"Wingshot",569},{"Splash",0},
        {"Re.built",0},{"Inferno",0},{"Iron Clad",0},{"Contamination",0}
    }});
    m_weapons.push_back({"Desert Eagle", 1, "Pistols", {
        {"Blaze",37},{"Printstream",1018},{"Code Red",661},{"Mecha Industries",828},
        {"Kumicho Dragon",520},{"Conspiracy",351},{"Sunset Storm",0},{"Trigger Discipline",0},
        {"Ocean Drive",0},{"Fennec Fox",0},{"Light Rail",0},{"Sputnik",0},
        {"Bronze Deco",0},{"Midnight Storm",0},{"Night",27},{"Cobalt Disruption",0},
        {"Emerald Jormungandr",0},{"Golden Koi",0},{"Hypnotic",0},{"Heirloom",0},
        {"Hand Cannon",0},{"Oxide Blaze",685}
    }});
    m_weapons.push_back({"Five-SeveN", 3, "Pistols", {
        {"Hyper Beast",510},{"Monkey Business",535},{"Angry Mob",735},
        {"Neon Kimono",733},{"Case Hardened",44},{"Fowl Play",0},
        {"Berries And Cherries",0},{"Fairy Tale",0},{"Scrawl",0},
        {"Triumvirate",0},{"Retrobution",0},{"Copper Galaxy",0},
        {"Nightshade",0},{"Kami",0},{"Silver Quartz",0}
    }});
    m_weapons.push_back({"Tec-9", 30, "Pistols", {
        {"Fuel Injector",649},{"Decimator",647},{"Nuclear Threat",252},
        {"Re-Entry",0},{"Bamboozle",882},{"Remote Control",0},
        {"Terrace",0},{"Toxic",0},{"Titanium Bit",0},{"Isaac",407},
        {"Avalanche",0},{"Red Quartz",0},{"Ice Cap",0}
    }});
    m_weapons.push_back({"CZ75-Auto", 63, "Pistols", {
        {"Victoria",313},{"Xiangliu",866},{"Emerald Quartz",657},
        {"Vendetta",523},{"Eco",0},{"Tacticat",0},
        {"The Fuschia Is Now",0},{"Trigger Discipline",0},
        {"Poison Dart",0},{"Red Astor",0},{"Chalice",0},
        {"Nitro",0},{"Yellow Jacket",0}
    }});
    m_weapons.push_back({"Dual Berettas", 2, "Pistols", {
        {"Cobalt Quartz",0},{"Twin Turbo",0},{"Melondrama",0},
        {"Flora Carnivora",0},{"Royal Consorts",0},{"Urban Shock",0},
        {"Hemoglobin",0},{"Shred",0},{"Stained",173},
        {"Marina",0},{"Cartel",495},{"Balance",0}
    }});
    m_weapons.push_back({"P2000", 32, "Pistols", {
        {"Fire Elemental",0},{"Imperial Dragon",0},{"Amber Fade",0},
        {"Ocean Foam",0},{"Corticera",0},{"Imperial",0},
        {"Handgun",0},{"Scorpion",0},{"Obsidian",0},
        {"Pathfinder",0},{"Oceanic",0},{"Silver",0}
    }});
    m_weapons.push_back({"R8 Revolver", 64, "Pistols", {
        {"Fade",38},{"Banana Cannon",904},{"Llama Cannon",0},
        {"Crimson Web",12},{"Amber Fade",0},{"Memento",0},
        {"Grip",0},{"Skull Crusher",0},{"Nitro",0},{"Bone Mask",0}
    }});

    // ── Rifles ──
    m_weapons.push_back({"AK-47", 7, "Rifles", {
        {"Wild Lotus",0},{"Fire Serpent",180},{"Vulcan",349},{"Neon Rider",786},
        {"Redline",282},{"Asiimov",524},{"The Empress",712},{"Bloodsport",796},
        {"Neon Revolution",656},{"Fuel Injector",653},{"Aquamarine Revenge",545},
        {"Phantom Disruptor",0},{"Jaguar",441},{"Frontside Misty",550},
        {"Point Disarray",624},{"Wasteland Rebel",511},{"Hydroponic",547},
        {"Case Hardened",44},{"Ice Coaled",0},{"Nightwish",0},
        {"Slate",0},{"X-Ray",312},{"Rat Rod",0},{"Orbit Mk01",662},
        {"Head Shot",0},{"Legion of Anubis",0},{"Jet Set",0},
        {"Blue Laminate",206},{"Safety Net",0},{"Inheritance",0}
    }});
    m_weapons.push_back({"M4A4", 16, "Rifles", {
        {"Howl",309},{"Asiimov",255},{"Neo-Noir",770},{"Desolate Space",648},
        {"The Emperor",749},{"Buzz Kill",0},{"Royal Paladin",0},
        {"Dragon King",400},{"Spider Lily",0},{"In Living Color",0},
        {"Tooth Fairy",0},{"The Coalition",0},{"Temukau",0},
        {"Cyber Security",0},{"Radiation Hazard",0},{"Desert-Strike",0},
        {"X-Ray",0},{"Mainframe",0},{"Daybreak",0},{"Poly Mag",0},
        {"Eye of Horus",0},{"Global Offensive",0}
    }});
    m_weapons.push_back({"M4A1-S", 60, "Rifles", {
        {"Printstream",1016},{"Hyper Beast",519},{"Chantico's Fire",639},
        {"Golden Coil",632},{"Mecha Industries",0},{"Decimator",0},
        {"Player Two",0},{"Leaded Glass",0},{"Night Terror",0},
        {"Nightmare",0},{"Master Piece",0},{"Hot Rod",445},
        {"Icarus Fell",524},{"Cyrex",360},{"Guardian",290},
        {"Atomic Alloy",325},{"Knight",0},{"Welcome to the Jungle",0},
        {"Blue Phosphor",0},{"Control Panel",0},{"Basilisk",0}
    }});
    m_weapons.push_back({"AWP", 9, "Rifles", {
        {"Dragon Lore",344},{"Gungnir",0},{"Medusa",0},{"Fade",38},
        {"Asiimov",279},{"Lightning Strike",141},{"Hyper Beast",520},
        {"Containment Breach",0},{"The Prince",0},{"Wildfire",0},
        {"Neo-Noir",0},{"Chromatic Aberration",0},{"Electric Hive",346},
        {"Redline",282},{"BOOM",258},{"Corticera",0},{"Man-o'-war",0},
        {"Fever Dream",0},{"Atheris",0},{"PAW",0},{"Silk Tiger",0},
        {"Phobos",0},{"Sun in Leo",0},{"Duality",0},
        {"Exoskeleton",0},{"Pit Viper",0},{"Graphite",0},
        {"Desert Hydra",0},{"Capillary",0}
    }});
    m_weapons.push_back({"SG 553", 39, "Rifles", {
        {"Integrale",0},{"Darkwing",0},{"Cyrex",0},{"Pulse",0},
        {"Bulldozer",0},{"Tiger Moth",0},{"Phantom",0},
        {"Danger Close",0},{"Ultraviolet",0},{"Tornado",0}
    }});
    m_weapons.push_back({"AUG", 8, "Rifles", {
        {"Akihabara Accept",0},{"Chameleon",0},{"Flame Jinn",0},
        {"Stymphalian",0},{"Midnight Lily",0},{"Momentum",0},
        {"Fleet Flock",0},{"Syd Mead",0},{"Hot Rod",445},
        {"Bengal Tiger",0},{"Arctic Wolf",0},{"Random Access",0}
    }});
    m_weapons.push_back({"FAMAS", 10, "Rifles", {
        {"Eye of Athena",0},{"Mecha Industries",0},{"Roll Cage",0},
        {"Commemoration",0},{"Valence",0},{"Neural Net",0},
        {"Djinn",0},{"Pulse",0},{"Afterimage",0},
        {"Decommissioned",0},{"Sundown",0},{"Sergeant",0}
    }});
    m_weapons.push_back({"Galil AR", 13, "Rifles", {
        {"Chatterbox",0},{"Cerberus",0},{"Aqua Terrace",0},
        {"Firefight",0},{"Sugar Rush",0},{"Eco",0},
        {"Crimson Tsunami",0},{"Stone Cold",0},{"Rocket Pop",0},
        {"Connexion",0},{"Akoben",0},{"Orange DDPAT",0}
    }});

    // ── SMGs ──
    m_weapons.push_back({"MAC-10", 17, "SMGs", {
        {"Neon Rider",786},{"Stalker",0},{"Case Hardened",44},{"Disco Tech",0},
        {"Heat",0},{"Pipe Down",0},{"Last Dive",0},{"Propaganda",0},
        {"Fade",38},{"Copper Borre",0},{"Silver",0},{"Nuclear Garden",683},
        {"Curse",0},{"Allure",0},{"Light Box",0}
    }});
    m_weapons.push_back({"MP9", 34, "SMGs", {
        {"Wild Lily",0},{"Hydra",695},{"Bulldozer",0},{"Airlock",0},
        {"Rose Iron",0},{"Music Box",0},{"Stained Glass",0},
        {"Setting Sun",0},{"Bioleak",0},{"Food Chain",0},
        {"Ruby Poison Dart",0},{"Mount Fuji",0},{"Old Roots",0}
    }});
    m_weapons.push_back({"UMP-45", 24, "SMGs", {
        {"Primal Saber",0},{"Moonrise",837},{"Fade",38},{"Momentum",0},
        {"Arctic Wolf",0},{"Plastique",0},{"Day Lily",0},
        {"Oscillator",0},{"Wild Child",0},{"Exposure",0},
        {"Scaffold",0},{"Corporal",0},{"Labyrinth",0}
    }});
    m_weapons.push_back({"MP7", 33, "SMGs", {
        {"Nemesis",395},{"Bloodsport",0},{"Neon Ply",0},{"Impire",0},
        {"Special Delivery",0},{"Fade",38},{"Skulls",0},
        {"Teal Blossom",0},{"Akoben",0},{"Cirrus",0},
        {"Armor Core",0},{"Whiteout",0}
    }});
    m_weapons.push_back({"P90", 19, "SMGs", {
        {"Asiimov",551},{"Death by Kitty",0},{"Nostalgia",0},{"Shapewood",0},
        {"Trigon",0},{"Emerald Dragon",0},{"Cold Blooded",0},
        {"Run and Hide",0},{"Shallow Grave",0},{"Blind Spot",0},
        {"Baroque Red",0},{"Off World",850},{"Facility Dark",0},
        {"Virus",0},{"Elite Build",0},{"Desert Warfare",0}
    }});
    m_weapons.push_back({"PP-Bizon", 26, "SMGs", {
        {"Judgement of Anubis",0},{"High Roller",0},{"Embargo",0},
        {"Blue Streak",0},{"Fuel Rod",0},{"Antique",0},
        {"Osiris",0},{"Carbon Fiber",0},{"Rust Coat",0}
    }});
    m_weapons.push_back({"MP5-SD", 23, "SMGs", {
        {"Lab Rats",0},{"Phosphor",0},{"Agent",0},{"Gauss",0},
        {"Oxide Oasis",0},{"Co-Processor",0},{"Condition Zero",0}
    }});

    // ── Shotguns ──
    m_weapons.push_back({"Nova", 35, "Shotguns", {
        {"Hyper Beast",0},{"Antique",0},{"Wild Six",0},{"Gila",0},
        {"Toy Soldier",0},{"Exo",0},{"Koi",0},{"Graphite",0},
        {"Bloomstick",0},{"Plume",0},{"Sand Dune",208}
    }});
    m_weapons.push_back({"XM1014", 25, "Shotguns", {
        {"Incinegator",0},{"Ziggy",0},{"Tranquility",0},
        {"Entombed",0},{"Bone Machine",0},{"Seasons",0},
        {"Teclu Burner",0},{"Black Tie",0},{"Blue Steel",0}
    }});
    m_weapons.push_back({"MAG-7", 27, "Shotguns", {
        {"Justice",0},{"SWAG-7",0},{"Heat",0},{"Prism Terrace",0},
        {"Petroglyph",0},{"Cinquedea",0},{"Bulldozer",0},
        {"Sonar",0},{"Popdog",0},{"Hard Water",0}
    }});
    m_weapons.push_back({"Sawed-Off", 29, "Shotguns", {
        {"The Kraken",0},{"Apocalypto",0},{"Devourer",0},
        {"Limelight",0},{"Morris",0},{"Copper",0},{"Brake Light",0}
    }});

    // ── Machine Guns ──
    m_weapons.push_back({"M249", 14, "Machine Guns", {
        {"Magma",0},{"Spectre",0},{"Impact Drill",0},
        {"System Lock",0},{"Shipping Forecast",0},{"Downtown",0}
    }});
    m_weapons.push_back({"Negev", 28, "Machine Guns", {
        {"Power Loader",0},{"Loudmouth",0},{"Bratatat",0},
        {"Lionfish",0},{"Dazzle",0},{"Dev Texture",0},{"Palm",0}
    }});

    // ── Snipers ──
    m_weapons.push_back({"SSG 08", 40, "Snipers", {
        {"Dragonfire",0},{"Blood in the Water",0},{"Big Iron",0},
        {"Death Strike",0},{"Fever Dream",0},{"Necropos",0},
        {"Ghost Crusader",0},{"Abyss",0},{"Dark Water",187},
        {"Detour",0},{"Turbo Peek",0},{"Acid Fade",0}
    }});
    m_weapons.push_back({"SCAR-20", 38, "Snipers", {
        {"Emerald",0},{"Cardiac",0},{"Cyrex",0},{"Bloodsport",796},
        {"Jungle Slipstream",0},{"Blueprint",826},{"Torn",0}
    }});
    m_weapons.push_back({"G3SG1", 11, "Snipers", {
        {"The Executioner",0},{"Flux",0},{"Scavenger",0},
        {"Stinger",0},{"Chronos",0},{"High Seas",0},
        {"Ventilator",0},{"Black Sand",0}
    }});

    // ── Knives ──
    m_weapons.push_back({"Karambit", 507, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Autotronic",619},{"Gamma Doppler",568},
        {"Bright Water",0},{"Freehand",0},{"Black Laminate",0},
        {"Vanilla",0},{"Damascus Steel",0},{"Ultraviolet",0},
        {"Night Stripe",0},{"Rust Coat",0},{"Blue Steel",215},
        {"Boreal Forest",77},{"Stained",173},{"Forest DDPAT",0},
        {"Safari Mesh",72},{"Urban Masked",73},{"Scorched",175}
    }});
    m_weapons.push_back({"Butterfly Knife", 515, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Autotronic",619},{"Gamma Doppler",568},
        {"Vanilla",0},{"Damascus Steel",0},{"Ultraviolet",0},
        {"Night Stripe",0},{"Rust Coat",0},{"Blue Steel",215},
        {"Boreal Forest",77},{"Stained",173},{"Forest DDPAT",0},
        {"Safari Mesh",72},{"Urban Masked",73},{"Scorched",175}
    }});
    m_weapons.push_back({"M9 Bayonet", 508, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Autotronic",619},{"Gamma Doppler",568},
        {"Vanilla",0},{"Damascus Steel",0},{"Ultraviolet",0},
        {"Blue Steel",215},{"Boreal Forest",77},{"Stained",173},
        {"Forest DDPAT",0},{"Safari Mesh",72}
    }});
    m_weapons.push_back({"Bayonet", 500, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Autotronic",619},{"Vanilla",0},
        {"Damascus Steel",0},{"Ultraviolet",0},{"Blue Steel",215},
        {"Boreal Forest",77},{"Stained",173},{"Forest DDPAT",0},
        {"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Skeleton Knife", 525, "Knives", {
        {"Fade",38},{"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Blue Steel",215},{"Stained",173},{"Vanilla",0},
        {"Night Stripe",0},{"Boreal Forest",77},{"Forest DDPAT",0},
        {"Safari Mesh",72},{"Urban Masked",73},{"Scorched",175}
    }});
    m_weapons.push_back({"Talon Knife", 520, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Vanilla",0},{"Damascus Steel",0},
        {"Ultraviolet",0},{"Blue Steel",215},{"Boreal Forest",77},
        {"Stained",173},{"Forest DDPAT",0},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Stiletto Knife", 522, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Vanilla",0},{"Damascus Steel",0},{"Blue Steel",215},
        {"Boreal Forest",77},{"Stained",173},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Ursus Knife", 519, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Vanilla",0},{"Damascus Steel",0},{"Blue Steel",215},
        {"Boreal Forest",77},{"Stained",173},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Navaja Knife", 523, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Crimson Web",12},
        {"Slaughter",59},{"Case Hardened",44},{"Vanilla",0},
        {"Damascus Steel",0},{"Blue Steel",215},{"Boreal Forest",77},
        {"Stained",173},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Huntsman Knife", 509, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Vanilla",0},{"Damascus Steel",0},
        {"Blue Steel",215},{"Boreal Forest",77},{"Stained",173},
        {"Forest DDPAT",0},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Flip Knife", 505, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Gamma Doppler",568},{"Vanilla",0},
        {"Damascus Steel",0},{"Blue Steel",215},{"Boreal Forest",77},
        {"Stained",173},{"Forest DDPAT",0},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Gut Knife", 506, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Gamma Doppler",568},{"Vanilla",0},
        {"Blue Steel",215},{"Boreal Forest",77},{"Stained",173},
        {"Forest DDPAT",0},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Kukri Knife", 526, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Marble Fade",413},
        {"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Lore",344},{"Gamma Doppler",568},{"Vanilla",0},
        {"Damascus Steel",0},{"Blue Steel",215},{"Boreal Forest",77},
        {"Stained",173},{"Forest DDPAT",0},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Bowie Knife", 514, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Crimson Web",12},
        {"Slaughter",59},{"Case Hardened",44},{"Lore",344},
        {"Vanilla",0},{"Blue Steel",215},{"Boreal Forest",77},
        {"Stained",173},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Falchion Knife", 512, "Knives", {
        {"Doppler",418},{"Fade",38},{"Tiger Tooth",409},{"Crimson Web",12},
        {"Slaughter",59},{"Case Hardened",44},{"Lore",344},
        {"Vanilla",0},{"Blue Steel",215},{"Boreal Forest",77},
        {"Stained",173},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Classic Knife", 503, "Knives", {
        {"Fade",38},{"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Blue Steel",215},{"Vanilla",0},{"Boreal Forest",77},
        {"Forest DDPAT",0},{"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Paracord Knife", 517, "Knives", {
        {"Fade",38},{"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Blue Steel",215},{"Vanilla",0},{"Boreal Forest",77},
        {"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Survival Knife", 518, "Knives", {
        {"Fade",38},{"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Blue Steel",215},{"Vanilla",0},{"Boreal Forest",77},
        {"Safari Mesh",72},{"Scorched",175}
    }});
    m_weapons.push_back({"Nomad Knife", 521, "Knives", {
        {"Fade",38},{"Crimson Web",12},{"Slaughter",59},{"Case Hardened",44},
        {"Blue Steel",215},{"Vanilla",0},{"Boreal Forest",77},
        {"Safari Mesh",72},{"Scorched",175}
    }});

    // ── Gloves ──
    m_weapons.push_back({"Sport Gloves", 5030, "Gloves", {
        {"Pandora's Box",0},{"Hedge Maze",0},{"Superconductor",0},{"Vice",0},
        {"Amphibious",0},{"Bronze Morph",0},{"Omega",0},{"Arid",0},
        {"Scarlet Shamagh",0},{"Slingshot",0},{"Nocts",0},{"Big Game",0}
    }});
    m_weapons.push_back({"Specialist Gloves", 5033, "Gloves", {
        {"Crimson Kimono",0},{"Emerald Web",0},{"Fade",38},{"Mogul",0},
        {"Foundation",0},{"Marble Fade",0},{"Field Agent",0},
        {"Forest DDPAT",0},{"Buckshot",0},{"Tiger Strike",0},
        {"Lt. Commander",0},{"Cool Mint",0}
    }});
    m_weapons.push_back({"Driver Gloves", 5031, "Gloves", {
        {"King Snake",0},{"Imperial Plaid",0},{"Crimson Weave",0},
        {"Lunar Weave",0},{"Diamondback",0},{"Overtake",0},
        {"Racing Green",0},{"Convoy",0},{"Queen Jaguar",0},
        {"Snow Leopard",0},{"Black Tie",0}
    }});
    m_weapons.push_back({"Hand Wraps", 5032, "Gloves", {
        {"Cobalt Skulls",0},{"Overprint",0},{"Slaughter",59},{"Leather",0},
        {"Badlands",0},{"Spruce DDPAT",0},{"Duct Tape",0},
        {"Arboreal",0},{"Giraffe",0},{"Cashmere",0},{"Constrictor",0}
    }});
    m_weapons.push_back({"Moto Gloves", 5034, "Gloves", {
        {"Spearmint",0},{"Cool Mint",0},{"Polygon",0},{"Boom!",0},
        {"Eclipse",0},{"Transport",0},{"Turtle",0},{"POW!",0},
        {"Finish Line",0},{"Blood Pressure",0},{"Smoke Out",0}
    }});
    m_weapons.push_back({"Hydra Gloves", 5035, "Gloves", {
        {"Case Hardened",44},{"Emerald",0},{"Rattler",0},
        {"Mangrove",0},{"Circuit Board",0}
    }});
    m_weapons.push_back({"Broken Fang Gloves", 5036, "Gloves", {
        {"Jade",0},{"Yellow-banded",0},{"Unhinged",0},
        {"Needle Point",0},{"Finisher",0}
    }});
}

// ============================================================================
//  Mouse
// ============================================================================
void GameMenu::OnMouseMove(int x, int y) {
    m_mouseX = x; m_mouseY = y;
    if (m_sliderDrag && m_view == INV_DETAIL) {
        float t = (float)(x - m_sliderR.x) / (float)m_sliderR.w;
        if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
        m_detailFloat = t;
    }
}

void GameMenu::OnMouseUp() { m_sliderDrag = false; }

void GameMenu::OnMouseClick(int x, int y) {
    if (!m_visible) return;

    // Sidebar nav
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (Hit(m_sideR[i], x, y)) {
            m_page = (Page)i;
            m_scroll = 0;
            if (i == PAGE_INVENTORY) m_view = INV_EQUIPPED;
            if (i == PAGE_CONFIGS) ScanPresets();
            return;
        }
    }

    if (m_page == PAGE_INVENTORY) {
        // "Tilf\xF8j Skin" button
        if (Hit(m_addBtn, x, y)) { m_view = INV_WEAPONS; m_scroll = 0; return; }
        // Back button
        if (Hit(m_backBtn, x, y)) {
            if      (m_view == INV_DETAIL)  { m_view = INV_SKINS;   m_scroll = 0; }
            else if (m_view == INV_SKINS)   { m_view = INV_WEAPONS; m_scroll = 0; }
            else if (m_view == INV_WEAPONS) { m_view = INV_EQUIPPED; m_scroll = 0; }
            return;
        }
        // Apply (Tilf\xF8j) in detail
        if (m_view == INV_DETAIL && Hit(m_applyBtn, x, y)) {
            if (m_selWeapon >= 0 && m_selWeapon < (int)m_weapons.size()) {
                auto& w = m_weapons[m_selWeapon];
                if (m_selSkin >= 0 && m_selSkin < (int)w.skins.size()) {
                    bool found = false;
                    for (auto& eq : m_equipped)
                        if (eq.weapon == w.name && eq.skin == w.skins[m_selSkin].name)
                            { eq.floatVal = m_detailFloat; found = true; break; }
                    if (!found)
                        m_equipped.push_back({w.name, w.skins[m_selSkin].name,
                            w.weaponId, w.skins[m_selSkin].paintKit, m_detailFloat});
                    m_view = INV_EQUIPPED; m_scroll = 0;
                }
            }
            return;
        }
        // Slider
        if (m_view == INV_DETAIL && Hit(m_sliderR, x, y)) {
            m_sliderDrag = true;
            float t = (float)(x - m_sliderR.x) / (float)m_sliderR.w;
            if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
            m_detailFloat = t;
            return;
        }
        // Grid clicks
        for (int i = 0; i < m_gN; i++) {
            if (Hit(m_gR[i], x, y)) {
                if (m_view == INV_WEAPONS) { m_selWeapon = m_gId[i]; m_view = INV_SKINS; m_scroll = 0; }
                else if (m_view == INV_SKINS) { m_selSkin = m_gId[i]; m_detailFloat = 0.01f; m_view = INV_DETAIL; m_scroll = 0; }
                return;
            }
        }
        // Remove buttons
        for (int i = 0; i < m_remN; i++) {
            if (Hit(m_remR[i], x, y)) {
                if (i < (int)m_equipped.size()) m_equipped.erase(m_equipped.begin() + i);
                return;
            }
        }
    }

    if (m_page == PAGE_CONFIGS) {
        // Save button
        if (Hit(m_saveBtn, x, y)) { SavePreset(); ScanPresets(); return; }
        // Apply to game button
        if (Hit(m_applyGameBtn, x, y)) {
            // The SkinChanger integration would write to CS2 memory here.
            // For now, this is a placeholder that logs the action.
            DbgLog("ApplyToGame: %d items in inventory", (int)m_equipped.size());
            return;
        }
        // Preset load/delete
        for (int i = 0; i < m_pN; i++) {
            if (Hit(m_pLoadR[i], x, y)) { LoadPreset(i); return; }
            if (Hit(m_pDelR[i], x, y)) { DeletePreset(i); ScanPresets(); return; }
        }
    }
}

void GameMenu::OnMouseWheel(int delta) {
    if (!m_visible) return;
    m_scroll -= delta / 40;
    if (m_scroll < 0) m_scroll = 0;
}

// ============================================================================
//  GDI Helpers
// ============================================================================
void GameMenu::RRect(HDC hdc, int x, int y, int w, int h, int r,
                     COLORREF fill, COLORREF brd) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID, 1, brd);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN   op = (HPEN)SelectObject(hdc, pn);
    RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(br); DeleteObject(pn);
}

void GameMenu::Txt(HDC hdc, const char* s, int x, int y, int w, int h,
                   COLORREF c, HFONT f, UINT fmt) {
    HFONT of = (HFONT)SelectObject(hdc, f);
    SetTextColor(hdc, c); SetBkMode(hdc, TRANSPARENT);
    RECT rc = {x, y, x+w, y+h};
    DrawTextA(hdc, s, -1, &rc, fmt);
    SelectObject(hdc, of);
}

void GameMenu::Btn(HDC hdc, int x, int y, int w, int h,
                   const char* text, bool hov, COLORREF bg, COLORREF bgH) {
    RRect(hdc, x, y, w, h, 6, hov ? bgH : bg, hov ? NL_ACCENT_L : NL_ACCENT);
    Txt(hdc, text, x, y, w, h, NL_WHITE, m_fBold, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}

// ============================================================================
//  Render – main dispatcher
// ============================================================================
void GameMenu::Render(HDC hdc, int winW, int winH) {
    if (!m_visible) return;
    int mx = (winW - MENU_W) / 2, my = (winH - MENU_H) / 2;
    m_menuX = mx; m_menuY = my;
    m_gN = 0; m_remN = 0; m_pN = 0;

    // Drop shadow
    RRect(hdc, mx+6, my+6, MENU_W, MENU_H, RAD, RGB(3,3,5), RGB(3,3,5));
    // Main frame
    RRect(hdc, mx, my, MENU_W, MENU_H, RAD, NL_BG, NL_BORDER);

    DrawSidebar(hdc, mx, my, MENU_H);

    int cx = mx + SIDE_W, cw = MENU_W - SIDE_W;
    DrawHeader(hdc, cx, my, cw);

    int cy = my + HDR_H, ch = MENU_H - HDR_H;
    HRGN clip = CreateRectRgn(cx, cy, cx + cw, my + MENU_H);
    SelectClipRgn(hdc, clip);

    if (m_page == PAGE_INVENTORY) {
        switch (m_view) {
        case INV_EQUIPPED: DrawEquipped(hdc, cx, cy, cw, ch); break;
        case INV_WEAPONS:  DrawWeapons(hdc, cx, cy, cw, ch);  break;
        case INV_SKINS:    DrawSkins(hdc, cx, cy, cw, ch);    break;
        case INV_DETAIL:   DrawDetail(hdc, cx, cy, cw, ch);   break;
        }
    } else {
        DrawConfigs(hdc, cx, cy, cw, ch);
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);
}

// ============================================================================
//  Sidebar  –  exact NEVERLOSE layout
// ============================================================================
void GameMenu::DrawSidebar(HDC hdc, int x, int y, int h) {
    // Sidebar panel
    RECT sbr = {x, y, x + SIDE_W, y + h};
    HBRUSH sbb = CreateSolidBrush(NL_SIDEBAR);
    FillRect(hdc, &sbr, sbb);
    DeleteObject(sbb);

    // Right border
    HPEN bp = CreatePen(PS_SOLID, 1, NL_SIDEBAR_BR);
    HPEN obp = (HPEN)SelectObject(hdc, bp);
    MoveToEx(hdc, x + SIDE_W - 1, y, nullptr);
    LineTo(hdc, x + SIDE_W - 1, y + h);
    SelectObject(hdc, obp); DeleteObject(bp);

    // Logo area
    // Blue accent square
    RRect(hdc, x + 20, y + 18, 36, 36, 8, NL_ACCENT, NL_ACCENT_D);
    Txt(hdc, "AC", x + 20, y + 18, 36, 36, NL_WHITE, m_fBold,
        DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    // Brand name
    Txt(hdc, "AC Changer", x + 64, y + 20, SIDE_W - 72, 18, NL_WHITE, m_fBold,
        DT_LEFT|DT_SINGLELINE);
    Txt(hdc, "v2.0 · NEVERLOSE", x + 64, y + 38, SIDE_W - 72, 14, NL_TEXT3, m_fSmall,
        DT_LEFT|DT_SINGLELINE);

    // Separator
    HPEN sep = CreatePen(PS_SOLID, 1, NL_SIDEBAR_BR);
    HPEN osep = (HPEN)SelectObject(hdc, sep);
    MoveToEx(hdc, x + 16, y + 65, nullptr);
    LineTo(hdc, x + SIDE_W - 16, y + 65);
    SelectObject(hdc, osep); DeleteObject(sep);

    // Section label
    Txt(hdc, "MAIN MENU", x + 20, y + 74, SIDE_W - 40, 16, NL_TEXT3, m_fSmall,
        DT_LEFT|DT_SINGLELINE);

    // Navigation items
    const char* labels[PAGE_COUNT] = {"Inventory Changer", "Configs"};
    const char* icons[PAGE_COUNT]  = {"I", "C"};  // Single letter icon
    int ny = y + 96;

    for (int i = 0; i < PAGE_COUNT; i++) {
        bool sel = ((int)m_page == i);
        bool hov = Hit({x + 8, ny, SIDE_W - 16, 38}, m_mouseX, m_mouseY);

        if (sel) {
            // Active: blue-tinted card with left accent bar
            RRect(hdc, x + 8, ny, SIDE_W - 16, 38, 6, NL_CARD_SEL, NL_ACCENT_D);
            // Left accent bar (3px wide blue bar – NEVERLOSE signature)
            RECT bar = {x + 8, ny + 6, x + 11, ny + 32};
            HBRUSH ab = CreateSolidBrush(NL_ACCENT);
            FillRect(hdc, &bar, ab); DeleteObject(ab);
        } else if (hov) {
            RRect(hdc, x + 8, ny, SIDE_W - 16, 38, 6, RGB(22,22,30), NL_SIDEBAR_BR);
        }

        // Icon circle
        COLORREF ic = sel ? NL_ACCENT : NL_TEXT3;
        RRect(hdc, x + 18, ny + 8, 22, 22, 11, sel ? NL_ACCENT_D : RGB(30,30,38), ic);
        Txt(hdc, icons[i], x + 18, ny + 8, 22, 22, sel ? NL_WHITE : NL_TEXT2, m_fSmall,
            DT_CENTER|DT_VCENTER|DT_SINGLELINE);

        // Label
        COLORREF lc = sel ? NL_ACCENT_L : (hov ? NL_TEXT : NL_TEXT2);
        Txt(hdc, labels[i], x + 46, ny, SIDE_W - 54, 38, lc, m_fBody,
            DT_LEFT|DT_VCENTER|DT_SINGLELINE);

        m_sideR[i] = {x + 8, ny, SIDE_W - 16, 38};
        ny += 44;
    }

    // Bottom status
    int sy = y + h - 52;
    HPEN bsep = CreatePen(PS_SOLID, 1, NL_SIDEBAR_BR);
    HPEN obsep = (HPEN)SelectObject(hdc, bsep);
    MoveToEx(hdc, x + 16, sy, nullptr);
    LineTo(hdc, x + SIDE_W - 16, sy);
    SelectObject(hdc, obsep); DeleteObject(bsep);

    // Green dot + "Connected"
    HBRUSH gd = CreateSolidBrush(NL_GREEN);
    HPEN gp = CreatePen(PS_SOLID, 1, NL_GREEN);
    SelectObject(hdc, gd); SelectObject(hdc, gp);
    Ellipse(hdc, x + 20, sy + 16, x + 28, sy + 24);
    DeleteObject(gd); DeleteObject(gp);
    Txt(hdc, "Connected to CS2", x + 34, sy + 8, SIDE_W - 50, 22, NL_GREEN, m_fSmall,
        DT_LEFT|DT_VCENTER|DT_SINGLELINE);
}

// ============================================================================
//  Header
// ============================================================================
void GameMenu::DrawHeader(HDC hdc, int x, int y, int w) {
    RECT hr = {x, y, x + w, y + HDR_H};
    HBRUSH hb = CreateSolidBrush(NL_HEADER);
    FillRect(hdc, &hr, hb); DeleteObject(hb);

    // Bottom border
    HPEN sp = CreatePen(PS_SOLID, 1, NL_BORDER);
    HPEN op = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, x, y + HDR_H - 1, nullptr);
    LineTo(hdc, x + w, y + HDR_H - 1);
    SelectObject(hdc, op); DeleteObject(sp);

    // Title
    const char* title = "";
    if (m_page == PAGE_INVENTORY) {
        switch (m_view) {
        case INV_EQUIPPED: title = "Inventory"; break;
        case INV_WEAPONS:  title = "V\xE6lg V\xE5""ben"; break;
        case INV_SKINS:
            if (m_selWeapon >= 0 && m_selWeapon < (int)m_weapons.size())
                title = m_weapons[m_selWeapon].name.c_str();
            else title = "Skins";
            break;
        case INV_DETAIL: title = "Skin Detaljer"; break;
        }
    } else title = "Configs";

    Txt(hdc, title, x + PAD, y, 300, HDR_H, NL_WHITE, m_fTitle,
        DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    // "+ Tilf\xF8j Skin" button (equipped view only)
    if (m_page == PAGE_INVENTORY && m_view == INV_EQUIPPED) {
        int bw = 120, bh = 32, bx = x + w - bw - PAD, by = y + (HDR_H - bh) / 2;
        bool hov = Hit({bx, by, bw, bh}, m_mouseX, m_mouseY);
        Btn(hdc, bx, by, bw, bh, "+ Tilf\xF8j Skin", hov, NL_ACCENT, NL_ACCENT_L);
        m_addBtn = {bx, by, bw, bh};
    } else m_addBtn = {0,0,0,0};

    // Back button (weapons/skins/detail)
    if (m_page == PAGE_INVENTORY && m_view != INV_EQUIPPED) {
        int bw = 80, bh = 28, bx = x + w - bw - PAD, by = y + (HDR_H - bh) / 2;
        bool hov = Hit({bx, by, bw, bh}, m_mouseX, m_mouseY);
        Btn(hdc, bx, by, bw, bh, "\x3C Tilbage", hov, RGB(35,35,45), RGB(45,45,58));
        m_backBtn = {bx, by, bw, bh};
    } else m_backBtn = {0,0,0,0};
}

// ============================================================================
//  Equipped Inventory  –  grid of added items
// ============================================================================
void GameMenu::DrawEquipped(HDC hdc, int x, int y, int w, int h) {
    int py = y + PAD - m_scroll;

    if (m_equipped.empty()) {
        Txt(hdc, "Ingen skins tilf\xF8jet endnu.", x + PAD, py + 60, w - PAD*2, 24,
            NL_TEXT2, m_fBody, DT_CENTER|DT_SINGLELINE);
        Txt(hdc, "Klik \"+ Tilf\xF8j Skin\" for at tilf\xF8je skins til dit CS2 inventory.",
            x + PAD, py + 90, w - PAD*2, 20, NL_TEXT3, m_fSmall, DT_CENTER|DT_SINGLELINE);

        // Instruction box
        int bx = x + 80, bw = w - 160, infoY = py + 140;
        RRect(hdc, bx, infoY, bw, 90, 8, NL_CARD, NL_BORDER);
        Txt(hdc, "S\xE5""dan virker det:", bx + PAD, infoY + 10, bw - PAD*2, 18,
            NL_ACCENT_L, m_fBold, DT_LEFT|DT_SINGLELINE);
        Txt(hdc, "1. Klik \"+ Tilf\xF8j Skin\" og v\xE6lg et v\xE5""ben",
            bx + PAD, infoY + 32, bw - PAD*2, 16, NL_TEXT, m_fSmall, DT_LEFT|DT_SINGLELINE);
        Txt(hdc, "2. V\xE6lg en skin og indstil float v\xE6rdien",
            bx + PAD, infoY + 50, bw - PAD*2, 16, NL_TEXT, m_fSmall, DT_LEFT|DT_SINGLELINE);
        Txt(hdc, "3. G\xE5 til Configs og klik \"Anvend p\xE5 CS2\"",
            bx + PAD, infoY + 68, bw - PAD*2, 16, NL_TEXT, m_fSmall, DT_LEFT|DT_SINGLELINE);
        return;
    }

    int cols = 4;
    int cardW = (w - PAD*2 - (cols-1)*GGAP) / cols;
    int cardH = 140;
    int col = 0, startX = x + PAD;

    for (int i = 0; i < (int)m_equipped.size(); i++) {
        auto& eq = m_equipped[i];
        int cx = startX + col * (cardW + GGAP);
        int cy = py;

        if (cy + cardH > y && cy < y + h) {
            bool hov = Hit({cx, cy, cardW, cardH}, m_mouseX, m_mouseY);

            // Card
            RRect(hdc, cx, cy, cardW, cardH, 8,
                  hov ? NL_CARD_HOV : NL_CARD,
                  hov ? NL_ACCENT : NL_BORDER);

            // Left accent stripe (orange, like steam inventory)
            RECT ac = {cx + 1, cy + 8, cx + 4, cy + cardH - 8};
            HBRUSH ab = CreateSolidBrush(NL_ORANGE);
            FillRect(hdc, &ac, ab); DeleteObject(ab);

            // Weapon
            Txt(hdc, eq.weapon.c_str(), cx + 12, cy + 10, cardW - 24, 16,
                NL_WHITE, m_fBold, DT_LEFT|DT_SINGLELINE);
            // Skin
            Txt(hdc, eq.skin.c_str(), cx + 12, cy + 28, cardW - 24, 16,
                NL_TEXT, m_fBody, DT_LEFT|DT_SINGLELINE);

            // Float + wear
            char fb[32]; snprintf(fb, sizeof(fb), "Float: %.4f", eq.floatVal);
            Txt(hdc, fb, cx + 12, cy + 50, cardW - 24, 14, NL_TEXT2, m_fSmall, DT_LEFT|DT_SINGLELINE);
            Txt(hdc, WearName(eq.floatVal), cx + 12, cy + 66, cardW - 24, 14,
                NL_GREEN, m_fSmall, DT_LEFT|DT_SINGLELINE);

            // Paint kit ID
            if (eq.paintKit > 0) {
                char pk[32]; snprintf(pk, sizeof(pk), "Kit: %d", eq.paintKit);
                Txt(hdc, pk, cx + 12, cy + 82, cardW - 24, 14, NL_TEXT3, m_fSmall, DT_LEFT|DT_SINGLELINE);
            }

            // Remove button
            int rbw = 55, rbh = 22;
            int rbx = cx + (cardW - rbw) / 2, rby = cy + cardH - rbh - 10;
            bool rh = Hit({rbx, rby, rbw, rbh}, m_mouseX, m_mouseY);
            RRect(hdc, rbx, rby, rbw, rbh, 4,
                  rh ? RGB(60,25,25) : RGB(40,18,18), NL_RED);
            Txt(hdc, "Fjern", rbx, rby, rbw, rbh, NL_RED, m_fSmall,
                DT_CENTER|DT_VCENTER|DT_SINGLELINE);

            if (m_remN < MI) m_remR[m_remN++] = {rbx, rby, rbw, rbh};
        }

        col++;
        if (col >= cols) { col = 0; py += cardH + GGAP; }
    }
}

// ============================================================================
//  Weapon Grid  –  category cards
// ============================================================================
void GameMenu::DrawWeapons(HDC hdc, int x, int y, int w, int h) {
    int py = y + PAD - m_scroll;
    int cols = GCOLS;
    int cardW = (w - PAD*2 - (cols-1)*GGAP) / cols;
    int cardH = 80;
    int col = 0, startX = x + PAD;

    // Category tracking for section headers
    std::string lastCat;

    for (int i = 0; i < (int)m_weapons.size(); i++) {
        // Section header
        if (m_weapons[i].category != lastCat) {
            if (col != 0) { col = 0; py += cardH + GGAP; }
            lastCat = m_weapons[i].category;
            if (py + 24 > y && py < y + h) {
                Txt(hdc, lastCat.c_str(), startX, py, w - PAD*2, 20,
                    NL_ACCENT_L, m_fBold, DT_LEFT|DT_SINGLELINE);
            }
            py += 28;
        }

        int cx = startX + col * (cardW + GGAP);
        int cy = py;

        if (cy + cardH > y && cy < y + h) {
            bool hov = Hit({cx, cy, cardW, cardH}, m_mouseX, m_mouseY);

            RRect(hdc, cx, cy, cardW, cardH, 8,
                  hov ? NL_CARD_HOV : NL_CARD,
                  hov ? NL_ACCENT : NL_BORDER);

            // Weapon name centered
            Txt(hdc, m_weapons[i].name.c_str(), cx + 8, cy + 8, cardW - 16, 30,
                hov ? NL_WHITE : NL_TEXT, m_fBold,
                DT_CENTER|DT_VCENTER|DT_SINGLELINE);

            // Skin count
            char cb[24]; snprintf(cb, sizeof(cb), "%d skins", (int)m_weapons[i].skins.size());
            Txt(hdc, cb, cx + 8, cy + 40, cardW - 16, 16, NL_TEXT2, m_fSmall,
                DT_CENTER|DT_SINGLELINE);

            // Weapon ID small
            char idBuf[16]; snprintf(idBuf, sizeof(idBuf), "ID: %d", m_weapons[i].weaponId);
            Txt(hdc, idBuf, cx + 8, cy + cardH - 20, cardW - 16, 14, NL_TEXT3, m_fSmall,
                DT_CENTER|DT_SINGLELINE);

            if (m_gN < MG) { m_gR[m_gN] = {cx, cy, cardW, cardH}; m_gId[m_gN] = i; m_gN++; }
        }

        col++;
        if (col >= cols) { col = 0; py += cardH + GGAP; }
    }
}

// ============================================================================
//  Skin Grid  –  all skins for selected weapon
// ============================================================================
void GameMenu::DrawSkins(HDC hdc, int x, int y, int w, int h) {
    if (m_selWeapon < 0 || m_selWeapon >= (int)m_weapons.size()) return;
    auto& wd = m_weapons[m_selWeapon];

    int py = y + PAD - m_scroll;
    int cols = GCOLS;
    int cardW = (w - PAD*2 - (cols-1)*GGAP) / cols;
    int cardH = GCARDH;
    int col = 0, startX = x + PAD;

    for (int i = 0; i < (int)wd.skins.size(); i++) {
        int cx = startX + col * (cardW + GGAP);
        int cy = py;

        if (cy + cardH > y && cy < y + h) {
            bool hov = Hit({cx, cy, cardW, cardH}, m_mouseX, m_mouseY);

            // Check equipped
            bool eq = false;
            for (auto& e : m_equipped)
                if (e.weapon == wd.name && e.skin == wd.skins[i].name) { eq = true; break; }

            COLORREF bg = eq ? RGB(20,35,25) : (hov ? NL_CARD_HOV : NL_CARD);
            COLORREF br = eq ? NL_GREEN : (hov ? NL_ACCENT : NL_BORDER);

            RRect(hdc, cx, cy, cardW, cardH, 8, bg, br);

            // Rarity color bar at top
            COLORREF rc = RarityColor(i);
            RECT tb = {cx+2, cy+2, cx+cardW-2, cy+5};
            HBRUSH tbr = CreateSolidBrush(rc);
            FillRect(hdc, &tb, tbr); DeleteObject(tbr);

            // Weapon name (small, dim)
            Txt(hdc, wd.name.c_str(), cx + 8, cy + 12, cardW - 16, 14,
                NL_TEXT3, m_fSmall, DT_CENTER|DT_SINGLELINE);

            // Skin name (centered, main)
            Txt(hdc, wd.skins[i].name.c_str(), cx + 6, cy + 32, cardW - 12, 40,
                hov ? NL_WHITE : NL_TEXT, m_fGrid, DT_CENTER|DT_WORDBREAK);

            // Paint kit ID
            if (wd.skins[i].paintKit > 0) {
                char pk[24]; snprintf(pk, sizeof(pk), "#%d", wd.skins[i].paintKit);
                Txt(hdc, pk, cx + 8, cy + cardH - 20, cardW - 16, 14,
                    NL_TEXT3, m_fSmall, DT_LEFT|DT_SINGLELINE);
            }

            // Equipped badge
            if (eq) {
                Txt(hdc, "EQUIPPED", cx + 8, cy + cardH - 20, cardW - 16, 14,
                    NL_GREEN, m_fSmall, DT_RIGHT|DT_SINGLELINE);
            }

            if (m_gN < MG) { m_gR[m_gN] = {cx, cy, cardW, cardH}; m_gId[m_gN] = i; m_gN++; }
        }

        col++;
        if (col >= cols) { col = 0; py += cardH + GGAP; }
    }
}

// ============================================================================
//  Skin Detail  –  float slider + apply
// ============================================================================
void GameMenu::DrawDetail(HDC hdc, int x, int y, int w, int h) {
    if (m_selWeapon < 0 || m_selWeapon >= (int)m_weapons.size()) return;
    auto& wd = m_weapons[m_selWeapon];
    if (m_selSkin < 0 || m_selSkin >= (int)wd.skins.size()) return;

    int py = y + 20;
    int cw = w - PAD*2;

    // Large detail card
    int cardH = 190;
    RRect(hdc, x + PAD, py, cw, cardH, 10, NL_CARD, NL_BORDER);

    // Rarity bar
    COLORREF rc = RarityColor(m_selSkin);
    RECT tb = {x + PAD + 2, py + 2, x + PAD + cw - 2, py + 6};
    HBRUSH tbr = CreateSolidBrush(rc);
    FillRect(hdc, &tb, tbr); DeleteObject(tbr);

    // Weapon name (dim)
    Txt(hdc, wd.name.c_str(), x + PAD + 24, py + 16, cw - 48, 18,
        NL_TEXT2, m_fBody, DT_LEFT|DT_SINGLELINE);

    // Skin name (large)
    Txt(hdc, wd.skins[m_selSkin].name.c_str(), x + PAD + 24, py + 40, cw - 48, 28,
        NL_WHITE, m_fTitle, DT_LEFT|DT_SINGLELINE);

    // Wear name
    Txt(hdc, WearName(m_detailFloat), x + PAD + 24, py + 74, cw - 48, 20,
        NL_GREEN, m_fBold, DT_LEFT|DT_SINGLELINE);

    // Float value
    char fb[48]; snprintf(fb, sizeof(fb), "Float Value: %.6f", m_detailFloat);
    Txt(hdc, fb, x + PAD + 24, py + 98, cw - 48, 18,
        NL_TEXT, m_fBody, DT_LEFT|DT_SINGLELINE);

    // Paint kit
    if (wd.skins[m_selSkin].paintKit > 0) {
        char pk[48]; snprintf(pk, sizeof(pk), "Paint Kit ID: %d", wd.skins[m_selSkin].paintKit);
        Txt(hdc, pk, x + PAD + 24, py + 120, cw - 48, 18,
            NL_TEXT2, m_fBody, DT_LEFT|DT_SINGLELINE);
    }

    // Weapon ID
    char wid[48]; snprintf(wid, sizeof(wid), "Weapon ID: %d", wd.weaponId);
    Txt(hdc, wid, x + PAD + 24, py + 142, cw - 48, 18,
        NL_TEXT2, m_fBody, DT_LEFT|DT_SINGLELINE);

    py += cardH + 24;

    // ── Float Slider ──
    Txt(hdc, "Float Slider", x + PAD, py, cw, 18, NL_TEXT, m_fBold, DT_LEFT|DT_SINGLELINE);
    py += 24;

    // Labels
    Txt(hdc, "0.00", x + PAD, py - 2, 35, 14, NL_TEXT3, m_fSmall, DT_LEFT|DT_SINGLELINE);
    Txt(hdc, "1.00", x + PAD + cw - 35, py - 2, 35, 14, NL_TEXT3, m_fSmall, DT_RIGHT|DT_SINGLELINE);

    int sx = x + PAD + 40, sw = cw - 80, sy = py, sh = 20;

    // Track bg
    RRect(hdc, sx, sy + 6, sw, 8, 4, RGB(35,35,45), NL_BORDER);
    // Filled
    int fw = (int)(sw * m_detailFloat);
    if (fw > 3) RRect(hdc, sx, sy + 6, fw, 8, 4, NL_ACCENT, NL_ACCENT);
    // Knob
    int kx = sx + fw - 8; if (kx < sx) kx = sx;
    RRect(hdc, kx, sy + 2, 16, 16, 8, NL_WHITE, NL_ACCENT_L);

    m_sliderR = {sx, sy, sw, sh};

    // Wear zones
    py += 32;
    const char* zones[] = {"FN","MW","FT","WW","BS"};
    float ends[] = {0.07f, 0.15f, 0.38f, 0.45f, 1.0f};
    COLORREF zc[] = {NL_GREEN, RGB(120,200,70), NL_ORANGE, RGB(200,120,40), NL_RED};
    float zs = 0.f;
    for (int i = 0; i < 5; i++) {
        int zx = sx + (int)(sw * zs);
        int zw = (int)(sw * (ends[i] - zs));
        RRect(hdc, zx, py, zw, 16, 2, RGB(25,25,32), zc[i]);
        Txt(hdc, zones[i], zx, py, zw, 16, NL_WHITE, m_fSmall,
            DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        zs = ends[i];
    }

    py += 40;

    // Apply button
    int bw = 200, bh = 42;
    int bx = x + (w - bw) / 2;
    bool hov = Hit({bx, py, bw, bh}, m_mouseX, m_mouseY);
    Btn(hdc, bx, py, bw, bh, "Tilf\xF8j til Inventory", hov, NL_ACCENT, NL_ACCENT_L);
    m_applyBtn = {bx, py, bw, bh};
}

// ============================================================================
//  Configs  –  preset save/load/delete + apply to CS2
// ============================================================================
void GameMenu::DrawConfigs(HDC hdc, int x, int y, int w, int h) {
    int py = y + PAD;
    int cw = w - PAD*2;

    // Section: Current Inventory
    RRect(hdc, x + PAD, py, cw, 60, 8, NL_CARD, NL_BORDER);
    Txt(hdc, "Nuv\xE6rende Inventory", x + PAD + 16, py + 6, cw - 32, 18,
        NL_WHITE, m_fBold, DT_LEFT|DT_SINGLELINE);

    char countBuf[64];
    snprintf(countBuf, sizeof(countBuf), "%d skins tilf\xF8jet", (int)m_equipped.size());
    Txt(hdc, countBuf, x + PAD + 16, py + 28, cw - 200, 18,
        NL_TEXT2, m_fBody, DT_LEFT|DT_SINGLELINE);

    // Save button
    int sbw = 100, sbh = 28;
    int sbx = x + PAD + cw - sbw - 16, sby = py + 16;
    bool shov = Hit({sbx, sby, sbw, sbh}, m_mouseX, m_mouseY);
    Btn(hdc, sbx, sby, sbw, sbh, "Gem Preset", shov, NL_ACCENT, NL_ACCENT_L);
    m_saveBtn = {sbx, sby, sbw, sbh};

    py += 72;

    // Apply to CS2 button
    int abw = cw, abh = 42;
    int abx = x + PAD;
    bool ahov = Hit({abx, py, abw, abh}, m_mouseX, m_mouseY);
    RRect(hdc, abx, py, abw, abh, 8,
          ahov ? RGB(25,55,35) : RGB(18,45,28),
          NL_GREEN);
    Txt(hdc, "Anvend p\xE5 CS2 Inventory", abx, py, abw, abh,
        NL_GREEN, m_fBold, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    m_applyGameBtn = {abx, py, abw, abh};

    py += abh + 20;

    // Section: Saved Presets
    Txt(hdc, "GEMTE PRESETS", x + PAD, py, cw, 18, NL_TEXT3, m_fSmall, DT_LEFT|DT_SINGLELINE);
    py += 24;

    // Separator
    HPEN sep = CreatePen(PS_SOLID, 1, NL_BORDER);
    HPEN osep = (HPEN)SelectObject(hdc, sep);
    MoveToEx(hdc, x + PAD, py, nullptr);
    LineTo(hdc, x + PAD + cw, py);
    SelectObject(hdc, osep); DeleteObject(sep);
    py += 8;

    if (m_presets.empty()) {
        Txt(hdc, "Ingen gemte presets fundet.", x + PAD, py + 20, cw, 20,
            NL_TEXT2, m_fBody, DT_CENTER|DT_SINGLELINE);
        Txt(hdc, "Brug \"Gem Preset\" for at gemme dit nuv\xE6rende inventory.",
            x + PAD, py + 44, cw, 18, NL_TEXT3, m_fSmall, DT_CENTER|DT_SINGLELINE);
        return;
    }

    m_pN = 0;
    for (int i = 0; i < (int)m_presets.size() && i < MP; i++) {
        auto& p = m_presets[i];
        int ch = 56;
        bool hov = Hit({x + PAD, py, cw, ch}, m_mouseX, m_mouseY);

        RRect(hdc, x + PAD, py, cw, ch, 8, hov ? NL_CARD_HOV : NL_CARD, NL_BORDER);

        // Preset name + item count
        Txt(hdc, p.displayName.c_str(), x + PAD + 16, py + 8, cw - 200, 18,
            NL_WHITE, m_fBold, DT_LEFT|DT_SINGLELINE);

        char info[48]; snprintf(info, sizeof(info), "%d items", p.itemCount);
        Txt(hdc, info, x + PAD + 16, py + 28, cw - 200, 16,
            NL_TEXT2, m_fSmall, DT_LEFT|DT_SINGLELINE);

        // Load button
        int lbw = 60, lbh = 26;
        int lbx = x + PAD + cw - lbw - 80, lby = py + (ch - lbh) / 2;
        bool lhov = Hit({lbx, lby, lbw, lbh}, m_mouseX, m_mouseY);
        Btn(hdc, lbx, lby, lbw, lbh, "Load", lhov, NL_ACCENT, NL_ACCENT_L);
        m_pLoadR[m_pN] = {lbx, lby, lbw, lbh};

        // Delete button
        int dbw = 55, dbh = 26;
        int dbx = x + PAD + cw - dbw - 12, dby = py + (ch - dbh) / 2;
        bool dhov = Hit({dbx, dby, dbw, dbh}, m_mouseX, m_mouseY);
        RRect(hdc, dbx, dby, dbw, dbh, 6, dhov ? RGB(60,25,25) : RGB(40,18,18), NL_RED);
        Txt(hdc, "Slet", dbx, dby, dbw, dbh, NL_RED, m_fSmall,
            DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        m_pDelR[m_pN] = {dbx, dby, dbw, dbh};

        m_pN++;
        py += ch + 8;
    }
}

// ============================================================================
//  Preset Management  –  simple text files in presets/ folder
// ============================================================================
void GameMenu::ScanPresets() {
    m_presets.clear();
    _mkdir("presets");  // ensure folder exists

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA("presets\\*.acpreset", &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        PresetInfo pi;
        pi.filename = std::string("presets\\") + fd.cFileName;
        // Extract display name (filename without extension)
        pi.displayName = fd.cFileName;
        size_t dot = pi.displayName.rfind('.');
        if (dot != std::string::npos) pi.displayName = pi.displayName.substr(0, dot);

        // Count items in file
        pi.itemCount = 0;
        std::ifstream f(pi.filename);
        std::string line;
        while (std::getline(f, line))
            if (!line.empty() && line[0] != '#') pi.itemCount++;

        m_presets.push_back(pi);
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

void GameMenu::SavePreset() {
    _mkdir("presets");

    // Generate filename based on timestamp
    time_t now = time(nullptr);
    struct tm t; localtime_s(&t, &now);
    char fname[128];
    snprintf(fname, sizeof(fname), "presets\\Preset_%04d%02d%02d_%02d%02d%02d.acpreset",
             t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    std::ofstream f(fname);
    if (!f) return;

    f << "# AC Changer Preset\n";
    for (auto& eq : m_equipped) {
        f << eq.weapon << "|" << eq.skin << "|"
          << eq.weaponId << "|" << eq.paintKit << "|"
          << eq.floatVal << "\n";
    }
    f.close();

    DbgLog("Saved preset: %s (%d items)", fname, (int)m_equipped.size());
}

void GameMenu::LoadPreset(int idx) {
    if (idx < 0 || idx >= (int)m_presets.size()) return;

    std::ifstream f(m_presets[idx].filename);
    if (!f) return;

    m_equipped.clear();
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // Parse: weapon|skin|weaponId|paintKit|float
        std::istringstream ss(line);
        std::string weapon, skin, wid, pk, fv;
        std::getline(ss, weapon, '|');
        std::getline(ss, skin, '|');
        std::getline(ss, wid, '|');
        std::getline(ss, pk, '|');
        std::getline(ss, fv, '|');

        EquippedItem eq;
        eq.weapon = weapon;
        eq.skin = skin;
        eq.weaponId = wid.empty() ? 0 : std::stoi(wid);
        eq.paintKit = pk.empty() ? 0 : std::stoi(pk);
        eq.floatVal = fv.empty() ? 0.01f : std::stof(fv);
        m_equipped.push_back(eq);
    }

    DbgLog("Loaded preset: %s (%d items)", m_presets[idx].filename.c_str(), (int)m_equipped.size());
    m_page = PAGE_INVENTORY;
    m_view = INV_EQUIPPED;
    m_scroll = 0;
}

void GameMenu::DeletePreset(int idx) {
    if (idx < 0 || idx >= (int)m_presets.size()) return;
    DeleteFileA(m_presets[idx].filename.c_str());
    DbgLog("Deleted preset: %s", m_presets[idx].filename.c_str());
}


// ############################################################################
// ############################################################################
//  OverlayWindow  –  transparent layered window over CS2
// ############################################################################
// ############################################################################

OverlayWindow::OverlayWindow()
    : m_hwnd(nullptr), m_gameHwnd(nullptr), m_cs2Pid(0)
    , m_hModule(nullptr), m_running(false), m_autoShow(true) {}

OverlayWindow::~OverlayWindow() { Destroy(); }

// ── PID-based CS2 window finder ──
struct EnumCtx { DWORD pid; HWND result; };

static BOOL CALLBACK EnumCB(HWND hwnd, LPARAM lp) {
    auto* ctx = (EnumCtx*)lp;
    DWORD wPid = 0;
    GetWindowThreadProcessId(hwnd, &wPid);
    if (wPid != ctx->pid) return TRUE;
    if (!(GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE)) return TRUE;

    char title[128] = {};
    GetWindowTextA(hwnd, title, sizeof(title));
    if (strlen(title) == 0) return TRUE;

    RECT wr; GetWindowRect(hwnd, &wr);
    // Skip off-screen windows (Steam overlay helper, etc.)
    if (wr.left > 10000 || wr.top > 10000) return TRUE;
    if (wr.right - wr.left < 200 || wr.bottom - wr.top < 200) return TRUE;

    ctx->result = hwnd;
    return FALSE;
}

static HWND FindCS2Window(DWORD pid) {
    EnumCtx ctx = {pid, nullptr};
    EnumWindows(EnumCB, (LPARAM)&ctx);
    return ctx.result;
}

bool OverlayWindow::Create(HMODULE hModule) {
    return Create(hModule, 0);
}

bool OverlayWindow::Create(HMODULE hModule, DWORD cs2Pid) {
    m_hModule = hModule;
    m_cs2Pid = cs2Pid;

    DbgLog("OverlayWindow::Create  PID=%d", (int)cs2Pid);

    // Find CS2 window
    if (m_cs2Pid != 0) {
        for (int i = 0; i < 30 && !m_gameHwnd; i++) {
            m_gameHwnd = FindCS2Window(m_cs2Pid);
            if (!m_gameHwnd) { DbgLog("  attempt %d: not found, waiting...", i+1); Sleep(1000); }
        }
    }
    if (!m_gameHwnd) m_gameHwnd = FindWindowA(nullptr, "Counter-Strike 2");

    if (!m_gameHwnd) {
        DbgLog("ERROR: Could not find CS2 window");
        return false;
    }

    RECT wr; GetWindowRect(m_gameHwnd, &wr);
    DbgLog("CS2 window found: %p  rect=(%d,%d,%d,%d)",
           (void*)m_gameHwnd, wr.left, wr.top, wr.right, wr.bottom);

    // Register overlay window class
    WNDCLASSEXA wc = {sizeof(wc)};
    wc.lpfnWndProc   = OverlayWindow::WndProc;
    wc.hInstance      = (HINSTANCE)hModule;
    wc.lpszClassName  = "ACOverlay_NL";
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = NULL;
    RegisterClassExA(&wc);

    int w = wr.right - wr.left, h = wr.bottom - wr.top;
    m_hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        "ACOverlay_NL", "AC Overlay",
        WS_POPUP,
        wr.left, wr.top, w, h,
        NULL, NULL, (HINSTANCE)hModule, NULL);

    if (!m_hwnd) {
        DbgLog("ERROR: CreateWindowEx failed (%d)", (int)GetLastError());
        return false;
    }

    // Magenta color key for transparency
    SetLayeredWindowAttributes(m_hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(m_hwnd);

    m_menu.Initialize(hModule);
    m_running = true;

    DbgLog("Overlay created: hwnd=%p  size=%dx%d", (void*)m_hwnd, w, h);
    return true;
}

void OverlayWindow::Destroy() {
    m_running = false;
    if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
}

void OverlayWindow::ShowMenu() {
    DbgLog("ShowMenu() called");
    if (!m_menu.IsVisible()) m_menu.Toggle();

    LONG ex = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    ex &= ~WS_EX_TRANSPARENT;
    SetWindowLong(m_hwnd, GWL_EXSTYLE, ex);
    SetForegroundWindow(m_hwnd);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void OverlayWindow::UpdatePosition() {
    if (!m_gameHwnd || !IsWindow(m_gameHwnd)) {
        if (m_cs2Pid) m_gameHwnd = FindCS2Window(m_cs2Pid);
        if (!m_gameHwnd) return;
    }
    RECT wr; GetWindowRect(m_gameHwnd, &wr);
    if (wr.left > 10000) return;
    int w = wr.right - wr.left, h = wr.bottom - wr.top;
    SetWindowPos(m_hwnd, HWND_TOPMOST, wr.left, wr.top, w, h,
                 SWP_NOACTIVATE);
}

void OverlayWindow::RunFrame() {
    if (!m_running || !m_hwnd) return;

    UpdatePosition();

    // Auto-show on first frame
    if (m_autoShow) {
        m_autoShow = false;
        ShowMenu();
    }

    // ── INSERT key toggle  (ONLY when CS2 is the foreground window) ──
    static bool keyWasDown = false;
    bool cs2Focused = false;

    if (m_cs2Pid != 0) {
        HWND fg = GetForegroundWindow();
        DWORD fgPid = 0;
        GetWindowThreadProcessId(fg, &fgPid);
        cs2Focused = (fgPid == m_cs2Pid);
    } else {
        // Fallback: check if CS2 window or our overlay is foreground
        HWND fg = GetForegroundWindow();
        cs2Focused = (fg == m_gameHwnd || fg == m_hwnd);
    }

    if (cs2Focused) {
        bool keyDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (keyDown && !keyWasDown) {
            if (m_menu.IsVisible()) {
                m_menu.Toggle();
                LONG ex = GetWindowLong(m_hwnd, GWL_EXSTYLE);
                ex |= WS_EX_TRANSPARENT;
                SetWindowLong(m_hwnd, GWL_EXSTYLE, ex);
            } else {
                ShowMenu();
            }
        }
        keyWasDown = keyDown;
    } else {
        keyWasDown = false;
    }

    m_menu.ProcessInput();
    InvalidateRect(m_hwnd, nullptr, TRUE);
    UpdateWindow(m_hwnd);

    MSG msg;
    while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK OverlayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(mem, bmp);

        // Fill with magenta (transparent via color key)
        HBRUSH mgBr = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(mem, &rc, mgBr);
        DeleteObject(mgBr);

        if (g_pOverlay) {
            g_pOverlay->m_menu.Render(mem, rc.right, rc.bottom);
        }

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
        if (g_pOverlay) {
            g_pOverlay->m_menu.OnMouseMove((short)LOWORD(lp), (short)HIWORD(lp));
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (g_pOverlay) {
            g_pOverlay->m_menu.OnMouseClick((short)LOWORD(lp), (short)HIWORD(lp));
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (g_pOverlay) g_pOverlay->m_menu.OnMouseUp();
        return 0;

    case WM_MOUSEWHEEL:
        if (g_pOverlay) {
            g_pOverlay->m_menu.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wp));
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_ERASEBKGND: return 1;
    case WM_DESTROY:    return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}


// ############################################################################
//  GameInjector
// ############################################################################
GameInjector::GameInjector() : m_initialized(false) {}

GameInjector::~GameInjector() { Shutdown(); }

bool GameInjector::Initialize(HMODULE hModule) {
    if (m_initialized) return true;

    m_overlay = std::make_unique<OverlayWindow>();
    g_pOverlay = m_overlay.get();

    if (!m_overlay->Create(hModule)) {
        DbgLog("GameInjector: overlay creation failed");
        m_overlay.reset();
        g_pOverlay = nullptr;
        return false;
    }

    m_initialized = true;
    DbgLog("GameInjector initialized");
    return true;
}

void GameInjector::Shutdown() {
    if (m_overlay) {
        m_overlay->Destroy();
        m_overlay.reset();
        g_pOverlay = nullptr;
    }
    m_initialized = false;
}

void GameInjector::Update() {
    if (m_overlay) m_overlay->RunFrame();
}
