; source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0161_prefetching.asm
; Written for the System-V ABI

global PeriodicRead
global PeriodicPrefetchedRead

section .text

PeriodicRead:
    ; parameters:
    ; rcx -> rdi: outer loop count
    ; rdx -> rsi: source pointer
    ; r8 -> rdx: inner loop count

    align 64

.outer_loop:
    vmovdqa ymm0, [rsi]         ; Load the cache line as "data"
    vmovdqa ymm1, [rsi + 0x20]  ;

    mov rsi, [rsi]              ; Load the next block pointer out of the current block
    mov r10, rdx                ; Reset the inner loop counter

.inner_loop:                    ; Do a loop of pretend operations on the "data"
    vpxor ymm0, ymm1
    vpaddd ymm0, ymm1
    dec r10
    jnz .inner_loop

    dec rdi
    jnz .outer_loop
    ret

PeriodicPrefetchedRead:
    align 64

.outer_loop:
    vmovdqa ymm0, [rsi]         ; Load the cache line as "data"
    vmovdqa ymm1, [rsi + 0x20]  ;

    mov rsi, [rsi]              ; Load the next block pointer out of the current block
    mov r10, rdx                ; Reset the inner loop counter

    prefetcht0 [rdx]            ; Start prefetching the next cache line while we work on this one

.inner_loop:
    vpxor ymm0, ymm1
    vpaddd ymm0, ymm1
    dec r10
    jnz .inner_loop

    dec rdi

    jnz .outer_loop
    ret
