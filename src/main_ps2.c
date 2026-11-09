/*
 * Aquafresh PS2 - ponto de entrada no PlayStation 2.
 * Video: gsKit (640x448 NTSC entrelacado, 60 Hz). Controle: libpad (rom0:PADMAN).
 * Audio: mixer proprio (src/audio.c) -> audsrv, 48 kHz estereo, numa thread.
 */
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <timer.h>
#include <stdio.h>
#include <string.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <audsrv.h>
#include <sbv_patches.h>

#include "gfx.h"
#include "assets.h"
#include "ui.h"
#include "audio.h"

static GSGLOBAL *gs;
static GSTEXTURE tex[NUM_ATLASES];

/* ------------------------------------------------------------------ video */

static u64 to_gs(Color c)
{
    /* alfa do GS vai de 0 a 128 */
    return GS_SETREG_RGBAQ(COL_R(c), COL_G(c), COL_B(c), (COL_A(c) * 128 + 127) / 255, 0x00);
}

void gfx_rect(float x, float y, float w, float h, Color c)
{
    if (w <= 0 || h <= 0)
        return;
    gsKit_prim_sprite(gs, x, y, x + w, y + h, 0, to_gs(c));
}

void gfx_tri(float x1, float y1, float x2, float y2, float x3, float y3, Color c)
{
    gsKit_prim_triangle(gs, x1, y1, x2, y2, x3, y3, 0, to_gs(c));
}

void gfx_rect_vgrad(float x, float y, float w, float h, Color top, Color bottom)
{
    u64 t = to_gs(top), b = to_gs(bottom);
    gsKit_prim_quad_gouraud(gs, x, y, x + w, y, x, y + h, x + w, y + h, 0, t, t, b, b);
}

void gfx_sprite(int a, float x, float y, float w, float h,
                float u0, float v0, float u1, float v1, Color mod)
{
    /* na modulacao do GS, 0x80 = 1.0 */
    u64 c = GS_SETREG_RGBAQ((COL_R(mod) + 1) >> 1, (COL_G(mod) + 1) >> 1, (COL_B(mod) + 1) >> 1,
                            (COL_A(mod) + 1) >> 1, 0x00);
    gsKit_TexManager_bind(gs, &tex[a]);
    gsKit_prim_sprite_texture(gs, &tex[a], x, y, u0, v0, x + w, y + h, u1, v1, 0, c);
}

/* Texturas T8 (paleta de 256 cores) embutidas no executavel; o
 * TexManager do gsKit envia para a VRAM quando sao usadas. */
static void textures_init(void)
{
    int i;
    for (i = 0; i < NUM_ATLASES; i++) {
        GSTEXTURE *t = &tex[i];
        memset(t, 0, sizeof(*t));
        t->Width = atlases[i].w;
        t->Height = atlases[i].h;
        t->PSM = GS_PSM_T8;
        t->ClutPSM = GS_PSM_CT32;
        t->Mem = (u32 *)atlases[i].data;
        t->Clut = (u32 *)(atlases[i].data + atlases[i].w * atlases[i].h);
        t->Filter = GS_FILTER_LINEAR;
        t->ClutStorageMode = GS_CLUT_STORAGE_CSM1;
        gsKit_setup_tbw(t);
    }
}

static void video_init(void)
{
    gs = gsKit_init_global_custom(GS_RENDER_QUEUE_OS_POOLSIZE * 4, GS_RENDER_QUEUE_PER_POOLSIZE);
    gs->Mode = GS_MODE_NTSC;
    gs->Interlace = GS_INTERLACED;
    gs->Field = GS_FIELD;
    gs->Width = SCREEN_W;
    gs->Height = SCREEN_H;
    gs->PSM = GS_PSM_CT24;
    gs->PSMZ = GS_PSMZ_16S;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_ON;
    gs->PrimAAEnable = GS_SETTING_OFF;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_set_clamp(gs, GS_CMODE_REPEAT);
    gsKit_vram_clear(gs);
    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);
    /* mistura normal: Cs*As + Cd*(1-As) */
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ZTEST_OFF);
    gsKit_TexManager_init(gs);
    textures_init();
}

/* --------------------------------------------------------------- controle */

static char pad_buf[2][256] __attribute__((aligned(64)));
static int pad_open[2];

static void pad_init(void)
{
    int port;
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    padInit(0);
    for (port = 0; port < 2; port++)
        pad_open[port] = padPortOpen(port, 0, pad_buf[port]) != 0;
}

static unsigned int pad_read_port(int port)
{
    struct padButtonStatus st;
    unsigned int out = 0, b;
    int state;
    if (!pad_open[port])
        return 0;
    state = padGetState(port, 0);
    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1)
        return 0;
    if (padRead(port, 0, &st) == 0)
        return 0;
    b = 0xFFFF ^ st.btns;   /* botoes sao ativos em nivel baixo */
    if (b & PAD_UP)       out |= BTN_UP;
    if (b & PAD_DOWN)     out |= BTN_DOWN;
    if (b & PAD_LEFT)     out |= BTN_LEFT;
    if (b & PAD_RIGHT)    out |= BTN_RIGHT;
    if (b & PAD_CROSS)    out |= BTN_CROSS;
    if (b & PAD_CIRCLE)   out |= BTN_CIRCLE;
    if (b & PAD_TRIANGLE) out |= BTN_TRIANGLE;
    if (b & PAD_SQUARE)   out |= BTN_SQUARE;
    if (b & PAD_START)    out |= BTN_START;
    if (b & PAD_SELECT)   out |= BTN_SELECT;
    if (b & (PAD_L1 | PAD_L2)) out |= BTN_L1;
    if (b & (PAD_R1 | PAD_R2)) out |= BTN_R1;
    /* analogico esquerdo tambem move o cursor (modo DualShock) */
    if ((st.mode >> 4) == 0x7) {
        if (st.ljoy_h < 40)  out |= BTN_LEFT;
        if (st.ljoy_h > 215) out |= BTN_RIGHT;
        if (st.ljoy_v < 40)  out |= BTN_UP;
        if (st.ljoy_v > 215) out |= BTN_DOWN;
    }
    return out;
}

/* ------------------------------------------------------------------- main */

/* ------------------------------------------------------------------ audio */

extern unsigned char irx_libsd[], irx_libsd_end[];
extern unsigned char irx_audsrv[], irx_audsrv_end[];

#define AUDIO_CHUNK 1024                 /* quadros por envio (~21 ms) */
static short audio_out[AUDIO_CHUNK * 2] __attribute__((aligned(64)));
static unsigned char audio_stack[16 * 1024] __attribute__((aligned(16)));
extern void *_gp;

static void audio_thread(void *arg)
{
    (void)arg;
    for (;;) {
        audio_mix(audio_out, AUDIO_CHUNK);
        /* bloqueia (dormindo no RPC) ate caber no buffer do IOP */
        audsrv_wait_audio(sizeof(audio_out));
        audsrv_play_audio((const char *)audio_out, sizeof(audio_out));
    }
}

static void sound_init(void)
{
    struct audsrv_fmt_t fmt;
    ee_thread_t th;
    int id, ret;

    audio_init();
    sbv_patch_enable_lmb();     /* permite carregar modulos a partir da RAM do EE */
    SifExecModuleBuffer(irx_libsd, irx_libsd_end - irx_libsd, 0, NULL, &ret);
    if (SifExecModuleBuffer(irx_audsrv, irx_audsrv_end - irx_audsrv, 0, NULL, &ret) < 0 || ret < 0) {
        printf("audsrv.irx nao carregou: jogo segue sem som\n");
        return;
    }
    if (audsrv_init() != 0) {
        printf("audsrv_init falhou: %s\n", audsrv_get_error_string());
        return;
    }
    fmt.freq = AUDIO_RATE;
    fmt.bits = 16;
    fmt.channels = 2;
    if (audsrv_set_format(&fmt) != 0) {
        printf("audsrv_set_format falhou\n");
        return;
    }
    audsrv_set_volume(MAX_VOLUME);

    /* thread de audio com prioridade maior que a do jogo: o som nao
     * engasga quando a IA da CPU pensa por alguns quadros */
    ChangeThreadPriority(GetThreadId(), 64);
    memset(&th, 0, sizeof(th));
    th.func = (void *)audio_thread;
    th.stack = audio_stack;
    th.stack_size = sizeof(audio_stack);
    th.gp_reg = &_gp;
    th.initial_priority = 40;
    id = CreateThread(&th);
    if (id >= 0)
        StartThread(id, NULL);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    SifInitRpc(0);
    /* reinicia o IOP para um estado limpo (ex.: vindo do uLaunchELF) */
    while (!SifIopReset("", 0)) {}
    while (!SifIopSync()) {}
    SifInitRpc(0);

    pad_init();
    sound_init();
    video_init();
    ui_init((unsigned int)cpu_ticks());

    for (;;) {
        /* controle 1 = jogador 1 e menus; controle 2 = jogador 2 */
        ui_update(pad_read_port(0), pad_read_port(1));
        gsKit_clear(gs, GS_SETREG_RGBAQ(6, 30, 80, 0x80, 0x00));
        ui_draw();
        gsKit_queue_exec(gs);
        gsKit_sync_flip(gs);
        gsKit_TexManager_nextFrame(gs);
    }
    return 0;
}
