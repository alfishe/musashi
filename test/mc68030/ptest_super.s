.include "entry_030_mmu.s"
* OPCODE: PTEST supervisor-only page
* Test that PTEST sets S bit in MMUSR for supervisor-only descriptor

op_PTEST_SUPER:

    .set TC_DATA, 0x300100
    .set MMUSR_DATA, 0x300110
    .set TEST_ADDR, 0x10000000

    * TT0 covers 0x00xxxxxx only
    mov.l   #0x0000C007, 0x300200
    lea     0x300200, %a0
    .word   0xF010
    .word   0x0800

    * Disable TT1
    mov.l   #0x00000000, 0x300204
    lea     0x300204, %a0
    .word   0xF010
    .word   0x0C00

    * Set up CRP pointing to root table with 8-byte descriptors
    * Use type=3 for 8-byte table descriptors
    mov.l   #0x80000003, 0x300108
    mov.l   #PT_ROOT, 0x30010C
    lea     0x300108, %a0
    .word   0xF010
    .word   0x4C00

    * Set up page table for address 0x10000000 with S=1 (supervisor only)
    * TC: E=1, PS=8, IS=0, TIA=8, TIB=8, TIC=8, TID=0
    * Root entry at index 0x10 for 0x10xxxxxx
    * NOTE: S bit only exists in 8-byte (long) descriptors

    * Root table: entry at index 0x10 (8-byte descriptor, type=3)
    * 8-byte descriptor: first word = limit/type, second word = address
    mov.l   #0x00000103, PT_ROOT+(0x10*8)     | S=1, type=3 (8-byte table)
    mov.l   #PT_LEVEL_A, PT_ROOT+(0x10*8)+4   | Table address

    * Level A: 8-byte page descriptor with S=1
    * For long-format page descriptor: first word has S bit, second word has address
    mov.l   #0x00000101, PT_LEVEL_A           | S=1, type=1 (page)
    mov.l   #0x10000000, PT_LEVEL_A+4         | Physical address

    * Enable MMU
    mov.l   #0x80808880, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * PTEST with FC=1 (user data) on supervisor-only page
    lea     TEST_ADDR, %a0
    .word   0xF010                  | PTEST (a0)
    .word   0x9E01                  | R/W=1, level=7, FC=1 (user data)

    * Read MMUSR
    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Check Supervisor-only bit (bit 13 = 0x2000)
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x2000, %d0
    beq     TEST_FAIL               | S bit should be set

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

    rts
