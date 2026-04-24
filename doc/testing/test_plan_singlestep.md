# SingleStepTests Integration Plan

This plan documents the verification harness for Musashi using the SingleStepTests (SST) datasets. This harness provides architecture-level parity testing against high-fidelity 68k reference cores (MAME, etc.).

## Data Sources

The harness integrates two primary datasets:

1. **TomHarte/SingleStepTests (680x0)**
   - Format: Gzipped JSON
   - Coverage: 68000 through 68040
   - Density: ~1,000,000+ vectors
   - Repository: `https://github.com/SingleStepTests/680x0`

2. **raddad772/SingleStepTests (m68000)**
   - Format: Packed Binary (`.json.bin`)
   - Coverage: 68000 base ISA
   - Density: ~317,000 vectors
   - Repository: `https://github.com/SingleStepTests/m68000`

Combined, these provide **1,317,560 test vectors** covering almost every opcode/addressing mode combination.

## Integration Architecture

The harness is designed for performance and zero-dependency runtime execution:

```
[Raw Data (JSON/Bin)] 
       │
       ▼ (tools/sst_convert.py)
[Unified Binary (.sst)] ──► Located in test/singlestep/unified/
       │
       ▼ (sst_runner / sst_runner.sh)
[Musashi Core] ◄──────────► [Memory/Register Parity Check]
```

### Why a Custom Runner?
- **Speed**: Running 1.3M tests through GTest/C++ is significantly slower due to overhead.
- **Simplicity**: The custom `sst_runner.c` is lean, compiles instantly, and has no dependencies.
- **Binary Format**: Using a custom `.sst` format allows us to "flatten" complex JSON into fixed-size arrays, enabling cache-friendly execution.

## The `.sst` Binary Format

Located in `test/singlestep/sst_loader.h`. All multi-byte values are stored in **Host Endian**.

### Header
- `uint32 magic`: `0x53535401` ("SST\x01")
- `uint32 num_tests`: Total vectors in this file
- `uint32 name_len`: Length of the mnemonic string
- `char name[]`: The instruction name (e.g., "MOVE.l")

### Test Vector (Repeated `num_tests` times)
- `uint32 regs_initial[19]`: D0-D7, A0-A6, USP, SSP, SR, PC
- `uint32 regs_final[19]`: Expected registers after execution
- `uint16 num_ram_initial`: Number of source RAM entries
- `uint16 num_ram_final`: Number of expected RAM changes
- `SST_RAM_Entry initial_ram[SST_MAX_RAM]`: `uint32 addr; uint8 value;`
- `SST_RAM_Entry final_ram[SST_MAX_RAM]`: `uint32 addr; uint8 value;`

*Note: `SST_MAX_RAM` is currently 64 to accommodate large MOVEM operations.*

## Usage

### 1. The "Zero Setup" Path (Recommended)
This script handles cloning, conversion, and building automatically:
```bash
./test/singlestep/sst_runner.sh --all --summary
```

### 2. Manual CMake Path
```bash
cmake -B test/singlestep/build test/singlestep/
cmake --build test/singlestep/build
./test/singlestep/sst_runner --all
```

## Performance & Results

- **Throughput**: ~5,200 vectors/sec (on modern hardware)
- **Total Duration**: ~4m 15s for the full 1.3M suite
- **Current Pass Rate**: **~80.7%**

### Primary Architecture Gaps
1. **Address Errors**: Discrepancies in the stack frame content/format pushed by Musashi vs. the test expectation when complex addressing modes hit odd addresses.
2. **BCD Flags**: Discrepancies in `ABCD`/`SBCD` flag behavior (C/V/Z bits) in specific edge cases.
3. **Complex CCR Updates**: Some shift/rotate operations show minor flag mismatches in specific overflow scenarios.

## Caveats & Design Decisions

1. **Memory Isolation**: Each test vector reset-initializes a 1MB RAM window.
2. **Cycle Accuracy**: While the runner captures cycles, they are currently excluded from pass/fail criteria.
3. **Host Endianness**: The `.sst` format is host-endian for speed. If moving data between machines, re-run `sst_convert.py`.
4. **Binary Exclusions**: `sst_runner` and the `build/` directory are git-ignored to prevent root pollution.
