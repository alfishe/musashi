.include "entry_030_mmu.s"
* OPCODE: MMU identity mapping test
* Tests basic MMU enable with identity mapping for all regions

op_MMU_IDENTITY:

    .set TC_DATA, 0x300100

    * Use TT0 and TT1 to transparently map the whole address space
    * This avoids needing complex page tables for basic testing

    * TT0: Map 0x00000000-0x7FFFFFFF (low half)
    * Format: Base=0x00, Mask=0x7F, E=1, CI=0, RWM=0, RW=0, FCB=0, FCM=0
    * = 0x007FC007
    mov.l   #0x007FC007, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800                  | PMOVE to TT0

    * TT1: Map 0x80000000-0xFFFFFFFF (high half)
    * Format: Base=0x80, Mask=0x7F, E=1, CI=0, RWM=0, RW=0, FCB=0, FCM=0
    * = 0x807FC007
    mov.l   #0x807FC007, 0x300204
    lea     0x300204, %a0
    .word   0xF010
    .word   0x0C00                  | PMOVE to TT1

    * Set up dummy CRP (won't be used due to TT match)
    mov.l   #0x80000002, 0x300108
    mov.l   #PT_ROOT, 0x30010C
    lea     0x300108, %a0
    .word   0xF010
    .word   0x4C00                  | PMOVE to CRP

    * Set up root table entry (won't be used)
    mov.l   #0x00000001, PT_ROOT    | Page descriptor for identity

    * Enable MMU with minimal TC (TT registers do all the work)
    * TC: E=1, SRE=0, FCL=0, PS=12, IS=0, TIA=0, TIB=0, TIC=0, TID=0
    * = 0x80000000 (just enable bit)
    mov.l   #0x80000000, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000                  | PMOVE to TC

    * If we get here, MMU is enabled with identity mapping
    * Test a memory access
    mov.l   #0xDEADBEEF, TEST_DATA
    mov.l   TEST_DATA, %d0
    cmpi.l  #0xDEADBEEF, %d0
    bne     TEST_FAIL

    * Disable MMU
    mov.l   #0x00000000, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Clear TT registers
    mov.l   #0x00000000, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800
    .word   0xF010
    .word   0x0C00

    rts
