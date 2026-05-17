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

### AERR: Pre-Fault Cycle Tracking

**Problem:** All AERR show `got=50` but expected = 50 + EA cycles consumed before fault.

**Root Cause (from Yacht.txt):** The 50 cycles is exception processing only. EA calculation
cycles consumed before the fault must be added.

**EA Cycle Overheads (68000, word operand):**
| Mode | Cycles |
|------|-------:|
| (An), (An)+  | 4 |
| -(An)        | 6 |
| (d16,An)     | 8 |
| (d8,An,Xn)   | 10 |
| (xxx).W      | 8 |
| (xxx).L      | 12 |

**Implementation Plan:**
1. Add `uint m68ki_aerr_cycles` global variable in m68kcpu.c
2. Reset to 0 at instruction start (in execute loop)
3. Accumulate EA cycles in `m68ki_get_ea_*` functions before memory access
4. In `m68ki_exception_address_error()`: `USE_CYCLES(50 + m68ki_aerr_cycles)`

**Files to modify:**
- `m68kcpu.c`: Add variable, reset in execute loop
- `m68kcpu.h`: Declare extern, update exception handler
- `m68kops.c` / `m68k_in.c`: Update EA calculation functions to accumulate cycles

**Complexity:** Medium - localized to EA calculation paths, ~20 functions to update.

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
