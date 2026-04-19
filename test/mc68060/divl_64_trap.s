
*
* divl_64_trap.s — Verify 64-bit DIVL traps on 060, 32-bit DIVL works
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * --- 64-bit DIVU.L (Dr:Dq form) should trap ---
    clr.l   %d7
    mov.l   #0, %d2             | Dh = 0 (high 32 bits of dividend)
    mov.l   #1000, %d0          | Dl = 1000 (low 32 bits of dividend)
    mov.l   #7, %d1             | divisor
    * DIVU.L D1,D2:D0
    *   opcode = 4C41 (DIVUL.L, EA=D1)
    *   ext word: Dq=D0(0000), unsigned(0), 64-bit(1), reserved(00), Dr=D2(010)
    *   = 0000 0 1 00 000 00 010 = 0x0402
    .short  0x4C41              | DIVU.L Dn
    .short  0x0402              | Dq=D0, 64-bit, Dr=D2
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- 64-bit DIVS.L should also trap ---
    clr.l   %d7
    .short  0x4C41              | DIVS.L Dn
    .short  0x0C02              | Dq=D0, signed, 64-bit, Dr=D2
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- 32-bit DIVU.L should NOT trap ---
    clr.l   %d7
    mov.l   #1000, %d0
    mov.l   #7, %d1
    divu.l  %d1, %d0            | 32-bit result → native
    tst.l   %d7                 | d7 must be 0 — no trap
    bne     TEST_FAIL
    cmpi.l  #142, %d0           | 1000 / 7 = 142 (truncated)
    bne     TEST_FAIL

    rts
