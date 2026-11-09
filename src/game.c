/*
 * Aquafresh PS2 - implementacao das regras e da IA.
 */
#include <string.h>
#include "game.h"

unsigned int game_rand(Game *g)
{
    /* xorshift32 */
    unsigned int x = g->rng ? g->rng : 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static void refill_pool(Game *g)
{
    while (g->pool_n < POOL_MAX && g->stock_n > 0)
        g->pool[g->pool_n++] = g->stock[--g->stock_n];
}

void game_new(Game *g, unsigned int seed, int first_player, int role_of_player0)
{
    int i;
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 12345u;
    for (i = 0; i < BOARD_CELLS; i++)
        g->board[i] = EMPTY;

    /* Estoque embaralhado (Fisher-Yates). */
    for (i = 0; i < NUM_TILES; i++)
        g->stock[i] = (signed char)i;
    for (i = NUM_TILES - 1; i > 0; i--) {
        int j = (int)(game_rand(g) % (unsigned)(i + 1));
        signed char t = g->stock[i];
        g->stock[i] = g->stock[j];
        g->stock[j] = t;
    }
    g->stock_n = NUM_TILES;
    g->pool_n = 0;
    refill_pool(g);   /* seis criaturas viradas para cima */

    g->first = first_player & 1;
    g->turn = g->first;
    g->role[0] = role_of_player0 ? ROLE_ANIMAL : ROLE_COLOR;
    g->role[1] = g->role[0] == ROLE_COLOR ? ROLE_ANIMAL : ROLE_COLOR;
}

int game_is_over(const Game *g)
{
    return g->on_board >= BOARD_CELLS || (g->pool_n == 0 && g->stock_n == 0);
}

int game_first_turn(const Game *g)
{
    return g->on_board == 0;
}

/* ---------------------------------------------------------------- moves */

static int board_can_move(const signed char *b, int from, int to)
{
    int fr, fc, tr, tc, dr, dc, r, c;
    if (from < 0 || from >= BOARD_CELLS || to < 0 || to >= BOARD_CELLS)
        return 0;
    if (from == to || b[from] == EMPTY || b[to] != EMPTY)
        return 0;
    fr = from / BOARD_N; fc = from % BOARD_N;
    tr = to / BOARD_N;   tc = to % BOARD_N;
    if (fr != tr && fc != tc)
        return 0;
    dr = (tr > fr) - (tr < fr);
    dc = (tc > fc) - (tc < fc);
    r = fr + dr; c = fc + dc;
    for (;;) {
        if (b[r * BOARD_N + c] != EMPTY)
            return 0;          /* nao pode pular outra criatura */
        if (r == tr && c == tc)
            return 1;
        r += dr; c += dc;
    }
}

int game_can_move(const Game *g, int from, int to)
{
    return board_can_move(g->board, from, to);
}

static int board_move_targets(const signed char *b, int from, int *out)
{
    static const int DR[4] = { -1, 1, 0, 0 };
    static const int DC[4] = { 0, 0, -1, 1 };
    int n = 0, d;
    int fr = from / BOARD_N, fc = from % BOARD_N;
    if (b[from] == EMPTY)
        return 0;
    for (d = 0; d < 4; d++) {
        int r = fr + DR[d], c = fc + DC[d];
        while (r >= 0 && r < BOARD_N && c >= 0 && c < BOARD_N &&
               b[r * BOARD_N + c] == EMPTY) {
            out[n++] = r * BOARD_N + c;
            r += DR[d]; c += DC[d];
        }
    }
    return n;
}

int game_move_targets(const Game *g, int from, int *out)
{
    return board_move_targets(g->board, from, out);
}

void game_do_move(Game *g, int from, int to)
{
    if (!game_can_move(g, from, to))
        return;
    g->board[to] = g->board[from];
    g->board[from] = EMPTY;
}

int game_place(Game *g, int pool_idx, int cell)
{
    int i;
    if (pool_idx < 0 || pool_idx >= g->pool_n)
        return -1;
    if (cell < 0 || cell >= BOARD_CELLS || g->board[cell] != EMPTY)
        return -1;
    g->board[cell] = g->pool[pool_idx];
    g->on_board++;
    for (i = pool_idx; i < g->pool_n - 1; i++)
        g->pool[i] = g->pool[i + 1];
    g->pool_n--;
    refill_pool(g);      /* fase 3: repor o viveiro */
    g->turn ^= 1;
    return 0;
}

int game_apply_turn(Game *g, const Turn *t)
{
    if (t->move_from >= 0) {
        if (!game_can_move(g, t->move_from, t->move_to))
            return -1;
        game_do_move(g, t->move_from, t->move_to);
    }
    return game_place(g, t->pool_idx, t->cell);
}

/* -------------------------------------------------------------- scoring */

int points_for_school(int size)
{
    /* 2->1, 3->3, 4->6, 5->10, 6->15 */
    if (size < 2)
        return 0;
    return size * (size - 1) / 2;
}

int score_board(const signed char *b, int role, ScoreDetail *detail)
{
    unsigned char seen[BOARD_CELLS];
    int stack[BOARD_CELLS];
    int total = 0, i;

    if (detail)
        memset(detail, 0, sizeof(*detail));
    memset(seen, 0, sizeof(seen));

    for (i = 0; i < BOARD_CELLS; i++) {
        int key, sp = 0, size = 0;
        if (b[i] == EMPTY || seen[i])
            continue;
        key = role == ROLE_COLOR ? TILE_COLOR(b[i]) : TILE_ANIMAL(b[i]);
        seen[i] = 1;
        stack[sp++] = i;
        while (sp) {
            int p = stack[--sp];
            int r = p / BOARD_N, c = p % BOARD_N, k;
            int nb[4], nn = 0;
            size++;
            if (r > 0)           nb[nn++] = p - BOARD_N;
            if (r < BOARD_N - 1) nb[nn++] = p + BOARD_N;
            if (c > 0)           nb[nn++] = p - 1;
            if (c < BOARD_N - 1) nb[nn++] = p + 1;
            for (k = 0; k < nn; k++) {
                int q = nb[k], qk;
                if (b[q] == EMPTY || seen[q])
                    continue;
                qk = role == ROLE_COLOR ? TILE_COLOR(b[q]) : TILE_ANIMAL(b[q]);
                if (qk == key) {
                    seen[q] = 1;
                    stack[sp++] = q;
                }
            }
        }
        if (size >= 2) {
            int pts = points_for_school(size);
            total += pts;
            if (detail) {
                detail->group_points[key] += pts;
                detail->group_count++;
            }
        }
    }
    if (detail)
        detail->total = total;
    return total;
}

int game_score_player(const Game *g, int player, ScoreDetail *detail)
{
    return score_board(g->board, g->role[player & 1], detail);
}

int game_winner(const Game *g)
{
    int s0 = game_score_player(g, 0, 0);
    int s1 = game_score_player(g, 1, 0);
    if (s0 > s1) return 0;
    if (s1 > s0) return 1;
    return g->first ^ 1;      /* empate: vence quem jogou em segundo */
}

/* ------------------------------------------------------------------- IA */

/* Avaliacao do ponto de vista de 'role_me': diferenca de pontos. */
static int evaluate(const signed char *b, int role_me)
{
    int me = score_board(b, role_me, 0);
    int op = score_board(b, role_me ^ 1, 0);
    return (me - op) * 8;
}

typedef struct {
    Turn t;
    int value;
} Cand;

#define MAX_CANDS 12

static void insert_cand(Cand *list, int *n, int cap, const Turn *t, int v)
{
    int pos;
    if (*n == cap && v <= list[*n - 1].value)
        return;
    pos = *n < cap ? (*n)++ : cap - 1;
    while (pos > 0 && list[pos - 1].value < v) {
        list[pos] = list[pos - 1];
        pos--;
    }
    list[pos].t = *t;
    list[pos].value = v;
}

/* Enumera todos os turnos; guarda os 'cap' melhores e retorna o melhor valor. */
static int search_greedy(const signed char *board, const signed char *pool, int pool_n,
                         int role_me, Cand *best, int *nbest, int cap,
                         unsigned int *rng)
{
    signed char b[BOARD_CELLS];
    int targets[BOARD_CELLS];
    int from, ti, nt, p, c;
    int bestv = -1000000;
    Turn t;

    /* from = -1 representa "nao mover" */
    for (from = -1; from < BOARD_CELLS; from++) {
        if (from >= 0) {
            if (board[from] == EMPTY)
                continue;
            nt = board_move_targets(board, from, targets);
        } else {
            nt = 1;
        }
        for (ti = 0; ti < nt; ti++) {
            memcpy(b, board, sizeof(b));
            if (from >= 0) {
                b[targets[ti]] = b[from];
                b[from] = EMPTY;
            }
            for (p = 0; p < pool_n; p++) {
                for (c = 0; c < BOARD_CELLS; c++) {
                    int v;
                    if (b[c] != EMPTY)
                        continue;
                    b[c] = pool[p];
                    v = evaluate(b, role_me);
                    b[c] = EMPTY;
                    if (rng) {
                        /* desempate aleatorio para variar as partidas */
                        unsigned int x = *rng;
                        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
                        *rng = x;
                        v = v * 4 + (int)(x & 3);
                    }
                    /* mover sem ganhar nada so desperdica; prefira nao mover */
                    if (from >= 0 && rng)
                        v -= 1;
                    t.move_from = from;
                    t.move_to = from >= 0 ? targets[ti] : -1;
                    t.pool_idx = p;
                    t.cell = c;
                    if (best)
                        insert_cand(best, nbest, cap, &t, v);
                    if (v > bestv)
                        bestv = v;
                }
            }
        }
    }
    return bestv;
}

void ai_choose(const Game *g, int level, Turn *out)
{
    Cand cands[MAX_CANDS];
    int ncand = 0, i;
    unsigned int rng = g->rng ^ 0xA5A5A5A5u ^ (unsigned)(g->on_board * 7919);
    int role_me = g->role[g->turn];

    if (rng == 0) rng = 1;

    if (level == AI_EASY) {
        /* Metade das vezes joga uma criatura aleatoria num lugar aleatorio. */
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        if ((rng & 1) == 0) {
            int empties[BOARD_CELLS], ne = 0;
            for (i = 0; i < BOARD_CELLS; i++)
                if (g->board[i] == EMPTY)
                    empties[ne++] = i;
            out->move_from = -1;
            out->move_to = -1;
            out->pool_idx = (int)((rng >> 3) % (unsigned)g->pool_n);
            out->cell = empties[(rng >> 9) % (unsigned)ne];
            return;
        }
    }

    search_greedy(g->board, g->pool, g->pool_n, role_me, cands, &ncand,
                  level == AI_HARD ? MAX_CANDS : 1, &rng);

    if (level == AI_HARD && ncand > 1 && g->on_board < BOARD_CELLS - 1) {
        /* Olha um lance a frente: para cada candidato, supoe a melhor
         * resposta gulosa do adversario com o viveiro restante. */
        int best_i = 0, best_v = -1000000;
        for (i = 0; i < ncand; i++) {
            signed char b[BOARD_CELLS];
            signed char pool[POOL_MAX];
            int pn = 0, k, opp_v, v;
            const Turn *t = &cands[i].t;
            memcpy(b, g->board, sizeof(b));
            if (t->move_from >= 0) {
                b[t->move_to] = b[t->move_from];
                b[t->move_from] = EMPTY;
            }
            b[t->cell] = g->pool[t->pool_idx];
            for (k = 0; k < g->pool_n; k++)
                if (k != t->pool_idx)
                    pool[pn++] = g->pool[k];
            if (pn == 0) {
                v = evaluate(b, role_me) * 4;
            } else {
                opp_v = search_greedy(b, pool, pn, role_me ^ 1, 0, 0, 0, 0);
                v = -opp_v * 4;
            }
            v = v * 2 + cands[i].value / 64;   /* pequeno peso do valor imediato */
            if (v > best_v) {
                best_v = v;
                best_i = i;
            }
        }
        *out = cands[best_i].t;
        return;
    }

    if (ncand > 0) {
        *out = cands[0].t;
    } else {
        out->move_from = -1;
        out->move_to = -1;
        out->pool_idx = 0;
        for (i = 0; i < BOARD_CELLS; i++)
            if (g->board[i] == EMPTY)
                break;
        out->cell = i;
    }
}
