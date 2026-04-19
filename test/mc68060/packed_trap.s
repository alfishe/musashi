
*
* packed_trap.s — Verify packed decimal FPU ops trap on 060
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Load a value into FP0
    fmove.l #42, %fp0

    * FMOVE.P FP0,(A0) — packed decimal store, must trap
    * Encoding: F210 7C00 (move to EA, packed decimal, FP0, k-factor=0)
    * F-line + coprocessor: F2xx, EA = (A0)
    clr.l   %d7
    lea.l   0x300000, %a0
    .short  0xF210              | F-line, EA=(A0)
    .short  0x7C00              | FMOVE to EA, packed decimal (src=3), FP0
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
