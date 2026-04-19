
*
* pcr_write_mask.s — Test PCR write mask on 68060
* High 16 bits (silicon ID) are read-only, low 16 are writable
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Read initial PCR (should contain silicon ID in high word)
    movec   %pcr, %d0
    mov.l   %d0, %d2           | Save original

    * Try writing all 1s — high word should be preserved
    mov.l   #0xFFFFFFFF, %d0
    movec   %d0, %pcr
    movec   %pcr, %d1

    * High word should still be silicon ID (0x0430)
    mov.l   %d1, %d0
    swap    %d0
    cmpi.w  #0x0430, %d0
    bne     TEST_FAIL

    * Low word should have writable bits set
    * (exact mask depends on PCR writable bits, but should not be zero)
    mov.l   %d1, %d0
    andi.l  #0x0000FFFF, %d0
    tst.w   %d0
    beq     TEST_FAIL

    * Restore original PCR
    movec   %d2, %pcr

    rts
