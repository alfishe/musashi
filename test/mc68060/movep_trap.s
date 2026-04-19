
*
* movep_trap.s — Verify that MOVEP generates Vector 61 trap on 68060
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Clear trap indicator
    clr.l   %d7

    * Execute MOVEP.W (register-to-memory)
    * On 68060 this should trap to Vector 61
    mov.l   #0x12345678, %d0
    movep.w %d0, 0(%a0)

    * Check that Vector 61 was taken
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * Test MOVEP.L (register-to-memory)
    clr.l   %d7
    movep.l %d0, 0(%a0)
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * Test MOVEP.W (memory-to-register)
    clr.l   %d7
    movep.w 0(%a0), %d1
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * Test MOVEP.L (memory-to-register)
    clr.l   %d7
    movep.l 0(%a0), %d1
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    rts
