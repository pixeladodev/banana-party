#!/usr/bin/env python3
"""Banana Party's shelf art. The picture is the drawing below, unchanged.
   Writes icon.png (32 x 32) and tile.png (320 x 288) next to this file:
   nearest-neighbor, and alpha 0 outside the rounded square. Needs Pillow.

     python3 packaging/store/lobby-icon.py
"""
import math
import os

from PIL import Image

W, H = 108, 90
HERE = os.path.dirname(os.path.abspath(__file__))
TRANSPARENT = (0, 0, 0, 0)

CIRCLE_OUTLINE = (16, 12, 24, 255)
CIRCLE_RIM_TOP = (176, 124, 242, 255)
CIRCLE_RIM = (132, 78, 204, 255)
CIRCLE_INNER_RING = (104, 54, 172, 255)
CIRCLE_FILL_TOP = (86, 40, 148, 255)
CIRCLE_FILL_BOT = (68, 28, 122, 255)


def make_purple_circle(r_outer=42.0, cx=54.0, cy=45.0):
    img = Image.new("RGBA", (W, H), TRANSPARENT)
    px = img.load()
    for y in range(H):
        for x in range(W):
            dist = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
            if dist <= r_outer:
                if dist > r_outer - 1.1:
                    px[x, y] = CIRCLE_OUTLINE
                elif dist > r_outer - 2.2:
                    px[x, y] = CIRCLE_RIM_TOP if y < cy - 15 else CIRCLE_RIM
                elif dist > r_outer - 3.3:
                    px[x, y] = CIRCLE_INNER_RING
                else:
                    px[x, y] = CIRCLE_FILL_TOP if y < cy else CIRCLE_FILL_BOT
    return img, px

# ==============================================================================
# NINTENDO DS PIXEL-ART SPRITE (30 x 35 px) — Identical to approved version
# ==============================================================================
nds_sprite = [
    "..OOOOOOOOOOOOOOOOOOOOOOOOOO..",  # 0
    ".OHHHHHHHHHHHHHHHHHHHHHHHHHHO.",  # 1
    "OHPSHHOOOOOOOOOOOOOOOOOHHSPBBO",  # 2
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 3  <- Top screen 1/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 4  <- Top screen 2/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 5  <- Top screen 3/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 6  <- Top screen 4/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 7  <- Top screen 5/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 8  <- Top screen 6/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 9  <- Top screen 7/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 10 <- Top screen 8/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 11 <- Top screen 9/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 12 <- Top screen 10/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 13 <- Top screen 11/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 14 <- Top screen 12/12
    "OHBBBBOOOOOOOOOOOOOOOOOOBBBBBO",  # 15
    ".OOOOOOOOOOOOOOOOOOOOOOOOOOGO.",  # 16
    "OHHHHHHHHHHHHHHHHHHHHHHHHHHHHO",  # 17
    "OHBBBBOOOOOOOOOOOOOOOOOOBBBBBO",  # 18
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 19 <- Bottom screen 1/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 20 <- Bottom screen 2/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 21 <- Bottom screen 3/12
    "OHBWBBOLLLLLLLLLLLLLLLLOBBYBBO",  # 22 <- D-pad top + X button
    "OHWWWBOLLLLLLLLLLLLLLLLOBYBYBO",  # 23 <- D-pad mid + Y & A buttons
    "OHBWBBOLLLLLLLLLLLLLLLLOBBYBBO",  # 24 <- D-pad bot + B button
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 25 <- Bottom screen 7/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 26 <- Bottom screen 8/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 27 <- Bottom screen 9/12
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBPBO",  # 28 <- Start button
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBPBO",  # 29 <- Select button
    "OHBBBBOLLLLLLLLLLLLLLLLOBBBBBO",  # 30 <- Bottom screen 12/12
    "OHBBBBOOOOOOOOOOOOOOOOOOBBBBBO",  # 31
    "ODDDDDDDDDDDDDDDDDDDDDDDDDDDDO",  # 32
    ".OODDDDDDDDDDDDDDDDDDDDDDDDOO.",  # 33
    "..OOOOOOOOOOOOOOOOOOOOOOOOOO..",  # 34
]

p1_nds_pal = {
    "O": (16, 12, 24, 255),
    "H": (186, 230, 253, 255),
    "B": (56, 189, 248, 255),     # Electric Cyan P1
    "D": (2, 132, 199, 255),
    "P": (12, 74, 110, 255),
    "S": (12, 74, 110, 255),
    "L": (224, 247, 255, 255),
    "G": (74, 222, 128, 255),
    "W": (255, 255, 255, 255),
    "Y": (255, 230, 80, 255),
}

p2_nds_pal = {
    "O": (16, 12, 24, 255),
    "H": (254, 205, 211, 255),
    "B": (251, 113, 133, 255),    # Vibrant Coral P2
    "D": (225, 29, 72, 255),
    "P": (136, 19, 55, 255),
    "S": (136, 19, 55, 255),
    "L": (255, 241, 245, 255),
    "G": (74, 222, 128, 255),
    "W": (255, 255, 255, 255),
    "Y": (255, 230, 80, 255),
}


def draw_nds(px, x0, y0, pal):
    for r, row in enumerate(nds_sprite):
        for c, ch in enumerate(row):
            if ch in pal:
                px[x0 + c, y0 + r] = pal[ch]


def draw_bezier_banana(px, p0, p1, p2, r_mid=6.35, r_stem=2.05, r_tip=1.40, flip_shading=False):
    OUTLINE = (16, 12, 24, 255)
    STEM    = (116, 70, 30, 255)
    GREEN   = (108, 216, 64, 255)
    HILITE  = (255, 252, 180, 255)
    GOLD    = (255, 224, 34, 255)
    SHADE   = (244, 162, 18, 255)

    samples = []
    N = 650
    for i in range(N + 1):
        t = i / N
        mt = 1.0 - t
        bx = mt * mt * p0[0] + 2 * mt * t * p1[0] + t * t * p2[0]
        by = mt * mt * p0[1] + 2 * mt * t * p1[1] + t * t * p2[1]
        tx = 2 * mt * (p1[0] - p0[0]) + 2 * t * (p2[0] - p1[0])
        ty = 2 * mt * (p1[1] - p0[1]) + 2 * t * (p2[1] - p1[1])
        tlen = math.hypot(tx, ty) or 1.0
        tx /= tlen
        ty /= tlen
        base_r = r_stem * (1.0 - t) + r_tip * t
        bulge = math.sin(math.pi * (t ** 0.88))
        radius = base_r + (r_mid - base_r) * (bulge ** 0.72)
        samples.append((t, bx, by, tx, ty, radius))

    for y in range(H):
        for x in range(W):
            px_c, py_c = x + 0.5, y + 0.5
            best_norm = 999.0
            best_t = 0.0
            best_side = 0.0
            best_rad = 1.0
            for t, bx, by, tx, ty, radius in samples:
                dx = px_c - bx
                dy = py_c - by
                d = math.hypot(dx, dy)
                norm = d / radius
                if norm < best_norm:
                    best_norm = norm
                    best_t = t
                    best_side = tx * dy - ty * dx
                    best_rad = radius

            if best_norm <= 1.0:
                if (best_rad - best_norm * best_rad) <= 1.05:
                    px[x, y] = OUTLINE
                else:
                    side = -best_side if flip_shading else best_side
                    if best_t < 0.065:
                        px[x, y] = STEM
                    elif best_t < 0.13:
                        px[x, y] = GREEN if side < 0 else GOLD
                    elif best_t > 0.975:
                        px[x, y] = STEM
                    else:
                        if side > 1.35:
                            px[x, y] = HILITE
                        elif side < -1.35:
                            px[x, y] = SHADE
                        else:
                            px[x, y] = GOLD


def fit_nearest(src, w, h):
    """Uniform nearest scale, centered. No new colors."""
    sw, sh = src.size
    scale = min(w / sw, h / sh)
    nw = min(w, max(1, int(round(sw * scale))))
    nh = min(h, max(1, int(round(sh * scale))))
    scaled = src.resize((nw, nh), Image.Resampling.NEAREST)
    canvas = Image.new("RGBA", (w, h), TRANSPARENT)
    canvas.paste(scaled, ((w - nw) // 2, (h - nh) // 2), scaled)
    return canvas


def clear_outside_rounded(im):
    """Hard rounded square: corners outside it are alpha 0. No antialiasing."""
    w, h = im.size
    r = max(2, int(round(0.22 * min(w, h))))
    px = im.load()
    for y in range(h):
        for x in range(w):
            dx = r - 1 - x if x < r else x - (w - r) if x >= w - r else 0
            dy = r - 1 - y if y < r else y - (h - r) if y >= h - r else 0
            if dx * dx + dy * dy >= r * r:
                px[x, y] = TRANSPARENT
    return im


def store_images(src):
    icon = clear_outside_rounded(fit_nearest(src, 32, 32))
    tile = clear_outside_rounded(fit_nearest(src, 320, 288))
    icon_path = os.path.join(HERE, "icon.png")
    tile_path = os.path.join(HERE, "tile.png")
    icon.save(icon_path)
    tile.save(tile_path)
    return icon_path, tile_path


img, px = make_purple_circle(r_outer=42.0, cx=54.0, cy=45.0)

# Banana 1 (Back-diagonal '\')
draw_bezier_banana(
    px,
    p0=(38.0, 27.0),
    p1=(47.0, 48.0),
    p2=(70.0, 61.5),
    r_mid=6.35,
    r_stem=2.05,
    r_tip=1.40,
    flip_shading=True,
)

# Banana 2 (Forward-diagonal '/')
draw_bezier_banana(
    px,
    p0=(70.0, 27.0),
    p1=(61.0, 48.0),
    p2=(38.0, 61.5),
    r_mid=6.35,
    r_stem=2.05,
    r_tip=1.40,
    flip_shading=False,
)

# 2 Perfectly Symmetric Nintendo DS consoles (30x35 px)
draw_nds(px, 2, 27, p1_nds_pal)
draw_nds(px, 76, 27, p2_nds_pal)

# ==============================================================================
# COLORED CONFETTI DOTS ON THE PURPLE BACKGROUND CIRCLE ONLY
# Strictly guarded: only drawn on untouched purple fill pixels with a >= 2px
# safety buffer around every banana and Nintendo DS pixel!
# ==============================================================================
fg_mask = set()
for y in range(H):
    for x in range(W):
        c = px[x, y]
        if c not in (TRANSPARENT, CIRCLE_FILL_TOP, CIRCLE_FILL_BOT):
            fg_mask.add((x, y))


def is_safe_bg(x, y):
    if not (0 <= x < W and 0 <= y < H):
        return False
    if px[x, y] not in (CIRCLE_FILL_TOP, CIRCLE_FILL_BOT):
        return False
    # Keep at least 4.5 px inside circle outer edge so dots stay strictly inside purple fill
    if math.hypot(x + 0.5 - 54.0, y + 0.5 - 45.0) > 37.5:
        return False
    # Keep a 2px safety margin around all foreground sprites (bananas & consoles)
    for dy in range(-2, 3):
        for dx in range(-2, 3):
            if (x + dx, y + dy) in fg_mask:
                return False
    return True


DOT_CYAN  = (56, 189, 248, 255)
DOT_CORAL = (251, 113, 133, 255)
DOT_MINT  = (74, 222, 128, 255)
DOT_GOLD  = (255, 224, 34, 255)
DOT_LAV   = (216, 180, 254, 255)

# Harmoniously balanced festive pixel dots across the purple circular background
confetti_dots = [
    # Top purple arch (above the X of bananas)
    (46, 10, DOT_GOLD),
    (61, 11, DOT_CYAN),
    (38, 13, DOT_CORAL),
    (54, 14, DOT_MINT),
    (69, 14, DOT_LAV),
    (31, 17, DOT_CYAN),
    (48, 18, DOT_LAV),
    (59, 19, DOT_GOLD),
    (76, 18, DOT_CORAL),
    (42, 22, DOT_MINT),
    (54, 23, DOT_CORAL),
    (65, 22, DOT_CYAN),

    # Upper-left & upper-right flanks (above the two DS consoles)
    (23, 21, DOT_GOLD),
    (84, 21, DOT_MINT),

    # Mid-left & mid-right inner gaps (between DS consoles and bananas)
    (34, 38, DOT_LAV),
    (73, 38, DOT_GOLD),
    (34, 51, DOT_CYAN),
    (73, 51, DOT_CORAL),

    # Lower-left & lower-right flanks (below the two DS consoles)
    (22, 66, DOT_MINT),
    (29, 71, DOT_CORAL),
    (85, 66, DOT_CYAN),
    (78, 71, DOT_GOLD),

    # Bottom purple arch (below the X of bananas)
    (54, 65, DOT_GOLD),
    (45, 69, DOT_CYAN),
    (63, 69, DOT_MINT),
    (37, 74, DOT_LAV),
    (53, 74, DOT_CORAL),
    (70, 74, DOT_LAV),
    (46, 78, DOT_MINT),
    (61, 78, DOT_GOLD),
]

placed = 0
for dx, dy, col in confetti_dots:
    if is_safe_bg(dx, dy):
        px[dx, dy] = col
        placed += 1

print(f"Placed {placed}/{len(confetti_dots)} safe background confetti dots!")
icon_path, tile_path = store_images(img)
print(icon_path)
print(tile_path)
