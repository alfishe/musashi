# Musashi 68000 SST Parity — TODO

## Current SST Baseline (2026-04-27)

| Metric | Value |
|--------|-------|
| SST vectors passing | 983,582 / 1,000,060 |
| SST files passing | 100 / 124 |
| Binary tests (mc68000) | 59 / 60 (move.bin fails) |

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

---

## Priority 1: DBcc (251 failures)

`SSP exp=0x000007F2 got=0x00000800` — stack frame pushed when it shouldn't be,
or wrong size. The 68000 may handle DBcc address errors differently than a
standard Group 0 exception.

---

## Priority 2: MOVEM (821 failures)

- MOVEM.w (426): `A0 exp=0x5B2BA541 got=0x5B2BA53F` — 2-byte delta
- MOVEM.l (395): `A0 exp=0x91772AAD got=0x91772AAB` — 2-byte delta

MOVEM has its own post-increment logic that doesn't go through the generic
`m68ki_ea_ax_pi_*` functions. Needs its own register update fix.

---

## Priority 3: MOVE.l / MOVE.w (1,912 failures)

- MOVE.l (1,214): Mix of stack frame content (X-flag) and register issues
- MOVE.w (698): Address register + SR + stack frame issues

These are partially the X-flag-on-stack issue but also have MOVE-specific
addressing mode interactions (source→destination with address error on
destination write).

---

## Priority 4: MOVEfromSR (2,521 failures)

Stack frame + addressing issues. MOVEfromSR writes the SR value to a
memory EA; if that EA triggers an address error, the stack frame content
may be wrong.

---

## Priority 5: DIVS / DIVU SR flags (3,149 failures)

- DIVS (1,900): `SR exp=0x2702 got=0x2703` — V or C flag wrong
- DIVU (1,249): `SR exp=0x2716 got=0x2717` — V or C flag wrong

Single-bit flag differences in division results. Separate from addressing
mode issues.

---

## Priority 6: X-flag on stack (~8,000 failures across 15 .l files)

When an address error occurs during a `.l` instruction using `-(An)`, the SR
pushed onto the exception stack frame has the wrong X flag (bit 4). This
affects: ADD.l, AND.l, CLR.l, CMP.l, CMPA.l, EOR.l, NEG.l, NEGX.l, NOT.l,
OR.l, SUB.l, SUBA.l, ADDA.l, TST.l, MOVEA.l.

Pattern: `RAM@0x0007F3 exp=0xB5 got=0xA5` (bit 4 = X flag wrong).

---

## Priority 7: ASL.b — 2 corrupt Tom Harte vectors (WONTFIX)

Vectors 1583 and 1761 in `ASL.b.json.gz` have corrupt expected D2 values —
all 32 bits change for a byte-size shift, which is architecturally impossible.
Test data bug, not a Musashi bug.

---

## Low Priority: SST loader — YAML exclusion config

Add `test/singlestep/sst_exclusions.yaml` to mark known-bad test vectors
(like ASL.b 1583/1761) as excluded.
