
*
* fpu_rounding_ops.s — Verify single/double rounding FPU variants execute on 060
* Tests FSADD, FDADD, FSMUL, FDMUL, FSSUB, FDSUB, FSDIV, FDDIV,
*       FSSQRT, FDSQRT, FSMOVE, FDMOVE, FSABS, FDABS, FSNEG, FDNEG
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Load test values
    fmove.l #100, %fp0
    fmove.l #3, %fp1

    * --- FSADD (0x62) — single-precision FADD ---
    clr.l   %d7
    .short  0xF200              | FPU op, src=FP reg
    .short  0x0462              | FP1 -> FP0, opmode=0x62 (FSADD)
    tst.l   %d7                 | No trap
    bne     TEST_FAIL

    * --- FDADD (0x66) — double-precision FADD ---
    clr.l   %d7
    fmove.l #100, %fp0
    .short  0xF200
    .short  0x0466              | FP1 -> FP0, opmode=0x66 (FDADD)
    tst.l   %d7
    bne     TEST_FAIL

    * --- FSMUL (0x63) ---
    clr.l   %d7
    fmove.l #10, %fp0
    .short  0xF200
    .short  0x0463              | FP1 -> FP0, opmode=0x63 (FSMUL)
    tst.l   %d7
    bne     TEST_FAIL

    * --- FDMUL (0x67) ---
    clr.l   %d7
    fmove.l #10, %fp0
    .short  0xF200
    .short  0x0467              | FDMUL
    tst.l   %d7
    bne     TEST_FAIL

    * --- FSSUB (0x68) ---
    clr.l   %d7
    fmove.l #100, %fp0
    .short  0xF200
    .short  0x0468              | FSSUB
    tst.l   %d7
    bne     TEST_FAIL

    * --- FDSUB (0x6C) ---
    clr.l   %d7
    fmove.l #100, %fp0
    .short  0xF200
    .short  0x046C              | FDSUB
    tst.l   %d7
    bne     TEST_FAIL

    * --- FSDIV (0x60) ---
    clr.l   %d7
    fmove.l #100, %fp0
    .short  0xF200
    .short  0x0460              | FSDIV
    tst.l   %d7
    bne     TEST_FAIL

    * --- FDDIV (0x64) ---
    clr.l   %d7
    fmove.l #100, %fp0
    .short  0xF200
    .short  0x0464              | FDDIV
    tst.l   %d7
    bne     TEST_FAIL

    * --- FSSQRT (0x41) ---
    clr.l   %d7
    fmove.l #16, %fp0
    .short  0xF200
    .short  0x0041              | FP0 -> FP0, FSSQRT
    tst.l   %d7
    bne     TEST_FAIL

    * --- FDSQRT (0x45) ---
    clr.l   %d7
    fmove.l #16, %fp0
    .short  0xF200
    .short  0x0045              | FDSQRT
    tst.l   %d7
    bne     TEST_FAIL

    * --- FSMOVE (0x40) ---
    clr.l   %d7
    .short  0xF200
    .short  0x0440              | FP1 -> FP0, FSMOVE
    tst.l   %d7
    bne     TEST_FAIL

    * --- FDMOVE (0x44) ---
    clr.l   %d7
    .short  0xF200
    .short  0x0444              | FDMOVE
    tst.l   %d7
    bne     TEST_FAIL

    * --- FSABS (0x58) ---
    clr.l   %d7
    fmove.l #-42, %fp0
    .short  0xF200
    .short  0x0058              | FP0 -> FP0, FSABS
    tst.l   %d7
    bne     TEST_FAIL

    * --- FDABS (0x5C) ---
    clr.l   %d7
    fmove.l #-42, %fp0
    .short  0xF200
    .short  0x005C              | FDABS
    tst.l   %d7
    bne     TEST_FAIL

    * --- FSNEG (0x5A) ---
    clr.l   %d7
    fmove.l #42, %fp0
    .short  0xF200
    .short  0x005A              | FSNEG
    tst.l   %d7
    bne     TEST_FAIL

    * --- FDNEG (0x5E) ---
    clr.l   %d7
    fmove.l #42, %fp0
    .short  0xF200
    .short  0x005E              | FDNEG
    tst.l   %d7
    bne     TEST_FAIL

    rts
