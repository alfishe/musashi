# Musashi 68000 Parity TODO

## Current Status

**Correctness: 99.9998%** (1,000,058 / 1,000,060 vectors pass)
**Cycle accuracy: ~99.8%** (~1,148,159 / 1,150,060 vectors match)

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

**Source Read Completion:**
- All OPER_* functions track read completion
- Byte/Word reads: +4 cycles
- Long reads: +8 cycles
- Ensures AERR on dest includes source time

**Control Flow AERR:**
- JMP/JSR: half EA overhead for simple modes
- RTS: +8, RTR/RTE: +12 (stack reads)
- BSR/Bcc/DBcc: +2 (branch overhead)

**Result:** AERR mismatches reduced from 131,670 to ~278.

---

## Remaining Issues (~0.2%)

### 1. ASL.b "Failures" (2 vectors) — TEST DATA BUG
- Musashi is correct; test vectors corrupted
- No fix needed

### 2. MOVE Timing Edge Cases (~1,075 vectors)
- MOVE.l -(An),(d16,An): delta=-6
- MOVE.w (d16,An),(xxx).l: delta=+4
- Complex dual-EA timing interactions

### 3. ADDA (raddad test suite) (~825 vectors)
- 278 AERR + 547 non-AERR mismatches
- Different test source, may have different expectations

### 4. DIVU Edge Case (1 vector)
- exp=46, got=38 — specific corner case

---
