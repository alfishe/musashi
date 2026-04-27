# Musashi 68000 SST Parity — TODO

## Current SST Baseline (2026-04-27)

| Metric | Value |
|--------|-------|
| SST vectors passing | 992,113 / 1,000,060 |
| SST files passing | 116 / 124 |
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

### DBcc / DBF address error check (251 failures)
**Fix**: Added `m68ki_check_pc_address_error_010_less()` to DBF handler (was
present in DBcc but missing from DBF/DBRA).
**Result**: DBcc 8065/8065.

### Pre-decrement -(An) R/W mode in address error frame (~8,000 failures)
**Fix**: Changed `MODE_WRITE` to `MODE_READ` in `m68ki_ea_ay_pd_32()` and
`m68ki_ea_ax_pd_32()`. Real silicon reports MODE_READ because the faulting
bus cycle is always a read (data read or RMW read phase).
**Result**: 15 .l files now pass: ADD.l, ADDA.l, AND.l, CLR.l, CMP.l, CMPA.l,
EOR.l, MOVEA.l, NEG.l, NEGX.l, NOT.l, OR.l, SUB.l, SUBA.l, TST.l.

### Binary test harness — recursive exception fix + move.bin
**Fix**: Route ALL exception vectors (2–63) to TEST_FAIL in setup_bootsec.
Fix 68040-only CMPI PC-relative encoding in move.s to use LEA+CMP.
**Result**: Binary tests 60/60 (was 59/60).

---

## Priority 1: MOVEM (821 failures)

- MOVEM.w (426): `A0 exp=0x5B2BA541 got=0x5B2BA53F` — 2-byte delta
- MOVEM.l (395): `A0 exp=0x91772AAD got=0x91772AAB` — 2-byte delta

MOVEM has its own post-increment logic that doesn't go through the generic
`m68ki_ea_ax_pi_*` functions. Needs its own register update fix.

---

## Priority 2: MOVE.l / MOVE.w (1,454 failures)

- MOVE.l (756): Mix of stack frame content and register issues
- MOVE.w (698): Address register + SR + stack frame issues

These are partially related to addressing mode interactions (source→destination
with address error on destination write).

---

## Priority 3: MOVEfromSR (2,521 failures)

Stack frame + addressing issues. MOVEfromSR writes the SR value to a
memory EA; if that EA triggers an address error, the stack frame content
may be wrong.

---

## Priority 4: DIVS / DIVU SR flags (3,149 failures)

- DIVS (1,900): `SR exp=0x2702 got=0x2703` — V or C flag wrong
- DIVU (1,249): `SR exp=0x2716 got=0x2717` — V or C flag wrong

Single-bit flag differences in division results. Separate from addressing
mode issues.

---

## Priority 5: ASL.b — 2 corrupt Tom Harte vectors (WONTFIX)

Vectors 1583 and 1761 in `ASL.b.json.gz` have corrupt expected D2 values —
all 32 bits change for a byte-size shift, which is architecturally impossible.
Test data bug, not a Musashi bug.

---

## Low Priority: SST loader — YAML exclusion config

Add `test/singlestep/sst_exclusions.yaml` to mark known-bad test vectors
(like ASL.b 1583/1761) as excluded.
