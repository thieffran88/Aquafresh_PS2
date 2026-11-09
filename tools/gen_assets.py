#!/usr/bin/env python3
"""
Converte a arte de assets/src/ em texturas prontas para o GS do PS2.

Saida:
  assets/bin/<atlas>.bin   pixels T8 (1 byte/pixel) + CLUT de 256 cores CT32
                           (ja no formato do GS: alfa 0..128 e ordem CSM1)
  src/assets.c / assets.h  tabela de atlas e de sprites

Requer Pillow, numpy e scipy. Uso: python3 tools/gen_assets.py
"""
import os
import sys

import numpy as np
from PIL import Image, ImageFilter
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets", "src")
BIN = os.path.join(ROOT, "assets", "bin")

SHEET = Image.open(os.path.join(SRC, "sprites.png")).convert("RGB")
FONT = Image.open(os.path.join(SRC, "font.png")).convert("RGB")
TITLE = Image.open(os.path.join(SRC, "title.png")).convert("RGB")

# ---------------------------------------------------------------- recortes

def cut(img, box, margin=3, white=228, feather=True):
    """Recorta 'box' e torna transparente o branco ligado a borda."""
    x0, y0, x1, y1 = box
    x0 -= margin; y0 -= margin; x1 += margin; y1 += margin
    a = np.asarray(img.crop((x0, y0, x1, y1))).astype(np.float32)
    mn = a.min(axis=2)
    whiteish = mn > white
    lab, n = ndimage.label(whiteish)
    border = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]])))
    border.discard(0)
    bg = np.isin(lab, list(border))
    alpha = np.where(bg, 0.0, 255.0)
    if feather:
        # suaviza a borda: pixels claros vizinhos do fundo ficam semi-transparentes
        near = ndimage.binary_dilation(bg, iterations=2) & ~bg
        soft = np.clip((255.0 - mn) / (255.0 - 150.0), 0, 1) * 255.0
        alpha = np.where(near, np.minimum(alpha, soft), alpha)
        # cor das bordas semi-transparentes = contorno escuro (evita halo branco)
        edge = near & (alpha < 250)
        a[edge] = np.minimum(a[edge], 40)
    # descarta pedacos soltos de objetos vizinhos que entraram no recorte
    lab2, n2 = ndimage.label(alpha > 0)
    if n2 > 1:
        sizes = ndimage.sum(np.ones_like(alpha), lab2, range(1, n2 + 1))
        keep = [i + 1 for i, sz in enumerate(sizes) if sz >= 0.05 * sizes.max()]
        alpha = np.where(np.isin(lab2, keep), alpha, 0)
    out = np.dstack([a, alpha]).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def fit(im, w, h):
    return im.resize((w, h), Image.LANCZOS)

# Tabuleiro de pecas da folha: (cor, animal) por linha/coluna.
# cores: 0 azul 1 verde 2 rosa 3 roxo 4 vermelho 5 amarelo
# animais: 0 caranguejo 1 peixe 2 agua-viva 3 cavalo-marinho 4 estrela 5 tartaruga
B, G, P, U, R, Y = range(6)
CRAB, FISH, JELLY, SEA, STAR, TURT = range(6)
TILE_GRID = [
    [(R, SEA), (U, STAR), (R, STAR), (R, FISH), (Y, SEA), (G, FISH)],
    [(Y, FISH), (P, STAR), (P, TURT), (B, FISH), (U, SEA), (G, JELLY)],
    [(U, JELLY), (R, JELLY), (Y, STAR), (P, SEA), (G, SEA), (Y, CRAB)],
    [(P, JELLY), (Y, JELLY), (Y, TURT), (P, FISH), (P, CRAB), (G, CRAB)],
    [(U, CRAB), (G, TURT), (B, SEA), (B, JELLY), (G, STAR), (R, CRAB)],
    [(R, TURT), (U, TURT), (U, FISH), (B, STAR), (B, TURT), (B, CRAB)],
]
TILE_COLS_X = [(31, 184), (205, 356), (375, 526), (546, 697), (714, 865), (882, 1035)]
TILE_ROWS_Y = [(12, 150), (158, 290), (297, 420), (426, 542), (547, 658), (666, 781)]
TILE_W, TILE_H = 64, 56

def tiles_atlas():
    seen = set()
    atlas = Image.new("RGBA", (512, 384), (0, 0, 0, 0))
    sprites = []
    for r, row in enumerate(TILE_GRID):
        for c, (col, ani) in enumerate(row):
            assert (col, ani) not in seen
            seen.add((col, ani))
            x0, x1 = TILE_COLS_X[c]
            y0, y1 = TILE_ROWS_Y[r]
            im = fit(cut(SHEET, (x0, y0, x1, y1), margin=2), TILE_W, TILE_H)
            idx = col * 6 + ani
            ax, ay = (idx % 7) * (TILE_W + 4) + 2, (idx // 7) * (TILE_H + 4) + 2
            atlas.paste(im, (ax, ay))
            sprites.append(("TILE_%02d" % idx, ax, ay, TILE_W, TILE_H, idx))
    assert len(seen) == 36
    sprites.sort(key=lambda s: s[5])
    return atlas, [s[:5] for s in sprites]

# Itens decorativos: nome -> (caixa na folha, altura final)
DECO = [
    ("TURTLE",  (1052, 8, 1301, 201), 84),
    ("PUFFER",  (1299, 28, 1522, 207), 76),
    ("STARFISH", (1063, 200, 1268, 379), 76),
    ("CRAB",    (1277, 221, 1520, 378), 64),
    ("CLAM",    (1076, 378, 1283, 524), 56),
    ("SHELL",   (1321, 403, 1500, 510), 40),
    ("BUBBLE",  (1436, 520, 1499, 579), 32),
    ("WEED",    (1054, 646, 1168, 779), 72),
    ("CORAL_PINK", (1174, 645, 1287, 773), 72),
    ("CORAL_ORANGE", (1295, 649, 1404, 772), 72),
    ("CORAL_PURPLE", (1414, 642, 1520, 771), 72),
    ("CORAL_GREEN", (1039, 795, 1188, 883), 48),
    ("CORAL_BALLS", (1209, 788, 1351, 884), 50),
    ("CORAL_TUBES", (1368, 784, 1504, 884), 54),
    ("ROCKS",   (1010, 898, 1223, 1010), 56),
    ("SAND",    (1236, 936, 1518, 1011), 40),
    ("SLOT_DARK", (660, 836, 820, 989), 56),
    ("SLOT_LIGHT", (840, 836, 1002, 989), 56),
]

def pack(items, width, height):
    """Empacotamento simples em prateleiras."""
    items = sorted(items, key=lambda it: -it[1].size[1])
    atlas = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    x = y = 2
    shelf = 0
    out = []
    for name, im in items:
        w, h = im.size
        if x + w + 2 > width:
            x = 2
            y += shelf + 3
            shelf = 0
        if y + h + 2 > height:
            sys.exit("atlas cheio: %s" % name)
        atlas.paste(im, (x, y))
        out.append((name, x, y, w, h))
        x += w + 3
        shelf = max(shelf, h)
    return atlas, out

def deco_atlas():
    items = []
    for name, box, h in DECO:
        im = cut(SHEET, box)
        if name.startswith("SLOT"):
            im = fit(im, TILE_W, TILE_H)
        else:
            w = max(1, round(im.size[0] * h / im.size[1]))
            im = fit(im, w, h)
        items.append((name, im))
    return pack(items, 512, 256)

# Fonte: componentes ligados da folha, em ordem de leitura.
FONT_ROW4_REAL = "!?.,:;'\"()[]{}+-\x01\x02=%"   # \x01 vezes, \x02 divisao
FONT_H = 40

def font_atlas():
    a = np.asarray(FONT).astype(int)
    mask = a.min(axis=2) < 225
    lab, n = ndimage.label(mask)
    objs = ndimage.find_objects(lab)
    comps = []
    for i, s in enumerate(objs):
        x0, x1, y0, y1 = s[1].start, s[1].stop, s[0].start, s[0].stop
        if (x1 - x0) * (y1 - y0) > 150:
            comps.append((x0, y0, x1, y1, i + 1))
    comps.sort(key=lambda b: ((b[1] + b[3]) // 2 // 180, b[0]))
    rows = [[], [], [], [], []]
    for c in comps:
        rows[min(4, ((c[1] + c[3]) // 2) // 180)].append(c)
    chars = ["ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ", "0123456789", FONT_ROW4_REAL]
    glyphs = []
    for r, comps_r in enumerate(rows):
        assert len(comps_r) == len(chars[r]), (r, len(comps_r))
        bottom = max(c[3] for c in comps_r)
        top = bottom - 178
        for ch, (x0, y0, x1, y1, li) in zip(chars[r], comps_r):
            if ch in "{}\x01\x02":
                continue
            sub = np.zeros_like(mask)
            sl = (slice(top, bottom + 2), slice(x0 - 2, x1 + 2))
            own = ndimage.binary_dilation(lab == li, iterations=3)
            crop = a[sl].astype(np.float32)
            mn = crop.min(axis=2)
            alpha = np.where(own[sl], 255.0, 0.0)
            edge = own[sl] & (mn > 150)
            alpha = np.where(edge, np.clip((255.0 - mn) / 105.0, 0, 1) * 255.0, alpha)
            crop[edge] = np.minimum(crop[edge], 30)
            im = Image.fromarray(np.dstack([crop, alpha]).astype(np.uint8), "RGBA")
            w = max(1, round(im.size[0] * FONT_H / im.size[1]))
            glyphs.append((ch, fit(im, w, FONT_H)))
    items = [("GLYPH_%02X" % ord(ch), im) for ch, im in glyphs]
    # variante clara (miolo branco, contorno escuro) para textos que recebem cor
    for ch, im in glyphs:
        a = np.asarray(im).astype(np.float32)
        rgb = a[..., :3]
        lum = rgb.mean(axis=2)
        dark = rgb.max(axis=2) < 70
        light = np.clip(215 + (lum - 120) * 0.35, 200, 255)
        out = np.where(dark[..., None], rgb, np.dstack([light, light, light]))
        items.append(("GLYPHW_%02X" % ord(ch),
                      Image.fromarray(np.dstack([out, a[..., 3]]).astype(np.uint8), "RGBA")))
    atlas, sprites = pack(items, 512, 512)
    return atlas, sprites

def backgrounds():
    # fundo de jogo: ilustracao submarina da folha (sem a moldura)
    play = SHEET.crop((52, 802, 306, 1004)).resize((640, 448), Image.BICUBIC)
    play = play.filter(ImageFilter.GaussianBlur(1.2))
    title = TITLE.resize((640, 448), Image.LANCZOS)
    return play.convert("RGBA"), title.convert("RGBA")

# ------------------------------------------------------------- exportacao

def to_t8(im):
    """RGBA -> (indices, paleta RGBA de 256 entradas)."""
    q = im.quantize(colors=256, method=Image.Quantize.FASTOCTREE,
                    dither=Image.Dither.FLOYDSTEINBERG)
    pal = q.getpalette(rawmode="RGBA")
    pal = (pal + [0] * 1024)[:1024]
    return np.asarray(q, dtype=np.uint8), np.array(pal, dtype=np.uint8).reshape(256, 4)

def gs_clut(pal):
    """Converte paleta para o GS: alfa 0..128 e reordenacao CSM1."""
    p = pal.copy().astype(np.int32)
    p[:, 3] = (p[:, 3] * 128 + 127) // 255
    out = p.copy()
    for i in range(256):
        j = (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1)
        out[j] = p[i]
    return out.astype(np.uint8)

def export(name, im, atlases):
    w, h = im.size
    assert w % 128 == 0 and h % 32 == 0, (name, w, h)
    idx, pal = to_t8(im)
    os.makedirs(BIN, exist_ok=True)
    with open(os.path.join(BIN, name + ".bin"), "wb") as f:
        f.write(idx.tobytes())
        f.write(gs_clut(pal).tobytes())
    atlases.append((name, w, h))
    im.save(os.path.join(BIN, name + "_preview.png"))

def main():
    atlases = []
    sprites = []
    play, title = backgrounds()
    export("bg_play", play, atlases)
    export("bg_title", title, atlases)
    tiles, s = tiles_atlas()
    export("tiles", tiles, atlases)
    sprites += [(n, "ATLAS_TILES") + tuple(t) for n, *t in s]
    deco, s = deco_atlas()
    export("deco", deco, atlases)
    sprites += [(n, "ATLAS_DECO") + tuple(t) for n, *t in s]
    font, s = font_atlas()
    export("font", font, atlases)
    sprites += [(n, "ATLAS_FONT") + tuple(t) for n, *t in s]

    hdr = ["/* Gerado por tools/gen_assets.py - nao edite. */",
           "#ifndef AQUAFRESH_ASSETS_H", "#define AQUAFRESH_ASSETS_H", "",
           "enum {"]
    hdr += ["    ATLAS_%s," % n.upper() for n, _, _ in atlases]
    hdr += ["    NUM_ATLASES", "};", "", "enum {"]
    hdr += ["    SPR_%s," % n for n, *_ in sprites]
    hdr += ["    NUM_SPRITES", "};", "",
            "typedef struct { unsigned short w, h; const unsigned char *data; } AtlasDef;",
            "typedef struct { unsigned char atlas; short x, y, w, h; } SpriteDef;",
            "extern const AtlasDef atlases[NUM_ATLASES];",
            "extern const SpriteDef sprite_defs[NUM_SPRITES];",
            "/* sprite de um glifo da fonte, ou -1 */",
            "extern const short glyph_sprite[128];",
            "/* mesma fonte com miolo branco (aceita cor) */",
            "extern const short glyph_sprite_plain[128];",
            "", "#endif", ""]
    open(os.path.join(ROOT, "src", "assets.h"), "w").write("\n".join(hdr))

    c = ["/* Gerado por tools/gen_assets.py - nao edite. */", '#include "assets.h"', ""]
    for n, _, _ in atlases:
        c.append("extern const unsigned char asset_%s[];" % n)
    c += ["", "const AtlasDef atlases[NUM_ATLASES] = {"]
    c += ["    { %d, %d, asset_%s }," % (w, h, n) for n, w, h in atlases]
    c += ["};", "", "const SpriteDef sprite_defs[NUM_SPRITES] = {"]
    c += ["    { %s, %d, %d, %d, %d }, /* %s */" % (a, x, y, w, h, n) for n, a, x, y, w, h in sprites]
    c += ["};", "", "const short glyph_sprite[128] = {"]
    gl = {int(n[6:], 16): "SPR_" + n for n, *_ in sprites if n.startswith("GLYPH_")}
    c += ["    %s," % gl.get(i, "-1") for i in range(128)]
    c += ["};", "", "const short glyph_sprite_plain[128] = {"]
    gl = {int(n[7:], 16): "SPR_" + n for n, *_ in sprites if n.startswith("GLYPHW_")}
    c += ["    %s," % gl.get(i, "-1") for i in range(128)]
    c += ["};", ""]
    open(os.path.join(ROOT, "src", "assets.c"), "w").write("\n".join(c))

    s = ["/* Gerado por tools/gen_assets.py - embute as texturas no executavel. */"]
    for n, _, _ in atlases:
        s += ["    .section .rodata", "    .balign 128", "    .global asset_%s" % n,
              "asset_%s:" % n, '    .incbin "assets/bin/%s.bin"' % n]
    s += ["#if defined(__linux__) && defined(__ELF__) && !defined(_EE)",
          '    .section .note.GNU-stack,"",@progbits', "#endif", ""]
    open(os.path.join(ROOT, "src", "assets_bin.S"), "w").write("\n".join(s))
    print("ok: %d atlas, %d sprites" % (len(atlases), len(sprites)))

if __name__ == "__main__":
    main()
