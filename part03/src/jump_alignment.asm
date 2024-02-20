; source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0139_jump_alignment.asm
; Modified to work with the System V ABI

global NOPAligned64
global NOPAligned1
global NOPAligned15
global NOPAligned31
global NOPAligned63

section .text

NOPAligned64:
    xor rax, rax
align 64
.loop:
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOPAligned1:
    xor rax, rax
align 64
nop
.loop:
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOPAligned15:
    xor rax, rax
align 64
%rep 15
nop
%endrep
.loop:
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOPAligned31:
    xor rax, rax
align 64
%rep 31
nop
%endrep
.loop:
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOPAligned63:
    xor rax, rax
align 64
%rep 63
nop
%endrep
.loop:
    inc rax
    cmp rax, rdi
    jb .loop
    ret
