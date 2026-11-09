/* Gerado por tools/gen_audio.py - embute os sons no executavel. */
    .section .rodata
    .balign 64
    .global snd_mus_menu
snd_mus_menu:
    .incbin "assets/bin/snd_mus_menu.bin"
    .section .rodata
    .balign 64
    .global snd_mus_game
snd_mus_game:
    .incbin "assets/bin/snd_mus_game.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_cursor
snd_sfx_cursor:
    .incbin "assets/bin/snd_sfx_cursor.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_select
snd_sfx_select:
    .incbin "assets/bin/snd_sfx_select.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_back
snd_sfx_back:
    .incbin "assets/bin/snd_sfx_back.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_move
snd_sfx_move:
    .incbin "assets/bin/snd_sfx_move.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_place
snd_sfx_place:
    .incbin "assets/bin/snd_sfx_place.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_school
snd_sfx_school:
    .incbin "assets/bin/snd_sfx_school.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_error
snd_sfx_error:
    .incbin "assets/bin/snd_sfx_error.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_turn
snd_sfx_turn:
    .incbin "assets/bin/snd_sfx_turn.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_start
snd_sfx_start:
    .incbin "assets/bin/snd_sfx_start.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_win
snd_sfx_win:
    .incbin "assets/bin/snd_sfx_win.bin"
    .section .rodata
    .balign 64
    .global snd_sfx_lose
snd_sfx_lose:
    .incbin "assets/bin/snd_sfx_lose.bin"
#if defined(__linux__) && defined(__ELF__) && !defined(_EE)
    .section .note.GNU-stack,"",@progbits
#endif
