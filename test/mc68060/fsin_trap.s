
*
* fsin_trap.s — Verify that FSIN traps to F-line on 68060
* FSIN (opmode 0x0E) is not implemented in 060 hardware
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Clear trap indicator
    clr.l   %d7

    * Load a value into FP0
    fmove.l #1, %fp0

    * Execute FSIN fp0,fp0 — should trap on 060
    * Encoding: F200 (coprocessor ID 1, type 0) + 000E (opmode FSIN, src=fp0, dst=fp0)
    .short  0xF200
    .short  0x000E

    * Check that Vector 11 (F-line) was taken
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
