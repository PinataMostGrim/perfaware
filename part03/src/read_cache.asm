;

global Read_32x4
global Read_32x8
global Read_32x16

section .text

; Reference
; -----------------------------
; Read_32x2:
;     xor rax, rax
;     align 64
; .loop:
;     vmovdqu ymm0, [rsi]
;     vmovdqu ymm1, [rsi + 32]
;     add rax, 64
;     cmp rax, rdi
;     jb .loop
;     ret


; -----------------------------
Read_32x4:
; -----------------------------

    ; L1 cache speeds can be fast enough such that this function is artificially
    ; limited by the dependency chains created by address calculation and its use
    ; is not recommended.

    ; parameters:
    ; rdi - total byte count to read
    ; rsi - pointer to buffer address
    ; rdx - address mask

    ; internal register use:
    ; rax - effective pointer
    ; r10 - base pointer offset

    ; minimum read span: 128 bytes (32x4)

    xor r10, r10    ; clear base pointer offset
    mov rax, rsi    ; reset rax to the base pointer value

    align 64

.loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 32]
    vmovdqu ymm2, [rax + 64]
    vmovdqu ymm3, [rax + 96]

    add r10, 128    ; increment the pointer offset
    and r10, rdx    ; AND the pointer offset with the mask to produce the effective pointer offset

    mov rax, rsi    ; reset rax to the base pointer value
    add rax, r10    ; add the effective pointer offset to the base pointer

    sub rdi, 128
    ja .loop

    ret


; -----------------------------
Read_32x8:
; -----------------------------

    ; parameters:
    ; rdi - total byte count to read
    ; rsi - pointer to buffer address
    ; rdx - address mask

    ; internal register use:
    ; rax - effective pointer
    ; r10 - base pointer offset

    ; minimum read span: 256 bytes (32x8)

    xor r10, r10    ; clear base pointer offset
    mov rax, rsi   ; reset rax to the base pointer value

    align 64

.loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 32]
    vmovdqu ymm2, [rax + 64]
    vmovdqu ymm3, [rax + 96]
    vmovdqu ymm4, [rax + 128]
    vmovdqu ymm5, [rax + 160]
    vmovdqu ymm6, [rax + 192]
    vmovdqu ymm7, [rax + 224]

    add r10, 256    ; increment the pointer offset
    and r10, rdx    ; AND the pointer offset with the mask to produce the effective pointer offset

    mov rax, rsi    ; reset rax to the base pointer value
    add rax, r10    ; add the effective pointer offset to the base pointer

    sub rdi, 256
    ja .loop

    ret


;
; -----------------------------
Read_32x16:

    ; parameters:
    ; rdi - bytes count to read
    ; rsi - pointer to buffer address
    ; rdx - address mask

    ; internal register use:
    ; rax - effective pointer
    ; r10 - base pointer offset

    ; minimum read span: 512 bytes (32x16)

    xor r10, r10    ; clear base pointer offset
    mov rax, rsi   ; reset rax to the base pointer value

    align 64

.loop:
    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 32]
    vmovdqu ymm2, [rax + 64]
    vmovdqu ymm3, [rax + 96]
    vmovdqu ymm4, [rax + 128]
    vmovdqu ymm5, [rax + 160]
    vmovdqu ymm6, [rax + 192]
    vmovdqu ymm7, [rax + 224]
    vmovdqu ymm8, [rax + 256]
    vmovdqu ymm9, [rax + 288]
    vmovdqu ymm10, [rax + 320]
    vmovdqu ymm11, [rax + 352]
    vmovdqu ymm12, [rax + 384]
    vmovdqu ymm13, [rax + 416]
    vmovdqu ymm14, [rax + 448]
    vmovdqu ymm15, [rax + 480]

    add r10, 512    ; increment the bytes read counter
    and r10, rdx    ; AND the pointer offset with the mask to produce the effective pointer offset

    mov rax, rsi    ; reset rax to the base pointer value
    add rax, r10    ; add the effective pointer offset to the base pointer

    sub rdi, 512
    ja .loop

    ret
