.include "entry_040_mmu.s"
* OPCODE: PFLUSH (An)
* Test PFLUSH flushes single ATC entry

op_PFLUSH_PAGE:

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

    * Set up page table for 0x10000000 and 0x10001000
    mov.l   #PT_PTR, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_ROOT+(8*4)

    mov.l   #PT_PAGE, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_PTR

    * Page entry 0: 0x10000000
    mov.l   #0x10000003, PT_PAGE
    * Page entry 1: 0x10001000
    mov.l   #0x10001003, PT_PAGE+4

    * Enable MMU with 4K pages
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * PTEST both addresses to populate ATC
    lea     TEST_ADDR1, %a0
    .word   0xF568              | PTESTR (A0)
    lea     TEST_ADDR2, %a0
    .word   0xF568              | PTESTR (A0)

    * Invalidate first page table entry
    clr.l   PT_PAGE

    * PFLUSH only TEST_ADDR1
    lea     TEST_ADDR1, %a0
    .word   0xF508              | PFLUSH (A0)

    * PTEST on TEST_ADDR1 - should fail (ATC flushed, page invalid)
    lea     TEST_ADDR1, %a0
    .word   0xF568              | PTESTR (A0)

    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805
    andi.l  #0x01, %d0
    bne     TEST_FAIL           | R bit should NOT be set

    * PTEST on TEST_ADDR2 - should succeed (ATC still valid)
    lea     TEST_ADDR2, %a0
    .word   0xF568              | PTESTR (A0)

    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805
    andi.l  #0x01, %d0
    beq     TEST_FAIL           | R bit should be set

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Clear DTT0
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    rts
