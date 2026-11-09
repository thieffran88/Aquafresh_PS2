/*
 * Aquafresh PS2 - telas, menus e fluxo de partida.
 */
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "game.h"
#include "draw.h"

enum { ST_TITLE, ST_SETUP, ST_RULES, ST_PLAY, ST_PAUSE, ST_RESULT };
enum { PH_MOVE, PH_MOVE_DEST, PH_PICK, PH_DROP, PH_CPU, PH_END };

/* ---------------------------------------------------------------- layout */
#define BOARD_X   30
#define BOARD_Y   58
#define CELL      58
#define TILE_IN   3
#define PANEL_X   400
#define PANEL_W   220
#define CARD_H    66
#define CARD1_Y   54
#define CARD2_Y   126
#define POOL_Y    222
#define POOL_S    60
#define POOL_GAP  8
#define HELP_Y    420

/* ----------------------------------------------------------------- estado */
static int state = ST_TITLE, rules_return = ST_TITLE;
static unsigned int frame_no, seed_base;
static unsigned int prev_held, pressed;
static int repeat_btn, repeat_timer;

static int menu_sel;
static int num_humans = 1;          /* 1 = contra CPU, 2 = dois jogadores */
static int opt_role = ROLE_COLOR;   /* papel do jogador 0 */
static int opt_level = AI_NORMAL;
static int opt_first = 0;
static int setup_sel;
static int rules_page;
static int pause_sel;

static Game game, undo_game;
static int is_cpu[2];
static int phase;
static int cur_r = 2, cur_c = 2;
static int pool_sel;
static int sel_cell = -1;
static int targets[BOARD_CELLS], ntargets;
static int moved_this_turn;
static int mark_from = -1, mark_to = -1, mark_cell = -1;
static int undo_marks[3];
static int timer, cpu_step;
static Turn cpu_turn;
static int banner_timer;
static char msg[64];
static int msg_timer;
static int result_view;             /* qual papel destacar na tela final */
static ScoreDetail result_detail[2];

static struct {
    int active, cell, t, dur;
    float x0, y0;
} anim;

/* ---------------------------------------------------------------- helpers */

static const char *player_name(int p)
{
    if (num_humans == 1)
        return p == 0 ? "VOCE" : "CPU";
    return p == 0 ? "JOGADOR 1" : "JOGADOR 2";
}

static const char *role_name(int role)
{
    return role == ROLE_COLOR ? "CORES" : "ANIMAIS";
}

static void cell_pos(int cell, float *x, float *y)
{
    *x = (float)(BOARD_X + (cell % BOARD_N) * CELL);
    *y = (float)(BOARD_Y + (cell / BOARD_N) * CELL);
}

static void pool_pos(int i, float *x, float *y)
{
    float total = 3 * POOL_S + 2 * POOL_GAP;
    *x = PANEL_X + (PANEL_W - total) / 2 + (i % 3) * (POOL_S + POOL_GAP);
    *y = (float)(POOL_Y + (i / 3) * (POOL_S + POOL_GAP));
}

static void set_msg(const char *m)
{
    strncpy(msg, m, sizeof(msg) - 1);
    msg[sizeof(msg) - 1] = 0;
    msg_timer = 150;
}

static void start_anim(int cell, float x0, float y0)
{
    anim.active = 1;
    anim.cell = cell;
    anim.x0 = x0;
    anim.y0 = y0;
    anim.t = 0;
    anim.dur = 14;
}

static int pulse(int period, int lo, int hi)
{
    int t = (int)(frame_no % (unsigned)period);
    int half = period / 2;
    int v = t < half ? t : period - t;
    return lo + (hi - lo) * v / half;
}

/* ----------------------------------------------------------------- partida */

static void start_turn(void);

static void new_game(void)
{
    unsigned int seed = seed_base * 2654435761u ^ (frame_no * 40503u) ^ 0x5EEDu;
    game_new(&game, seed, opt_first, opt_role);
    is_cpu[0] = 0;
    is_cpu[1] = num_humans == 1;
    mark_from = mark_to = mark_cell = -1;
    anim.active = 0;
    msg_timer = 0;
    pool_sel = 0;
    cur_r = cur_c = 2;
    state = ST_PLAY;
    start_turn();
}

static void start_turn(void)
{
    moved_this_turn = 0;
    sel_cell = -1;
    if (game_is_over(&game)) {
        phase = PH_END;
        timer = 80;
        return;
    }
    if (pool_sel >= game.pool_n)
        pool_sel = game.pool_n - 1;
    if (pool_sel < 0)
        pool_sel = 0;
    if (is_cpu[game.turn]) {
        phase = PH_CPU;
        cpu_step = 0;
        timer = 35;
    } else {
        phase = game_first_turn(&game) ? PH_PICK : PH_MOVE;
        banner_timer = num_humans == 2 ? 80 : 50;
    }
}

static void enter_results(void)
{
    game_score_player(&game, 0, &result_detail[0]);
    game_score_player(&game, 1, &result_detail[1]);
    result_view = game.role[0];
    state = ST_RESULT;
}

static void finish_place(int pool_idx, int cell)
{
    float px, py;
    pool_pos(pool_idx, &px, &py);
    if (game_place(&game, pool_idx, cell) == 0) {
        start_anim(cell, px, py);
        mark_cell = cell;
        if (!moved_this_turn)
            mark_from = mark_to = -1;
        start_turn();
    }
}

static void do_move(int from, int to)
{
    float fx, fy;
    cell_pos(from, &fx, &fy);
    undo_game = game;
    undo_marks[0] = mark_from;
    undo_marks[1] = mark_to;
    undo_marks[2] = mark_cell;
    game_do_move(&game, from, to);
    start_anim(to, fx, fy);
    mark_from = from;
    mark_to = to;
    mark_cell = -1;
    moved_this_turn = 1;
}

static void cursor_dirs(int *r, int *c)
{
    if (pressed & BTN_UP)    *r = (*r + BOARD_N - 1) % BOARD_N;
    if (pressed & BTN_DOWN)  *r = (*r + 1) % BOARD_N;
    if (pressed & BTN_LEFT)  *c = (*c + BOARD_N - 1) % BOARD_N;
    if (pressed & BTN_RIGHT) *c = (*c + 1) % BOARD_N;
}

static void update_play(void)
{
    int cell = cur_r * BOARD_N + cur_c;
    int i;

    if (anim.active && ++anim.t >= anim.dur)
        anim.active = 0;
    if (banner_timer > 0)
        banner_timer = pressed ? 0 : banner_timer - 1;   /* some ao apertar algo */
    if (msg_timer > 0)
        msg_timer--;

    if (pressed & BTN_START && phase != PH_END) {
        state = ST_PAUSE;
        pause_sel = 0;
        return;
    }

    switch (phase) {
    case PH_MOVE:
        cursor_dirs(&cur_r, &cur_c);
        if (pressed & BTN_TRIANGLE) {
            phase = PH_PICK;
        } else if (pressed & BTN_CROSS) {
            if (game.board[cell] == EMPTY) {
                set_msg("ESCOLHA UMA CRIATURA OU APERTE ^ PARA PULAR");
            } else {
                ntargets = game_move_targets(&game, cell, targets);
                if (ntargets == 0) {
                    set_msg("ESSA CRIATURA ESTA CERCADA");
                } else {
                    sel_cell = cell;
                    phase = PH_MOVE_DEST;
                }
            }
        }
        break;

    case PH_MOVE_DEST:
        cursor_dirs(&cur_r, &cur_c);
        if (pressed & BTN_CIRCLE || (pressed & BTN_CROSS && cell == sel_cell)) {
            phase = PH_MOVE;
            sel_cell = -1;
        } else if (pressed & BTN_CROSS) {
            if (game_can_move(&game, sel_cell, cell)) {
                do_move(sel_cell, cell);
                sel_cell = -1;
                phase = PH_PICK;
            } else {
                set_msg("SO EM LINHA RETA, SEM PULAR CRIATURAS");
            }
        }
        break;

    case PH_PICK: {
        int n = game.pool_n;
        if (pressed & BTN_LEFT)  pool_sel = (pool_sel + n - 1) % n;
        if (pressed & BTN_RIGHT || pressed & BTN_R1) pool_sel = (pool_sel + 1) % n;
        if (pressed & BTN_L1)    pool_sel = (pool_sel + n - 1) % n;
        if (pressed & BTN_UP && pool_sel >= 3) pool_sel -= 3;
        if (pressed & BTN_DOWN && pool_sel + 3 < n) pool_sel += 3;
        if (pressed & BTN_CROSS) {
            phase = PH_DROP;
        } else if (pressed & BTN_CIRCLE && !game_first_turn(&game)) {
            if (moved_this_turn) {
                game = undo_game;
                mark_from = undo_marks[0];
                mark_to = undo_marks[1];
                mark_cell = undo_marks[2];
                moved_this_turn = 0;
                anim.active = 0;
            }
            phase = PH_MOVE;
        }
        break;
    }

    case PH_DROP:
        cursor_dirs(&cur_r, &cur_c);
        if (pressed & BTN_L1) pool_sel = (pool_sel + game.pool_n - 1) % game.pool_n;
        if (pressed & BTN_R1) pool_sel = (pool_sel + 1) % game.pool_n;
        if (pressed & BTN_CIRCLE) {
            phase = PH_PICK;
        } else if (pressed & BTN_CROSS) {
            if (game.board[cell] != EMPTY)
                set_msg("ESCOLHA UMA CASA VAZIA");
            else
                finish_place(pool_sel, cell);
        }
        break;

    case PH_CPU:
        if (timer > 0) {
            timer--;
            break;
        }
        switch (cpu_step) {
        case 0:
            ai_choose(&game, opt_level, &cpu_turn);
            if (cpu_turn.move_from >= 0) {
                do_move(cpu_turn.move_from, cpu_turn.move_to);
                timer = 45;
            }
            cpu_step = 1;
            break;
        case 1:
            pool_sel = cpu_turn.pool_idx;
            timer = 35;
            cpu_step = 2;
            break;
        default:
            i = cpu_turn.pool_idx;
            finish_place(i, cpu_turn.cell);   /* chama start_turn */
            break;
        }
        break;

    case PH_END:
        if (timer > 0)
            timer--;
        else
            enter_results();
        break;
    }
}

/* -------------------------------------------------------------- atualizar */

#define MENU_ITEMS 3

static void update_title(void)
{
    if (pressed & BTN_UP)   menu_sel = (menu_sel + MENU_ITEMS - 1) % MENU_ITEMS;
    if (pressed & BTN_DOWN) menu_sel = (menu_sel + 1) % MENU_ITEMS;
    if (pressed & (BTN_CROSS | BTN_START)) {
        seed_base ^= frame_no;
        if (menu_sel == 2) {
            rules_page = 0;
            rules_return = ST_TITLE;
            state = ST_RULES;
        } else {
            num_humans = menu_sel + 1;
            setup_sel = 0;
            state = ST_SETUP;
        }
    }
}

static int setup_rows(void) { return num_humans == 1 ? 4 : 3; }

static void update_setup(void)
{
    int rows = setup_rows();
    int dir = 0, row;
    if (pressed & BTN_UP)   setup_sel = (setup_sel + rows - 1) % rows;
    if (pressed & BTN_DOWN) setup_sel = (setup_sel + 1) % rows;
    if (pressed & BTN_LEFT)  dir = -1;
    if (pressed & BTN_RIGHT) dir = 1;
    if (pressed & BTN_CIRCLE) {
        state = ST_TITLE;
        return;
    }
    /* mapeia linha para opcao */
    row = setup_sel;
    if (num_humans == 2 && row >= 1)
        row++;   /* sem dificuldade */
    if ((pressed & BTN_CROSS) && row != 3)
        dir = 1;
    if (dir) {
        if (row == 0) opt_role ^= 1;
        if (row == 1) opt_level = (opt_level + 3 + dir) % 3;
        if (row == 2) opt_first ^= 1;
    }
    if ((pressed & (BTN_CROSS | BTN_START)) && (row == 3 || (pressed & BTN_START))) {
        seed_base ^= frame_no * 7u;
        new_game();
    }
}

#define RULES_PAGES 4

static void update_rules(void)
{
    if (pressed & (BTN_RIGHT | BTN_CROSS | BTN_R1)) {
        if (rules_page + 1 < RULES_PAGES)
            rules_page++;
        else if (pressed & BTN_CROSS)
            state = rules_return;
    }
    if (pressed & (BTN_LEFT | BTN_L1) && rules_page > 0)
        rules_page--;
    if (pressed & (BTN_CIRCLE | BTN_START))
        state = rules_return;
}

static void update_pause(void)
{
    if (pressed & BTN_UP)   pause_sel = (pause_sel + 2) % 3;
    if (pressed & BTN_DOWN) pause_sel = (pause_sel + 1) % 3;
    if (pressed & (BTN_START | BTN_CIRCLE)) {
        state = ST_PLAY;
        return;
    }
    if (pressed & BTN_CROSS) {
        if (pause_sel == 0) state = ST_PLAY;
        if (pause_sel == 1) {
            rules_page = 0;
            rules_return = ST_PAUSE;
            state = ST_RULES;
        }
        if (pause_sel == 2) state = ST_TITLE;
    }
}

static void update_result(void)
{
    if (pressed & (BTN_LEFT | BTN_RIGHT | BTN_L1 | BTN_R1))
        result_view ^= 1;
    if (pressed & BTN_CROSS) {
        opt_role ^= 1;         /* revanche: papeis trocados */
        new_game();
    } else if (pressed & (BTN_CIRCLE | BTN_START)) {
        state = ST_TITLE;
    }
}

void ui_init(unsigned int seed)
{
    draw_init();
    seed_base = seed ? seed : 1u;
    state = ST_TITLE;
}

int ui_state(void) { return state; }
int ui_phase(void) { return phase; }
const Game *ui_game(void) { return &game; }
void ui_cursor(int *r, int *c) { *r = cur_r; *c = cur_c; }

void ui_update(unsigned int held)
{
    const unsigned int dirs = BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT;
    frame_no++;
    pressed = held & ~prev_held;
    /* repeticao automatica do direcional */
    if (held & dirs & repeat_btn) {
        if (--repeat_timer <= 0) {
            pressed |= held & dirs & repeat_btn;
            repeat_timer = 6;
        }
    } else {
        repeat_btn = (int)(pressed & dirs);
        repeat_timer = 18;
    }
    prev_held = held;

    switch (state) {
    case ST_TITLE:  update_title(); break;
    case ST_SETUP:  update_setup(); break;
    case ST_RULES:  update_rules(); break;
    case ST_PLAY:   update_play(); break;
    case ST_PAUSE:  update_pause(); break;
    case ST_RESULT: update_result(); break;
    }
}

/* ---------------------------------------------------------------- desenho */

static void draw_background(void)
{
    int i;
    gfx_rect_vgrad(0, 0, SCREEN_W, SCREEN_H, RGB(20, 120, 190), RGB(6, 30, 80));
    /* raios de luz */
    for (i = 0; i < 4; i++) {
        float x = 60.0f + i * 170.0f + (float)pulse(600 + i * 97, 0, 40);
        gfx_tri(x, 0, x + 70, 0, x - 60, SCREEN_H, RGBA(200, 240, 255, 18));
    }
    /* bolhas */
    for (i = 0; i < 14; i++) {
        unsigned int h = (unsigned)i * 2654435761u;
        float speed = 0.3f + (h % 7) * 0.08f;
        float x = (float)(h % SCREEN_W) + (float)pulse(120 + i * 13, -6, 6);
        float y = SCREEN_H + 20 - (float)((unsigned)((frame_no + h % 997) * speed) % (SCREEN_H + 40));
        float r = 2.0f + (h >> 8) % 5;
        draw_circle(x, y, r, RGBA(220, 245, 255, 70));
        draw_circle(x - r * 0.3f, y - r * 0.3f, r * 0.35f, RGBA(255, 255, 255, 110));
    }
    /* areia e algas */
    for (i = 0; i < 9; i++) {
        float x = 10.0f + i * 75.0f;
        float sway = (float)pulse(180 + i * 11, -8, 8);
        Color alga = (i & 1) ? RGBA(40, 160, 110, 140) : RGBA(30, 130, 90, 140);
        draw_line(x, SCREEN_H, x + sway, SCREEN_H - 40 - (i % 3) * 14, 7, alga);
    }
}

static void draw_mini_role(int role, float x, float y, float s)
{
    int i;
    for (i = 0; i < 6; i++) {
        float xx = x + i * (s + 3);
        if (role == ROLE_COLOR)
            draw_round_rect(xx, y, s, s, s * 0.25f, creature_colors[i]);
        else
            draw_animal(i, xx - 1, y - 1, s + 2, RGB(235, 240, 255), RGB(18, 38, 92));
    }
}

static void draw_player_card(int p, float y)
{
    char buf[16];
    int active = (state == ST_PLAY || state == ST_PAUSE) && game.turn == p && phase != PH_END;
    Color border = active ? RGB(120, 240, 255) : RGBA(150, 200, 255, 90);
    int score = game_score_player(&game, p, 0);

    if (active)
        draw_round_rect(PANEL_X - 3, y - 3, PANEL_W + 6, CARD_H + 6, 12,
                        RGBA(120, 240, 255, pulse(60, 40, 120)));
    draw_round_rect(PANEL_X, y, PANEL_W, CARD_H, 10, border);
    draw_round_rect(PANEL_X + 2, y + 2, PANEL_W - 4, CARD_H - 4, 9, RGBA(6, 24, 64, 230));
    draw_text_shadow(PANEL_X + 10, y + 8, 2, active ? RGB(255, 255, 255) : RGB(180, 200, 230),
                     player_name(p));
    draw_text(PANEL_X + 10, y + 28, 2, RGB(140, 220, 255), role_name(game.role[p]));
    draw_mini_role(game.role[p], PANEL_X + 10, y + 47, 11);
    snprintf(buf, sizeof(buf), "%d", score);
    draw_text_shadow(PANEL_X + PANEL_W - 12 - text_width(buf, 4), y + 12, 4,
                     RGB(255, 230, 120), buf);
    draw_text(PANEL_X + PANEL_W - 12 - text_width("PTS", 1), y + 50, 1, RGB(180, 200, 230), "PTS");
}

static void board_frame(void)
{
    int i;
    float bw = BOARD_N * CELL;
    draw_round_rect(BOARD_X - 12, BOARD_Y - 12, bw + 24, bw + 24, 16, RGBA(160, 230, 255, 90));
    draw_round_rect(BOARD_X - 8, BOARD_Y - 8, bw + 16, bw + 16, 13, RGBA(30, 110, 170, 220));
    for (i = 0; i < BOARD_CELLS; i++) {
        float x, y;
        cell_pos(i, &x, &y);
        draw_round_rect(x + TILE_IN, y + TILE_IN, CELL - 2 * TILE_IN, CELL - 2 * TILE_IN, 7,
                        RGBA(140, 215, 245, 70));
    }
}

static void schools_mask(unsigned char *in_school, int role)
{
    int i, j;
    /* uma celula esta num cardume se tem vizinho ortogonal com a mesma chave */
    for (i = 0; i < BOARD_CELLS; i++) {
        int r = i / BOARD_N, c = i % BOARD_N, t = game.board[i];
        int key, nb[4], nn = 0;
        in_school[i] = 0;
        if (t == EMPTY)
            continue;
        key = role == ROLE_COLOR ? TILE_COLOR(t) : TILE_ANIMAL(t);
        if (r > 0) nb[nn++] = i - BOARD_N;
        if (r < BOARD_N - 1) nb[nn++] = i + BOARD_N;
        if (c > 0) nb[nn++] = i - 1;
        if (c < BOARD_N - 1) nb[nn++] = i + 1;
        for (j = 0; j < nn; j++) {
            int u = game.board[nb[j]];
            if (u != EMPTY && (role == ROLE_COLOR ? TILE_COLOR(u) : TILE_ANIMAL(u)) == key)
                in_school[i] = 1;
        }
    }
}

static void draw_board(int interactive)
{
    int i, cell = cur_r * BOARD_N + cur_c;
    unsigned char in_school[BOARD_CELLS];
    int dim = state == ST_RESULT;

    board_frame();
    if (dim)
        schools_mask(in_school, result_view);

    /* casas de destino possiveis */
    if (interactive && phase == PH_MOVE_DEST) {
        for (i = 0; i < ntargets; i++) {
            float x, y;
            cell_pos(targets[i], &x, &y);
            draw_round_rect(x + TILE_IN, y + TILE_IN, CELL - 2 * TILE_IN, CELL - 2 * TILE_IN, 7,
                            RGBA(120, 255, 160, pulse(50, 60, 130)));
            draw_circle(x + CELL / 2, y + CELL / 2, 6, RGBA(200, 255, 210, 200));
        }
    }

    /* marcas da ultima jogada */
    if (mark_from >= 0 && state != ST_RESULT) {
        float x, y;
        cell_pos(mark_from, &x, &y);
        draw_frame(x + TILE_IN + 4, y + TILE_IN + 4, CELL - 2 * TILE_IN - 8, CELL - 2 * TILE_IN - 8,
                   2, RGBA(255, 220, 120, 150));
    }

    for (i = 0; i < BOARD_CELLS; i++) {
        float x, y;
        if (game.board[i] == EMPTY)
            continue;
        cell_pos(i, &x, &y);
        if (anim.active && anim.cell == i) {
            float t = (float)anim.t / anim.dur;
            t = 1.0f - (1.0f - t) * (1.0f - t);
            x = anim.x0 + (x - anim.x0) * t;
            y = anim.y0 + (y - anim.y0) * t;
        }
        draw_tile(game.board[i], x + TILE_IN, y + TILE_IN, CELL - 2 * TILE_IN);
        if (dim && !in_school[i])
            draw_round_rect(x + TILE_IN, y + TILE_IN, CELL - 2 * TILE_IN, CELL - 2 * TILE_IN, 7,
                            RGBA(4, 16, 40, 175));
    }

    if (state != ST_RESULT && !anim.active) {
        if (mark_to >= 0) {
            float x, y;
            cell_pos(mark_to, &x, &y);
            draw_frame(x + 1, y + 1, CELL - 2, CELL - 2, 3, RGBA(255, 220, 120, 220));
        }
        if (mark_cell >= 0) {
            float x, y;
            cell_pos(mark_cell, &x, &y);
            draw_frame(x + 1, y + 1, CELL - 2, CELL - 2, 3, RGBA(255, 255, 255, 200));
        }
    }

    if (!interactive)
        return;

    if (sel_cell >= 0) {
        float x, y;
        cell_pos(sel_cell, &x, &y);
        draw_frame(x - 2, y - 2, CELL + 4, CELL + 4, 4, RGB(255, 160, 40));
    }

    if (phase == PH_MOVE || phase == PH_MOVE_DEST || phase == PH_DROP) {
        float x, y;
        cell_pos(cell, &x, &y);
        if (phase == PH_DROP && game.board[cell] == EMPTY && game.pool_n > 0) {
            draw_tile(game.pool[pool_sel], x + TILE_IN, y + TILE_IN, CELL - 2 * TILE_IN);
            draw_round_rect(x + TILE_IN, y + TILE_IN, CELL - 2 * TILE_IN, CELL - 2 * TILE_IN, 7,
                            RGBA(255, 255, 255, pulse(40, 20, 70)));
        }
        draw_frame(x - 3, y - 3, CELL + 6, CELL + 6, 4, RGBA(255, 255, 255, pulse(40, 150, 255)));
    }
}

static void draw_pool(int interactive)
{
    int i;
    char buf[24];
    draw_text_shadow(PANEL_X + 4, POOL_Y - 22, 2, RGB(220, 240, 255), "VIVEIRO");
    for (i = 0; i < POOL_MAX; i++) {
        float x, y;
        pool_pos(i, &x, &y);
        draw_round_rect(x, y, POOL_S, POOL_S, 8, RGBA(140, 215, 245, 50));
        if (i < game.pool_n) {
            int sel = interactive && i == pool_sel &&
                      (phase == PH_PICK || phase == PH_DROP || (phase == PH_CPU && cpu_step >= 2));
            float lift = sel ? (phase == PH_DROP ? -6.0f : (float)-pulse(30, 0, 4)) : 0.0f;
            if (sel)
                draw_round_rect(x - 4, y - 4 + lift, POOL_S + 8, POOL_S + 8, 10,
                                phase == PH_CPU ? RGB(255, 120, 120) : RGB(255, 255, 255));
            draw_tile(game.pool[i], x, y + lift, POOL_S);
        }
    }
    /* estoque */
    {
        float y = POOL_Y + 2 * POOL_S + POOL_GAP + 10;
        int n = game.stock_n < 3 ? game.stock_n : 3;
        for (i = 0; i < n; i++)
            draw_tile_back(PANEL_X + 8 + i * 4, y - i * 3, 34);
        snprintf(buf, sizeof(buf), "ESTOQUE: %d", game.stock_n);
        draw_text_shadow(PANEL_X + 56, y + 10, 2, RGB(200, 225, 255), buf);
    }
}

static void draw_help(const char *text, Color c)
{
    gfx_rect(0, HELP_Y - 6, SCREEN_W, 30, RGBA(0, 10, 30, 150));
    draw_text_center(SCREEN_W / 2, HELP_Y, 2, c, text);
}

static void draw_banner(const char *text, int alpha)
{
    int w = text_width(text, 3) + 40;
    float x = BOARD_X + BOARD_N * CELL / 2 - w / 2;
    float y = BOARD_Y + BOARD_N * CELL / 2 - 26;
    draw_round_rect(x, y, (float)w, 52, 14, RGBA(0, 20, 60, alpha * 200 / 255));
    draw_frame(x + 6, y + 6, w - 12, 40, 2, RGBA(120, 240, 255, alpha));
    draw_text(x + 20, y + 16, 3, RGBA(255, 255, 255, alpha), text);
}

static void draw_phase_info(void)
{
    const char *l = "";
    switch (phase) {
    case PH_MOVE:      l = "1. MOVER (OPCIONAL)"; break;
    case PH_MOVE_DEST: l = "1. MOVER"; break;
    case PH_PICK:
    case PH_DROP:      l = "2. COLOCAR"; break;
    case PH_CPU:       l = cpu_step == 0 ? "CPU PENSANDO..." : "CPU JOGANDO"; break;
    default:           l = "RECIFE CHEIO!"; break;
    }
    draw_text_shadow(PANEL_X + PANEL_W - text_width(l, 2), 20, 2, RGB(255, 230, 120), l);
}

static void draw_title_bar(void)
{
    draw_text_shadow(BOARD_X - 6, 14, 3, RGB(255, 255, 255), "AQUAFRESH");
    draw_text_shadow(BOARD_X - 6 + text_width("AQUAFRESH ", 3), 14, 3, RGB(120, 240, 255), "PS2");
}

static void draw_play(void)
{
    char buf[48];
    int interactive = state == ST_PLAY && !is_cpu[game.turn];
    draw_title_bar();
    draw_board(state == ST_PLAY && phase != PH_CPU && phase != PH_END);
    draw_player_card(0, CARD1_Y);
    draw_player_card(1, CARD2_Y);
    draw_pool(state == ST_PLAY);
    draw_phase_info();

    if (msg_timer > 0) {
        draw_help(msg, RGB(255, 170, 150));
    } else if (!interactive) {
        draw_help(phase == PH_END ? "CONTANDO OS CARDUMES..." : "CPU JOGANDO...   START PAUSA",
                  RGB(220, 235, 255));
    } else {
        const char *h = "";
        switch (phase) {
        case PH_MOVE:      h = "x MOVER CRIATURA   ^ PULAR   START PAUSA"; break;
        case PH_MOVE_DEST: h = "x ESCOLHER DESTINO   o CANCELAR"; break;
        case PH_PICK:      h = game_first_turn(&game) ? "x ESCOLHER CRIATURA" : "x ESCOLHER CRIATURA   o VOLTAR"; break;
        case PH_DROP:      h = "x COLOCAR   L1/R1 TROCAR   o VOLTAR"; break;
        }
        draw_help(h, RGB(220, 235, 255));
    }

    if (state == ST_PLAY && banner_timer > 0 && !is_cpu[game.turn]) {
        int a = banner_timer > 20 ? 255 : banner_timer * 255 / 20;
        snprintf(buf, sizeof(buf), num_humans == 2 ? "VEZ DE %s" : "SUA VEZ!", player_name(game.turn));
        draw_banner(buf, a);
    }
}

static void draw_menu_item(float cx, float y, int scale, const char *s, int sel)
{
    if (sel) {
        int w = text_width(s, scale) + 40;
        draw_round_rect(cx - w / 2, y - 8, (float)w, 7 * scale + 16, 10,
                        RGBA(120, 240, 255, pulse(60, 70, 140)));
    }
    draw_text_center(cx, y, scale, sel ? RGB(255, 255, 255) : RGB(170, 205, 240), s);
}

static void draw_title(void)
{
    static const char *items[MENU_ITEMS] = { "1 JOGADOR", "2 JOGADORES", "COMO JOGAR" };
    int i;
    float bob;
    draw_text_center(SCREEN_W / 2 + 3, 46, 9, RGB(10, 60, 130), "AQUAFRESH");
    draw_text_center(SCREEN_W / 2, 42, 9, RGB(240, 252, 255), "AQUAFRESH");
    draw_round_rect(SCREEN_W / 2 - 62, 118, 124, 46, 12, RGB(120, 240, 255));
    draw_text_center(SCREEN_W / 2, 125, 5, RGB(10, 50, 110), "PS2");
    draw_text_center(SCREEN_W / 2, 178, 2, RGB(210, 240, 255), "FORME OS MAIORES CARDUMES DO RECIFE");
    for (i = 0; i < 6; i++) {
        bob = (float)pulse(90 + i * 7, -5, 5);
        draw_tile(MAKE_TILE(i, (i * 5 + 2) % 6), 118 + i * 70, 208 + bob, 56);
    }
    for (i = 0; i < MENU_ITEMS; i++)
        draw_menu_item(SCREEN_W / 2, 296 + i * 38, 3, items[i], i == menu_sel);
    draw_help("x CONFIRMAR", RGB(220, 235, 255));
}

static void draw_option_row(float y, const char *label, const char *value, int sel, int arrows)
{
    if (sel)
        draw_round_rect(70, y - 8, 500, 34, 10, RGBA(120, 240, 255, pulse(60, 60, 120)));
    draw_text_shadow(90, y, 2, sel ? RGB(255, 255, 255) : RGB(170, 205, 240), label);
    if (value) {
        char buf[40];
        if (arrows)
            snprintf(buf, sizeof(buf), "< %s >", value);
        else
            snprintf(buf, sizeof(buf), "%s", value);
        draw_text_shadow(550 - text_width(buf, 2), y, 2, RGB(255, 230, 120), buf);
    }
}

static void draw_setup(void)
{
    static const char *levels[3] = { "FACIL", "NORMAL", "DIFICIL" };
    char buf[64];
    int row = 0;
    float y = 120;
    draw_text_center(SCREEN_W / 2, 36, 4, RGB(255, 255, 255),
                     num_humans == 1 ? "1 JOGADOR" : "2 JOGADORES");
    draw_round_rect(50, 96, 540, 270, 16, RGBA(0, 20, 60, 170));

    if (num_humans == 1) {
        draw_option_row(y, "SEUS CARDUMES", role_name(opt_role), setup_sel == row++, 1);
        y += 44;
        draw_option_row(y, "DIFICULDADE", levels[opt_level], setup_sel == row++, 1);
        y += 44;
        draw_option_row(y, "QUEM COMECA", opt_first == 0 ? "VOCE" : "CPU", setup_sel == row++, 1);
        y += 44;
    } else {
        snprintf(buf, sizeof(buf), "%s", role_name(opt_role));
        draw_option_row(y, "JOGADOR 1 FORMA", buf, setup_sel == row++, 1);
        y += 44;
        draw_option_row(y, "QUEM COMECA", opt_first == 0 ? "JOGADOR 1" : "JOGADOR 2",
                        setup_sel == row++, 1);
        y += 44;
    }
    draw_option_row(y, "COMECAR PARTIDA!", 0, setup_sel == row, 0);

    y = 300;
    snprintf(buf, sizeof(buf), "%s: CARDUMES DE %s", num_humans == 1 ? "VOCE" : "JOGADOR 1",
             opt_role == ROLE_COLOR ? "MESMA COR" : "MESMO ANIMAL");
    draw_text_center(SCREEN_W / 2, y, 2, RGB(200, 235, 255), buf);
    draw_mini_role(opt_role, SCREEN_W / 2 - 50 - 60, y + 24, 14);
    snprintf(buf, sizeof(buf), "%s", num_humans == 1 ? "CPU" : "JOGADOR 2");
    draw_text(SCREEN_W / 2 + 40, y + 26, 2, RGB(200, 235, 255), buf);
    draw_mini_role(opt_role ^ 1, SCREEN_W / 2 + 40 + text_width(buf, 2) + 10, y + 24, 14);
    draw_help("DIRECIONAL ALTERA   x CONFIRMAR   o VOLTAR", RGB(220, 235, 255));
}

static void draw_rules(void)
{
    static const char *titles[RULES_PAGES] = {
        "OBJETIVO", "SEU TURNO", "PONTUACAO", "CONTROLES"
    };
    static const char *pages[RULES_PAGES][9] = {
        { "CRIATURAS COLORIDAS NADAM PELO RECIFE.",
          "UM JOGADOR FORMA CARDUMES DA MESMA COR,",
          "O OUTRO CARDUMES DO MESMO ANIMAL.",
          "",
          "CARDUME = 2 OU MAIS CRIATURAS VIZINHAS",
          "LADO A LADO (DIAGONAL NAO CONTA).",
          "",
          "QUANDO AS 36 CRIATURAS ENCHEREM O RECIFE,",
          "QUEM TIVER MAIS PONTOS VENCE!" },
        { "1. MOVER (OPCIONAL): DESLIZE UMA CRIATURA",
          "   DO RECIFE NA LINHA OU COLUNA, QUANTAS",
          "   CASAS QUISER, SEM PULAR OUTRAS.",
          "   (NAO EXISTE NO PRIMEIRO TURNO)",
          "2. COLOCAR: PEGUE UMA CRIATURA DO VIVEIRO",
          "   E PONHA EM QUALQUER CASA VAZIA.",
          "3. REPOR: UMA CRIATURA DO ESTOQUE VAI",
          "   PARA O VIVEIRO (AUTOMATICO).",
          "QUALQUER JOGADOR PODE MOVER QUALQUER UMA." },
        { "CRIATURAS NO CARDUME:  2  3  4  5   6",
          "PONTOS:                1  3  6  10  15",
          "",
          "UMA CRIATURA SOZINHA NAO VALE NADA.",
          "UMA CRIATURA PODE CONTAR NUM CARDUME DE",
          "COR E NUM DE ANIMAL AO MESMO TEMPO.",
          "EMPATE: VENCE QUEM JOGOU EM SEGUNDO.",
          "REVANCHE: OS PAPEIS SAO TROCADOS.",
          "" },
        { "DIRECIONAL   MOVE O CURSOR",
          "x            CONFIRMA",
          "o            VOLTA / CANCELA",
          "^            PULA O MOVIMENTO",
          "L1 / R1      TROCA A CRIATURA DO VIVEIRO",
          "START        PAUSA",
          "",
          "NA TELA FINAL, DIRECIONAL ALTERNA ENTRE",
          "OS CARDUMES DE CADA JOGADOR." },
    };
    char buf[32];
    int i;
    draw_round_rect(24, 20, 592, 384, 16, RGBA(0, 20, 60, 190));
    draw_text_shadow(48, 38, 4, RGB(120, 240, 255), titles[rules_page]);
    snprintf(buf, sizeof(buf), "%d/%d", rules_page + 1, RULES_PAGES);
    draw_text_shadow(592 - text_width(buf, 2), 44, 2, RGB(200, 225, 255), buf);
    for (i = 0; i < 9; i++)
        draw_text_shadow(48, 92 + i * 28, 2, RGB(235, 245, 255), pages[rules_page][i]);
    if (rules_page == 0) {
        for (i = 0; i < 6; i++)
            draw_tile(MAKE_TILE(i, i), 140 + i * 62, 352, 44);
    } else if (rules_page == 2) {
        /* exemplo: 3 azuis em linha = 3 pontos */
        draw_tile(MAKE_TILE(COL_BLUE, ANI_CRAB), 120, 336, 50);
        draw_tile(MAKE_TILE(COL_BLUE, ANI_STAR), 172, 336, 50);
        draw_tile(MAKE_TILE(COL_BLUE, ANI_FISH), 224, 336, 50);
        draw_text_shadow(290, 354, 2, RGB(255, 230, 120), "= 3 PONTOS PARA CORES");
    }
    draw_help("< > PAGINA   x PROXIMA   o SAIR", RGB(220, 235, 255));
}

static void draw_pause(void)
{
    static const char *items[3] = { "CONTINUAR", "COMO JOGAR", "SAIR PARA O MENU" };
    int i;
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, RGBA(0, 10, 30, 160));
    draw_round_rect(150, 110, 340, 220, 18, RGBA(0, 30, 80, 230));
    draw_text_center(SCREEN_W / 2, 132, 4, RGB(255, 255, 255), "PAUSA");
    for (i = 0; i < 3; i++)
        draw_menu_item(SCREEN_W / 2, 200 + i * 40, 2, items[i], i == pause_sel);
}

static void draw_result_card(int p, float y)
{
    char buf[40];
    int i, n = 0;
    const ScoreDetail *d = &result_detail[p];
    int view = game.role[p] == result_view;
    draw_round_rect(PANEL_X, y, PANEL_W, 150, 10, view ? RGB(120, 240, 255) : RGBA(150, 200, 255, 90));
    draw_round_rect(PANEL_X + 2, y + 2, PANEL_W - 4, 146, 9, RGBA(6, 24, 64, 235));
    draw_text_shadow(PANEL_X + 10, y + 8, 2, RGB(255, 255, 255), player_name(p));
    draw_text(PANEL_X + 10, y + 28, 2, RGB(140, 220, 255), role_name(game.role[p]));
    snprintf(buf, sizeof(buf), "%d", d->total);
    draw_text_shadow(PANEL_X + PANEL_W - 12 - text_width(buf, 4), y + 10, 4, RGB(255, 230, 120), buf);
    for (i = 0; i < 6; i++) {
        float x = PANEL_X + 12 + (i % 2) * 104;
        float yy = y + 54 + (i / 2) * 30;
        if (game.role[p] == ROLE_COLOR)
            draw_round_rect(x + 2, yy + 2, 20, 20, 5, creature_colors[i]);
        else
            draw_animal(i, x, yy, 24, RGB(235, 240, 255), RGB(6, 24, 64));
        snprintf(buf, sizeof(buf), "%d", d->group_points[i]);
        draw_text(x + 32, yy + 5, 2, d->group_points[i] ? RGB(255, 255, 255) : RGB(110, 130, 170), buf);
        n++;
    }
    (void)n;
}

static void draw_result(void)
{
    char buf[48];
    int w = game_winner(&game);
    int s0 = result_detail[0].total, s1 = result_detail[1].total;
    draw_board(0);
    draw_result_card(0, 54);
    draw_result_card(1, 212);
    if (num_humans == 1)
        snprintf(buf, sizeof(buf), w == 0 ? "VOCE VENCEU!" : "A CPU VENCEU!");
    else
        snprintf(buf, sizeof(buf), "%s VENCEU!", player_name(w));
    draw_text_shadow(BOARD_X - 6, 14, 3, RGB(255, 230, 120), buf);
    if (s0 == s1)
        draw_text_shadow(PANEL_X - 10, 22, 1, RGB(220, 235, 255), "EMPATE: VENCE QUEM JOGOU EM 2O");
    snprintf(buf, sizeof(buf), "CARDUMES DE %s", role_name(result_view));
    draw_text_shadow(PANEL_X + 4, 372, 2, RGB(200, 225, 255), buf);
    draw_text_shadow(PANEL_X + 4, 390, 1, RGB(170, 200, 235), "< > ALTERNA O DESTAQUE");
    draw_help("x REVANCHE (PAPEIS TROCADOS)   o MENU", RGB(220, 235, 255));
}

void ui_draw(void)
{
    draw_background();
    switch (state) {
    case ST_TITLE:  draw_title(); break;
    case ST_SETUP:  draw_setup(); break;
    case ST_RULES:  draw_rules(); break;
    case ST_PLAY:   draw_play(); break;
    case ST_PAUSE:  draw_play(); draw_pause(); break;
    case ST_RESULT: draw_result(); break;
    }
}
