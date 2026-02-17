/*
 * AC Skin Changer - Skin Database
 * Contains 70+ weapons and 800+ skins with real CS2 paint kit IDs.
 * Organized by category: Pistols, Rifles, SMGs, Shotguns, Machine Guns,
 * Snipers, Knives, and Gloves.
 */

#include "core.h"

namespace {
    // Helper to add weapon
    void addWpn(const std::string& category, const std::string& name, int id,
                std::vector<SkinInfo> skins) {
        WeaponInfo w;
        w.name = name;
        w.id = id;
        w.category = category;
        w.skins = std::move(skins);
        G::allWeapons.push_back(w);
        G::weaponsByCategory[category].push_back(w);
    }
}

namespace SkinDB {
    void Initialize() {
        G::allWeapons.clear();
        G::weaponsByCategory.clear();

        // ================================================================
        // PISTOLS
        // ================================================================
        addWpn("Pistols", "Desert Eagle", 1, {
            {"Blaze", 37, 6},
            {"Kumicho Dragon", 520, 5},
            {"Code Red", 661, 6},
            {"Mecha Industries", 641, 5},
            {"Printstream", 1018, 6},
            {"Sunset Storm", 61, 4},
            {"Conspiracy", 351, 3},
            {"Midnight Storm", 185, 3},
            {"Golden Koi", 62, 4},
            {"Emerald Jorm", 714, 4},
            {"Trigger Discipline", 808, 5},
            {"Ocean Drive", 760, 4},
            {"Fennec Fox", 688, 4},
            {"Crimson Web", 44, 4},
            {"Hypnotic", 28, 4},
            {"Cobalt Disruption", 327, 4},
            {"Heirloom", 175, 5},
            {"Pilot", 402, 3},
            {"Directive", 465, 3},
            {"Oxide Blaze", 571, 3},
            {"Blue Ply", 612, 2},
            {"Night", 100, 2},
        });

        addWpn("Pistols", "Glock-18", 4, {
            {"Fade", 38, 6},
            {"Water Elemental", 410, 5},
            {"Wasteland Rebel", 511, 5},
            {"Gamma Doppler", 568, 6},
            {"Twilight Galaxy", 638, 5},
            {"Bullet Queen", 773, 5},
            {"Weasel", 543, 4},
            {"Moonrise", 713, 4},
            {"Steel Disruption", 541, 3},
            {"Oxide Blaze", 572, 3},
            {"Off World", 680, 4},
            {"Royal Legion", 440, 3},
            {"Grinder", 349, 3},
            {"Vogue", 818, 4},
            {"Neo-Noir", 770, 4},
            {"Franklin", 367, 4},
            {"Reactor", 209, 4},
            {"Dragon Tattoo", 46, 4},
            {"Candy Apple", 3, 3},
            {"Blue Fissure", 247, 2},
            {"Sand Dune", 208, 1},
        });

        addWpn("Pistols", "USP-S", 61, {
            {"Kill Confirmed", 504, 6},
            {"Orion", 313, 5},
            {"Neo-Noir", 769, 5},
            {"Printstream", 1017, 6},
            {"Caiman", 423, 4},
            {"Cortex", 730, 4},
            {"Blueprint", 657, 3},
            {"Serum", 265, 3},
            {"Stainless", 218, 3},
            {"Road Rash", 490, 5},
            {"Dark Water", 95, 3},
            {"Monster Mashup", 651, 4},
            {"Guardian", 290, 3},
            {"Lead Conduit", 460, 2},
            {"Para Green", 454, 2},
            {"Overgrowth", 338, 4},
            {"Ticket to Hell", 814, 5},
            {"The Traitor", 757, 6},
            {"Whiteout", 217, 4},
            {"Night Ops", 77, 2},
        });

        addWpn("Pistols", "P250", 36, {
            {"Asiimov", 399, 5},
            {"See Ya Later", 674, 5},
            {"Muertos", 496, 4},
            {"Cartel", 443, 4},
            {"Vino Primo", 585, 3},
            {"Supernova", 213, 3},
            {"Franklin", 367, 3},
            {"Mehndi", 330, 4},
            {"Splash", 37, 3},
            {"Nevermore", 797, 4},
            {"Re.built", 695, 3},
            {"Inferno", 578, 3},
            {"Valence", 363, 2},
            {"Metallic DDPAT", 156, 2},
            {"Boreal Forest", 77, 1},
            {"Sand Dune", 208, 1},
        });

        addWpn("Pistols", "Five-SeveN", 3, {
            {"Monkey Business", 481, 5},
            {"Hyper Beast", 588, 5},
            {"Retrobution", 685, 4},
            {"Neon Kimono", 455, 4},
            {"Case Hardened", 44, 5},
            {"Copper Galaxy", 222, 3},
            {"Fowl Play", 386, 4},
            {"Angry Mob", 767, 4},
            {"Crimson Blossom", 776, 4},
            {"Candy Apple", 3, 3},
            {"Nightshade", 352, 3},
            {"Flame Test", 620, 3},
            {"Triumvirate", 542, 3},
            {"Berries and Cherries", 456, 5},
            {"Orange Peel", 163, 2},
            {"Forest Night", 77, 1},
        });

        addWpn("Pistols", "Tec-9", 30, {
            {"Fuel Injector", 602, 5},
            {"Decimator", 621, 5},
            {"Re-Entry", 551, 4},
            {"Hades", 468, 4},
            {"Ice Cap", 379, 3},
            {"Toxic", 230, 3},
            {"Titanium Bit", 366, 3},
            {"Isaac", 293, 3},
            {"Remote Control", 495, 3},
            {"Rebel", 775, 4},
            {"Bamboozle", 717, 4},
            {"Cut Out", 777, 3},
            {"Sandstorm", 66, 3},
            {"Red Quartz", 221, 2},
            {"Urban DDPAT", 155, 1},
            {"Army Mesh", 207, 1},
        });

        addWpn("Pistols", "CZ75-Auto", 63, {
            {"Victoria", 298, 5},
            {"Xiangliu", 557, 5},
            {"Eco", 712, 4},
            {"Tacticat", 581, 4},
            {"Poison Dart", 470, 3},
            {"Pole Position", 335, 3},
            {"Tigris", 332, 3},
            {"Yellow Jacket", 397, 4},
            {"Tuxedo", 112, 3},
            {"Crimson Web", 44, 3},
            {"Emerald Quartz", 301, 2},
            {"Army Sheen", 345, 2},
            {"Green Plaid", 168, 1},
        });

        addWpn("Pistols", "Dual Berettas", 2, {
            {"Twin Turbo", 773, 5},
            {"Cobalt Quartz", 299, 5},
            {"Royal Consorts", 696, 4},
            {"Urban Shock", 547, 3},
            {"Ventilators", 305, 3},
            {"Shred", 432, 3},
            {"Hemoglobin", 231, 3},
            {"Marina", 349, 4},
            {"Duelist", 500, 4},
            {"Cartel", 443, 3},
            {"Black Limba", 261, 3},
            {"Moon in Libra", 249, 2},
            {"Briar", 214, 2},
            {"Stained", 91, 2},
        });

        addWpn("Pistols", "R8 Revolver", 64, {
            {"Fade", 38, 6},
            {"Amber Fade", 544, 4},
            {"Llama Cannon", 602, 5},
            {"Grip", 626, 3},
            {"Reboot", 521, 3},
            {"Crimson Web", 44, 3},
            {"Survivalist", 428, 2},
            {"Bone Mask", 155, 1},
            {"Canal Spray", 655, 2},
            {"Nitro", 108, 3},
            {"Memento", 754, 4},
        });

        addWpn("Pistols", "P2000", 32, {
            {"Fire Elemental", 400, 5},
            {"Ocean Foam", 253, 5},
            {"Corticera", 313, 3},
            {"Amber Fade", 51, 4},
            {"Imperial Dragon", 513, 4},
            {"Handgun", 440, 3},
            {"Turf", 363, 3},
            {"Red FragCam", 300, 3},
            {"Ivory", 218, 3},
            {"Silver", 217, 3},
            {"Chainmail", 127, 2},
            {"Grassland", 357, 2},
        });

        // ================================================================
        // RIFLES
        // ================================================================
        addWpn("Rifles", "AK-47", 7, {
            {"Wild Lotus", 867, 6},
            {"Fire Serpent", 180, 6},
            {"Vulcan", 349, 5},
            {"Fuel Injector", 653, 5},
            {"Neon Revolution", 656, 5},
            {"Bloodsport", 796, 5},
            {"The Empress", 697, 5},
            {"Asiimov", 524, 5},
            {"Neon Rider", 744, 5},
            {"Phantom Disruptor", 778, 4},
            {"Redline", 282, 4},
            {"Wasteland Rebel", 434, 4},
            {"Frontside Misty", 509, 4},
            {"Orbit Mk01", 636, 4},
            {"Point Disarray", 506, 4},
            {"Case Hardened", 44, 5},
            {"Hydroponic", 526, 5},
            {"Aquamarine Revenge", 479, 5},
            {"Jaguar", 316, 4},
            {"Cartel", 443, 3},
            {"Blue Laminate", 179, 3},
            {"Emerald Pinstripe", 230, 3},
            {"Red Laminate", 33, 4},
            {"First Class", 579, 3},
            {"Elite Build", 422, 3},
            {"Rat Rod", 636, 3},
            {"Leet Museo", 713, 4},
            {"Ice Coaled", 741, 4},
            {"Nightwish", 819, 5},
            {"Head Shot", 772, 4},
            {"Slate", 770, 3},
            {"Safari Mesh", 72, 1},
            {"Jungle Spray", 198, 1},
            {"Predator", 12, 2},
        });

        addWpn("Rifles", "M4A4", 16, {
            {"Howl", 309, 7},
            {"Asiimov", 255, 5},
            {"Desolate Space", 648, 5},
            {"The Emperor", 749, 5},
            {"Neo-Noir", 770, 5},
            {"Buzz Kill", 671, 5},
            {"Royal Paladin", 512, 4},
            {"Hellfire", 654, 5},
            {"In Living Color", 687, 5},
            {"Tooth Fairy", 733, 4},
            {"Spider Lily", 810, 5},
            {"Temukau", 827, 5},
            {"The Coalition", 786, 4},
            {"Cyber Security", 694, 4},
            {"Evil Daimyo", 488, 3},
            {"Dragon King", 400, 4},
            {"Bullet Rain", 379, 4},
            {"X-Ray", 187, 4},
            {"Daybreak", 469, 4},
            {"Faded Zebra", 161, 2},
            {"Urban DDPAT", 155, 2},
            {"Jungle Tiger", 105, 2},
            {"Desert-Strike", 292, 3},
            {"Mainframe", 328, 3},
            {"Converter", 335, 3},
            {"Zirka", 120, 4},
            {"Griffin", 398, 3},
        });

        addWpn("Rifles", "M4A1-S", 60, {
            {"Printstream", 1016, 6},
            {"Hyper Beast", 519, 5},
            {"Golden Coil", 632, 5},
            {"Chantico's Fire", 639, 5},
            {"Mecha Industries", 641, 5},
            {"Decimator", 595, 4},
            {"Leaded Glass", 714, 4},
            {"Nightmare", 726, 5},
            {"Player Two", 781, 5},
            {"Welcome to the Jungle", 815, 5},
            {"Cyrex", 360, 4},
            {"Atomic Alloy", 339, 4},
            {"Guardian", 290, 3},
            {"Master Piece", 381, 6},
            {"Knight", 326, 5},
            {"Hot Rod", 445, 6},
            {"Icarus Fell", 440, 6},
            {"Dark Water", 95, 3},
            {"Basilisk", 432, 3},
            {"Briefing", 549, 3},
            {"Control Panel", 733, 3},
            {"Nitro", 108, 4},
            {"Flashback", 227, 3},
            {"Boreal Forest", 77, 2},
            {"Bright Water", 252, 3},
            {"Blue Phosphor", 754, 4},
            {"Emphorosaur-S", 826, 5},
        });

        addWpn("Rifles", "AWP", 9, {
            {"Dragon Lore", 344, 7},
            {"Medusa", 446, 6},
            {"Gungnir", 756, 6},
            {"The Prince", 852, 6},
            {"Containment Breach", 810, 6},
            {"Wildfire", 738, 5},
            {"Asiimov", 279, 5},
            {"Hyper Beast", 520, 5},
            {"Fade", 38, 6},
            {"Lightning Strike", 141, 6},
            {"Redline", 282, 4},
            {"Graphite", 51, 4},
            {"BOOM", 174, 5},
            {"Electric Hive", 212, 4},
            {"Oni Taiji", 734, 5},
            {"Neo-Noir", 770, 4},
            {"Fever Dream", 640, 4},
            {"PAW", 677, 4},
            {"Man-o'-war", 460, 4},
            {"Corticera", 313, 3},
            {"Elite Build", 422, 3},
            {"Phobos", 475, 3},
            {"Atheris", 730, 4},
            {"Pit Viper", 191, 3},
            {"Worm God", 424, 3},
            {"Sun in Leo", 285, 3},
            {"Safari Mesh", 72, 1},
            {"Snake Camo", 131, 2},
            {"Chromatic Aberration", 819, 4},
            {"Exoskeleton", 786, 4},
        });

        addWpn("Rifles", "AUG", 8, {
            {"Akihabara Accept", 455, 6},
            {"Chameleon", 499, 4},
            {"Syd Mead", 691, 5},
            {"Stymphalian", 558, 4},
            {"Fleet Flock", 578, 3},
            {"Aristocrat", 467, 3},
            {"Wings", 286, 3},
            {"Torque", 262, 3},
            {"Momentum", 661, 3},
            {"Random Access", 640, 3},
            {"Hot Rod", 445, 6},
            {"Amber Slipstream", 733, 4},
            {"Arctic Wolf", 805, 4},
            {"Storm", 100, 2},
            {"Colony", 248, 2},
            {"Contractor", 207, 1},
        });

        addWpn("Rifles", "FAMAS", 10, {
            {"Mecha Industries", 641, 5},
            {"Roll Cage", 704, 4},
            {"Decommissioned", 669, 4},
            {"Djinn", 448, 3},
            {"Afterimage", 290, 3},
            {"Valence", 362, 3},
            {"Pulse", 262, 3},
            {"Styx", 310, 3},
            {"Neural Net", 398, 3},
            {"Sergeant", 345, 2},
            {"Cyanospatter", 217, 2},
            {"Colony", 248, 2},
            {"Doomkitty", 223, 4},
            {"Commemoration", 779, 5},
            {"Eye of Athena", 767, 4},
            {"Rapid Eye Movement", 818, 4},
        });

        addWpn("Rifles", "Galil AR", 13, {
            {"Chatterbox", 398, 5},
            {"Cerberus", 379, 4},
            {"Eco", 499, 4},
            {"Sugar Rush", 519, 3},
            {"Cold Fusion", 467, 5},
            {"Crimson Tsunami", 467, 4},
            {"Stone Cold", 558, 3},
            {"Signal", 549, 3},
            {"Rocket Pop", 361, 3},
            {"Firefight", 422, 3},
            {"Kami", 279, 3},
            {"Orange DDPAT", 240, 2},
            {"Urban Rubble", 293, 2},
            {"Sage Spray", 206, 1},
            {"Hunting Blind", 181, 1},
            {"Phoenix Blacklight", 813, 5},
            {"Dusk Ruins", 735, 4},
        });

        addWpn("Rifles", "SG 553", 39, {
            {"Integrale", 700, 5},
            {"Cyrex", 360, 4},
            {"Phantom", 449, 3},
            {"Tiger Moth", 561, 3},
            {"Pulse", 262, 3},
            {"Atlas", 345, 3},
            {"Danger Close", 694, 4},
            {"Triarch", 545, 4},
            {"Bulldozer", 350, 4},
            {"Darkwing", 795, 4},
            {"Ultraviolet", 153, 2},
            {"Waves Perforated", 313, 2},
            {"Army Sheen", 222, 2},
            {"Damascus Steel", 361, 3},
        });

        // ================================================================
        // SMGS
        // ================================================================
        addWpn("SMGs", "MP9", 34, {
            {"Hydra", 695, 5},
            {"Starlight Protector", 733, 5},
            {"Wild Lily", 867, 5},
            {"Rose Iron", 589, 3},
            {"Airlock", 569, 3},
            {"Stained Glass", 475, 3},
            {"Ruby Poison Dart", 540, 3},
            {"Goo", 302, 3},
            {"Bioleak", 371, 3},
            {"Dart", 324, 3},
            {"Slide", 672, 3},
            {"Modest Threat", 780, 3},
            {"Setting Sun", 224, 2},
            {"Sand Dashed", 307, 2},
            {"Orange Peel", 163, 1},
            {"Storm", 100, 2},
            {"Hot Snakes", 833, 4},
        });

        addWpn("SMGs", "MAC-10", 17, {
            {"Neon Rider", 744, 5},
            {"Case Hardened", 44, 5},
            {"Stalker", 773, 4},
            {"Disco Tech", 694, 4},
            {"Last Dive", 765, 4},
            {"Pipe Down", 649, 4},
            {"Malachite", 357, 3},
            {"Tatter", 336, 3},
            {"Aloha", 505, 3},
            {"Heat", 384, 3},
            {"Fade", 38, 4},
            {"Carnivore", 487, 3},
            {"Nuclear Garden", 447, 3},
            {"Graven", 279, 3},
            {"Amber Fade", 51, 3},
            {"Calf Skin", 237, 2},
            {"Indigo", 136, 2},
            {"Palm", 167, 1},
            {"Silver", 217, 2},
            {"Curse", 717, 4},
            {"Toybox", 825, 5},
            {"Button Masher", 808, 4},
        });

        addWpn("SMGs", "MP7", 33, {
            {"Nemesis", 395, 5},
            {"Bloodsport", 601, 4},
            {"Fade", 38, 4},
            {"Impire", 450, 4},
            {"Skulls", 214, 3},
            {"Special Delivery", 509, 3},
            {"Akoben", 690, 3},
            {"Mischief", 637, 3},
            {"Motherboard", 820, 4},
            {"Powercore", 773, 4},
            {"Teal Blossom", 455, 4},
            {"Ocean Foam", 253, 3},
            {"Armor Core", 345, 3},
            {"Orange Peel", 163, 2},
            {"Olive Plaid", 167, 1},
            {"Forest DDPAT", 5, 1},
            {"Anodized Navy", 28, 2},
            {"Urban Hazard", 535, 3},
            {"Abyssal Apparition", 833, 4},
            {"Neon Ply", 733, 3},
        });

        addWpn("SMGs", "UMP-45", 24, {
            {"Primal Saber", 510, 5},
            {"Moonrise", 713, 4},
            {"Briefing", 549, 4},
            {"Metal Flowers", 635, 4},
            {"Exposure", 786, 4},
            {"Riot", 459, 3},
            {"Scaffold", 461, 3},
            {"Plastique", 773, 4},
            {"Corporal", 345, 3},
            {"Labyrinth", 316, 3},
            {"Facility Dark", 535, 2},
            {"Carbon Fiber", 5, 2},
            {"Mudder", 208, 1},
            {"Caramel", 167, 1},
            {"Wild Child", 703, 3},
            {"Gold Bismuth", 822, 4},
        });

        addWpn("SMGs", "P90", 19, {
            {"Asiimov", 399, 5},
            {"Death by Kitty", 219, 5},
            {"Emerald Dragon", 316, 5},
            {"Shapewood", 524, 4},
            {"Trigon", 300, 4},
            {"Cold Blooded", 281, 4},
            {"Virus", 293, 3},
            {"Baroque Red", 335, 3},
            {"Module", 395, 3},
            {"Traction", 630, 3},
            {"Elite Build", 422, 3},
            {"Nostalgia", 712, 4},
            {"Freight", 535, 3},
            {"Desert Warfare", 773, 4},
            {"Facility Negative", 535, 2},
            {"Ash Wood", 238, 2},
            {"Sand Spray", 207, 1},
            {"Leather", 148, 1},
            {"Cocoa Rampage", 833, 4},
            {"Schematic", 820, 3},
        });

        addWpn("SMGs", "PP-Bizon", 26, {
            {"Judgement of Anubis", 505, 5},
            {"High Roller", 613, 4},
            {"Blue Streak", 459, 3},
            {"Fuel Rod", 383, 3},
            {"Water Sigil", 240, 3},
            {"Photic Zone", 497, 3},
            {"Jungle Slipstream", 543, 3},
            {"Iron Work", 613, 3},
            {"Antique", 217, 3},
            {"Night Riot", 625, 3},
            {"Chemical Green", 175, 2},
            {"Rust Coat", 414, 2},
            {"Carbon Fiber", 5, 2},
            {"Sand Dashed", 307, 1},
            {"Space Cat", 803, 4},
        });

        addWpn("SMGs", "MP5-SD", 23, {
            {"Lab Rats", 684, 4},
            {"Phosphor", 632, 3},
            {"Gauss", 636, 3},
            {"Co-Processor", 773, 3},
            {"Agent", 714, 3},
            {"Acid Wash", 699, 2},
            {"Desert Strike", 292, 3},
            {"Dirt Drop", 833, 3},
            {"Kitbash", 825, 4},
            {"Liquidation", 810, 4},
        });

        // ================================================================
        // SHOTGUNS
        // ================================================================
        addWpn("Shotguns", "Nova", 35, {
            {"Hyper Beast", 520, 5},
            {"Antique", 295, 4},
            {"Ranger", 383, 3},
            {"Koi", 294, 3},
            {"Graphite", 51, 3},
            {"Rising Skull", 302, 3},
            {"Toy Soldier", 479, 3},
            {"Predator", 12, 2},
            {"Camo", 5, 1},
            {"Sand Dune", 208, 1},
            {"Wild Six", 713, 4},
            {"Clear Polymer", 773, 3},
            {"Windblown", 833, 3},
        });

        addWpn("Shotguns", "XM1014", 25, {
            {"Tranquility", 502, 4},
            {"Ziggy", 628, 4},
            {"Seasons", 264, 3},
            {"Incinegator", 713, 4},
            {"Bone Machine", 302, 3},
            {"Teclu Burner", 399, 4},
            {"Heaven Guard", 375, 3},
            {"Slipstream", 543, 3},
            {"Black Tie", 444, 3},
            {"Blue Spruce", 77, 2},
            {"Grassland", 357, 1},
            {"Entombed", 773, 4},
            {"XOXO", 833, 4},
        });

        addWpn("Shotguns", "MAG-7", 27, {
            {"Praetorian", 712, 5},
            {"SWAG-7", 578, 4},
            {"Heat", 384, 4},
            {"Petroglyph", 669, 3},
            {"Hard Water", 500, 3},
            {"Sonar", 264, 3},
            {"Memento", 440, 3},
            {"Cobalt Core", 622, 3},
            {"Irradiated Alert", 372, 3},
            {"Metallic DDPAT", 156, 2},
            {"Sand Dune", 208, 1},
            {"Monster Call", 773, 4},
            {"Cinquedea", 833, 4},
            {"Justice", 788, 4},
        });

        addWpn("Shotguns", "Sawed-Off", 29, {
            {"The Kraken", 498, 5},
            {"Wasteland Princess", 534, 4},
            {"Limelight", 510, 4},
            {"Morris", 717, 4},
            {"Devourer", 537, 4},
            {"Origami", 396, 3},
            {"Highwayman", 524, 3},
            {"Amber Fade", 51, 3},
            {"Rust Coat", 414, 2},
            {"Forest DDPAT", 5, 1},
            {"Snake Camo", 131, 1},
            {"Kiss Love", 773, 4},
            {"Black Sand", 833, 4},
        });

        // ================================================================
        // MACHINE GUNS
        // ================================================================
        addWpn("Machine Guns", "M249", 14, {
            {"Emerald Poison Dart", 580, 5},
            {"Spectre", 377, 3},
            {"System Lock", 487, 3},
            {"Nebula Crusader", 497, 3},
            {"Magma", 227, 3},
            {"Downtown", 513, 3},
            {"Impact Drill", 373, 3},
            {"Jungle DDPAT", 5, 1},
            {"Gator Mesh", 24, 2},
            {"Blizzard Marbleized", 161, 1},
            {"Deep Relief", 773, 3},
            {"Warbird", 833, 4},
        });

        addWpn("Machine Guns", "Negev", 28, {
            {"Power Loader", 755, 5},
            {"Loudmouth", 613, 4},
            {"Lionfish", 555, 4},
            {"Dazzle", 456, 3},
            {"Bratatat", 505, 3},
            {"Man-o'-war", 460, 3},
            {"Terrain", 128, 2},
            {"Army Sheen", 345, 2},
            {"CaliCamo", 246, 1},
            {"Palm", 167, 1},
            {"Prototype", 773, 3},
            {"Drop Me", 833, 4},
        });

        // ================================================================
        // SNIPERS
        // ================================================================
        addWpn("Snipers", "SSG 08", 40, {
            {"Dragonfire", 624, 5},
            {"Blood in the Water", 375, 5},
            {"Big Iron", 503, 4},
            {"Necropos", 640, 3},
            {"Ghost Crusader", 669, 4},
            {"Abyss", 302, 3},
            {"Detour", 271, 3},
            {"Dark Water", 95, 3},
            {"Slashed", 326, 3},
            {"Acid Fade", 240, 3},
            {"Mayan Dreams", 213, 2},
            {"Blue Spruce", 77, 2},
            {"Sand Dune", 208, 1},
            {"Turbo Peek", 773, 4},
            {"Spring Twilly", 833, 4},
            {"Death Strike", 810, 5},
        });

        addWpn("Snipers", "SCAR-20", 38, {
            {"Emerald", 296, 6},
            {"Cardiac", 389, 5},
            {"Bloodsport", 601, 4},
            {"Jungle Slipstream", 543, 3},
            {"Enforcer", 509, 4},
            {"Powercore", 773, 4},
            {"Cyrex", 360, 4},
            {"Grotto", 397, 3},
            {"Contractor", 207, 2},
            {"Storm", 100, 2},
            {"Sand Mesh", 74, 1},
            {"Fragments", 833, 3},
        });

        addWpn("Snipers", "G3SG1", 11, {
            {"The Executioner", 502, 5},
            {"Flux", 622, 4},
            {"Stinger", 487, 3},
            {"Ventilator", 543, 3},
            {"Scavenger", 400, 3},
            {"Orange Kimono", 467, 3},
            {"Demeter", 479, 3},
            {"Hunter", 289, 3},
            {"Contractor", 207, 2},
            {"Jungle Dashed", 307, 1},
            {"Safari Mesh", 72, 1},
            {"Digital Mesh", 833, 3},
            {"High Seas", 810, 4},
        });

        // ================================================================
        // KNIVES
        // ================================================================
        addWpn("Knives", "Karambit", 507, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Gamma Doppler", 568, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Autotronic", 619, 6},
            {"Lore", 344, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Rust Coat", 414, 6},
            {"Ultraviolet", 98, 6},
            {"Night", 100, 6},
            {"Stained", 91, 6},
            {"Scorched", 157, 6},
            {"Urban Masked", 24, 6},
            {"Boreal Forest", 77, 6},
            {"Forest DDPAT", 5, 6},
            {"Safari Mesh", 72, 6},
            {"Bright Water", 252, 6},
            {"Freehand", 620, 6},
            {"Black Laminate", 685, 6},
        });

        addWpn("Knives", "Butterfly Knife", 515, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Gamma Doppler", 568, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Autotronic", 619, 6},
            {"Lore", 344, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Rust Coat", 414, 6},
            {"Ultraviolet", 98, 6},
            {"Night", 100, 6},
            {"Stained", 91, 6},
            {"Scorched", 157, 6},
            {"Urban Masked", 24, 6},
            {"Boreal Forest", 77, 6},
            {"Forest DDPAT", 5, 6},
            {"Safari Mesh", 72, 6},
            {"Freehand", 620, 6},
            {"Black Laminate", 685, 6},
            {"Bright Water", 252, 6},
        });

        addWpn("Knives", "M9 Bayonet", 508, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Gamma Doppler", 568, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Autotronic", 619, 6},
            {"Lore", 344, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Rust Coat", 414, 6},
            {"Ultraviolet", 98, 6},
            {"Night", 100, 6},
            {"Stained", 91, 6},
            {"Scorched", 157, 6},
            {"Urban Masked", 24, 6},
            {"Boreal Forest", 77, 6},
            {"Forest DDPAT", 5, 6},
            {"Safari Mesh", 72, 6},
            {"Freehand", 620, 6},
            {"Black Laminate", 685, 6},
        });

        addWpn("Knives", "Bayonet", 500, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Gamma Doppler", 568, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Autotronic", 619, 6},
            {"Lore", 344, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Rust Coat", 414, 6},
            {"Ultraviolet", 98, 6},
            {"Night", 100, 6},
            {"Stained", 91, 6},
            {"Scorched", 157, 6},
            {"Boreal Forest", 77, 6},
            {"Forest DDPAT", 5, 6},
            {"Safari Mesh", 72, 6},
            {"Freehand", 620, 6},
            {"Black Laminate", 685, 6},
        });

        addWpn("Knives", "Flip Knife", 505, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Gamma Doppler", 568, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Autotronic", 619, 6},
            {"Lore", 344, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Rust Coat", 414, 6},
            {"Ultraviolet", 98, 6},
            {"Night", 100, 6},
        });

        addWpn("Knives", "Gut Knife", 506, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Gamma Doppler", 568, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Autotronic", 619, 6},
            {"Lore", 344, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Rust Coat", 414, 6},
        });

        addWpn("Knives", "Huntsman Knife", 509, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Rust Coat", 414, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
            {"Scorched", 157, 6},
            {"Safari Mesh", 72, 6},
        });

        addWpn("Knives", "Falchion Knife", 512, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
        });

        addWpn("Knives", "Bowie Knife", 514, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
        });

        addWpn("Knives", "Shadow Daggers", 516, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Night", 100, 6},
        });

        addWpn("Knives", "Ursus Knife", 519, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Night", 100, 6},
        });

        addWpn("Knives", "Navaja Knife", 520, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
        });

        addWpn("Knives", "Stiletto Knife", 522, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
        });

        addWpn("Knives", "Talon Knife", 523, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
        });

        addWpn("Knives", "Paracord Knife", 517, {
            {"Fade", 38, 6},
            {"Slaughter", 59, 6},
            {"Crimson Web", 44, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Stained", 91, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
        });

        addWpn("Knives", "Nomad Knife", 521, {
            {"Fade", 38, 6},
            {"Slaughter", 59, 6},
            {"Crimson Web", 44, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Stained", 91, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
        });

        addWpn("Knives", "Skeleton Knife", 525, {
            {"Fade", 38, 6},
            {"Slaughter", 59, 6},
            {"Crimson Web", 44, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Stained", 91, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
        });

        addWpn("Knives", "Kukri Knife", 526, {
            {"Fade", 38, 6},
            {"Doppler", 418, 6},
            {"Tiger Tooth", 409, 6},
            {"Marble Fade", 413, 6},
            {"Crimson Web", 44, 6},
            {"Slaughter", 59, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Damascus Steel", 410, 6},
            {"Night", 100, 6},
            {"Autotronic", 619, 6},
            {"Lore", 344, 6},
        });

        addWpn("Knives", "Survival Knife", 518, {
            {"Fade", 38, 6},
            {"Slaughter", 59, 6},
            {"Crimson Web", 44, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Stained", 91, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
            {"Forest DDPAT", 5, 6},
            {"Safari Mesh", 72, 6},
            {"Scorched", 157, 6},
        });

        addWpn("Knives", "Classic Knife", 503, {
            {"Fade", 38, 6},
            {"Crimson Web", 44, 6},
            {"Case Hardened", 44, 6},
            {"Blue Steel", 185, 6},
            {"Vanilla", 0, 6},
            {"Night", 100, 6},
            {"Boreal Forest", 77, 6},
            {"Slaughter", 59, 6},
            {"Stained", 91, 6},
            {"Urban Masked", 24, 6},
        });

        // ================================================================
        // GLOVES
        // ================================================================
        addWpn("Gloves", "Sport Gloves", 5030, {
            {"Hedge Maze", 10006, 6},
            {"Pandora's Box", 10009, 6},
            {"Superconductor", 10010, 6},
            {"Arid", 10003, 6},
            {"Vice", 10018, 6},
            {"Omega", 10026, 6},
            {"Amphibious", 10037, 6},
            {"Bronze Morph", 10039, 6},
            {"Scarlet Shamagh", 10042, 6},
            {"Nocts", 10046, 6},
        });

        addWpn("Gloves", "Driver Gloves", 5031, {
            {"Crimson Weave", 10027, 6},
            {"King Snake", 10028, 6},
            {"Imperial Plaid", 10030, 6},
            {"Overtake", 10031, 6},
            {"Racing Green", 10032, 6},
            {"Lunar Weave", 10013, 6},
            {"Diamondback", 10015, 6},
            {"Convoy", 10016, 6},
            {"Snow Leopard", 10043, 6},
            {"Queen Jaguar", 10047, 6},
        });

        addWpn("Gloves", "Hand Wraps", 5032, {
            {"Cobalt Skulls", 10036, 6},
            {"Overprint", 10040, 6},
            {"Duct Tape", 10041, 6},
            {"Arboreal", 10044, 6},
            {"Leather", 10007, 6},
            {"Spruce DDPAT", 10008, 6},
            {"Slaughter", 10021, 6},
            {"Badlands", 10023, 6},
            {"Giraffe", 10049, 6},
            {"Constrictor", 10050, 6},
            {"Desert Shamagh", 10048, 6},
        });

        addWpn("Gloves", "Moto Gloves", 5033, {
            {"Spearmint", 10024, 6},
            {"Cool Mint", 10025, 6},
            {"POW!", 10029, 6},
            {"Polygon", 10033, 6},
            {"Transport", 10034, 6},
            {"Turtle", 10035, 6},
            {"Eclipse", 10012, 6},
            {"Boom!", 10019, 6},
            {"3rd Commando Company", 10045, 6},
            {"Smoke Out", 10051, 6},
            {"Blood Pressure", 10052, 6},
        });

        addWpn("Gloves", "Specialist Gloves", 5034, {
            {"Crimson Kimono", 10014, 6},
            {"Emerald Web", 10017, 6},
            {"Foundation", 10020, 6},
            {"Mogul", 10022, 6},
            {"Fade", 10038, 6},
            {"Lt. Commander", 10053, 6},
            {"Marble Fade", 10054, 6},
            {"Tiger Strike", 10055, 6},
            {"Field Agent", 10056, 6},
            {"Buckshot", 10057, 6},
        });

        addWpn("Gloves", "Bloodhound Gloves", 5027, {
            {"Charred", 10004, 6},
            {"Snakebite", 10005, 6},
            {"Bronzed", 10006, 6},
            {"Guerrilla", 10011, 6},
        });

        addWpn("Gloves", "Hydra Gloves", 5035, {
            {"Case Hardened", 10058, 6},
            {"Emerald", 10059, 6},
            {"Mangrove", 10060, 6},
            {"Rattler", 10061, 6},
        });

        addWpn("Gloves", "Broken Fang Gloves", 4725, {
            {"Yellow-banded", 10062, 6},
            {"Jade", 10063, 6},
            {"Unhinged", 10064, 6},
            {"Needle Point", 10065, 6},
        });

        LogMsg("Skin database initialized: %zu weapons total", G::allWeapons.size());
    }
}
