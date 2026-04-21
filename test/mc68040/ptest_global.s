.include "entry_040_mmu.s"
* OPCODE: PTEST G bit
* Test PTEST detecting global bit in page descriptor

op_PTEST_GLOBAL:

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

    * Set up page table for 0x10000000 with G=1
    * Root entry at index 8 (0x10000000 >> 25 = 8)
    mov.l   #PT_PTR, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_ROOT+(8*4)

    * Pointer entry at index 0
    mov.l   #PT_PAGE, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_PTR

    * Page descriptor with G bit set (bit 10 = 0x400)
    * Physical addr 0x10000000 + G=1 + UDT=3
    mov.l   #0x10000403, PT_PAGE

    * Enable MMU with 4K pages
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Set DFC to supervisor data for PTESTW
    moveq   #5, %d0
    .word   0x4E7B              | MOVEC D0,DFC
    .word   0x0001

    * PTEST on address 0x10000000
    lea     TEST_ADDR, %a0
    .word   0xF568              | PTESTW (A0)

    * Read MMUSR
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805

    * Check G bit (global) - bit 7 in MMUSR (0x80)
    andi.l  #0x80, %d0
    beq     TEST_FAIL           | G bit should be set

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Clear DTT0
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    rts
