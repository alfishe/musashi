# 68040 MMU Implementation Plan

## Overview

The 68040 MMU differs significantly from the 68030:
- Fixed 3-level page table (root, pointer, page)
- Separate URP/SRP (no SRE bit)
- Split TT registers: ITT0/ITT1 (instruction), DTT0/DTT1 (data)
- Fixed page sizes: 4KB or 8KB only
- 4-byte descriptors only
- Simpler TC register

## Current State

### What Works:
- Register storage: mmu040_tc, mmu040_urp, mmu040_srp, mmu040_itt0/1, mmu040_dtt0/1
- MOVEC access to all registers
- Basic 3-level table walk in `pmmu_translate_addr_with_fc_040()`
- U/M bit updates in page descriptors
- Write-protect checking
- Supervisor-only page detection (bit 7)

### Issues to Fix:
1. **No isolation from 060**: Uses `CPU_TYPE_IS_040_PLUS` which includes 060
2. **No ATC caching**: Every access does full table walk
3. **PTEST SR incomplete**: Line 902 mixes address and page bits oddly
4. **No 040-specific PFLUSH**: Only PFLUSHA exists

## Implementation Tasks

### 1. Add CPU Type Macros for Isolation
```c
#define CPU_TYPE_IS_040(A) ((A) & (CPU_TYPE_040 | CPU_TYPE_EC040))
```

### 2. Fix FC Interpretation for TT Selection
Current code at lines 706-720 is confusing. The 68040 FC encoding:
- FC=1: User data → DTT
- FC=2: User program → ITT
- FC=5: Supervisor data → DTT
- FC=6: Supervisor program → ITT
- FC=7: CPU space (no TT)

### 3. Implement 040 ATC
Reuse existing ATC structures from 030:
- `mmu_atc_tag[MMU_ATC_ENTRIES]`
- `mmu_atc_data[MMU_ATC_ENTRIES]`
- `mmu_atc_rr` (round-robin counter)

Add ATC lookup before table walk, ATC add after successful walk.

### 4. Fix PTEST MMUSR Population
68040 MMUSR format:
- Bits 31:10: Physical address (page frame)
- Bit 9: G (global)
- Bit 8: U1 (user page attribute)
- Bit 7: U0 (user page attribute)
- Bit 6: S (supervisor only)
- Bit 5: CM1 (cache mode)
- Bit 4: CM0 (cache mode)
- Bit 3: M (modified)
- Bit 2: W (write protected)
- Bit 1: T (transparent translation hit)
- Bit 0: R (resident)

### 5. Implement 040 PFLUSH
68040 has:
- PFLUSHA: Flush entire ATC (opcode 0xF518)
- PFLUSH (An): Flush entries matching address in An (opcode 0xF508-0xF510)

### 6. Create Test Suite
Tests needed:
- movec_urp_srp: URP/SRP register access
- movec_tc: TC register access
- movec_tt: ITT0/1, DTT0/1 register access
- tt_data: DTT matching for data accesses
- tt_instr: ITT matching for instruction accesses
- walk_4k: 4KB page table walk
- walk_8k: 8KB page table walk
- ptest_walk: PTEST with full table walk
- ptest_wp: PTEST detecting write-protect
- ptest_super: PTEST detecting supervisor-only
- pflush_page: PFLUSH single page
- pflusha: PFLUSHA flush all
- wp_buserr: Write-protect bus error
- invalid_buserr: Invalid descriptor bus error

## File Changes

### m68kcpu.h
- Add `CPU_TYPE_IS_040(A)` macro

### m68kmmu.h
- Fix FC interpretation in `pmmu_translate_addr_with_fc_040()`
- Add ATC lookup/add calls for 040
- Fix PTEST MMUSR population
- Add 040 PFLUSH handler

### test/mc68040/
- Create entry_040_mmu.s (test harness)
- Create all test files
- Update Makefile

### Makefile
- Add 68040 MMU test targets
