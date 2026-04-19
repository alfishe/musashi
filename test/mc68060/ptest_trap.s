
*
* ptest_trap.s — Verify PTEST (PMMU coprocessor) traps to F-line on 060
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * PTEST encoding: F000 | mode/reg, ext=0x8xxx
    clr.l   %d7
    .short  0xF010              | PMMU op, EA=(A0)
    .short  0x8000              | PTEST W
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
