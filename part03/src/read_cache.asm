; Written for the System-V ABI

global Read_32x8

section .text

Read_32x8:
    ; parameters:
    ; rdi - byte count to read per-loop
    ; rsi - pointer to buffer address
    ; rdx - loop repeat count

    ; internal register use:
    ; rax - effective address pointer
    ; r10 - inner loop byte counter

    ; minimum read span: 256 bytes (32x8)

    xor r10, r10    ; zero out inner loop byte counter

    align 64

.outer_loop:
    mov rax, rsi    ; reset pointer to buffer address
    mov r10, rdi    ; reset loop byte counter

.inner_loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 32]
    vmovdqu ymm2, [rax + 64]
    vmovdqu ymm3, [rax + 96]
    vmovdqu ymm4, [rax + 128]
    vmovdqu ymm5, [rax + 160]
    vmovdqu ymm6, [rax + 192]
    vmovdqu ymm7, [rax + 224]

    add rax, 256    ; increment the effective address pointer
    sub r10, 256    ; decrement the loop byte counter

    ja .inner_loop
    ; end .inner_loop

    sub rdx, 1      ; decrement the loop repeat counter
    ja .outer_loop
    ; end .outer_loop

    ret
