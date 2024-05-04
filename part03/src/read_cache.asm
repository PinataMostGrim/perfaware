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


;
; -----------------------------
Read_32x4:

    ; parameters:
    ; rdi - bytes count to read
    ; rsi - mask
    ; rdx - pointer to buffer address

    ; internal register use:
    ; rax - effective pointer
    ; r10 - bytes read counter
    ; r11 - base pointer offset

    ; minimum read span: 128 bytes (32x4)

    xor rax, rax
    xor r10, r10
    xor r11, r11

    align 64

.loop:
    mov rax, rdx    ; reset rax to the base pointer value
    add rax, r11    ; add base pointer to the base pointer offset

    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 32]
    vmovdqu ymm2, [rax + 64]
    vmovdqu ymm3, [rax + 96]

    add r10, 128    ; increment the bytes read counter
    mov r11, r10    ; prepare the base pointer offset
    and r11, rsi    ; AND the pointer offset with the mask to produce the effective pointer offset

    cmp r10, rdi
    jb .loop

    ret


;
; -----------------------------
Read_32x8:

    ; parameters:
    ; rdi - bytes count to read
    ; rsi - mask
    ; rdx - pointer to buffer address

    ; internal register use:
    ; rax - effective pointer
    ; r10 - bytes read counter
    ; r11 - base pointer offset

    ; minimum read span: 256 bytes (32x8)

    xor rax, rax
    xor r10, r10
    xor r11, r11

    align 64

.loop:
    mov rax, rdx    ; reset rax to the base pointer value
    add rax, r11    ; add base pointer to the base pointer offset

    vmovdqu ymm0, [rax]
    vmovdqu ymm1, [rax + 32]
    vmovdqu ymm2, [rax + 64]
    vmovdqu ymm3, [rax + 96]
    vmovdqu ymm4, [rax + 128]
    vmovdqu ymm5, [rax + 160]
    vmovdqu ymm6, [rax + 192]
    vmovdqu ymm7, [rax + 224]

    add r10, 256   ; increment the bytes read counter
    mov r11, r10    ; prepare the base pointer offset
    and r11, rsi    ; AND the pointer offset with the mask to produce the effective pointer offset

    cmp r10, rdi
    jb .loop

    ret


;
; -----------------------------
Read_32x16:

    ; parameters:
    ; rdi - bytes count to read
    ; rsi - mask
    ; rdx - pointer to buffer address

    ; internal register use:
    ; rax - effective pointer
    ; r10 - bytes read counter
    ; r11 - base pointer offset

    ; minimum read span: 512 bytes (32x16)

    xor rax, rax
    xor r10, r10
    xor r11, r11

    align 64

.loop:
    mov rax, rdx    ; reset rax to the base pointer value
    add rax, r11    ; add base pointer to the base pointer offset

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

    add r10, 512   ; increment the bytes read counter
    mov r11, r10    ; prepare the base pointer offset
    and r11, rsi    ; AND the pointer offset with the mask to produce the effective pointer offset

    cmp r10, rdi
    jb .loop

    ret
