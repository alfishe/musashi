# System, Regression & Benchmark Test Plan

Covers exceptions, privileged registers, interrupts, co-simulation callbacks,
CPU type verification, fixed-bug regression, and performance benchmarks.

---

## File: `test/gtest/system/test_exceptions.cpp`

Fixture: `ExceptionTest` (parameterized per CPU type)

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | Format0_TRAP | TRAP #0: SP-=8, format=$0, vector=32 |
| 2 | Format0_Illegal | ILLEGAL: vector 4, format $0 |
| 3 | Format0_PrivViol | MOVEC in user mode: vector 8 |
| 4 | Format0_LineA | $A000: vector 10 |
| 5 | Format0_LineF | $F000: vector 11 |
| 6 | Format2_Interrupt | IRQ level 5: format $2, vector from ACK |
| 7 | Format7_040_BusError | 040 bus error: 30-word frame, SSW |
| 8 | FormatB_030_BusError | 030 bus error: 46-word frame, SSW |
| 9 | AddressError_OddWord | Word read from odd: vector 3 |
| 10 | DivByZero | DIVS #0: vector 5 |
| 11 | CHK_Exception | CHK out of bounds: vector 6 |
| 12 | TRAPV_Exception | TRAPV with V=1: vector 7 |
| 13 | RTE_Format0 | Build format $0 frame, execute RTE |
| 14 | RTE_Format2 | Build format $2 frame, execute RTE |
| 15 | RTE_RestoresSR | Verify SR fully restored |
| 16 | RTE_RestoresPC | Verify PC restored |
| 17 | Exception_SavesSR | SR on stack has pre-exception state |
| 18 | Exception_SetsSuper | SR S-bit set on exception entry |
| 19 | Exception_DisablesTrace | SR T-bits cleared on exception |
| 20 | DoublebusFault_Halt | Bus error during bus error → halt |

Est. coverage: **20 tests**

---

## File: `test/gtest/system/test_privileged.cpp`

Fixture: `PrivilegedTest` (parameterized per CPU type)

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | MOVEC_VBR_ReadWrite | Round-trip VBR (010+) |
| 2 | MOVEC_CACR_ReadWrite | CACR round-trip (020+) |
| 3 | MOVEC_CACR_Mask_030 | CACR mask = $00003F1F |
| 4 | MOVEC_CACR_Mask_040 | CACR mask = $80008000 |
| 5 | MOVEC_CACR_Mask_060 | CACR mask = $F0F0C800 (approx) |
| 6 | MOVEC_SFC_DFC | SFC/DFC round-trip |
| 7 | MOVEC_USP | USP via MOVEC (010+) |
| 8 | MOVEC_TC_040 | TC via MOVEC (040) |
| 9 | MOVEC_URP_SRP_040 | URP/SRP via MOVEC (040) |
| 10 | MOVEC_MMUSR_040 | MMUSR via MOVEC (040) |
| 11 | MOVEC_PCR_060 | PCR via MOVEC (060) |
| 12 | MOVEC_PCR_WriteMask | PCR bits 0,1 read-only |
| 13 | MOVEC_BUSCR_060 | BUSCR via MOVEC (060) |
| 14 | MOVEC_UserMode_Traps | Any MOVEC in user mode → priv viol |
| 15 | STOP_UserMode_Traps | STOP in user mode → priv viol |
| 16 | RESET_UserMode_Traps | RESET in user mode → priv viol |

Est. coverage: **16 tests**

---

## File: `test/gtest/system/test_interrupts.cpp`

Fixture: `InterruptTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | IRQ_Level1 | m68k_set_irq(1), verify exception |
| 2 | IRQ_Level7_NMI | Level 7: non-maskable |
| 3 | IRQ_Masked | IPL < SR mask: no interrupt |
| 4 | IRQ_Unmasked | IPL >= SR mask: interrupt fires |
| 5 | IRQ_AckCallback | int_ack_callback returns vector |
| 6 | IRQ_AutoVector | Autovector (24+level) |
| 7 | VIRQ_SetClear | m68k_set_virq / get_virq |
| 8 | IRQ_Priority | Higher level preempts |
| 9 | IRQ_Timing | Fires between instructions |
| 10 | IRQ_StackFrame | Format $2 frame correct |

Est. coverage: **10 tests**

---

## File: `test/gtest/system/test_callbacks.cpp`

Fixture: `CallbackTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | PCChanged_OnBranch | pc_changed fires on BRA |
| 2 | PCChanged_OnException | pc_changed fires on exception |
| 3 | PCChanged_OnRTE | pc_changed fires on RTE |
| 4 | FCChanged | fc_callback fires on user↔super |
| 5 | ResetInstr | reset_instr fires on RESET |
| 6 | TASCallback_Allow | tas_callback returns 1: normal |
| 7 | TASCallback_Block | tas_callback returns 0: no write |
| 8 | IllegalInstr | illg_instr_callback fires |
| 9 | InstrHook | instr_hook fires every instruction |
| 10 | MMU_Translate | mmu_translate callback on ATC hit |
| 11 | MMU_Translate_Walk | mmu_translate callback on walk |
| 12 | MMU_ATC_Insert | mmu_atc callback INSERT |
| 13 | MMU_ATC_Flush | mmu_atc callback FLUSH |
| 14 | BkptAck | bkpt_ack callback on BKPT |

Est. coverage: **14 tests**

---

## File: `test/gtest/system/test_cpu_types.cpp`

Fixture: `CPUTypeTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | IS_000 | CPU_TYPE_IS_000(68000) = true |
| 2 | IS_010_PLUS | 68010,020,030,040,060 = true |
| 3 | IS_020_PLUS | 68020+ = true, 000/010 = false |
| 4 | IS_030_PLUS | 030+ including LC040 |
| 5 | IS_040_PLUS | 040,EC040,LC040,060+ |
| 6 | IS_060_PLUS | 060,EC060,LC060 |
| 7 | LC040_In_Macros | LC040 in all correct _PLUS chains |
| 8 | EC040_NoFPU | EC040 has no FPU |
| 9 | LC040_NoMMU | LC040 has no MMU |
| 10 | EC060_NoFPU_NoMMU | EC060 has neither |
| 11 | LC060_NoFPU | LC060 has no FPU |
| 12 | ContextSaveRestore | get/set context round-trip |
| 13 | IsValidInstruction | m68k_is_valid_instruction smoke |

Est. coverage: **13 tests**

---

## File: `test/gtest/system/test_regression.cpp`

Fixture: `RegressionTest`

Each test reproduces a specific fixed bug to prevent recurrence.

| # | Test Name | Bug | Verification |
|---|-----------|-----|-------------|
| 1 | Bug_FSSQRT_MappedToFINT | Opmode mask stripped | FSSQRT(4)=2, not 4 |
| 2 | Bug_FSGLDIV_HostCast | Host float cast | Compare vs SoftFloat |
| 3 | Bug_FSGLMUL_HostCast | Host float cast | Compare vs SoftFloat |
| 4 | Bug_LC040_Missing_Macros | LC040 not in _PLUS | IS_040_PLUS(LC040)=true |
| 5 | Bug_MOVE16_NoAlignment | No 16-byte mask | Addr $1003 → $1000 |
| 6 | Bug_MOVE16_Missing4Forms | Only (Ax)+,(Ay)+ | All 5 forms work |
| 7 | Bug_Packed040_NoTrap | No F-line trap | FMOVE packed → trap |
| 8 | Bug_FPCR_Precision | Bits [7:6] unwired | Set prec=SP, verify |
| 9 | Bug_FPCR_Rounding | Used host conversion | Set RZ, verify truncation |
| 10 | Bug_SSW_LK_CAS | LK not set for CAS | CAS fault → LK=1 |
| 11 | Bug_SSW_LK_TAS | LK not set for TAS | TAS fault → LK=1 |
| 12 | Bug_040_Transcendental_NoTrap | FSIN executed on 040 | Must F-line trap |
| 13 | Bug_FMOVECR_040_NoTrap | FMOVECR on 040 | Must F-line trap |
| 14 | Bug_FmoveRegMem_NoRounding | Missing FPCR in fmove_reg_mem | Rounding applied |

Est. coverage: **14 tests**

---

## Benchmark Files

### File: `test/gtest/bench/bench_execute.cpp`

| # | Benchmark | Measures |
|---|-----------|---------|
| 1 | BM_NOP_68000 | NOP dispatch overhead (68000) |
| 2 | BM_NOP_68040 | NOP dispatch overhead (68040) |
| 3 | BM_ADD_Long | ADD.L throughput |
| 4 | BM_MULS_Word | MULS.W throughput |
| 5 | BM_DIVS_Word | DIVS.W throughput |
| 6 | BM_MOVE_Long | MOVE.L throughput |
| 7 | BM_MOVE16 | MOVE16 throughput |

### File: `test/gtest/bench/bench_fpu.cpp`

| # | Benchmark | Measures |
|---|-----------|---------|
| 1 | BM_FADD | FADD throughput |
| 2 | BM_FMUL | FMUL throughput |
| 3 | BM_FDIV | FDIV throughput |
| 4 | BM_FSQRT | FSQRT throughput |

### File: `test/gtest/bench/bench_mmu.cpp`

| # | Benchmark | Measures |
|---|-----------|---------|
| 1 | BM_ATC_Hit | ATC hit (cached translation) |
| 2 | BM_Table_Walk | Full 3-level walk |
| 3 | BM_TT_Match | TT transparent fast-path |

### File: `test/gtest/bench/bench_context.cpp`

| # | Benchmark | Measures |
|---|-----------|---------|
| 1 | BM_GetContext | m68k_get_context() |
| 2 | BM_SetContext | m68k_set_context() |

---

## System Test Summary

| File | Tests |
|------|:-----:|
| test_exceptions.cpp | 20 |
| test_privileged.cpp | 16 |
| test_interrupts.cpp | 10 |
| test_callbacks.cpp | 14 |
| test_cpu_types.cpp | 13 |
| test_regression.cpp | 14 |
| bench_execute.cpp | 7 |
| bench_fpu.cpp | 4 |
| bench_mmu.cpp | 3 |
| bench_context.cpp | 2 |
| **Total** | **103** |

---

## Grand Total (All Plans)

| Plan File | Tests |
|-----------|:-----:|
| Integer ISA (test_plan_integer.md) | 233 |
| FPU (test_plan_fpu.md) | 121 |
| 030 MMU (test_plan_mmu_030.md) | 67 |
| 040 MMU (test_plan_mmu_040.md) | 74 |
| System/Regression/Bench (test_plan_system.md) | 103 |
| **Grand Total** | **598** |

---

## Complementary Bulk Regression (SST)

While the hand-written tests above target specific subsystems and known bugs, the **SingleStepTests (SST)** harness provides exhaustive coverage for the base 68k ISA.

- **Scale**: 1,317,560 test vectors.
- **Workflow**: Automated via `sst_runner.sh`.
- **Target**: Architectural parity with silicon-accurate reference models.

Refer to [test_plan_singlestep.md](test_plan_singlestep.md) for details on the bulk verification suite.
