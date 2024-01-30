global Write_x1
global Write_x2
global Write_x3
global Write_x4

section .text


Write_x1:
    align 64
.loop:
    mov [rsi], rax
    sub rdi, 1
    jnle .loop
    ret

Write_x2:
    align 64
.loop:
    mov [rsi], rax
    mov [rsi], rax
    sub rdi, 2
    jnle .loop
    ret

Write_x3:
    align 64
.loop:
    mov [rsi], rax
    mov [rsi], rax
    mov [rsi], rax
    sub rdi, 3
    jnle .loop
    ret

Write_x4:
    align 64
.loop:
    mov [rsi], rax
    mov [rsi], rax
    mov [rsi], rax
    mov [rsi], rax
    sub rdi, 4
    jnle .loop
    ret
