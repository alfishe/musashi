.include "entry_030_mmu.s"
* OPCODE: PFLUSH with FC selector
* Test PFLUSH with specific function code

op_PFLUSH_FC:

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

    * Set up CRP pointing to root table
    mov.l   #0x80000002, 0x300108
    mov.l   #PT_ROOT, 0x30010C
    lea     0x300108, %a0
    .word   0xF010
    .word   0x4C00                  | PMOVE to CRP

    * Set up page table for address 0x10000000
    mov.l   #PT_LEVEL_A, %d0
    ori.l   #0x02, %d0
    mov.l   %d0, PT_ROOT+(0x10*4)
    mov.l   #0x10000001, PT_LEVEL_A

    * Enable MMU
    mov.l   #0x80808880, TC_DATA
    lea     TC_DATA, %a0
    .word   0xF010
    .word   0x4000

    * Flush ATC to start clean
    .word   0xF000
    .word   0x2400                  | PFLUSHA

    * PLOAD to populate ATC with FC=1 (user data)
    * PLOAD format: bits 4:0 = FC selector
    * For immediate FC: bit 4=1, bits 3:0=FC value
    * FC=1: bits 4:0 = 10001 = 0x11
    lea     TEST_ADDR, %a0
    .word   0xF010
    .word   0x2011                  | PLOADW FC=1 (immediate)

    * Verify translation works (PTEST level=7)
    * PTEST format: bits 12:10=level, bits 4:0=FC selector
    * level=7, FC=1 (immediate): 0x9C11
    lea     TEST_ADDR, %a0
    .word   0xF010
    .word   0x9C11                  | PTESTR level=7, FC=1

    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Should NOT be invalid (walk succeeded)
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0400, %d0
    bne     TEST_FAIL

    * PFLUSH with FC=1 to flush only user data entries
    * PFLUSH mode 4: bits 12:10 = 100
    * FC from D0: bits 4:3 = 01, bits 2:0 = 000
    * fcmask = 7: bits 7:5 = 111
    * Encoding: 0011 0000 1110 1000 = 0x30E8
    mov.l   #1, %d0
    .word   0xF000
    .word   0x30E8                  | PFLUSH FC from D0

    * PTEST level=0 should now miss (ATC flushed)
    lea     TEST_ADDR, %a0
    .word   0xF010
    .word   0x8011                  | PTESTR level=0, FC=1

    lea     MMUSR_DATA, %a0
    .word   0xF010
    .word   0x6200

    * Should be invalid (ATC miss after PFLUSH)
    mov.w   MMUSR_DATA, %d0
    andi.w  #0x0400, %d0
    beq     TEST_FAIL               | Invalid should be set

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
