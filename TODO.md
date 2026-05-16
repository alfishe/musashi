# Musashi 68000 Parity TODO

## 🎯 Current Objective
Achieve 100% architectural and cycle-accuracy parity with the M68000, as verified by the Tom Harte SingleStepTests (SST).

**Cycle accuracy: 86.3%** (863,012 / 1,000,060 vectors match tomharte cycle counts).

| Category | Mismatches | % of Total |
|----------|----------:|----------:|
| AERR (got=50) | 131,676 | 96.1% |
| Non-AERR | 5,372 | 3.9% |
| **Total** | **137,048** | |

---

## 📊 The "Big Picture" (Current Test Status)

**Latest run**: `test/singlestep/reports/2026-05-15_003912/`

### tomharte (primary)
- **1,000,058 / 1,000,060 vectors passed (99.9998%)**
- 124 files, 123 pass, 1 file with failures
- Only failing file: **ASL.b** (2 failures — flag edge case)

### raddad (secondary, 67 files blacklisted)
- **149,514 / 150,000 vectors passed (99.7%)**
- 60 files tested (67 blacklisted due to tomharte contradictions)
- Only failing file: **ADDA.l** (486 failures — reference data contradiction)

### raddad blacklist (67 files skipped)
The following raddad test files are blacklisted because their reference data
contradicts tomharte on AERR stack frame details (PC offset, FC field, register
restore). Tomharte is treated as authoritative for the 68000.

<details>
<summary>Blacklisted raddad instructions (click to expand)</summary>

ADDA.w, ADDX.l, ADDX.w, ADD.l, ADD.w, AND.l, AND.w, ASL.w, ASR.b, ASR.l,
ASR.w, CHK, CLR.l, CLR.w, CMPA.l, CMPA.w, CMP.l, CMP.w, DBcc, DIVS, DIVU,
EOR.l, EOR.w, ILLEGAL_LINEF, JMP, JSR, LINK, LSL.w, LSR.w, MOVEA.l, MOVEA.w,
MOVEM.l, MOVEM.w, MOVE.l, MOVE.w, MOVEfromSR, MOVEtoCCR, MOVEtoSR, MULS,
MULU, NEGX.l, NEGX.w, NEG.l, NEG.w, NOT.l, NOT.w, OR.l, OR.w, ROL.w, ROR.w,
ROXL.w, ROXR.w, RTE, RTR, RTS, STOP, SUBA.l, SUBA.w, SUBX.l, SUBX.w, SUB.l,
SUB.w, TST.l, TST.w, UNLINK, Bcc, BSR

</details>

---

## 📋 Tasks

### 1. ASL.b Flag Edge Case [0.02% of tomharte]
- [ ] **ASL.b**: 2 failures out of 8,065 vectors — flag edge case, not AERR-related
  - This is a non-AERR correctness issue in the shift logic

### 2. raddad ADDA.l Contradiction [486 failures]
raddad expects different AERR behavior from tomharte for ADDA.l with memory-source
EA modes reading from odd addresses. Three distinct contradictions:

| Category | Count | raddad expects | tomharte expects | Status |
|----------|------:|----------------|------------------|--------|
| PC offset | ~347 | REG_PC (offset=2) | REG_PC-2 (offset=0) | Blocked |
| FC field | ~40 | FC=program | FC=data | Blocked |
| Register restore | ~139 | pi restored | pi NOT restored | Blocked |

**Resolution**: Cannot fix without regressing tomharte. Requires hardware verification
(on real 68000 silicon) to determine which reference dataset is correct.

The m68kmake token substitution infrastructure is in place to fix this instantly
once the correct behavior is determined (see `doc/concepts/opcode-generation.md` §7).

### 3. Cycle Accuracy Improvements
Cycle verification infrastructure is live (`sst_runner --cycles`). Current status:

#### Completed (Phase 1+3)
- [x] **TAS memory base fix**: Base 14→10 in `m68k_in.c`. Fixed 8,065 vectors (100% TAS match)
- [x] **Byte immediate +2 removal**: Guard `op->size != 8` in `m68kmake.c`. Fixed ADD/AND/OR/SUB.b #imm,Dn (279 vectors)
- [x] **ADDQ.w An fix**: Base 4→8 in `m68k_in.c`. Fixed 2,398 vectors
- [x] **OR.l Dn,Dn fix**: Base 6→8 in `m68k_in.c`. Fixed ~2,868 vectors
- [x] **SUBA.l Dn/An fix**: Base 6→8 in `m68k_in.c`. Fixed ~3,987 vectors
- [x] **Gtest cycle tests**: 23 tests across 5 suites covering TAS, bitops, byte-imm, ADDQ, register-long

#### Completed (Phase 5 — data-dependent + register-long cycle fixes)
- [x] **MULS Booth encoding fix**: Variable-cycle signed multiply bit-transition counting. Fixed +2,524 vectors
- [x] **BTST Dn,#imm +2 cycle fix**: Register-count immediate-EA variant. Fixed +138 vectors
- [x] **CHK data-dependent exception cycles**: Trap cost depends on src/bound signs on 68000. Fixed +3,146 vectors (675 remaining)
- [x] **ADDA.l Dn/An base 6→8**: Register-source ADDA.l takes 8 cycles on 68000. Fixed +2,112 vectors
- [x] **ADDA/SUBA.w #imm remove +2 bonus**: Word-immediate ADDA/SUBA don't get the +2 internal cycle. Fixed +290 vectors
- [x] **ADD.l/SUB.l Dn/An base 6→8**: Register-source long ADD/SUB takes 8 cycles on 68000. Fixed +2,320 vectors
- [x] **ADDQ/SUBQ.l An base 8→6**: ADDQ.l/SUBQ.l #imm,An takes only 6 cycles. Fixed +731 vectors

#### Remaining (Phase 6 — root cause analysis 2026-05-16)

**Remaining cycle mismatches breakdown (137,730 total):**

| Category | Count | % | Status |
|----------|------:|--:|--------|
| AERR (got=50) | 131,676 | 96.1% | Blocked on per-instruction AERR accounting |
| DIVS normal | 3,030 | 2.2% | Popcount formula imprecise |
| DIVU normal | 2,349 | 1.7% | Popcount formula imprecise |

**Completed fixes:**
- [x] **AND.l Dn,Dn base 6→8 + ANDI.l 14→16**: Fixed 676 vectors (2026-05-16)
- [x] **BCHG/BCLR/BSET.32 bit>=16 +2**: Runtime check for upper word bit ops. Fixed 1,922 vectors (2026-05-16)
- [x] **ADD.w/SUB.w #imm**: Removed incorrect +2 bonus for word-immediate ALU ops. Fixed 107 vectors (2026-05-16)
- [x] **DIVU overflow early-exit**: 10 cycles base instead of 108. Fixed ~1,933 vectors (2026-05-16)
- [x] **DIVS overflow early-exit**: 16 cycles base instead of 120. Fixed ~1,855 vectors (2026-05-16)
- [x] **DIVU/DIVS popcount**: Variable cycles based on quotient bit population. Partial fix.

**Remaining non-AERR issues:**
- [ ] **DIVU/DIVS normal division timing**: 5,379 vectors. Current popcount formula is imprecise.
  Real 68000 uses iterative subtract-shift algorithm; exact cycle count depends on dividend/divisor
  relationship, not just quotient popcount. Would need accurate 68000 division microcode emulation.
- [x] **CHK data-dependent cycles**: Fixed 682 vectors. +2 cycles when src < 0 AND src <= bound
  (pure underflow), +0 when src > bound (overflow, even if src < 0). (2026-05-16)

#### Remaining (Phase 4 — AERR cycle mechanism)
- [ ] **AERR per-instruction cycle accounting**: 131,676 mismatches (95.6% of total). All show `got=50`
  because `CYC_EXCEPTION[EXCEPTION_ADDRESS_ERROR]` is a flat 50 for all instructions. Real hardware
  produces different totals (50–62) depending on how much instruction work completed before
  the exception fires. This is the largest remaining issue but requires significant infrastructure.

### 4. Testability Infrastructure Improvements

#### High Impact
- [ ] **`make sst` target**: Add Makefile target to run SST regression gate with one command
- [x] **`--aerr-only` filter**: Filter to only show vectors that trigger address errors for fast AERR iteration
- [x] **`--no-aerr` filter**: Filter to only show non-AERR cycle mismatches
- [ ] **`--baseline` diff mode**: Compare new run against a baseline report, show delta (+N new, -M fixed)

#### Medium Impact
- [x] **Cycle-count verification**: SST runner now compares tomharte cycle counts via `--cycles` flag
- [ ] **`--baseline` diff mode**: Compare new run against a baseline report, show delta (+N new, -M fixed)

#### Low Impact
- [ ] **Expand gtest coverage**: Framework has BCD (15 tests) + cycle tests (23 tests); directories for arithmetic/bitop/branch/logic/move/shift_rotate still mostly empty
- [ ] **Missing instruction SST coverage**: 8 base 68000 instructions have no SST data at all: `addi`, `addq`, `cmpi`, `cmpm`, `moveq`, `subi`, `subq`, `unlk` (upstream data gap)

---

## 📊 SST Coverage Map

### By instruction (68000 base, 71 mnemonics)

| Status | Count | Details |
|--------|------:|---------|
| tomharte 100% pass | 68 | All vectors pass |
| tomharte has failures | 1 | ASL.b (2 failures) |
| tomharte missing, raddad covers | 1 | STOP (but raddad blacklisted) |
| Not covered by any SST | 8 | addi, addq, cmpi, cmpm, moveq, subi, subq, unlk |
| raddad-only (not blacklisted) | 60 | Pass at 99.7% (ADDA.l 486 fail) |
| raddad blacklisted | 67 | Contradicts tomharte on AERR details |

### By test count

| Suite | Files | Vectors | Passed | Failed |
|-------|------:|--------:|-------:|-------:|
| tomharte | 124 | 1,000,060 | 1,000,058 | 2 |
| raddad (active) | 60 | 150,000 | 149,514 | 486 |
| raddad (blacklisted) | 67 | — | — | — |
