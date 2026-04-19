
*
* movec_caar_bug.s — Regression test for MOVEC CAAR read on 68010
* See: https://github.com/kstenerud/Musashi/issues/94
*
* CAAR (0x802) is only valid on EC020+. On a 68010, MOVEC CAAR,D0
* should trigger exactly one illegal instruction exception.
* Before the fix, a break instead of return caused a privilege
* violation exception to fire in addition to the illegal exception.
*
* Strategy:
*   - Set up illegal instruction handler (vector 4) that increments D5
*   - Set up privilege violation handler (vector 8) that increments D6
*   - Execute MOVEC CAAR,D0 (supervisor mode, so priv-viol shouldn't fire)
*   - Verify D5 == 1 (illegal fired once) and D6 == 0 (no priv viol)
*

.include "entry.s"

    * Install our custom handlers
    lea.l   HANDLER_ILLEGAL(%pc), %a0
    mov.l   %a0, 0x10               | Vector 4: Illegal Instruction

    lea.l   HANDLER_PRIVVIOL(%pc), %a0
    mov.l   %a0, 0x20               | Vector 8: Privilege Violation

    * Clear counters
    clr.l   %d5                     | Illegal exception counter
    clr.l   %d6                     | Privilege violation counter

    * Execute MOVEC CAAR, D0 — should be illegal on 68010
    .short  0x4E7A                  | MOVEC  Rc, Rn
    .short  0x0802                  | D0 <- CAAR (register 0x802)

    * Verify: exactly 1 illegal, 0 privilege violations
    cmp.l   #1, %d5
    bne     TEST_FAIL

    cmp.l   #0, %d6
    bne     TEST_FAIL

    * Pass
    mov.l   #1, TEST_PASS_REG
    stop    #0x2700

HANDLER_ILLEGAL:
    addq.l  #1, %d5
    * Skip the 4-byte MOVEC instruction
    mov.l   (%sp,2), %a0           | Get return PC from stack frame
    addq.l  #4, (%sp,2)            | Advance past the MOVEC instruction
    rte

HANDLER_PRIVVIOL:
    addq.l  #1, %d6
    rte
