.include "entry_030_mmu.s"
* OPCODE: TT matching
* Test TT0/TT1 transparent translation sets MMUSR T bit

op_TT_MATCH:

    .set TC_DATA, 0x300100
    .set MMUSR_DATA, 0x300118

    * Use TT0/TT1 for identity mapping
    mov.l   #0x007FC007, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800
    mov.l   #0x807FC007, 0x300204
    lea     0x300204, %a0
    .word   0xF010
    .word   0x0C00

    * Set up CRP with INVALID root pointer (type 0)
    * If TT doesn't match, access would fail
    mov.l   #0x80000000, 0x300108   | Type 0 = invalid
    mov.l   #0x00000000, 0x30010C
    lea     0x300108, %a0
    .word   0xF010
    .word   0x4C00

    * Enable MMU
    mov.l   #0x80C00000, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Access TEST_DATA - should succeed via TT match
    mov.l   #0x12345678, TEST_DATA
    mov.l   TEST_DATA, %d0
    cmpi.l  #0x12345678, %d0
    bne     TEST_FAIL

    * PTEST on TEST_DATA address - should show Transparent bit
    lea     TEST_DATA, %a0
    .word   0xF010
    .word   0x9E08                  | PTESTR, level=7, FC=5

    * Read MMUSR
    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Check Transparent bit (bit 6 = 0x0040)
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0040, %d0
    beq     TEST_FAIL               | T bit should be set

    * Disable MMU
    mov.l   #0x00000000, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Clear TT
    mov.l   #0x00000000, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800
    .word   0xF010
    .word   0x0C00

    rts
