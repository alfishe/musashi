# FPU Test Plan

All FPU tests use SoftFloat as an independent oracle. The test fixture sets up
the FPU, loads operands into FP registers, executes the instruction, and
compares the result against a SoftFloat-computed reference.

Applicable CPUs: 68020+FPU, 68030+FPU, 68040, 68060 (not EC/LC variants)

---

## File: `test/gtest/fpu/test_fpu_basic.cpp`

Fixture: `FPUBasicTest`

| # | Test Name | Op | Verification |
|---|-----------|-----|-------------|
| 1 | FMOVE_ExtToFPn | FMOVE.X mem,FP0 | FP0 = loaded value |
| 2 | FMOVE_SingleToFPn | FMOVE.S mem,FP0 | single→extended conversion |
| 3 | FMOVE_DoubleToFPn | FMOVE.D mem,FP0 | double→extended conversion |
| 4 | FMOVE_FPnToMem_Single | FMOVE.S FP0,mem | extended→single rounding |
| 5 | FMOVE_FPnToMem_Double | FMOVE.D FP0,mem | extended→double rounding |
| 6 | FMOVE_FPnToFPn | FMOVE FP0,FP1 | register copy |
| 7 | FADD_Normal | FADD FP0,FP1 | SoftFloat oracle |
| 8 | FADD_PosInf_NegInf | FADD +inf,-inf | NaN result |
| 9 | FADD_Zero_Zero | FADD +0,+0 | +0 |
| 10 | FADD_Denormal | FADD denorm,denorm | correct subnormal result |
| 11 | FSUB_Normal | FSUB FP0,FP1 | oracle |
| 12 | FSUB_SameValue | FSUB x,x | +0 |
| 13 | FMUL_Normal | FMUL FP0,FP1 | oracle |
| 14 | FMUL_Zero_Inf | FMUL 0,inf | NaN |
| 15 | FMUL_Neg_Neg | FMUL neg,neg | positive result |
| 16 | FDIV_Normal | FDIV FP0,FP1 | oracle |
| 17 | FDIV_ByZero | FDIV x,0 | ±inf, DZ exception |
| 18 | FDIV_ZeroByZero | FDIV 0,0 | NaN |
| 19 | FSQRT_Positive | FSQRT FP0(4.0) | FP0 = 2.0 |
| 20 | FSQRT_Zero | FSQRT FP0(0) | 0 |
| 21 | FSQRT_Negative | FSQRT FP0(-1) | NaN, OPERR |
| 22 | FSQRT_Denormal | FSQRT(denorm) | oracle |

Est. coverage: **22 tests**

---

## File: `test/gtest/fpu/test_fpu_compare.cpp`

Fixture: `FPUCompareTest`

| # | Test Name | Op | Verification |
|---|-----------|-----|-------------|
| 1 | FCMP_Equal | FCMP FP0,FP1 | FPCC Z=1 |
| 2 | FCMP_Greater | FCMP | FPCC N=0,Z=0 |
| 3 | FCMP_Less | FCMP | FPCC N=1 |
| 4 | FCMP_NaN | FCMP NaN,x | FPCC NAN=1 |
| 5 | FCMP_Inf | FCMP +inf,x | FPCC I depends |
| 6 | FTST_Positive | FTST FP0(>0) | N=0,Z=0 |
| 7 | FTST_Zero | FTST FP0(0) | Z=1 |
| 8 | FTST_Negative | FTST FP0(<0) | N=1 |
| 9 | FTST_NaN | FTST FP0(NaN) | NAN=1 |
| 10 | FBcc_EQ_Taken | FBEQ (Z=1) | branch taken |
| 11 | FBcc_EQ_NotTaken | FBEQ (Z=0) | falls through |
| 12 | FBcc_GT_Taken | FBGT | branch taken |
| 13 | FBcc_UN_NaN | FBUN (NaN) | branch taken |
| 14 | FScc_EQ_True | FSEQ D0 (Z=1) | D0.b = $FF |
| 15 | FScc_EQ_False | FSEQ D0 (Z=0) | D0.b = $00 |
| 16 | FDBcc_CountDown | FDBEQ D0,target | decrement, branch |
| 17 | FTRAPcc_Taken | FTRAPEQ (Z=1) | FP trap |
| 18 | FTRAPcc_NotTaken | FTRAPEQ (Z=0) | no trap |

Est. coverage: **18 tests**

---

## File: `test/gtest/fpu/test_fpu_rounding.cpp`

Fixture: `FPURoundingTest`

| # | Test Name | Op | Verification |
|---|-----------|-----|-------------|
| 1 | FINT_RN_Positive | FINT 1.5 (RN) | 2.0 (round to even) |
| 2 | FINT_RN_Even | FINT 2.5 (RN) | 2.0 (round to even) |
| 3 | FINT_RZ_Positive | FINT 1.9 (RZ) | 1.0 |
| 4 | FINT_RZ_Negative | FINT -1.9 (RZ) | -1.0 |
| 5 | FINT_RM | FINT 1.1 (RM) | 1.0 |
| 6 | FINT_RM_Negative | FINT -1.1 (RM) | -2.0 |
| 7 | FINT_RP | FINT 1.1 (RP) | 2.0 |
| 8 | FINT_RP_Negative | FINT -1.1 (RP) | -1.0 |
| 9 | FINTRZ_Always | FINTRZ 1.9 | 1.0 (always RZ) |
| 10 | FPCR_RoundMode_RN | set FPCR, FADD | verify RN behavior |
| 11 | FPCR_RoundMode_RZ | set FPCR, FADD | verify RZ behavior |
| 12 | FPCR_RoundMode_RM | set FPCR, FADD | verify RM behavior |
| 13 | FPCR_RoundMode_RP | set FPCR, FADD | verify RP behavior |
| 14 | FPCR_Precision_Single | FPCR[7:6]=01 | floatx80_rounding_precision=32 |
| 15 | FPCR_Precision_Double | FPCR[7:6]=10 | floatx80_rounding_precision=64 |
| 16 | FPCR_Precision_Extended | FPCR[7:6]=00 | floatx80_rounding_precision=80 |

Est. coverage: **16 tests**

---

## File: `test/gtest/fpu/test_fpu_040_ops.cpp`

Fixture: `FPU040OpsTest` (cpu=68040)

| # | Test Name | Op | Verification |
|---|-----------|-----|-------------|
| 1 | FSMOVE | FSMOVE FP0,FP1 | single-precision round |
| 2 | FDMOVE | FDMOVE FP0,FP1 | double-precision round |
| 3 | FSSQRT | FSSQRT FP0(4.0) | single-rounded sqrt |
| 4 | FDSQRT | FDSQRT FP0(4.0) | double-rounded sqrt |
| 5 | FSABS | FSABS FP0(-1.5) | single-rounded abs |
| 6 | FDABS | FDABS FP0(-1.5) | double-rounded abs |
| 7 | FSNEG | FSNEG FP0(1.5) | single-rounded neg |
| 8 | FDNEG | FDNEG FP0(1.5) | double-rounded neg |
| 9 | FSADD | FSADD FP0,FP1 | single-precision add |
| 10 | FDADD | FDADD FP0,FP1 | double-precision add |
| 11 | FSSUB | FSSUB FP0,FP1 | single-precision sub |
| 12 | FDSUB | FDSUB FP0,FP1 | double-precision sub |
| 13 | FSMUL | FSMUL FP0,FP1 | single-precision mul |
| 14 | FDMUL | FDMUL FP0,FP1 | double-precision mul |
| 15 | FSDIV | FSDIV FP0,FP1 | single-precision div |
| 16 | FDDIV | FDDIV FP0,FP1 | double-precision div |
| 17 | FSSQRT_NotFINT | FSSQRT opmode | verify not mapped to FINT |

Est. coverage: **17 tests**

---

## File: `test/gtest/fpu/test_fpu_special.cpp`

Fixture: `FPUSpecialTest`

| # | Test Name | Op | Verification |
|---|-----------|-----|-------------|
| 1 | FABS_Positive | FABS FP0(5) | 5 |
| 2 | FABS_Negative | FABS FP0(-5) | 5 |
| 3 | FNEG_Positive | FNEG FP0(5) | -5 |
| 4 | FNEG_NegZero | FNEG FP0(-0) | +0 |
| 5 | FSGLDIV | FSGLDIV FP0,FP1 | SoftFloat single round-trip |
| 6 | FSGLMUL | FSGLMUL FP0,FP1 | SoftFloat single round-trip |
| 7 | FMOD | FMOD FP0,FP1 | IEEE remainder |
| 8 | FREM | FREM FP0,FP1 | IEEE 754 remainder |
| 9 | FGETEXP | FGETEXP FP0 | unbiased exponent |
| 10 | FGETMAN | FGETMAN FP0 | mantissa with implicit bit |
| 11 | FSCALE | FSCALE FP0,FP1 | FP1 * 2^FP0 |

Est. coverage: **11 tests**

---

## File: `test/gtest/fpu/test_fpu_transfer.cpp`

Fixture: `FPUTransferTest`

| # | Test Name | Op | Verification |
|---|-----------|-----|-------------|
| 1 | FMOVEM_RegToMem | FMOVEM FP0-FP3,-(A7) | 4 extended regs pushed |
| 2 | FMOVEM_MemToReg | FMOVEM (A7)+,FP0-FP3 | 4 extended regs popped |
| 3 | FMOVEM_Dynamic | FMOVEM D0,-(A7) | register list from Dn |
| 4 | FMOVE_ToFPCR | FMOVE.L D0,FPCR | FPCR updated |
| 5 | FMOVE_FromFPCR | FMOVE.L FPCR,D0 | D0 = FPCR |
| 6 | FMOVE_ToFPSR | FMOVE.L D0,FPSR | FPSR updated |
| 7 | FMOVE_FromFPSR | FMOVE.L FPSR,D0 | D0 = FPSR |
| 8 | FMOVE_ToFPIAR | FMOVE.L D0,FPIAR | FPIAR updated |
| 9 | FMOVE_FromFPIAR | FMOVE.L FPIAR,D0 | D0 = FPIAR |

Est. coverage: **9 tests**

---

## File: `test/gtest/fpu/test_fpu_constants.cpp`

Fixture: `FPUConstantsTest` (cpu=68030 with 68881/68882; traps on 040)

| # | Test Name | Constant | Expected Value |
|---|-----------|----------|---------------|
| 1 | FMOVECR_Pi | #$00 | 3.14159265358979... |
| 2 | FMOVECR_Log10_2 | #$01 | 0.30102999566... |
| 3 | FMOVECR_e | #$02 | 2.71828182845... |
| 4 | FMOVECR_Log2_e | #$03 | 1.44269504088... |
| 5 | FMOVECR_Log10_e | #$04 | 0.43429448190... |
| 6 | FMOVECR_Zero | #$05 | 0.0 |
| 7 | FMOVECR_Ln2 | #$30 | 0.69314718055... |
| 8 | FMOVECR_Ln10 | #$31 | 2.30258509299... |
| 9 | FMOVECR_One | #$32 | 1.0 |
| 10 | FMOVECR_Ten | #$33 | 10.0 |
| 11 | FMOVECR_Hundred | #$34 | 100.0 |
| 12 | FMOVECR_TenToFour | #$35 | 10000.0 |
| 13 | FMOVECR_TenToEight | #$36 | 1e8 |
| 14 | FMOVECR_040_Traps | on 040 | F-line exception |

Est. coverage: **14 tests**

---

## File: `test/gtest/fpu/test_fpu_exceptions.cpp`

Fixture: `FPUExceptionTest`

| # | Test Name | Op | Verification |
|---|-----------|-----|-------------|
| 1 | DivByZero_FPSR | FDIV x,0 | FPSR DZ bit set |
| 2 | Overflow_FPSR | FMUL huge,huge | FPSR OVFL bit set |
| 3 | Underflow_FPSR | FMUL tiny,tiny | FPSR UNFL bit set |
| 4 | OPERR_SqrtNeg | FSQRT(-1) | FPSR OPERR bit set |
| 5 | INEXACT_FPSR | FDIV 1,3 | FPSR INEX bit set |
| 6 | SNAN_FPSR | FADD SNAN,x | FPSR SNAN bit set |
| 7 | BSUN_Condition | FBcc with NaN | FPSR BSUN bit set |
| 8 | Trap_040_FSIN | FSIN on 040 | F-line exception |
| 9 | Trap_040_FCOS | FCOS on 040 | F-line exception |
| 10 | Trap_040_FTAN | FTAN on 040 | F-line exception |
| 11 | Trap_040_FETOX | FETOX on 040 | F-line exception |
| 12 | Trap_040_FLOGN | FLOGN on 040 | F-line exception |
| 13 | Trap_040_PackedDst | FMOVE FP0,packed on 040 | F-line exception |
| 14 | Trap_040_PackedSrc | FMOVE packed,FP0 on 040 | F-line exception |

Est. coverage: **14 tests**

---

## Cumulative FPU Test Summary

| File | Tests |
|------|:-----:|
| test_fpu_basic.cpp | 22 |
| test_fpu_compare.cpp | 18 |
| test_fpu_rounding.cpp | 16 |
| test_fpu_040_ops.cpp | 17 |
| test_fpu_special.cpp | 11 |
| test_fpu_transfer.cpp | 9 |
| test_fpu_constants.cpp | 14 |
| test_fpu_exceptions.cpp | 14 |
| **Total** | **121** |
