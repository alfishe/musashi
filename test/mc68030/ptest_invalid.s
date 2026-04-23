.include "entry_030_mmu.s"
* OPCODE: PTEST invalid descriptor
* Test that PTEST sets Invalid bit in MMUSR for type=0 descriptor

op_PTEST_INVALID:

    .set TC_DATA, 0x300100
    .set MMUSR_DATA, 0x300110
    .set TEST_ADDR, 0x10000000

    * TT0 covers 0x00xxxxxx only (code/data area)
    mov.l   #0x0000C007, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800

    * Disable TT1
    mov.l   #0x00000000, 0x300204
    lea     0x300204, %a0
    .word   0xF010
    .word   0x0C00

    * Set up CRP pointing to root table
    mov.l   #0x80000002, 0x300108
    mov.l   #PT_ROOT, 0x30010C
    lea     0x300108, %a0
    .word   0xF010
    .word   0x4C00

    * Set up page table with INVALID entry (type=0) for address 0x10000000
    * TC: E=1, PS=8, IS=0, TIA=8, TIB=8, TIC=8, TID=0
    * Root entry at index 0x10 for 0x10xxxxxx

    * Root table: entry at index 0x10 with TYPE=0 (invalid)
    mov.l   #0x00000000, PT_ROOT+(0x10*4)   | Type 0 = invalid

    * Enable MMU
    mov.l   #0x88088800, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * PTEST on address 0x10000000 - should find invalid descriptor
    lea     TEST_ADDR, %a0
    .word   0xF010                  | PTEST (a0)
    .word   0x9E08                  | R/W=1, level=7, FC=5

    * Read MMUSR
    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Check Invalid bit (bit 10 = 0x0400)
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0400, %d0
    beq     TEST_FAIL               | Invalid bit MUST be set

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

    rts
