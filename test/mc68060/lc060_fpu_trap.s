
*
* lc060_fpu_trap.s — Verify that ALL FPU instructions trap on LC060
* The LC060 has no FPU at all, so even hardware ops (FADD, FMOVE, etc.)
* must trap to F-line.
*
* Run with: ./test_driver --cpu=68lc060 test/mc68060/lc060_fpu_trap.bin
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * On LC060, even FMOVE.L should trap (no FPU present)
    clr.l   %d7
    fmove.l #1, %fp0
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    * FADD should also trap
    clr.l   %d7
    .short  0xF200
    .short  (1 << 10) | (0 << 7) | 0x22
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    * Even FMOVE reg-to-reg should trap
    clr.l   %d7
    .short  0xF200
    .short  (1 << 10) | (0 << 7) | 0x00
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
