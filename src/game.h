/*
 * Aquafresh PS2 - regras do jogo (independente de plataforma).
 *
 * Tabuleiro 6x6 (o "recife"), 36 criaturas = 6 cores x 6 animais.
 * Um jogador pontua cardumes da MESMA COR, o outro cardumes do MESMO ANIMAL.
 * Turno: (1) mover opcionalmente uma criatura em linha reta, (2) colocar uma
 * criatura do viveiro (pool) no recife, (3) repor o viveiro com o estoque.
 */
#ifndef AQUAFRESH_GAME_H
#define AQUAFRESH_GAME_H

#define BOARD_N     6
#define BOARD_CELLS (BOARD_N * BOARD_N)
#define NUM_TILES   36
#define POOL_MAX    6
#define EMPTY       (-1)

enum { COL_BLUE, COL_GREEN, COL_PINK, COL_PURPLE, COL_RED, COL_YELLOW, NUM_COLORS };
enum { ANI_CRAB, ANI_FISH, ANI_JELLY, ANI_SEAHORSE, ANI_STAR, ANI_TURTLE, NUM_ANIMALS };

enum { ROLE_COLOR = 0, ROLE_ANIMAL = 1 };

enum { AI_EASY = 0, AI_NORMAL = 1, AI_HARD = 2 };

#define TILE_COLOR(t)  ((t) / NUM_ANIMALS)
#define TILE_ANIMAL(t) ((t) % NUM_ANIMALS)
#define MAKE_TILE(c, a) ((c) * NUM_ANIMALS + (a))

typedef struct {
    signed char board[BOARD_CELLS];
    signed char pool[POOL_MAX];
    int pool_n;
    signed char stock[NUM_TILES];
    int stock_n;
    int on_board;
    int turn;        /* jogador da vez (0 ou 1) */
    int first;       /* quem comecou a partida */
    int role[2];     /* ROLE_COLOR ou ROLE_ANIMAL para cada jogador */
    unsigned int rng;
} Game;

/* Um turno completo: movimento opcional + colocacao. */
typedef struct {
    int move_from;   /* -1 = sem movimento */
    int move_to;
    int pool_idx;
    int cell;
} Turn;

/* Detalhe da pontuacao para a tela final. */
typedef struct {
    int total;
    int group_points[6];  /* pontos por cor ou por animal */
    int group_count;      /* quantos cardumes (>=2) existem */
} ScoreDetail;

void game_new(Game *g, unsigned int seed, int first_player, int role_of_player0);
unsigned int game_rand(Game *g);

int  game_is_over(const Game *g);
int  game_first_turn(const Game *g);   /* 1 se o recife ainda esta vazio */

/* Movimento: desliza em linha/coluna sem pular criaturas. */
int  game_can_move(const Game *g, int from, int to);
int  game_move_targets(const Game *g, int from, int *out); /* retorna quantidade */
void game_do_move(Game *g, int from, int to);

/* Coloca pool[pool_idx] em cell, repoe o viveiro e passa a vez. Retorna 0 se ok. */
int  game_place(Game *g, int pool_idx, int cell);

/* Aplica um turno completo (sem validacao extra alem das regras). */
int  game_apply_turn(Game *g, const Turn *t);

int  points_for_school(int size);
int  score_board(const signed char *board, int role, ScoreDetail *detail);
int  game_score_player(const Game *g, int player, ScoreDetail *detail);
/* Vencedor: 0 ou 1. Em empate vence quem jogou em segundo. */
int  game_winner(const Game *g);

/* IA: escolhe um turno para g->turn. */
void ai_choose(const Game *g, int level, Turn *out);

#endif
