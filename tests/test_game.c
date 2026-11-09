/* Testes das regras (roda no PC: make test). */
#include <stdio.h>
#include <string.h>
#include "../src/game.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void clear(signed char *b) { memset(b, EMPTY, BOARD_CELLS); }

static void test_points(void)
{
    CHECK(points_for_school(1) == 0);
    CHECK(points_for_school(2) == 1);
    CHECK(points_for_school(3) == 3);
    CHECK(points_for_school(4) == 6);
    CHECK(points_for_school(5) == 10);
    CHECK(points_for_school(6) == 15);
}

static void test_scoring(void)
{
    signed char b[BOARD_CELLS];
    ScoreDetail d;
    clear(b);
    /* 4 azuis em L (animais diferentes) */
    b[0] = MAKE_TILE(COL_BLUE, ANI_CRAB);
    b[1] = MAKE_TILE(COL_BLUE, ANI_FISH);
    b[2] = MAKE_TILE(COL_BLUE, ANI_JELLY);
    b[8] = MAKE_TILE(COL_BLUE, ANI_STAR);
    /* diagonal nao conta: estrela vermelha em 15 (diagonal de 8) */
    b[15] = MAKE_TILE(COL_RED, ANI_STAR);
    CHECK(score_board(b, ROLE_COLOR, &d) == 6);
    CHECK(d.group_points[COL_BLUE] == 6);
    CHECK(score_board(b, ROLE_ANIMAL, 0) == 0);
    /* agora estrela vermelha ortogonal a azul (14 esta abaixo de 8) */
    b[14] = MAKE_TILE(COL_RED, ANI_STAR);
    /* 14 e 15 sao vermelhas e estrelas lado a lado */
    CHECK(score_board(b, ROLE_COLOR, 0) == 6 + 1);
    /* 8,14,15 estrelas ligadas = 3 */
    CHECK(score_board(b, ROLE_ANIMAL, 0) == 3);
}

static void test_moves(void)
{
    Game g;
    int t[BOARD_CELLS], n;
    game_new(&g, 42, 0, ROLE_COLOR);
    g.board[14] = 0;          /* linha 2, coluna 2 */
    g.board[17] = 1;          /* linha 2, coluna 5 */
    g.board[2] = 2;           /* linha 0, coluna 2 */
    CHECK(game_can_move(&g, 14, 15));
    CHECK(game_can_move(&g, 14, 16));
    CHECK(!game_can_move(&g, 14, 17));   /* ocupada */
    CHECK(!game_can_move(&g, 14, 21));   /* diagonal */
    CHECK(game_can_move(&g, 14, 8));
    CHECK(!game_can_move(&g, 14, 2));
    CHECK(!game_can_move(&g, 17, 13));   /* pularia a 14 */
    n = game_move_targets(&g, 14, t);
    /* cima: 8 ; baixo: 20,26,32 ; esquerda: 13,12 ; direita: 15,16 */
    CHECK(n == 8);
}

static void test_full_game(int level0, int level1, unsigned seed, int *score0, int *score1)
{
    Game g;
    int turns = 0;
    game_new(&g, seed, 0, ROLE_COLOR);
    CHECK(g.pool_n == 6 && g.stock_n == 30);
    while (!game_is_over(&g)) {
        Turn t;
        int before = g.on_board;
        ai_choose(&g, g.turn == 0 ? level0 : level1, &t);
        if (game_first_turn(&g))
            CHECK(t.move_from < 0);
        CHECK(game_apply_turn(&g, &t) == 0);
        CHECK(g.on_board == before + 1);
        turns++;
        if (turns > 40) break;
    }
    CHECK(turns == 36);
    CHECK(g.pool_n == 0 && g.stock_n == 0);
    *score0 = game_score_player(&g, 0, 0);
    *score1 = game_score_player(&g, 1, 0);
}

static void test_tie(void)
{
    Game g;
    game_new(&g, 1, 0, ROLE_COLOR);
    /* tabuleiro vazio: 0 x 0, quem comecou foi 0 -> vence 1 */
    CHECK(game_winner(&g) == 1);
    g.first = 1;
    CHECK(game_winner(&g) == 0);
}

int main(void)
{
    int i, a, b, wins_hard = 0;
    test_points();
    test_scoring();
    test_moves();
    test_tie();
    for (i = 0; i < 6; i++) {
        test_full_game(AI_HARD, AI_EASY, 100 + i, &a, &b);
        wins_hard += a > b;
    }
    printf("IA dificil venceu %d/6 contra a facil\n", wins_hard);
    CHECK(wins_hard >= 4);
    if (fails) {
        printf("%d falhas\n", fails);
        return 1;
    }
    printf("Todos os testes passaram.\n");
    return 0;
}
