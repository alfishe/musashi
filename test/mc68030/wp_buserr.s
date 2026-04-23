.include "entry_030_mmu.s"
* OPCODE: Write-protect bus error
* Test that write to write-protected page causes bus error exception
* Uses address 0x10000000 which is NOT covered by TT

op_WP_BUSERR:

    .set TC_DATA, 0x300100
    .set TEST_ADDR, 0x10000000
    .set BUSERR_FLAG, 0x300120

    * Clear bus error flag
    clr.l   BUSERR_FLAG

    * Set up bus error handler
    mov.l   #WP_BUSERR_HANDLER, 0x08

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

    * Set up page table for address 0x10000000 with WRITE-PROTECT
    * TC: E=1, PS=8, IS=0, TIA=8, TIB=8, TIC=8, TID=0
    * Address bits [31:24] = 0x10 -> index 0x10

    * Root table: entry at index 0x10 points to level A
    mov.l   #PT_LEVEL_A, %d0
    ori.l   #0x02, %d0              | Type 2 (4-byte table)
    mov.l   %d0, PT_ROOT+(0x10*4)

    * Level A: page descriptor with WP=1
    mov.l   #0x10000005, PT_LEVEL_A | addr=0x10000000, WP=1, type=page

    * Enable MMU
    mov.l   #0x88088800, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Try to write to write-protected page - should cause bus error
    lea     TEST_ADDR, %a0
    mov.l   #0x12345678, (%a0)      | This should trigger bus error

    * If we get here without bus error, test fails
    bra     TEST_FAIL

WP_BUSERR_HANDLER:
    * Bus error occurred - pass the test
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

    * Pass test - bus error was correctly triggered
    mov.l   #1, TEST_PASS_REG
    stop    #0x2700
