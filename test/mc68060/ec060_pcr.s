
*
* ec060_pcr.s — Verify EC060 variant PCR silicon ID = 0x0432
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * MOVEC PCR,D0 — should read silicon ID for EC060
    .short  0x4E7A              | MOVEC cr,Rn
    .short  0x0808              | D0, PCR
    swap    %d0                 | Get high 16 bits (silicon ID)
    cmpi.w  #0x0432, %d0        | EC060 silicon revision
    bne     TEST_FAIL

    rts
