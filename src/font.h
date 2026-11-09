#ifndef AQUAFRESH_FONT_H
#define AQUAFRESH_FONT_H
/* Glifos ASCII 32..127, 7 linhas de 5 bits (bit 4 = coluna esquerda).
 * Minusculas sao desenhadas como maiusculas, exceto:
 *   'x' = botao X, 'o' = botao circulo, '^' = botao triangulo, '[' = botao quadrado. */
extern const unsigned char font_glyphs[96][7];
#endif
