# Musashi GTest Test Plan — Overview

## Document Index

| File | Content |
|------|---------|
| [test_plan_overview.md](test_plan_overview.md) | This file — structure, conventions, coverage summary |
| [test_plan_integer.md](test_plan_integer.md) | Integer ISA tests (68000 base + 020+ extensions) |
| [test_plan_fpu.md](test_plan_fpu.md) | FPU tests with SoftFloat oracle |
| [test_plan_mmu_030.md](test_plan_mmu_030.md) | MMU tests for 68030 |
| [test_plan_mmu_040.md](test_plan_mmu_040.md) | MMU tests for 68040/060 |
| [test_plan_system.md](test_plan_system.md) | Exceptions, privileged regs, co-sim, regression, benchmarks |

---

## Directory Structure

musashi/
├── test/
│   ├── gtest/            # Native GTest suite
│   │   ├── CMakeLists.txt
│   │   ├── common/
│   │   ├── cpu_68000/
│   │   ├── cpu_68020/
│   │   ├── cpu_68040/
│   │   ├── fpu/
│   │   ├── mmu/
│   │   ├── system/
│   │   └── bench/
│   └── singlestep/       # Bulk regression harness
│       ├── README.md
│       ├── sst_runner.sh
│       ├── data/
│       ├── unified/
│       ├── reports/
│       ├── sst_loader.c/h
│       └── sst_runner.c
├── tools/
│   └── sst_convert.py    # Dual-source converter
├── CMakeLists.txt        # Root CMake
└── doc/testing/
    ├── testing_strategy.md
    ├── test_plan_overview.md
    ├── test_plan_integer.md
    ├── test_plan_fpu.md
    ├── test_plan_mmu_030.md
    ├── test_plan_mmu_040.md
    ├── test_plan_system.md
    ├── test_plan_singlestep.md
    └── singlestep_harness_design.md
```

---

## Test Naming Convention

```
TEST_F(SuiteName, Instruction_Variant_Condition)

Examples:
  TEST_F(DataMoveTest,    MOVE_Long_DnToDn)
  TEST_F(ArithmeticTest,  ADD_Byte_ImmToMem_CarryFlag)
  TEST_F(BitfieldTest,    BFEXTU_RegOffset_Width32)
  TEST_F(FPUBasicTest,    FADD_PosInf_PlusNegInf_IsNaN)
  TEST_F(MMU040WalkTest,  Walk4K_IndirectDescriptor_Depth2)
  TEST_F(RegressionTest,  Bug_FSSQRT_MappedToFINT)
```

---

## GTest Filter Patterns

```bash
# All 68000 base ISA tests
./musashi_tests --gtest_filter="cpu_68000/*"

# Just FPU rounding
./musashi_tests --gtest_filter="FPURounding*"

# All 040 MMU tests
./musashi_tests --gtest_filter="MMU040*"

# Just regression tests
./musashi_tests --gtest_filter="Regression*"

# Everything
./musashi_tests
```

---

## Coverage Summary

| Category | Test File Count | Est. Tests | Coverage Target |
|----------|:-:|:-:|:-:|
| 68000 Integer ISA | 9 | 171 | 100% of base mnemonics |
| 020+ Extensions | 7 | 55 | 100% of 020+ mnemonics |
| 040 Specific | 1 | 7 | MOVE16 |
| FPU | 8 | 121 | All ops, rounding modes, edge cases |
| MMU 030 | 9 | 67 | Full 030 PMMU coverage |
| MMU 040 | 8 | 74 | Full 040 PMMU coverage |
| System | 5 | 73 | Exceptions, privileged, co-sim |
| Regression | 1 | 14 | All known fixed bugs |
| Benchmarks | 4 | 16 | Throughput baselines |
| **Total** | **52** | **598** | |

---

## CPU Type × Suite Applicability

| Suite | 000 | 010 | EC020 | 020 | 030 | EC040 | LC040 | 040 | EC060 | LC060 | 060 |
|-------|:---:|:---:|:-----:|:---:|:---:|:-----:|:-----:|:---:|:-----:|:-----:|:---:|
| cpu_68000/* | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| cpu_68020/* | — | — | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| cpu_68040/* | — | — | — | — | — | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| fpu/* | — | — | — | opt¹ | opt¹ | — | — | ✅ | — | — | ✅ |
| mmu/mmu_030/* | — | — | — | — | ✅ | — | — | — | — | — | — |
| mmu/mmu_040/* | — | — | — | — | — | — | ✅ | ✅ | — | ✅ | ✅ |
| system/* | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| bench/* | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

¹ FPU instructions supported via external 68881/68882 coprocessor.

---

## SingleStepTests (SST) Integration

In parallel with the hand-written GTest suite, we integrate **1.3M+**
test vectors from two independent sources for exhaustive 68000 base ISA
verification:

- [SingleStepTests/680x0](https://github.com/SingleStepTests/680x0) — ~1.08M vectors (TomHarte, gzipped JSON)
- [SingleStepTests/m68000](https://github.com/SingleStepTests/m68000) — ~237K vectors (raddad772, binary)

Both are converted to a unified `.sst` format via `tools/sst_convert.py`.

| Document | Content |
|----------|---------|
| [test_plan_singlestep.md](test_plan_singlestep.md) | Format spec, file list, caveats |
| [singlestep_harness_design.md](singlestep_harness_design.md) | Dual-source harness, binary spec, CLI design |

### Quick Reference

The recommended entry point is the **Zero-Setup** bootstrap script:
```bash
./test/singlestep/sst_runner.sh --all --summary
```

For manual debugging and focused testing:
```bash
# Standalone runner
./test/singlestep/sst_runner ADD.b               # single file, both sources
./test/singlestep/sst_runner --source=tomharte   # one source only
./test/singlestep/sst_runner --all --summary     # full regression
```

### Combined Coverage

| Source | Tests | Coverage |
|--------|:-----:|---------|
| Hand-written GTest | ~600 | Edge cases, flags, 020+/FPU/MMU/co-sim |
| SST — TomHarte/680x0 | ~1,080,000 | Exhaustive 68k ISA (8K vectors/mnemonic) |
| SST — raddad/m68000 | ~237,000 | Cross-validation (~1-2K vectors/mnemonic) |
| Legacy binary tests | 136 | Existing regression suite |
| **Total** | **~1,317,736** | |
