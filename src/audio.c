/*
 * Aquafresh PS2 - mixer de audio (independente de plataforma).
 */
#include "audio.h"

#define MAX_VOICES   8
#define QUEUE_SIZE   32
#define STEP_FIX     ((unsigned)(((unsigned long long)SOUND_RATE << 16) / AUDIO_RATE))
#define FADE_FRAMES  (AUDIO_RATE * 2 / 5)     /* 0,4 s */
#define GAIN_ONE     4096

typedef struct {
    int id;            /* -1 = livre */
    int pos;           /* amostra atual na fonte */
    unsigned frac;     /* fracao 16.16 */
    int vol_l, vol_r;  /* 0..GAIN_ONE */
} Voice;

/* fila de pedidos: escrita pela thread do jogo, lida pela de audio */
static volatile struct { short kind, id, pan; } queue[QUEUE_SIZE];
static volatile unsigned int q_head, q_tail;

static Voice voices[MAX_VOICES];
static int music_cur = MUSIC_NONE, music_next = MUSIC_NONE;
static int music_pos;
static unsigned music_frac;
static int music_gain;              /* 0..MUSIC_ONE */
#define MUSIC_ONE 65536
static int duck_gain = GAIN_ONE;
static volatile int music_on = 1, sfx_on = 1;

enum { REQ_SFX, REQ_MUSIC };

static void push(int kind, int id, int pan)
{
    unsigned int h = q_head;
    if (h - q_tail >= QUEUE_SIZE)
        return;                      /* fila cheia: descarta */
    queue[h % QUEUE_SIZE].kind = (short)kind;
    queue[h % QUEUE_SIZE].id = (short)id;
    queue[h % QUEUE_SIZE].pan = (short)pan;
    q_head = h + 1;                  /* publica depois de preencher */
}

void audio_init(void)
{
    int i;
    for (i = 0; i < MAX_VOICES; i++)
        voices[i].id = -1;
    q_head = q_tail = 0;
}

void snd_play_pan(int id, int pan)
{
    if (sfx_on && id >= 0 && id < NUM_SOUNDS)
        push(REQ_SFX, id, pan);
}

void snd_play(int id) { snd_play_pan(id, 0); }

void music_play(int id) { push(REQ_MUSIC, id, 0); }

void audio_set_music_enabled(int on) { music_on = on; }
void audio_set_sfx_enabled(int on) { sfx_on = on; }
int  audio_music_enabled(void) { return music_on; }
int  audio_sfx_enabled(void) { return sfx_on; }

static void start_voice(int id, int pan)
{
    int i, best = 0, best_pos = -1;
    Voice *v;
    if (pan < -100) pan = -100;
    if (pan > 100) pan = 100;
    /* voz livre; senao rouba a que esta tocando ha mais tempo */
    for (i = 0; i < MAX_VOICES; i++) {
        if (voices[i].id < 0) { best = i; break; }
        if (voices[i].pos > best_pos) { best_pos = voices[i].pos; best = i; }
    }
    v = &voices[best];
    v->pos = 0;
    v->frac = 0;
    v->vol_l = GAIN_ONE * (100 - (pan > 0 ? pan : 0)) / 100;
    v->vol_r = GAIN_ONE * (100 + (pan < 0 ? pan : 0)) / 100;
    v->id = id;
}

static void process_queue(void)
{
    while (q_tail != q_head) {
        unsigned int t = q_tail % QUEUE_SIZE;
        if (queue[t].kind == REQ_SFX)
            start_voice(queue[t].id, queue[t].pan);
        else
            music_next = queue[t].id;
        q_tail++;
    }
}

static inline int sample_at(const short *pcm, int len, int pos, unsigned frac, int loop)
{
    int a = pcm[pos];
    int nxt = pos + 1;
    int b;
    if (nxt >= len)
        nxt = loop ? 0 : pos;
    b = pcm[nxt];
    return a + (int)(((long long)(b - a) * (int)frac) >> 16);
}

void audio_mix(short *out, int frames)
{
    int f, i;
    int any_jingle = 0;

    process_queue();
    for (i = 0; i < MAX_VOICES; i++)
        if (voices[i].id == SFX_WIN || voices[i].id == SFX_LOSE)
            any_jingle = 1;

    for (f = 0; f < frames; f++) {
        int l = 0, r = 0;

        /* musica com fade entre faixas e abaixada durante as vinhetas */
        {
            int target = (music_on && music_next == music_cur && music_cur != MUSIC_NONE) ? MUSIC_ONE : 0;
            int duck_target = any_jingle ? GAIN_ONE / 5 : GAIN_ONE;
            if (music_gain < target) music_gain += MUSIC_ONE / FADE_FRAMES;
            if (music_gain > target) music_gain -= MUSIC_ONE / FADE_FRAMES;
            if (music_gain < 0) music_gain = 0;
            if (music_gain > MUSIC_ONE) music_gain = MUSIC_ONE;
            if (duck_gain < duck_target) duck_gain++;
            if (duck_gain > duck_target) duck_gain -= 2;
            if (music_gain == 0 && music_cur != music_next) {
                music_cur = music_next;
                music_pos = 0;
                music_frac = 0;
            }
            if (music_cur != MUSIC_NONE) {
                const SoundDef *d = &sound_defs[music_cur];
                if (music_gain > 0) {
                    int s = sample_at(d->pcm, d->length, music_pos, music_frac, 1);
                    s = (int)(((long long)s * music_gain * duck_gain) >> 28);
                    s = s * 5 / 8;           /* musica um pouco abaixo dos efeitos */
                    l += s;
                    r += s;
                }
                music_frac += STEP_FIX;
                music_pos += (int)(music_frac >> 16);
                music_frac &= 0xFFFF;
                if (music_pos >= d->length)
                    music_pos -= d->length;
            }
        }

        for (i = 0; i < MAX_VOICES; i++) {
            Voice *v = &voices[i];
            const SoundDef *d;
            int s;
            if (v->id < 0)
                continue;
            d = &sound_defs[v->id];
            s = sample_at(d->pcm, d->length, v->pos, v->frac, 0);
            l += (s * v->vol_l) >> 12;
            r += (s * v->vol_r) >> 12;
            v->frac += STEP_FIX;
            v->pos += (int)(v->frac >> 16);
            v->frac &= 0xFFFF;
            if (v->pos >= d->length)
                v->id = -1;
        }

        /* volume geral e limitador suave para varios sons ao mesmo tempo */
        l = l * 13 / 16;
        r = r * 13 / 16;
        if (l > 24000) l = 24000 + (l - 24000) / 3;
        if (l < -24000) l = -24000 + (l + 24000) / 3;
        if (r > 24000) r = 24000 + (r - 24000) / 3;
        if (r < -24000) r = -24000 + (r + 24000) / 3;
        if (l > 32767) l = 32767;
        if (l < -32768) l = -32768;
        if (r > 32767) r = 32767;
        if (r < -32768) r = -32768;
        out[2 * f] = (short)l;
        out[2 * f + 1] = (short)r;
    }
}
