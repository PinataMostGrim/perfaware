; Written for the System-V ABI

global Load_32x2

section .text


Load_32x2:
    ; parameters:
    ; rdi - byte count to read per-loop
    ; rsi - pointer to buffer address
    ; rdx - loop repeat count
    ; rcx - stride, in bytes

    ; internal register use:
    ; rax       - effective address pointer
    ; rbx       - inner loop byte counter

    align 64

.outer_loop:
    mov rax, rsi    ; reset pointer to buffer address
    mov rbx, rdi    ; reset loop byte counter

.inner_loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 0x20]

    add rax, rcx        ; increment the effective address pointer
    dec rbx             ; decrement the loop byte counter

    ja .inner_loop
    ; end .inner_loop

    dec rdx             ; decrement the loop repeat counter
    ja .outer_loop
    ; end .outer_loop

    ret
