#!/usr/bin/env python3
"""
Download CS2 skin images from csgodatabase.com and convert webp → PNG.
Stores in assets/skins/ as WeaponName_SkinName.png
Also generates a C header with embedded PNG data for key images.
"""
import os
import sys
import time
import requests
from io import BytesIO

try:
    from PIL import Image
except ImportError:
    print("pip install Pillow")
    sys.exit(1)

BASE_URL = "https://www.csgodatabase.com/images/skins/webp/"
OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "assets", "skins")

# All weapons and skins from our embedded database
WEAPONS = {
    # Pistols
    "Glock-18": ["Fade", "Water Elemental", "Wasteland Rebel", "Gamma Doppler", "Twilight Galaxy",
                 "Bullet Queen", "Moonrise", "Neo-Noir", "Vogue", "Dragon Tattoo", "Reactor",
                 "Candy Apple", "Grinder", "Steel Disruption", "Sand Dune"],
    "USP-S": ["Kill Confirmed", "Printstream", "The Traitor", "Orion", "Neo-Noir", "Road Rash",
              "Ticket to Hell", "Cortex", "Caiman", "Overgrowth", "Whiteout", "Blueprint",
              "Serum", "Stainless", "Guardian"],
    "Desert Eagle": ["Blaze", "Code Red", "Printstream", "Kumicho Dragon", "Mecha Industries",
                     "Trigger Discipline", "Heirloom", "Golden Koi", "Sunset Storm", "Ocean Drive",
                     "Fennec Fox", "Crimson Web", "Hypnotic", "Conspiracy", "Oxide Blaze"],
    "P250": ["See Ya Later", "Asiimov", "Muertos", "Cartel", "Supernova", "Mehndi",
             "Vino Primo", "Steel Disruption", "Splash"],
    "Five-SeveN": ["Hyper Beast", "Angry Mob", "Monkey Business", "Retrobution", "Copper Galaxy",
                   "Neon Kimono", "Flame Test", "Triumvirate", "Orange Peel"],
    "Tec-9": ["Fuel Injector", "Decimator", "Re-Entry", "Avalanche", "Titanium Bit", "Blue Titanium"],
    "Dual Berettas": ["Twin Turbo", "Cobra Strike", "Royal Consorts", "Hemoglobin", "Urban Shock", "Shred"],
    "CZ75-Auto": ["Victoria", "Xiangliu", "Red Astor", "Emerald", "Polymer", "Tuxedo"],
    "R8 Revolver": ["Fade", "Amber Fade", "Grip", "Bone Mask", "Crimson Web", "Llama Cannon"],
    
    # Rifles
    "AK-47": ["Wild Lotus", "Fire Serpent", "Fuel Injector", "Neon Revolution", "Bloodsport",
              "The Empress", "Vulcan", "Asiimov", "Neon Rider", "Phantom Disruptor", "Redline",
              "Frontside Misty", "Aquamarine Revenge", "Point Disarray", "Slate", "Blue Laminate",
              "Elite Build", "Rat Rod", "Safari Mesh", "Predator"],
    "M4A4": ["Howl", "Asiimov", "Poseidon", "The Emperor", "Neo-Noir", "Desolate Space",
             "Buzz Kill", "Royal Paladin", "Hellfire", "In Living Color", "Dragon King",
             "Evil Daimyo", "Cyber Security", "Faded Zebra", "Urban DDPAT"],
    "M4A1-S": ["Printstream", "Chantico's Fire", "Hyper Beast", "Golden Coil", "Mecha Industries",
               "Decimator", "Nightmare", "Leaded Glass", "Atomic Alloy", "Guardian", "Bright Water",
               "Nitro", "Flashback", "Boreal Forest"],
    "AWP": ["Dragon Lore", "Gungnir", "Fade", "Lightning Strike", "Asiimov", "Hyper Beast",
            "Containment Breach", "Fever Dream", "Redline", "Chromatic Aberration", "Atheris",
            "PAW", "Electric Hive", "Elite Build", "Pit Viper", "Safari Mesh"],
    "SG 553": ["Integrale", "Cyrex", "Pulse", "Phantom", "Aerial", "Tiger Moth"],
    "AUG": ["Akihabara Accept", "Chameleon", "Syd Mead", "Stymphalian", "Fleet Flock", "Midnight Lily"],
    "FAMAS": ["Mecha Industries", "Roll Cage", "Djinn", "Neural Net", "Afterimage", "Decommissioned"],
    "Galil AR": ["Chatterbox", "Cerberus", "Firefight", "Rocket Pop", "Stone Cold", "Sage Spray"],
    
    # SMGs
    "MP9": ["Hydra", "Starlight Protector", "Rose Iron", "Bioleak", "Dark Age"],
    "MAC-10": ["Neon Rider", "Disco Tech", "Stalker", "Heat", "Pipe Down", "Silver"],
    "UMP-45": ["Primal Saber", "Momentum", "Briefing", "Plastique", "Corporal", "Caramel"],
    "MP7": ["Nemesis", "Fade", "Bloodsport", "Impire", "Akoben", "Special Delivery"],
    "P90": ["Asiimov", "Death by Kitty", "Shapewood", "Trigon", "Module", "Elite Build"],
    "PP-Bizon": ["Judgement of Anubis", "Fuel Rod", "Blue Streak", "Night Ops", "Sand Dashed"],
    
    # Heavy
    "Nova": ["Hyper Beast", "Antique", "Bloomstick", "Toy Soldier", "Koi", "Sand Dune"],
    "XM1014": ["Ziggy", "Incinegator", "Tranquility", "Teclu Burner", "Slipstream", "Blue Steel"],
    "MAG-7": ["Justice", "SWAG-7", "Praetorian", "Heat", "Metallic DDPAT", "Sand Dune"],
    "Negev": ["Mjolnir", "Power Loader", "Dazzle", "Bratatat", "Desert Strike", "Army Sheen"],
    "M249": ["Emerald Poison Dart", "Spectre", "System Lock", "Downtown", "Gator Mesh"],
    
    # Knives
    "Karambit": ["Fade", "Doppler", "Tiger Tooth", "Marble Fade", "Gamma Doppler", "Autotronic",
                 "Lore", "Crimson Web", "Slaughter", "Case Hardened", "Blue Steel", "Night",
                 "Vanilla", "Stained", "Urban Masked", "Boreal Forest", "Scorched", "Safari Mesh"],
    "Butterfly Knife": ["Fade", "Doppler", "Tiger Tooth", "Marble Fade", "Gamma Doppler", "Lore",
                        "Slaughter", "Crimson Web", "Case Hardened", "Blue Steel", "Night", "Vanilla",
                        "Boreal Forest", "Scorched", "Safari Mesh"],
    "Bayonet": ["Fade", "Doppler", "Tiger Tooth", "Marble Fade", "Lore", "Autotronic",
                "Slaughter", "Crimson Web", "Case Hardened", "Blue Steel", "Vanilla", "Boreal Forest"],
    "M9 Bayonet": ["Fade", "Doppler", "Tiger Tooth", "Marble Fade", "Gamma Doppler", "Lore",
                   "Crimson Web", "Slaughter", "Case Hardened", "Blue Steel", "Vanilla", "Night"],
    "Flip Knife": ["Fade", "Doppler", "Tiger Tooth", "Marble Fade", "Lore", "Autotronic",
                   "Slaughter", "Crimson Web", "Vanilla", "Boreal Forest", "Scorched"],
    "Huntsman Knife": ["Fade", "Doppler", "Tiger Tooth", "Lore", "Slaughter", "Crimson Web",
                       "Case Hardened", "Blue Steel", "Vanilla"],
    "Gut Knife": ["Fade", "Doppler", "Tiger Tooth", "Crimson Web", "Slaughter", "Vanilla",
                  "Night", "Boreal Forest"],
    "Falchion Knife": ["Fade", "Doppler", "Lore", "Crimson Web", "Slaughter", "Case Hardened",
                       "Vanilla", "Boreal Forest"],
    "Shadow Daggers": ["Fade", "Doppler", "Crimson Web", "Slaughter", "Case Hardened", "Vanilla",
                       "Blue Steel", "Night"],
    "Bowie Knife": ["Fade", "Doppler", "Crimson Web", "Slaughter", "Vanilla", "Blue Steel"],
    "Navaja Knife": ["Fade", "Doppler", "Tiger Tooth", "Crimson Web", "Vanilla", "Boreal Forest"],
    "Stiletto Knife": ["Fade", "Doppler", "Tiger Tooth", "Crimson Web", "Vanilla", "Blue Steel"],
    "Talon Knife": ["Fade", "Doppler", "Tiger Tooth", "Marble Fade", "Crimson Web", "Vanilla"],
    "Ursus Knife": ["Fade", "Doppler", "Tiger Tooth", "Crimson Web", "Vanilla", "Night"],
    "Classic Knife": ["Fade", "Crimson Web", "Slaughter", "Case Hardened", "Vanilla", "Blue Steel"],
    "Paracord Knife": ["Fade", "Crimson Web", "Slaughter", "Vanilla", "Boreal Forest", "Stained"],
    "Survival Knife": ["Fade", "Crimson Web", "Slaughter", "Vanilla", "Night", "Boreal Forest"],
    "Nomad Knife": ["Fade", "Crimson Web", "Slaughter", "Vanilla", "Blue Steel", "Boreal Forest"],
    "Skeleton Knife": ["Fade", "Crimson Web", "Slaughter", "Case Hardened", "Vanilla", "Stained"],
    
    # Gloves
    "Sport Gloves": ["Pandora's Box", "Hedge Maze", "Superconductor", "Arid", "Vice", "Omega",
                     "Amphibious", "Bronze Morph", "Slingshot", "Scarlet Shamagh"],
    "Driver Gloves": ["Crimson Weave", "Imperial Plaid", "Lunar Weave", "Diamondback",
                      "King Snake", "Queen Jaguar", "Overtake", "Racing Green"],
    "Hand Wraps": ["Cobalt Skulls", "Overprint", "Slaughter", "Leather", "Duct Tape", "Arboreal",
                   "Constrictor", "Desert Shamagh"],
    "Moto Gloves": ["Spearmint", "Cool Mint", "POW!", "Boom!", "Eclipse", "Polygon",
                    "Transport", "Turtle"],
    "Specialist Gloves": ["Crimson Kimono", "Emerald Web", "Foundation", "Mogul", "Fade",
                          "Marble Fade", "Forest DDPAT", "Buckshot"],
    "Hydra Gloves": ["Emerald", "Case Hardened", "Rattler", "Mangrove"],
    "Broken Fang Gloves": ["Jade", "Yellow-banded", "Unhinged", "Needle Point"],
}

def make_filename(weapon: str, skin: str) -> str:
    """Convert weapon + skin name to csgodatabase.com URL filename pattern."""
    # Weapon name transformations
    w = weapon.replace("-", "-")  # keep hyphens
    w = w.replace(" ", "_")
    
    # Skin name transformations  
    s = skin.replace(" ", "_")
    s = s.replace("'", "")  # Chantico's → Chanticos
    
    return f"{w}_{s}"

def download_skin(weapon: str, skin: str, retries: int = 2) -> bool:
    """Download a single skin image, convert webp → PNG."""
    fname = make_filename(weapon, skin)
    out_path = os.path.join(OUT_DIR, f"{fname}.png")
    
    if os.path.exists(out_path):
        return True  # Already downloaded
    
    url = f"{BASE_URL}{fname}.webp"
    
    for attempt in range(retries):
        try:
            r = requests.get(url, timeout=10, headers={
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0.0.0'
            })
            if r.status_code == 200:
                img = Image.open(BytesIO(r.content))
                img = img.convert("RGBA")
                # Resize to 256x192 for reasonable file size
                img = img.resize((256, 192), Image.LANCZOS)
                img.save(out_path, "PNG", optimize=True)
                return True
            elif r.status_code == 404:
                # Try alternative naming conventions
                alt_names = [
                    fname.replace("-", ""),
                    fname.replace("_", "-"),
                    fname.replace("★_", ""),
                ]
                for alt in alt_names:
                    alt_url = f"{BASE_URL}{alt}.webp"
                    r2 = requests.get(alt_url, timeout=10, headers={
                        'User-Agent': 'Mozilla/5.0'
                    })
                    if r2.status_code == 200:
                        img = Image.open(BytesIO(r2.content))
                        img = img.convert("RGBA")
                        img = img.resize((256, 192), Image.LANCZOS)
                        img.save(out_path, "PNG", optimize=True)
                        return True
                return False
        except Exception as e:
            if attempt < retries - 1:
                time.sleep(1)
    return False

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    
    total = sum(len(skins) for skins in WEAPONS.values())
    downloaded = 0
    failed = []
    skipped = 0
    
    print(f"Downloading {total} skin images to {OUT_DIR}")
    print(f"Source: {BASE_URL}")
    print()
    
    for weapon, skins in WEAPONS.items():
        for skin in skins:
            fname = make_filename(weapon, skin)
            out_path = os.path.join(OUT_DIR, f"{fname}.png")
            
            if os.path.exists(out_path):
                skipped += 1
                downloaded += 1
                continue
            
            ok = download_skin(weapon, skin)
            if ok:
                downloaded += 1
                print(f"  ✓ {weapon} | {skin}")
            else:
                failed.append(f"{weapon} | {skin}")
                print(f"  ✗ {weapon} | {skin}")
            
            # Be nice to the server
            time.sleep(0.15)
    
    print()
    print(f"Results: {downloaded}/{total} downloaded ({skipped} cached), {len(failed)} failed")
    if failed:
        print(f"\nFailed ({len(failed)}):")
        for f in failed[:20]:
            print(f"  - {f}")
        if len(failed) > 20:
            print(f"  ... and {len(failed)-20} more")

if __name__ == "__main__":
    main()
