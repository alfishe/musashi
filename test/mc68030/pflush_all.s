.include "entry_030_mmu.s"
* OPCODE: PFLUSHA
* Test PFLUSHA clears ATC

op_PFLUSH:

    .set TC_DATA, 0x300100

    * Use TT0 and TT1 for full identity mapping
    mov.l   #0x007FC007, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800                  | PMOVE to TT0

    mov.l   #0x807FC007, 0x300204
    lea     0x300204, %a0
    .word   0xF010
    .word   0x0C00                  | PMOVE to TT1

    * Set up CRP (needed for PFLUSH to have something to work with)
    mov.l   #0x80000002, 0x300108
    mov.l   #PT_ROOT, 0x30010C
    lea     0x300108, %a0
    .word   0xF010
    .word   0x4C00                  | PMOVE to CRP

    * Set up a page table entry
    mov.l   #0x00000001, PT_ROOT

    * Enable MMU
    mov.l   #0x80C00000, TC_DATA    | E=1, PS=12, rest zero (TT does mapping)
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000                  | PMOVE to TC

    * Do a memory access to potentially populate ATC
    mov.l   TEST_DATA, %d0

    * Now do PFLUSHA
    .word   0xF000
    .word   0x2400                  | PFLUSHA

    * Do another access - should still work (TT mapping)
    mov.l   TEST_DATA+4, %d1

    * Verify accesses worked
    mov.l   #0xCAFEBABE, TEST_DATA
    mov.l   TEST_DATA, %d2
    cmpi.l  #0xCAFEBABE, %d2
    bne     TEST_FAIL

    * Disable MMU
    mov.l   #0x00000000, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Clear TT registers
    mov.l   #0x00000000, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800
    .word   0xF010
    .word   0x0C00

    rts
