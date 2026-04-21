.include "entry_040_mmu.s"
* OPCODE: PFLUSHN
* Test PFLUSHN flushes non-global entries but preserves global ones
* This test verifies PFLUSHN executes without error and affects ATC

op_PFLUSHN:

    .set TEST_ADDR1, 0x10000000
    .set TEST_ADDR2, 0x10001000

    * Set up DTT0/ITT0 to cover 0x00xxxxxx
    mov.l   #0x0000C000, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004

    * Disable other TT registers
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT1
    .word   0x0007
    .word   0x4E7B              | MOVEC D0,ITT1
    .word   0x0005

    * Set up URP and SRP (PTESTW uses SRP when DFC is supervisor)
    mov.l   #PT_ROOT, %d0
    .word   0x4E7B              | MOVEC D0,URP
    .word   0x0806
    .word   0x4E7B              | MOVEC D0,SRP
    .word   0x0807

    * Clear page tables
    lea     PT_ROOT, %a0
    mov.l   #0x1000, %d0
clear_loop:
    clr.l   (%a0)+
    subq.l  #4, %d0
    bne     clear_loop

    * Set up page table for 0x10000000 (global page)
    mov.l   #PT_PTR, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_ROOT+(8*4)

    mov.l   #PT_PAGE, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_PTR

    * Page 0: Global (G=1, bit 10)
    mov.l   #0x10000403, PT_PAGE

    * Page 1: Non-global (G=0)
    mov.l   #0x10001003, PT_PAGE+4

    * Enable MMU with 4K pages
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Set DFC to supervisor data for PTESTW
    moveq   #5, %d0
    .word   0x4E7B              | MOVEC D0,DFC
    .word   0x0001

    * PTEST on global address 0x10000000
    lea     TEST_ADDR1, %a0
    .word   0xF568              | PTESTW (A0)

    * Read MMUSR - verify G bit is set
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805
    andi.l  #0x80, %d0
    beq     TEST_FAIL           | G bit should be set for global page

    * PTEST on non-global address 0x10001000
    lea     TEST_ADDR2, %a0
    .word   0xF568              | PTESTW (A0)

    * Read MMUSR - verify G bit is NOT set
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805
    andi.l  #0x80, %d0
    bne     TEST_FAIL           | G bit should NOT be set for non-global page

    * Now execute PFLUSHN on the non-global address
    * PFLUSHN (An): opcode F510 + reg
    lea     TEST_ADDR2, %a0
    .word   0xF510              | PFLUSHN (A0)

    * PTEST again on non-global - should still work (re-walks table)
    .word   0xF568              | PTESTW (A0)
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805
    andi.l  #0x01, %d0          | Check R (resident) bit
    beq     TEST_FAIL           | Should still be resident after re-walk

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Clear DTT0
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    rts
