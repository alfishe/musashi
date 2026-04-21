.include "entry_030_mmu.s"
* OPCODE: PLOAD
* Test PLOAD preloads ATC entry

op_PLOAD:

    .set TC_DATA, 0x300100
    .set MMUSR_DATA, 0x300110

    * Use TT0/TT1 for identity mapping
    mov.l   #0x007FC007, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800
    mov.l   #0x807FC007, 0x300204
    lea     0x300204, %a0
    .word   0xF010
    .word   0x0C00

    * Set up CRP
    mov.l   #0x80000002, 0x300108
    mov.l   #PT_ROOT, 0x30010C
    lea     0x300108, %a0
    .word   0xF010
    .word   0x4C00

    * Set up page descriptor
    mov.l   #0x00000001, PT_ROOT

    * Enable MMU
    mov.l   #0x80C00000, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Flush ATC
    .word   0xF000
    .word   0x2400                  | PFLUSHA

    * PLOAD read on address 0
    lea     0, %a0
    .word   0xF010
    .word   0x2210                  | PLOADR #5

    * PTEST to verify ATC was populated
    lea     0, %a0
    .word   0xF010
    .word   0x9E08                  | PTESTR, level=7, FC=5

    * Read MMUSR
    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Check that Invalid bit is not set
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0400, %d0
    bne     TEST_FAIL               | Invalid should NOT be set

    * Disable MMU
    mov.l   #0x00000000, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Clear TT
    mov.l   #0x00000000, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800
    .word   0xF010
    .word   0x0C00

    rts
