.include "entry_030_mmu.s"
* OPCODE: PMOVE TC
* Test PMOVE to/from Translation Control register
* NOTE: Does NOT enable MMU (bit 31 = 0) to avoid translation issues

op_PMOVE_TC:

    .set TC_DATA, 0x300100
    .set CRP_DATA, 0x300108
    .set SRP_DATA, 0x300110

    * Test 1: Write TC without enable bit, read back
    mov.l   #0x00C87500, TC_DATA    | PS=12, IS=8, TIA=7, TIB=5 but E=0
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000                  | PMOVE to TC

    * Read TC back
    clr.l   TC_DATA
    .word   0xF010
    .word   0x4200                  | PMOVE from TC

    mov.l   TC_DATA, %d0
    cmpi.l  #0x00C87500, %d0
    bne     TEST_FAIL

    * Test 2: Write CRP, read back
    mov.l   #0x80000002, CRP_DATA      | Limit, Type=2
    mov.l   #0x00300000, CRP_DATA+4    | Base address

    lea     CRP_DATA, %a0
    .word   0xF010
    .word   0x4C00                  | PMOVE to CRP

    * Read CRP back
    clr.l   CRP_DATA
    clr.l   CRP_DATA+4
    .word   0xF010
    .word   0x4E00                  | PMOVE from CRP

    mov.l   CRP_DATA, %d0
    cmpi.l  #0x80000002, %d0
    bne     TEST_FAIL

    mov.l   CRP_DATA+4, %d0
    cmpi.l  #0x00300000, %d0
    bne     TEST_FAIL

    * Test 3: Write SRP, read back
    mov.l   #0x80000003, SRP_DATA      | Limit, Type=3
    mov.l   #0x00301000, SRP_DATA+4    | Base address

    lea     SRP_DATA, %a0
    .word   0xF010
    .word   0x4800                  | PMOVE to SRP

    * Read SRP back
    clr.l   SRP_DATA
    clr.l   SRP_DATA+4
    .word   0xF010
    .word   0x4A00                  | PMOVE from SRP

    mov.l   SRP_DATA, %d0
    cmpi.l  #0x80000003, %d0
    bne     TEST_FAIL

    mov.l   SRP_DATA+4, %d0
    cmpi.l  #0x00301000, %d0
    bne     TEST_FAIL

    rts
