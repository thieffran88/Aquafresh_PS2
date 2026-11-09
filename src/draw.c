/*
 * Aquafresh PS2 - desenho de formas, texto e criaturas usando apenas
 * triangulos e retangulos (nenhuma textura externa necessaria).
 */
#include <math.h>
#include <string.h>
#include "draw.h"
#include "font.h"
#include "game.h"

#define STEPS 24
static float cos_t[STEPS + 1], sin_t[STEPS + 1];

const Color creature_colors[6] = {
    RGB(70, 160, 255),   /* azul */
    RGB(95, 215, 95),    /* verde */
    RGB(255, 140, 205),  /* rosa */
    RGB(170, 120, 250),  /* roxo */
    RGB(245, 70, 70),    /* vermelho */
    RGB(255, 215, 60),   /* amarelo */
};
const char *color_names[6] = { "AZUL", "VERDE", "ROSA", "ROXO", "VERMELHO", "AMARELO" };
const char *animal_names[6] = { "CARANGUEJO", "PEIXE", "AGUA-VIVA", "CAVALO-MARINHO", "ESTRELA", "TARTARUGA" };

void draw_init(void)
{
    int i;
    for (i = 0; i <= STEPS; i++) {
        float a = (float)i * 6.2831853f / STEPS;
        cos_t[i] = cosf(a);
        sin_t[i] = sinf(a);
    }
}

static int clamp255(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

Color color_shade(Color c, int p)
{
    return RGBA(clamp255((int)COL_R(c) * p / 100), clamp255((int)COL_G(c) * p / 100),
                clamp255((int)COL_B(c) * p / 100), COL_A(c));
}

Color color_alpha(Color c, int a)
{
    return (c & 0xFFFFFF00u) | (unsigned)clamp255(a);
}

void draw_ellipse_sector(float cx, float cy, float rx, float ry, int s0, int s1, Color c)
{
    int i, stride = 1;
    float rmax = rx > ry ? rx : ry;
    /* circulos pequenos usam menos triangulos (economiza tempo do EE) */
    if (rmax < 5.0f && (s1 - s0) % 3 == 0)
        stride = 3;
    else if (rmax < 12.0f && (s1 - s0) % 2 == 0)
        stride = 2;
    for (i = s0; i < s1; i += stride) {
        int a = ((i % STEPS) + STEPS) % STEPS, b = (a + stride) % STEPS;
        gfx_tri(cx, cy, cx + cos_t[a] * rx, cy + sin_t[a] * ry,
                cx + cos_t[b] * rx, cy + sin_t[b] * ry, c);
    }
}

void draw_ellipse(float cx, float cy, float rx, float ry, Color c)
{
    draw_ellipse_sector(cx, cy, rx, ry, 0, STEPS, c);
}

void draw_circle(float cx, float cy, float r, Color c)
{
    draw_ellipse(cx, cy, r, r, c);
}

void draw_line(float x1, float y1, float x2, float y2, float t, Color c)
{
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    float nx, ny;
    if (len < 0.001f)
        return;
    nx = -dy / len * t * 0.5f;
    ny = dx / len * t * 0.5f;
    gfx_tri(x1 + nx, y1 + ny, x2 + nx, y2 + ny, x2 - nx, y2 - ny, c);
    gfx_tri(x1 + nx, y1 + ny, x2 - nx, y2 - ny, x1 - nx, y1 - ny, c);
}

void draw_round_rect(float x, float y, float w, float h, float r, Color c)
{
    /* cantos como quartos de circulo (sem sobreposicao para alfa correto) */
    gfx_rect(x + r, y, w - 2 * r, h, c);
    gfx_rect(x, y + r, r, h - 2 * r, c);
    gfx_rect(x + w - r, y + r, r, h - 2 * r, c);
    draw_ellipse_sector(x + w - r, y + h - r, r, r, 0, 6, c);
    draw_ellipse_sector(x + r, y + h - r, r, r, 6, 12, c);
    draw_ellipse_sector(x + r, y + r, r, r, 12, 18, c);
    draw_ellipse_sector(x + w - r, y + r, r, r, 18, 24, c);
}

void draw_frame(float x, float y, float w, float h, float t, Color c)
{
    gfx_rect(x, y, w, t, c);
    gfx_rect(x, y + h - t, w, t, c);
    gfx_rect(x, y + t, t, h - 2 * t, c);
    gfx_rect(x + w - t, y + t, t, h - 2 * t, c);
}

/* ------------------------------------------------------------------ texto */

int text_width(const char *s, int scale)
{
    int n = (int)strlen(s);
    return n ? (n * 6 - 1) * scale : 0;
}

void draw_text(float x, float y, int scale, Color c, const char *s)
{
    for (; *s; s++, x += 6 * scale) {
        int ch = (unsigned char)*s;
        int row;
        const unsigned char *g;
        Color cc = c;
        if (ch < 32 || ch > 127)
            continue;
        g = font_glyphs[ch - 32];
        /* simbolos dos botoes com as cores classicas */
        if (COL_R(c) > 60 || COL_G(c) > 60 || COL_B(c) > 60) {
            if (ch == 'x') cc = RGBA(120, 170, 255, COL_A(c));
            if (ch == 'o') cc = RGBA(255, 110, 110, COL_A(c));
            if (ch == '^') cc = RGBA(90, 230, 170, COL_A(c));
            if (ch == '[') cc = RGBA(255, 140, 220, COL_A(c));
        }
        for (row = 0; row < 7; row++) {
            int bits = g[row], col = 0;
            while (col < 5) {
                int start;
                if (!(bits & (0x10 >> col))) { col++; continue; }
                start = col;
                while (col < 5 && (bits & (0x10 >> col)))
                    col++;
                gfx_rect(x + start * scale, y + row * scale,
                         (float)((col - start) * scale), (float)scale, cc);
            }
        }
    }
}

void draw_text_shadow(float x, float y, int scale, Color c, const char *s)
{
    float d = scale > 1 ? (float)(scale / 2 + 1) : 1.0f;
    draw_text(x + d, y + d, scale, RGBA(0, 20, 50, 160), s);
    draw_text(x, y, scale, c, s);
}

void draw_text_center(float cx, float y, int scale, Color c, const char *s)
{
    draw_text_shadow(cx - text_width(s, scale) / 2.0f, y, scale, c, s);
}

/* --------------------------------------------------------------- criaturas */

#define U(v) (x + (v) * s)
#define V(v) (y + (v) * s)

static void draw_crab(float x, float y, float s, Color c, Color bg)
{
    Color d = color_shade(c, 70);
    int i;
    /* pernas */
    for (i = 0; i < 3; i++) {
        float yy = 0.56f + i * 0.09f;
        draw_line(U(0.33f), V(yy), U(0.12f), V(yy + 0.10f), s * 0.05f, d);
        draw_line(U(0.67f), V(yy), U(0.88f), V(yy + 0.10f), s * 0.05f, d);
    }
    /* bracos e pincas */
    draw_line(U(0.36f), V(0.52f), U(0.22f), V(0.36f), s * 0.07f, c);
    draw_line(U(0.64f), V(0.52f), U(0.78f), V(0.36f), s * 0.07f, c);
    draw_circle(U(0.20f), V(0.28f), s * 0.11f, c);
    draw_circle(U(0.80f), V(0.28f), s * 0.11f, c);
    gfx_tri(U(0.20f), V(0.30f), U(0.12f), V(0.12f), U(0.28f), V(0.14f), bg);
    gfx_tri(U(0.80f), V(0.30f), U(0.72f), V(0.14f), U(0.88f), V(0.12f), bg);
    /* corpo */
    draw_ellipse(U(0.5f), V(0.58f), s * 0.25f, s * 0.17f, c);
    draw_ellipse(U(0.5f), V(0.62f), s * 0.16f, s * 0.08f, color_shade(c, 85));
    /* olhos */
    draw_line(U(0.43f), V(0.45f), U(0.42f), V(0.36f), s * 0.035f, d);
    draw_line(U(0.57f), V(0.45f), U(0.58f), V(0.36f), s * 0.035f, d);
    draw_circle(U(0.42f), V(0.35f), s * 0.045f, RGB(250, 250, 250));
    draw_circle(U(0.58f), V(0.35f), s * 0.045f, RGB(250, 250, 250));
    draw_circle(U(0.42f), V(0.35f), s * 0.022f, RGB(10, 20, 40));
    draw_circle(U(0.58f), V(0.35f), s * 0.022f, RGB(10, 20, 40));
}

static void draw_fish(float x, float y, float s, Color c, Color bg)
{
    Color d = color_shade(c, 70);
    (void)bg;
    gfx_tri(U(0.66f), V(0.5f), U(0.92f), V(0.28f), U(0.86f), V(0.5f), d);
    gfx_tri(U(0.66f), V(0.5f), U(0.86f), V(0.5f), U(0.92f), V(0.72f), d);
    gfx_tri(U(0.36f), V(0.36f), U(0.56f), V(0.18f), U(0.60f), V(0.38f), d);
    gfx_tri(U(0.42f), V(0.62f), U(0.56f), V(0.78f), U(0.58f), V(0.62f), d);
    draw_ellipse(U(0.44f), V(0.5f), s * 0.30f, s * 0.19f, c);
    /* escamas / listras */
    draw_ellipse_sector(U(0.50f), V(0.5f), s * 0.07f, s * 0.15f, -6, 6, color_shade(c, 82));
    draw_ellipse_sector(U(0.36f), V(0.5f), s * 0.05f, s * 0.17f, -6, 6, color_shade(c, 82));
    draw_circle(U(0.26f), V(0.45f), s * 0.055f, RGB(250, 250, 250));
    draw_circle(U(0.25f), V(0.45f), s * 0.028f, RGB(10, 20, 40));
    draw_line(U(0.15f), V(0.55f), U(0.21f), V(0.56f), s * 0.03f, d);
}

static void draw_jelly(float x, float y, float s, Color c, Color bg)
{
    Color d = color_shade(c, 75);
    int i;
    (void)bg;
    /* tentaculos ondulados */
    for (i = 0; i < 4; i++) {
        float bx = 0.32f + i * 0.12f;
        float w = (i & 1) ? 0.04f : -0.04f;
        draw_line(U(bx), V(0.48f), U(bx + w), V(0.62f), s * 0.045f, d);
        draw_line(U(bx + w), V(0.62f), U(bx - w), V(0.76f), s * 0.045f, d);
        draw_line(U(bx - w), V(0.76f), U(bx + w * 0.5f), V(0.90f), s * 0.045f, d);
    }
    /* chapeu */
    draw_ellipse_sector(U(0.5f), V(0.48f), s * 0.30f, s * 0.32f, 12, 24, c);
    for (i = 0; i < 5; i++)
        draw_circle(U(0.26f + i * 0.12f), V(0.49f), s * 0.06f, c);
    draw_ellipse(U(0.42f), V(0.30f), s * 0.07f, s * 0.05f, color_shade(c, 125));
    draw_circle(U(0.42f), V(0.40f), s * 0.035f, RGB(10, 20, 40));
    draw_circle(U(0.58f), V(0.40f), s * 0.035f, RGB(10, 20, 40));
}

static void draw_seahorse(float x, float y, float s, Color c, Color bg)
{
    Color d = color_shade(c, 72);
    (void)bg;
    /* barbatana dorsal */
    gfx_tri(U(0.36f), V(0.40f), U(0.20f), V(0.50f), U(0.36f), V(0.62f), d);
    /* cauda enrolada */
    draw_line(U(0.52f), V(0.66f), U(0.50f), V(0.80f), s * 0.09f, c);
    draw_line(U(0.50f), V(0.80f), U(0.42f), V(0.89f), s * 0.07f, c);
    draw_line(U(0.42f), V(0.89f), U(0.33f), V(0.85f), s * 0.06f, c);
    draw_line(U(0.33f), V(0.85f), U(0.35f), V(0.77f), s * 0.05f, c);
    /* corpo */
    draw_ellipse(U(0.46f), V(0.42f), s * 0.12f, s * 0.13f, c);
    draw_ellipse(U(0.50f), V(0.56f), s * 0.12f, s * 0.13f, c);
    draw_ellipse(U(0.55f), V(0.53f), s * 0.06f, s * 0.13f, color_shade(c, 120));
    /* cabeca e focinho */
    draw_circle(U(0.48f), V(0.22f), s * 0.11f, c);
    draw_line(U(0.52f), V(0.25f), U(0.76f), V(0.30f), s * 0.07f, c);
    gfx_tri(U(0.40f), V(0.14f), U(0.44f), V(0.05f), U(0.50f), V(0.13f), d);
    draw_circle(U(0.50f), V(0.20f), s * 0.035f, RGB(250, 250, 250));
    draw_circle(U(0.51f), V(0.20f), s * 0.018f, RGB(10, 20, 40));
    draw_line(U(0.38f), V(0.48f), U(0.30f), V(0.46f), s * 0.025f, d);
}

static void draw_star(float x, float y, float s, Color c, Color bg)
{
    float cx = U(0.5f), cy = V(0.53f);
    float ro = s * 0.42f, ri = s * 0.17f;
    int i;
    Color d = color_shade(c, 80);
    (void)bg;
    for (i = 0; i < 5; i++) {
        /* ponta i aponta para cima: angulo -90 + 72*i graus */
        float a = -1.5707963f + i * 1.2566371f;
        float a1 = a - 0.6283185f, a2 = a + 0.6283185f;
        float ox = cx + cosf(a) * ro, oy = cy + sinf(a) * ro;
        float ix1 = cx + cosf(a1) * ri, iy1 = cy + sinf(a1) * ri;
        float ix2 = cx + cosf(a2) * ri, iy2 = cy + sinf(a2) * ri;
        gfx_tri(cx, cy, ix1, iy1, ox, oy, c);
        gfx_tri(cx, cy, ox, oy, ix2, iy2, d);
        draw_circle(cx + cosf(a) * ro * 0.55f, cy + sinf(a) * ro * 0.55f, s * 0.025f,
                    color_shade(c, 130));
    }
    draw_circle(cx, cy, s * 0.06f, color_shade(c, 120));
}

static void draw_turtle(float x, float y, float s, Color c, Color bg)
{
    Color d = color_shade(c, 70);
    Color l = color_shade(c, 125);
    (void)bg;
    /* nadadeiras */
    draw_ellipse(U(0.24f), V(0.34f), s * 0.12f, s * 0.07f, d);
    draw_ellipse(U(0.76f), V(0.34f), s * 0.12f, s * 0.07f, d);
    draw_ellipse(U(0.28f), V(0.74f), s * 0.09f, s * 0.06f, d);
    draw_ellipse(U(0.72f), V(0.74f), s * 0.09f, s * 0.06f, d);
    gfx_tri(U(0.46f), V(0.80f), U(0.54f), V(0.80f), U(0.5f), V(0.90f), d);
    /* cabeca */
    draw_circle(U(0.5f), V(0.16f), s * 0.09f, d);
    draw_circle(U(0.46f), V(0.14f), s * 0.02f, RGB(10, 20, 40));
    draw_circle(U(0.54f), V(0.14f), s * 0.02f, RGB(10, 20, 40));
    /* casco com desenho hexagonal */
    draw_ellipse(U(0.5f), V(0.52f), s * 0.25f, s * 0.29f, c);
    draw_ellipse(U(0.5f), V(0.52f), s * 0.10f, s * 0.11f, l);
    {
        int i;
        for (i = 0; i < 6; i++) {
            float a = i * 1.0471976f + 0.5235988f;
            draw_line(U(0.5f) + cosf(a) * s * 0.10f, V(0.52f) + sinf(a) * s * 0.11f,
                      U(0.5f) + cosf(a) * s * 0.22f, V(0.52f) + sinf(a) * s * 0.26f,
                      s * 0.03f, l);
        }
    }
}

void draw_animal(int animal, float x, float y, float s, Color c, Color bg)
{
    switch (animal) {
    case ANI_CRAB:     draw_crab(x, y, s, c, bg); break;
    case ANI_FISH:     draw_fish(x, y, s, c, bg); break;
    case ANI_JELLY:    draw_jelly(x, y, s, c, bg); break;
    case ANI_SEAHORSE: draw_seahorse(x, y, s, c, bg); break;
    case ANI_STAR:     draw_star(x, y, s, c, bg); break;
    default:           draw_turtle(x, y, s, c, bg); break;
    }
}

#define TILE_BG     RGB(18, 38, 92)
#define TILE_EDGE   RGB(40, 72, 150)
#define TILE_SHADOW RGBA(0, 10, 30, 120)

void draw_tile(int tile, float x, float y, float s)
{
    float r = s * 0.14f;
    draw_round_rect(x + 3, y + 4, s, s, r, TILE_SHADOW);
    draw_round_rect(x, y, s, s, r, TILE_EDGE);
    draw_round_rect(x + 2, y + 2, s - 4, s - 4, r * 0.8f, TILE_BG);
    if (tile >= 0)
        draw_animal(TILE_ANIMAL(tile), x + s * 0.08f, y + s * 0.08f, s * 0.84f,
                    creature_colors[TILE_COLOR(tile)], TILE_BG);
}

void draw_tile_back(float x, float y, float s)
{
    float r = s * 0.14f;
    draw_round_rect(x + 2, y + 3, s, s, r, TILE_SHADOW);
    draw_round_rect(x, y, s, s, r, TILE_EDGE);
    draw_round_rect(x + 2, y + 2, s - 4, s - 4, r * 0.8f, RGB(26, 52, 120));
    /* onda decorativa */
    draw_ellipse_sector(x + s * 0.5f, y + s * 0.62f, s * 0.22f, s * 0.14f, 12, 24, RGB(46, 88, 170));
}
