
*
* lpstop_exec.s — Verify LPSTOP instruction on 060
* LPSTOP halts the CPU like STOP but in low-power mode.
*
* Strategy: Write pass marker via TEST_PASS_REG (0x100004) BEFORE
* LPSTOP. The CPU halts after LPSTOP, so the test driver sees
* pass=1, fail=0. The test body never returns via rts.
*

.include "entry.s"
.include "entry_060.s"

    jsr setup_060_vectors

    * Mark pass BEFORE stopping (TEST_PASS_REG = 0x100004)
    mov.l   #1, 0x100004

    * LPSTOP with supervisor + IPL=7
    .short  0xF800
    .short  0x01C0
    .short  0x2700              | SR value: supervisor, IPL=7

    * Should NOT reach here — CPU is stopped
    .short  0x4E72              | STOP (fallback)
    .short  0x2700
