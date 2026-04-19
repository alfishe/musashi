
*
* lc060_pcr.s — Verify LC060 variant PCR silicon ID = 0x0431
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * MOVEC PCR,D0 — should read silicon ID for LC060
    .short  0x4E7A              | MOVEC cr,Rn
    .short  0x0808              | D0, PCR
    swap    %d0                 | Get high 16 bits (silicon ID)
    cmpi.w  #0x0431, %d0        | LC060 silicon revision
    bne     TEST_FAIL

    rts
