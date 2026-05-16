# Musashi 68000 Parity TODO

## Current Status

**Correctness: 99.9998%** (1,000,058 / 1,000,060 vectors pass)
**Cycle accuracy: 86.8%** (868,390 / 1,000,060 vectors match)

---

## Remaining Issues

### 1. ASL.b "Failures" (2 vectors) — TEST DATA BUG ✓
- Both failing vectors have corrupted expected values in tomharte test suite
- Upper 24 bits of D2 change when ASL.b only affects low 8 bits
- **Musashi is correct; test vectors are wrong**
- No fix needed

### 2. Cycle Mismatches (131,670 vectors)

| Category | Count | % | Fix Complexity |
|----------|------:|--:|----------------|
| AERR (got=50) | 131,669 | 99.999% | ⭐⭐⭐⭐ Major |
| Other | 1 | 0.001% | Edge case |

**DIVU/DIVS: FIXED** — Implemented Jorge Cwik's cycle-accurate division algorithm

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
- [x] DIVU/DIVS cycle-accurate timing (Jorge Cwik algorithm)
- [x] DIVS overflow +2 when dividend negative
