; source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0132_nop_loop.asm
; Modified to work with the System V ABI

global MOVAllBytesASM
global NOPAllBytesASM
global CMPAllBytesASM
global DECAllBytesASM

section .text


MOVAllBytesASM:
    xor rax, rax
.loop:
    mov [rsi + rax], al
    inc rax
    cmp rax, rdi
    jb .loop
    ret


NOPAllBytesASM:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00 ; This is the byte sequence for a 3-byte NOP
    inc rax
    cmp rax, rdi
    jb .loop
    ret


CMPAllBytesASM:
    xor rax, rax
.loop:
    inc rax
    cmp rax, rdi
    jb .loop
    ret


DECAllBytesASM:
.loop:
    dec rdi
    jnz .loop
    ret
