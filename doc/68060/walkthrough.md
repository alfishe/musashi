# 68060 Implementation Walkthrough

## Summary

Implemented Motorola 68060 CPU emulation in the Musashi core, including three variants (68EC060, 68LC060, MC68060), 060-specific register plumbing, FPU hardware subset trapping, unimplemented integer instruction trapping (MOVEP, CAS2, CHK2/CMP2, 64-bit MULL/DIVL), PMMU instruction trapping, and a comprehensive 19-test binary test suite. All changes maintain full backward compatibility — zero regressions on existing 68000 and 68040 test suites.

## Architecture

The implementation follows Musashi's established "unity build" pattern: all 060-specific logic is added via inline `CPU_TYPE_IS_060_PLUS()` guards within the existing source files. No new CPU-specific files were created.

The 68060 is a superscalar evolution of the 68040 that **removed** several instructions and FPU operations, delegating them to software exception handlers (FPSP/ISP). This inverted approach — trapping what was previously native — is the core design challenge.

## Files Modified

### `m68k.h` — Public API
- Added `M68K_CPU_TYPE_68EC060`, `M68K_CPU_TYPE_68LC060`, `M68K_CPU_TYPE_68060` to CPU type enum
- Added `M68K_REG_PCR` (Processor Configuration Register) and `M68K_REG_BUSCR` (Bus Control Register)

---

### `m68kconf.h` — Configuration
- Added `M68K_EMULATE_060` (compile-time enable/disable)

---

### `m68kcpu.h` — Core Internals
- Added CPU bitmasks: `CPU_TYPE_EC060` (0x800), `CPU_TYPE_LC060` (0x1000), `CPU_TYPE_060` (0x2000)
- Added exception constants: `EXCEPTION_UNIMPLEMENTED_FP` (60), `EXCEPTION_UNIMPLEMENTED_INTEGER` (61)
- Added `CPU_TYPE_IS_060_PLUS()` macro
- Updated all 7 `_PLUS` macros (010–040) to include 060 types
- Added `HAS_FPU`, `REG_PCR`, `REG_BUSCR` accessor macros
- Added `has_fpu`, `pcr`, `buscr` fields to CPU struct
- Added `m68ki_exception_unimplemented_integer()` (Vector 61 handler using Format $0 frame)

---

### `m68kcpu.c` — CPU Core
- Added 3 CPU name strings for logging
- Added `m68k_set_cpu_type()` cases for EC060/LC060/060 with correct PCR silicon IDs (0x0432/0x0431/0x0430)
- Added `get_reg`/`set_reg` cases for PCR and BUSCR
- Added CPU type reverse mapping for 060 variants
- Initialized `HAS_FPU` for all existing CPU types (backfilled for 000/040/EC040/LC040)
- PCR write preserves silicon ID (high 16 bits read-only)

---

### `m68k_in.c` — Instruction Handlers
- **FPU Guard**: Added `HAS_FPU` check to `040fpu0`/`040fpu1` — traps to F-line on FPU-less variants (LC040/EC040/LC060/EC060)
- **MOVEC PCR/BUSCR**: Added read (cr) and write (rc) handlers for register codes 0x808 (PCR) and 0x008 (BUSCR)
- **CACR Mask**: Added 060-specific writable mask (0xF0F0C800) before the 040 full-write case
- **MMUSR Trap**: Added 060 illegal check on MMUSR (removed from MOVEC on 060)
- **MOVEP Trap**: All 4 MOVEP variants (16/32 × re/er) trap unconditionally to Vector 61 on 060
- **CAS2 Trap**: Both CAS2.W and CAS2.L trap to Vector 61 (CAS single still works)
- **CHK2/CMP2 Trap**: All 9 handlers (3 sizes × 3 EA modes) trap to Vector 61
- **64-bit MULL Trap**: MULL with 64-bit result (Dh:Dl, BIT_A set) traps to Vector 61; 32-bit MULL works natively
- **64-bit DIVL Trap**: DIVL with 64-bit dividend (Dr:Dq, BIT_A set) traps to Vector 61; 32-bit DIVL works natively
- **PMMU Trap**: Old-style PMMU coprocessor instructions (PTEST, PFLUSH, PLOAD, PBcc, PMOVE) trap to F-line on 060

---

### `m68kfpu.c` — FPU
- **fpgen_rm_reg**: Added 060 trap logic at the FPU opmode dispatch with:
  - FMOVECR trap (rm==1, src==7 → F-line)
  - Hardware ops whitelist (FMOVE/FADD/FSUB/FMUL/FDIV/FABS/FNEG/FSQRT/FCMP/FTST/FINT/FINTRZ + single/double rounding variants)
  - Non-hardware ops → F-line trap
  - Packed decimal source (src==3) → F-line trap
- **fmove_reg_mem**: Added packed decimal store trap (dst 3/7 → F-line)
- **fmove_fpcr**: Added FPIAR trap (060 removed FPIAR register)

---

### `test/test_driver.c` — Test Infrastructure
- Added `--cpu=TYPE` command-line flag supporting all CPU variants
- Default remains `68040` for backward compatibility

## New Files

### `test/mc68060/` — 060 Test Suite (19 files)

| Test | What it verifies |
|------|-----------------|
| **Integer traps (Vector 61)** | |
| `movep_trap.s` | All 4 MOVEP variants trap to Vector 61 |
| `cas2_trap.s` | CAS2.W and CAS2.L trap; CAS single executes normally |
| `chk2_cmp2_trap.s` | CHK2/CMP2 in all 3 sizes (.B/.W/.L) trap |
| `mull_64_trap.s` | 64-bit MULL traps; 32-bit MULL executes with correct result |
| `divl_64_trap.s` | 64-bit DIVL traps; 32-bit DIVL executes with correct result |
| **MOVEC registers** | |
| `movec_pcr.s` | PCR reads back silicon ID 0x0430 |
| `movec_cacr_mask.s` | CACR write mask = 0xF0F0C800 |
| `movec_buscr.s` | BUSCR read/write round-trip |
| `pcr_write_mask.s` | PCR high 16 bits (silicon ID) read-only |
| `mmusr_trap.s` | MOVEC MMUSR traps to illegal (Vector 4) |
| **FPU traps (Vector 11/F-line)** | |
| `fmovecr_trap.s` | ALL 64 FMOVECR offsets trap (full coverage) |
| `fpu_hw_ops.s` | 12 hardware FPU ops execute natively with result verification |
| `fpu_sw_traps.s` | 21 unimplemented FPU ops (incl FREM) trap to F-line |
| `fsin_trap.s` | FSIN traps to F-line |
| `fadd_exec.s` | FADD executes natively, produces correct result |
| `fpiar_trap.s` | FMOVEM with FPIAR traps; FPCR/FPSR-only works |
| `packed_trap.s` | Packed decimal FPU store traps to F-line |
| **MMU** | |
| `ptest_trap.s` | PTEST (old PMMU coprocessor instruction) traps to F-line |
| **Variants** | |
| `lc060_fpu_trap.s` | LC060 (no FPU) traps ALL FPU instructions |
| **Infrastructure** | |
| `entry_060.s` | Shared exception handlers for Vectors 4/11/60/61 |

## Verification Results

```
=== 68000 Regression ===
60 passed, 0 failed

=== 68040 Regression ===
18 passed, 0 failed

=== 68060 Tests ===
cas2_trap              PASS
chk2_cmp2_trap         PASS
divl_64_trap           PASS
fadd_exec              PASS
fmovecr_trap           PASS
fpiar_trap             PASS
fpu_hw_ops             PASS
fpu_sw_traps           PASS
fsin_trap              PASS
mmusr_trap             PASS
movec_buscr            PASS
movec_cacr_mask        PASS
movec_pcr              PASS
movep_trap             PASS
mull_64_trap           PASS
packed_trap            PASS
pcr_write_mask         PASS
ptest_trap             PASS
18 passed, 0 failed

=== LC060 ===
lc060_fpu_trap         PASS
1 passed, 0 failed

TOTAL: 97 passed, 0 failed
```

## Remaining (Not Implemented)

| Item | Notes |
|------|-------|
| PLPA instruction | 060-specific MMU translation instruction |
| LPSTOP instruction | Low-power stop (superscalar power management) |
| 060 MMU (different from 030/040 PMMU) | TC/DTT/ITT registers via MOVEC, not PMOVE |
| Exception frame Format $4 (access fault) | Not yet tested/validated |
| EC060 variant tests | PCR ID = 0x0432, no MMU+FPU |

## Build Requirements

The 060 test assembly requires `m68k-elf-binutils`:
```bash
# macOS
brew install m68k-elf-binutils

# Linux (Debian/Ubuntu)
apt install binutils-m68k-linux-gnu
```
