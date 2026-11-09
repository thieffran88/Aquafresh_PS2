/*
 * Aquafresh PS2 - telas, menus e fluxo de partida.
 */
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "game.h"
#include "draw.h"
#include "audio.h"

enum { ST_TITLE, ST_SETUP, ST_RULES, ST_PLAY, ST_PAUSE, ST_RESULT };
enum { PH_MOVE, PH_MOVE_DEST, PH_PICK, PH_DROP, PH_CPU, PH_END };

/* ---------------------------------------------------------------- layout */
#define BOARD_X   30
#define BOARD_Y   66
#define CELL_W    62      /* as pecas da arte sao um pouco mais largas que altas */
#define CELL_H    54
#define TILE_IN   1
#define PANEL_X   418
#define PANEL_W   206
#define CARD_H    70
#define CARD1_Y   58
#define CARD2_Y   134
#define POOL_Y    236
#define POOL_W    60
#define POOL_H    52
#define POOL_GAP  6
#define HELP_Y    421

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
    *x = (float)(BOARD_X + (cell % BOARD_N) * CELL_W);
    *y = (float)(BOARD_Y + (cell / BOARD_N) * CELL_H);
}

static void pool_pos(int i, float *x, float *y)
{
    float total = 3 * POOL_W + 2 * POOL_GAP;
    *x = PANEL_X + (PANEL_W - total) / 2 + (i % 3) * (POOL_W + POOL_GAP);
    *y = (float)(POOL_Y + (i / 3) * (POOL_H + POOL_GAP));
}

/* ------------------------------------------------------------------- som */

static int sfx_this_frame;
static int music_wanted = MUSIC_NONE;

static void sfx(int id, int pan)
{
    snd_play_pan(id, pan);
    sfx_this_frame = 1;
}

/* pan estereo pela posicao de uma casa do recife */
static int cell_pan(int cell)
{
    float x = (float)(BOARD_X + (cell % BOARD_N) * CELL_W + CELL_W / 2);
    return (int)((x - SCREEN_W / 2) * 100 / (SCREEN_W / 2));
}

static void set_msg(const char *m)
{
    sfx(SFX_ERROR, 0);
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
    sfx(SFX_START, 0);
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
        sfx(SFX_TURN, 0);
    }
}

static void enter_results(void)
{
    game_score_player(&game, 0, &result_detail[0]);
    game_score_player(&game, 1, &result_detail[1]);
    result_view = game.role[0];
    state = ST_RESULT;
    /* contra a CPU, perder toca a vinheta triste */
    sfx(num_humans == 1 && game_winner(&game) != 0 ? SFX_LOSE : SFX_WIN, 0);
}

static void finish_place(int pool_idx, int cell)
{
    float px, py;
    int p = game.turn;
    int before = game_score_player(&game, p, 0);
    pool_pos(pool_idx, &px, &py);
    if (game_place(&game, pool_idx, cell) == 0) {
        sfx(SFX_PLACE, cell_pan(cell));
        if (game_score_player(&game, p, 0) > before)
            sfx(SFX_SCHOOL, cell_pan(cell));    /* formou ou aumentou um cardume */
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
    sfx(SFX_MOVE, cell_pan(to));
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

#define PAUSE_ITEMS 5

static void update_pause(void)
{
    if (pressed & BTN_UP)   pause_sel = (pause_sel + PAUSE_ITEMS - 1) % PAUSE_ITEMS;
    if (pressed & BTN_DOWN) pause_sel = (pause_sel + 1) % PAUSE_ITEMS;
    if ((pressed & (BTN_LEFT | BTN_RIGHT | BTN_CROSS)) && pause_sel >= 2 && pause_sel <= 3) {
        if (pause_sel == 2)
            audio_set_music_enabled(!audio_music_enabled());
        else
            audio_set_sfx_enabled(!audio_sfx_enabled());
        sfx(SFX_SELECT, 0);
        return;
    }
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
        if (pause_sel == 4) state = ST_TITLE;
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

    {
        /* foto do estado para descobrir que som tocar */
        int before[12], after_[12], moved_ui = 0, changed, i;
        int st0 = state, ph0 = phase;
        int *watch[12] = { &cur_r, &cur_c, &pool_sel, &menu_sel, &setup_sel, &pause_sel,
                           &rules_page, &opt_role, &opt_level, &opt_first, &result_view, &state };
        for (i = 0; i < 12; i++)
            before[i] = *watch[i];
        sfx_this_frame = 0;

        if (pressed & BTN_SELECT) {          /* SELECT liga/desliga a musica */
            audio_set_music_enabled(!audio_music_enabled());
            pressed &= ~BTN_SELECT;
        }

        switch (state) {
        case ST_TITLE:  update_title(); break;
        case ST_SETUP:  update_setup(); break;
        case ST_RULES:  update_rules(); break;
        case ST_PLAY:   update_play(); break;
        case ST_PAUSE:  update_pause(); break;
        case ST_RESULT: update_result(); break;
        }

        for (i = 0; i < 11; i++) {
            after_[i] = *watch[i];
            moved_ui |= after_[i] != before[i];
        }
        changed = state != st0 || phase != ph0;
        if (!sfx_this_frame) {
            if (changed && (pressed & BTN_CIRCLE))
                sfx(SFX_BACK, 0);
            else if (changed && (pressed & (BTN_CROSS | BTN_TRIANGLE | BTN_START)))
                sfx(SFX_SELECT, 0);
            else if (moved_ui)
                sfx(SFX_CURSOR, 0);
        }
    }

    /* musica de cada tela */
    {
        int want = MUS_GAME;
        if (state == ST_TITLE || state == ST_SETUP || (state == ST_RULES && rules_return == ST_TITLE))
            want = MUS_MENU;
        if (want != music_wanted) {
            music_wanted = want;
            music_play(want);
        }
    }
}

/* ---------------------------------------------------------------- desenho */

#define WHITE RGB(255, 255, 255)
#define CW (CELL_W - 2 * TILE_IN)
#define CH (CELL_H - 2 * TILE_IN)

static void draw_bubbles(int count)
{
    int i;
    for (i = 0; i < count; i++) {
        unsigned int h = (unsigned)(i + 1) * 2654435761u;
        float speed = 0.35f + (h % 7) * 0.09f;
        float x = (float)(h % SCREEN_W) + (float)pulse(120 + i * 13, -8, 8);
        float y = SCREEN_H + 30 - (float)((unsigned)((frame_no + h % 997) * speed) % (SCREEN_H + 60));
        float r = 6.0f + (h >> 8) % 12;
        draw_sprite(SPR_BUBBLE, x, y, r, r, RGBA(255, 255, 255, 190));
    }
}

static void draw_background(int title)
{
    gfx_sprite(title ? ATLAS_BG_TITLE : ATLAS_BG_PLAY, 0, 0, SCREEN_W, SCREEN_H,
               0, 0, SCREEN_W, SCREEN_H, WHITE);
    if (!title)
        gfx_rect(0, 0, SCREEN_W, SCREEN_H, RGBA(0, 20, 60, 70));   /* realca as pecas */
    draw_bubbles(title ? 8 : 12);
}

/* corais e pedras atras do recife */
static void draw_reef_decor(void)
{
    float sway = (float)pulse(240, -2, 2);
    draw_sprite_h(SPR_WEED, -4 + sway, 300, 96, WHITE);
    draw_sprite_h(SPR_CORAL_PINK, -16, 352, 80, WHITE);
    draw_sprite_h(SPR_CORAL_ORANGE, 352, 340, 76, WHITE);
    draw_sprite_h(SPR_CORAL_PURPLE, 376 - sway, 320, 72, WHITE);
    draw_sprite_h(SPR_ROCKS, 300, 382, 46, WHITE);
}

static void draw_avatar(int p, float x, float y, float box_w, float box_h)
{
    int spr = p == 0 ? SPR_CRAB : SPR_PUFFER;
    float h = box_h, w = sprite_width_for(spr, h);
    float bob = (game.turn == p && state == ST_PLAY) ? (float)pulse(50, 0, 4) : 0.0f;
    if (w > box_w) {
        h = h * box_w / w;
        w = box_w;
    }
    draw_sprite(spr, x + (box_w - w) / 2, y + (box_h - h) / 2 - bob, w, h, WHITE);
}

static void draw_mini_role(int role, float x, float y, float s)
{
    int i;
    for (i = 0; i < 6; i++) {
        float xx = x + i * (s + 3);
        if (role == ROLE_COLOR) {
            draw_round_rect(xx - 1, y - 1, s + 2, s + 2, s * 0.3f, RGBA(0, 10, 30, 220));
            draw_round_rect(xx, y, s, s, s * 0.25f, creature_colors[i]);
        } else {
            draw_animal(i, xx - 1, y - 1, s + 2, RGB(235, 240, 255), RGB(18, 38, 92));
        }
    }
}

static void card_box(float y, float h, int active)
{
    if (active)
        draw_round_rect(PANEL_X - 3, y - 3, PANEL_W + 6, h + 6, 13,
                        RGBA(255, 220, 60, pulse(60, 90, 200)));
    draw_round_rect(PANEL_X, y, PANEL_W, h, 11, RGBA(10, 20, 50, 230));
    draw_round_rect(PANEL_X + 2, y + 2, PANEL_W - 4, h - 4, 10,
                    active ? RGBA(20, 80, 160, 235) : RGBA(14, 48, 104, 225));
}

static void draw_player_card(int p, float y)
{
    char buf[16];
    int active = (state == ST_PLAY || state == ST_PAUSE) && game.turn == p && phase != PH_END;
    int score = game_score_player(&game, p, 0);

    card_box(y, CARD_H, active);
    draw_avatar(p, PANEL_X + 3, y + 10, 42, 40);
    draw_text(PANEL_X + 48, y + 7, 2, active ? RGB(255, 230, 90) : WHITE, player_name(p));
    draw_text(PANEL_X + 48, y + 29, 2, RGB(150, 220, 255), role_name(game.role[p]));
    draw_mini_role(game.role[p], PANEL_X + 48, y + 52, 10);
    snprintf(buf, sizeof(buf), "%d", score);
    draw_text(PANEL_X + PANEL_W - 8 - text_width(buf, 4), y + 24, 4, WHITE, buf);
}

static void board_frame(void)
{
    int i;
    float bw = BOARD_N * CELL_W, bh = BOARD_N * CELL_H;
    draw_round_rect(BOARD_X - 10, BOARD_Y - 10, bw + 20, bh + 20, 16, RGBA(10, 20, 50, 220));
    draw_round_rect(BOARD_X - 7, BOARD_Y - 7, bw + 14, bh + 14, 13, RGBA(90, 190, 240, 230));
    draw_round_rect(BOARD_X - 4, BOARD_Y - 4, bw + 8, bh + 8, 11, RGBA(20, 70, 140, 235));
    for (i = 0; i < BOARD_CELLS; i++) {
        float x, y;
        cell_pos(i, &x, &y);
        draw_sprite(SPR_SLOT_LIGHT, x + TILE_IN, y + TILE_IN, CW, CH, RGBA(255, 255, 255, 120));
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

static void cell_frame(int cell, float grow, float t, Color c)
{
    float x, y;
    cell_pos(cell, &x, &y);
    draw_frame(x - grow, y - grow, CELL_W + 2 * grow, CELL_H + 2 * grow, t, c);
}

static void draw_board(int interactive)
{
    int i, cell = cur_r * BOARD_N + cur_c;
    unsigned char in_school[BOARD_CELLS];
    int dim = state == ST_RESULT;

    draw_reef_decor();
    board_frame();
    if (dim)
        schools_mask(in_school, result_view);

    /* casas de destino possiveis */
    if (interactive && phase == PH_MOVE_DEST) {
        for (i = 0; i < ntargets; i++) {
            float x, y;
            cell_pos(targets[i], &x, &y);
            draw_round_rect(x + 2, y + 2, CELL_W - 4, CELL_H - 4, 8,
                            RGBA(120, 255, 160, pulse(50, 70, 150)));
            draw_sprite(SPR_BUBBLE, x + CELL_W / 2 - 9, y + CELL_H / 2 - 9, 18, 18, WHITE);
        }
    }

    if (mark_from >= 0 && state != ST_RESULT) {
        float x, y;
        cell_pos(mark_from, &x, &y);
        draw_frame(x + 6, y + 6, CELL_W - 12, CELL_H - 12, 2, RGBA(255, 220, 120, 170));
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
        if (dim && !in_school[i])
            draw_tile_mod(game.board[i], x + TILE_IN, y + TILE_IN, CW, CH, RGB(90, 100, 120));
        else
            draw_tile(game.board[i], x + TILE_IN, y + TILE_IN, CW, CH);
    }

    if (state != ST_RESULT && !anim.active) {
        if (mark_to >= 0)
            cell_frame(mark_to, -1, 3, RGBA(255, 220, 80, 230));
        if (mark_cell >= 0)
            cell_frame(mark_cell, -1, 3, RGBA(255, 255, 255, 210));
    }

    if (!interactive)
        return;

    if (sel_cell >= 0)
        cell_frame(sel_cell, 2, 4, RGB(255, 150, 30));

    if (phase == PH_MOVE || phase == PH_MOVE_DEST || phase == PH_DROP) {
        float x, y;
        cell_pos(cell, &x, &y);
        if (phase == PH_DROP && game.board[cell] == EMPTY && game.pool_n > 0)
            draw_tile_mod(game.pool[pool_sel], x + TILE_IN, y + TILE_IN - 3, CW, CH,
                          RGBA(255, 255, 255, pulse(40, 170, 240)));
        cell_frame(cell, 4, 2, RGB(10, 20, 40));
        cell_frame(cell, 3, 4, RGBA(255, 255, 255, pulse(40, 170, 255)));
    }
}

static void draw_pool(int interactive)
{
    int i;
    char buf[24];
    draw_text(PANEL_X + 4, POOL_Y - 24, 2, WHITE, "VIVEIRO");
    for (i = 0; i < POOL_MAX; i++) {
        float x, y;
        pool_pos(i, &x, &y);
        draw_sprite(SPR_SLOT_LIGHT, x, y, POOL_W, POOL_H, RGBA(255, 255, 255, 90));
        if (i < game.pool_n) {
            int sel = interactive && i == pool_sel &&
                      (phase == PH_PICK || phase == PH_DROP || (phase == PH_CPU && cpu_step >= 2));
            float lift = sel ? (phase == PH_DROP ? -5.0f : (float)-pulse(30, 0, 4)) : 0.0f;
            if (sel)
                draw_round_rect(x - 4, y - 4 + lift, POOL_W + 8, POOL_H + 8, 11,
                                phase == PH_CPU ? RGB(255, 110, 110) : RGB(255, 220, 60));
            draw_tile(game.pool[i], x, y + lift, POOL_W, POOL_H);
        }
    }
    /* estoque */
    {
        float y = POOL_Y + 2 * POOL_H + POOL_GAP + 12;
        int n = game.stock_n < 3 ? game.stock_n : 3;
        for (i = 0; i < n; i++)
            draw_tile_back(PANEL_X + 6 + i * 4, y - i * 3, 40, 35);
        snprintf(buf, sizeof(buf), "ESTOQUE %d", game.stock_n);
        draw_text(PANEL_X + 62, y + 9, 2, WHITE, buf);
    }
}

static void draw_help(const char *text, Color c)
{
    gfx_rect(0, HELP_Y - 7, SCREEN_W, 34, RGBA(0, 10, 30, 190));
    gfx_rect(0, HELP_Y - 7, SCREEN_W, 2, RGBA(90, 190, 240, 200));
    draw_text_center(SCREEN_W / 2, HELP_Y, 2, c, text);
}

static void draw_banner(const char *text, int alpha)
{
    int w = text_width(text, 4) + 100;
    float x = BOARD_X + BOARD_N * CELL_W / 2 - w / 2;
    float y = BOARD_Y + BOARD_N * CELL_H / 2 - 32;
    draw_round_rect(x, y, (float)w, 64, 18, RGBA(10, 20, 50, alpha * 230 / 255));
    draw_round_rect(x + 3, y + 3, (float)w - 6, 58, 16, RGBA(30, 110, 190, alpha * 230 / 255));
    draw_sprite_h(SPR_STARFISH, x - 20, y - 14, 76, RGBA(255, 255, 255, alpha));
    draw_text(x + 76, y + 15, 4, RGBA(255, 255, 255, alpha), text);
}

static void draw_phase_info(void)
{
    const char *l = "";
    switch (phase) {
    case PH_MOVE:      l = "1 MOVER"; break;
    case PH_MOVE_DEST: l = "1 MOVER"; break;
    case PH_PICK:
    case PH_DROP:      l = "2 COLOCAR"; break;
    case PH_CPU:       l = cpu_step == 0 ? "CPU PENSANDO..." : "CPU JOGANDO"; break;
    default:           l = "RECIFE CHEIO!"; break;
    }
    draw_text(PANEL_X + PANEL_W - text_width(l, 2), 26, 2, WHITE, l);
}

static void draw_title_bar(void)
{
    draw_text(BOARD_X - 8, 12, 4, WHITE, "AQUAFRESH PS2");
}

static void draw_play(void)
{
    char buf[48];
    int interactive = !is_cpu[game.turn];
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
                  WHITE);
    } else {
        const char *h = "";
        switch (phase) {
        case PH_MOVE:      h = "x MOVER CRIATURA   ^ PULAR   START PAUSA"; break;
        case PH_MOVE_DEST: h = "x ESCOLHER DESTINO   o CANCELAR"; break;
        case PH_PICK:      h = game_first_turn(&game) ? "x ESCOLHER CRIATURA" : "x ESCOLHER CRIATURA   o VOLTAR"; break;
        case PH_DROP:      h = "x COLOCAR   L1/R1 TROCAR   o VOLTAR"; break;
        }
        draw_help(h, WHITE);
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
        int w = text_width(s, scale) + 44;
        draw_round_rect(cx - w / 2 - 2, y - 7, (float)w + 4, 8 * scale + 14, 14, RGBA(10, 20, 50, 230));
        draw_round_rect(cx - w / 2, y - 5, (float)w, 8 * scale + 10, 12,
                        RGBA(255, 210, 50, pulse(60, 170, 250)));
    }
    draw_text_center(cx, y, scale, sel ? WHITE : RGB(170, 180, 200), s);
}

static void draw_title(void)
{
    static const char *items[MENU_ITEMS] = { "1 JOGADOR", "2 JOGADORES", "COMO JOGAR" };
    int i;
    draw_round_rect(SCREEN_W / 2 - 150, 290, 300, 128, 18, RGBA(10, 20, 50, 200));
    draw_round_rect(SCREEN_W / 2 - 146, 294, 292, 120, 15, RGBA(20, 80, 160, 190));
    for (i = 0; i < MENU_ITEMS; i++)
        draw_menu_item(SCREEN_W / 2, 304 + i * 38, 3, items[i], i == menu_sel);
    draw_text_center(SCREEN_W / 2, 428, 2, WHITE, "x CONFIRMAR");
}

static void dim_screen(int a)
{
    gfx_rect(0, 0, SCREEN_W, SCREEN_H, RGBA(0, 15, 45, a));
}

static void big_panel(float x, float y, float w, float h)
{
    draw_round_rect(x, y, w, h, 18, RGBA(10, 20, 50, 230));
    draw_round_rect(x + 3, y + 3, w - 6, h - 6, 16, RGBA(90, 190, 240, 220));
    draw_round_rect(x + 6, y + 6, w - 12, h - 12, 14, RGBA(10, 34, 80, 240));
}

static void draw_option_row(float y, const char *label, const char *value, int sel, int arrows)
{
    if (sel)
        draw_round_rect(84, y - 7, 472, 32, 12, RGBA(255, 210, 50, pulse(60, 90, 160)));
    draw_text(100, y, 2, sel ? WHITE : RGB(180, 190, 210), label);
    if (value) {
        char buf[40];
        if (arrows)
            snprintf(buf, sizeof(buf), "< %s >", value);
        else
            snprintf(buf, sizeof(buf), "%s", value);
        draw_text(540 - text_width(buf, 2), y, 2, WHITE, buf);
    }
}

static void draw_setup(void)
{
    static const char *levels[3] = { "FACIL", "NORMAL", "DIFICIL" };
    char buf[64];
    int row = 0;
    float y = 128;
    dim_screen(60);
    draw_text_center(SCREEN_W / 2, 26, 5, WHITE, num_humans == 1 ? "1 JOGADOR" : "2 JOGADORES");
    big_panel(70, 92, 500, 296);

    if (num_humans == 1) {
        draw_option_row(y, "SEUS CARDUMES", role_name(opt_role), setup_sel == row++, 1);
        y += 44;
        draw_option_row(y, "DIFICULDADE", levels[opt_level], setup_sel == row++, 1);
        y += 44;
        draw_option_row(y, "QUEM COMECA", opt_first == 0 ? "VOCE" : "CPU", setup_sel == row++, 1);
        y += 44;
    } else {
        draw_option_row(y, "JOGADOR 1 FORMA", role_name(opt_role), setup_sel == row++, 1);
        y += 44;
        draw_option_row(y, "QUEM COMECA", opt_first == 0 ? "JOGADOR 1" : "JOGADOR 2",
                        setup_sel == row++, 1);
        y += 44;
    }
    draw_option_row(y, "COMECAR PARTIDA!", 0, setup_sel == row, 0);

    /* quem forma o que */
    y = 300;
    draw_avatar(0, 96, y - 4, 60, 52);
    snprintf(buf, sizeof(buf), "%s %s", player_name(0), role_name(opt_role));
    draw_text(162, y, 2, WHITE, buf);
    draw_mini_role(opt_role, 162, y + 24, 14);
    draw_avatar(1, 336, y - 4, 60, 52);
    snprintf(buf, sizeof(buf), "%s %s", player_name(1), role_name(opt_role ^ 1));
    draw_text(402, y, 2, WHITE, buf);
    draw_mini_role(opt_role ^ 1, 402, y + 24, 14);
    draw_help("DIRECIONAL ALTERA   x CONFIRMAR   o VOLTAR", WHITE);
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
          "QUANDO AS 36 CRIATURAS ENCHEREM O",
          "RECIFE, QUEM TIVER MAIS PONTOS VENCE!" },
        { "1 MOVER (OPCIONAL): DESLIZE UMA CRIATURA",
          "  NA LINHA OU COLUNA, QUANTAS CASAS",
          "  QUISER, SEM PULAR OUTRAS.",
          "  (NAO EXISTE NO PRIMEIRO TURNO)",
          "2 COLOCAR: PEGUE UMA CRIATURA DO",
          "  VIVEIRO E PONHA NUMA CASA VAZIA.",
          "3 REPOR: UMA CRIATURA DO ESTOQUE VAI",
          "  PARA O VIVEIRO (AUTOMATICO).",
          "QUALQUER UM PODE MOVER QUALQUER CRIATURA." },
        { "CRIATURAS NO CARDUME  2  3  4  5   6",
          "PONTOS                1  3  6  10  15",
          "",
          "UMA CRIATURA SOZINHA NAO VALE NADA.",
          "UMA CRIATURA PODE CONTAR NUM CARDUME",
          "DE COR E NUM DE ANIMAL AO MESMO TEMPO.",
          "EMPATE: VENCE QUEM JOGOU EM SEGUNDO.",
          "REVANCHE: OS PAPEIS SAO TROCADOS.",
          "" },
        { "DIRECIONAL   MOVE O CURSOR",
          "x            CONFIRMA",
          "o            VOLTA OU CANCELA",
          "^            PULA O MOVIMENTO",
          "L1 R1        TROCA A CRIATURA",
          "START        PAUSA",
          "SELECT       LIGA/DESLIGA A MUSICA",
          "NA TELA FINAL O DIRECIONAL MOSTRA",
          "OS CARDUMES DE CADA JOGADOR." },
    };
    char buf[32];
    int i;
    dim_screen(60);
    big_panel(18, 14, 604, 394);
    draw_text(44, 30, 4, WHITE, titles[rules_page]);
    snprintf(buf, sizeof(buf), "%d/%d", rules_page + 1, RULES_PAGES);
    draw_text(596 - text_width(buf, 2), 40, 2, WHITE, buf);
    for (i = 0; i < 9; i++) {
        const char *ln = pages[rules_page][i];
        /* coluna de botoes alinhada na pagina de controles */
        if (rules_page == 3 && ln[0] && strlen(ln) > 13 && ln[12] == ' ') {
            char key[16];
            memcpy(key, ln, 12);
            key[12] = 0;
            draw_text(44, 80 + i * 28, 2, WHITE, key);
            draw_text(230, 80 + i * 28, 2, WHITE, ln + 13);
        } else {
            draw_text(44, 80 + i * 28, 2, WHITE, ln);
        }
    }
    if (rules_page == 0) {
        for (i = 0; i < 6; i++)
            draw_tile(MAKE_TILE(i, i), 110 + i * 66, 340, 60, 52);
    } else if (rules_page == 2) {
        draw_tile(MAKE_TILE(COL_BLUE, ANI_CRAB), 80, 334, 60, 52);
        draw_tile(MAKE_TILE(COL_BLUE, ANI_STAR), 140, 334, 60, 52);
        draw_tile(MAKE_TILE(COL_BLUE, ANI_FISH), 200, 334, 60, 52);
        draw_text(276, 350, 2, WHITE, "= 3 PONTOS PARA CORES");
    }
    draw_sprite_h(SPR_STARFISH, 548, 330, 80, WHITE);
    draw_help("< > PAGINA   x PROXIMA   o SAIR", WHITE);
}

static void draw_pause(void)
{
    const char *items[PAUSE_ITEMS] = {
        "CONTINUAR", "COMO JOGAR",
        audio_music_enabled() ? "MUSICA: SIM" : "MUSICA: NAO",
        audio_sfx_enabled() ? "EFEITOS: SIM" : "EFEITOS: NAO",
        "SAIR PARA O MENU" };
    int i;
    dim_screen(150);
    big_panel(140, 64, 360, 330);
    draw_text_center(SCREEN_W / 2, 84, 5, WHITE, "PAUSA");
    draw_sprite_h(SPR_TURTLE, 440, 24, 84, WHITE);
    for (i = 0; i < PAUSE_ITEMS; i++)
        draw_menu_item(SCREEN_W / 2, 158 + i * 44, 3, items[i], i == pause_sel);
}

static void draw_result_card(int p, float y)
{
    char buf[40];
    int i;
    const ScoreDetail *d = &result_detail[p];
    int view = game.role[p] == result_view;
    card_box(y, 156, view);
    draw_avatar(p, PANEL_X + 3, y + 6, 42, 40);
    draw_text(PANEL_X + 48, y + 7, 2, WHITE, player_name(p));
    draw_text(PANEL_X + 48, y + 28, 2, RGB(150, 220, 255), role_name(game.role[p]));
    snprintf(buf, sizeof(buf), "%d", d->total);
    draw_text(PANEL_X + PANEL_W - 8 - text_width(buf, 4), y + 10, 4, WHITE, buf);
    for (i = 0; i < 6; i++) {
        float x = PANEL_X + 12 + (i % 2) * 98;
        float yy = y + 56 + (i / 2) * 32;
        if (game.role[p] == ROLE_COLOR)
            draw_round_rect(x + 2, yy + 2, 20, 20, 5, creature_colors[i]);
        else
            draw_animal(i, x, yy, 24, RGB(235, 240, 255), RGB(6, 24, 64));
        snprintf(buf, sizeof(buf), "%d", d->group_points[i]);
        draw_text(x + 32, yy + 4, 2, d->group_points[i] ? WHITE : RGB(110, 120, 140), buf);
    }
}

static void draw_result(void)
{
    char buf[48];
    int w = game_winner(&game);
    int s0 = result_detail[0].total, s1 = result_detail[1].total;
    float tw;
    draw_board(0);
    draw_result_card(0, 58);
    draw_result_card(1, 222);
    if (num_humans == 1)
        snprintf(buf, sizeof(buf), w == 0 ? "VOCE VENCEU!" : "A CPU VENCEU!");
    else
        snprintf(buf, sizeof(buf), "%s VENCEU!", player_name(w));
    tw = (float)text_width(buf, 4);
    draw_text(BOARD_X - 8, 12, 4, WHITE, buf);
    draw_sprite_h(w == 0 ? SPR_CRAB : SPR_PUFFER, BOARD_X + tw + 4, 4 - (float)pulse(40, 0, 8), 48, WHITE);
    if (s0 == s1)
        draw_text_shadow(PANEL_X - 30, 44, 1, WHITE, "EMPATE: VENCE QUEM JOGOU EM SEGUNDO");
    snprintf(buf, sizeof(buf), "CARDUMES DE %s", role_name(result_view));
    draw_text(PANEL_X + 4, 386, 2, WHITE, buf);
    draw_help("x REVANCHE (PAPEIS TROCADOS)   o MENU", WHITE);
}

void ui_draw(void)
{
    draw_background(state == ST_TITLE);
    switch (state) {
    case ST_TITLE:  draw_title(); break;
    case ST_SETUP:  draw_setup(); break;
    case ST_RULES:  draw_rules(); break;
    case ST_PLAY:   draw_play(); break;
    case ST_PAUSE:  draw_play(); draw_pause(); break;
    case ST_RESULT: draw_result(); break;
    }
}
