#!/usr/bin/env python3
"""
Generate the AC logo matching the brand reference:
  - Letters "AC" in bold
  - Diagonal gradient: dark navy (top-left) → bright cyan (bottom-right)
  - Transparent background, hard alpha edges (no semi-transparent pixels)
  - Output: 128x128 RGBA PNG + embedded C header
"""

from PIL import Image, ImageDraw, ImageFont
import struct, io, os, sys

SIZE = 512        # render at high-res for quality
OUT_SIZE = 128    # final size for embedding

def find_bold_font():
    """Find a suitable bold font on the system."""
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
    ]
    for f in candidates:
        if os.path.exists(f):
            return f
    return None

def lerp_color(c1, c2, t):
    """Lerp between two RGBA colors."""
    return tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(len(c1)))

def generate_logo():
    # High-res canvas
    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Find a bold font
    font_path = find_bold_font()
    if font_path is None:
        print("ERROR: No bold font found on system")
        sys.exit(1)

    # Determine font size to fill ~75% of canvas
    font_size = 280
    font = ImageFont.truetype(font_path, font_size)
    text = "AC"
    bbox = draw.textbbox((0, 0), text, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]

    while tw > SIZE * 0.82 and font_size > 20:
        font_size -= 4
        font = ImageFont.truetype(font_path, font_size)
        bbox = draw.textbbox((0, 0), text, font=font)
        tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]

    while tw < SIZE * 0.65 and font_size < 400:
        font_size += 4
        font = ImageFont.truetype(font_path, font_size)
        bbox = draw.textbbox((0, 0), text, font=font)
        tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]

    # Center text
    tx = (SIZE - tw) // 2 - bbox[0]
    ty = (SIZE - th) // 2 - bbox[1]

    # Step 1: Render text in white to get a mask
    mask_img = Image.new('L', (SIZE, SIZE), 0)
    mask_draw = ImageDraw.Draw(mask_img)
    mask_draw.text((tx, ty), text, fill=255, font=font)

    # Step 2: Create DIAGONAL gradient (top-left=dark navy, bottom-right=bright cyan)
    gradient = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))

    # Color stops matching the reference:
    # Top-left:  dark navy (#0a1e5e)
    # Center:    medium blue (#1565c0)  
    # Bottom-right: bright cyan (#29b8d4)
    c_dark  = (10, 30, 94, 255)    # Dark navy
    c_mid   = (21, 101, 192, 255)  # Medium blue
    c_light = (41, 184, 212, 255)  # Bright cyan

    for y in range(SIZE):
        for x in range(SIZE):
            # Diagonal parameter: 0 at top-left, 1 at bottom-right
            t = (x + y) / (2.0 * (SIZE - 1))
            if t < 0.45:
                c = lerp_color(c_dark, c_mid, t / 0.45)
            else:
                c = lerp_color(c_mid, c_light, (t - 0.45) / 0.55)
            gradient.putpixel((x, y), c)

    # Step 3: Apply text mask to gradient
    result = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    result.paste(gradient, mask=mask_img)

    # Downsample with LANCZOS
    result = result.resize((OUT_SIZE, OUT_SIZE), Image.LANCZOS)

    # Hard threshold alpha — no semi-transparent pixels
    # This prevents any color-key fringe in the overlay
    pixels = result.load()
    for y in range(OUT_SIZE):
        for x in range(OUT_SIZE):
            r, g, b, a = pixels[x, y]
            if a < 100:
                pixels[x, y] = (0, 0, 0, 0)
            else:
                pixels[x, y] = (r, g, b, 255)

    return result

def save_header(img, header_path):
    """Save image as C header with embedded PNG bytes."""
    buf = io.BytesIO()
    img.save(buf, format='PNG', optimize=True)
    png_data = buf.getvalue()

    lines = []
    lines.append('// AC Glow Logo — transparent background, blue gradient')
    lines.append(f'// {len(png_data)} bytes, {img.width}x{img.height} RGBA PNG')
    lines.append(f'static const unsigned char g_acGlowPng[] = {{')

    # Format as hex bytes, 16 per line
    for i in range(0, len(png_data), 16):
        chunk = png_data[i:i+16]
        hex_vals = ','.join(f'0x{b:02x}' for b in chunk)
        lines.append(f'    {hex_vals},')

    lines.append('};')
    lines.append(f'static const unsigned int g_acGlowPngSize = {len(png_data)};')
    lines.append('')

    with open(header_path, 'w') as f:
        f.write('\n'.join(lines))
    
    print(f"Written {header_path}: {len(png_data)} bytes, {img.width}x{img.height}")

def main():
    img = generate_logo()
    
    # Save PNG for preview
    png_path = '/workspaces/Skin-Changer/assets/ac_glow_logo.png'
    img.save(png_path)
    print(f"Saved preview: {png_path}")

    # Save C header
    header_path = '/workspaces/Skin-Changer/assets/ac_glow_logo_data.h'
    save_header(img, header_path)

if __name__ == '__main__':
    main()
