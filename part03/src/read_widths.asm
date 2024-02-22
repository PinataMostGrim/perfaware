; source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0150_read_widths.asm
; Modified to work with the System V ABI

global Read_4x2
global Read_8x2
global Read_16x2
global Read_32x2

section .text


Read_4x2:
    xor rax, rax
    align 64
.loop:
    mov r8d, [rsi ]
    mov r8d, [rsi + 4]
    add rax, 8
    cmp rax, rdi
    jb .loop
    ret

Read_8x2:
    xor rax, rax
    align 64
.loop:
    mov r8, [rsi ]
    mov r8, [rsi + 8]
    add rax, 16
    cmp rax, rdi
    jb .loop
    ret

Read_16x2:
    xor rax, rax
    align 64
.loop:
    vmovdqu xmm0, [rsi]
    vmovdqu xmm1, [rsi + 16]
    add rax, 32
    cmp rax, rdi
    jb .loop
    ret

Read_32x2:
    xor rax, rax
    align 64
.loop:
    vmovdqu ymm0, [rsi]
    vmovdqu ymm1, [rsi + 32]
    add rax, 64
    cmp rax, rdi
    jb .loop
    ret
