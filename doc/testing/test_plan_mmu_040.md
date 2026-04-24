# 68040 MMU Test Plan

Applicable CPU: 68040 only. The 040 MMU uses MOVEC for register access,
has a fixed 3-level walk (root→pointer→page), separate URP/SRP, split
ITT/DTT registers, U0/U1/G page attributes, and 64-entry ATC with
pseudo-LRU. PMOVE is illegal on 040 (F-line trap).

---

## File: `test/gtest/mmu/mmu_040/test_040_tc.cpp`

Fixture: `MMU040TCTest` (cpu=68040)

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | TC_ReadWrite | MOVEC to/from TC |
| 2 | TC_Enable | Set E bit, translation active |
| 3 | TC_Disable | Clear E bit, bypass |
| 4 | TC_PageSize_4K | P=0 (4KB) |
| 5 | TC_PageSize_8K | P=1 (8KB) |
| 6 | TC_FlushATC | Writing TC flushes ATC |
| 7 | URP_ReadWrite | MOVEC URP round-trip |
| 8 | SRP_ReadWrite | MOVEC SRP round-trip |
| 9 | URP_SRP_Separate | User uses URP, super uses SRP |

Est. coverage: **9 tests**

---

## File: `test/gtest/mmu/mmu_040/test_040_tt.cpp`

Fixture: `MMU040TTTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | ITT0_ReadWrite | MOVEC ITT0 |
| 2 | ITT1_ReadWrite | MOVEC ITT1 |
| 3 | DTT0_ReadWrite | MOVEC DTT0 |
| 4 | DTT1_ReadWrite | MOVEC DTT1 |
| 5 | ITT_InstructionFetch | ITT matches instruction fetch |
| 6 | DTT_DataAccess | DTT matches data access |
| 7 | TT_AddressMask | Base/mask address matching |
| 8 | TT_FC_Filter | FC matching |
| 9 | TT_SField | S-field user/super/both |
| 10 | TT_WP | Write-protect in TT |
| 11 | TT_CacheMode | CM bits propagated |

Est. coverage: **11 tests**

---

## File: `test/gtest/mmu/mmu_040/test_040_walk.cpp`

Fixture: `MMU040WalkTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | Walk_4K_Identity | VA=PA, 4KB page |
| 2 | Walk_8K_Identity | VA=PA, 8KB page |
| 3 | Walk_4K_Remap | VA≠PA, verify physical output |
| 4 | Walk_3Level | Root→pointer→page descriptor chain |
| 5 | Walk_InvalidRoot | Invalid root descriptor → bus error |
| 6 | Walk_InvalidPointer | Invalid pointer level → bus error |
| 7 | Walk_InvalidPage | Invalid page descriptor → bus error |
| 8 | Walk_IndirectDesc | Indirect page descriptor |
| 9 | Walk_IndirectDepth | Indirect chain depth limit (4) |
| 10 | Walk_MultiPage | Map 4 pages, verify each translation |
| 11 | Walk_CoSim_Translate | Co-sim translate callback on walk |

Est. coverage: **11 tests**

---

## File: `test/gtest/mmu/mmu_040/test_040_atc.cpp`

Fixture: `MMU040ATCTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | ATC_Hit | Walk populates, re-access hits |
| 2 | ATC_Miss | Unmapped addr misses |
| 3 | ATC_LRU_Eviction | Fill beyond capacity, LRU evicted |
| 4 | ATC_PseudoLRU_Tree | Verify 3-bit LRU tree state |
| 5 | ATC_U0U1_Propagation | U0/U1 from PD stored in ATC |
| 6 | ATC_Global_Bit | G bit preserved across PFLUSHN |
| 7 | ATC_CoSim_Insert | INSERT callback fires |
| 8 | ATC_FC_Isolation | User/super separate entries |

Est. coverage: **8 tests**

---

## File: `test/gtest/mmu/mmu_040/test_040_pflush.cpp`

Fixture: `MMU040PflushTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | PFLUSHA | Flush all entries |
| 2 | PFLUSH_An | Flush page by address |
| 3 | PFLUSHN_An | Flush non-global page |
| 4 | PFLUSHN_Global | Global entry preserved |
| 5 | PFLUSH_CoSim | FLUSH callback fires |
| 6 | PFLUSH_Invalidates | Subsequent access forces walk |

Est. coverage: **6 tests**

---

## File: `test/gtest/mmu/mmu_040/test_040_ptest.cpp`

Fixture: `MMU040PtestTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | PTESTR_Valid | MMUSR R=1, PA correct |
| 2 | PTESTW_Valid | MMUSR R=1 |
| 3 | PTEST_Invalid | MMUSR R=0 |
| 4 | PTEST_WP | MMUSR W=1 |
| 5 | PTEST_Supervisor | MMUSR S=1 |
| 6 | PTEST_Global | MMUSR G=1 (bit 9) |
| 7 | PTEST_Modified | MMUSR M=1 |
| 8 | PTEST_U0U1 | MMUSR U0, U1 bits |
| 9 | PTEST_CacheMode | MMUSR CM bits |
| 10 | PTEST_TT_Hit | MMUSR T=1 for TT match |
| 11 | PTESTR_SFC | Uses SFC for FC |
| 12 | PTESTW_DFC | Uses DFC for FC |

Est. coverage: **12 tests**

---

## File: `test/gtest/mmu/mmu_040/test_040_protection.cpp`

Fixture: `MMU040ProtectionTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | WP_Root_WriteFault | WP at root level → bus error |
| 2 | WP_Pointer_WriteFault | WP at pointer level → bus error |
| 3 | WP_Page_WriteFault | WP at page level → bus error |
| 4 | WP_ReadAllowed | Read from WP succeeds |
| 5 | Super_UserFault | User access to S page → bus error |
| 6 | BusError_Format7 | Verify Format $7 (30-word frame) |
| 7 | SSW_ATC_Bit | ATC bit in SSW |
| 8 | SSW_LK_Bit_CAS | LK=1 for CAS bus error |
| 9 | SSW_LK_Bit_TAS | LK=1 for TAS bus error |
| 10 | SSW_RW_Bit | RW=1 for read, RW=0 for write |
| 11 | SSW_SIZ_Bits | SIZ correct for byte/word/long |
| 12 | FaultAddress | Correct fault address in frame |

Est. coverage: **12 tests**

---

## File: `test/gtest/mmu/mmu_040/test_040_um_bits.cpp`

Fixture: `MMU040UMBitsTest`

| # | Test Name | Verification |
|---|-----------|-------------|
| 1 | U_Bit_SetOnAccess | U bit set in page descriptor |
| 2 | M_Bit_SetOnWrite | M bit set on write access |
| 3 | M_Bit_ClearOnRead | M bit not set for read-only |
| 4 | U_Bit_Writeback | Re-read PTE, verify U set |
| 5 | M_Bit_Writeback | Re-read PTE, verify M set |

Est. coverage: **5 tests**

---

## 040 MMU Test Summary

| File | Tests |
|------|:-----:|
| test_040_tc.cpp | 9 |
| test_040_tt.cpp | 11 |
| test_040_walk.cpp | 11 |
| test_040_atc.cpp | 8 |
| test_040_pflush.cpp | 6 |
| test_040_ptest.cpp | 12 |
| test_040_protection.cpp | 12 |
| test_040_um_bits.cpp | 5 |
| **Total** | **74** |
