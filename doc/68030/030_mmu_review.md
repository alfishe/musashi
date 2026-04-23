# 030 MMU Implementation Review

This document summarizes the progress against the `task.md` and `mmu_implementation_plan.md` for the 68030 MMU implementation.

## ✅ Covered (Phases 1 & 2 Core)

The core translation, infrastructure, and instructions have been successfully implemented:

*   **ATC Infrastructure:** `mmu_atc_tag`, `mmu_atc_data`, and TT0/TT1 registers added to `m68ki_cpu_core` and cleared on reset.
*   **Translation & TT Matching:** `pmmu_match_tt` correctly handles TT0/TT1 overrides. Table walking correctly threads the function code (FC) through `pmmu_translate_addr_with_fc`.
*   **Instruction Emulation:** `PFLUSH`, `PTEST`, and `PLOAD` have been successfully ported from MAME to Musashi. PMOVE for TT0/TT1 is implemented.
*   **MMU Walking details:** `pmmu_update_descriptor` correctly updates U/M bits. `pmmu_update_sr` populates the MMUSR correctly.
*   **Bus Error Triggering:** `pmmu_set_buserror()` replaces fatal error aborts.

## ✅ Fixed (April 2026)

*   **CRP/SRP registers cleared on reset:** Added `mmu_crp_aptr`, `mmu_crp_limit`, `mmu_srp_aptr`, `mmu_srp_limit` clearing in `m68k_pulse_reset()`.
*   **Read paths check mmu_tmp_buserror_occurred:** Fixed `m68ki_read_8_fc`, `m68ki_read_16_fc`, `m68ki_read_32_fc` to check for bus errors after MMU translation.
*   **Instruction fetch through MMU:** Fixed `m68ki_read_imm_16()` and `m68ki_read_imm_32()` to translate PC through MMU (removed broken undefined `address` variable).
*   **TC Register Validation:** PMOVE to TC now validates IS + TIA + TIB + TIC + TID + PS = 32, raising configuration error (vector 56) if invalid.
*   **PLRU Replacement Policy:** Replaced round-robin with pseudo-LRU for ATC replacement. History register `mmu_atc_history` tracks recently used entries.
*   **030 Bus Error Frames:** `m68ki_exception_bus_error()` now builds Format $B stack frames for 68020/030 with SSW and fault address fields populated.

## ✅ Co-Simulation Interface (April 2026)

*   **`M68K_MMU_COSIM` compile toggle** in `m68kconf.h` — OFF by default, zero overhead.
*   **Guarded callback infrastructure** — when OFF, no function pointers in CPU struct, no branches in hot path.
*   **Three callback hooks:**
    - `m68k_set_mmu_translate_callback()` — fires on every address translation (both ATC hit and table walk paths)
    - `m68k_set_mmu_atc_callback()` — fires on ATC insert and flush operations
    - `m68k_set_mmu_fault_callback()` — fires on access fault (bus error)
*   **`m68k_get_atc_entries()`** — dumps valid ATC entries for state comparison with RTL TLB.
*   **Backwards compatible** — follows existing Musashi callback pattern (`M68K_OPT_OFF`/`M68K_OPT_ON`), no impact on PiStorm, MAME, or any consumer that doesn't define `M68K_MMU_COSIM`.

## ✅ CI Integration

*   **`mc68030` wired into `test/Makefile`** — 030 tests now build alongside 68000/040/060.

## ✅ Test Coverage (14 tests)

| Test | Validates |
|---|---|
| `pmove_tt` | PMOVE to/from TT0/TT1 |
| `pmove_tc` | PMOVE to TC, enable/disable |
| `mmu_identity` | Identity mapping via TT registers |
| `pflush_all` | PFLUSHA full ATC invalidation |
| `pflush_fc` | PFLUSH by FC / FC+EA |
| `ptest_walk` | Table walk with PTEST |
| `ptest_wp` | Write-protect detection via PTEST |
| `ptest_invalid` | Invalid descriptor detection |
| `ptest_multi` | 3-level table walk verification |
| `ptest_super` | Supervisor-only page detection |
| `pload_atc` | PLOAD ATC preload |
| `tt_match` | Transparent translation matching |
| `wp_buserr` | Write to WP page → bus error exception |
| `mmu_reset` | CRP/SRP cleared on reset |

## Deferred Items

### 1. Co-Simulation ATC Replacement Hook (Optional)
*   **Status:** Deferred
*   **Description:** Callback hook to let external testbench dictate ATC replacement choices (bypass pseudo-LRU).
*   **Note:** Can be added later if FPGA RTL uses a different eviction algorithm.

### 2. Apple HMMU (Mac II/LC)
*   **Status:** Deferred — not relevant for current MiSTer targets.

### 3. 040/060 MMU Stabilization
*   **Status:** Deferred — separate branches per user decision.