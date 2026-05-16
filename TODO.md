# Musashi 68000 Parity TODO

## Current Status

**Correctness: 99.9998%** (1,000,058 / 1,000,060 vectors pass)
**Cycle accuracy: 86.3%** (863,012 / 1,000,060 vectors match)

---

## Remaining Issues

### 1. ASL.b "Failures" (2 vectors) — TEST DATA BUG ✓
- Both failing vectors have corrupted expected values in tomharte test suite
- Upper 24 bits of D2 change when ASL.b only affects low 8 bits
- **Musashi is correct; test vectors are wrong**
- No fix needed

### 2. Cycle Mismatches (137,048 vectors)

| Category | Count | % | Fix Complexity |
|----------|------:|--:|----------------|
| AERR (got=50) | 131,669 | 96.1% | ⭐⭐⭐⭐ Major |
| DIVS normal | 3,030 | 2.2% | ⭐⭐⭐ Hard |
| DIVU normal | 2,349 | 1.7% | ⭐⭐⭐ Hard |

---

## Root Cause Analysis (2026-05-16)

### DIVU/DIVS: Complex Timing Model Mismatch

**Problem:** The 68000 division timing depends on the iterative subtract-shift algorithm,
not just quotient popcount. Current Musashi model is a simplified approximation.

**Attempted fixes:**
1. `USE_CYCLES(2 * popcount)` — Original, gives 3644/8065 DIVU, 2974/8065 DIVS
2. `USE_CYCLES(32 - 2 * popcount)` — Inverted, made things worse
3. `USE_CYCLES(16 - 2 * popcount)` — Centered, DIVU +95, DIVS -107

**Root cause:** The base cycle values in the table (DIVU=108, DIVS=120) don't match
the actual 68000 minimum/maximum cycle ranges (DIVU: 76-140, DIVS: 122-158).
The mismatch means no simple popcount formula can be correct.

**Accurate fix would require:**
- Changing base cycle values in the instruction table
- Implementing actual 68000 non-restoring division algorithm timing
- Per-iteration cycle tracking based on dividend/divisor relationship

**Current best:** Original popcount formula (adds cycles for 1-bits), ~45% cycle accuracy for DIV.

### AERR: Per-Instruction Cycle Tracking

**Problem:** All AERR show `got=50` but expected ranges 50-72 based on:
- Cycles consumed before fault (EA calculations already done)
- Bus cycle phase when error occurred
- Source/dest addressing mode complexity

**Fix would require:**
- Tracking accumulated cycles in CPU state variable
- Updating every EA calculation path to increment counter
- Exception handler reads accumulated value at longjmp time

**Complexity:** Major infrastructure change, touches all EA paths.

---

## Completed Fixes

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
- [x] CHK data-dependent: +2 when src<0 && src<=bound
