# Musashi MMU — Milestone 1 Tasks (68030)

## Phase 1: ATC Infrastructure
- [x] Add TT0/TT1, ATC arrays, replacement policy fields to `m68ki_cpu_core` struct in `m68kcpu.h`
- [x] Implement ATC functions in `m68kmmu.h`: lookup, insert, flush_all, flush_fc, flush_fc_ea
- [x] Implement pseudo-LRU replacement with `mmu_atc_history` register
- [x] Add ATC reset in `m68k_pulse_reset()`

## Phase 2: 030 MMU Compliance (port from MAME)
- [x] Port `pmmu_match_tt()` — Transparent Translation matching
- [x] Port `fc_from_modes()` — FC level select (SFC/DFC/Dn/#imm)
- [x] Port PMOVE TT0/TT1 support (register codes 0x02, 0x03)
- [x] Port `pmmu_atc_flush_fc_ea()` — PFLUSH (all 3 forms + PFLUSHA)
- [x] Port `m68851_ptest()` — PTEST with MMUSR population + A-register
- [x] Port `m68851_pload()` — PLOAD ATC preload
- [x] Port `update_sr()` — MMUSR bit population during table walk
- [x] Port `update_descriptor()` — U/M bit writeback to descriptors
- [x] Port write-protection checking with bus error
- [x] Port page size from TC PS field
- [x] Port `pmmu_set_buserror()` — replace fatalerror() with bus error
- [x] Port FC threading through `pmmu_translate_addr_with_fc()`
- [x] Port indirect descriptor handling
- [x] Port TC validation (bits must sum to 32)
- [x] Port FD bit (flush disable) conditional ATC flush on PMOVE

## Phase 5.1: 030 Bus Error Frames
- [x] Implement Format $B stack frame builder for 68020/030
- [x] Populate architectural fields: SSW, Fault Address, Data Output Buffer
- [x] Fill internal pipeline words with 0x0000
- [x] Hook into MMU fault path

## Phase 6: Co-Simulation Interface
- [x] Add `M68K_MMU_COSIM` compile toggle to `m68kconf.h`
- [x] Add guarded callback typedefs + function pointers to CPU struct
- [x] Add no-op macros when `M68K_MMU_COSIM` is OFF
- [x] Add `m68k_set_mmu_translate_callback()` setter
- [x] Add `m68k_set_mmu_atc_callback()` setter
- [x] Add `m68k_set_mmu_fault_callback()` setter
- [x] Add `m68k_get_atc_entries()` state dump API
- [x] Fire translate callback on ATC hit path
- [x] Fire translate callback on table walk path
- [x] Fire ATC callback on insert
- [x] Fire ATC callback on flush all
- [x] Fire fault callback on bus error
- [x] Init callbacks to NULL in `m68k_init()`

## CI Integration
- [x] Wire `mc68030` into `test/Makefile` (all + clean targets)

## Verification
- [x] Build clean with `M68K_MMU_COSIM` OFF (default)
- [x] Build clean with `M68K_MMU_COSIM` ON (`-DM68K_MMU_COSIM=1`)
- [x] 14 test binaries compiled and present
