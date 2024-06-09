; Written for the System-V ABI

global Load_32x8
global Load_32x8a
global Load_32x4b
global Load_32x4c
global Load_32x4d
global Load_32x2

section .text


; Original load function, offset is hard-coded to 32 bytes
;-----------------------
Load_32x8:
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


; Hard-coded offsets, first attempt
;-----------------------
Load_32x8a:
    ; parameters:
    ; rdi - byte count to read per-loop
    ; rsi - pointer to buffer address
    ; rdx - loop repeat count
    ; rcx - load offset, bytes

    ; internal register use:
    ; rax - effective address pointer
    ; r10 - inner loop byte counter

    ; minimum read span: 256 bytes (32x8)

    align 64

.outer_loop:
    mov rax, rsi    ; reset pointer to buffer address
    mov r10, rdi    ; reset loop byte counter

.inner_loop:
    ; TODO: Hard-coding this to load offsets of 512 for now, until I can figure out the best way to use RCX
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 512]
    vmovdqu ymm2, [rax + 1024]
    vmovdqu ymm3, [rax + 1536]
    vmovdqu ymm4, [rax + 2048]
    vmovdqu ymm5, [rax + 2560]
    vmovdqu ymm6, [rax + 3072]
    vmovdqu ymm7, [rax + 3584]

    ; TODO: The add also needs to be incremented by the total number of bytes we spanned
    add rax, 4096    ; increment the effective address pointer
    sub r10, 256    ; decrement the loop byte counter

    ja .inner_loop
    ; end .inner_loop

    sub rdx, 1      ; decrement the loop repeat counter
    ja .outer_loop
    ; end .outer_loop

    ret


; Dynamic offset, first attempt
;-----------------------
Load_32x4b:
    ; parameters:
    ; rdi - byte count to read per-loop
    ; rsi - pointer to buffer address
    ; rdx - loop repeat count
    ; rcx - load offset, bytes

    ; internal register use:
    ; rax - effective address pointer
    ; r10 - inner loop byte counter
    ; r11- r14 - dynamic load offsets

    ; minimum read span: 128 bytes (32x4)

    ; MUL will clobber RDX, so we have to save it first
    mov r10, rdx

    mov rax, rcx
    mov rbx, 1
    mul rbx
    mov r11, rax

    mov rax, rcx
    mov rbx, 2
    mul rbx
    mov r12, rax

    mov rax, rcx
    mov rbx, 3
    mul rbx
    mov r13, rax

    mov rax, rcx
    mov rbx, 4
    mul rbx
    mov r14, rax

    ; Restore rdx
    mov rdx, r10

    align 64

.outer_loop:
    mov rax, rsi    ; reset pointer to buffer address
    mov r10, rdi    ; reset loop byte counter

.inner_loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + r11]
    vmovdqu ymm2, [rax + r12]
    vmovdqu ymm3, [rax + r13]

    add rax, r14    ; increment the effective address pointer
    sub r10, 128    ; decrement the loop byte counter

    ja .inner_loop
    ; end .inner_loop

    sub rdx, 1      ; decrement the loop repeat counter
    ja .outer_loop
    ; end .outer_loop

    ret


; Dynamic offset, second attempt, use LEA instruction instead of more registers
;-----------------------
Load_32x4c:
    ; parameters:
    ; rdi - byte count to read per-loop
    ; rsi - pointer to buffer address
    ; rdx - loop repeat count
    ; rcx - load offset, bytes

    ; internal register use:
    ; rax - effective address pointer
    ; r10 - inner loop byte counter
    ; r11 - intermediate value for lea effective address calculation

    ; minimum read span: 128 bytes (32x4)

    align 64

.outer_loop:
    mov rax, rsi    ; reset pointer to buffer address
    mov r10, rdi    ; reset loop byte counter

.inner_loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + rcx * 1]
    vmovdqu ymm2, [rax + rcx * 2]
    ; The extra LEA is needed to circumvent NASMs power of two scaling factor limitation,
    ; even though the AMD x64 specification allows scaling factors of 3.
    lea r11, [rcx + rcx * 2]
    vmovdqu ymm3, [rax + r11]

    lea rax, [rax + rcx * 4]    ; increment the effective address pointer
    sub r10, 128    ; decrement the loop byte counter

    ja .inner_loop
    ; end .inner_loop

    sub rdx, 1      ; decrement the loop repeat counter
    ja .outer_loop
    ; end .outer_loop

    ret


; Dynamic offset, third attempt, use LEA instructions outside of the loop
;-----------------------
Load_32x4d:
    ; parameters:
    ; rdi - byte count to read per-loop
    ; rsi - pointer to buffer address
    ; rdx - loop repeat count
    ; rcx - load offset, bytes

    ; internal register use:
    ; rax       - effective address pointer
    ; rbx       - inner loop byte counter
    ; r8-r11    - dynamic load offsets

    ; minimum read span: 128 bytes (32x4)

    mov r8, rcx                 ; rcx * 1
    lea r9, [rcx + rcx]         ; rcx * 2
    lea r10, [rcx + rcx * 2]    ; rcx * 3
    lea r11, [rcx * 4]          ; rcx * 4

    align 64

.outer_loop:
    mov rax, rsi    ; reset pointer to buffer address
    mov rbx, rdi    ; reset loop byte counter

.inner_loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + r8]
    vmovdqu ymm2, [rax + r9]
    vmovdqu ymm3, [rax + r10]

    add rax, r11    ; increment the effective address pointer
    sub rbx, 128    ; decrement the loop byte counter

    ja .inner_loop
    ; end .inner_loop

    sub rdx, 1      ; decrement the loop repeat counter
    ja .outer_loop
    ; end .outer_loop

    ret


;
;-----------------------
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
