# Cycle-Count Verification — Infrastructure & Baseline Analysis

## Goal

Add cycle-count verification to the SST test harness, establishing a baseline of how
Musashi's instruction timing compares to real 68000 hardware as recorded by the
TomHarte SingleStepTests reference data.

---

## Infrastructure Changes

### SST Binary Format Upgrade (SST1 → SST2)

The SST binary format was extended with per-vector cycle counts:

| Field | SST1 | SST2 |
|-------|------|------|
| Magic | `"SST1"` | `"SST2"` |
| Per-vector `expected_cycles` | — | `uint16` (0 = not available) |

The loader (`sst_loader.c`) accepts both formats transparently — SST1 files have
`expected_cycles = 0` (ignored).

### Cycle Data Sources

| Source | Field | Meaning |
|--------|-------|---------|
| tomharte | `length` | Sum of bus transaction cycle counts = total instruction clock cycles |
| raddad | `num_cycles` | Same — total clock cycles from bus trace |

Both represent the **total number of clock cycles** consumed by a single instruction
execution on real 68000 silicon.

### Files Modified

| File | Change |
|------|--------|
| `test/singlestep/sst_loader.h` | Added `uint16 expected_cycles` to `sst_vector_t` |
| `test/singlestep/sst_loader.c` | Reads SST2 `expected_cycles` field; backward-compatible with SST1 |
| `tools/sst_convert.py` | Captures `length` from tomharte JSON, `num_cycles` from raddad binary; writes SST2 |
| `test/singlestep/sst_runner.c` | Added `--cycles` flag; captures `m68k_execute(1)` return value; per-file cycle stats |

### Usage

```bash
# Full cycle audit with summary
./sst_runner --all --source=tomharte --cycles --summary

# Single file with per-vector cycle details
./sst_runner ADD_b --cycles --verbose

# raddad cycle audit
./sst_runner --all --source=raddad --cycles --summary
```

Cycle mismatches are reported **separately** from functional failures — they do not
affect pass/fail status.

---

## Baseline Results (tomharte, 1,000,060 vectors)

### Overall

| Metric | Value |
|--------|------:|
| Vectors checked | 1,000,060 |
| Cycle match | 834,057 (**83.4%**) |
| Cycle mismatch | 166,003 (16.6%) |
| Files with 100% match | 53 / 124 |
| Files with mismatches | 71 / 124 |

### By Operand Size

| Size | Files | Match Rate | Perfect Files |
|------|------:|-----------:|--------------:|
| `.b` (byte) | 22 | **99.8%** | 18 / 22 |
| `.w` (word) | 29 | **79.5%** | 2 / 29 |
| `.l` (long) | 29 | **78.5%** | 10 / 29 |
| no suffix | 43 | **80.5%** | 22 / 43 |

Byte-sized operations are nearly perfect. Word and long diverge due to EA cycle
costs and prefetch timing.

### By Instruction Category

| Category | Match Rate | Vectors Wrong | Root Cause |
|----------|-----------:|--------------:|------------|
| BCD (ABCD/SBCD/NBCD) | **100.0%** | 0 | Simple RMW byte ops, model is exact |
| Extend/Swap/Exchange | **100.0%** | 0 | Register-only, no EA timing |
| Link/Unlink | **100.0%** | 0 | Simple fixed-cycle stack ops |
| System (NOP/TRAP/PEA/LEA/RESET) | **100.0%** | 0 | Fixed-cycle instructions |
| Shift/Rotate | **97.5%** | 4,839 | Word-size EA cycle costs off by 2-6 |
| Bit Manipulation | **93.7%** | 2,046 | Memory EA cycle undercount |
| Negate (NEG/NEGX/NOT/CLR) | **83.5%** | 16,015 | Word/long memory EA cycles |
| Misc (TAS/Scc/etc) | **82.5%** | 12,737 | TAS overcounts by 4 |
| Compare/Test | **82.4%** | 11,371 | Word/long EA cycle delta |
| Move (MOVE/MOVEA/MOVEM) | **79.8%** | 24,427 | MOVEM reg-list not modeled; EA cycles |
| Logic (AND/OR/EOR) | **79.3%** | 15,018 | Word/long memory EA cycles |
| Arithmetic (ADD/SUB/ADDA) | **74.5%** | 32,916 | ADDA/SUBA long wrong base; EA cycles |
| Branch/Call (Bcc/BSR/DBcc/JMP/JSR) | **65.8%** | 13,787 | Prefetch cycles not modeled for taken |
| Return (RTS/RTR/RTE) | **49.8%** | 12,138 | Stack readback cycles not modeled |
| Multiply/Divide (MULS/MULU/DIVS/DIVU) | **35.8%** | 20,709 | Variable-cycle ops use fixed cost |

### 53 Cycle-Perfect Instructions

These instructions match real hardware cycles for **all** 8,065 test vectors each:

```
ABCD        ADDX.b      ANDItoCCR   ANDItoSR    ASL.b       ASL.l
ASR.b       ASR.l       CLR.b       CMP.b       EOR.b       EORItoCCR
EORItoSR    EXG         EXT.l       EXT.w       LEA         LINK
LSL.b       LSL.l       LSR.b       LSR.l       MOVE.b      MOVE.q
MOVEfromUSP MOVEP.l     MOVEP.w     MOVEtoUSP   NBCD        NEG.b
NEGX.b      NOP         NOT.b       ORItoCCR    ORItoSR     PEA
RESET       ROL.b       ROL.l       ROR.b       ROR.l       ROXL.b
ROXL.l      ROXR.b      ROXR.l      SBCD        SUBX.b      SWAP
Scc         TRAP        TRAPV       TST.b       UNLINK
```

Common traits: register-only, byte-sized, or simple fixed-cycle operations.

### Top 10 Worst Cycle Accuracy

| Instruction | Match Rate | Vectors Wrong | Typical Delta |
|-------------|-----------:|--------------:|---------------|
| DIVS | 12.5% | 7,053 | Variable (depends on operands) |
| DIVU | 12.8% | 7,036 | Variable |
| TAS | 15.3% | 6,828 | +4 (overcounts 18 vs 14) |
| CHK | 26.7% | 5,909 | Variable (exception path) |
| MULS | 42.9% | 4,605 | Variable |
| RTS | 49.7% | 4,057 | -8 (undercounts, no prefetch) |
| RTE | 49.7% | 4,054 | -12 (undercounts, no prefetch) |
| RTR | 50.1% | 4,027 | -12 (undercounts, no prefetch) |
| SUBA.l | 50.6% | 3,987 | -2 to -6 (EA undercount) |
| BSR | 50.7% | 3,980 | -8 to -12 (prefetch) |

---

## Root Cause Analysis

### Cause 1: Simplified EA Cycle Table (~60% of mismatches)

Musashi uses a single per-opcode entry in `m68ki_cycles[]` plus a fixed EA index
table (`m68ki_ea_idx_cycle_table[]`). The real 68000 charges different cycle costs
per EA mode:

- `(An)` — 0 extra cycles (register indirect)
- `(An)+` / `-(An)` — 4 extra cycles (pre/post decrement)
- `(d16,An)` — 4-8 extra cycles (displacement fetch)
- `(d8,An,Xn)` — 4-10 extra cycles (extension word decode)
- `(xxx).w` / `(xxx).l` — 4-8 extra cycles (absolute address fetch)

Musashi's table doesn't distinguish all of these, so word/long memory operations
with complex EA modes are often off by 2-8 cycles.

**Affected**: All word/long arithmetic, logic, negate, compare, move instructions
with memory operands. ~100K vectors.

### Cause 2: Missing Prefetch Overlap (~25% of mismatches)

The 68000 prefetches the next instruction word(s) during the current instruction's
execution. This overlap reduces total cycle count by 2-4 cycles per instruction in
many cases. Musashi counts cycles linearly without prefetch overlap.

**Affected**: Branch/Call instructions (Bcc taken, BSR, JMP, JSR) and Return
instructions (RTS, RTR, RTE). These all undercount by 8-12 cycles because the
real 68000 was doing prefetch during the memory accesses for the branch target
or return address stack reads. ~45K vectors.

### Cause 3: Variable-Cycle Instructions (~12% of mismatches)

DIVS, DIVU, MULS, MULU have cycle counts that depend on operand values:

- **DIVU**: 116-140 cycles depending on quotient leading zeros
- **DIVS**: 122-154 cycles depending on operand magnitude and sign
- **MULS**: 38-70 cycles depending on the number of 1-bits in multiplier
- **MULU**: 38-70 cycles depending on the number of 1-bits

Musashi uses a fixed cost per opcode. Since SST test vectors randomize operands,
most vectors get the "wrong" cycle count.

**Affected**: DIVS (12.5%), DIVU (12.8%), MULS (42.9%), MULU (75.0%). ~20K vectors.

### Cause 4: TAS Overcount (~3% of mismatches)

TAS performs a read-modify-write bus cycle. Musashi counts 18 cycles but real
hardware takes 14. The 4-cycle overcount suggests Musashi charges extra for the
"set" part of the read-modify-write sequence.

**Affected**: TAS (15.3% match). ~7K vectors.

---

## Potential Improvements

### Tier 1: High Impact, Moderate Effort

1. **Per-EA-mode cycle table**: Replace the flat `m68ki_cycles[]` lookup with a
   2D table `[opcode][ea_mode]` that captures real hardware timing per EA mode.
   This could fix ~60% of all mismatches (the EA cycle cost issue).

2. **Prefetch cycle modeling**: Add 2-4 cycle credit for instruction prefetch
   overlap. A simple heuristic (subtract 2 for non-branch instructions) would
   fix many of the word/long near-misses.

### Tier 2: Moderate Impact, High Effort

3. **Variable-cycle MUL/DIV**: Implement operand-dependent cycle counting for
   MULS/MULU/DIVS/DIVU based on bit analysis of the operands.

4. **Branch prefetch**: Model the prefetch behavior for taken branches and
   subroutine calls/returns more accurately.

### Tier 3: Low Impact

5. **TAS fix**: Simple 4-cycle correction in the cycle table.
6. **CHK exception path**: Model the different cycle costs for the exception path.

---

## Data Files

- Report: `test/singlestep/reports/2026-05-14_210041/report.json`
- Converted SST2 data: `test/singlestep/unified/tomharte/*.sst` and `raddad/*.sst`

---

## Deep Delta Analysis

Sampled 200 vectors per instruction with `--cycles --verbose` to capture actual
(Musashi - reference) delta distributions.

### How Musashi Counts Cycles

`m68k_execute(1)` sets a budget of 1 cycle, then the instruction handler calls
`USE_CYCLES(n)` which subtracts `n` from remaining. The return value is
`1 - remaining`, i.e. total consumed. Two components:

1. **Base cost**: `CYC_INSTRUCTION[opcode]` from the `m68ki_cycles[0][0x10000]`
   table, populated by `m68ki_build_opcode_table()` from per-handler entries.
   This is a **single value per opcode** regardless of EA mode.
2. **EA extra cost**: Runtime `USE_CYCLES()` calls for indexed EA
   (`m68ki_ea_idx_cycle_table[extension & 0x3f]`), MOVEM register counts
   (`count << CYC_MOVEM_W/L`), shift counts, etc.

Most opcodes have base cost around 4-8 for register ops and 8-14 for memory ops.
The `USE_CYCLES()` extras add 0-13 more. The net result is often 50 for many
memory-destination instructions (base + EA + overhead).

### Delta Pattern Groups

Four distinct delta patterns emerged from sampling 71 imperfect instructions:

#### Group A: Constant Overcount (+2 to +4) — 10 instructions

Musashi always returns more cycles than real hardware. The delta is constant per
instruction, meaning the cycle table entry is simply too high.

| Instruction | Delta | Musashi → Real | Vectors affected | Fix complexity |
|-------------|------:|----------------|------------------:|----------------|
| TAS | +4 | 18 → 14 | 6,828 | Trivial |
| BCHG | +2 | 8 → 6 | 643 | Trivial |
| BCLR | +2 | 10 → 8 | 604 | Trivial |
| BSET | +2 | 8 → 6 | 661 | Trivial |
| ADD.b (imm) | +2 | 10 → 8 | 53 | Trivial |
| SUB.b (imm) | +2 | 10 → 8 | 66 | Trivial |
| AND.b (imm) | +2 | 10 → 8 | 86 | Trivial |
| OR.b (imm) | +2 | 10 → 8 | 74 | Trivial |

**Fix**: Subtract the delta from the cycle table entry. Single-line change per
instruction in the `m68k_opcode_handler_table[]` in `m68k_in.c`.

#### Group B: Constant Undercount (-8 to -12) — 3 instructions

Musashi returns significantly fewer cycles. These are return instructions where
the real 68000 does extra prefetch/stack-read bus cycles that Musashi doesn't model.

| Instruction | Delta | Musashi → Real | Vectors affected | Fix complexity |
|-------------|------:|----------------|------------------:|----------------|
| RTS | -8 | 50 → 58 | 4,057 | Easy |
| RTE | -12 | 50 → 62 | 4,054 | Easy |
| RTR | -12 | 50 → 62 | 4,027 | Easy |

**Fix**: Add `USE_CYCLES(n)` calls in the handler to account for stack readback
and prefetch. Or increase the cycle table entry by the delta.

#### Group C: Variable Undercount (-2 to -12) — 50+ instructions

The dominant group. Musashi undercounts by a variable amount depending on EA mode.
The delta correlates with EA complexity: register ops match perfectly, but memory
ops with displacement/indexed/absolute addressing lose 2-12 cycles.

Typical pattern for one instruction (e.g., ADD.w):

```
  delta=-2 x 2   (simple memory EA, minor miss)
  delta=-4 x 6   (displacement EA)
  delta=-6 x 1   (indexed EA)
```

| Sub-group | Delta range | Representative instructions | Vectors |
|-----------|------------:|---------------------------|--------:|
| Branch prefetch | -2 | Bcc, DBcc | 4,164 |
| Call/branch prefetch | -6 to -10 | BSR, JMP, JSR | 9,623 |
| EA minor miss (.w/.l mem) | -2 to -4 | ADDX.w, SUBX.w, ADDA.l, SUBA.l | ~16K |
| EA moderate miss | -4 to -6 | ADD.w, SUB.w, AND.w, OR.w, MOVE.w/.l | ~50K |
| EA + MOVEM overhead | -4 to -12 | MOVEM.w, MOVEM.l | 7,626 |
| CMP long (heavy EA) | -4 to -8 | CMP.l | 1,931 |

**Root cause**: The cycle table has one entry per opcode. Real 68000 charges
different cycle costs per EA mode. Musashi picks a middle value that's right
for some EA modes but wrong for others.

**Fix**: Per-EA-mode cycle adjustment via `USE_CYCLES()` in each handler, or
expand the cycle table to be 2D `[opcode][ea_mode]`.

#### Group D: Variable-Cycle (no fixed delta) — 4 instructions

MUL/DIV cycle count depends on operand bit patterns. Fixed delta is impossible.

| Instruction | Match rate | Real range | Musashi value | Vectors affected |
|-------------|-----------:|------------|---------------|-----------------:|
| DIVS | 12.5% | 122-154 | 50 (base) | 7,053 |
| DIVU | 12.8% | 116-140 | 50 (base) | 7,036 |
| MULS | 42.9% | 38-70 | 50-52 | 4,605 |
| MULU | 75.0% | 38-70 | 50 | 2,015 |

**Fix**: Implement operand-dependent cycle counting:
- MULU/MULS: count leading zeros or ones in multiplier
- DIVU/DIVS: model iterative division hardware

### Per-Instruction Delta Detail

Delta = Musashi output minus reference value. Negative = Musashi undercounts.
Sampled from 200 vectors per instruction.

```
Instruction   Wrong   Dominant deltas (most frequent first)
----------   -----   -----------------------------------------
DIVS          7053   variable +142, +26, +14, -4, -6
DIVU          7036   variable +26, +22, +16, +18, -8
TAS           6828   +4 (constant)
CHK           5909   -2 to -8 (mixed EA)
MULS          4605   -2 x5, -4 x3, -6 x2
RTS           4057   -8 (constant)
RTE           4054   -12 (constant)
RTR           4027   -12 (constant)
SUBA.l        3987   -2 x7, -4 x2, -6 x1
BSR           3980   -10 (constant)
ADDA.l        3884   -2 x7, -4 x3
MOVEM.l       3842   -4 x6, -8 x3, -10 x1
MOVEM.w       3784   -4 x5, -8 x4, -12 x1
MOVE.l        3603   -2 x2, -4 x3, -6 x3, -8 x2
MOVE.w        3487   -2 x2, -4 x2, -6 x3, -8 x2
SUB.l         3148   -2 x5, -6 x2, +2 x2
ADD.l         3140   -2 x5, -4 x3, -6 x2
AND.l         2929   -2 x4, -4 x4, -6 x1
OR.l          2868   -2 x3, -4 x2, -6 x5
JSR           2865   -2 x4, -6 x6
JMP           2778   -2 x5, -6 x5
SUBX.w        2678   -2 x7, -6 x3
ADDX.w        2600   -2 x6, -6 x4
ADDX.l        2593   -2 x8, -10 x2
SUBX.l        2584   -2 x4, -10 x6
AND.w         2408   -2 x4, -4 x2, -6 x3
ADD.w         2398   -2 x2, -4 x6, -6 x1
OR.w          2331   -4 x2, -6 x8
Bcc           2200   -2 (constant)
EOR.l         2169   -2 x2, -4 x3, -6 x3
EOR.w         2153   -2 x1, -4 x4, -6 x4
MOVEtoSR      2096   -2 x2, -4 x4, -6 x4
TST.w         2054   -2 x4, -6 x4, -8 x1
MOVEtoCCR     2041   -2 x2, -4 x5, -6 x2
TST.l         2038   -2 x4, -4 x5, -6 x1
NEGX.l        2029   -2 x5, -4 x2, -6 x3
SUB.w         2024   -2 x3, -4 x4, -6 x3
NEG.w         2023   -2 x2, -4 x5, -6 x3
NOT.w         2023   -2 x4, -4 x2, -6 x4
MULU          2015   -2 x2, -4 x3, -6 x4
CLR.l         2010   -2 x2, -4 x4, -6 x4
NOT.l         2010   -2 x2, -4 x6, -6 x2
CLR.w         2008   -2 x2, -4 x6, -6 x2
MOVEfromSR    1968   -2 x2, -4 x3, -6 x5
NEG.l         1965   -2 x2, -4 x3, -6 x5
DBcc          1964   -2 (constant)
NEGX.w        1947   -2 x3, -4 x3, -6 x4
CMP.l         1931   -2 x2, -4 x2, -6 x2, -8 x3
ADDA.w        1887   -2 x2, -4 x2, -6 x4
SUBA.w        1874   -2 x3, -4 x6, -6 x1
CMP.w         1865   -2 x1, -4 x5, -8 x2
MOVEA.w       1806   -2 x3, -4 x3, -6 x4
MOVEA.l       1800   -2 x5, -4 x2, -6 x3
CMPA.w        1755   -2 x4, -4 x4, -6 x2
CMPA.l        1728   -2 x2, -4 x1, -6 x7
BSET           661   +2 (constant)
ROL.w          648   -2 x5, -4 x1, -6 x3, -8 x1
BCHG           643   +2 (constant)
LSR.w          631   -4 x6, -6 x4
BCLR           604   +2 (constant)
LSL.w          603   -2 x5, -4 x3, -6 x2
ROXL.w         603   -4 x5, -6 x4, -8 x1
ASR.w          600   -2 x3, -4 x2, -6 x5
ASL.w          599   -2 x3, -4 x6, -6 x1
ROR.w          580   -2 x6, -4 x1, -6 x3
ROXR.w         575   -2 x3, -4 x5, -6 x1, -8 x1
BTST           138   -2 (constant)
AND.b           86   +2 (constant, imm only)
OR.b            74   +2 (constant, imm only)
SUB.b           66   +2 (constant, imm only)
ADD.b           53   +2 (constant, imm only)
```

---

## Prioritized Action Plan

### Approach

Each phase is **self-contained and verifiable**: implement, rebuild, run
`--cycles --summary`, measure improvement. No phase depends on a later phase.

Current baseline: **834,057 / 1,000,060 match (83.4%)**

### Phase 1: Trivial Table Fixes (+13,555 vectors, ~1 hour)

**Target**: 847,612 / 1,000,060 (84.8%)

Fix constant-delta instructions by adjusting cycle table entries in
`m68k_opcode_handler_table[]` in `m68k_in.c`.

| # | Instruction | Change | File location | Vectors fixed |
|---|-------------|--------|---------------|--------------:|
| 1 | TAS | cycles 18→14 | table entry | 6,828 |
| 2 | BCHG mem | cycles 8→6 | table entry | 643 |
| 3 | BCLR mem | cycles 10→8 | table entry | 604 |
| 4 | BSET mem | cycles 8→6 | table entry | 661 |
| 5 | BTST mem | +2 in handler | handler `USE_CYCLES(2)` | 138 |

Note: ADD.b/SUB.b/AND.b/OR.b (+2 delta) only affect the immediate EA mode
(#$imm), which is just 53-86 vectors each. These are not worth a separate fix
since the immediate-mode cycle table entry is shared with register-to-register
mode that already matches. **Skip for now.**

### Phase 2: Return Instruction Fixes (+12,138 vectors, ~1 hour)

**Target**: 859,750 / 1,000,060 (86.0%)

RTS/RTR/RTE have constant deltas. The cycle table base covers the instruction
but not the prefetch/readback overhead. Add `USE_CYCLES()` in each handler.

| # | Instruction | Delta | Fix | Vectors fixed |
|---|-------------|------:|-----|--------------:|
| 6 | RTS | -8 | Add `USE_CYCLES(8)` in `m68k_op_rts` handler | 4,057 |
| 7 | RTE | -12 | Add `USE_CYCLES(12)` in `m68k_op_rte` handler | 4,054 |
| 8 | RTR | -12 | Add `USE_CYCLES(12)` in `m68k_op_rtr` handler | 4,027 |

**Implementation**: In `m68kops.c` (generated), find each handler and add
the `USE_CYCLES()` call at the end of the handler body. Since `m68kops.c` is
generated from `m68k_in.c`, the fix goes into the template in `m68k_in.c`.

### Phase 3: Branch/Call Constant Fixes (+14,144 vectors, ~2 hours)

**Target**: 873,894 / 1,000,060 (87.4%)

Bcc, DBcc, BSR have constant or near-constant deltas. The delta represents
prefetch cycles that Musashi doesn't account for when the branch is taken.

| # | Instruction | Delta | Fix approach | Vectors fixed |
|---|-------------|------:|-------------|--------------:|
| 9 | Bcc taken | -2 | `USE_CYCLES(2)` in taken path | 2,200 |
| 10 | DBcc | -2 | `USE_CYCLES(2)` in loop-taken path | 1,964 |
| 11 | BSR | -10 | `USE_CYCLES(10)` after stack push | 3,980 |

**Caveat**: Bcc and DBcc deltas may not be perfectly constant — the -2 we
measured could be a simplification. Need to verify the taken/not-taken split
and EA mode correlation before fixing. BSR's -10 appears truly constant.

**Implementation strategy**: Add the `USE_CYCLES()` call in the "taken" branch
of each handler in `m68k_in.c`. The not-taken path should remain unchanged
(not-taken cycles are already correct per our data).

### Phase 4: JMP/JSR Prefetch Fix (+5,643 vectors, ~1 hour)

**Target**: 879,537 / 1,000,060 (88.0%)

JMP and JSR have a split delta: -2 and -6 each about half the time. The -2
likely corresponds to register-indirect EA (already close) and -6 to memory
indirect / absolute EA (more prefetch cycles).

| # | Instruction | Delta | Fix approach | Vectors fixed |
|---|-------------|------:|-------------|--------------:|
| 12 | JMP | -2 or -6 | Split by EA mode: add 2-6 cycles per EA | 2,778 |
| 13 | JSR | -2 or -6 | Same approach as JMP | 2,865 |

**Implementation**: These are tricky because the delta varies by EA mode.
Options:
1. Add `USE_CYCLES()` per EA branch in the handler — most accurate
2. Use an average of 4 extra cycles — simpler but won't be perfect

The per-EA approach is recommended since we're already modifying handlers.

### Phase 5: EA Cycle Cost Table Expansion (~80K vectors, ~1 day)

**Target**: 960,000 / 1,000,060 (96.0%)

This is the highest-impact single change. The problem: `m68ki_cycles[0][opcode]`
has one entry per opcode regardless of EA mode. The real 68000 charges:

| EA Mode | Extra cycles (vs register) |
|---------|--------------------------:|
| Dn / An | 0 |
| (An) | 0 |
| (An)+ | 4 |
| -(An) | 6 |
| (d16,An) | 8 |
| (d8,An,Xn) | 10-14 |
| (xxx).w | 8 |
| (xxx).l | 12 |
| #$imm | 4 (word) / 8 (long) |

**Approach**: Rather than expanding the table to 2D (which would be 65536 x 12
entries), add EA-mode-aware `USE_CYCLES()` calls in the EA helper functions
in `m68kcpu.h`. The EA helpers already exist (`OPER_AY_AI_8()`, `OPER_AY_PI_8()`,
etc.) — we add `USE_CYCLES(n)` to each one based on the real 68000 timing.

Affected instructions: all Group C instructions (~50+ opcodes, ~80K vectors).

**Steps**:
1. Build an EA-mode-to-extra-cycles table from 68000 reference data
2. Add `USE_CYCLES()` to each `OPER_AY_xx` and `EAAY_xx` macro in `m68kcpu.h`
3. Reduce the base cycle table entries to their register-mode values
   (since the EA helpers now add the correct extra cost)
4. Rebuild and verify with `--cycles --summary`

**Risk**: The EA helpers are called from many handlers. Changing cycle costs
in the helpers affects all instructions uniformly, which could break instructions
that currently match. Need careful before/after comparison.

### Phase 6: MUL/DIV Variable-Cycle (~23K vectors, ~1 day)

**Target**: 983,000 / 1,000,060 (98.3%)

MULU, MULS, DIVU, DIVS have data-dependent cycle counts. Musashi uses fixed cost.

**MULU algorithm** (68000 reference):
```
cycles = 38 + 2 * popcount(multiplier)
```
Where `popcount` = number of 1-bits in the 16-bit multiplier.
Range: 38 (multiplier=0) to 70 (multiplier=0xFFFF).

**MULS algorithm**:
```
cycles = 38 + 2 * popcount(positive_multiplier)
```
Same formula with the absolute value of the multiplier.

**DIVU algorithm**:
```
cycles = ~116-140 depending on quotient leading zeros
```
Iterative restoration divider: depends on operand magnitudes.

**DIVS algorithm**:
```
cycles = ~122-154 depending on operand magnitudes and signs
```

**Implementation**: Replace fixed `USE_CYCLES(50)` in the MUL/DIV handlers
with computed cycle cost. The MUL formula is straightforward. DIV requires
a more detailed model or a lookup-based approximation.

### Phase 7: CHK Exception Path (~5,909 vectors, ~2 hours)

**Target**: 988,909 / 1,000,060 (98.9%)

CHK has variable delta (-2 to -8) depending on whether it takes the exception
or not. The exception path has different cycle costs. Need to investigate the
split between normal-completion and exception-triggered vectors.

### Phase 8: Remaining Polish (~11K vectors, ongoing)

Remaining mismatches after phases 1-7:
- MOVEM register-list overhead not perfectly modeled (~7K)
- Byte immediate EA +2 deltas for ADD.b/SUB.b/AND.b/OR.b (~280)
- Edge cases in shift/rotate EA costs (~4K)
- Any residuals from Phase 5 imperfect EA modeling

### Execution Summary

```
Phase   Fix                                 Vectors   Cumulative Match   Effort
-----   ----------------------------------   -------   ----------------   ------
 1      Trivial table fixes (TAS/BCHG/etc)    13,555   83.4% → 84.8%      ~1h
 2      Return prefetch (RTS/RTE/RTR)        12,138   84.8% → 86.0%      ~1h
 3      Branch/call prefetch (Bcc/DBcc/BSR)  14,144   86.0% → 87.4%      ~2h
 4      JMP/JSR prefetch split                 5,643   87.4% → 88.0%      ~1h
 5      EA cycle cost table expansion         ~80,000   88.0% → 96.0%      ~1d
 6      MUL/DIV variable cycles               ~23,000   96.0% → 98.3%      ~1d
 7      CHK exception path                     ~5,909   98.3% → 98.9%      ~2h
 8      Remaining polish                      ~11,151   98.9% → ~100%       ongoing
-----   ----------------------------------   -------   ----------------
Total                                           166,000   83.4% → 100%
```

### Key Files to Modify

| File | Phase | What changes |
|------|-------|-------------|
| `m68k_in.c` | 1-4, 6-8 | Cycle table entries, handler `USE_CYCLES()` |
| `m68kcpu.h` | 5 | EA helper macros get `USE_CYCLES()` per mode |
| `m68kcpu.c` | 5 (maybe) | `m68ki_ea_idx_cycle_table[]` values |
| `m68kmake.c` | — | No changes (table builder not affected) |

**Build flow**: Edit `m68k_in.c` → run `m68kmake` → generates `m68kops.c/h` →
rebuild `m68kops.o` → relink `sst_runner`. Verify with:
```bash
./sst_runner --all --source=tomharte --cycles --summary 2>&1 | tee report.txt
```
