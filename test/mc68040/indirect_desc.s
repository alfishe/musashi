.include "entry_040_mmu.s"
* OPCODE: Indirect Page Descriptor
* Test indirect page descriptors resolve correctly

op_INDIRECT:

    .set TEST_ADDR, 0x10000000
    .set PT_INDIRECT, PT_PAGE+0x100

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

    * Set up URP and SRP
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

    * Set up page table for 0x10000000
    * Root entry at index 8
    mov.l   #PT_PTR, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_ROOT+(8*4)

    * Pointer entry at index 0
    mov.l   #PT_PAGE, %d0
    ori.l   #0x03, %d0
    mov.l   %d0, PT_PTR

    * Page descriptor as indirect (type=2) pointing to actual page
    mov.l   #PT_INDIRECT, %d0
    ori.l   #0x02, %d0
    mov.l   %d0, PT_PAGE

    * Actual page descriptor at indirect location
    mov.l   #0x10000003, PT_INDIRECT

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

    * Check R bit (resident) - should be set
    andi.l  #0x01, %d0
    beq     TEST_FAIL

    * Disable MMU
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * Clear DTT0
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006

    rts
