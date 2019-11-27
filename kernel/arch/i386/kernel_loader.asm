[bits 32]

; multiboot header 
MBALIGN           equ  (1 << 0)              ; align loaded modules on page boundaries
MBMEMINFO         equ  (1 << 1)              ; provide memory map
MBMAGIC           equ 0x1BADB002             ; define the magic number constant
MBFLAGS           equ MBALIGN | MBMEMINFO    ; multiboot flags
MBCHECKSUM        equ  -(MBMAGIC + MBFLAGS)  ; calculate the checksum

;NOTE: always keep this up-to-date with the value in link_high.ld!
KERNEL_VMA        equ 0xc0100000             ; the virtual address of our "high" kernel 
KERNEL_VMA_OFFSET equ 0xc0000000             ; subtracting this from a virtual address gives us the 0-relative offset

section .multiboot
global _mboot
_mboot:
align 4
    dd MBMAGIC
    dd MBFLAGS
    dd MBCHECKSUM
    dd _mboot
._mboot_text:
    dd 0
._mboot_data:
    dd 0 
._mboot_bss:
    dd 0

# kernel startup stack
section .bss
align 16
    stack_bottom:
    resb 16384
    _stack_top:

; this is our boot page directory used to map our high kernel virtual addresses to physical memory.
; the vritual address range starts at KERNEL_VMA and is mapped to offset 1Meg in physical RAM
align 4096
    boot_page_directory:
    resb 4096
    
section .data
extern _k_gdt_desc

section .rodata
; empty

section .text

; void _k_main(uint32_t magic, multiboot_info_t *mboot)
extern _k_main

;uint32_t _k_check_memory_availability(multiboot_info_t* mboot, uint32_t from_phys)
extern  _k_check_memory_availability

_k_phys_memory_available:
    dd 0

; used to store away multiboot arguments
main_args:
    dd  0
    dd  0

; these are defined in linker_high.ld
extern _k_phys_start
extern _k_virtual_start
extern _k_virtual_end
extern _k_phys_end

; -----------------------------------------------------------------------

global _k_frame_alloc_ptr
_k_frame_alloc_ptr:
    dd 0                ; physical address
    dd 0                ; virtual (effectively physical + KERNEL_VMA_OFFSET)

; initialise our simple linear frame allocator by making sure allocations 
; are 4K aligned, starting at the end of the loaded kernel space
_alloc_init:
        push ebx
        push edx
        lea edx, [dword(_k_frame_alloc_ptr - KERNEL_VMA_OFFSET)]
        mov ebx,_k_phys_end
        add ebx,0xfff
        and ebx,~0xfff
        mov [edx],ebx
        add ebx, KERNEL_VMA_OFFSET  ; to virtual
        mov [edx+4], ebx
        pop edx
        pop ebx
        ret

; allocate a 4K frame and return it's physical address in ebx
; also zeros out the contents of the frame. Update allocator.
; ebx => physical frame
_alloc_frame:
        push eax
        push ecx
        push edi
        lea edi, [dword(_k_frame_alloc_ptr - KERNEL_VMA_OFFSET)]
        mov ebx, [edi]
        mov edi, ebx
        
        ; zero 400h dwords of memory
        mov ecx, 0x400
        xor eax,eax
        rep stosd
        ; advance _k_frame_alloc_ptr by one frame
        lea edi, [dword(_k_frame_alloc_ptr - KERNEL_VMA_OFFSET)]
        add dword [edi], 0x1000
        add dword [edi+4], 0x1000
        pop edi
        pop ecx
        pop eax
        ret

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

; eax = virt
; -> ebx = physical address or 0
_virt_to_phys:
        push eax
        push edx
        push ecx

        mov ecx, eax
        shr ecx, 22             ; / 400000h = page table entry index
        
        lea ebx,[dword (boot_page_directory - KERNEL_VMA_OFFSET)]
        shl ecx, 2          ; to byte offset (page table entry)
        add ebx, ecx        ; offset into page directory

        mov ebx, [ebx]      ; page table entry
        test ebx, 1
        jz .invalid

        and ebx, ~0fffh      ; mask out flags to get phys table address
        VIRT_TO_PT_OFFSET eax
        add ebx, eax        
        mov ebx, [ebx]      ; page table entry (frame)
        test ebx, 1
        jnz .done

    .invalid:
        xor ebx,ebx
    .done:
        and ebx, ~0fffh
        pop ecx
        pop edx
        pop eax
        ret

; uintptr_t _pt_insert(uintptr_t pt, uintptr_t virt, uintptr_t phys) -> eax
_pt_insert:
        push ebp
        mov ebp, esp

        push ebx
        push ecx
        push edx

        LOAD_ARG ecx, 1             ; virt
        VIRT_TO_PD_OFFSET ecx
        lea edx,[dword (boot_page_directory - KERNEL_VMA_OFFSET)]
        add edx, ecx
        mov edx, [edx]
        and edx, ~0fffh
        cmp edx, ARG(0)       ; if ( virt_to_pagetable(virt) == pt )
        je .insert

        ; virtual address maps to different page table, either insert a new one or switch to one that's there
        test edx, edx
        jnz .insert
        
        ; need to add new page table
        call _alloc_frame
        or ebx, 1

        lea edx,[dword (boot_page_directory - KERNEL_VMA_OFFSET)]
        add edx, ecx
        mov [edx], ebx       ; insert new page table entry into page directory
        mov edx, ebx
        and edx, ~0fffh     ; edx = address of new pt
        
    .insert:
        mov ebx, edx
        LOAD_ARG ecx, 1         ; virt
        VIRT_TO_PT_OFFSET ecx
        add edx, ecx
        LOAD_ARG ecx, 2         ; phys
        or ecx, 1
        mov [edx], ecx
        mov eax, ebx

        pop edx
        pop ecx
        pop ebx

        pop ebp
        ret

; identity map the BIOS and <1Meg RAM areas [0, 1Meg] -> [0, 1Meg]
_identity_map_low_ram:
        push ebx
        push edx

        call _alloc_frame           ; ebx = page table frame
        xor edx, edx                ; start at 0

    .map_1st_meg:
        push edx
        push edx
        push ebx
        call _pt_insert
        POP_CALL_STACK(3)
        mov ebx, eax    ; active page table returned by _pt_insert
        ; next frame (4K increments) until we've covered the first meg
        add edx, 0x1000
        cmp edx, 0x100000
        jle .map_1st_meg

        pop edx
        pop ebx
        ret

; map the kernel [_k_virtual_start,_k_virtual_end] -> [_k_phys_start, _k_phys_end]
_map_kernel:
        push ebx
        push ecx
        push edx
        
        call _alloc_frame
        
        mov  edx, _k_virtual_start
        mov  ecx, _k_phys_start
    .map_kernel_page:
        push ecx
        push edx
        push ebx
        call _pt_insert
        POP_CALL_STACK(3)
        mov ebx, eax
        add edx, 0x1000
        add ecx, 0x1000
        cmp ecx, _k_phys_end
        jl .map_kernel_page

        pop edx
        pop ecx
        pop ebx
        ret

; -----------------------------------------------------------------------
; 
global _start:function (_start.end - _start)
_start:                
        ; safety check against being loaded in a non-multiboot way
        cmp eax, 2badb002h
        ;TODO: write something to screen
        jne .fi
        
        lea ebp, [dword (_stack_top - KERNEL_VMA_OFFSET)]
        mov esp, ebp

        
        ; the multiboot the machine state is as follows:
        ; https://www.gnu.org/software/grub/manual/multiboot/html_node/Machine-state.html#Machine-state
        cld
        ; store these for later, when we call main
        ; note: ebx is the LMA and will point somewhere in <1Meg RAM
        mov [main_args - KERNEL_VMA_OFFSET], ebx
        mov [(main_args+4) - KERNEL_VMA_OFFSET], eax

        ; get amount of physical memory available in the region inlcuding our kernel and store it for later
        push dword _k_phys_end 
        push ebx
        call _k_check_memory_availability
        add esp, 4*2        
        mov [_k_phys_memory_available-KERNEL_VMA_OFFSET], eax
        ;TODO: check we've got at least some amount available

        ; initialise our physical frame allocator
        call _alloc_init

        ;--------------------------------------
        ; set up the initial page mapping from 0->0
        ; --------------------------------------------
        ; identity map the first meg in boot_page_table_0
        call _identity_map_low_ram

        ; --------------------------------------------
        ; set up the kernel mapping from KERNEL_VMA->0x100000
        ; --------------------------------------------       
        call _map_kernel

        ; --------------------------------------------
        ; add the "self pointer" for the page directory to itself
        ; at industry standard 1024
        ; --------------------------------------------
        lea edi, [dword(boot_page_directory - KERNEL_VMA_OFFSET)]
        mov ebx, 0ffch          ; 1023 * 4 -> virtual address 0xffc00000
        mov edx, edi
        or edx, 0b11            ; present + writable
        mov [edi + ebx], edx    ; point to PD[0]

        ; --------------------------------------------
        ;  switch on paging
        ; --------------------------------------------
        lea ebx, [dword(boot_page_directory - KERNEL_VMA_OFFSET)]
        mov cr3, ebx
        mov ebx, cr0
        or ebx, 0x80000001
        mov cr0, ebx
        jmp dword 10h:.high_half
    .high_half:
        
        ; at this point all kernel virtual addresses are valid through our mappings

        ; reset the stack to the virtual top 
        mov ebp, _stack_top
        mov esp, ebp

        ; switch to our own GDT
        cli
        lgdt [dword _k_gdt_desc]
        mov eax, 10h
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        ; this obviously depends on the new GDTs layout for data segments matching whatever we came from, or the ret won't happen
        mov ss, ax
        jmp dword 08h:.flush_gdt
    .flush_gdt:        
        push dword [main_args]              ; ebx = multiboot pointer
        push dword [main_args+4]            ; eax = multiboot magic number
        call _k_main
        add esp, 2*4
.fi:
        cli
.exit:  hlt
        jmp .exit
.end:


; ====================================================================
; left here for reference; getting a switch to real mode is fiddly.
; it requries code to be located in < 1MB memory so this piece of 
; code has to be copied or loaded there. We're not using this because 
; multiboot provides us with the information we need on boot, eliminating the
; need for making BIOS calls etc.
 
[bits 16]
_k_16_bit_data_start:
    dw 0x3ff		; 256 entries, 4b each = 1K
	dd 0			; Real Mode IVT @ 0x0000
_k_16_bit_data_jmp_offs:
    dw 1234h    
global _16bit_main:function
_16bit_main:
    ;  just need this temporarely
    mov eax, 10h
    mov ds, ax
    mov es, ax
    
    ; load the IVT 
    lidt [500h]

    ; now switch out of protected mode so that we can use the BIOS for some of our initial set up
    mov eax, cr0
    xor eax, 1
    mov cr0, eax
    mov ebx,[506h]
    jmp far ebx
_16bit_main_real_mode:
    mov eax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; top of conventional RAM block, should be enough (see for example https://wiki.osdev.org/Memory_Map_(x86)) or 
    ; https://web.archive.org/web/20130609073242/http://www.osdever.net/tutorials/rm_addressing.php?the_id=50
    mov sp, 0x7bff
    sti

    xor eax,eax
    mov eax, 0e820h
    int 15h

    xchg bx,bx
    
_16bit_main_end:    
    nop
