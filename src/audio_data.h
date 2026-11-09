/* Gerado por tools/gen_audio.py - nao edite. */
#ifndef AQUAFRESH_AUDIO_DATA_H
#define AQUAFRESH_AUDIO_DATA_H

#define SOUND_RATE 22050

enum {
    MUS_MENU,
    MUS_GAME,
    SFX_CURSOR,
    SFX_SELECT,
    SFX_BACK,
    SFX_MOVE,
    SFX_PLACE,
    SFX_SCHOOL,
    SFX_ERROR,
    SFX_TURN,
    SFX_START,
    SFX_WIN,
    SFX_LOSE,
    NUM_SOUNDS
};

typedef struct { const short *pcm; int length; int loop; } SoundDef;
extern const SoundDef sound_defs[NUM_SOUNDS];

#endif
