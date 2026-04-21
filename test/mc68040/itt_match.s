.include "entry_040_mmu.s"
* Test ITT0 matching with mask=0x00 (should match 0x00xxxxxx)
* This test verifies the TT matching logic for the code region

op_ITT_MATCH:

    * Set up ITT0 to cover 0x00xxxxxx with mask=0x00
    * This should cover addresses 0x00000000-0x00FFFFFF
    * Test code runs from 0x10000 = 0x00010000, so should match
    mov.l   #0x0000C000, %d0    | Base=0x00, Mask=0x00, E=1, S=10
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004

    * Also set DTT0 to same value for data accesses
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    * Disable other TT registers
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,ITT1
    .word   0x0005
    .word   0x4E7B              | MOVEC D0,DTT1
    .word   0x0007

    * Enable MMU with 4K pages
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * If we get here, ITT0 matching worked for instruction fetches!
    * Now execute a few data accesses to verify DTT0 matching
    lea     0x100, %a0          | Some address in 0x00xxxxxx range
    move.l  (%a0), %d1          | Data read - should match DTT0

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Clear TT registers
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    rts
