
*
* movec_pcr.s — Test MOVEC PCR read on 68060
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Read PCR — expect MC68060 silicon ID in high 16 bits
    movec   %pcr, %d0

    * Check high word contains revision/ID (0x0430 for MC68060)
    swap    %d0
    cmpi.w  #0x0430, %d0
    bne     TEST_FAIL

    rts
