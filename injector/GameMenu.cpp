// ============================================================================
//  CS2 In-Game Overlay Menu  –  NEVERLOSE-style
//  INSERT toggles.  Grid skin browser.  Float slider.
// ============================================================================
#include "GameMenu.h"
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>

#pragma comment(lib, "gdi32.lib")

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
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f, "\n");
    fclose(f);
}
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")

static OverlayWindow* g_pOverlay = nullptr;

// ============================================================================
//  GameMenu
// ============================================================================

GameMenu::GameMenu()
    : m_visible(false)
    , m_activePage(PAGE_INVENTORY)
    , m_invView(INV_EQUIPPED)
    , m_scrollOffset(0)
    , m_mouseX(0), m_mouseY(0)
    , m_menuX(0), m_menuY(0)
    , m_selWeapon(-1), m_selSkin(-1)
    , m_detailFloat(0.01f)
    , m_skinChangerEnabled(true)
    , m_knifeChangerEnabled(false)
    , m_gloveChangerEnabled(false)
    , m_gridCount(0)
    , m_sliderDrag(false)
    , m_invRemoveCount(0)
    , m_toggleCount(0)
    , m_fontTitle(nullptr), m_fontBody(nullptr)
    , m_fontSmall(nullptr), m_fontBold(nullptr)
    , m_fontLogo(nullptr), m_fontGrid(nullptr) {
    memset(m_gridRects, 0, sizeof(m_gridRects));
    memset(m_sidebarRects, 0, sizeof(m_sidebarRects));
    memset(&m_addBtnRect, 0, sizeof(m_addBtnRect));
    memset(&m_backBtnRect, 0, sizeof(m_backBtnRect));
    memset(&m_applyBtnRect, 0, sizeof(m_applyBtnRect));
    memset(&m_sliderRect, 0, sizeof(m_sliderRect));
}

GameMenu::~GameMenu() {
    if (m_fontTitle) DeleteObject(m_fontTitle);
    if (m_fontBody)  DeleteObject(m_fontBody);
    if (m_fontSmall) DeleteObject(m_fontSmall);
    if (m_fontBold)  DeleteObject(m_fontBold);
    if (m_fontLogo)  DeleteObject(m_fontLogo);
    if (m_fontGrid)  DeleteObject(m_fontGrid);
}

bool GameMenu::Initialize(HMODULE) {
    m_fontTitle = CreateFontA(22, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    m_fontBody  = CreateFontA(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    m_fontSmall = CreateFontA(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    m_fontBold  = CreateFontA(15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    m_fontLogo  = CreateFontA(20, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    m_fontGrid  = CreateFontA(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    BuildDatabase();
    return true;
}

void GameMenu::Shutdown() { m_visible = false; }

void GameMenu::Toggle() {
    m_visible = !m_visible;
    if (m_visible) { m_scrollOffset = 0; }
}

void GameMenu::BuildDatabase() {
    // Pistols
    m_weaponDB.push_back({"Glock-18", {"Fade", "Gamma Doppler", "Wasteland Rebel", "Water Elemental", "Twilight Galaxy",
        "Bullet Queen", "Vogue", "Neo-Noir", "Moonrise", "Off World", "Weasel", "Royal Legion", "Grinder", "Steel Disruption",
        "Oxide Blaze", "Wraiths", "Sacrifice", "Franklin", "Synth Leaf", "Snack Attack", "Clear Polymer", "Nuclear Garden",
        "High Beam", "Umbral Rabbit", "Pink DDPAT", "Sand Dune"}});

    m_weaponDB.push_back({"USP-S", {"Kill Confirmed", "Neo-Noir", "Printstream", "Caiman", "Cortex", "Monster Mashup",
        "Blueprint", "Orion", "The Traitor", "Check Engine", "Flashback", "Serum", "Cyrex", "Guardian", "Stainless",
        "Ticket to Hell", "Whiteout", "Road Rash", "Dark Water", "Overgrowth", "Night Ops", "Target Acquired",
        "Purple DDPAT", "Pathfinder", "Ancient Visions", "Black Lotus"}});

    m_weaponDB.push_back({"P250", {"Asiimov", "See Ya Later", "Vino Primo", "Nevermore", "Undertow", "Muertos",
        "Mehndi", "Supernova", "Nuclear Threat", "Cartel", "Wingshot", "Splash", "Exchanger", "Ripple Effect",
        "Re.built", "Cassette", "Inferno", "Iron Clad", "Contamination"}});

    m_weaponDB.push_back({"Desert Eagle", {"Blaze", "Printstream", "Code Red", "Mecha Industries", "Kumicho Dragon",
        "Conspiracy", "Sunset Storm", "Trigger Discipline", "Ocean Drive", "Fennec Fox", "Light Rail", "Sputnik",
        "Bronze Deco", "Midnight Storm", "Night", "Cobalt Disruption", "Emerald Jormungandr", "Golden Koi",
        "Hypnotic", "Heirloom", "Hand Cannon", "Oxide Blaze"}});

    m_weaponDB.push_back({"Five-SeveN", {"Hyper Beast", "Monkey Business", "Angry Mob", "Neon Kimono", "Case Hardened",
        "Fowl Play", "Berries And Cherries", "Fairy Tale", "Scrawl", "Flame Test", "Fall Hazard", "Triumvirate",
        "Retrobution", "Copper Galaxy", "Nightshade", "Kami", "Silver Quartz"}});

    m_weaponDB.push_back({"Tec-9", {"Fuel Injector", "Decimator", "Nuclear Threat", "Re-Entry", "Bamboozle",
        "Remote Control", "Terrace", "Toxic", "Titanium Bit", "Isaac", "Avalanche", "Red Quartz", "Ice Cap",
        "Snek-9", "Flash Out", "Brother", "Fubar"}});

    m_weaponDB.push_back({"Dual Berettas", {"Cobalt Quartz", "Twin Turbo", "Melondrama", "Flora Carnivora",
        "Dezastre", "Royal Consorts", "Urban Shock", "Hemoglobin", "Dualing Dragons", "Shred", "Elite 1.6",
        "Stained", "Marina", "Balance", "Ventilators", "Cartel", "Hideout"}});

    m_weaponDB.push_back({"P2000", {"Fire Elemental", "Imperial Dragon", "Amber Fade", "Ocean Foam", "Corticera",
        "Imperial", "Handgun", "Scorpion", "Gnarled", "Granite Marbleized", "Pathfinder", "Obsidian",
        "Woven Dark", "Turf", "Oceanic", "Silver", "Grassland Leaves"}});

    m_weaponDB.push_back({"CZ75-Auto", {"Victoria", "Xiangliu", "Emerald Quartz", "Vendetta", "Eco", "Tacticat",
        "The Fuschia Is Now", "Trigger Discipline", "Poison Dart", "Red Astor", "Distressed", "Polymer",
        "Circaetus", "Chalice", "Nitro", "Yellow Jacket"}});

    m_weaponDB.push_back({"R8 Revolver", {"Fade", "Banana Cannon", "Llama Cannon", "Crimson Web", "Amber Fade",
        "Memento", "Grip", "Skull Crusher", "Nitro", "Bone Mask", "Canal Spray", "Survival Galil"}});

    // Rifles
    m_weaponDB.push_back({"AK-47", {"Wild Lotus", "Fire Serpent", "Vulcan", "Neon Rider", "Redline", "Asiimov",
        "The Empress", "Bloodsport", "Neon Revolution", "Fuel Injector", "Aquamarine Revenge", "Phantom Disruptor",
        "Jaguar", "Frontside Misty", "Point Disarray", "Wasteland Rebel", "Hydroponic", "Case Hardened",
        "Ice Coaled", "Nightwish", "Slate", "X-Ray", "Rat Rod", "Orbit Mk01", "Head Shot", "Legion of Anubis",
        "Jet Set", "Leet Museo", "Blue Laminate", "Safety Net", "Inheritance"}});

    m_weaponDB.push_back({"M4A4", {"Howl", "Asiimov", "Neo-Noir", "Desolate Space", "The Emperor", "Buzz Kill",
        "Royal Paladin", "Dragon King", "Spider Lily", "In Living Color", "Tooth Fairy", "The Coalition",
        "Temukau", "Cyber Security", "Radiation Hazard", "Desert-Strike", "X-Ray", "Mainframe",
        "Daybreak", "Poly Mag", "Eye of Horus", "Global Offensive"}});

    m_weaponDB.push_back({"M4A1-S", {"Printstream", "Hyper Beast", "Chantico's Fire", "Golden Coil", "Mecha Industries",
        "Decimator", "Player Two", "Leaded Glass", "Night Terror", "Nightmare", "Master Piece", "Hot Rod",
        "Icarus Fell", "Cyrex", "Guardian", "Atomic Alloy", "Knight", "Welcome to the Jungle", "Blue Phosphor",
        "Emphorosaur-S", "Control Panel", "Imminent Danger", "Basilisk", "Fizzy POP"}});

    m_weaponDB.push_back({"AWP", {"Dragon Lore", "Gungnir", "Medusa", "Fade", "Asiimov", "Lightning Strike",
        "Hyper Beast", "Containment Breach", "The Prince", "Wildfire", "Neo-Noir", "Chromatic Aberration",
        "Electric Hive", "Redline", "BOOM", "Corticera", "Man-o'-war", "Fever Dream", "Atheris",
        "PAW", "Silk Tiger", "Phobos", "Sun in Leo", "Duality", "Exoskeleton", "Pit Viper",
        "Worm God", "Mortis", "Graphite", "Desert Hydra", "Capillary"}});

    m_weaponDB.push_back({"SG 553", {"Integrale", "Darkwing", "Cyrex", "Pulse", "Bulldozer", "Tiger Moth",
        "Phantom", "Danger Close", "Atlas", "Triarch", "Aloha", "Barricade", "Aerial", "Heavy Metal",
        "Colony IV", "Ultraviolet", "Tornado", "Anodized Navy", "Wave Spray"}});

    m_weaponDB.push_back({"AUG", {"Akihabara Accept", "Chameleon", "Flame Jinn", "Stymphalian", "Midnight Lily",
        "Momentum", "Fleet Flock", "Syd Mead", "Hot Rod", "Bengal Tiger", "Arctic Wolf", "Random Access",
        "Aristocrat", "Amber Slipstream", "Torque", "Wings", "Triqua", "Carved Jade"}});

    m_weaponDB.push_back({"FAMAS", {"Eye of Athena", "Mecha Industries", "Roll Cage", "Commemoration",
        "Valence", "Neural Net", "Djinn", "Pulse", "Afterimage", "Decommissioned", "Step Ahead",
        "Prime Conspiracy", "Sundown", "Sergeant", "Teardown", "Cyanospatter", "Macabre"}});

    m_weaponDB.push_back({"Galil AR", {"Chatterbox", "Cerberus", "Aqua Terrace", "Firefight", "Sugar Rush",
        "Eco", "Crimson Tsunami", "Chromatic Aberration", "Stone Cold", "Rocket Pop", "Signal",
        "Connexion", "Akoben", "Dusk Ruins", "Winter Forest", "Orange DDPAT", "Sage Spray"}});

    // SMGs
    m_weaponDB.push_back({"MAC-10", {"Neon Rider", "Stalker", "Case Hardened", "Disco Tech", "Toybox",
        "Heat", "Pipe Down", "Last Dive", "Propaganda", "Graven", "Leet", "Button Masher",
        "Fade", "Copper Borre", "Whitefish", "Carnivore", "Classic Crate", "Allure", "Silver",
        "Light Box", "Nuclear Garden", "Tatter", "Curse"}});

    m_weaponDB.push_back({"MP9", {"Wild Lily", "Hydra", "Bulldozer", "Airlock", "Modest Threat",
        "Rose Iron", "Music Box", "Stained Glass", "Capillary", "Setting Sun", "Bioleak",
        "Goo", "Food Chain", "Slide", "Ruby Poison Dart", "Mount Fuji", "Featherweight",
        "Old Roots", "Black Sand", "Dark Age", "Pandora's Box"}});

    m_weaponDB.push_back({"UMP-45", {"Primal Saber", "Moonrise", "Fade", "Momentum", "Arctic Wolf",
        "Plastique", "Day Lily", "Crime Scene", "Oscillator", "Wild Child", "Exposure",
        "Riot", "Scaffold", "Metal Flowers", "Corporal", "Labyrinth", "Bone Pile"}});

    m_weaponDB.push_back({"MP7", {"Nemesis", "Bloodsport", "Neon Ply", "Impire", "Special Delivery",
        "Mischief", "Powercore", "Fade", "Skulls", "Teal Blossom", "Akoben", "Abyssal Apparition",
        "Cirrus", "Armor Core", "Urban Hazard", "Asteria", "Motherboard", "Whiteout"}});

    m_weaponDB.push_back({"PP-Bizon", {"Judgement of Anubis", "High Roller", "Embargo", "Blue Streak",
        "Fuel Rod", "Antique", "Jungle Slipstream", "Seabird", "Night Riot", "Harvester",
        "Osiris", "Carbon Fiber", "Photic Zone", "Space Cat", "Rust Coat"}});

    m_weaponDB.push_back({"P90", {"Asiimov", "Death by Kitty", "Nostalgia", "Shapewood", "Trigon",
        "Emerald Dragon", "Cold Blooded", "Run and Hide", "Shallow Grave", "Freight",
        "Blind Spot", "Baroque Red", "Off World", "Traction", "Facility Dark", "Cocoa Rampage",
        "Vent Rush", "Virus", "Sunset Lily", "Elite Build", "Desert Warfare"}});

    m_weaponDB.push_back({"MP5-SD", {"Lab Rats", "Phosphor", "Agent", "Gauss", "Oxide Oasis",
        "Co-Processor", "Acid Wash", "Condition Zero", "Kitbash", "Bamboo Garden", "Dirt Drop"}});

    // Shotguns
    m_weaponDB.push_back({"Nova", {"Hyper Beast", "Antique", "Wild Six", "Gila", "Toy Soldier",
        "Exo", "Baroque Orange", "Koi", "Graphite", "Bloomstick", "Plume", "Dark Sigil",
        "Sand Dune", "Moon in Libra", "Clear Polymer", "Caged Steel", "Mandrel"}});

    m_weaponDB.push_back({"XM1014", {"Incinegator", "Ziggy", "Tranquility", "Entombed", "Bone Machine",
        "Seasons", "Teclu Burner", "Black Tie", "Ancient Lore", "CaliCamo", "Blue Steel", "Oxide Blaze"}});

    m_weaponDB.push_back({"Sawed-Off", {"The Kraken", "Apocalypto", "Devourer", "Limelight", "Origami",
        "Wasteland Princess", "Morris", "Copper", "Yorick", "Black Sand", "Brake Light", "Amber Fade"}});

    m_weaponDB.push_back({"MAG-7", {"Justice", "SWAG-7", "Heat", "Prism Terrace", "Monster Call",
        "Petroglyph", "Cinquedea", "Bulldozer", "Sonar", "Popdog", "Carbon Fiber", "Hard Water"}});

    // Machine Guns
    m_weaponDB.push_back({"M249", {"Magma", "Nebula Crusader", "Spectre", "Emerald Poison Dart",
        "Impact Drill", "System Lock", "Shipping Forecast", "Downtown", "Jungle DDPAT"}});

    m_weaponDB.push_back({"Negev", {"Power Loader", "Loudmouth", "Bratatat", "Prototype",
        "Lionfish", "Dazzle", "Bulkhead", "Dev Texture", "Terrain", "Drop Me", "Palm"}});

    // Knives
    m_weaponDB.push_back({"Karambit", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Gamma Doppler", "Bright Water",
        "Freehand", "Black Laminate", "Vanilla", "Damascus Steel", "Ultraviolet", "Night Stripe",
        "Rust Coat", "Blue Steel", "Boreal Forest", "Stained", "Forest DDPAT", "Safari Mesh",
        "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Butterfly Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Gamma Doppler", "Bright Water",
        "Freehand", "Black Laminate", "Vanilla", "Damascus Steel", "Ultraviolet", "Night Stripe",
        "Rust Coat", "Blue Steel", "Boreal Forest", "Stained", "Forest DDPAT", "Safari Mesh",
        "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"M9 Bayonet", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Gamma Doppler", "Bright Water",
        "Freehand", "Black Laminate", "Vanilla", "Damascus Steel", "Ultraviolet", "Night Stripe",
        "Rust Coat", "Blue Steel", "Boreal Forest", "Stained", "Forest DDPAT", "Safari Mesh"}});

    m_weaponDB.push_back({"Bayonet", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Gamma Doppler", "Vanilla",
        "Damascus Steel", "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel",
        "Boreal Forest", "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Skeleton Knife", {"Fade", "Crimson Web", "Slaughter", "Case Hardened",
        "Blue Steel", "Stained", "Night Stripe", "Boreal Forest", "Forest DDPAT",
        "Safari Mesh", "Urban Masked", "Scorched", "Vanilla"}});

    m_weaponDB.push_back({"Talon Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Vanilla", "Damascus Steel",
        "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel", "Boreal Forest",
        "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Stiletto Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Vanilla", "Damascus Steel", "Ultraviolet",
        "Night Stripe", "Rust Coat", "Blue Steel", "Boreal Forest", "Stained",
        "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Ursus Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Vanilla", "Damascus Steel", "Ultraviolet",
        "Night Stripe", "Rust Coat", "Blue Steel", "Boreal Forest", "Stained",
        "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Navaja Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Vanilla", "Damascus Steel", "Ultraviolet",
        "Night Stripe", "Rust Coat", "Blue Steel", "Boreal Forest", "Stained",
        "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Huntsman Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Vanilla", "Damascus Steel",
        "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel", "Boreal Forest",
        "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Bowie Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Vanilla", "Damascus Steel",
        "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel", "Boreal Forest",
        "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Falchion Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Vanilla", "Damascus Steel",
        "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel", "Boreal Forest",
        "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Flip Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Gamma Doppler", "Vanilla",
        "Damascus Steel", "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel",
        "Boreal Forest", "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Gut Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Gamma Doppler", "Vanilla",
        "Damascus Steel", "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel",
        "Boreal Forest", "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Classic Knife", {"Fade", "Crimson Web", "Slaughter", "Case Hardened",
        "Blue Steel", "Vanilla", "Boreal Forest", "Forest DDPAT", "Safari Mesh",
        "Urban Masked", "Night Stripe", "Scorched", "Stained"}});

    m_weaponDB.push_back({"Paracord Knife", {"Fade", "Crimson Web", "Slaughter", "Case Hardened",
        "Blue Steel", "Stained", "Vanilla", "Night Stripe", "Boreal Forest", "Forest DDPAT",
        "Safari Mesh", "Urban Masked", "Scorched"}});

    m_weaponDB.push_back({"Survival Knife", {"Fade", "Crimson Web", "Slaughter", "Case Hardened",
        "Blue Steel", "Vanilla", "Night Stripe", "Boreal Forest", "Forest DDPAT",
        "Safari Mesh", "Urban Masked", "Scorched", "Stained"}});

    m_weaponDB.push_back({"Nomad Knife", {"Fade", "Crimson Web", "Slaughter", "Case Hardened",
        "Blue Steel", "Vanilla", "Night Stripe", "Boreal Forest", "Forest DDPAT",
        "Safari Mesh", "Urban Masked", "Scorched", "Stained"}});

    m_weaponDB.push_back({"Kukri Knife", {"Doppler", "Fade", "Tiger Tooth", "Marble Fade", "Crimson Web",
        "Slaughter", "Case Hardened", "Lore", "Autotronic", "Gamma Doppler", "Vanilla",
        "Damascus Steel", "Ultraviolet", "Night Stripe", "Rust Coat", "Blue Steel",
        "Boreal Forest", "Stained", "Forest DDPAT", "Safari Mesh", "Urban Masked", "Scorched"}});

    // Gloves
    m_weaponDB.push_back({"Sport Gloves", {"Pandora's Box", "Hedge Maze", "Superconductor", "Vice",
        "Amphibious", "Bronze Morph", "Omega", "Arid", "Scarlet Shamagh", "Slingshot",
        "Nocts", "Big Game"}});

    m_weaponDB.push_back({"Specialist Gloves", {"Crimson Kimono", "Emerald Web", "Fade", "Mogul",
        "Foundation", "Marble Fade", "Field Agent", "Forest DDPAT", "Buckshot",
        "Tiger Strike", "Lt. Commander", "Cool Mint"}});

    m_weaponDB.push_back({"Driver Gloves", {"King Snake", "Imperial Plaid", "Crimson Weave", "Lunar Weave",
        "Diamondback", "Overtake", "Racing Green", "Convoy", "Queen Jaguar", "Rezan the Red",
        "Snow Leopard", "Black Tie"}});

    m_weaponDB.push_back({"Hand Wraps", {"Cobalt Skulls", "Overprint", "Slaughter", "Leather",
        "Badlands", "Spruce DDPAT", "Duct Tape", "Arboreal", "Giraffe", "Cashmere",
        "Constrictor", "Desert Shamagh"}});

    m_weaponDB.push_back({"Moto Gloves", {"Spearmint", "Cool Mint", "Polygon", "Boom!",
        "Eclipse", "Transport", "Turtle", "POW!", "Finish Line", "Blood Pressure",
        "Smoke Out", "Snakebite"}});

    m_weaponDB.push_back({"Hydra Gloves", {"Case Hardened", "Emerald", "Rattler", "Mangrove",
        "Circuit Board"}});

    m_weaponDB.push_back({"Broken Fang Gloves", {"Jade", "Yellow-banded", "Unhinged", "Needle Point",
        "Finisher"}});

    // Snipers/MGs
    m_weaponDB.push_back({"SSG 08", {"Dragonfire", "Blood in the Water", "Big Iron", "Death Strike",
        "Fever Dream", "Necropos", "Ghost Crusader", "Abyss", "Dark Water", "Detour",
        "Turbo Peek", "Sea Calico", "Hand Brake", "Acid Fade", "Blue Spruce"}});

    m_weaponDB.push_back({"SCAR-20", {"Emerald", "Cardiac", "Cyrex", "Bloodsport", "Jungle Slipstream",
        "Enforcer", "Assault", "Blueprint", "Torn", "Poultry Manager", "Stone Mosaico"}});

    m_weaponDB.push_back({"G3SG1", {"The Executioner", "Flux", "Scavenger", "Stinger", "Chronos",
        "High Seas", "Ventilator", "Demeter", "Black Sand", "Orange Crash", "Polar Camo"}});
}

// ────────────────────────────────────────────────
//  Mouse
// ────────────────────────────────────────────────
void GameMenu::OnMouseMove(int x, int y) {
    m_mouseX = x;
    m_mouseY = y;
    // Slider dragging
    if (m_sliderDrag && m_invView == INV_DETAIL) {
        int sx = m_sliderRect.x;
        int sw = m_sliderRect.w;
        float t = (float)(x - sx) / (float)sw;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        m_detailFloat = t;
    }
}

void GameMenu::OnMouseClick(int x, int y) {
    if (!m_visible) return;

    // Sidebar
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (HitTest(m_sidebarRects[i], x, y)) {
            m_activePage = (Page)i;
            m_scrollOffset = 0;
            if (i == PAGE_INVENTORY) m_invView = INV_EQUIPPED;
            return;
        }
    }

    if (m_activePage == PAGE_INVENTORY) {
        // "Tilf\xF8j Skin" button
        if (HitTest(m_addBtnRect, x, y)) {
            m_invView = INV_WEAPONS;
            m_scrollOffset = 0;
            return;
        }
        // Back button
        if (HitTest(m_backBtnRect, x, y)) {
            if (m_invView == INV_DETAIL)      { m_invView = INV_SKINS; m_scrollOffset = 0; }
            else if (m_invView == INV_SKINS)  { m_invView = INV_WEAPONS; m_scrollOffset = 0; }
            else if (m_invView == INV_WEAPONS){ m_invView = INV_EQUIPPED; m_scrollOffset = 0; }
            return;
        }
        // Apply / Tilf\xF8j button in detail view
        if (m_invView == INV_DETAIL && HitTest(m_applyBtnRect, x, y)) {
            if (m_selWeapon >= 0 && m_selWeapon < (int)m_weaponDB.size()) {
                auto& wd = m_weaponDB[m_selWeapon];
                if (m_selSkin >= 0 && m_selSkin < (int)wd.skins.size()) {
                    // Check if already equipped, if so update float
                    bool found = false;
                    for (auto& eq : m_equipped) {
                        if (eq.weapon == wd.name && eq.skin == wd.skins[m_selSkin]) {
                            eq.floatVal = m_detailFloat;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        m_equipped.push_back({ wd.name, wd.skins[m_selSkin], m_detailFloat });
                    }
                    m_invView = INV_EQUIPPED;
                    m_scrollOffset = 0;
                }
            }
            return;
        }
        // Float slider click → start drag
        if (m_invView == INV_DETAIL && HitTest(m_sliderRect, x, y)) {
            m_sliderDrag = true;
            float t = (float)(x - m_sliderRect.x) / (float)m_sliderRect.w;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            m_detailFloat = t;
            return;
        }
        // Grid clicks (weapons or skins)
        for (int i = 0; i < m_gridCount; i++) {
            if (HitTest(m_gridRects[i], x, y)) {
                if (m_invView == INV_WEAPONS) {
                    m_selWeapon = m_gridIds[i];
                    m_invView = INV_SKINS;
                    m_scrollOffset = 0;
                } else if (m_invView == INV_SKINS) {
                    m_selSkin = m_gridIds[i];
                    m_detailFloat = 0.01f;
                    m_invView = INV_DETAIL;
                    m_scrollOffset = 0;
                }
                return;
            }
        }
        // Remove buttons in equipped view
        for (int i = 0; i < m_invRemoveCount; i++) {
            if (HitTest(m_invRemoveRects[i], x, y)) {
                if (i < (int)m_equipped.size()) {
                    m_equipped.erase(m_equipped.begin() + i);
                }
                return;
            }
        }
    }

    if (m_activePage == PAGE_CONFIGS) {
        for (int i = 0; i < m_toggleCount; i++) {
            if (HitTest(m_toggleRects[i], x, y)) {
                bool* vals[] = { &m_skinChangerEnabled, &m_knifeChangerEnabled, &m_gloveChangerEnabled };
                if (i < 3) *vals[i] = !*vals[i];
                return;
            }
        }
    }
}

void GameMenu::OnMouseWheel(int delta) {
    if (!m_visible) return;
    m_scrollOffset -= delta / 40;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
}

void GameMenu::OnMouseUp() {
    m_sliderDrag = false;
}

// ────────────────────────────────────────────────
//  Drawing helpers
// ────────────────────────────────────────────────
void GameMenu::DrawRoundRect(HDC hdc, int x, int y, int w, int h, int r,
                             COLORREF fill, COLORREF border) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID, 1, border);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN   op = (HPEN)SelectObject(hdc, pn);
    RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
    DeleteObject(pn);
}

void GameMenu::DrawText_(HDC hdc, const char* text, int x, int y, int w, int h,
                         COLORREF col, HFONT font, UINT fmt) {
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, col);
    SetBkMode(hdc, TRANSPARENT);
    RECT rc = { x, y, x + w, y + h };
    DrawTextA(hdc, text, -1, &rc, fmt);
    SelectObject(hdc, old);
}

void GameMenu::DrawButton(HDC hdc, int x, int y, int w, int h, const char* text,
                          bool hov, COLORREF bg, COLORREF bgHov) {
    DrawRoundRect(hdc, x, y, w, h, 6, hov ? bgHov : bg, hov ? MENU_ACCENT_L : MENU_ACCENT);
    DrawText_(hdc, text, x, y, w, h, MENU_WHITE, m_fontBold, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ────────────────────────────────────────────────
//  Render
// ────────────────────────────────────────────────
void GameMenu::Render(HDC hdc, int winW, int winH) {
    if (!m_visible) return;

    int mx = (winW - MENU_WIDTH) / 2;
    int my = (winH - MENU_HEIGHT) / 2;
    m_menuX = mx;  m_menuY = my;
    m_gridCount = 0;
    m_invRemoveCount = 0;
    m_toggleCount = 0;

    // Shadow + background
    DrawRoundRect(hdc, mx + 5, my + 5, MENU_WIDTH, MENU_HEIGHT, MENU_RAD, RGB(2, 2, 4), RGB(2, 2, 4));
    DrawRoundRect(hdc, mx, my, MENU_WIDTH, MENU_HEIGHT, MENU_RAD, MENU_BG, MENU_BORDER);

    // Sidebar
    DrawSidebar(hdc, mx, my, MENU_HEIGHT);

    // Content
    int cx = mx + MENU_SIDEBAR_W;
    int cw = MENU_WIDTH - MENU_SIDEBAR_W;

    // Header
    DrawHeader(hdc, cx, my, cw);

    // Content body
    int contentY = my + MENU_HEADER_H;
    int contentH = MENU_HEIGHT - MENU_HEADER_H;

    HRGN clip = CreateRectRgn(cx, contentY, cx + cw, my + MENU_HEIGHT);
    SelectClipRgn(hdc, clip);

    if (m_activePage == PAGE_INVENTORY) {
        switch (m_invView) {
        case INV_EQUIPPED: DrawEquipped(hdc, cx, contentY, cw, contentH); break;
        case INV_WEAPONS:  DrawWeaponGrid(hdc, cx, contentY, cw, contentH); break;
        case INV_SKINS:    DrawSkinGrid(hdc, cx, contentY, cw, contentH); break;
        case INV_DETAIL:   DrawSkinDetail(hdc, cx, contentY, cw, contentH); break;
        }
    } else {
        DrawConfigs(hdc, cx, contentY, cw, contentH);
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);
}

void GameMenu::DrawSidebar(HDC hdc, int x, int y, int h) {
    RECT sbr = { x, y, x + MENU_SIDEBAR_W, y + h };
    HBRUSH sbb = CreateSolidBrush(MENU_SIDEBAR_C);
    FillRect(hdc, &sbr, sbb);
    DeleteObject(sbb);

    // Right border
    HPEN bp = CreatePen(PS_SOLID, 1, MENU_SIDEBAR_BR);
    HPEN obp = (HPEN)SelectObject(hdc, bp);
    MoveToEx(hdc, x + MENU_SIDEBAR_W - 1, y, nullptr);
    LineTo(hdc, x + MENU_SIDEBAR_W - 1, y + h);
    SelectObject(hdc, obp);
    DeleteObject(bp);

    // Logo
    DrawText_(hdc, "NEVERLOSE", x + 14, y + 14, MENU_SIDEBAR_W - 28, 28,
              MENU_WHITE, m_fontLogo, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Separator
    HPEN sp = CreatePen(PS_SOLID, 1, MENU_SIDEBAR_BR);
    HPEN osp = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, x + 14, y + 50, nullptr);
    LineTo(hdc, x + MENU_SIDEBAR_W - 14, y + 50);
    SelectObject(hdc, osp);
    DeleteObject(sp);

    // Nav items
    const char* labels[PAGE_COUNT] = { "Inventory Changer", "Configs" };
    const char* icons[PAGE_COUNT] = { "\xB7", "\xB7" };
    int navY = y + 62;

    for (int i = 0; i < PAGE_COUNT; i++) {
        bool sel = ((int)m_activePage == i);
        bool hov = HitTest({x + 8, navY, MENU_SIDEBAR_W - 16, 34}, m_mouseX, m_mouseY);

        if (sel) {
            DrawRoundRect(hdc, x + 8, navY, MENU_SIDEBAR_W - 16, 34, 6,
                          RGB(30, 50, 80), MENU_ACCENT);
        } else if (hov) {
            DrawRoundRect(hdc, x + 8, navY, MENU_SIDEBAR_W - 16, 34, 6,
                          RGB(25, 25, 32), MENU_SIDEBAR_BR);
        }

        COLORREF nc = sel ? MENU_ACCENT_L : (hov ? MENU_TEXT : MENU_TEXT_DIM);
        char buf[64];
        snprintf(buf, sizeof(buf), "  %s  %s", icons[i], labels[i]);
        DrawText_(hdc, buf, x + 12, navY, MENU_SIDEBAR_W - 24, 34,
                  nc, m_fontBody, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        m_sidebarRects[i] = { x + 8, navY, MENU_SIDEBAR_W - 16, 34 };
        navY += 40;
    }
}

void GameMenu::DrawHeader(HDC hdc, int x, int y, int w) {
    RECT hr = { x, y, x + w, y + MENU_HEADER_H };
    HBRUSH hb = CreateSolidBrush(MENU_BG_HEADER);
    FillRect(hdc, &hr, hb);
    DeleteObject(hb);

    // Separator
    HPEN sp = CreatePen(PS_SOLID, 1, MENU_BORDER);
    HPEN op = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, x, y + MENU_HEADER_H - 1, nullptr);
    LineTo(hdc, x + w, y + MENU_HEADER_H - 1);
    SelectObject(hdc, op);
    DeleteObject(sp);

    // Title based on current view
    const char* title = "";
    if (m_activePage == PAGE_INVENTORY) {
        switch (m_invView) {
        case INV_EQUIPPED: title = "Inventory"; break;
        case INV_WEAPONS:  title = "V\xE6lg V\xE5""ben"; break;
        case INV_SKINS:
            if (m_selWeapon >= 0 && m_selWeapon < (int)m_weaponDB.size())
                title = m_weaponDB[m_selWeapon].name.c_str();
            else title = "Skins";
            break;
        case INV_DETAIL:   title = "Skin Detaljer"; break;
        }
    } else {
        title = "Configs";
    }
    DrawText_(hdc, title, x + MENU_PAD, y, 300, MENU_HEADER_H,
              MENU_WHITE, m_fontTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // "Tilf\xF8j Skin" button (only in equipped view)
    if (m_activePage == PAGE_INVENTORY && m_invView == INV_EQUIPPED) {
        int bw = 110, bh = 30;
        int bx = x + w - bw - MENU_PAD;
        int by = y + (MENU_HEADER_H - bh) / 2;
        bool hov = HitTest({bx, by, bw, bh}, m_mouseX, m_mouseY);
        DrawButton(hdc, bx, by, bw, bh, "+ Tilf\xF8j Skin", hov, MENU_BTN_BG, MENU_BTN_HOV);
        m_addBtnRect = { bx, by, bw, bh };
    } else {
        m_addBtnRect = { 0, 0, 0, 0 };
    }

    // Back button (in weapons/skins/detail views)
    if (m_activePage == PAGE_INVENTORY && m_invView != INV_EQUIPPED) {
        int bw = 70, bh = 28;
        int bx = x + w - bw - MENU_PAD;
        int by = y + (MENU_HEADER_H - bh) / 2;
        // Move left if addBtn is visible
        if (m_invView == INV_EQUIPPED) bx -= 120;
        bool hov = HitTest({bx, by, bw, bh}, m_mouseX, m_mouseY);
        DrawButton(hdc, bx, by, bw, bh, "\x3C Tilbage", hov, RGB(50, 50, 60), RGB(65, 65, 78));
        m_backBtnRect = { bx, by, bw, bh };
    } else {
        m_backBtnRect = { 0, 0, 0, 0 };
    }
}

void GameMenu::DrawEquipped(HDC hdc, int x, int y, int w, int h) {
    int py = y + 12 - m_scrollOffset;

    if (m_equipped.empty()) {
        DrawText_(hdc, "Ingen skins udstyret endnu.", x + MENU_PAD, py + 40, w - MENU_PAD * 2, 30,
                  MENU_TEXT_DIM, m_fontBody, DT_CENTER | DT_SINGLELINE);
        DrawText_(hdc, "Klik \"+ Tilf\xF8j Skin\" for at tilf\xF8je skins til dit inventory.",
                  x + MENU_PAD, py + 70, w - MENU_PAD * 2, 24,
                  MENU_TEXT_DIM, m_fontSmall, DT_CENTER | DT_SINGLELINE);
        return;
    }

    // Grid of equipped items
    int cols = 4;
    int cardW = (w - MENU_PAD * 2 - (cols - 1) * GRID_GAP) / cols;
    int cardH = 130;
    int col = 0;
    int startX = x + MENU_PAD;

    for (int i = 0; i < (int)m_equipped.size(); i++) {
        auto& eq = m_equipped[i];
        int cx = startX + col * (cardW + GRID_GAP);
        int cy = py;

        if (cy + cardH > y && cy < y + h) {
            bool hov = HitTest({cx, cy, cardW, cardH}, m_mouseX, m_mouseY);

            // Card background with orange-ish left border (like image 2)
            DrawRoundRect(hdc, cx, cy, cardW, cardH, 6, MENU_CARD, hov ? MENU_CARD_BR : MENU_BORDER);

            // Orange left accent
            RECT accent = { cx, cy + 4, cx + 3, cy + cardH - 4 };
            HBRUSH ab = CreateSolidBrush(MENU_ORANGE);
            FillRect(hdc, &accent, ab);
            DeleteObject(ab);

            // Weapon + skin name
            DrawText_(hdc, eq.weapon.c_str(), cx + 10, cy + 10, cardW - 20, 18,
                      MENU_WHITE, m_fontBold, DT_LEFT | DT_SINGLELINE);
            DrawText_(hdc, eq.skin.c_str(), cx + 10, cy + 30, cardW - 20, 18,
                      MENU_TEXT, m_fontBody, DT_LEFT | DT_SINGLELINE);

            // Float value
            char floatBuf[32];
            snprintf(floatBuf, sizeof(floatBuf), "Float: %.4f", eq.floatVal);
            DrawText_(hdc, floatBuf, cx + 10, cy + 52, cardW - 20, 16,
                      MENU_TEXT_DIM, m_fontSmall, DT_LEFT | DT_SINGLELINE);

            // Wear name
            const char* wear = "Factory New";
            if (eq.floatVal > 0.07f) wear = "Minimal Wear";
            if (eq.floatVal > 0.15f) wear = "Field-Tested";
            if (eq.floatVal > 0.38f) wear = "Well-Worn";
            if (eq.floatVal > 0.45f) wear = "Battle-Scarred";
            DrawText_(hdc, wear, cx + 10, cy + 70, cardW - 20, 16,
                      MENU_GREEN, m_fontSmall, DT_LEFT | DT_SINGLELINE);

            // Remove button
            int rbw = 60, rbh = 22;
            int rbx = cx + (cardW - rbw) / 2;
            int rby = cy + cardH - rbh - 8;
            bool rhov = HitTest({rbx, rby, rbw, rbh}, m_mouseX, m_mouseY);
            DrawRoundRect(hdc, rbx, rby, rbw, rbh, 4,
                          rhov ? RGB(70, 30, 30) : RGB(50, 22, 22),
                          MENU_RED);
            DrawText_(hdc, "Fjern", rbx, rby, rbw, rbh,
                      MENU_RED, m_fontSmall, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (m_invRemoveCount < MAX_INV) {
                m_invRemoveRects[m_invRemoveCount++] = { rbx, rby, rbw, rbh };
            }
        }

        col++;
        if (col >= cols) {
            col = 0;
            py += cardH + GRID_GAP;
        }
    }
}

void GameMenu::DrawWeaponGrid(HDC hdc, int x, int y, int w, int h) {
    int py = y + 12 - m_scrollOffset;
    int cols = GRID_COLS;
    int cardW = (w - MENU_PAD * 2 - (cols - 1) * GRID_GAP) / cols;
    int cardH = 70;
    int col = 0;
    int startX = x + MENU_PAD;

    for (int i = 0; i < (int)m_weaponDB.size(); i++) {
        int cx = startX + col * (cardW + GRID_GAP);
        int cy = py;

        if (cy + cardH > y && cy < y + h) {
            bool hov = HitTest({cx, cy, cardW, cardH}, m_mouseX, m_mouseY);

            DrawRoundRect(hdc, cx, cy, cardW, cardH, 6,
                          hov ? MENU_CARD_HOV : MENU_CARD,
                          hov ? MENU_ACCENT : MENU_BORDER);

            // Weapon name centered
            DrawText_(hdc, m_weaponDB[i].name.c_str(), cx + 8, cy, cardW - 16, cardH,
                      hov ? MENU_WHITE : MENU_TEXT, m_fontBold,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Skin count
            char countBuf[16];
            snprintf(countBuf, sizeof(countBuf), "%d skins", (int)m_weaponDB[i].skins.size());
            DrawText_(hdc, countBuf, cx + 8, cy + cardH - 20, cardW - 16, 16,
                      MENU_TEXT_DIM, m_fontSmall, DT_CENTER | DT_SINGLELINE);

            if (m_gridCount < MAX_GRID) {
                m_gridRects[m_gridCount] = { cx, cy, cardW, cardH };
                m_gridIds[m_gridCount] = i;
                m_gridCount++;
            }
        }

        col++;
        if (col >= cols) {
            col = 0;
            py += cardH + GRID_GAP;
        }
    }
}

void GameMenu::DrawSkinGrid(HDC hdc, int x, int y, int w, int h) {
    if (m_selWeapon < 0 || m_selWeapon >= (int)m_weaponDB.size()) return;
    auto& wd = m_weaponDB[m_selWeapon];

    int py = y + 12 - m_scrollOffset;
    int cols = GRID_COLS;
    int cardW = (w - MENU_PAD * 2 - (cols - 1) * GRID_GAP) / cols;
    int cardH = GRID_CARD_H;
    int col = 0;
    int startX = x + MENU_PAD;

    for (int i = 0; i < (int)wd.skins.size(); i++) {
        int cx = startX + col * (cardW + GRID_GAP);
        int cy = py;

        if (cy + cardH > y && cy < y + h) {
            bool hov = HitTest({cx, cy, cardW, cardH}, m_mouseX, m_mouseY);

            // Check if equipped
            bool isEquipped = false;
            for (auto& eq : m_equipped) {
                if (eq.weapon == wd.name && eq.skin == wd.skins[i]) { isEquipped = true; break; }
            }

            COLORREF bg = isEquipped ? RGB(25, 40, 30) : (hov ? MENU_CARD_HOV : MENU_CARD);
            COLORREF br = isEquipped ? MENU_GREEN : (hov ? MENU_ACCENT : MENU_BORDER);

            DrawRoundRect(hdc, cx, cy, cardW, cardH, 6, bg, br);

            // Colored top bar (simulates skin rarity like in image 2)
            COLORREF rarityCol = MENU_ACCENT;
            // Simplified rarity based on position
            if (i < 3) rarityCol = RGB(200, 50, 50);       // red = covert
            else if (i < 8) rarityCol = RGB(180, 60, 180);  // pink = classified
            else if (i < 15) rarityCol = RGB(100, 80, 200); // purple = restricted
            else rarityCol = RGB(60, 120, 200);             // blue = mil-spec

            RECT topBar = { cx + 1, cy + 1, cx + cardW - 1, cy + 4 };
            HBRUSH tb = CreateSolidBrush(rarityCol);
            FillRect(hdc, &topBar, tb);
            DeleteObject(tb);

            // Skin name (centered, two lines)
            DrawText_(hdc, wd.skins[i].c_str(), cx + 8, cy + 30, cardW - 16, 50,
                      hov ? MENU_WHITE : MENU_TEXT, m_fontGrid,
                      DT_CENTER | DT_WORDBREAK);

            // Weapon name small at top
            DrawText_(hdc, wd.name.c_str(), cx + 8, cy + 10, cardW - 16, 16,
                      MENU_TEXT_DIM, m_fontSmall, DT_CENTER | DT_SINGLELINE);

            // Equipped badge
            if (isEquipped) {
                DrawText_(hdc, "EQUIPPED", cx + 8, cy + cardH - 22, cardW - 16, 16,
                          MENU_GREEN, m_fontSmall, DT_CENTER | DT_SINGLELINE);
            }

            if (m_gridCount < MAX_GRID) {
                m_gridRects[m_gridCount] = { cx, cy, cardW, cardH };
                m_gridIds[m_gridCount] = i;
                m_gridCount++;
            }
        }

        col++;
        if (col >= cols) {
            col = 0;
            py += cardH + GRID_GAP;
        }
    }
}

void GameMenu::DrawSkinDetail(HDC hdc, int x, int y, int w, int h) {
    if (m_selWeapon < 0 || m_selWeapon >= (int)m_weaponDB.size()) return;
    auto& wd = m_weaponDB[m_selWeapon];
    if (m_selSkin < 0 || m_selSkin >= (int)wd.skins.size()) return;

    int py = y + 20;
    int cw = w - MENU_PAD * 2;

    // Large skin card
    int cardH = 200;
    DrawRoundRect(hdc, x + MENU_PAD, py, cw, cardH, 8, MENU_CARD, MENU_BORDER);

    // Rarity bar
    COLORREF rarityCol = MENU_ACCENT;
    if (m_selSkin < 3) rarityCol = RGB(200, 50, 50);
    else if (m_selSkin < 8) rarityCol = RGB(180, 60, 180);
    else if (m_selSkin < 15) rarityCol = RGB(100, 80, 200);
    else rarityCol = RGB(60, 120, 200);
    RECT topBar = { x + MENU_PAD + 1, py + 1, x + MENU_PAD + cw - 1, py + 5 };
    HBRUSH tb = CreateSolidBrush(rarityCol);
    FillRect(hdc, &topBar, tb);
    DeleteObject(tb);

    // Weapon name
    DrawText_(hdc, wd.name.c_str(), x + MENU_PAD + 20, py + 20, cw - 40, 24,
              MENU_TEXT_DIM, m_fontBody, DT_LEFT | DT_SINGLELINE);

    // Skin name (large)
    DrawText_(hdc, wd.skins[m_selSkin].c_str(), x + MENU_PAD + 20, py + 50, cw - 40, 30,
              MENU_WHITE, m_fontTitle, DT_LEFT | DT_SINGLELINE);

    // Wear name
    const char* wear = "Factory New";
    if (m_detailFloat > 0.07f) wear = "Minimal Wear";
    if (m_detailFloat > 0.15f) wear = "Field-Tested";
    if (m_detailFloat > 0.38f) wear = "Well-Worn";
    if (m_detailFloat > 0.45f) wear = "Battle-Scarred";

    DrawText_(hdc, wear, x + MENU_PAD + 20, py + 90, cw - 40, 22,
              MENU_GREEN, m_fontBold, DT_LEFT | DT_SINGLELINE);

    // Float value display
    char floatBuf[64];
    snprintf(floatBuf, sizeof(floatBuf), "Float Value: %.6f", m_detailFloat);
    DrawText_(hdc, floatBuf, x + MENU_PAD + 20, py + 116, cw - 40, 20,
              MENU_TEXT, m_fontBody, DT_LEFT | DT_SINGLELINE);

    py += cardH + 20;

    // Float slider
    DrawText_(hdc, "Float Slider", x + MENU_PAD, py, cw, 20,
              MENU_TEXT, m_fontBold, DT_LEFT | DT_SINGLELINE);
    py += 26;

    // Slider labels
    DrawText_(hdc, "0.00", x + MENU_PAD, py - 2, 40, 16, MENU_TEXT_DIM, m_fontSmall, DT_LEFT | DT_SINGLELINE);
    DrawText_(hdc, "1.00", x + MENU_PAD + cw - 40, py - 2, 40, 16, MENU_TEXT_DIM, m_fontSmall, DT_RIGHT | DT_SINGLELINE);

    int sliderX = x + MENU_PAD + 40;
    int sliderW = cw - 80;
    int sliderY = py;
    int sliderH = 20;

    // Track background
    DrawRoundRect(hdc, sliderX, sliderY + 6, sliderW, 8, 4, RGB(40, 40, 50), MENU_BORDER);

    // Filled portion
    int fillW = (int)(sliderW * m_detailFloat);
    if (fillW > 2) {
        DrawRoundRect(hdc, sliderX, sliderY + 6, fillW, 8, 4, MENU_ACCENT, MENU_ACCENT);
    }

    // Knob
    int knobX = sliderX + fillW - 8;
    if (knobX < sliderX) knobX = sliderX;
    DrawRoundRect(hdc, knobX, sliderY + 2, 16, 16, 8, MENU_WHITE, MENU_ACCENT_L);

    m_sliderRect = { sliderX, sliderY, sliderW, sliderH };

    // Wear zone indicators
    py += 30;
    const char* zones[] = { "FN", "MW", "FT", "WW", "BS" };
    float zoneEnds[] = { 0.07f, 0.15f, 0.38f, 0.45f, 1.0f };
    float zoneStart = 0.0f;
    COLORREF zoneColors[] = { MENU_GREEN, RGB(130,200,80), RGB(200,180,50), MENU_ORANGE, MENU_RED };

    for (int i = 0; i < 5; i++) {
        int zx = sliderX + (int)(sliderW * zoneStart);
        int zw = (int)(sliderW * (zoneEnds[i] - zoneStart));
        DrawRoundRect(hdc, zx, py, zw, 14, 2, RGB(30, 30, 38), zoneColors[i]);
        DrawText_(hdc, zones[i], zx, py, zw, 14, MENU_WHITE, m_fontSmall, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        zoneStart = zoneEnds[i];
    }

    py += 34;

    // "Tilf\xF8j" button
    int btnW = 160, btnH = 40;
    int btnX = x + (w - btnW) / 2;
    bool hov = HitTest({btnX, py, btnW, btnH}, m_mouseX, m_mouseY);
    DrawButton(hdc, btnX, py, btnW, btnH, "Tilf\xF8j til Inventory", hov, MENU_BTN_BG, MENU_BTN_HOV);
    m_applyBtnRect = { btnX, py, btnW, btnH };
}

void GameMenu::DrawConfigs(HDC hdc, int x, int y, int w, int h) {
    struct Setting { const char* name; const char* desc; bool* value; };
    Setting settings[] = {
        { "Skin Changer", "Aktiver skin changer", &m_skinChangerEnabled },
        { "Knife Changer", "Skift kniv model", &m_knifeChangerEnabled },
        { "Glove Changer", "Skift handske model", &m_gloveChangerEnabled },
    };
    int count = 3;
    m_toggleCount = 0;

    int py = y + 16;
    for (int i = 0; i < count; i++) {
        int cardH = 50;
        bool hov = HitTest({x + MENU_PAD, py, w - MENU_PAD * 2, cardH}, m_mouseX, m_mouseY);

        DrawRoundRect(hdc, x + MENU_PAD, py, w - MENU_PAD * 2, cardH, 6,
                      hov ? MENU_CARD_HOV : MENU_CARD, hov ? MENU_CARD_BR : MENU_BORDER);

        DrawText_(hdc, settings[i].name, x + MENU_PAD + 14, py + 6, w - MENU_PAD * 2 - 80, 20,
                  MENU_WHITE, m_fontBold, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        DrawText_(hdc, settings[i].desc, x + MENU_PAD + 14, py + 26, w - MENU_PAD * 2 - 80, 16,
                  MENU_TEXT_DIM, m_fontSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Toggle circle
        bool on = *settings[i].value;
        int tw = 40, th = 20;
        int tx = x + w - MENU_PAD - tw - 10;
        int ty = py + (cardH - th) / 2;

        DrawRoundRect(hdc, tx, ty, tw, th, th / 2, on ? MENU_TOGGLE_ON : MENU_TOGGLE_OFF,
                      on ? MENU_ACCENT_L : RGB(80, 80, 90));

        int knobD = th - 4;
        int knobX = on ? (tx + tw - knobD - 2) : (tx + 2);
        DrawRoundRect(hdc, knobX, ty + 2, knobD, knobD, knobD / 2, MENU_WHITE, MENU_WHITE);

        if (m_toggleCount < 8) {
            m_toggleRects[m_toggleCount++] = { x + MENU_PAD, py, w - MENU_PAD * 2, cardH };
        }

        py += cardH + 10;
    }
}


// ============================================================================
//  OverlayWindow
// ============================================================================

OverlayWindow::OverlayWindow()
    : m_hwnd(nullptr)
    , m_gameHwnd(nullptr)
    , m_cs2Pid(0)
    , m_hModule(nullptr)
    , m_running(false)
    , m_autoShow(true) {
}

OverlayWindow::~OverlayWindow() {
    Destroy();
}

// Helper struct for EnumWindows callback
struct FindCS2WndData {
    DWORD  pid;
    HWND   bestHwnd;
    int    bestArea;
};

static BOOL CALLBACK FindCS2WndCallback(HWND hwnd, LPARAM lp) {
    auto* data = (FindCS2WndData*)lp;
    DWORD wndPid = 0;
    GetWindowThreadProcessId(hwnd, &wndPid);
    if (wndPid != data->pid) return TRUE;
    // Skip invisible windows
    if (!IsWindowVisible(hwnd)) return TRUE;
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int w = wr.right - wr.left;
    int h = wr.bottom - wr.top;
    // Skip tiny or off-screen windows
    if (w < 200 || h < 200) return TRUE;
    if (wr.left > 10000 || wr.top > 10000) return TRUE;
    if (wr.left < -10000 || wr.top < -10000) return TRUE;
    int area = w * h;
    char cls[128] = {}, ttl[256] = {};
    GetClassNameA(hwnd, cls, 128);
    GetWindowTextA(hwnd, ttl, 256);
    DbgLog("  PID-enum: hwnd=%p class='%s' title='%s' rect=(%d,%d,%d,%d) area=%d",
           (void*)hwnd, cls, ttl, wr.left, wr.top, wr.right, wr.bottom, area);
    if (area > data->bestArea) {
        data->bestArea = area;
        data->bestHwnd = hwnd;
    }
    return TRUE;
}

bool OverlayWindow::Create(HMODULE hModule) {
    return Create(hModule, 0);
}

bool OverlayWindow::Create(HMODULE hModule, DWORD cs2Pid) {
    m_hModule = hModule;
    m_cs2Pid = cs2Pid;
    g_pOverlay = this;
    DbgLog("=== OverlayWindow::Create START (PID=%d) ===", (int)cs2Pid);

    // Strategy 1: Find by PID (most reliable)
    if (cs2Pid != 0) {
        DbgLog("Searching windows by PID %d...", (int)cs2Pid);
        FindCS2WndData data = { cs2Pid, nullptr, 0 };
        EnumWindows(FindCS2WndCallback, (LPARAM)&data);
        m_gameHwnd = data.bestHwnd;
        DbgLog("PID search result => %p (area=%d)", (void*)m_gameHwnd, data.bestArea);
    }

    // Strategy 2: FindWindow by title
    if (!m_gameHwnd) {
        m_gameHwnd = FindWindowA(nullptr, "Counter-Strike 2");
        DbgLog("FindWindowA(title='Counter-Strike 2') => %p", (void*)m_gameHwnd);
    }

    // Strategy 3: FindWindow by class, but validate position
    if (!m_gameHwnd) {
        HWND sdlHwnd = FindWindowA("SDL_app", nullptr);
        if (sdlHwnd) {
            RECT wr; GetWindowRect(sdlHwnd, &wr);
            DbgLog("FindWindowA(class='SDL_app') => %p rect=(%d,%d,%d,%d)",
                   (void*)sdlHwnd, wr.left, wr.top, wr.right, wr.bottom);
            // Only use if on-screen
            if (wr.left < 10000 && wr.top < 10000 && wr.left > -10000 && wr.top > -10000) {
                int w = wr.right - wr.left;
                int h = wr.bottom - wr.top;
                if (w >= 200 && h >= 200) {
                    m_gameHwnd = sdlHwnd;
                    DbgLog("SDL_app window accepted (on-screen, %dx%d)", w, h);
                } else {
                    DbgLog("SDL_app window REJECTED (too small: %dx%d)", w, h);
                }
            } else {
                DbgLog("SDL_app window REJECTED (off-screen)");
            }
        }
    }

    if (m_gameHwnd) {
        char cls[128] = {}, ttl[256] = {};
        GetClassNameA(m_gameHwnd, cls, 128);
        GetWindowTextA(m_gameHwnd, ttl, 256);
        RECT wr; GetWindowRect(m_gameHwnd, &wr);
        DbgLog("Selected CS2 window: hwnd=%p class='%s' title='%s' rect=(%d,%d,%d,%d)",
               (void*)m_gameHwnd, cls, ttl, wr.left, wr.top, wr.right, wr.bottom);
        LONG style = GetWindowLong(m_gameHwnd, GWL_STYLE);
        LONG exstyle = GetWindowLong(m_gameHwnd, GWL_EXSTYLE);
        DbgLog("CS2 style=0x%08X exstyle=0x%08X", style, exstyle);
    } else {
        DbgLog("WARNING: No suitable CS2 window found! Using primary monitor.");
    }

    // Register overlay window class
    WNDCLASSEXA wc = { sizeof(wc) };
    wc.lpfnWndProc   = OverlayWindow::WndProc;
    wc.hInstance      = hModule;
    wc.lpszClassName  = "AC_Overlay";
    wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
    ATOM regResult = RegisterClassExA(&wc);
    DbgLog("RegisterClassExA => %d (LastError=%d)", regResult, (int)GetLastError());

    // Get game window position / size (use primary monitor as fallback)
    RECT gameRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    if (m_gameHwnd) {
        RECT wr;
        GetWindowRect(m_gameHwnd, &wr);
        // Validate the rect is sane
        if (wr.left < 10000 && wr.top < 10000 && wr.left > -10000 && wr.top > -10000) {
            int w = wr.right - wr.left;
            int h = wr.bottom - wr.top;
            if (w >= 200 && h >= 200) {
                gameRect = wr;
            } else {
                DbgLog("Game window rect too small (%dx%d), using monitor", w, h);
                m_gameHwnd = nullptr;
            }
        } else {
            DbgLog("Game window rect off-screen (%d,%d), using monitor", wr.left, wr.top);
            m_gameHwnd = nullptr;
        }
    }
    int gw = gameRect.right  - gameRect.left;
    int gh = gameRect.bottom - gameRect.top;
    DbgLog("Overlay size: %dx%d at (%d,%d)", gw, gh, gameRect.left, gameRect.top);

    // Create transparent layered window
    m_hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        "AC_Overlay", "AC Overlay",
        WS_POPUP,
        gameRect.left, gameRect.top, gw, gh,
        nullptr, nullptr, hModule, nullptr);

    if (!m_hwnd) {
        DbgLog("ERROR: CreateWindowExA FAILED! LastError=%d", (int)GetLastError());
        return false;
    }
    DbgLog("Overlay window created: hwnd=%p", (void*)m_hwnd);

    // Make window transparent (magenta is the transparent colour key)
    BOOL layerOk = SetLayeredWindowAttributes(m_hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
    DbgLog("SetLayeredWindowAttributes => %d", (int)layerOk);

    // Initialise menu
    if (!m_menu.Initialize(hModule)) {
        DbgLog("ERROR: GameMenu::Initialize failed!");
        return false;
    }
    DbgLog("GameMenu initialized OK");

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    SetForegroundWindow(m_hwnd);
    BringWindowToTop(m_hwnd);
    DbgLog("Overlay window shown and brought to front");

    m_running = true;
    DbgLog("=== OverlayWindow::Create SUCCESS ===");
    return true;
}

void OverlayWindow::Destroy() {
    m_running = false;
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_menu.Shutdown();
    g_pOverlay = nullptr;
}

void OverlayWindow::ShowMenu() {
    DbgLog("ShowMenu() called, m_visible was %d", (int)m_menu.IsVisible());
    if (!m_menu.IsVisible()) {
        m_menu.Toggle();
    }
    // Make overlay interactive (remove click-through)
    LONG ex = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    ex &= ~WS_EX_TRANSPARENT;
    SetWindowLong(m_hwnd, GWL_EXSTYLE, ex);
    DbgLog("ShowMenu() done, m_visible=%d exstyle=0x%08X", (int)m_menu.IsVisible(), (int)GetWindowLong(m_hwnd, GWL_EXSTYLE));
}

void OverlayWindow::UpdatePosition() {
    // Try to find the real CS2 window if we don't have one or it's invalid
    if (!m_gameHwnd || !IsWindow(m_gameHwnd)) {
        m_gameHwnd = nullptr;
        // Search by PID first
        if (m_cs2Pid != 0) {
            FindCS2WndData data = { m_cs2Pid, nullptr, 0 };
            EnumWindows(FindCS2WndCallback, (LPARAM)&data);
            m_gameHwnd = data.bestHwnd;
        }
        // Fallback: by title
        if (!m_gameHwnd) {
            m_gameHwnd = FindWindowA(nullptr, "Counter-Strike 2");
        }
        if (!m_gameHwnd) return;

        // Log when we find a new window
        static bool loggedNewWindow = false;
        if (!loggedNewWindow) {
            char cls[128] = {}, ttl[256] = {};
            GetClassNameA(m_gameHwnd, cls, 128);
            GetWindowTextA(m_gameHwnd, ttl, 256);
            RECT wr; GetWindowRect(m_gameHwnd, &wr);
            DbgLog("UpdatePosition: Found new CS2 window hwnd=%p class='%s' title='%s' rect=(%d,%d,%d,%d)",
                   (void*)m_gameHwnd, cls, ttl, wr.left, wr.top, wr.right, wr.bottom);
            loggedNewWindow = true;
        }
    }

    RECT gr;
    GetWindowRect(m_gameHwnd, &gr);

    // Validate position is sane
    if (gr.left > 10000 || gr.top > 10000 || gr.left < -10000 || gr.top < -10000) {
        // Window is off-screen, use monitor instead
        gr.left = 0; gr.top = 0;
        gr.right = GetSystemMetrics(SM_CXSCREEN);
        gr.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    int gw = gr.right  - gr.left;
    int gh = gr.bottom - gr.top;
    if (gw < 200 || gh < 200) return;  // skip invalid sizes

    SetWindowPos(m_hwnd, HWND_TOPMOST, gr.left, gr.top, gw, gh,
                 SWP_NOACTIVATE);
}

void OverlayWindow::RunFrame() {
    if (!m_running) return;

    // Follow game window position
    UpdatePosition();

    // Auto-show menu on first frame
    if (m_autoShow) {
        m_autoShow = false;
        DbgLog("Auto-show: showing menu on first frame");
        ShowMenu();
        DbgLog("Auto-show complete. m_hwnd=%p visible=%d", (void*)m_hwnd, (int)IsWindowVisible(m_hwnd));
    }

    // Periodic debug (every ~5 seconds = 300 frames)
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 300 == 1) {
        DbgLog("Frame %d: m_hwnd=%p gameHwnd=%p menuVisible=%d hwndVisible=%d",
               frameCount, (void*)m_hwnd, (void*)m_gameHwnd,
               (int)m_menu.IsVisible(), (int)IsWindowVisible(m_hwnd));
        RECT wr; GetWindowRect(m_hwnd, &wr);
        DbgLog("  overlay rect=(%d,%d,%d,%d) size=%dx%d",
               wr.left, wr.top, wr.right, wr.bottom,
               wr.right-wr.left, wr.bottom-wr.top);
    }

    // Check INSERT key to toggle menu
    static bool keyWasDown = false;
    bool keyDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
    if (keyDown && !keyWasDown) {
        if (m_menu.IsVisible()) {
            // Hide menu, make window click-through
            m_menu.Toggle();
            LONG ex = GetWindowLong(m_hwnd, GWL_EXSTYLE);
            ex |= WS_EX_TRANSPARENT;
            SetWindowLong(m_hwnd, GWL_EXSTYLE, ex);
        } else {
            ShowMenu();
        }
    }
    keyWasDown = keyDown;

    // Process menu input
    m_menu.ProcessInput();

    // Repaint
    InvalidateRect(m_hwnd, nullptr, TRUE);
    UpdateWindow(m_hwnd);

    // Process pending messages
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

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;

        // Double buffer
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        SelectObject(mem, bmp);

        // Clear to colour key (magenta = transparent)
        HBRUSH bg = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(mem, &rc, bg);
        DeleteObject(bg);

        // Draw menu
        if (g_pOverlay) {
            g_pOverlay->m_menu.Render(mem, w, h);
        }

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        DeleteObject(bmp);
        DeleteDC(mem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
        if (g_pOverlay) {
            int mx = (short)LOWORD(lp);
            int my = (short)HIWORD(lp);
            g_pOverlay->m_menu.OnMouseMove(mx, my);
            InvalidateRect(hwnd, nullptr, FALSE);  // repaint for hover
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (g_pOverlay) {
            int mx = (short)LOWORD(lp);
            int my = (short)HIWORD(lp);
            g_pOverlay->m_menu.OnMouseClick(mx, my);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (g_pOverlay) {
            g_pOverlay->m_menu.OnMouseUp();
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (g_pOverlay) {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            g_pOverlay->m_menu.OnMouseWheel(delta);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}


// ============================================================================
//  GameInjector
// ============================================================================

GameInjector::GameInjector()
    : m_initialized(false)
    , m_overlay(std::make_unique<OverlayWindow>()) {
}

GameInjector::~GameInjector() {
    if (m_initialized) Shutdown();
}

bool GameInjector::Initialize(HMODULE hModule) {
    if (!m_overlay->Create(hModule)) {
        return false;
    }
    m_initialized = true;
    return true;
}

void GameInjector::Shutdown() {
    if (m_overlay) m_overlay->Destroy();
    m_initialized = false;
}

void GameInjector::Update() {
    if (!m_initialized) return;
    m_overlay->RunFrame();
}
