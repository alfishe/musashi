# 68040 MMU Stabilization Plan

## Background

The 040 branch now inherits all 030 review fixes (PLRU, Format $B, co-sim hooks) via the merge. The 040 MMU code is largely functional — table walking, TT matching, ATC, PFLUSH/PFLUSHA/PFLUSHN, and PTESTR/PTESTW are all implemented. The stabilization work is about closing the remaining gaps and hardening correctness.

## Audit Findings

### ✅ Already Working (from 040 branch)
- `pmmu040_match_tt()` — TT register matching with S-field and WP checking
- `pmmu040_atc_lookup()` / `pmmu040_atc_add()` — 040-specific ATC with pseudo-LRU
- `pmmu_translate_addr_with_fc_040()` — full 3-level walk, 4K/8K pages, indirect descriptors
- PFLUSH (An), PFLUSHN (An), PFLUSHA — all three 040 flush variants
- PTESTR (An) via SFC, PTESTW (An) via DFC
- CINV/CPUSH — dispatched as no-ops
- MOVEC: TC, URP, SRP, ITT0/1, DTT0/1, MMUSR
- U/M bit writeback on page access
- Co-sim callbacks inherited from merge
- 17 test binaries (register access, walks, PFLUSH, PTEST, CINV/CPUSH, indirect, U/M bits)

### Gaps Found

#### 1. **MMUSR G bit position is wrong** (Severity: HIGH)
The 040 MMUSR layout per Motorola manual places the G bit at bit **9** (not bit 7 as currently coded):
```
Bits 31:12 — Physical page address (4K) or bits 31:13 (8K)
Bit  9 — G (global)
Bit  8 — U1 (user page attribute 1)  ← NOT IMPLEMENTED
Bit  7 — U0 (user page attribute 0)  ← NOT IMPLEMENTED
Bit  6 — S (supervisor only)
Bit  5 — CM1 (cache mode bit 1)
Bit  4 — CM0 (cache mode bit 0)
Bit  3 — M (modified)
Bit  2 — W (write protected)
Bit  1 — T (transparent translation hit)
Bit  0 — R (resident)
```

Currently `M68K_MMU040_SR_GLOBAL` is `0x0080` (bit 7) — should be `0x0200` (bit 9).

#### 2. **U0/U1 user page attribute bits missing** (Severity: MEDIUM)
The 040 has U0 (bit 8 of page descriptor) and U1 (bit 9 of page descriptor) which map to MMUSR bits 7 and 8. These are not defined or propagated.

#### 3. **Co-sim callbacks not fired in 040 paths** (Severity: MEDIUM)
The 040 translation function doesn't fire `m68ki_mmu_cosim_translate()` on the ATC hit or table walk paths. Only `m68ki_mmu_cosim_atc(INSERT)` is fired via `pmmu040_atc_add()` from the merge.

#### 4. **PFLUSH co-sim callbacks missing** (Severity: LOW)
The 040 `PFLUSH (An)` and `PFLUSHN (An)` don't fire `m68ki_mmu_cosim_atc(FLUSH)`.

#### 5. **040 bus error frame format** (Severity: MEDIUM)
The 040 uses **Format $7** stack frames (30 words) for access faults, not Format $B (030). The current `m68ki_exception_bus_error()` generates Format $B for 020/030+ but doesn't have a Format $7 path.

> [!IMPORTANT]
> Format $7 is complex (30 words). For MiSTer/co-sim purposes, the critical fields are the fault address and the SSW. We can implement a simplified version that populates the key fields and zeros the rest.

#### 6. **040 PTEST opcode confusion** (Severity: LOW)
PTESTR uses SFC (opcode F548), PTESTW uses DFC (opcode F568). This is currently **correct** in the code. However, some tests use F568 with the comment "PTESTR" which is misleading but doesn't affect correctness.

#### 7. **8K page index field mask** (Severity: LOW)
For 8K pages, the page index bits 17:13 should yield 5 bits (0x1f), and pointer_entry mask is `~0x7f`. This looks correct.

## Proposed Changes

### Phase 1: MMUSR Bit Position Fix
#### [MODIFY] [m68kmmu.h](file:///Volumes/TB4-4Tb/Projects/mister/musashi/m68kmmu.h)
- Fix `M68K_MMU040_SR_GLOBAL` from `0x0080` to `0x0200`
- Add `M68K_MMU040_SR_U0` (`0x0080`) and `M68K_MMU040_SR_U1` (`0x0100`)
- Add `M68K_MMU040_PD_U0` (`0x00000100`) and `M68K_MMU040_PD_U1` (`0x00000200`)
- Map U0/U1 from page descriptors to MMUSR in PTEST path and ATC lookup

### Phase 2: Co-Sim Callbacks in 040 Paths
#### [MODIFY] [m68kmmu.h](file:///Volumes/TB4-4Tb/Projects/mister/musashi/m68kmmu.h)
- Fire `m68ki_mmu_cosim_translate()` on 040 ATC hit path
- Fire `m68ki_mmu_cosim_translate()` on 040 table walk path
- Fire `m68ki_mmu_cosim_atc(FLUSH)` in PFLUSH (An) and PFLUSHN (An)

### Phase 3: 040 Bus Error Frame (Format $7)
#### [MODIFY] [m68kcpu.c](file:///Volumes/TB4-4Tb/Projects/mister/musashi/m68kcpu.c)
- Add Format $7 stack frame builder for 040 access faults
- Gate by `CPU_TYPE_IS_040()` in `m68ki_exception_bus_error()`
- Populate: SSW, effective address, fault address (zero remaining fields)

### Phase 4: Test Fixes
#### [MODIFY] test/mc68040/*.s
- Fix `ptest_global.s` to check bit 9 (not bit 7) for G flag
- Fix `pflushn.s` to check bit 9 for G flag
- Add test for U0/U1 propagation if feasible

### Phase 5: Documentation
#### [MODIFY] [doc/68040/mmu_implementation_plan.md](file:///Volumes/TB4-4Tb/Projects/mister/musashi/doc/68040/mmu_implementation_plan.md)
- Update current state to reflect all fixes
- Mark completed items

## Verification Plan

### Automated Tests
- `make clean && make -j4` — zero new warnings/errors
- `make -j4 -DM68K_MMU_COSIM=1` — verify co-sim build
- Rebuild all 040 test binaries
- Run test suite: `make test` from root (all CPU families)

### Manual Verification
- Spot-check MMUSR values in PTEST results match 68040 UM bit layout

## Open Questions

> [!NOTE]
> The Format $7 bus error frame is the most complex item. It has 30 words including internal pipeline state. Should we implement a full 30-word frame, or a simplified version that zeros internal state fields and only populates SSW + fault address? The simplified version is sufficient for OS-level exception handling and MiSTer co-sim.
