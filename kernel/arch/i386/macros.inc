%define BOCHS_BREAKPOINT xchg bx,bx

; virtual address to page directory (byte offset of entry)
%macro VIRT_TO_PD_OFFSET 1
        shr %1, 22      ; / 400000h
        shl %1, 2       ; * 4 (sizeof(uintptr_t))
%endmacro

; virtual address to page table entry byte offset
%macro VIRT_TO_PT_OFFSET 1
        and %1, 3fffffh ; mod 400000h
        shr %1, 12      ; / 1000h
        shl %1, 2       ; * 4 (sizeof(uintptr_t))
%endmacro

; ARG(1) -> [ebp + 12]
%define ARG(i) [ebp+4+4*(1+i)]

; LOAD_ARG ecx 0 -> ecx = [ebp + 8]
%macro LOAD_ARG 2
        mov %1, [ebp+4+4*(1+%2)]
%endmacro

%define POP_CALL_STACK(argCount) add esp, 4*argCount
