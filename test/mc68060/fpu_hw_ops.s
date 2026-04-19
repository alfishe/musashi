
*
* fpu_hw_ops.s — Verify all 060 hardware FPU ops execute without trapping
*
* Tests: FMOVE, FINT, FINTRZ, FSQRT, FABS, FNEG, FADD, FSUB, FMUL, FDIV,
*        FCMP, FTST
*
* word2 for reg-to-reg, src=fp1(001), dst=fp0(000):
*   word2 = 0x0400 | opmode
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

* Macro: execute an FPU opmode and verify no trap
.macro TEST_HW_OP word2_val
    clr.l    %d7
    .short   0xF200
    .short   \word2_val
    tst.l    %d7
    bne      TEST_FAIL
.endm

    * Load test values
    fmove.l  #10, %fp0
    fmove.l  #3, %fp1

    * FMOVE (0x00) — fp0 = fp1 = 3
    TEST_HW_OP 0x0400

    * Verify: fp0 should be 3 (copied from fp1)
    fmove.l  %fp0, %d0
    cmpi.l   #3, %d0
    bne      TEST_FAIL

    * Reload
    fmove.l  #10, %fp0
    fmove.l  #3, %fp1

    * FADD (0x22) — fp0 += fp1 -> 10+3=13
    TEST_HW_OP 0x0422
    fmove.l  %fp0, %d0
    cmpi.l   #13, %d0
    bne      TEST_FAIL

    * Reload
    fmove.l  #10, %fp0
    fmove.l  #3, %fp1

    * FSUB (0x28) — fp0 -= fp1 -> 10-3=7
    TEST_HW_OP 0x0428
    fmove.l  %fp0, %d0
    cmpi.l   #7, %d0
    bne      TEST_FAIL

    * Reload
    fmove.l  #10, %fp0
    fmove.l  #3, %fp1

    * FMUL (0x23) — fp0 *= fp1 -> 10*3=30
    TEST_HW_OP 0x0423
    fmove.l  %fp0, %d0
    cmpi.l   #30, %d0
    bne      TEST_FAIL

    * Reload
    fmove.l  #12, %fp0
    fmove.l  #3, %fp1

    * FDIV (0x20) — fp0 = fp0/fp1 = 12/3=4
    TEST_HW_OP 0x0420
    fmove.l  %fp0, %d0
    cmpi.l   #4, %d0
    bne      TEST_FAIL

    * FNEG (0x1A) — fp0 = -fp1
    fmove.l  #5, %fp1
    TEST_HW_OP 0x041A
    fmove.l  %fp0, %d0
    cmpi.l   #-5, %d0
    bne      TEST_FAIL

    * FABS (0x18) — fp0 = |fp1|
    fmove.l  #-7, %fp1
    TEST_HW_OP 0x0418
    fmove.l  %fp0, %d0
    cmpi.l   #7, %d0
    bne      TEST_FAIL

    * FSQRT (0x04) — fp0 = sqrt(fp1)
    fmove.l  #16, %fp1
    TEST_HW_OP 0x0404
    fmove.l  %fp0, %d0
    cmpi.l   #4, %d0
    bne      TEST_FAIL

    * FCMP (0x38) — compare only, no data stored
    fmove.l  #5, %fp0
    fmove.l  #5, %fp1
    TEST_HW_OP 0x0438

    * FTST (0x3A) — test fp1, set condition codes
    fmove.l  #1, %fp1
    TEST_HW_OP 0x043A

    * FINT (0x01) — fp0 = round(fp1)
    fmove.l  #3, %fp1
    TEST_HW_OP 0x0401
    fmove.l  %fp0, %d0
    cmpi.l   #3, %d0
    bne      TEST_FAIL

    * FINTRZ (0x03) — fp0 = trunc(fp1)
    fmove.l  #7, %fp1
    TEST_HW_OP 0x0403
    fmove.l  %fp0, %d0
    cmpi.l   #7, %d0
    bne      TEST_FAIL

    rts
