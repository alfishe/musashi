.include "entry_030_mmu.s"
* OPCODE: PMOVE TT0/TT1
* Test PMOVE to/from Transparent Translation registers

op_PMOVE_TT:

    * Use RAM area for test data (0x300000 is mapped)
    .set TT0_DATA, 0x300000
    .set TT1_DATA, 0x300004
    .set TT0_VERIFY, 0x300008

    * Test 1: Write to TT0, read back, verify
    mov.l   #0x00FF8543, TT0_DATA

    lea     TT0_DATA, %a0
    * PMOVE to TT0 (opcode: F010 0800)
    .word   0xF010                  | PMOVE (a0)
    .word   0x0800                  | to TT0

    * PMOVE from TT0 to memory
    .word   0xF010                  | PMOVE (a0)
    .word   0x0A00                  | from TT0

    mov.l   TT0_DATA, %d0
    cmpi.l  #0x00FF8543, %d0
    bne     TEST_FAIL

    * Test 2: Write to TT1, read back, verify
    mov.l   #0x80008107, TT1_DATA
    lea     TT1_DATA, %a0

    * PMOVE to TT1
    .word   0xF010                  | PMOVE (a0)
    .word   0x0C00                  | to TT1

    * PMOVE from TT1 to memory
    .word   0xF010                  | PMOVE (a0)
    .word   0x0E00                  | from TT1

    mov.l   TT1_DATA, %d0
    cmpi.l  #0x80008107, %d0
    bne     TEST_FAIL

    * Test 3: Verify TT0 was not affected by TT1 write
    lea     TT0_VERIFY, %a0
    .word   0xF010                  | PMOVE (a0)
    .word   0x0A00                  | from TT0

    mov.l   TT0_VERIFY, %d0
    cmpi.l  #0x00FF8543, %d0
    bne     TEST_FAIL

    rts
