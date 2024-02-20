; source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0136_conditional_nop_loops.asm
; Modified to work with the System V ABI

global ConditionalNOP

section .text

ConditionalNOP:
    xor rax, rax
.loop:
    mov r10, [rsi + rax]
    inc rax
    test r10, 1
    jnz .skip
    nop
.skip:
    cmp rax, rdi
    jb .loop
    ret
