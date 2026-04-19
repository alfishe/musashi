
*
* cas2_trap.s — Verify CAS2 (.W and .L) traps to Vector 61 on 060
*
* CAS2 is 6 bytes (opcode + 2 extension words). The exception handler
* advances PC by 4 from REG_PPC. So PC after RTE = opcode_start + 4,
* which lands on the 2nd extension word (bytes 4-5). We set that word
* to 0x4E71 (NOP) so it executes harmlessly, then the check follows.
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * --- CAS2.W should trap ---
    clr.l   %d7
    .short  0x0CFC              | CAS2.W opcode (bytes 0-1)
    .short  0x0008              | ext word 1 (bytes 2-3)
    .short  0x4E71              | ext word 2 = NOP (bytes 4-5, executed after trap)
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CAS2.L should trap ---
    clr.l   %d7
    .short  0x0EFC              | CAS2.L opcode
    .short  0x0008              | ext word 1
    .short  0x4E71              | ext word 2 = NOP
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CAS.L (single operand) should NOT trap ---
    clr.l   %d7
    lea.l   0x300000, %a0
    mov.l   #0xABCD, (%a0)
    mov.l   #0xABCD, %d0
    mov.l   #0x1234, %d1
    cas.l   %d0, %d1, (%a0)
    tst.l   %d7                 | d7 must be 0 — no trap
    bne     TEST_FAIL

    rts
