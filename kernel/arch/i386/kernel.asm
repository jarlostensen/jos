[bits 32]
section .bss
; empty
section .data
; empty
section .rodata
; empty 

section .text

global _bochs_debugbreak:function
_bochs_debugbreak:
    xchg bx,bx
    ret

