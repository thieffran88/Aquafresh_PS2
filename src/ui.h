#ifndef AQUAFRESH_UI_H
#define AQUAFRESH_UI_H

#include "game.h"

/* Botoes logicos (o backend converte do controle DualShock 2). */
enum {
    BTN_UP       = 1 << 0,
    BTN_DOWN     = 1 << 1,
    BTN_LEFT     = 1 << 2,
    BTN_RIGHT    = 1 << 3,
    BTN_CROSS    = 1 << 4,
    BTN_CIRCLE   = 1 << 5,
    BTN_TRIANGLE = 1 << 6,
    BTN_SQUARE   = 1 << 7,
    BTN_START    = 1 << 8,
    BTN_SELECT   = 1 << 9,
    BTN_L1       = 1 << 10,
    BTN_R1       = 1 << 11
};

void ui_init(unsigned int seed);
/* Chamado 1x por quadro (60 Hz) com os botoes pressionados agora em cada
 * controle. O controle 1 comanda o jogador 1 e os menus; o controle 2 so
 * comanda o jogador 2 (modo de 2 jogadores). */
void ui_update(unsigned int held_pad1, unsigned int held_pad2);
void ui_draw(void);

/* Consultas usadas pelo programa de teste no PC (capturas de tela). */
int  ui_state(void);
int  ui_phase(void);
void ui_cursor(int *r, int *c);
const Game *ui_game(void);

#endif
