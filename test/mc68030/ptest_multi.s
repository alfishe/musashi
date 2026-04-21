.include "entry_030_mmu.s"
* OPCODE: PTEST multi-level table walk
* Test 3-level table walk: Root -> Level A -> Level B -> Page

op_PTEST_MULTI:

    .set TC_DATA, 0x300100
    .set MMUSR_DATA, 0x300110

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

    * Set up 3-level page table for address 0x12300000
    * TC: E=1, PS=12 (4K pages), IS=0, TIA=4, TIB=4, TIC=4, TID=0
    * Sum = 12 + 0 + 4 + 4 + 4 + 0 = 24... need 32
    * Use: PS=8, IS=0, TIA=8, TIB=8, TIC=8, TID=0 = 32
    *
    * With PS=8 (256-byte pages), address breakdown:
    * TIA (8 bits): bits [31:24] = 0x12 -> index 0x12
    * TIB (8 bits): bits [23:16] = 0x30 -> index 0x30
    * TIC (8 bits): bits [15:8] = 0x00 -> index 0
    * Page offset (8 bits): bits [7:0]

    .set TEST_ADDR, 0x12300000

    * Root table: entry at index 0x12 points to level A
    mov.l   #PT_LEVEL_A, %d0
    ori.l   #0x02, %d0              | Type 2 (4-byte table)
    mov.l   %d0, PT_ROOT+(0x12*4)

    * Level A: entry at index 0x30 points to level B
    mov.l   #PT_LEVEL_B, %d0
    ori.l   #0x02, %d0              | Type 2 (4-byte table)
    mov.l   %d0, PT_LEVEL_A+(0x30*4)

    * Level B: entry at index 0 is page descriptor
    mov.l   #0x12300001, PT_LEVEL_B | Page descriptor

    * Enable MMU
    * TC: E=1, PS=8, IS=0, TIA=8, TIB=8, TIC=8, TID=0
    mov.l   #0x80808880, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * PTEST on address 0x12300000 - should walk 3 levels
    lea     TEST_ADDR, %a0
    .word   0xF010                  | PTEST (a0)
    .word   0x9E08                  | R/W=1, level=7, FC=5

    * Read MMUSR
    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Check that Invalid bit is NOT set
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0400, %d0
    bne     TEST_FAIL               | Should NOT be invalid

    * Check level field (bits 2:0) - should be 3 (3 descriptor reads)
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0007, %d0
    cmpi.w  #3, %d0
    bne     TEST_FAIL               | Level should be 3

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
