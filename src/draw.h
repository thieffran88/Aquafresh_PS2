#ifndef AQUAFRESH_DRAW_H
#define AQUAFRESH_DRAW_H

#include "gfx.h"

void  draw_init(void);
Color color_shade(Color c, int percent);           /* 100 = igual, <100 escurece */
Color color_alpha(Color c, int a);

void draw_ellipse(float cx, float cy, float rx, float ry, Color c);
void draw_circle(float cx, float cy, float r, Color c);
/* arco preenchido (setor), angulos em "passos" de 1/24 de volta */
void draw_ellipse_sector(float cx, float cy, float rx, float ry, int step0, int step1, Color c);
void draw_line(float x1, float y1, float x2, float y2, float thick, Color c);
void draw_round_rect(float x, float y, float w, float h, float r, Color c);
void draw_frame(float x, float y, float w, float h, float t, Color c);

/* Texto (fonte 5x7). scale 2 = 10x14 pixels por caractere. */
int  text_width(const char *s, int scale);
void draw_text(float x, float y, int scale, Color c, const char *s);
void draw_text_shadow(float x, float y, int scale, Color c, const char *s);
void draw_text_center(float cx, float y, int scale, Color c, const char *s);

extern const Color creature_colors[6];
extern const char *color_names[6];
extern const char *animal_names[6];

/* Desenha o simbolo de um animal no quadrado (x,y,s). */
void draw_animal(int animal, float x, float y, float s, Color c, Color bg);
/* Desenha uma peca completa (fundo + criatura). */
void draw_tile(int tile, float x, float y, float s);
/* Peca virada para baixo (estoque). */
void draw_tile_back(float x, float y, float s);

#endif
