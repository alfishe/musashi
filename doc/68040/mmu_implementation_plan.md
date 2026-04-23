# 68040 MMU Implementation Plan

## Overview

The 68040 MMU differs significantly from the 68030:
- Fixed 3-level page table (root, pointer, page)
- Separate URP/SRP (no SRE bit)
- Split TT registers: ITT0/ITT1 (instruction), DTT0/DTT1 (data)
- Fixed page sizes: 4KB or 8KB only
- 4-byte descriptors only
- Simpler TC register

## Current State (Post-Stabilization)

### ✅ Fully Implemented and Tested:
- **Register access**: mmu040_tc, mmu040_urp, mmu040_srp, mmu040_itt0/1, mmu040_dtt0/1
- **MOVEC access**: TC, URP, SRP, ITT0/1, DTT0/1, MMUSR (read/write)
- **Table walking**: Full 3-level walk in `pmmu_translate_addr_with_fc_040()`
- **ATC caching**: `pmmu040_atc_lookup()` / `pmmu040_atc_add()` with pseudo-LRU eviction
- **TT matching**: `pmmu040_match_tt()` with S-field and WP checking
- **PFLUSH variants**: PFLUSH (An), PFLUSHN (An), PFLUSHA
- **PTEST**: PTESTR (SFC) / PTESTW (DFC) with correct MMUSR population
- **MMUSR bit layout**: Per MC68040 UM Section 3 (G=bit9, U1=bit8, U0=bit7, S=bit6, CM=bit5:4, M=bit3, W=bit2, T=bit1, R=bit0)
- **U0/U1 user page attributes**: Propagated through ATC and PTEST paths
- **U/M bit writeback**: On page access during table walks
- **Write-protect checking**: Bus error on write to WP page
- **Supervisor-only detection**: S bit in page descriptor
- **Indirect descriptors**: Supported in page table entries
- **Co-sim callbacks**: translate() on ATC hit and table walk; atc() on INSERT, FLUSH
- **Bus error frames**: Format $7 (30 words, simplified) with 040 SSW layout
- **CPU type isolation**: `CPU_TYPE_IS_040()` macro for 040-specific paths

### Test Suite (17 tests — all passing):
- `movec_tc` — TC register read/write with DTT0/ITT0 guard
- `movec_urp_srp` — URP/SRP register access
- `mmu_simple` — Identity-mapped 4K page walk
- `tt_data` — DTT matching for data accesses
- `walk_4k` — 4KB page table walk
- `walk_8k` — 8KB page table walk
- `ptest_wp` — PTEST detecting write-protect
- `ptest_super` — PTEST detecting supervisor-only
- `ptest_modified` — PTEST detecting modified bit
- `ptest_global` — PTEST detecting global bit (bit 9)
- `pflusha` — PFLUSHA flush all
- `pflush_page` — PFLUSH single page
- `pflushn` — PFLUSHN preserves global entries
- `invalid_page` — Invalid descriptor handling
- `cinv_cpush` — CINV/CPUSH dispatch (no-op)
- `indirect_desc` — Indirect page descriptors
- `um_bits` — U/M bit writeback

## Architecture Notes

### MMUSR Layout (040-specific)
```
Bits 31:12 — Physical page address (4K) or bits 31:13 (8K)
Bit  9 — G (global)
Bit  8 — U1 (user page attribute 1)
Bit  7 — U0 (user page attribute 0)
Bit  6 — S (supervisor only)
Bit  5 — CM1 (cache mode bit 1)
Bit  4 — CM0 (cache mode bit 0)
Bit  3 — M (modified)
Bit  2 — W (write protected)
Bit  1 — T (transparent translation hit)
Bit  0 — R (resident)
```

### Format $7 Bus Error Frame (040 Access Fault)
30-word stack frame per MC68040 UM Section 8.4.2.
Currently implements simplified model: SSW + fault address + effective address populated;
internal pipeline state fields zeroed. Sufficient for OS exception handling and co-sim.

### 040 SSW Layout (different from 030)
```
Bit 12:   CP  — Continuation/Pending
Bit 11:   CU  — Copyback/Update
Bit 10:   CT  — Continuation/Transfer type
Bit  9:   CM  — Completion/Misaligned
Bit  8:   MA  — Misaligned Access
Bit  7:   ATC — ATC Fault (vs bus error)
Bit  6:   LK  — Locked transfer
Bit  5:   RW  — Read/Write (1=read, 0=write)
Bits 4-3: SIZ — Transfer size
Bits 2-0: TT  — Transfer type
```

## File Changes Summary

### m68kmmu.h
- MMUSR constants: G=0x0200, U0=0x0080, U1=0x0100
- Page descriptor constants: PD_U0=0x0100, PD_U1=0x0200
- ATC flags: ATC_U0, ATC_U1
- U0/U1 propagation through ATC add/lookup/PTEST
- Co-sim callbacks in translate and flush paths

### m68kcpu.h
- Format $7 stack frame builder: `m68ki_stack_frame_0111()`
- Bus error handler: 040+ uses Format $7, 020/030 uses Format $B

### test/mc68040/
- Fixed movec_tc.s: DTT0/ITT0 guard before MMU enable
- Fixed ptest_global.s: G bit mask 0x200 (was 0x80)
- Fixed pflushn.s: G bit mask 0x200 (was 0x80)

### Makefile
- Fixed test target: added build_tests prerequisite
