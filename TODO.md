# Musashi 68000 Parity TODO

## Current Status

**Correctness: 99.9998%** (1,000,058 / 1,000,060 vectors pass)
**Cycle accuracy: 99.1%** (1,139,289 / 1,150,060 vectors match)

---

## Completed Fixes

### DIVU/DIVS Timing ✓
Implemented Jorge Cwik's cycle-accurate division algorithm.

### AERR Pre-Fault Cycle Tracking ✓ (Phase 1)
Added `m68ki_aerr_cycles` tracking for:
- Predecrement modes: +2 cycles
- Displacement modes (d16,An): +4 cycles  
- Indexed modes (d8,An,Xn): +6 cycles
- Absolute word (xxx).W: +4 cycles
- Absolute long (xxx).L: +8 cycles
- PC-relative modes: +4/+6 cycles
- Immediate operands: +4/+8 cycles

Control flow AERR (JMP/JSR/RTS/RTE/RTR/BSR/Bcc/DBcc) handled with special cases:
- Simple EA modes use half cycles (prefetch overlap)
- RTS: +8, RTR/RTE: +12 (stack reads)
- Branches: +2 (displacement overhead)

**Result:** Control flow instructions now 100% cycle accurate.

---

## Remaining Issues (~0.9%)

### 1. ASL.b "Failures" (2 vectors) — TEST DATA BUG
- Musashi is correct; test vectors have corrupted expected values
- No fix needed

### 2. Dual-Operand AERR Tracking (~10,771 vectors)

| Category | Count | Example |
|----------|------:|---------|
| ADDX/SUBX -(An),-(An) | ~6,100 | exp=52, got=50 |
| CMPM (An)+,(An)+ | ~380 | exp=58, got=50 |
| MOVE indexed src | ~2,600 | exp=64, got=56 |
| ADDA postinc | ~825 | exp=58, got=50 |
| DIVU edge case | 1 | exp=46, got=38 |

**Root Cause:** Current tracking doesn't distinguish source vs destination EA.
When AERR occurs on destination operand, source EA cycles should be included.

**Implementation Plan:**
1. Track source EA cycles separately from destination EA cycles
2. In AERR handler, determine which operand faulted based on instruction phase
3. Add source cycles only when AERR is on destination access

---
