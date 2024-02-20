; source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0134_multinop_loops.asm
; Modified to work with the System V ABI

global NOP3x1AllBytes
global NOP1x3AllBytes
global NOP1x9AllBytes

section .text

NOP3x1AllBytes:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00 ; This is the byte sequence for a 3-byte NOP
    inc rax
    cmp rax, rdi
    jb .loop
    ret


NOP1x3AllBytes:
    xor rax, rax
.loop:
    nop
    nop
    nop
    inc rax
    cmp rax, rdi
    jb .loop
    ret


NOP1x9AllBytes:
    xor rax, rax
.loop:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    inc rax
    cmp rax, rdi
    jb .loop
    ret
