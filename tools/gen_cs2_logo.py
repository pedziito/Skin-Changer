#!/usr/bin/env python3
"""
Generate a CS2-style logo as a PNG and convert to C header.
The logo: orange/blue split background with a simplified soldier silhouette.
Output: assets/cs2_logo_data.h with embedded PNG bytes.
"""
import struct, zlib, os, math

W, H = 48, 48

# Colors
ORANGE = (234, 168, 0, 255)
BLUE   = (36, 59, 128, 255)
WHITE  = (255, 255, 255, 255)
TRANS  = (0, 0, 0, 0)

# Create pixel buffer
pixels = [[TRANS for _ in range(W)] for _ in range(H)]

# Background: left half orange, right half blue
for y in range(H):
    for x in range(W):
        if x < W // 2:
            pixels[y][x] = ORANGE
        else:
            pixels[y][x] = BLUE

# Simplified soldier silhouette - drawn as white polygon
# The classic CS logo soldier is facing right, holding a gun
# We'll approximate with geometric shapes

def fill_rect(x1, y1, x2, y2, color):
    for y in range(max(0, y1), min(H, y2)):
        for x in range(max(0, x1), min(W, x2)):
            pixels[y][x] = color

def fill_circle(cx, cy, r, color):
    for y in range(max(0, int(cy-r-1)), min(H, int(cy+r+2))):
        for x in range(max(0, int(cx-r-1)), min(W, int(cx+r+2))):
            dx, dy = x - cx, y - cy
            if dx*dx + dy*dy <= r*r:
                pixels[y][x] = color

def fill_polygon(points, color):
    """Simple scanline polygon fill"""
    min_y = max(0, int(min(p[1] for p in points)))
    max_y = min(H-1, int(max(p[1] for p in points)))
    for y in range(min_y, max_y + 1):
        intersections = []
        n = len(points)
        for i in range(n):
            x1, y1 = points[i]
            x2, y2 = points[(i+1) % n]
            if y1 == y2:
                continue
            if y < min(y1, y2) or y >= max(y1, y2):
                continue
            x_intersect = x1 + (y - y1) * (x2 - x1) / (y2 - y1)
            intersections.append(x_intersect)
        intersections.sort()
        for i in range(0, len(intersections) - 1, 2):
            x_start = max(0, int(intersections[i]))
            x_end = min(W, int(intersections[i+1]) + 1)
            for x in range(x_start, x_end):
                pixels[y][x] = color

# Draw the soldier silhouette (simplified, centered)
# Head
fill_circle(24, 8, 4, WHITE)

# Neck
fill_rect(22, 12, 26, 15, WHITE)

# Torso (slightly angled, leaning forward)
fill_polygon([
    (18, 15), (30, 15),
    (31, 30), (16, 32)
], WHITE)

# Left arm (holding gun forward) - extends right
fill_polygon([
    (30, 16), (38, 14),
    (40, 16), (30, 20)
], WHITE)

# Gun (extending from right hand)
fill_polygon([
    (36, 12), (44, 10),
    (45, 13), (38, 15)
], WHITE)
# Gun barrel
fill_rect(42, 10, 47, 12, WHITE)

# Right arm (back)
fill_polygon([
    (18, 16), (14, 22),
    (16, 24), (20, 18)
], WHITE)

# Left leg (forward stride)
fill_polygon([
    (18, 30), (24, 30),
    (16, 44), (10, 44)
], WHITE)

# Left foot
fill_polygon([
    (8, 43), (17, 43),
    (17, 47), (6, 47)
], WHITE)

# Right leg (back stride)
fill_polygon([
    (24, 30), (31, 30),
    (38, 44), (32, 44)
], WHITE)

# Right foot
fill_polygon([
    (31, 43), (40, 43),
    (42, 47), (30, 47)
], WHITE)


# --- Encode as PNG ---
def make_png(width, height, pixels):
    def chunk(chunk_type, data):
        c = chunk_type + data
        crc = struct.pack('>I', zlib.crc32(c) & 0xffffffff)
        return struct.pack('>I', len(data)) + c + crc

    # Signature
    sig = b'\x89PNG\r\n\x1a\n'

    # IHDR
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)  # 8-bit RGBA
    ihdr = chunk(b'IHDR', ihdr_data)

    # IDAT
    raw = b''
    for y in range(height):
        raw += b'\x00'  # filter: none
        for x in range(width):
            r, g, b, a = pixels[y][x]
            raw += struct.pack('BBBB', r, g, b, a)
    compressed = zlib.compress(raw, 9)
    idat = chunk(b'IDAT', compressed)

    # IEND
    iend = chunk(b'IEND', b'')

    return sig + ihdr + idat + iend

png_data = make_png(W, H, pixels)

# Save PNG for preview
os.makedirs('/workspaces/Skin-Changer/assets', exist_ok=True)
with open('/workspaces/Skin-Changer/assets/cs2_logo.png', 'wb') as f:
    f.write(png_data)

# Generate C header
lines = []
lines.append('// Auto-generated CS2 logo PNG data (48x48 RGBA)')
lines.append('// Embedded in binary — works on all machines')
lines.append('#pragma once')
lines.append('')
lines.append(f'static const unsigned int g_cs2LogoPngSize = {len(png_data)};')
lines.append(f'static const unsigned char g_cs2LogoPng[{len(png_data)}] = {{')

# Format as hex bytes, 16 per line
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

print(f'PNG: {len(png_data)} bytes')
print(f'Header: {header_path}')
print('Done!')
