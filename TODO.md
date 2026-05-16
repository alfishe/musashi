# Musashi 68000 Parity TODO

## Current Status

**Correctness: 99.9998%** (1,000,058 / 1,000,060 vectors pass)
**Cycle accuracy: 86.3%** (863,012 / 1,000,060 vectors match)

---

## Remaining Failures

### 1. Correctness Failures (2 vectors)

| Instruction | Failures | Issue |
|-------------|----------|-------|
| ASL.b | 2 | Flag edge case in shift logic |

### 2. Cycle Mismatches (137,048 vectors)

| Category | Count | % | Status |
|----------|------:|--:|--------|
| AERR (got=50) | 131,669 | 96.1% | Needs per-instruction AERR accounting |
| DIVS normal | 3,030 | 2.2% | Popcount formula imprecise |
| DIVU normal | 2,349 | 1.7% | Popcount formula imprecise |
| **Total** | **137,048** | | |

---

## Task Details

### ASL.b Flag Edge Case
- 2 failures out of 8,065 vectors
- Non-AERR correctness issue in shift logic
- Low priority (0.02% of tests)

### DIVU/DIVS Normal Division Timing
- 5,379 vectors total (DIVS: 3,030 + DIVU: 2,349)
- Current popcount formula gives approximate cycles
- Real 68000 uses iterative subtract-shift algorithm
- Exact timing depends on dividend/divisor relationship, not just quotient popcount
- Would require accurate 68000 division microcode emulation
- Medium priority

### AERR Per-Instruction Cycle Accounting
- 131,669 vectors (96% of all cycle mismatches)
- All show `got=50` because `CYC_EXCEPTION[EXCEPTION_ADDRESS_ERROR]` is flat 50
- Real hardware produces 50–62 cycles depending on instruction progress when exception fires
- Requires tracking how much work completed before address error
- Major infrastructure work
- Low priority (functional correctness unaffected)

---

## Completed Fixes (2026-05-16)

- [x] TAS memory base 14→10
- [x] Byte immediate +2 removal
- [x] ADDQ.w An base 4→8
- [x] OR.l Dn,Dn base 6→8
- [x] SUBA.l Dn/An base 6→8
- [x] MULS Booth encoding fix
- [x] BTST Dn,#imm +2 cycle fix
- [x] ADDA.l Dn/An base 6→8
- [x] ADDA/SUBA.w #imm remove +2 bonus
- [x] ADD.l/SUB.l Dn/An base 6→8
- [x] ADDQ/SUBQ.l An base 8→6
- [x] AND.l Dn,Dn base 6→8 + ANDI.l 14→16
- [x] BCHG/BCLR/BSET.32 bit>=16 +2
- [x] ADD.w/SUB.w #imm remove +2 bonus
- [x] DIVU/DIVS overflow early-exit (10/16 cycles)
- [x] DIVU/DIVS popcount formula
- [x] CHK data-dependent: +2 when src<0 && src<=bound
