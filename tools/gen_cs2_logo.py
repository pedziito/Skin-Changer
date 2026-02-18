#!/usr/bin/env python3
"""
Generate a pixel-accurate CS (Counter-Strike) logo at 64x64.
Classic design: orange left half, blue right half, white soldier silhouette.
Renders at 4x then downscales with LANCZOS for smooth anti-aliased edges.
Output: assets/cs2_logo_data.h with embedded PNG bytes.
"""
import os, io

from PIL import Image, ImageDraw

# Render at 4x for anti-aliasing
SCALE = 4
W_HI, H_HI = 64 * SCALE, 64 * SCALE
W_OUT, H_OUT = 64, 64

# --- Create high-res image ---
img = Image.new('RGBA', (W_HI, H_HI), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

# Background: left orange, right blue
ORANGE = (234, 170, 0, 255)
BLUE   = (36, 55, 120, 255)
draw.rectangle([0, 0, W_HI // 2, H_HI], fill=ORANGE)
draw.rectangle([W_HI // 2, 0, W_HI, H_HI], fill=BLUE)

WHITE = (255, 255, 255, 255)

# --- Soldier silhouette ---
# Classic CS logo: soldier facing right, holding rifle, dynamic stance
# All coords in 0-256 space (will be used directly at 4x scale = 256px)

# Head (circle)
hcx, hcy, hr = 125, 28, 16
draw.ellipse([hcx - hr, hcy - hr, hcx + hr, hcy + hr], fill=WHITE)

# Main body polygon — carefully traced from the classic CS logo
body = [
    # Neck to right shoulder
    (124, 44), (130, 46), (136, 44),
    # Right arm extends forward-right holding gun
    (142, 40), (150, 36), (158, 34), (166, 32),
    (174, 30), (182, 28), (190, 26),
    # Gun barrel
    (200, 24), (210, 22), (220, 20), (230, 18),
    # Barrel tip (top)
    (234, 22),
    # Gun underside back
    (228, 26), (218, 28), (208, 30),
    (198, 32), (188, 36), (178, 40),
    # Right forearm / hand / trigger guard
    (170, 46), (166, 52), (162, 56),
    # Magazine / stock area
    (156, 52), (152, 46), (148, 44),
    (150, 56), (152, 64),
    # Right torso
    (150, 76), (148, 88), (146, 100),
    (148, 112), (150, 120),
    # Right hip
    (152, 126),
    # Right leg (extends back-right)
    (156, 134), (162, 148), (168, 160),
    (174, 172), (180, 184), (186, 196),
    (192, 208), (196, 218), (200, 226),
    (204, 234), (208, 240), (212, 246),
    # Right foot
    (216, 250), (220, 254), (222, 256),
    # Right foot sole
    (214, 256), (208, 252),
    (200, 244), (194, 234), (188, 222),
    (182, 210), (176, 198), (170, 186),
    (164, 174), (158, 162), (152, 150),
    (148, 140), (144, 132),
    # Crotch
    (136, 128), (130, 126), (126, 130),
    # Left leg (extends forward-left)
    (122, 138), (118, 148), (114, 158),
    (108, 170), (102, 182), (96, 192),
    (88, 204), (80, 214), (72, 224),
    (64, 232), (56, 238), (48, 242),
    # Left foot
    (40, 246), (34, 250), (30, 254), (28, 256),
    # Left foot sole
    (38, 256), (48, 252), (56, 246),
    (64, 238), (72, 228), (80, 218),
    (88, 208), (96, 196), (104, 184),
    (110, 172), (114, 162), (118, 150),
    (120, 140), (120, 132),
    # Left hip up torso
    (118, 120), (114, 108), (110, 96),
    (106, 84), (102, 72), (100, 64),
    # Left shoulder area
    (98, 58), (96, 54),
    # Left arm (tucked, gripping)
    (90, 50), (84, 48), (78, 50),
    (74, 56), (72, 62), (74, 66),
    (78, 64), (84, 58), (90, 54),
    (96, 52),
    # Back to neck
    (100, 50), (106, 48), (112, 46), (118, 44),
]

draw.polygon(body, fill=WHITE)

# --- Downscale with high-quality anti-aliasing ---
img_out = img.resize((W_OUT, H_OUT), Image.LANCZOS)

# --- Save PNG ---
os.makedirs('/workspaces/Skin-Changer/assets', exist_ok=True)
img_out.save('/workspaces/Skin-Changer/assets/cs2_logo.png')

# --- Get PNG bytes ---
buf = io.BytesIO()
img_out.save(buf, format='PNG', optimize=True)
png_data = buf.getvalue()

# --- Generate C header ---
lines = []
lines.append('// CS logo — 64x64 RGBA PNG embedded as C array')
lines.append('// Classic orange/blue split with soldier silhouette')
lines.append('// Compiled into binary — works on all machines')
lines.append('#pragma once')
lines.append('')
lines.append(f'static const unsigned int g_cs2LogoPngSize = {len(png_data)};')
lines.append(f'static const unsigned char g_cs2LogoPng[{len(png_data)}] = {{')

for i in range(0, len(png_data), 16):
    chunk_bytes = png_data[i:i+16]
    hex_str = ', '.join(f'0x{b:02x}' for b in chunk_bytes)
    if i + 16 < len(png_data):
        hex_str += ','
    lines.append(f'    {hex_str}')

lines.append('};')
lines.append('')

header_path = '/workspaces/Skin-Changer/assets/cs2_logo_data.h'
with open(header_path, 'w') as f:
    f.write('\n'.join(lines))

print(f'PNG: {len(png_data)} bytes ({W_OUT}x{H_OUT})')
print(f'Header: {header_path}')
print('Done!')
