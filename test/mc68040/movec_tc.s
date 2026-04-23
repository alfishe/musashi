.include "entry_040_mmu.s"
* OPCODE: MOVEC TC
* Test TC (Translation Control) register read/write via MOVEC

op_MOVEC_TC:

    * Set up DTT0 to transparently map 0x00xxxxxx (covers code, stack, devices)
    * This must be done BEFORE enabling the MMU, otherwise instruction
    * fetches through unmapped pages cause recursive bus errors
    mov.l   #0x0000C000, %d0    | Base=0x00, Mask=0x00, E=1, S=U+S
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004

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

    * Clear TT registers
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004

    rts
