.include "entry_040_mmu.s"
* OPCODE: MOVEC TC
* Test TC (Translation Control) register read/write via MOVEC

op_MOVEC_TC:

    * Ensure MMU is disabled initially
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Write TC with 4K pages (bit 14 = 0)
    mov.l   #0x8000, %d0        | E=1, P=0 (4K pages)
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Read back TC
    .word   0x4E7A              | MOVEC TC,D1
    .word   0x1003

    * Verify TC value (only bits 15 and 14 are significant)
    andi.l  #0xC000, %d1
    cmp.l   #0x8000, %d1
    bne     TEST_FAIL

    * Write TC with 8K pages (bit 14 = 1)
    mov.l   #0xC000, %d0        | E=1, P=1 (8K pages)
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Read back TC
    .word   0x4E7A              | MOVEC TC,D1
    .word   0x1003

    andi.l  #0xC000, %d1
    cmp.l   #0xC000, %d1
    bne     TEST_FAIL

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    rts
