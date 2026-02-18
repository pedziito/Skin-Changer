#!/usr/bin/env python3
"""
Generate the AC logo PNG matching the purple tropical sunset theme.
Creates a 512x512 image with:
- Purple/blue gradient sky with rays
- Palm tree silhouettes
- Water reflection
- "AC" text centered with glow
- Rounded corners for use as icon
"""

from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math, random, struct, os

W, H = 512, 512

def lerp_color(c1, c2, t):
    return tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(len(c1)))

def gen_logo():
    img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Sky gradient - deep purple to bright center glow
    sky_top = (45, 20, 85)
    sky_mid = (80, 50, 140)
    sky_bright = (220, 200, 245)
    sky_low = (140, 90, 180)
    water_top = (100, 130, 200)
    water_bottom = (30, 50, 120)

    horizon_y = H * 0.62

    for y in range(H):
        if y < horizon_y:
            # Sky
            t = y / horizon_y
            if t < 0.3:
                c = lerp_color(sky_top, sky_mid, t / 0.3)
            elif t < 0.6:
                c = lerp_color(sky_mid, sky_bright, (t - 0.3) / 0.3)
            else:
                c = lerp_color(sky_bright, sky_low, (t - 0.6) / 0.4)
        else:
            # Water
            t = (y - horizon_y) / (H - horizon_y)
            c = lerp_color(water_top, water_bottom, t)

        draw.line([(0, y), (W, y)], fill=c)

    # Central bright glow
    glow = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow)
    cx, cy = W // 2, int(horizon_y * 0.75)
    for r in range(250, 0, -1):
        alpha = int(180 * (1.0 - r / 250.0) ** 2)
        glow_draw.ellipse([cx - r, cy - r * 0.7, cx + r, cy + r * 0.7],
                          fill=(255, 240, 255, alpha))
    img = Image.alpha_composite(img, glow)
    draw = ImageDraw.Draw(img)

    # Light rays from center
    rays = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    rays_draw = ImageDraw.Draw(rays)
    random.seed(42)
    for _ in range(12):
        angle = random.uniform(-0.8, 0.8)
        spread = random.uniform(0.02, 0.06)
        length = random.uniform(300, 480)
        alpha = random.randint(15, 40)
        x1 = cx + math.sin(angle - spread) * length
        y1 = cy - math.cos(angle - spread) * length
        x2 = cx + math.sin(angle + spread) * length
        y2 = cy - math.cos(angle + spread) * length
        rays_draw.polygon([(cx, cy), (x1, y1), (x2, y2)],
                          fill=(220, 200, 255, alpha))
    rays = rays.filter(ImageFilter.GaussianBlur(3))
    img = Image.alpha_composite(img, rays)
    draw = ImageDraw.Draw(img)

    # Stars
    random.seed(99)
    for _ in range(60):
        sx = random.randint(0, W)
        sy = random.randint(0, int(horizon_y * 0.8))
        sa = random.randint(80, 220)
        ss = random.randint(1, 2)
        draw.ellipse([sx, sy, sx + ss, sy + ss], fill=(255, 255, 255, sa))

    # Palm tree silhouettes
    palm_color = (20, 12, 35, 230)

    def draw_palm(px, py, trunk_h, lean, scale=1.0):
        """Draw a simple palm tree silhouette"""
        # Trunk
        for i in range(int(trunk_h)):
            t = i / trunk_h
            tx = px + lean * t * t
            tw = max(1, int((3 - t * 1.5) * scale))
            draw.line([(tx - tw, py - i), (tx + tw, py - i)], fill=palm_color)

        # Fronds (leaf clusters at top)
        top_x = px + lean
        top_y = py - trunk_h
        frond_len = int(40 * scale)

        for angle_deg in [-150, -120, -80, -40, 0, 30, 60, 100, 140]:
            a = math.radians(angle_deg)
            segments = 12
            droop = 0.04
            cur_x, cur_y = top_x, top_y
            for s in range(segments):
                t = s / segments
                actual_angle = a + droop * s * s
                cur_x += math.cos(actual_angle) * frond_len / segments
                cur_y += math.sin(actual_angle) * frond_len / segments
                fw = max(1, int((1.0 - t) * 3 * scale))
                draw.ellipse([cur_x - fw, cur_y - fw, cur_x + fw, cur_y + fw],
                             fill=palm_color)

    # Left island
    draw.ellipse([0, int(horizon_y) - 8, 130, int(horizon_y) + 15], fill=palm_color)
    draw_palm(30, int(horizon_y) - 3, 80, -15, 0.8)
    draw_palm(60, int(horizon_y) - 5, 95, 10, 0.9)
    draw_palm(90, int(horizon_y) - 2, 65, 20, 0.7)

    # Right island
    draw.ellipse([380, int(horizon_y) - 6, W, int(horizon_y) + 12], fill=palm_color)
    draw_palm(420, int(horizon_y) - 3, 70, -20, 0.75)
    draw_palm(460, int(horizon_y) - 5, 90, 5, 0.85)
    draw_palm(490, int(horizon_y) - 2, 60, 15, 0.65)

    # Water reflection (mirror top half faintly)
    reflect = img.crop((0, 0, W, int(horizon_y)))
    reflect = reflect.transpose(Image.FLIP_TOP_BOTTOM)
    reflect = reflect.resize((W, H - int(horizon_y)))
    reflect_a = reflect.copy()
    reflect_a.putalpha(Image.new('L', reflect_a.size, 60))
    img.paste(reflect_a, (0, int(horizon_y)), reflect_a)
    draw = ImageDraw.Draw(img)

    # Water lines (horizontal shimmer)
    for y in range(int(horizon_y), H, 4):
        alpha = random.randint(10, 35)
        draw.line([(0, y), (W, y)], fill=(150, 180, 230, alpha))

    # "AC" text with glow
    # Try to find a bold font
    font_paths = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
    ]

    font = None
    for fp in font_paths:
        if os.path.exists(fp):
            font = ImageFont.truetype(fp, 160)
            break
    if font is None:
        font = ImageFont.load_default()

    # Text glow layer
    text_glow = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    tg_draw = ImageDraw.Draw(text_glow)

    text = "AC"
    bbox = tg_draw.textbbox((0, 0), text, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    tx = (W - tw) // 2
    ty = int(H * 0.30) - th // 2

    # Outer glow
    for offset in range(20, 0, -1):
        alpha = int(30 * (1.0 - offset / 20.0))
        tg_draw.text((tx, ty), text, font=font,
                     fill=(200, 180, 255, alpha),
                     stroke_width=offset, stroke_fill=(200, 180, 255, alpha // 2))

    text_glow = text_glow.filter(ImageFilter.GaussianBlur(8))
    img = Image.alpha_composite(img, text_glow)
    draw = ImageDraw.Draw(img)

    # Main text - "A" in blue-gray, "C" in purple
    # Draw character by character
    a_bbox = draw.textbbox((0, 0), "A", font=font)
    a_w = a_bbox[2] - a_bbox[0]
    c_bbox = draw.textbbox((0, 0), "C", font=font)
    c_w = c_bbox[2] - c_bbox[0]
    total_w = a_w + c_w - 10  # slight overlap
    ax = (W - total_w) // 2
    ay = ty

    # "A" - blue-gray
    draw.text((ax, ay), "A", font=font, fill=(140, 160, 200, 255))
    # "C" - lavender purple
    draw.text((ax + a_w - 10, ay), "C", font=font, fill=(180, 150, 220, 255))

    return img


def round_corners(img, radius):
    """Add rounded corners with transparency"""
    w, h = img.size
    mask = Image.new('L', (w, h), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle([0, 0, w, h], radius=radius, fill=255)
    result = img.copy()
    result.putalpha(mask)
    return result


def png_to_ico(png_path, ico_path, sizes=[256, 128, 64, 48, 32, 16]):
    """Convert PNG to ICO with multiple sizes"""
    img = Image.open(png_path)
    icons = []
    for size in sizes:
        resized = img.resize((size, size), Image.LANCZOS)
        icons.append(resized)
    icons[0].save(ico_path, format='ICO', sizes=[(s, s) for s in sizes],
                  append_images=icons[1:])


def png_to_c_header(png_path, header_path, var_name):
    """Convert PNG file to C header with embedded data"""
    with open(png_path, 'rb') as f:
        data = f.read()

    with open(header_path, 'w') as f:
        f.write(f"// Auto-generated from {os.path.basename(png_path)}\n")
        f.write(f"// Size: {len(data)} bytes\n")
        f.write(f"#pragma once\n\n")
        f.write(f"static const unsigned char {var_name}[] = {{\n")

        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(f"    {hex_str},\n")

        f.write(f"}};\n\n")
        f.write(f"static const unsigned int {var_name}Size = {len(data)};\n")


if __name__ == '__main__':
    print("Generating AC logo...")
    logo = gen_logo()

    # Save full-size
    out_dir = os.path.join(os.path.dirname(__file__), '..', 'assets')
    os.makedirs(out_dir, exist_ok=True)

    full_path = os.path.join(out_dir, 'ac_logo_full.png')
    logo.save(full_path, 'PNG')
    print(f"  Full logo: {full_path}")

    # Rounded version (radius = 80 for 512px image)
    rounded = round_corners(logo, radius=80)
    rounded_path = os.path.join(out_dir, 'ac_logo_rounded.png')
    rounded.save(rounded_path, 'PNG')
    print(f"  Rounded logo: {rounded_path}")

    # Generate ICO for loader.exe
    ico_path = os.path.join(out_dir, 'ac_logo.ico')
    png_to_ico(rounded_path, ico_path)
    print(f"  ICO file: {ico_path}")

    # Generate C header for embedding in loader
    header_path = os.path.join(out_dir, 'ac_logo_data.h')
    png_to_c_header(rounded_path, header_path, 'g_acLogoPng')
    print(f"  C header: {header_path}")

    # Also generate a small version for in-game overlay (64x64)
    small = rounded.resize((64, 64), Image.LANCZOS)
    small_path = os.path.join(out_dir, 'ac_logo_small.png')
    small.save(small_path, 'PNG')
    print(f"  Small logo: {small_path}")

    print("Done!")
