# SingleStepTests Harness Design

## Goal

Provide a high-performance, turnkey verification environment that runs architectural parity tests against Musashi using datasets from two independent sources:

- **Source A**: [SingleStepTests/680x0](https://github.com/SingleStepTests/680x0) (TomHarte) — ~1.08M vectors, gzipped JSON
- **Source B**: [SingleStepTests/m68000](https://github.com/SingleStepTests/m68000) (raddad772) — ~237K vectors, proprietary binary

**Total Coverage: 1,317,560 test vectors.**

---

## Source Comparison

| Feature | 680x0 (TomHarte) | m68000 (raddad772) |
|---------|-------------------|-------------------|
| Format | gzipped JSON | Binary `.json.bin` |
| Vectors/file | ~8,000 | ~1,000 |
| Total vectors | ~1,080,000 | ~237,000 |
| RAM format | byte-addressed | 16-bit words (split) |
| Bus cycles | r/w/n/t | r/w/n/t/re/we |
| PC semantics | next instruction | next prefetch addr |

### Why Both?

- **Cross-validation**: If both independent sources agree on a result, the architectural "ground truth" is verified.
- **Error Coverage**: raddad772 tests include address error scenarios (`re`/`we`) that exercise exception frame parity.
- **Randomization**: Different sources use different random seeds for register/RAM state, increasing the probability of hitting corner cases.

---

---

## Architecture Overview

The harness is split into three decoupled components:

1. **Converter (`sst_convert.py`)**: An offline tool that normalizes both JSON and proprietary binary formats into a unified, high-performance `.sst` binary format.
2. **Loader (`sst_loader.c/h`)**: A lightweight, zero-allocation C library for streaming `.sst` vectors from disk.
3. **Runner (`sst_runner.c`)**: A standalone C binary that embeds the Musashi core and executes the tests.
4. **Bootstrap (`sst_runner.sh`)**: A user-friendly wrapper that handles cloning, building, and executing the full suite.

---

## Directory Structure

```
musashi/
├── test/
│   ├── singlestep/
│   │   ├── README.md                # Entry point and usage
│   │   ├── sst_runner.sh            # Zero-Setup bootstrap script
│   │   ├── build/                   # CMake build tree (gitignored)
│   │   ├── data/                    # Raw test repositories (gitignored)
│   │   │   ├── 680x0/               # SingleStepTests/680x0 (TomHarte)
│   │   │   │   └── 68000/v1/*.json.gz
│   │   │   └── m68000/              # SingleStepTests/m68000 (raddad772)
│   │   │       └── v1/*.json.bin
│   │   ├── unified/                 # Normalized .sst files (gitignored)
│   │   │   ├── tomharte/            # Source A output
│   │   │   │   └── ADD_b.sst
│   │   │   └── raddad/              # Source B output
│   │   │       └── ADD_b.sst
│   │   ├── reports/                 # Auto-generated failure reports
│   │   ├── sst_loader.h             # .sst format parser
│   │   ├── sst_loader.c
│   │   ├── sst_runner.c             # Custom harness binary
│   │   └── CMakeLists.txt           # Build system
│   └── entry.s                      # 68k test scaffold (for other tests)
├── tools/
│   └── sst_convert.py               # Dual-source converter
└── doc/testing/
    ├── testing_strategy.md
    ├── test_plan_overview.md
    ├── test_plan_integer.md
    ├── test_plan_fpu.md
    ├── test_plan_mmu_030.md
    ├── test_plan_mmu_040.md
    ├── test_plan_system.md
    ├── test_plan_singlestep.md
    └── singlestep_harness_design.md # This document
```

---

## Unified SST Binary Format

The `.sst` format is optimized for **sequential streaming** and **cache locality**. It avoids pointers, heap allocation, and complex parsing.

### Byte Order
All multi-byte values use **Native Host Endianness**. The format is optimized for the machine performing the verification.

### Header (12 bytes + mnemonic)
- `char[4] magic`: "SST1"
- `uint32 num_vectors`: Total tests in this file
- `uint8  source_id`: 0 = TomHarte, 1 = raddad772
- `uint8  name_len`: Length of mnemonic string
- `char[] mnemonic`: Instruction name (e.g., "MOVE.l")

### Test Vector (Repeated)
- `uint8  vec_name_len`: Length of unique vector name
- `char[] vec_name`: Full name (e.g., "MOVE.l (A0)+, D1")
- `uint32 initial_regs[19]`: D0-D7, A0-A6, USP, SSP, SR, PC
- `uint32 final_regs[19]`: Expected results
- `uint16 num_ram_initial`: Initial memory entries
- `uint16 num_ram_final`: Expected memory changes
- `struct { uint32 a; uint8 v; } ram_initial[SST_MAX_RAM]`
- `struct { uint32 a; uint8 v; } ram_final[SST_MAX_RAM]`

---

## Normalization Rules

To unify the sources, `sst_convert.py` applies the following transformations:

| Field | TomHarte | raddad772 | Unified (.sst) |
|-------|----------|-----------|---------------|
| **RAM** | `[addr, byte]` | `[addr, word]` | Split words into 2 byte entries |
| **PC** | next instruction | next prefetch (PC+4) | Adjusted to next-instruction boundary |
| **Regs** | native uint32 | native uint32 | Host-endian uint32 |

---

## Execution Philosophy

### Zero-Setup (Bootstrap)
The `sst_runner.sh` script detects if the binary or data is missing. If so, it:
1. Detects the platform (macOS/Linux/WSL).
2. Guides the user through `cmake`/`gcc` installation if needed.
3. Automatically clones raw data if `test/singlestep/data/` is empty.
4. Triggers `sst_convert.py` to populate `test/singlestep/unified/`.
5. Builds the runner and executes with `--all --summary` by default.

### Standalone Runner
The `sst_runner` binary can be used directly for deep debugging:
- `--verbose`: Shows every vector as it runs.
- `--stop-on-fail`: Halts immediately on the first mismatch.
- `--report-all=DIR`: Generates JSON, YAML, Markdown, and HTML reports for regression analysis.
- `--max-vectors=N`: Limits processing to N vectors per file for quick smoke testing.

---

## Verification Rules

| Field | Check | Rationale |
|-------|:-----:|-----------|
| D0-D7 | ✅ | Architectural state parity |
| A0-A6 | ✅ | Architectural state parity |
| USP/SSP| ✅ | Stack pointer parity (critical for supervisor mode) |
| SR/CCR| ✅ | Full 16-bit flag parity |
| PC    | ✅ | Normalized to next-instruction boundary |
| RAM   | ✅ | Memory side-effects |
| Cycles| ❌ | Musashi is not cycle-exact; captured but not verified |
| Bus   | ❌ | Bus transaction timing is not modeled |

---

## Known Discrepancies (The "Gap")

The harness currently tracks an **~80.7% pass rate**. Known failure categories include:
1. **Address Errors**: Discrepancies in the stack frame layout when hitting unaligned `.w` or `.l` operands.
2. **BCD Arith**: Edge-case flag behaviors in `ABCD`/`SBCD`.
3. **SSP Mismatches**: Specific supervisor stack frame differences during exceptions.
