# SingleStepTests Harness Design

## Goal

Run test vectors from **both** SingleStepTests repositories against Musashi,
unified into a single common format:

- **Source A**: [SingleStepTests/680x0](https://github.com/SingleStepTests/680x0) — ~1M vectors, gzipped JSON
- **Source B**: [SingleStepTests/m68000](https://github.com/SingleStepTests/m68000) — ~100K vectors, binary `.json.bin`

Both are converted by `tools/sst_convert.py` into a unified `.sst` format.

### Flexible Execution

- **Single file**: `./sst_runner ADD.b`
- **Single source**: `./sst_runner --source=tomharte` or `--source=raddad`
- **All tests**: `./sst_runner --all`
- **Limit per file**: `./sst_runner --max-vectors=100` (smoke testing)

---

## Source Comparison

| Feature | 680x0 (TomHarte) | m68000 (raddad772) |
|---------|-------------------|-------------------|
| Format | gzipped JSON | Binary `.json.bin` |
| Vectors/file | ~8,000 | ~1,000 |
| Total vectors | ~1,000,000 | ~100,000 |
| RAM format | byte-addressed | 16-bit words (split) |
| Bus cycles | r/w/n/t | r/w/n/t/re/we |
| Prefetch | 2-entry queue | 2-entry queue |
| PC semantics | next instruction | next prefetch addr |
| Opcode map | included | no |
| Known issues | none documented | TAS, TRAPV |
| Decoder | `gunzip` + JSON | custom `decode.py` |

### Why Both?

- Different emulator cores (TomHarte's vs MAME's microcoded) = different random seeds
- Cross-validation: if both agree on a result, confidence is very high
- raddad tests include address error bus cycles (`re`/`we`) not in TomHarte
- Combined: **~1.1 million unique test vectors**

---

## Repository Layout

```
musashi/
├── test/
│   ├── singlestep/
│   │   ├── README.md                # Setup and usage
│   │   ├── sst_manifest.json        # Group definitions + skip lists
│   │   ├── data/
│   │   │   ├── 680x0/              # git submodule → SingleStepTests/680x0
│   │   │   │   └── 68000/v1/*.json.gz
│   │   │   └── m68000/             # git submodule → SingleStepTests/m68000
│   │   │       └── v1/*.json.bin
│   │   ├── unified/                 # Generated .sst files (gitignored)
│   │   │   ├── tomharte/
│   │   │   │   ├── ADD_b.sst
│   │   │   │   └── ...
│   │   │   └── raddad/
│   │   │       ├── ADD_b.sst
│   │   │       └── ...
│   │   ├── sst_loader.h             # Unified .sst format parser (C)
│   │   ├── sst_loader.c
│   │   └── sst_runner.c             # Standalone runner
│   │
├── tools/
│   └── sst_convert.py               # Dual-source → unified .sst converter
│
└── doc/testing/
    └── singlestep_harness_design.md  # This document
```

### Data Storage: Git Submodules

```bash
git submodule add https://github.com/SingleStepTests/680x0.git test/singlestep/data/680x0
git submodule add https://github.com/SingleStepTests/m68000.git test/singlestep/data/m68000
```

The `unified/` directory is `.gitignore`'d — generated on first build.

---

## Unified SST Binary Format

Both sources are converted to the same compact binary:

```
Header:
  char[4]  magic = "SST1"
  uint32   num_vectors
  uint8    source_id        (0=tomharte, 1=raddad)
  uint8    name_len
  char[]   mnemonic         (e.g., "ADD.b")

Per vector:
  uint8    name_len
  char[]   name             (e.g., "ADD.b #42")
  uint32   initial_regs[19] (D0-D7, A0-A6, USP, SSP, SR, PC)
  uint32   final_regs[19]
  uint16   num_ram_initial
  uint16   num_ram_final
  {uint32 addr, uint8 val}[num_ram_initial]
  {uint32 addr, uint8 val}[num_ram_final]
```

---

## Converter: `tools/sst_convert.py`

### CLI

```bash
# Convert both sources
python3 tools/sst_convert.py --all

# Convert one source
python3 tools/sst_convert.py --source=tomharte
python3 tools/sst_convert.py --source=raddad

# Convert single file
python3 tools/sst_convert.py --file=ADD.b --source=tomharte

# Custom paths
python3 tools/sst_convert.py \
  --tomharte-dir=test/singlestep/data/680x0/68000/v1 \
  --raddad-dir=test/singlestep/data/m68000/v1 \
  --output-dir=test/singlestep/unified
```

### Conversion Pipeline

```
680x0/68000/v1/ADD.b.json.gz          m68000/v1/ADD.b.json.bin
        │                                      │
        ▼                                      ▼
   gunzip + json.load()              struct.unpack (binary decode)
        │                                      │
        ▼                                      ▼
   Normalize to common dict:                   │
   {regs[19], ram[(addr,byte)]}                │
        │                                      │
        └──────────┬───────────────────────────┘
                   ▼
        unified/tomharte/ADD_b.sst
        unified/raddad/ADD_b.sst
```

### Normalization Rules

| Field | TomHarte | raddad | Unified |
|-------|----------|--------|---------|
| RAM | `[addr, byte_val]` | `[addr, word_val]` → split to 2 bytes | `[addr, byte_val]` |
| PC | next instruction | next prefetch (PC+4) | next instruction (raddad PC-4+insn_len) |
| SR | decimal int | decimal int via unpack | uint32 |
| Regs | decimal ints | uint32 via unpack | uint32 |

---

## Test Manifest

`test/singlestep/sst_manifest.json`:

```json
{
  "version": 1,
  "groups": {
    "alu": {
      "description": "Arithmetic and Logic Unit",
      "files": ["ADD.b","ADD.w","ADD.l","ADDA.w","ADDA.l",
                "ADDX.b","ADDX.w","ADDX.l",
                "SUB.b","SUB.w","SUB.l","SUBA.w","SUBA.l",
                "SUBX.b","SUBX.w","SUBX.l",
                "NEG.b","NEG.w","NEG.l","NEGX.b","NEGX.w","NEGX.l",
                "CLR.b","CLR.w","CLR.l",
                "CMP.b","CMP.w","CMP.l","CMPA.w","CMPA.l",
                "MULS","MULU","DIVS","DIVU","EXT.w","EXT.l"]
    },
    "logic": {
      "description": "Logical operations",
      "files": ["AND.b","AND.w","AND.l","OR.b","OR.w","OR.l",
                "EOR.b","EOR.w","EOR.l","NOT.b","NOT.w","NOT.l"]
    },
    "shift": {
      "description": "Shifts and rotates",
      "files": ["ASL.b","ASL.w","ASL.l","ASR.b","ASR.w","ASR.l",
                "LSL.b","LSL.w","LSL.l","LSR.b","LSR.w","LSR.l",
                "ROL.b","ROL.w","ROL.l","ROR.b","ROR.w","ROR.l",
                "ROXL.b","ROXL.w","ROXL.l","ROXR.b","ROXR.w","ROXR.l"]
    },
    "move": {
      "description": "Data movement",
      "files": ["MOVE.b","MOVE.w","MOVE.l","MOVE.q",
                "MOVEA.w","MOVEA.l","MOVEM.w","MOVEM.l",
                "MOVEP.w","MOVEP.l","MOVEfromSR","MOVEtoSR",
                "MOVEtoCCR","MOVEfromUSP","MOVEtoUSP",
                "LEA","PEA","EXG","SWAP"]
    },
    "bcd":    { "files": ["ABCD","SBCD","NBCD"] },
    "bit":    { "files": ["BTST","BSET","BCLR","BCHG"] },
    "branch": {
      "files": ["Bcc","BSR","DBcc","Scc","JMP","JSR","RTS","RTR","RTE",
                "LINK","UNLINK"]
    },
    "misc": {
      "files": ["NOP","TAS","TRAP","TRAPV","STOP","RESET",
                "ILLEGAL_LINEA","ILLEGAL_LINEF","TST.b","TST.w","TST.l"]
    },
    "sr_ops": {
      "files": ["ANDItoCCR","ANDItoSR","ORItoCCR","ORItoSR",
                "EORItoCCR","EORItoSR"]
    }
  },
  "skip": {
    "raddad": {
      "TAS": "Known timing issues in raddad test suite",
      "TRAPV": "Reported S-bit anomaly in raddad test suite"
    }
  }
}
```

---

## Standalone Runner: `sst_runner`

### CLI

```
Usage: sst_runner [OPTIONS] [FILE...]

Arguments:
  FILE...              Mnemonic names (e.g., "ADD.b")

Options:
  --all                Run all test files
  --group=NAME         Run logical group (alu, logic, shift, move, etc.)
  --groups             List available groups
  --source=NAME        Filter by source (tomharte, raddad, or both [default])
  --data-dir=PATH      Override unified data directory
  --stop-on-fail       Stop at first failure
  --verbose            Print each vector
  --summary            Per-file pass/fail summary
  --skip-known         Skip files in manifest skip list
  --max-vectors=N      Limit vectors per file (smoke testing)
```

### Examples

```bash
# Single instruction, both sources
./sst_runner ADD.b

# One group, TomHarte only
./sst_runner --group=alu --source=tomharte

# Quick smoke (10 vectors per file, both sources)
./sst_runner --all --max-vectors=10 --skip-known

# Full regression
./sst_runner --all --skip-known --summary

# Output:
  [tomharte] ADD.b .............. 8192/8192 PASS
  [raddad]   ADD.b .............. 1000/1000 PASS
  [tomharte] ADD.w .............. 8192/8192 PASS
  ...
  Summary: 200/200 files passed, 1,108,000/1,108,000 vectors passed
           Sources: tomharte=1,000,000  raddad=108,000
```

### Makefile Targets

```makefile
sst-init:
	git submodule update --init test/singlestep/data/680x0
	git submodule update --init test/singlestep/data/m68000
	python3 tools/sst_convert.py --all

sst: sst_runner$(EXE)
	./sst_runner$(EXE) --all --skip-known --summary

sst-tomharte: sst_runner$(EXE)
	./sst_runner$(EXE) --all --source=tomharte --summary

sst-raddad: sst_runner$(EXE)
	./sst_runner$(EXE) --all --source=raddad --skip-known --summary

sst-smoke: sst_runner$(EXE)
	./sst_runner$(EXE) --all --max-vectors=10 --skip-known

sst-alu: sst_runner$(EXE)
	./sst_runner$(EXE) --group=alu

sst-shift: sst_runner$(EXE)
	./sst_runner$(EXE) --group=shift
```

---

## GTest Integration

### Naming Scheme

```
SST/SOURCE/GROUP/MNEMONIC/VectorN

Examples:
  SST/TomHarte/ALU/ADD_b/Vector0
  SST/Raddad/ALU/ADD_b/Vector0
  SST/TomHarte/SHIFT/ASL_w/Vector42
```

### Filter Examples

```bash
# All SST
./musashi_tests --gtest_filter="SST/*"

# One source
./musashi_tests --gtest_filter="SST/TomHarte/*"

# One group from both sources
./musashi_tests --gtest_filter="SST/*/ALU/*"

# One mnemonic, both sources
./musashi_tests --gtest_filter="SST/*/*/ADD_b/*"
```

---

## Verification Rules

| Field | Compare | Notes |
|-------|:-------:|-------|
| D0-D7 | ✅ | Exact |
| A0-A6 | ✅ | Exact |
| USP | ✅ | Exact |
| SSP | ✅ | Exact |
| SR | ✅ | Full 16 bits |
| PC | ⚠️ | Normalize per-source before compare |
| RAM | ✅ | Every final byte |
| Prefetch | ❌ | Not modeled |
| Bus cycles | ❌ | Not modeled |
| Cycle count | ❌ | Not cycle-exact |

---

## Cross-Validation Mode

When both sources have vectors for the same mnemonic, `sst_runner --cross`
checks that Musashi produces the same result for both. Any discrepancy flags
either a Musashi bug or a test suite bug.

---

## Maintenance

1. `git submodule update --remote` to pull upstream updates
2. Re-run `python3 tools/sst_convert.py --all` after updates
3. Edit `sst_manifest.json` to add groups or skip entries
4. CI runs `make sst` after `make test`
