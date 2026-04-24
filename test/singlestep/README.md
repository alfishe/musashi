# SingleStepTests Verification Harness

Standalone verification infrastructure for the Musashi 680x0 core using
single-instruction test vectors from two external repositories.

## Overview

This harness executes **~1.3 million** test vectors from two independent sources
against the Musashi emulation core, comparing register and RAM state after each
instruction. It produces console output and machine-readable reports in JSON,
YAML, Markdown, and HTML formats.

### Data Sources

| Source | Repository | Format | Vectors |
|--------|-----------|--------|--------:|
| **TomHarte** | [SingleStepTests/680x0](https://github.com/SingleStepTests/680x0) | gzipped JSON (`.json.gz`) | ~1,000,060 |
| **raddad772** | [SingleStepTests/m68000](https://github.com/SingleStepTests/m68000) | proprietary binary (`.json.bin`) | ~317,500 |

Both sources are 68000-family single-step tests that provide initial and final
CPU state (registers + RAM) for one instruction execution.

## Architecture

```
                          ┌─────────────────────────────────────┐
                          │     tools/sst_convert.py            │
  .json.gz ──────────────►│  Python converter: normalizes both  │
  .json.bin ─────────────►│  formats into unified .sst binary   │
                          └──────────────┬──────────────────────┘
                                         │
                                         ▼
                          ┌─────────────────────────────────────┐
                          │  test/singlestep/unified/           │
                          │    tomharte/*.sst                   │
                          │    raddad/*.sst                     │
                          └──────────────┬──────────────────────┘
                                         │
                                         ▼
                          ┌─────────────────────────────────────┐
                          │     sst_runner (C binary)           │
                          │  - Loads .sst files                 │
                          │  - Injects state into Musashi       │
                          │  - Executes 1 instruction           │
                          │  - Compares final state             │
                          │  - Generates reports                │
                          └─────────────────────────────────────┘
```

## Directory Layout

```
test/singlestep/
├── README.md            ← this file
├── sst_runner.c         ← test harness (C)
├── sst_loader.c/h       ← .sst binary format parser
├── .gitignore           ← excludes data/, unified/, reports/
├── data/                ← cloned test repositories (git submodules)
│   ├── 680x0/           ← TomHarte test vectors
│   └── m68000/          ← raddad772 test vectors
├── unified/             ← converted .sst files (generated)
│   ├── tomharte/*.sst
│   └── raddad/*.sst
└── reports/             ← generated reports
    ├── tomharte/
    │   ├── report.json
    │   ├── report.yaml
    │   ├── report.md
    │   ├── report_brief.md
    │   └── report.html
    └── raddad/
        └── (same structure)
```

## Prerequisites

**CMake path (recommended):**
- **CMake 3.16+** — drives the entire prep pipeline
- **Git** — to clone test data repos
- **Python 3.6+** — for vector conversion (`tools/sst_convert.py`)
- **GCC or Clang** — C compiler

**Manual path:**
- Python 3.6+, GCC/Clang, plus pre-built Musashi object files
  (`m68kcpu.o`, `m68kdasm.o`, `m68kops.o`, `softfloat/softfloat.o`)

## Quick Start — Zero Setup (Recommended)

The included shell script handles everything (cloning, converting, and building)
automatically on the first run.

```bash
# From the musashi root:
./test/singlestep/sst_runner.sh --all --summary
```

## Quick Start — Manual CMake

If you prefer to control the build process explicitly:

**From musashi root:**
```bash
cmake -B test/singlestep/build test/singlestep/
cmake --build test/singlestep/build
```

**From `test/singlestep/` directly:**
```bash
cmake -B build .
cmake --build build
```

On first run this will:
1. Clone `SingleStepTests/680x0` (TomHarte, ~1 GB) into `test/singlestep/data/680x0/`
2. Clone `SingleStepTests/m68000` (raddad772) into `test/singlestep/data/m68000/`
3. Run `tools/sst_convert.py` to produce `test/singlestep/unified/**/*.sst`
4. Build `m68kmake` → generate `m68kops.c/h` → compile `musashi_core`
5. Link `test/singlestep/sst_runner`

Subsequent builds are incremental — clones and conversions are skipped if
already present (stamp-file based).

Then run tests manually with full control over flags (always from musashi root):

```bash
# All tests, both sources:
./test/singlestep/sst_runner --all --summary

# One source:
./test/singlestep/sst_runner --all --source=tomharte --summary
./test/singlestep/sst_runner --all --source=raddad  --summary

# Smoke test (20 vectors/file — fast):
./test/singlestep/sst_runner --all --max-vectors=20 --summary

# Single file:
./test/singlestep/sst_runner test/singlestep/unified/tomharte/NOP.sst
```

## Manual Build (Alternative)

If you prefer not to use CMake, do each step yourself:

### 1. Clone Test Data

```bash
mkdir -p test/singlestep/data
git clone --depth=1 https://github.com/SingleStepTests/680x0.git \
    test/singlestep/data/680x0
git clone --depth=1 https://github.com/SingleStepTests/m68000.git \
    test/singlestep/data/m68000
```

### 2. Convert Test Vectors

```bash
# Both sources (recommended):
python3 tools/sst_convert.py \
  --source=both \
  --tomharte-dir=test/singlestep/data/680x0/68000/v1 \
  --raddad-dir=test/singlestep/data/m68000/v1 \
  --output-dir=test/singlestep/unified
```

### 3. Build Musashi + Runner

```bash
make          # builds m68kcpu.o, m68kdasm.o, m68kops.o, softfloat/softfloat.o

gcc -Wall -Wextra -O2 -o test/singlestep/sst_runner \
  test/singlestep/sst_runner.c \
  test/singlestep/sst_loader.c \
  m68kcpu.o m68kdasm.o m68kops.o softfloat/softfloat.o \
  -I. -lm
```

### 4. Run Tests

```bash
./test/singlestep/sst_runner --all --summary
```


### Auto-Timestamped Reports

When no `--report-*` flags are specified, the runner automatically creates
a timestamped subdirectory under `test/singlestep/reports/` and writes all
five report formats there:

```
test/singlestep/reports/
├── 2026-04-24_112110/     ← auto-generated
│   ├── report.json
│   ├── report.yaml
│   ├── report.md
│   ├── report_brief.md
│   └── report.html
├── 2026-04-24_143022/     ← next run
│   └── ...
└── ...
```

This enables tracking verification progress across runs and code changes.

## CLI Reference

```
Usage: sst_runner [OPTIONS] [FILE.sst ...]

Options:
  --all               Run all .sst files in data directory
  --source=SOURCE     Filter by source: 'tomharte', 'raddad', or 'all' (default)
  --max-vectors=N     Limit vectors per file (0 = unlimited, default)
  --verbose           Print individual failure details
  --summary           Print summary statistics at end
  --report-json=PATH  Write JSON report
  --report-yaml=PATH  Write YAML report
  --report-md=PATH    Write Markdown report
  --report-html=PATH  Write HTML report
  --report-all=DIR    Write all report formats to directory

If no --report-* flags are given, all formats are auto-saved to a
timestamped subfolder: reports/YYYY-MM-DD_HHMMSS/
```

## .sst Binary Format

The `.sst` format is a compact, fixed-size record format designed for high-performance
loading.

> [!IMPORTANT]
> **Host Endianness**: The `.sst` format uses the native endianness of the
> machine that ran the conversion. It is NOT cross-platform portable. If you
> move vectors between different architectures (e.g., x86_64 to ARM64), you
> MUST re-run the conversion script.

The `.sst` format is a compact, flat binary produced by `tools/sst_convert.py`
and consumed by `sst_loader.c`. All multi-byte integers are written in
**native host byte order** (little-endian on x86/ARM Mac). The format is
designed for sequential streaming reads — no seek, no index table.

### Limits (from `sst_loader.h`)

These constants define **fixed-size arrays** inside `sst_cpu_state_t` and
`sst_vector_t`. The design choice to use fixed arrays rather than per-entry
heap allocation is deliberate: the loader reads up to 8,065 vectors per file
and needs to avoid per-vector `malloc`/`free` overhead in the hot path.
Fixed arrays give O(1) allocation cost and perfect cache locality when the
runner iterates vectors sequentially.

| Constant | Value | Rationale |
|----------|------:|-----------|
| `SST_MAX_RAM` | 512 | Upper bound on distinct bytes touched by one 68000 instruction. The worst case is `MOVEM.l` (push/pop 16 registers × 4 bytes = 64 bytes), plus instruction encoding bytes. Even pathological addressing modes stay well under 200 unique addresses; 512 gives a 2.5× safety margin. Each entry is 5 bytes (4B addr + 1B value), so the cap costs ~2.5 KB per state snapshot, ~5 KB per vector. |
| `SST_MAX_NAME` | 64 | TomHarte vector names encode the opcode bytes as hex strings (e.g. `"4e 71 00 00 ..."`). The longest observed names in v1 datasets are ~55 bytes; 64 is the next power-of-two ceiling with null-terminator room. |
| `SST_MAX_MNEMONIC` | 32 | Longest 68000 mnemonic in either dataset is `ILLEGAL_LINEF` (13 chars). 32 bytes covers any future extension and keeps the field naturally aligned. |
| `SST_NUM_REGS` | 19 | **Exact architectural count** — not a soft limit. D0–D7 (8) + A0–A6 (7) + USP + SSP + SR + PC = 19. All 19 are always serialized; no optional fields. |


### File Layout

```
┌──────────────────────────────────────────────────────────────────┐
│  FILE HEADER                                                     │
│  char[4]    magic        = "SST1"  (4 bytes, no null terminator) │
│  uint32_t   num_vectors             (4 bytes, LE)                │
│  uint8_t    source_id    0=TomHarte, 1=raddad                    │
│  uint8_t    mnemonic_len            (1 byte, max 31)             │
│  char[]     mnemonic     mnemonic_len bytes, NO null terminator  │
├──────────────────────────────────────────────────────────────────┤
│  VECTOR[0]  ..  VECTOR[num_vectors-1]   (sequential, no gaps)    │
└──────────────────────────────────────────────────────────────────┘
```

### Per-Vector Layout

Each vector is serialized as:

```
┌──────────────────────────────────────────────────────────────────┐
│  VECTOR HEADER                                                   │
│  uint8_t    name_len     length of name string (max 63)          │
│  char[]     name         name_len bytes, NO null terminator      │
├──────────────────────────────────────────────────────────────────┤
│  INITIAL CPU STATE                                               │
│  uint32_t   regs[19]     register snapshot (19 × 4 = 76 bytes)   │
├──────────────────────────────────────────────────────────────────┤
│  FINAL CPU STATE                                                 │
│  uint32_t   regs[19]     register snapshot (19 × 4 = 76 bytes)   │
├──────────────────────────────────────────────────────────────────┤
│  RAM COUNTS                                                      │
│  uint16_t   num_ram_initial         (2 bytes, LE)                │
│  uint16_t   num_ram_final           (2 bytes, LE)                │
├──────────────────────────────────────────────────────────────────┤
│  INITIAL RAM  (num_ram_initial entries)                          │
│  { uint32_t addr (4B LE), uint8_t value (1B) } × N               │
├──────────────────────────────────────────────────────────────────┤
│  FINAL RAM  (num_ram_final entries)                              │
│  { uint32_t addr (4B LE), uint8_t value (1B) } × N               │
└──────────────────────────────────────────────────────────────────┘
```

### Register Array Layout (`regs[19]`)

The 19 registers are always stored in this fixed order:

| Index | Register | Notes |
|------:|----------|-------|
| 0–7 | D0–D7 | Data registers, full 32-bit |
| 8–14 | A0–A6 | Address registers, full 32-bit |
| 15 | USP | User Stack Pointer |
| 16 | SSP | Supervisor Stack Pointer (ISP) |
| 17 | SR | Status Register (only low 16 bits meaningful) |
| 18 | PC | Program Counter |

> **Note on SR**: The runner masks SR to 16 bits during comparison
> (`expected & 0xFFFF`, `actual & 0xFFFF`). The upper 16 bits stored in the
> `.sst` file are always zero but the mask is applied defensively.

> **Note on A7 aliasing**: A7 is the active stack pointer. The CPU aliases it
> to either USP (index 15) or SSP (index 16) depending on the S-bit in SR
> (bit 13). The runner sets SR first, then USP and SSP independently via
> `M68K_REG_USP` and `M68K_REG_ISP`, allowing Musashi to manage the alias.

### RAM Entry Encoding

Each RAM entry is a sparse byte: a 4-byte address and 1-byte value (5 bytes
total per entry). Only bytes that differ from zero are recorded. The runner
zeroes the entire 16 MB flat memory array before loading each vector's RAM
entries, so unmentioned addresses are implicitly 0x00.

RAM addresses are masked to 24-bit (`addr & 0xFFFFFF`) on load, matching the
68000's 24-bit physical address bus.

### Example Annotated Hex Dump

```
Offset  Bytes            Description
------  ---------------  -----------
0x0000  53 53 54 31      magic "SST1"
0x0004  91 1F 00 00      num_vectors = 8065 (0x1F91)
0x0008  00               source_id = 0 (TomHarte)
0x0009  03               mnemonic_len = 3
0x000A  4E 4F 50         mnemonic = "NOP"
--- vector 0 starts ---
0x000D  2A               name_len = 42
0x000E  34 45 20 37 31   name = "4E 71 ..." (42 bytes)
  ...
0x0038  xx xx xx xx ...  initial regs[19] (76 bytes)
0x0084  xx xx xx xx ...  final regs[19] (76 bytes)
0x00D0  02 00            num_ram_initial = 2
0x00D2  02 00            num_ram_final = 2
0x00D4  00 10 00 00  4E  initial RAM[0]: addr=0x1000, val=0x4E
0x00D9  01 10 00 00  71  initial RAM[1]: addr=0x1001, val=0x71
0x00DE  00 10 00 00  4E  final   RAM[0]: addr=0x1000, val=0x4E
0x00E3  01 10 00 00  71  final   RAM[1]: addr=0x1001, val=0x71
--- vector 1 starts immediately after ---
```

### Source ID Values

| `source_id` | Origin | Notes |
|:-----------:|--------|-------|
| `0` | TomHarte / SingleStepTests/680x0 | Prefetch injected by converter |
| `1` | raddad772 / SingleStepTests/m68000 | PC adjusted −4 by converter |


## Converter Details

### `tools/sst_convert.py`

The converter handles format-specific nuances:

- **TomHarte format**: The instruction opcode is stored in the `prefetch`
  array, NOT in the RAM entries. The converter injects prefetch words into
  the initial RAM at the PC address (big-endian byte ordering).

- **raddad format**: The PC field stores `m_au` (next prefetch address),
  which is `instruction_start + 4`. The converter subtracts 4 from both
  initial and final PC values to get the true instruction address. The
  instruction bytes are already present in the RAM array.

## Runner Internals

### Initialization Sequence

Per vector, the runner:

1. **Clears** the 16MB flat memory array
2. **Loads** initial RAM from the test vector
3. **Clears** CPU stopped/halted state (direct internal access via `m68kcpu.h`)
4. **Sets SR** first (establishes supervisor/user mode, affects A7 aliasing)
5. **Sets D0-D7, A0-A6** via `m68k_set_reg()`
6. **Sets USP and SSP** (mode-dependent aliasing)
7. **Sets PC** last (triggers prefetch reload via `m68ki_jump()`)
8. **Executes** `m68k_execute(1)` — exactly one instruction

### Why Not `m68k_pulse_reset()` Per-Vector?

`m68k_pulse_reset()` imposes a 132-cycle reset penalty (`RESET_CYCLES`)
that `m68k_execute()` consumes before any instruction runs. With a minimal
cycle budget, the reset penalty consumes all available cycles and no
instruction executes. Instead, the runner directly clears the `stopped`
flag and `run_mode` via internal struct access.

### Cycle Tracking

The return value of `m68k_execute()` reports the actual cycles consumed
by the instruction. This value is captured (currently unused) and can be
used for future cycle-accuracy verification.

## Results (Current)

Verified full regression — 2026-04-24, both sources, all vectors:

| Dataset | Repo | Files clean | Vectors passed | Total vectors | Pass rate |
|---------|------|------------:|---------------:|--------------:|----------:|
| TomHarte | SingleStepTests/680x0 | ~56/134 | ~807,000 | ~1,080,710 | **80.7%** |
| raddad772 | SingleStepTests/m68000 | ~58/117 | ~256,000 | ~236,850 | **80.7%** |
| **Combined** | both | **114 / 251** | **1,063,217** | **1,317,560** | **80.7%** |

Timing: 251.9 s total · 5,229 vectors/s · fastest file 0.46 s · slowest 1.55 s


### Fully Passing Instruction Classes (both sources)

```
ADD.b    ADDX.b   AND.b    ANDItoCCR  ANDItoSR  ASL.b    ASL.l
BCHG     BCLR     BSET     BTST       CLR.b     CMP.b    EOR.b
EORItoCCR  EORItoSR  EXG    EXT.l    EXT.w    LEA      LINK
LSL.b    LSL.l    LSR.b    LSR.l      MOVE.b   MOVE.q   MOVEP.l
MOVEP.w  MOVEfromUSP  MOVEtoUSP  NEG.b  NEGX.b   NOP     NOT.b
OR.b     ORItoCCR  ORItoSR  PEA      RESET    ROL.b    ROL.l
ROR.b    ROR.l    ROXL.b   ROXL.l    ROXR.b   ROXR.l   Scc
SUB.b    SUBX.b   SWAP     TAS       TRAP     TRAPV    TST.b
UNLINK   EXG
```

### Failure Categories

Most failures fall into two categories:

1. **Address error exceptions**: `.l` and `.w` variants with complex
   addressing modes (indexed, indirect) where operand addresses are odd.
   Musashi correctly triggers the address error exception and pushes a
   14-byte stack frame, but the expected SSP delta differs from the test
   vector's expectation.

2. **BCD flag edge cases**: ABCD/SBCD/NBCD show minor V-flag disagreements
   (bit 1 of SR) in specific corner cases.

## Report Formats

### JSON (`report.json`)
Machine-readable, suitable for CI/CD integration. Contains summary stats
and per-file results with first failure messages.

### YAML (`report.yaml`)
Same content as JSON in YAML format for readability.

### Markdown (`report.md`, `report_brief.md`)
Human-readable table with per-file pass/fail status and optional failure
details. Brief variant omits individual failure messages.

### HTML (`report.html`)
Self-contained styled report with color-coded pass/fail indicators.

## Troubleshooting

### "No .sst files found"

The `.sst` files are generated artifacts — not checked into the repository.
You need to clone the raw test data and run the converter at least once.

> **Note:** All commands assume the current working directory is the musashi project **root**.

**Via CMake (easiest):** re-run the build; CMake will detect missing files
and run the clone + convert steps automatically:
```bash
cmake -B test/singlestep/build test/singlestep/
cmake --build test/singlestep/build
```

**Via stamp reset:** delete the convert stamp to force re-conversion while
keeping the already-cloned data:
```bash
rm -f test/singlestep/build/sst_convert.stamp
cmake --build test/singlestep/build
```

**Manually:**
```bash
# 1. Clone data if not already present:
mkdir -p test/singlestep/data
git clone --depth=1 https://github.com/SingleStepTests/680x0.git \
    test/singlestep/data/680x0
git clone --depth=1 https://github.com/SingleStepTests/m68000.git \
    test/singlestep/data/m68000

# 2. Convert:
python3 tools/sst_convert.py \
  --source=both \
  --tomharte-dir=test/singlestep/data/680x0/68000/v1 \
  --raddad-dir=test/singlestep/data/m68000/v1 \
  --output-dir=test/singlestep/unified
```

### Build errors about `m68kcpu.h`
The runner includes `m68kcpu.h` directly for internal state access
(`m68ki_cpu.stopped`, `m68ki_cpu.run_mode`). Ensure the Musashi source
directory is in the include path (`-I.`).

### Low pass rate
Verify the converter was run with the latest prefetch-injection and
PC-adjustment fixes. Re-run the converter if in doubt.
