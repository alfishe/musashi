.include "entry_040_mmu.s"
* OPCODE: DTT0/DTT1
* Test data transparent translation register matching

op_TT_DATA:

    .set MMUSR_DATA, 0x300100
    .set TEST_ADDR, 0x10000000

    * Set up URP/SRP pointing to root table
    mov.l   #PT_ROOT, %d0
    .word   0x4E7B              | MOVEC D0,URP
    .word   0x0806
    .word   0x4E7B              | MOVEC D0,SRP
    .word   0x0807

    * Set up DTT0 to cover 0x00xxxxxx (data area)
    * Format: Base=0x00, Mask=0x00, E=1, S=10 (both), CM=00, WP=0
    * Bits: 31-24=base, 23-16=mask, 15=E, 14-13=S, 6-5=CM, 2=WP
    mov.l   #0x0000C000, %d0    | Base=0x00, Mask=0x00, E=1, S=10
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    * Set up ITT0 to cover 0x00xxxxxx (code area - for instruction fetches)
    mov.l   #0x0000C000, %d0
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004

    * Disable DTT1, ITT1
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT1
    .word   0x0007
    .word   0x4E7B              | MOVEC D0,ITT1
    .word   0x0005

    * Enable MMU with 4K pages
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Set DFC to supervisor data (FC=5) for PTESTW
    moveq   #5, %d0
    .word   0x4E7B              | MOVEC D0,DFC
    .word   0x0001

    * PTEST on address 0x00001000 (should be TT hit)
    lea     0x00001000, %a0
    .word   0xF568              | PTESTW (A0) - uses DFC

    * Read MMUSR
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805

    * Check T bit (transparent hit) - bit 1
    andi.l  #0x02, %d0
    beq     TEST_FAIL           | T bit should be set

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Clear DTT0
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    rts
