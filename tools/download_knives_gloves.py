8#!/usr/bin/env python3
"""Download missing knife and glove images from csgodatabase.com."""
import os
import sys
import time
import requests
from io import BytesIO
from PIL import Image

BASE = "https://www.csgodatabase.com/images"
OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "assets", "skins")

KNIVES = {
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
}

GLOVES = {
    "Sport Gloves": ["Pandoras Box", "Hedge Maze", "Superconductor", "Arid", "Vice", "Omega",
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

def fname(weapon, skin):
    w = weapon.replace(" ", "_").replace("-", "-")
    s = skin.replace(" ", "_").replace("'", "")
    return f"{w}_{s}"

def download(url_cat, weapon, skin):
    fn = fname(weapon, skin)
    out = os.path.join(OUT_DIR, f"{fn}.png")
    if os.path.exists(out):
        return True
    url = f"{BASE}/{url_cat}/webp/{fn}.webp"
    try:
        r = requests.get(url, timeout=10, headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0.0.0'})
        if r.status_code == 200:
            img = Image.open(BytesIO(r.content)).convert("RGBA")
            img = img.resize((256, 192), Image.LANCZOS)
            img.save(out, "PNG", optimize=True)
            return True
    except Exception as e:
        print(f"  ERR: {e}")
    return False

os.makedirs(OUT_DIR, exist_ok=True)
total = sum(len(v) for v in KNIVES.values()) + sum(len(v) for v in GLOVES.values())
ok = 0
fail = []

print(f"Downloading {total} knife/glove images...")
for weapon, skins in KNIVES.items():
    for skin in skins:
        if download("knives", weapon, skin):
            ok += 1
            print(f"  ✓ {weapon} | {skin}")
        else:
            fail.append(f"{weapon} | {skin}")
            print(f"  ✗ {weapon} | {skin}")
        time.sleep(0.1)

for weapon, skins in GLOVES.items():
    for skin in skins:
        if download("gloves", weapon, skin):
            ok += 1
            print(f"  ✓ {weapon} | {skin}")
        else:
            fail.append(f"{weapon} | {skin}")
            print(f"  ✗ {weapon} | {skin}")
        time.sleep(0.1)

print(f"\nResults: {ok}/{total} downloaded, {len(fail)} failed")
if fail:
    print(f"Failed: {fail[:10]}")
