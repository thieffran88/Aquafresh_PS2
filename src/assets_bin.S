/* Gerado por tools/gen_assets.py - embute as texturas no executavel. */
    .section .rodata
    .balign 128
    .global asset_bg_play
asset_bg_play:
    .incbin "assets/bin/bg_play.bin"
    .section .rodata
    .balign 128
    .global asset_bg_title
asset_bg_title:
    .incbin "assets/bin/bg_title.bin"
    .section .rodata
    .balign 128
    .global asset_tiles
asset_tiles:
    .incbin "assets/bin/tiles.bin"
    .section .rodata
    .balign 128
    .global asset_deco
asset_deco:
    .incbin "assets/bin/deco.bin"
    .section .rodata
    .balign 128
    .global asset_font
asset_font:
    .incbin "assets/bin/font.bin"
#if defined(__linux__) && defined(__ELF__) && !defined(_EE)
    .section .note.GNU-stack,"",@progbits
#endif
