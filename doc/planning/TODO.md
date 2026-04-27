# Musashi 68000 SST Parity — TODO

## Current SST Baseline (2026-04-27)

| Metric | Value |
|--------|-------|
| SST vectors passing | 996,082 / 1,000,060 |
| SST files passing | 119 / 124 |
| Binary tests (mc68000) | 60 / 60 |

---

## ✅ Completed

### Harness — Reset CPU_INSTR_MODE between vectors
**Fix**: `CPU_INSTR_MODE = INSTRUCTION_YES` in `run_vector()` before executing.
**Result**: Eliminated ~200k+ bus error status word contamination failures.

### Address register fidelity in (An)+/-(An) addressing modes
**Fix**: In `m68kcpu.h`, moved register updates BEFORE `m68ki_check_address_error_010_less()` 
in 6 functions: `m68ki_ea_ay_pi_16`, `m68ki_ea_ay_pi_32`, `m68ki_ea_ay_pd_32`,
`m68ki_ea_ax_pi_16`, `m68ki_ea_ax_pi_32`, `m68ki_ea_ax_pd_32`.
**Result**: 39,432 → 16,478 failures (22,954 fixed). 28 additional SST files now passing.

### Silicon-accurate two-step pre-decrement for ADDX/SUBX.l mm
**Fix**: Inline 2+2 pre-decrement in ADDX/SUBX handlers in `m68k_in.c`.
**Result**: ADDX.l 8065/8065, SUBX.l 8065/8065.

### Address error on odd branch/return targets
**Fix**: BSR/RTS/RTR/RTE/JMP/Bcc/BRA address error checks.
**Result**: All passing 8065/8065.

### RTE SR-sync fix
**Fix**: Apply stacked SR before address error check.
**Result**: RTE 8065/8065.

### ASR C/X flags when shift count >= operand size
**Fix**: Silicon-accurate flag computation.
**Result**: ASR.b/l 8065/8065.

### DBcc / DBF address error check
**Fix**: Added `m68ki_check_pc_address_error_010_less()` to DBF handler.
**Result**: DBcc 8065/8065.

### Pre-decrement -(An) R/W mode in address error frame
**Fix**: Changed `MODE_WRITE` to `MODE_READ` in `m68ki_ea_ay_pd_32()` and
`m68ki_ea_ax_pd_32()`.
**Result**: 15 .l files now pass (ADD.l, ADDA.l, AND.l, CLR.l, CMP.l, etc).

### Binary test harness — recursive exception fix + move.bin
**Fix**: Route ALL exception vectors (2–63) to TEST_FAIL in setup_bootsec.
Fix 68040-only CMPI PC-relative encoding in move.s to use LEA+CMP.
**Result**: Binary tests 60/60 (was 59/60).

### MOVEM (An)+ address register on address error
**Fix**: Pre-increment AY by 2 before each read in MOVEM.w/l (An)+
handlers. On address error (siglongjmp), AY reflects ea+2 (one word
past fault address). For .l, increment is +2 not +4 because the 68000
does two 16-bit bus cycles for a 32-bit access.
**Result**: MOVEM.w 8065/8065, MOVEM.l 8065/8065.

### DIVS/DIVU overflow flags
**Fix**: Clear C flag on overflow (V=1, C=0). N/Z are undefined on
overflow — real silicon preserves them from pre-division state.
**Result**: DIVS 8065/8065, DIVU 8064/8065 (1 unrelated EA fault).

---

## Remaining: MOVE.l / MOVE.w destination EA fault (1,454 failures)

- MOVE.l (756): Destination (An)+/-(An) address register wrong on write fault
- MOVE.w (698): Same destination EA fault pattern

When `MOVE.l src, (An)+` or `MOVE.l src, -(An)` triggers an address error
on the destination write, the address register reflects the post-increment/
pre-decrement state when real silicon expects it unchanged (or partially
updated). Requires restructuring EA computation to defer register update
past the write.

---

## Remaining: MOVEfromSR (2,521 failures)

Stack frame + addressing issues. MOVEfromSR writes SR to a memory EA;
if that EA triggers an address error, the stack frame content is wrong.

---

## Remaining: DIVU — 1 isolated failure

Vector 5745 in DIVU: `SR expected=0x2710 got=0x271D`. This is an
unrelated EA fault issue for `(d16, A7)` addressing mode.

---

## WONTFIX: ASL.b — 2 corrupt Tom Harte vectors

Vectors 1583 and 1761 in `ASL.b.json.gz` have corrupt expected D2 values.
Test data bug, not a Musashi bug.
