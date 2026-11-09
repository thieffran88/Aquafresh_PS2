/* Modulos do IOP embutidos no ELF (vem do PS2SDK: iop/irx). */
    .section .rodata
    .balign 16
    .global irx_libsd
irx_libsd:
    .incbin "libsd.irx"
    .global irx_libsd_end
irx_libsd_end:
    .balign 16
    .global irx_audsrv
irx_audsrv:
    .incbin "audsrv.irx"
    .global irx_audsrv_end
irx_audsrv_end:
