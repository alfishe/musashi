
*
* movec_buscr.s — Test MOVEC BUSCR read/write on 68060
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Write a value to BUSCR
    mov.l   #0x12345678, %d0
    movec   %d0, %buscr

    * Read it back
    movec   %buscr, %d1
    cmp.l   %d0, %d1
    bne     TEST_FAIL

    * Write zero
    clr.l   %d0
    movec   %d0, %buscr
    movec   %buscr, %d1
    tst.l   %d1
    bne     TEST_FAIL

    rts
