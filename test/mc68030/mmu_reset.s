*
* mmu_reset.s — Verify 68030 MMU registers are zero after reset
* Tests TC, TT0, TT1, CRP, SRP
*

.include "entry_030_mmu.s"

op_MMU_RESET:

    .set TC_DATA, 0x300100
    .set CRP_DATA, 0x300108
    .set SRP_DATA, 0x300110
    .set TT0_DATA, 0x300118
    .set TT1_DATA, 0x30011C

    * Read TC - should be 0 after reset
    mov.l   #0xFFFFFFFF, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4200                  | PMOVE from TC
    mov.l   TC_DATA, %d0
    tst.l   %d0
    bne     TEST_FAIL

    * Read CRP - both words should be 0 after reset
    mov.l   #0xFFFFFFFF, CRP_DATA
    mov.l   #0xFFFFFFFF, CRP_DATA+4
    lea     CRP_DATA, %a0
    .word   0xF010
    .word   0x4E00                  | PMOVE from CRP
    mov.l   CRP_DATA, %d0
    tst.l   %d0
    bne     TEST_FAIL
    mov.l   CRP_DATA+4, %d0
    tst.l   %d0
    bne     TEST_FAIL

    * Read SRP - both words should be 0 after reset
    mov.l   #0xFFFFFFFF, SRP_DATA
    mov.l   #0xFFFFFFFF, SRP_DATA+4
    lea     SRP_DATA, %a0
    .word   0xF010
    .word   0x4A00                  | PMOVE from SRP
    mov.l   SRP_DATA, %d0
    tst.l   %d0
    bne     TEST_FAIL
    mov.l   SRP_DATA+4, %d0
    tst.l   %d0
    bne     TEST_FAIL

    * Read TT0 - should be 0 after reset
    mov.l   #0xFFFFFFFF, TT0_DATA
    lea     TT0_DATA, %a0
    .word   0xF010
    .word   0x0A00                  | PMOVE from TT0
    mov.l   TT0_DATA, %d0
    tst.l   %d0
    bne     TEST_FAIL

    * Read TT1 - should be 0 after reset
    mov.l   #0xFFFFFFFF, TT1_DATA
    lea     TT1_DATA, %a0
    .word   0xF010
    .word   0x0E00                  | PMOVE from TT1
    mov.l   TT1_DATA, %d0
    tst.l   %d0
    bne     TEST_FAIL

    rts
