#!/usr/bin/env python3
"""
Sintetiza a musica e os efeitos sonoros do Aquafresh PS2 (tema recife de corais).
Tudo e gerado aqui (nenhum sample externo): marimba, steel drum, pads, baixo,
bolhas, ondas e chocalho.

Saida:
  assets/bin/snd_<nome>.bin  PCM 16 bits mono little-endian a 22050 Hz
  src/audio_data.c / .h      tabela de sons
  src/audio_bin.S            embute os .bin no executavel
  assets/preview/*.wav       para ouvir no PC (nao vai para o jogo)

Requer numpy e scipy. Uso: python3 tools/gen_audio.py
"""
import os
import wave

import numpy as np
from scipy.signal import butter, lfilter, fftconvolve

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "assets", "bin")
PREVIEW = os.path.join(ROOT, "assets", "preview")
SR = 22050
RNG = np.random.default_rng(7)


def midi_hz(m):
    return 440.0 * 2 ** ((m - 69) / 12.0)


def t_axis(dur):
    return np.arange(int(dur * SR)) / SR


def env_ad(n, attack, tau):
    t = np.arange(n) / SR
    e = np.exp(-t / tau)
    a = int(attack * SR)
    if a > 0:
        e[:a] *= np.linspace(0, 1, a)
    return e


def lowpass(x, fc, order=2):
    b, a = butter(order, min(fc / (SR / 2), 0.99))
    return lfilter(b, a, x)


def highpass(x, fc, order=2):
    b, a = butter(order, fc / (SR / 2), "high")
    return lfilter(b, a, x)


def bandpass(x, lo, hi, order=2):
    b, a = butter(order, [lo / (SR / 2), min(hi / (SR / 2), 0.99)], "band")
    return lfilter(b, a, x)

# ------------------------------------------------------------ instrumentos

def marimba(f, dur=1.2, vel=1.0):
    t = t_axis(dur)
    n = len(t)
    x = np.sin(2 * np.pi * f * t) * env_ad(n, 0.002, 0.30)
    x += 0.30 * np.sin(2 * np.pi * f * 3.93 * t) * env_ad(n, 0.001, 0.06)
    x += 0.06 * np.sin(2 * np.pi * f * 9.2 * t) * env_ad(n, 0.0005, 0.02)
    return x * vel


def steeldrum(f, dur=1.4, vel=1.0):
    t = t_axis(dur)
    n = len(t)
    bend = 1 + 0.004 * np.exp(-t / 0.03)        # leve "ping" no ataque
    ph = 2 * np.pi * f * np.cumsum(bend) / SR
    x = (np.sin(ph) + 0.55 * np.sin(2 * ph + 0.3) * np.exp(-t / 0.35)
         + 0.28 * np.sin(3.01 * ph) * np.exp(-t / 0.20)
         + 0.12 * np.sin(4.02 * ph) * np.exp(-t / 0.12))
    return x * env_ad(n, 0.003, 0.45) * vel


def bell(f, dur=2.0, vel=1.0):
    t = t_axis(dur)
    n = len(t)
    x = (np.sin(2 * np.pi * f * t) + 0.4 * np.sin(2 * np.pi * f * 2.76 * t) * np.exp(-t / 0.4)
         + 0.2 * np.sin(2 * np.pi * f * 5.4 * t) * np.exp(-t / 0.15))
    return x * env_ad(n, 0.002, 0.7) * vel


def bass(f, dur, vel=1.0):
    t = t_axis(dur + 0.15)
    n = len(t)
    x = np.sin(2 * np.pi * f * t) + 0.25 * np.sin(4 * np.pi * f * t) * np.exp(-t / 0.1)
    e = 0.55 * env_ad(n, 0.004, 0.18) + 0.45 * env_ad(n, 0.004, 1.2)
    rel = np.clip((dur + 0.15 - t) / 0.15, 0, 1)
    return x * e * rel * vel


def pad(freqs, dur, vel=1.0):
    t = t_axis(dur + 0.8)
    n = len(t)
    x = np.zeros(n)
    for f in freqs:
        for det in (-0.006, 0.0, 0.006):
            ph = (f * (1 + det) * t + RNG.random()) % 1.0
            x += 2 * ph - 1
    x = lowpass(x / (3 * len(freqs)), 900)
    att = np.clip(t / 0.5, 0, 1)
    rel = np.clip((dur + 0.8 - t) / 0.8, 0, 1)
    lfo = 0.85 + 0.15 * np.sin(2 * np.pi * 0.3 * t)
    return x * att * rel * lfo * vel


def shaker(vel=1.0, dur=0.07):
    n = int(dur * SR)
    x = bandpass(RNG.standard_normal(n), 5000, 10000)
    return x * env_ad(n, 0.004, dur / 3) * vel


def conga(f=180, vel=1.0):
    t = t_axis(0.3)
    ph = 2 * np.pi * np.cumsum(f * (1 + 0.6 * np.exp(-t / 0.02))) / SR
    x = np.sin(ph) * env_ad(len(t), 0.001, 0.09)
    x += 0.2 * bandpass(RNG.standard_normal(len(t)), 800, 3000) * env_ad(len(t), 0.0005, 0.01)
    return x * vel


def bubble(f0=500, f1=1500, dur=0.09, vel=1.0):
    t = t_axis(dur)
    f = f0 * (f1 / f0) ** (t / dur)
    ph = 2 * np.pi * np.cumsum(f) / SR
    return np.sin(ph) * env_ad(len(t), 0.002, dur / 2.5) * vel


def ocean(dur, vel=1.0):
    n = int(dur * SR)
    x = lowpass(RNG.standard_normal(n), 500, 2)
    t = np.arange(n) / SR
    waves = 0.45 + 0.55 * (0.5 + 0.5 * np.sin(2 * np.pi * t / (dur / 4) - 1.2)) ** 2
    return x * waves * vel


def mix_at(buf, x, start_s, gain=1.0, loop=True):
    s = int(start_s * SR)
    n = len(x)
    if loop:
        idx = (np.arange(n) + s) % len(buf)
        np.add.at(buf, idx, x * gain)
    else:
        e = min(len(buf), s + n)
        buf[s:e] += x[: e - s] * gain


def reverb(x, seconds=1.2, wet=0.25, loop=True):
    n = int(seconds * SR)
    t = np.arange(n) / SR
    ir = RNG.standard_normal(n) * np.exp(-t / (seconds / 5))
    ir = lowpass(ir, 4000)
    ir /= np.sqrt(np.sum(ir ** 2))
    y = fftconvolve(x, ir)
    if loop:   # cauda do fim volta para o comeco: loop sem emenda
        out = y[: len(x)].copy()
        tail = y[len(x):]
        out[: len(tail)] += tail[: len(x)]
        y = out
    else:      # mantem a cauda: estende o sinal seco com silencio
        x = np.concatenate([x, np.zeros(len(y) - len(x))])
    return x * (1 - wet) + y * wet * 0.6


def normalize(x, peak=0.85):
    return x * (peak / max(1e-9, np.max(np.abs(x))))

# --------------------------------------------------------------- musicas

NOTE = {"C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3, "E": 4, "F": 5, "F#": 6,
        "Gb": 6, "G": 7, "G#": 8, "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11}


def n(name):
    """'A5' -> numero MIDI"""
    p, o = name[:-1], int(name[-1])
    return 12 * (o + 1) + NOTE[p]


def song(bpm, bars, chords, melody, style):
    beat = 60.0 / bpm
    total = bars * 4 * beat
    L = int(round(total * SR))
    tracks = {k: np.zeros(L) for k in ("pad", "bass", "arp", "mel", "perc", "fx")}

    for bar, chord in enumerate(chords):
        t0 = bar * 4 * beat
        notes = [n(c) for c in chord]
        root = notes[0]
        tracks["pad"][:] += 0
        mix_at(tracks["pad"], pad([midi_hz(m) for m in notes[1:]], 4 * beat), t0, 0.5)
        if style == "calmo":
            for b, dur, off in ((0, 1.5, 0), (1.5, 1.0, 7), (2.5, 1.5, 12)):
                mix_at(tracks["bass"], bass(midi_hz(root - 12 + off), dur * beat), t0 + b * beat, 0.9)
            pattern = [1, 2, 3, 4, 3, 2, 3, 4]
            for i, pi in enumerate(pattern):
                m = notes[min(pi, len(notes) - 1)]
                vel = 0.55 if i % 2 == 0 else 0.4
                mix_at(tracks["arp"], marimba(midi_hz(m + 12), 0.9, vel), t0 + i * 0.5 * beat)
            for i in range(8):
                mix_at(tracks["perc"], shaker(0.35 if i % 2 else 0.2), t0 + i * 0.5 * beat)
        else:  # festa: calipso
            for b, off, d in ((0, 0, 0.75), (0.75, 0, 0.25), (1.5, 7, 0.5), (2, 12, 0.75),
                              (2.75, 7, 0.25), (3.5, 7, 0.5)):
                mix_at(tracks["bass"], bass(midi_hz(root - 12 + off), d * beat), t0 + b * beat, 0.9)
            for b in (0.5, 1.5, 2.5, 3.5):     # acordes no contratempo
                for m in notes[1:]:
                    mix_at(tracks["arp"], marimba(midi_hz(m + 12), 0.4, 0.28), t0 + b * beat)
            for i in range(16):
                mix_at(tracks["perc"], shaker(0.5 if i % 4 == 2 else 0.22, 0.05), t0 + i * 0.25 * beat)
            for b, f in ((0, 200), (1.5, 260), (2, 200), (3, 260), (3.5, 200)):
                mix_at(tracks["perc"], conga(f, 0.6), t0 + b * beat)

    inst = steeldrum if style == "festa" else steeldrum
    for bar, b, dur, name in melody:
        m = n(name)
        mix_at(tracks["mel"], inst(midi_hz(m), max(0.5, dur * beat + 0.4), 0.8), (bar * 4 + b) * beat)

    # bolhas espalhadas e ambiente de ondas
    for i in range(bars * 2):
        tb = RNG.random() * total
        f0 = 300 + RNG.random() * 500
        mix_at(tracks["fx"], bubble(f0, f0 * 2.5, 0.06 + RNG.random() * 0.05, 0.25), tb)
    if style == "calmo":
        tracks["fx"] += ocean(L / SR + 0.01, 0.5)[:L]

    gains = {"pad": 0.35, "bass": 0.55, "arp": 0.35, "mel": 0.5, "perc": 0.25, "fx": 0.3}
    if style == "festa":
        gains.update({"pad": 0.22, "arp": 0.5, "perc": 0.35})
    x = sum(tracks[k] * g for k, g in gains.items())
    x = highpass(x, 35, 1)
    x = reverb(x, 1.6 if style == "calmo" else 1.0, 0.3 if style == "calmo" else 0.18)
    return normalize(x, 0.8)


def music_game():
    # "Recife Calmo" - Fa maior, 92 BPM, 8 compassos
    chords = [["F3", "A3", "C4", "E4"], ["A2", "C4", "E4", "G4"], ["D3", "F3", "A3", "C4"],
              ["C3", "E3", "G3", "C4"], ["Bb2", "D4", "F4", "A4"], ["A2", "C4", "E4", "G4"],
              ["G2", "Bb3", "D4", "F4"], ["C3", "E3", "G3", "Bb3"]]
    mel = [(0, 0, 1.5, "A5"), (0, 1.5, 0.5, "C6"), (0, 2, 1, "A5"), (0, 3, 1, "G5"),
           (1, 0, 1.5, "E5"), (1, 1.5, 0.5, "G5"), (1, 2, 2, "A5"),
           (2, 0, 1, "F5"), (2, 1, 1, "A5"), (2, 2, 1.5, "C6"), (2, 3.5, 0.5, "D6"),
           (3, 0, 1.5, "C6"), (3, 1.5, 0.5, "G5"), (3, 2, 2, "E5"),
           (4, 0, 1, "D6"), (4, 1, 0.5, "C6"), (4, 1.5, 0.5, "Bb5"), (4, 2, 1, "A5"), (4, 3, 1, "F5"),
           (5, 0, 1, "E5"), (5, 1, 1, "G5"), (5, 2, 2, "C6"),
           (6, 0, 1, "Bb5"), (6, 1, 0.5, "A5"), (6, 1.5, 0.5, "G5"), (6, 2, 1, "F5"), (6, 3, 1, "D5"),
           (7, 0, 1.5, "E5"), (7, 1.5, 0.5, "G5"), (7, 2, 2, "C5")]
    return song(92, 8, chords, mel, "calmo")


def music_menu():
    # "Festa no Recife" - Do maior, calipso 112 BPM, 8 compassos
    chords = [["C3", "E4", "G4", "C5"], ["F2", "F4", "A4", "C5"], ["G2", "G4", "B4", "D5"],
              ["C3", "E4", "G4", "C5"], ["A2", "E4", "A4", "C5"], ["F2", "F4", "A4", "C5"],
              ["G2", "F4", "G4", "B4"], ["C3", "E4", "G4", "C5"]]
    mel = [(0, 0, 0.5, "E5"), (0, 0.5, 0.5, "G5"), (0, 1, 0.5, "C6"), (0, 1.5, 1, "G5"),
           (0, 2.5, 0.5, "E5"), (0, 3, 1, "G5"),
           (1, 0, 0.5, "F5"), (1, 0.5, 0.5, "A5"), (1, 1, 0.5, "C6"), (1, 1.5, 1, "A5"),
           (1, 2.5, 0.5, "F5"), (1, 3, 1, "A5"),
           (2, 0, 0.5, "G5"), (2, 0.5, 0.5, "B5"), (2, 1, 0.5, "D6"), (2, 1.5, 0.5, "B5"),
           (2, 2, 0.5, "G5"), (2, 2.5, 0.5, "A5"), (2, 3, 1, "B5"),
           (3, 0, 1.5, "C6"), (3, 1.5, 0.5, "B5"), (3, 2, 0.5, "A5"), (3, 2.5, 1.5, "G5"),
           (4, 0, 0.5, "A5"), (4, 0.5, 0.5, "C6"), (4, 1, 0.5, "E6"), (4, 1.5, 1, "C6"),
           (4, 2.5, 0.5, "A5"), (4, 3, 1, "E5"),
           (5, 0, 0.5, "F5"), (5, 0.5, 0.5, "A5"), (5, 1, 1, "C6"), (5, 2, 0.5, "D6"),
           (5, 2.5, 0.5, "C6"), (5, 3, 1, "A5"),
           (6, 0, 0.5, "G5"), (6, 0.5, 0.5, "F5"), (6, 1, 0.5, "D5"), (6, 1.5, 0.5, "F5"),
           (6, 2, 0.5, "G5"), (6, 2.5, 0.5, "B5"), (6, 3, 1, "D6"),
           (7, 0, 1.5, "C6"), (7, 1.5, 0.5, "G5"), (7, 2, 2, "C6")]
    return song(112, 8, chords, mel, "festa")

# ---------------------------------------------------------------- efeitos

def place(sounds, start, x, gain=1.0):
    s = int(start * SR)
    if s + len(x) > len(sounds):
        sounds = np.concatenate([sounds, np.zeros(s + len(x) - len(sounds))])
    sounds[s:s + len(x)] += x * gain
    return sounds


def sfx_cursor():
    return bubble(900, 1500, 0.045, 0.6)


def sfx_select():
    x = np.zeros(1)
    x = place(x, 0, marimba(midi_hz(n("C6")), 0.35, 0.8))
    x = place(x, 0.06, marimba(midi_hz(n("G6")), 0.4, 0.8))
    x = place(x, 0.0, bubble(600, 1600, 0.07, 0.4))
    return x


def sfx_back():
    x = np.zeros(1)
    x = place(x, 0, marimba(midi_hz(n("G5")), 0.3, 0.7))
    x = place(x, 0.07, marimba(midi_hz(n("C5")), 0.4, 0.7))
    return x


def sfx_move():
    d = 0.32
    t = t_axis(d)
    noise = RNG.standard_normal(len(t))
    out = np.zeros(len(t))
    # chiado de agua com filtro que sobe
    for i in range(8):
        a, b = int(i * len(t) / 8), int((i + 1) * len(t) / 8)
        f = 400 * (4 ** (i / 7))
        out[a:b] = bandpass(noise, f, f * 2.2)[a:b]
    out *= np.sin(np.pi * t / d) ** 1.5 * 0.6
    for k in range(3):
        out = place(out, 0.08 + k * 0.07, bubble(500 + k * 200, 1400 + k * 300, 0.05, 0.35))
    return out


def sfx_place():
    t = t_axis(0.2)
    f = 140 + 260 * np.exp(-t / 0.025)
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = np.sin(ph) * env_ad(len(t), 0.001, 0.05)
    x = place(x, 0.03, bubble(700, 1800, 0.06, 0.35))
    x = place(x, 0.0, marimba(midi_hz(n("E5")), 0.3, 0.25))
    return x


def sfx_school():
    x = np.zeros(1)
    for i, nm in enumerate(["C6", "E6", "G6", "C7"]):
        x = place(x, i * 0.055, steeldrum(midi_hz(n(nm)), 0.6, 0.55))
    x = place(x, 0.22, bell(midi_hz(n("G7")), 0.6, 0.15))
    return x


def sfx_error():
    x = np.zeros(1)
    for i in range(2):
        x = place(x, i * 0.09, lowpass(marimba(midi_hz(n("A3")), 0.25, 0.9), 900))
    return x


def sfx_turn():
    x = np.zeros(1)
    x = place(x, 0, bell(midi_hz(n("E6")), 1.0, 0.45))
    x = place(x, 0.1, bell(midi_hz(n("B6")), 1.0, 0.35))
    return x


def sfx_start():
    x = ocean(0.9, 1.4) * np.sin(np.pi * t_axis(0.9) / 0.9)
    for i, nm in enumerate(["C5", "G5", "C6", "E6"]):
        x = place(x, 0.15 + i * 0.07, steeldrum(midi_hz(n(nm)), 0.8, 0.5))
    return x


def sfx_win():
    x = np.zeros(1)
    seq = [("C5", 0), ("E5", 0.12), ("G5", 0.24), ("C6", 0.36), ("E6", 0.48)]
    for nm, t0 in seq:
        x = place(x, t0, steeldrum(midi_hz(n(nm)), 0.9, 0.6))
    for nm in ["C5", "E5", "G5", "C6", "E6", "G6"]:
        x = place(x, 0.7, steeldrum(midi_hz(n(nm)), 2.0, 0.35))
    for k in range(10):
        x = place(x, 0.7 + k * 0.08, bubble(500 + 90 * k, 1500 + 120 * k, 0.06, 0.25))
    return reverb(x, 1.0, 0.25, loop=False)


def sfx_lose():
    x = np.zeros(1)
    for i, nm in enumerate(["G5", "E5", "C5", "G4"]):
        x = place(x, i * 0.2, marimba(midi_hz(n(nm)), 0.8, 0.7))
    t = t_axis(1.0)
    wob = np.sin(2 * np.pi * midi_hz(n("C4")) * t + 3 * np.sin(2 * np.pi * 5 * t))
    x = place(x, 0.8, wob * env_ad(len(t), 0.01, 0.35) * 0.4)
    return x

# ------------------------------------------------------------- exportacao

def to_pcm(x):
    return (np.clip(x, -1, 1) * 32767).astype("<i2")


def write_wav(path, x, sr=SR):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(to_pcm(x).tobytes())


def main():
    os.makedirs(BIN, exist_ok=True)
    os.makedirs(PREVIEW, exist_ok=True)
    sounds = [
        ("MUS_MENU", music_menu(), 1.0),
        ("MUS_GAME", music_game(), 1.0),
        ("SFX_CURSOR", sfx_cursor(), 0.5),
        ("SFX_SELECT", sfx_select(), 0.7),
        ("SFX_BACK", sfx_back(), 0.6),
        ("SFX_MOVE", sfx_move(), 0.7),
        ("SFX_PLACE", sfx_place(), 0.8),
        ("SFX_SCHOOL", sfx_school(), 0.7),
        ("SFX_ERROR", sfx_error(), 0.6),
        ("SFX_TURN", sfx_turn(), 0.5),
        ("SFX_START", sfx_start(), 0.7),
        ("SFX_WIN", sfx_win(), 0.85),
        ("SFX_LOSE", sfx_lose(), 0.75),
    ]
    table = []
    for name, x, peak in sounds:
        x = normalize(x, peak) if name.startswith("SFX") else x
        pcm = to_pcm(x)
        if len(pcm) % 64:   # alinha para o .incbin seguinte
            pcm = np.concatenate([pcm, np.zeros(64 - len(pcm) % 64, dtype="<i2")])
        low = name.lower()
        with open(os.path.join(BIN, "snd_%s.bin" % low), "wb") as f:
            f.write(pcm.tobytes())
        write_wav(os.path.join(PREVIEW, "%s.wav" % low), x)
        table.append((name, len(pcm), name.startswith("MUS")))
        print("%-11s %6.2f s" % (name, len(x) / SR))

    h = ["/* Gerado por tools/gen_audio.py - nao edite. */",
         "#ifndef AQUAFRESH_AUDIO_DATA_H", "#define AQUAFRESH_AUDIO_DATA_H", "",
         "#define SOUND_RATE %d" % SR, "", "enum {"]
    h += ["    %s," % name for name, _, _ in table]
    h += ["    NUM_SOUNDS", "};", "",
          "typedef struct { const short *pcm; int length; int loop; } SoundDef;",
          "extern const SoundDef sound_defs[NUM_SOUNDS];", "", "#endif", ""]
    open(os.path.join(ROOT, "src", "audio_data.h"), "w").write("\n".join(h))

    c = ["/* Gerado por tools/gen_audio.py - nao edite. */", '#include "audio_data.h"', ""]
    c += ["extern const short snd_%s[];" % name.lower() for name, _, _ in table]
    c += ["", "const SoundDef sound_defs[NUM_SOUNDS] = {"]
    c += ["    { snd_%s, %d, %d }," % (name.lower(), ln, lp) for name, ln, lp in table]
    c += ["};", ""]
    open(os.path.join(ROOT, "src", "audio_data.c"), "w").write("\n".join(c))

    s = ["/* Gerado por tools/gen_audio.py - embute os sons no executavel. */"]
    for name, _, _ in table:
        low = name.lower()
        s += ["    .section .rodata", "    .balign 64", "    .global snd_%s" % low,
              "snd_%s:" % low, '    .incbin "assets/bin/snd_%s.bin"' % low]
    s += ["#if defined(__linux__) && defined(__ELF__) && !defined(_EE)",
          '    .section .note.GNU-stack,"",@progbits', "#endif", ""]
    open(os.path.join(ROOT, "src", "audio_bin.S"), "w").write("\n".join(s))


if __name__ == "__main__":
    main()
