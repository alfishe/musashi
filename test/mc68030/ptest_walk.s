.include "entry_030_mmu.s"
* OPCODE: PTEST
* Test PTEST performs table walk and populates MMUSR
* Strategy: TT0 covers 0x00xxxxxx (code/data), test PTEST on 0x10000000

op_PTEST:

    .set TC_DATA, 0x300100
    .set MMUSR_DATA, 0x300110
    .set TEST_ADDR, 0x10000000

    * TT0 covers 0x00000000-0x00FFFFFF only (code/data area)
    * Base=0x00, Mask=0x00, E=1, S=10, FCM=7 (match any FC)
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

    * Set up simple page table for address 0x10000000
    * TC: E=1, PS=8 (256-byte pages), IS=0, TIA=8, TIB=8, TIC=8, TID=0
    * Sum = 8 + 0 + 8 + 8 + 8 + 0 = 32 (valid)
    * Address bits [31:24] (8 bits) index root table

    * Root table: entry at index 0x10 (for addr 0x10xxxxxx)
    mov.l   #PT_LEVEL_A, %d0
    ori.l   #0x02, %d0              | Type 2 (4-byte table)
    mov.l   %d0, PT_ROOT+(0x10*4)   | Index 0x10 = 0x10000000 >> 24

    * Level A: first entry points to page
    mov.l   #0x10000001, PT_LEVEL_A | Page descriptor, addr=0x10000000

    * Enable MMU
    * TC: E=1, PS=8, IS=0, TIA=8, TIB=8, TIC=8, TID=0
    mov.l   #0x80808880, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * PTEST on address 0x10000000 (NOT TT-matched, requires table walk)
    lea     TEST_ADDR, %a0
    .word   0xF010                  | PTEST (a0)
    .word   0x9E08                  | R/W=1, level=7, FC=5

    * Read MMUSR
    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Table walk should succeed, check NOT Invalid
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0400, %d0            | Invalid bit
    bne     TEST_FAIL               | Should NOT be invalid

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
