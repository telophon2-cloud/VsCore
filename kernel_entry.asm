[bits 32]
[extern main] ; Было: extern kernel_main

global _start

_start:
    call main ; Было: call kernel_main
    jmp $
