/*
 * Interface minima de desenho. Implementada pelo backend do PS2 (gsKit)
 * em src/main_ps2.c e por um rasterizador em software em host/host_main.c
 * (usado para gerar capturas de tela e testar no PC).
 */
#ifndef AQUAFRESH_GFX_H
#define AQUAFRESH_GFX_H

#define SCREEN_W 640
#define SCREEN_H 448

typedef unsigned int Color;   /* 0xRRGGBBAA, alfa 255 = opaco */

#define RGBA(r, g, b, a) ((Color)(((unsigned)(r) << 24) | ((unsigned)(g) << 16) | ((unsigned)(b) << 8) | (unsigned)(a)))
#define RGB(r, g, b)     RGBA(r, g, b, 255)
#define COL_R(c) (((c) >> 24) & 0xFF)
#define COL_G(c) (((c) >> 16) & 0xFF)
#define COL_B(c) (((c) >> 8) & 0xFF)
#define COL_A(c) ((c) & 0xFF)

/* Backend */
void gfx_rect(float x, float y, float w, float h, Color c);
void gfx_tri(float x1, float y1, float x2, float y2, float x3, float y3, Color c);
void gfx_rect_vgrad(float x, float y, float w, float h, Color top, Color bottom);
/* Desenha um retangulo texturizado do atlas 'atlas' (ver assets.h).
 * u/v em texels; 'mod' multiplica a cor da textura (branco = original). */
void gfx_sprite(int atlas, float x, float y, float w, float h,
                float u0, float v0, float u1, float v1, Color mod);

#endif
