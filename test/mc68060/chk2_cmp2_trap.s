
*
* chk2_cmp2_trap.s — Verify CHK2 and CMP2 trap to Vector 61 on 060
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Setup bounds at 0x300000: lower=0x0010, upper=0x0050
    lea.l   0x300000, %a0
    mov.w   #0x0010, (%a0)
    mov.w   #0x0050, 2(%a0)

    * --- CHK2.B should trap ---
    clr.l   %d7
    mov.l   #0x30, %d0
    * CHK2.B encoding: 00C0 + EA | ext word with bit 11 set
    * EA = (A0) = mode 010, reg 000 = 010_000 = 0x10
    .short  0x00D0              | CHK2/CMP2.B (A0)
    .short  0x0800              | D0, CHK2 (bit 11 = 1)
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CMP2.B should also trap ---
    clr.l   %d7
    .short  0x00D0              | CMP2.B (A0)
    .short  0x0000              | D0, CMP2 (bit 11 = 0)
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CHK2.W should trap ---
    clr.l   %d7
    .short  0x02D0              | CHK2/CMP2.W (A0)
    .short  0x0800              | D0, CHK2
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CHK2.L should trap ---
    clr.l   %d7
    .short  0x04D0              | CHK2/CMP2.L (A0)
    .short  0x0800              | D0, CHK2
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    rts
