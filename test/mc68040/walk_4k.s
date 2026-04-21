.include "entry_040_mmu.s"
* OPCODE: 4K Page Walk
* Test 68040 3-level page table walk with 4KB pages
* Address 0x10000000:
*   Root index (bits 31-25): 0x08 (0x10000000 >> 25 = 8)
*   Pointer index (bits 24-18): 0x00
*   Page index (bits 17-12): 0x00
*   Page offset (bits 11-0): 0x000

op_WALK_4K:

    .set TEST_ADDR, 0x10000000

    * Set up DTT0/ITT0 to cover 0x00xxxxxx (code/data area) for identity map
    mov.l   #0x0000C000, %d0
    .word   0x4E7B              | MOVEC D0,DTT0
    .word   0x0006
    .word   0x4E7B              | MOVEC D0,ITT0
    .word   0x0004

    * Disable DTT1, ITT1
    moveq   #0, %d0
    .word   0x4E7B              | MOVEC D0,DTT1
    .word   0x0007
    .word   0x4E7B              | MOVEC D0,ITT1
    .word   0x0005

    * Set up URP pointing to root table
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

    * Set up 3-level page table for address 0x10000000
    * Root entry at index 8 -> pointer table
    mov.l   #PT_PTR, %d0
    ori.l   #0x03, %d0          | UDT=3 (resident, used)
    mov.l   %d0, PT_ROOT+(8*4)

    * Pointer entry at index 0 -> page table
    mov.l   #PT_PAGE, %d0
    ori.l   #0x03, %d0          | UDT=3
    mov.l   %d0, PT_PTR

    * Page entry at index 0 -> physical page
    * Map 0x10000000 to itself (identity)
    mov.l   #0x10000003, PT_PAGE  | Physical addr + UDT=3 (resident)

    * Enable MMU with 4K pages
    mov.l   #0x8000, %d0
    .word   0x4E7B              | MOVEC D0,TC
    .word   0x0003

    * PTEST on address 0x10000000
    lea     TEST_ADDR, %a0
    .word   0xF568              | PTESTR (A0)

    * Read MMUSR
    .word   0x4E7A              | MOVEC MMUSR,D0
    .word   0x0805

    * Check R bit (resident) - bit 0
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
