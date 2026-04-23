.include "entry_030_mmu.s"
* OPCODE: PTEST write-protect
* Test PTEST detects write-protect in MMUSR
* Strategy: TT0 covers 0x00xxxxxx (code/data), test PTEST on 0x10000000 with WP

op_PTEST_WP:

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

    * Set up page table for address 0x10000000 with WRITE-PROTECT
    * TC: E=1, PS=8, IS=0, TIA=8, TIB=8, TIC=8, TID=0
    * Root entry at index 0x10 for 0x10xxxxxx

    * Root table: entry at index 0x10
    mov.l   #PT_LEVEL_A, %d0
    ori.l   #0x02, %d0              | Type 2 (4-byte table)
    mov.l   %d0, PT_ROOT+(0x10*4)

    * Level A: page descriptor with WP=1
    * Page descriptor format: addr[31:8], WP[2], type[1:0]=01
    mov.l   #0x10000005, PT_LEVEL_A | addr=0x10000000, WP=1, type=page

    * Enable MMU
    mov.l   #0x88088800, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * PTEST write on address 0x10000000 (NOT TT-matched, will table walk)
    lea     TEST_ADDR, %a0
    .word   0xF010                  | PTEST (a0)
    .word   0x9E08                  | R/W=1, level=7, FC=5

    * Read MMUSR
    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Check Write-Protect bit (bit 10 = 0x0400)
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0800, %d0            | WP bit is bit 11
    beq     TEST_FAIL               | WP should be set

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
