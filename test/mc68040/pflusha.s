.include "entry_040_mmu.s"
* OPCODE: PFLUSHA
* Test PFLUSHA flushes entire ATC

op_PFLUSHA:

    .set TEST_ADDR, 0x10000000

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

    * Set up URP
    mov.l   #PT_ROOT, %d0
    .word   0x4E7B              | MOVEC D0,URP
    .word   0x0806

    * Clear page tables
    lea     PT_ROOT, %a0
    mov.l   #0x1000, %d0
clear_loop:
    clr.l   (%a0)+
    subq.l  #4, %d0
    bne     clear_loop

    * Set up valid page table for 0x10000000
    mov.l   #PT_PTR, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_ROOT+(8*4)

    mov.l   #PT_PAGE, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_PTR

    mov.l   #0x10000003, PT_PAGE

    * Enable MMU with 4K pages
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * PTEST to populate ATC (should succeed)
    lea     TEST_ADDR, %a0
    .word   0xF568              | PTESTR (A0)

    * Verify it worked (R bit set)
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805
    andi.l  #0x01, %d0
    beq     TEST_FAIL

    * Now PFLUSHA to flush ATC
    .word   0xF518              | PFLUSHA

    * Invalidate the page table entry
    clr.l   PT_PAGE

    * PTEST again - should fail now (page invalid, ATC flushed)
    lea     TEST_ADDR, %a0
    .word   0xF568              | PTESTR (A0)

    * Read MMUSR - R bit should NOT be set (page now invalid)
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805
    andi.l  #0x01, %d0
    bne     TEST_FAIL           | R bit should NOT be set

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Clear DTT0
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    rts
