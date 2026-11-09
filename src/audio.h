/*
 * Aquafresh PS2 - mixer de audio em software.
 * Mistura a musica (em loop) e ate 8 efeitos, gerando PCM 16 bits estereo a
 * 48000 Hz (taxa nativa do SPU2). O backend do PS2 envia o resultado ao audsrv
 * numa thread propria; o programa de teste no PC grava em WAV.
 *
 * As funcoes snd_* / music_* sao chamadas pelo jogo (thread principal) e so
 * deixam pedidos numa fila; audio_mix (thread de audio) aplica os pedidos.
 */
#ifndef AQUAFRESH_AUDIO_H
#define AQUAFRESH_AUDIO_H

#include "audio_data.h"

#define AUDIO_RATE 48000
#define MUSIC_NONE (-1)

void audio_init(void);
/* Gera 'frames' quadros estereo intercalados (L, R) em 'out'. */
void audio_mix(short *out, int frames);

/* Efeito sonoro; pan de -100 (esquerda) a 100 (direita). */
void snd_play(int id);
void snd_play_pan(int id, int pan);
/* Troca a musica com fade (MUS_MENU, MUS_GAME ou MUSIC_NONE). */
void music_play(int id);

void audio_set_music_enabled(int on);
void audio_set_sfx_enabled(int on);
int  audio_music_enabled(void);
int  audio_sfx_enabled(void);

#endif
