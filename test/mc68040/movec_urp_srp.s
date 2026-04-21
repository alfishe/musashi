.include "entry_040_mmu.s"
* OPCODE: MOVEC URP/SRP
* Test URP and SRP register read/write via MOVEC

op_MOVEC_URP_SRP:

    * Write to URP
    mov.l   #0x12340000, %d0
    .word   0x4E7B              | MOVEC D0,URP
    .word   0x0806

    * Read back URP
    .word   0x4E7A              | MOVEC URP,D1
    .word   0x1806

    * Verify URP value
    cmp.l   %d0, %d1
    bne     TEST_FAIL

    * Write to SRP
    mov.l   #0x56780000, %d0
    .word   0x4E7B              | MOVEC D0,SRP
    .word   0x0807

    * Read back SRP
    .word   0x4E7A              | MOVEC SRP,D1
    .word   0x1807

    * Verify SRP value
    cmp.l   %d0, %d1
    bne     TEST_FAIL

    * Verify URP unchanged
    .word   0x4E7A              | MOVEC URP,D2
    .word   0x2806
    cmp.l   #0x12340000, %d2
    bne     TEST_FAIL

    rts
