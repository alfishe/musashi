
*
* fpiar_trap.s — Verify FMOVEM with FPIAR traps on 060
* 060 removed the FPIAR register. FMOVEM with FPIAR bit set must trap.
* FMOVEM with only FPCR/FPSR must NOT trap.
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * --- FMOVEM FPCR to memory — should NOT trap ---
    clr.l   %d7
    lea.l   0x300000, %a0
    fmovem.l %fpcr, (%a0)
    tst.l   %d7
    bne     TEST_FAIL

    * --- FMOVEM FPSR to memory — should NOT trap ---
    clr.l   %d7
    fmovem.l %fpsr, (%a0)
    tst.l   %d7
    bne     TEST_FAIL

    * --- FMOVEM FPCR/FPSR to memory — should NOT trap ---
    clr.l   %d7
    fmovem.l %fpcr/%fpsr, (%a0)
    tst.l   %d7
    bne     TEST_FAIL

    * --- FMOVEM FPIAR to memory — MUST trap (FPIAR removed) ---
    clr.l   %d7
    fmovem.l %fpiar, (%a0)
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    * --- FMOVEM FPCR/FPSR/FPIAR — MUST trap (FPIAR in mask) ---
    clr.l   %d7
    fmovem.l %fpcr/%fpsr/%fpiar, (%a0)
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
