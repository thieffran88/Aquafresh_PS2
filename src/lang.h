/*
 * Aquafresh PS2 - textos em portugues (principal), ingles e espanhol.
 * A fonte nao tem acentos: os textos sao escritos sem eles.
 * Minusculas x o ^ [ viram icones dos botoes do controle.
 */
#ifndef AQUAFRESH_LANG_H
#define AQUAFRESH_LANG_H

enum { LANG_PT, LANG_EN, LANG_ES, NUM_LANGS };

enum {
    S_YOU, S_CPU, S_PLAYER1, S_PLAYER2, S_COLORS, S_ANIMALS,
    S_MSG_PICK_OR_SKIP, S_MSG_SURROUNDED, S_MSG_STRAIGHT, S_MSG_EMPTY,
    S_POOL, S_STOCK_FMT,
    S_PH_MOVE, S_PH_PLACE, S_CPU_THINKING, S_CPU_PLAYING, S_REEF_FULL,
    S_COUNTING, S_HELP_CPU, S_HELP_MOVE, S_HELP_DEST, S_HELP_PICK_FIRST, S_HELP_PICK, S_HELP_DROP,
    S_TURN_OF_FMT, S_YOUR_TURN,
    S_MENU_1P, S_MENU_2P, S_MENU_RULES, S_MENU_LANG,
    S_EASY, S_NORMAL, S_HARD,
    S_OPT_YOUR_SCHOOLS, S_OPT_DIFFICULTY, S_OPT_WHO_STARTS, S_OPT_P1_FORMS, S_OPT_START,
    S_HELP_SETUP, S_HELP_CONFIRM, S_HELP_RULES, S_HELP_RESULT,
    S_RULES_TITLE0, S_RULES_TITLE1, S_RULES_TITLE2, S_RULES_TITLE3,
    S_RULES_EXAMPLE,
    S_PAUSE, S_CONTINUE, S_MUSIC_ON, S_MUSIC_OFF, S_SFX_ON, S_SFX_OFF, S_QUIT,
    S_YOU_WIN, S_CPU_WINS, S_WINS_FMT, S_TIE_NOTE, S_SCHOOLS_OF_FMT,
    S_LANG_PROMPT,
    S_PAD_FMT,
    NUM_STRINGS
};

#define RULES_PAGES 4
#define RULES_LINES 9

extern int g_lang;
const char *tr(int id);
/* linha 'line' da pagina 'page' das regras no idioma atual */
const char *rules_line(int page, int line);
/* nome do idioma no proprio idioma */
const char *lang_name(int lang);

#endif
