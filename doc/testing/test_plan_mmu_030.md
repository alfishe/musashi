# 68030 MMU Test Plan

Applicable CPU: 68030 only. The 030 MMU uses PMOVE for register access,
supports 2/3/4-level table walks, variable page sizes, and 22-entry ATC
with pseudo-LRU.

---

## File: `test/gtest/mmu/mmu_030/test_030_tc.cpp`

Fixture: `MMU030TCTest` (cpu=68030)

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | TC_ReadWrite | PMOVE to/from TC, verify round-trip |
| 2 | TC_Enable | Set E bit, translation active |
| 3 | TC_Disable | Clear E bit, translation bypassed |
| 4 | TC_PageSize_256 | PS=0x08 (256 bytes) |
| 5 | TC_PageSize_4K | PS=0x0C (4096 bytes) |
| 6 | TC_PageSize_8K | PS=0x0D (8192 bytes) |
| 7 | TC_FlushATC | Writing TC flushes all ATC entries |
| 8 | TC_ISbits | IS field shifts initial lookup |

Est. coverage: **8 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_tt.cpp`

Fixture: `MMU030TTTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | TT0_ReadWrite | PMOVE to/from TT0 |
| 2 | TT1_ReadWrite | PMOVE to/from TT1 |
| 3 | TT_Enable | E bit enables transparent region |
| 4 | TT_AddressMatch | Base/mask matching |
| 5 | TT_FCMatch | FC field filtering |
| 6 | TT_RWField | R/W bit restricts read/write |
| 7 | TT_Priority | TT checked before table walk |
| 8 | TT_WP | Write-protect in TT |

Est. coverage: **8 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_walk.cpp`

Fixture: `MMU030WalkTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | Walk_2Level | Root→Page (IS skips levels) |
| 2 | Walk_3Level | Root→Pointer→Page |
| 3 | Walk_4Level | Root→Ptr→Ptr→Page |
| 4 | Walk_4K_Page | 4KB page, correct PA bits |
| 5 | Walk_8K_Page | 8KB page, correct PA bits |
| 6 | Walk_ShortDesc | 4-byte descriptors |
| 7 | Walk_LongDesc | 8-byte descriptors |
| 8 | Walk_InvalidRoot | Invalid descriptor at root |
| 9 | Walk_InvalidPointer | Invalid at pointer level |
| 10 | Walk_IndirectDesc | Indirect descriptor resolution |
| 11 | Walk_URP_SRP | Separate user/supervisor root |
| 12 | Walk_MultiplePages | Map several pages, verify each |

Est. coverage: **12 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_atc.cpp`

Fixture: `MMU030ATCTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | ATC_Hit | Walk populates ATC, second access hits |
| 2 | ATC_Miss | Unmapped address misses ATC |
| 3 | ATC_Eviction | Fill 22 entries, verify LRU victim |
| 4 | ATC_PseudoLRU | Access pattern, verify LRU tree |
| 5 | ATC_FC_Isolation | User/super entries separate |
| 6 | ATC_Writeback_M | Modified bit set in ATC on write |
| 7 | ATC_CoSim_Insert | Co-sim callback on ATC add |

Est. coverage: **7 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_pflush.cpp`

Fixture: `MMU030PflushTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | PFLUSHA | Flush all ATC entries |
| 2 | PFLUSH_FC | Flush by function code |
| 3 | PFLUSH_FC_EA | Flush by FC + effective address |
| 4 | PFLUSH_FC_Mask | FC mask filtering |
| 5 | PFLUSH_CoSim | Co-sim FLUSH callback fires |

Est. coverage: **5 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_ptest.cpp`

Fixture: `MMU030PtestTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | PTESTR_Valid | MMUSR R=1, physical addr correct |
| 2 | PTESTW_Valid | MMUSR R=1, M=1 after write test |
| 3 | PTEST_Invalid | MMUSR R=0 for invalid page |
| 4 | PTEST_WP | MMUSR W=1 for WP page |
| 5 | PTEST_Supervisor | MMUSR S=1 for super page |
| 6 | PTEST_Level | MMUSR level field |
| 7 | PTEST_Multiple | Sequential PTEST, different addresses |

Est. coverage: **7 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_pmove.cpp`

Fixture: `MMU030PmoveTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | PMOVE_TC_Write | Write TC register |
| 2 | PMOVE_TC_Read | Read TC register |
| 3 | PMOVE_SRP_Write | Write SRP (64-bit) |
| 4 | PMOVE_CRP_Write | Write CRP (64-bit) |
| 5 | PMOVE_TT0_Write | Write TT0 |
| 6 | PMOVE_TT1_Write | Write TT1 |
| 7 | PMOVE_MMUSR_Read | Read MMUSR |
| 8 | PMOVE_040_Traps | PMOVE on 040 → F-line trap |

Est. coverage: **8 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_protection.cpp`

Fixture: `MMU030ProtectionTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | WP_WriteFault | Write to WP page → bus error |
| 2 | WP_ReadAllowed | Read from WP page succeeds |
| 3 | Super_UserFault | User access to S-only page → bus error |
| 4 | Super_SuperAllowed | Supervisor access to S-only succeeds |
| 5 | BusError_FormatB | Verify Format $B stack frame |
| 6 | BusError_SSW | SSW DF, RW, FC bits correct |
| 7 | BusError_FaultAddr | Fault address in frame |

Est. coverage: **7 tests**

---

## File: `test/gtest/mmu/mmu_030/test_030_pload.cpp`

Fixture: `MMU030PloadTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | PLOADR | Preload ATC for read |
| 2 | PLOADW | Preload ATC for write |
| 3 | PLOAD_SFC | Uses SFC for function code |
| 4 | PLOAD_DFC | Uses DFC for function code |
| 5 | PLOAD_ATCPopulated | Subsequent access hits ATC |

Est. coverage: **5 tests**

---

## 030 MMU Test Summary

| File | Tests |
|------|:-----:|
| test_030_tc.cpp | 8 |
| test_030_tt.cpp | 8 |
| test_030_walk.cpp | 12 |
| test_030_atc.cpp | 7 |
| test_030_pflush.cpp | 5 |
| test_030_ptest.cpp | 7 |
| test_030_pmove.cpp | 8 |
| test_030_protection.cpp | 7 |
| test_030_pload.cpp | 5 |
| **Total** | **67** |
