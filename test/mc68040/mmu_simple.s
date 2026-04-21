.include "entry_040_mmu.s"
* Simple test: set TT registers, enable MMU briefly, disable
* This test verifies basic MMU enable/disable works

op_MMU_SIMPLE:

    * Set up DTT0/ITT0 to cover ALL addresses (mask=0xFF)
    * This makes all accesses transparent
    mov.l   #0x00FFC000, %d0    | Base=0x00, Mask=0xFF, E=1, S=10
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004
    .word   0x4E7B              | MOVEC D0,DTT1
    .word   0x0007
    .word   0x4E7B              | MOVEC D0,ITT1
    .word   0x0005

    * Enable MMU
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Execute a few simple instructions while MMU is on
    moveq   #1, %d1
    moveq   #2, %d2
    add.l   %d1, %d2

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Verify D2 = 3
    cmp.l   #3, %d2
    bne     TEST_FAIL

    rts
