# Musashi MMU — Milestone 1 Tasks (68030)

## Phase 1: ATC Infrastructure
- [x] Add TT0/TT1, ATC arrays, replacement policy fields to `m68ki_cpu_core` struct in `m68kcpu.h`
- [ ] Implement ATC functions in `m68kmmu.h`: lookup, insert, flush_all, flush_fc, flush_fc_ea
- [ ] Implement configurable replacement: FIFO default + callback hook
- [x] Add ATC reset in `m68k_pulse_reset()`

## Phase 2: 030 MMU Compliance (port from MAME)
- [ ] Port `pmmu_match_tt()` — Transparent Translation matching
- [ ] Port `fc_from_modes()` — FC level select (SFC/DFC/Dn/#imm)
- [ ] Port PMOVE TT0/TT1 support (register codes 0x02, 0x03)
- [ ] Port `pmmu_atc_flush_fc_ea()` — PFLUSH (all 3 forms + PFLUSHA)
- [ ] Port `m68851_ptest()` — PTEST with MMUSR population + A-register
- [ ] Port `m68851_pload()` — PLOAD ATC preload
- [ ] Port `update_sr()` — MMUSR bit population during table walk
- [ ] Port `update_descriptor()` — U/M bit writeback to descriptors
- [ ] Port write-protection checking with bus error
- [ ] Port page size from TC PS field
- [ ] Port `pmmu_set_buserror()` — replace fatalerror() with bus error
- [ ] Port FC threading through `pmmu_translate_addr_with_fc()`
- [ ] Port indirect descriptor handling
- [ ] Port TC validation (bits must sum to 32)
- [ ] Port FD bit (flush disable) conditional ATC flush on PMOVE
- [ ] Add PMOVE TT0/TT1 to MOVEC handler for 030 (if needed)

## Phase 5.1: 030 Bus Error Frames
- [ ] Implement Format $A/$B 46-word stack frame builder
- [ ] Populate architectural fields: SSW, Fault Address, Data Output Buffer
- [ ] Fill internal pipeline words with 0x0000
- [ ] Hook into MMU fault path

## Phase 6: Co-Simulation Interface
- [ ] Create `m68k_mmu_cosim.h` with callback typedefs
- [ ] Add `m68k_set_mmu_translate_callback()`
- [ ] Add `m68k_set_mmu_atc_callback()`
- [ ] Add `m68k_set_mmu_fault_callback()`
- [ ] Add `m68k_set_atc_replace_callback()`
- [ ] Add `m68k_get_atc_entries()` state dump API
- [ ] Add `M68K_MMU_COSIM` compile toggle to `m68kconf.h`
- [ ] Fire callbacks at appropriate points in translation/ATC/fault paths

## Verification
- [ ] Existing tests pass with PMMU disabled
- [ ] Existing tests pass with PMMU enabled, TC=0
- [ ] Build clean with no warnings
