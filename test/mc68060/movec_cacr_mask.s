
*
* movec_cacr_mask.s — Test 68060 CACR writable mask
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Write all 1s to CACR
    mov.l   #0xFFFFFFFF, %d0
    movec   %d0, %cacr
    * Read back
    movec   %cacr, %d1
    * Only writable bits should be set: 0xF0F0C800
    cmpi.l  #0xF0F0C800, %d1
    bne     TEST_FAIL

    * Write 0 to CACR
    clr.l   %d0
    movec   %d0, %cacr
    movec   %cacr, %d1
    tst.l   %d1
    bne     TEST_FAIL

    rts
