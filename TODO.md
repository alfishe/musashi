# Musashi 68000 Parity TODO

## Current Status

**Correctness: 99.9998%** (1,000,058 / 1,000,060 vectors pass)
**Cycle accuracy: 99.89%** (998,985 / 1,000,060 vectors match)

---

## Completed Fixes

### DIVU/DIVS Timing ✓
Implemented Jorge Cwik's cycle-accurate division algorithm.

### AERR Pre-Fault Cycle Tracking ✓
Comprehensive tracking for address error pre-fault cycles:

**EA Computation Overhead:**
- Predecrement -(An): +2 cycles
- Displacement (d16,An): +4 cycles
- Indexed (d8,An,Xn): +6 cycles
- Absolute word (xxx).W: +4 cycles
- Absolute long (xxx).L: +8 cycles
- PC-relative: +4/+6 cycles

**Control Flow AERR:**
- JMP/JSR: half EA overhead for simple modes (prefetch overlap)
- RTS: +8, RTR/RTE: +12 (stack reads)
- BSR/Bcc/DBcc: +2 (branch overhead)

**Result:** AERR mismatches reduced from 131,670 to 0.

---

## Remaining Issues (~0.1%)

### 1. ASL.b "Failures" (2 vectors) — TEST DATA BUG
- Musashi is correct; test vectors corrupted
- No fix needed

### 2. MOVE Base Timing (~1,075 vectors)
- MOVE.l/MOVE.w with complex EA combinations
- Non-AERR timing issues (base instruction cycles)
- Example: MOVE.w -(A7),(d16,A0): delta=-6

### 3. DIVU Edge Case ✓ FIXED
- Divide-by-zero exception now includes EA fetch cycles

---
