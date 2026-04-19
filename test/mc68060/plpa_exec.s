
*
* plpa_exec.s — Verify PLPA executes without trapping on 060
* PLPA translates logical to physical address via MMU.
* Without MMU translation, the address register is unchanged.
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * --- PLPAW (A0) — should NOT trap ---
    clr.l   %d7
    lea.l   0x300000, %a0
    .short  0xF518              | PLPAW (A0)
    tst.l   %d7                 | No trap expected
    bne     TEST_FAIL
    * Without MMU, address should be unchanged
    cmpa.l  #0x300000, %a0
    bne     TEST_FAIL

    * --- PLPAR (A0) — should NOT trap ---
    clr.l   %d7
    lea.l   0x400000, %a0
    .short  0xF548              | PLPAR (A0)
    tst.l   %d7
    bne     TEST_FAIL
    cmpa.l  #0x400000, %a0
    bne     TEST_FAIL

    * --- PLPAW with different registers ---
    clr.l   %d7
    lea.l   0x500000, %a3
    .short  0xF51B              | PLPAW (A3)
    tst.l   %d7
    bne     TEST_FAIL

    clr.l   %d7
    lea.l   0x600000, %a5
    .short  0xF51D              | PLPAW (A5)
    tst.l   %d7
    bne     TEST_FAIL

    rts
