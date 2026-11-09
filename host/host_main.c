/*
 * Aquafresh PS2 - versao de teste para PC.
 * Rasteriza em software as mesmas chamadas gfx_* do PS2, simula o controle
 * com um roteiro automatico e salva capturas de tela (PPM) em um diretorio.
 * Uso: host_shots <diretorio_saida>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/gfx.h"
#include "../src/ui.h"
#include "../src/game.h"

static unsigned char fb[SCREEN_H][SCREEN_W][3];
static long prim_count;

static void blend(int x, int y, Color c)
{
    int a = (int)COL_A(c);
    unsigned char *p;
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H)
        return;
    p = fb[y][x];
    p[0] = (unsigned char)((COL_R(c) * a + p[0] * (255 - a)) / 255);
    p[1] = (unsigned char)((COL_G(c) * a + p[1] * (255 - a)) / 255);
    p[2] = (unsigned char)((COL_B(c) * a + p[2] * (255 - a)) / 255);
}

void gfx_rect(float x, float y, float w, float h, Color c)
{
    prim_count++;
    int x0 = (int)floorf(x + 0.5f), y0 = (int)floorf(y + 0.5f);
    int x1 = (int)floorf(x + w + 0.5f), y1 = (int)floorf(y + h + 0.5f);
    int i, j;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++)
            blend(i, j, c);
}

void gfx_rect_vgrad(float x, float y, float w, float h, Color top, Color bot)
{
    int j;
    for (j = 0; j < (int)h; j++) {
        float t = h > 1 ? (float)j / (h - 1) : 0;
        Color c = RGBA((int)(COL_R(top) + (COL_R(bot) - (float)COL_R(top)) * t),
                       (int)(COL_G(top) + (COL_G(bot) - (float)COL_G(top)) * t),
                       (int)(COL_B(top) + (COL_B(bot) - (float)COL_B(top)) * t),
                       (int)(COL_A(top) + (COL_A(bot) - (float)COL_A(top)) * t));
        gfx_rect(x, y + j, w, 1, c);
    }
}

static float edge(float ax, float ay, float bx, float by, float px, float py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void gfx_tri(float x1, float y1, float x2, float y2, float x3, float y3, Color c)
{
    prim_count++;
    float area = edge(x1, y1, x2, y2, x3, y3);
    int minx, maxx, miny, maxy, x, y;
    if (fabsf(area) < 1e-6f)
        return;
    if (area < 0) {   /* orientacao consistente */
        float tx = x2, ty = y2;
        x2 = x3; y2 = y3; x3 = tx; y3 = ty;
    }
    minx = (int)floorf(fminf(x1, fminf(x2, x3)));
    maxx = (int)ceilf(fmaxf(x1, fmaxf(x2, x3)));
    miny = (int)floorf(fminf(y1, fminf(y2, y3)));
    maxy = (int)ceilf(fmaxf(y1, fmaxf(y2, y3)));
    for (y = miny; y <= maxy; y++)
        for (x = minx; x <= maxx; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float w0 = edge(x2, y2, x3, y3, px, py);
            float w1 = edge(x3, y3, x1, y1, px, py);
            float w2 = edge(x1, y1, x2, y2, px, py);
            /* regra de preenchimento simples: inclui borda em apenas um lado */
            if (w0 >= 0 && w1 >= 0 && w2 >= 0 && !(w0 == 0 || w1 == 0 || w2 == 0))
                blend(x, y, c);
            else if (w0 >= 0 && w1 >= 0 && w2 >= 0 && (w0 > 0 || w1 > 0))
                blend(x, y, c);
        }
}

static const char *outdir;

static void shot(const char *name)
{
    char path[512];
    FILE *f;
    memset(fb, 0, sizeof(fb));
    prim_count = 0;
    ui_draw();
    snprintf(path, sizeof(path), "%s/%s.ppm", outdir, name);
    f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    fwrite(fb, 1, sizeof(fb), f);
    fclose(f);
    printf("captura: %s (%ld primitivas)\n", path, prim_count);
}

static void step(unsigned held, int frames)
{
    while (frames-- > 0)
        ui_update(held);
}

static void press(unsigned b)
{
    step(b, 1);
    step(0, 1);
}

static void goto_cell(int cell)
{
    int r, c, guard = 0;
    for (;;) {
        ui_cursor(&r, &c);
        if (r * BOARD_N + c == cell || guard++ > 40) break;
        if (r < cell / BOARD_N) press(BTN_DOWN);
        else if (r > cell / BOARD_N) press(BTN_UP);
        else if (c < cell % BOARD_N) press(BTN_RIGHT);
        else press(BTN_LEFT);
    }
}

int main(int argc, char **argv)
{
    int took_move = 0, took_drop = 0, took_pause = 0, guard = 0;
    outdir = argc > 1 ? argv[1] : ".";
    ui_init(2024);

    step(0, 40);
    shot("01_titulo");
    press(BTN_DOWN); press(BTN_DOWN);
    press(BTN_CROSS);
    step(0, 5);
    shot("02_regras");
    press(BTN_RIGHT); press(BTN_RIGHT);
    shot("03_regras_pontos");
    press(BTN_CIRCLE);
    press(BTN_UP); press(BTN_UP);
    press(BTN_CROSS);               /* 1 jogador */
    press(BTN_DOWN); press(BTN_RIGHT);   /* dificuldade -> dificil */
    step(0, 10);
    shot("04_config");
    press(BTN_START);
    step(0, 20);
    shot("05_inicio");

    while (ui_state() != 5 && guard++ < 20000) {   /* 5 = resultado */
        const Game *g = ui_game();
        int ph = ui_phase();
        if (ui_state() != 3) { step(0, 1); continue; }
        if (ph == 4 || ph == 5) { step(0, 1); continue; }   /* CPU / fim */
        if (ph == 0) {          /* mover */
            if (!took_move && g->on_board >= 12) {
                int i, t[BOARD_CELLS];
                for (i = 0; i < BOARD_CELLS; i++)
                    if (g->board[i] != EMPTY && game_move_targets(g, i, t) >= 3)
                        break;
                if (i < BOARD_CELLS) {
                    goto_cell(i);
                    press(BTN_CROSS);
                    step(0, 30);
                    shot("06_mover");
                    goto_cell(t[game_move_targets(g, i, t) - 1]);
                    press(BTN_CROSS);
                    step(0, 20);
                    took_move = 1;
                    continue;
                }
            }
            if (!took_pause && g->on_board >= 20) {
                press(BTN_START);
                shot("09_pausa");
                press(BTN_START);
                took_pause = 1;
            }
            press(BTN_TRIANGLE);
        } else if (ph == 2) {   /* escolher no viveiro */
            press(BTN_RIGHT);
            press(BTN_CROSS);
        } else if (ph == 3) {   /* colocar */
            int i;
            for (i = 0; i < BOARD_CELLS; i++)
                if (g->board[(i * 7) % BOARD_CELLS] == EMPTY)
                    break;
            goto_cell((i * 7) % BOARD_CELLS);
            if (!took_drop && g->on_board >= 16) {
                step(0, 20);
                shot("07_colocar");
                took_drop = 1;
            }
            press(BTN_CROSS);
            if (g->on_board == 25) {
                step(0, 8);
                shot("08_cpu");
            }
        } else {
            step(0, 1);
        }
    }
    step(0, 10);
    shot("10_resultado");
    press(BTN_RIGHT);
    shot("11_resultado_outro");
    return 0;
}
