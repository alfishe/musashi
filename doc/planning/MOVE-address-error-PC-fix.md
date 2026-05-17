# MOVE Address Error Stack Frame PC Fix

## Problem Summary

The PC value in the 68000 address error exception stack frame is incorrect for MOVE instruction destination write faults. Tom Harte SingleStepTests show 732 failures for MOVE.l and MOVE.w due to wrong PC values.

### Current State
- **RTS/BSR**: Fixed - PC address errors use `target - 4` (8065/8065 pass)
- **MOVE.l**: 7688/8065 pass (377 fail due to PC)
- **MOVE.w**: 7710/8065 pass (355 fail due to PC)

### Root Cause

When an address error fires during a MOVE destination write, the exception frame PC should be captured **after source EA fetch, before destination EA fetch**. Currently, `m68ki_check_address_error()` uses `REG_PC - 2` unconditionally, which is wrong for MOVE destinations:

| Destination Mode | Current Output | Expected | Delta |
|------------------|----------------|----------|-------|
| `-(An)` | `REG_PC - 2` | `REG_PC` | +2 needed |
| `(xxx).w` | `REG_PC - 2` | `REG_PC - 4` | -2 needed |
| `(xxx).l` | `REG_PC - 2` | `REG_PC - 6` | -4 needed |
| `(d16,An)` | `REG_PC - 2` | `REG_PC - 4` | -2 needed |

The pattern: **Frame PC = PC after source operand fetch, before destination extension word consumption.**

## Failure Analysis

From SST test output:
```
FAIL 2117 [MOVE.l (A7), -(A0)] 39: RAM@0x0007FF expected=0x02 got=0x00
FAIL 2531 [MOVE.l (d8, A1, Xn), -(A2)] 96: RAM@0x0007FF expected=0x04 got=0x02
FAIL 23e0 [MOVE.l -(A0), (xxx).l] 495: RAM@0x0007FF expected=0x02 got=0x04
FAIL 2d3c [MOVE.l #, -(A6)] 283: RAM@0x0007FF expected=0x06 got=0x04
```

Breakdown by pattern:
- 249 cases: exp=0x02 got=0x00 (need +2) - simple source EA
- 87 cases: exp=0x04 got=0x02 (need +2) - source with 2-byte extension
- 15 cases: exp=0x06 got=0x04 (need +2) - source with 4-byte extension (immediate)
- 12 cases: exp=0x04 got=0x06 (need -2) - `(xxx).l` destination
- 12 cases: exp=0x02 got=0x04 (need -2) - `(xxx).l` destination with simple source

## Proposed Solution

### Approach: Flag-Based PC Preload

Add a mechanism for MOVE handlers to pre-set `m68ki_aerr_pc` before the destination write, preventing the address error check from overwriting it.

### Changes Required

#### 1. Add Preload Flag (m68kcpu.c, m68kcpu.h)

```c
// m68kcpu.c - add after m68ki_aerr_pc declaration
uint    m68ki_aerr_pc_preloaded;

// m68kcpu.h - add extern declaration
extern uint m68ki_aerr_pc_preloaded;
```

#### 2. Add Helper Macro (m68kcpu.h)

```c
/* Capture PC for MOVE destination address errors.
 * Called after source EA fetch, before destination EA processing.
 */
#define M68KI_MOVE_AERR_SAVE_PC() \
    do { m68ki_aerr_pc = REG_PC; m68ki_aerr_pc_preloaded = 1; } while(0)
```

#### 3. Modify Write Functions (m68kcpu.h)

In `m68ki_write_16_fc()`, `m68ki_write_32_fc()`, and `m68ki_write_32_pd_fc()`, add before the address error check:

```c
if (!m68ki_aerr_pc_preloaded)
    m68ki_aerr_pc = REG_PC - 2;
m68ki_aerr_pc_preloaded = 0;  // consume flag
```

#### 4. Guard Address Error Macro (m68kcpu.h)

Modify `m68ki_check_address_error()` to skip PC assignment for writes (since write functions now handle it):

```c
#define m68ki_check_address_error(ADDR, WRITE_MODE, FC) \
    if((ADDR)&1) \
    { \
        m68ki_aerr_address = ADDR; \
        m68ki_aerr_write_mode = WRITE_MODE; \
        m68ki_aerr_fc = FC; \
        if ((WRITE_MODE) != MODE_WRITE) m68ki_aerr_pc = REG_PC - 2; \
        siglongjmp(m68ki_aerr_trap, 1); \
    }
```

#### 5. Edit MOVE Templates (m68k_in.c)

Insert `M68KI_MOVE_AERR_SAVE_PC();` after source operand fetch in all MOVE.w and MOVE.l handlers with memory destinations.

**Affected templates** (42 total):
- Sizes: 16, 32 (not 8 - byte writes don't cause address errors)
- Destination modes: ai, pi, pd, di, ix, aw, al (not d - register destinations don't fault)
- Source variants: d, a, . (register and general)

Example edit for `M68KMAKE_OP(move, 16, pd, .)`:
```c
M68KMAKE_OP(move, 16, pd, .)
{
    uint res = M68KMAKE_GET_OPER_AY_16;
    M68KI_MOVE_AERR_SAVE_PC();  // <-- INSERT HERE

    FLAG_N = NFLAG_16(res);
    FLAG_Z = res;
    FLAG_V = VFLAG_CLEAR;
    FLAG_C = CFLAG_CLEAR;
    AX -= 2;
    m68ki_write_16(AX, res);
}
```

### Template Edit List

MOVE.w (16-bit) templates to modify:
- `move, 16, ai, d` / `move, 16, ai, a` / `move, 16, ai, .`
- `move, 16, pi, d` / `move, 16, pi, a` / `move, 16, pi, .`
- `move, 16, pd, d` / `move, 16, pd, a` / `move, 16, pd, .`
- `move, 16, di, d` / `move, 16, di, a` / `move, 16, di, .`
- `move, 16, ix, d` / `move, 16, ix, a` / `move, 16, ix, .`
- `move, 16, aw, d` / `move, 16, aw, a` / `move, 16, aw, .`
- `move, 16, al, d` / `move, 16, al, a` / `move, 16, al, .`

MOVE.l (32-bit) templates to modify:
- Same pattern as above (21 more templates)

**Total: 42 template edits**

## Why This Approach

1. **Minimal invasiveness**: No new function parameters, no call-site changes for non-MOVE code
2. **Works with m68kmake**: One-line insertion per template, regeneration handles expansion
3. **Self-clearing flag**: Consumed on every write, no leak to subsequent instructions
4. **Read faults untouched**: Macro still handles reads with `REG_PC - 2`
5. **Non-MOVE writes preserved**: Write functions fall back to `REG_PC - 2` when flag not set

## Edge Cases

### Source EA Read Faults
If source operand read faults (e.g., reading from `(AY)` with odd address), it fires BEFORE `M68KI_MOVE_AERR_SAVE_PC()` executes. The flag is still 0, so the read-path macro handles it with `REG_PC - 2` - correct behavior.

### Destination EA Extension Reads
`EA_AX_DI_16()`, `EA_AL_16()` etc. read from instruction stream (always even PC). These never fault on alignment. Flag is safe to set before these calls.

### MOVE.B
Byte writes don't trigger address errors on 68000. No changes needed for size-8 templates.

### MOVEM / MOVEP
Separate instructions with different handlers. If tests reveal issues, they need individual treatment.

## Testing

After implementation:
```bash
cd /Volumes/TB4-4Tb/Projects/mister/musashi && make clean && make -j4
cd test/singlestep/build && cmake --build . -j4
./sst_runner MOVE_l MOVE_w RTS BSR --source=tomharte --summary
```

Expected result: All 32260 vectors pass (currently 31528).

## Effort Estimate

- Implementation: 2-3 hours
- Testing: 1 hour
- Total: Half day

## Files to Modify

1. `m68kcpu.c` - Add `m68ki_aerr_pc_preloaded` variable
2. `m68kcpu.h` - Add extern, helper macro, modify write functions and address error macro
3. `m68k_in.c` - Insert `M68KI_MOVE_AERR_SAVE_PC()` in 42 MOVE templates

## References

- Tom Harte SingleStepTests: https://github.com/TomHarte/ProcessorTests
- 68000 Programmer's Reference Manual: Address Error exception frame format
- WinUAE source: Address error handling reference implementation
