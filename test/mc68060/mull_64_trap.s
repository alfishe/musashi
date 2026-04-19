
*
* mull_64_trap.s — Verify 64-bit MULL traps on 060, 32-bit MULL works
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * --- 64-bit MULU.L (D2:D0 form) should trap ---
    clr.l   %d7
    mov.l   #100, %d0
    mov.l   #200, %d1
    * MULU.L D1,D2:D0 = 4C01 0400 (64-bit: bit 10 set, Dh=D0 in bits 2:0)
    * Actually: ext word = 0x0401 means Dl=D0(bits15:12=0), unsigned(bit11=0), 64-bit(bit10=1), Dh=D1(bits2:0=1)
    * Let's encode: MULU.L D1,D2:D0
    *   opcode = 4C01 (MULU.L, EA=D1)
    *   ext word: Dl=D0(0000), unsigned(0), 64-bit(1), reserved(00), Dh=D2(010)
    *   = 0000 0 1 00 000 00 010 = 0x0402
    .short  0x4C01              | MULU.L Dn
    .short  0x0402              | Dl=D0, 64-bit, Dh=D2
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- 64-bit MULS.L should also trap ---
    clr.l   %d7
    .short  0x4C01              | MULS.L Dn
    .short  0x0C02              | Dl=D0, signed(bit11=1), 64-bit(bit10=1), Dh=D2
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- 32-bit MULU.L should NOT trap ---
    clr.l   %d7
    mov.l   #100, %d0
    mov.l   #200, %d1
    mulu.l  %d1, %d0            | 32-bit result → native
    tst.l   %d7                 | d7 must be 0 — no trap
    bne     TEST_FAIL
    cmpi.l  #20000, %d0         | 100 * 200 = 20000
    bne     TEST_FAIL

    rts
