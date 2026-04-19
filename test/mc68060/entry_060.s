
*
* entry_060.s — 68060 test helpers
*
* Include AFTER entry.s. Code execution jumps over the handlers
* directly to the test body that follows this include.
*

* Jump over handler code to the test body
    bra.w   _060_test_body

* -------------------------------------------------------------------
* Constants for 060-specific exception vectors
* -------------------------------------------------------------------

.set VEC_ILLEGAL,    0x10      | Vector  4: Illegal Instruction (4*4)
.set VEC_FLINE,      0x2C      | Vector 11: F-Line (11*4)
.set VEC_FP_UNIMPL,  0xF0      | Vector 60: Unimplemented FP instruction (60*4)
.set VEC_UNIMPL_INT, 0xF4      | Vector 61: Unimplemented Integer (61*4)

* -------------------------------------------------------------------
* setup_060_vectors — call from test body before any trap testing
* -------------------------------------------------------------------

setup_060_vectors:
    mov.l #HANDLER_ILLEGAL,    VEC_ILLEGAL
    mov.l #HANDLER_FLINE,      VEC_FLINE
    mov.l #HANDLER_FP_UNIMPL,  VEC_FP_UNIMPL
    mov.l #HANDLER_UNIMPL_INT, VEC_UNIMPL_INT
    rts

* -------------------------------------------------------------------
* Exception Handlers
*
* The stacked PC points to the start of the trapping instruction.
* We must advance it past the instruction to avoid an infinite trap loop.
*
* For F-line FPU instructions, the minimum size is 4 bytes (2-word encoding).
* For MOVEP, the size is 4 bytes (2-word encoding).
* For simplicity, we advance by 4 bytes. Tests that use longer encodings
* should adjust the stacked exception frame or use the simple encodings.
*
* Exception frame (Format $0, 010+):
*   SP+0: SR (16 bits)
*   SP+2: PC (32 bits, points to trapping instruction)
*   SP+6: Format|Vector (16 bits)
*
* We advance PC by 4 to skip past the trapping instruction.
* -------------------------------------------------------------------

HANDLER_ILLEGAL:
    mov.l  #0xEEEE0004, %d7
    mov.l  2(%sp), %a6          | Save original PC for verification
    addq.l #4, 2(%sp)           | Advance stacked PC past instruction
    rte

HANDLER_FLINE:
    mov.l  #0xEEEE000B, %d7
    mov.l  2(%sp), %a6
    addq.l #4, 2(%sp)
    rte

HANDLER_FP_UNIMPL:
    mov.l  #0xEEEE003C, %d7
    mov.l  2(%sp), %a6
    addq.l #4, 2(%sp)
    rte

HANDLER_UNIMPL_INT:
    mov.l  #0xEEEE003D, %d7
    mov.l  2(%sp), %a6
    addq.l #4, 2(%sp)
    rte

_060_test_body:
