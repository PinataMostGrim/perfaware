; source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0144_read_unroll.asm
; Modified to work with the System V ABI

global Read_x1
global Read_x2
global Read_x3
global Read_x4

section .text


Read_x1:
    align 64
.loop:
    mov rax, [rsi]
    sub rdi, 1
    jnle .loop
    ret

Read_x2:
    align 64
.loop:
    mov rax, [rsi]
    mov rax, [rsi]
    sub rdi, 2
    jnle .loop
    ret

Read_x3:
    align 64
.loop:
    mov rax, [rsi]
    mov rax, [rsi]
    mov rax, [rsi]
    sub rdi, 3
    jnle .loop
    ret

Read_x4:
    align 64
.loop:
    mov rax, [rsi]
    mov rax, [rsi]
    mov rax, [rsi]
    mov rax, [rsi]
    sub rdi, 4
    jnle .loop
    ret
