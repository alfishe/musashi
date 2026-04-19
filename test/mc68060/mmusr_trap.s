
*
* mmusr_trap.s — Verify MOVEC MMUSR traps to illegal on 060
* MMUSR ($805) was removed from MOVEC on 060
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * MOVEC MMUSR,D0 — should trap to Illegal (Vector 4) on 060
    clr.l   %d7
    * MOVEC cr,Dn encoding: 4E7A ext_word
    * ext_word = Dn(15:12) | cr(11:0)
    * D0 = 0000, MMUSR = 0x805
    .short  0x4E7A              | MOVEC cr,Rn
    .short  0x0805              | D0, MMUSR
    cmpi.l  #0xEEEE0004, %d7   | Vector 4 = Illegal
    bne     TEST_FAIL

    rts
