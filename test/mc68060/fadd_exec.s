
*
* fadd_exec.s — Verify that FADD executes natively on 68060
* FADD (opmode 0x22) IS implemented in 060 hardware
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Clear trap indicator
    clr.l   %d7

    * Load value into FP0
    fmove.l #3, %fp0

    * Load value into FP1
    fmove.l #4, %fp1

    * Execute FADD.X FP1,FP0 — should NOT trap on 060
    fadd.x  %fp1, %fp0

    * Verify no trap occurred
    tst.l   %d7
    bne     TEST_FAIL

    * Verify result: FP0 should be 7
    fmove.l %fp0, %d0
    cmpi.l  #7, %d0
    bne     TEST_FAIL

    rts
