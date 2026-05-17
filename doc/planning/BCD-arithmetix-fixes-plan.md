# BCD Arithmetic Fixes — Complete Reference

## Goal

Fix `ABCD`, `SBCD`, and `NBCD` undocumented V/N flag behavior to match real 68000 silicon, verified via SST (cputester) harness.

## Background

Motorola documents N and V flags as **"undefined"** for BCD instructions, but silicon is deterministic. The SST oracle captures exact CCR from real hardware, so Musashi must replicate the silicon's "undefined" behavior exactly.

---

## Reference Implementations

### 1. Original Musashi (HEAD, before any modifications)

#### ABCD (register-to-register)
```c
M68KMAKE_OP(abcd, 8, rr, .)
{
    uint* r_dst = &DX;
    uint src = DY;
    uint dst = *r_dst;
    uint res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1();

    FLAG_V = ~res; /* Undefined V behavior */

    if(res > 9)
        res += 6;
    res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
    FLAG_X = FLAG_C = (res > 0x99) << 8;
    if(FLAG_C)
        res -= 0xa0;

    FLAG_V &= res; /* Undefined V behavior part II */
    FLAG_N = NFLAG_8(res); /* Undefined N behavior */

    res = MASK_OUT_ABOVE_8(res);
    FLAG_Z |= res;

    *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}
```

#### SBCD (register-to-register)
```c
M68KMAKE_OP(sbcd, 8, rr, .)
{
    uint* r_dst = &DX;
    uint src = DY;
    uint dst = *r_dst;
    uint res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1();

    FLAG_V = ~res; /* Undefined V behavior */

    if(res > 9)
        res -= 6;
    res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
    FLAG_X = FLAG_C = (res > 0x99) << 8;
    if(FLAG_C)
        res += 0xa0;

    FLAG_V &= res; /* Undefined V behavior part II */
    FLAG_N = NFLAG_8(res); /* Undefined N behavior */

    res = MASK_OUT_ABOVE_8(res);
    FLAG_Z |= res;

    *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}
```

#### NBCD (data register)
```c
M68KMAKE_OP(nbcd, 8, ., d)
{
    uint* r_dst = &DY;
    uint dst = *r_dst;
    uint res = MASK_OUT_ABOVE_8(0x9a - dst - XFLAG_AS_1());

    if(res != 0x9a)
    {
        FLAG_V = ~res; /* Undefined V behavior */

        if((res & 0x0f) == 0xa)
            res = (res & 0xf0) + 0x10;

        res = MASK_OUT_ABOVE_8(res);

        FLAG_V &= res; /* Undefined V behavior part II */

        *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

        FLAG_Z |= res;
        FLAG_C = CFLAG_SET;
        FLAG_X = XFLAG_SET;
    }
    else
    {
        FLAG_V = VFLAG_CLEAR;
        FLAG_C = CFLAG_CLEAR;
        FLAG_X = XFLAG_CLEAR;
    }
    FLAG_N = NFLAG_8(res);
}
```

---

### 2. MAME (m68kops.cpp, current master)

#### ABCD
```cpp
void m68000_musashi_device::xc100_abcd_b_071234fc()
{
    u32* r_dst = &DX();
    u32 src = DY();
    u32 dst = *r_dst;
    u32 res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_1();
    u32 corf = 0;

    if(res > 9)
        corf = 6;
    res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
    m_v_flag = ~res; /* Undefined V behavior */
    res += corf;
    m_x_flag = m_c_flag = (res > 0x9f) << 8;
    if(m_c_flag)
        res -= 0xa0;

    m_v_flag &= res; /* Undefined V behavior part II */
    m_n_flag = NFLAG_8(res); /* Undefined N behavior */

    res = MASK_OUT_ABOVE_8(res);
    m_not_z_flag |= res;

    *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}
```

#### SBCD
```cpp
void m68000_musashi_device::x8100_sbcd_b_071234fc()
{
    u32* r_dst = &DX();
    u32 src = DY();
    u32 dst = *r_dst;
    u32 res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_1();
    u32 corf = 0;

    if(res > 0xf)
        corf = 6;
    res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
    m_v_flag = res; /* Undefined V behavior */
    if(res > 0xff) {
        res += 0xa0;
        m_x_flag = m_c_flag = CFLAG_SET;
    } else if(res < corf)
        m_x_flag = m_c_flag = CFLAG_SET;
    else
        m_n_flag = m_x_flag = m_c_flag = 0;

    res = MASK_OUT_ABOVE_8(res - corf);

    m_v_flag &= ~res; /* Undefined V behavior part II */
    m_n_flag = NFLAG_8(res); /* Undefined N behavior */
    m_not_z_flag |= res;

    *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}
```

#### NBCD
```cpp
void m68000_musashi_device::x4800_nbcd_b_071234fc()
{
    u32* r_dst = &DY();
    u32 dst = MASK_OUT_ABOVE_8(*r_dst);
    u32 res = -dst - XFLAG_1();

    if(res != 0) {
        m_v_flag = res; /* Undefined V behavior */

        if(((res|dst) & 0x0f) == 0)
            res = (res & 0xf0) | 6;

        res = MASK_OUT_ABOVE_8(res + 0x9a);

        m_v_flag &= ~res; /* Undefined V behavior part II */

        *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

        m_not_z_flag |= res;
        m_c_flag = CFLAG_SET;
        m_x_flag = XFLAG_SET;
    } else {
        m_v_flag = VFLAG_CLEAR;
        m_c_flag = CFLAG_CLEAR;
        m_x_flag = XFLAG_CLEAR;
    }
    m_n_flag = NFLAG_8(res);
}
```

---

### 3. WinUAE (gencpu.cpp — code generator, ground truth)

WinUAE generates C code per CPU level. The snippets below are the **generated C** for cpu_level < 2 (68000/68010).

#### ABCD
```c
// Generated from gencpu.cpp case i_ABCD:
uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG() ? 1 : 0);
uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
uae_u16 newv, tmp_newv;
int cflg;
newv = tmp_newv = newv_hi + newv_lo;
if (newv_lo > 9) { newv += 6; }
cflg = (newv & 0x3F0) > 0x90;
if (cflg) newv += 0x60;
SET_CFLG(cflg);
COPY_CARRY();
// N flag: set from final newv
// V flag:
SET_VFLG((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
```

#### SBCD
```c
// Generated from gencpu.cpp case i_SBCD:
uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG() ? 1 : 0);
uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
uae_u16 newv, tmp_newv;
int bcd = 0;
newv = tmp_newv = newv_hi + newv_lo;
if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG() ? 1 : 0)) & 0x100) > 0xFF)
    { newv -= 0x60; }
SET_CFLG((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG() ? 1 : 0)) & 0x300) > 0xFF);
COPY_CARRY();
// N flag: set from final newv
// V flag:
SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
```

#### NBCD
```c
// Generated from gencpu.cpp case i_NBCD:
uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
uae_u16 newv_hi = - (src & 0xF0);
uae_u16 newv;
int cflg, tmp_newv;
tmp_newv = newv_hi + newv_lo;
if (newv_lo > 9) newv_lo -= 6;
newv = newv_hi + newv_lo;
cflg = (newv & 0x1F0) > 0x90;
if (cflg) newv -= 0x60;
SET_CFLG(cflg);
COPY_CARRY();
// N flag: set from final newv
// V flag:
SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
```

#### WinUAE CPU-Level Dependent Flag Behavior
```c
#define xBCD_KEEPS_V_FLAG 2   // 68020+: V flag unchanged (or cleared)
#define xBCD_KEEPS_N_FLAG 4   // 68040+: N flag unchanged

// For cpu_level >= xBCD_KEEPS_V_FLAG && < xBCD_KEEPS_N_FLAG:
//   V is cleared to 0, N is still computed from result
// For cpu_level >= xBCD_KEEPS_N_FLAG:
//   Both V and N are unchanged (only Z is updated)
```

| CPU Level | V Flag | N Flag |
|-----------|--------|--------|
| 0-1 (68000/010) | Computed from MSB transition | Computed from result MSB |
| 2-3 (68020/030) | Cleared to 0 | Computed from result MSB |
| 4+ (68040/060) | Unchanged | Unchanged |

---

## Analysis

### Result/Carry Algorithm Comparison

#### ABCD: Immediate vs Deferred Low-Nibble Correction

**Original Musashi** applies `+6` correction BEFORE adding high nibbles:
```
res = lo_sum;  if(>9) res += 6;  res += hi_sum;  carry = (res > 0x99)
```

**MAME** defers `+6` as `corf`, adds it AFTER high nibbles:
```
res = lo_sum;  if(>9) corf=6;  res += hi_sum;  res += corf;  carry = (res > 0x9f)
```

**These are NOT equivalent.** Verified by exhaustive test:
```
DIFF: src=0x04 dst=0x8f x=1: orig=0xFA/C=1  mame=0x9A/C=0
DIFF: src=0x05 dst=0x8e x=1: orig=0xFA/C=1  mame=0x9A/C=0
... (hundreds of differences with invalid BCD inputs)
```

The deferred `corf` loses inter-nibble carry propagation. When `+6` pushes the low nibble past 0x10, the carry into the high nibble position is lost in the deferred approach.

**WinUAE** applies `+6` IMMEDIATELY (like original Musashi):
```
newv = newv_hi + newv_lo;  if(newv_lo > 9) newv += 6;
```

**Conclusion:** Original Musashi and WinUAE agree on result/carry. MAME diverges on invalid BCD inputs. Our binary tests validate against original Musashi's algorithm — we must keep it.

#### SBCD: Same Pattern

Original Musashi applies `-6` before high nibbles. MAME defers. WinUAE applies immediately. Same divergence on edge cases.

#### NBCD: All Three Differ

Each uses a fundamentally different approach:
- **Musashi**: `0x9a - dst - X`, then adjust low nibble
- **MAME**: `-dst - X`, then `+0x9a` adjustment
- **WinUAE**: Separate `-(lo)` and `-(hi)` with independent corrections

All three produce identical results for valid BCD inputs. WinUAE's is the most transparent.

### V Flag Comparison

This is the **only** difference that matters for SST failures.

| Emulator | ABCD V Flag | SBCD/NBCD V Flag |
|----------|-------------|-------------------|
| Musashi (original) | `(~low_nibble_sum) & final_result` | `(~low_nibble_diff) & final_result` |
| MAME | `(~pre_corf_sum) & final_result` | `pre_corf_diff & (~final_result)` |
| **WinUAE (silicon)** | `!(unadjusted & 0x80) && (adjusted & 0x80)` | `(unadjusted & 0x80) && !(adjusted & 0x80)` |

The Musashi/MAME `~res & res` approximation fails because:
1. It uses the wrong comparison point (low nibble only, not full unadjusted sum)
2. It's a bitwise AND, not a boolean MSB-transition check

**WinUAE's formula is a clean MSB transition detector:**
- ABCD: V=1 when MSB goes 0→1 during BCD correction (positive overflow)
- SBCD/NBCD: V=1 when MSB goes 1→0 during BCD correction (negative overflow)

Where:
- `unadjusted` = `newv_hi + newv_lo` (raw nibble sum/diff, NO corrections applied)
- `adjusted` = final result after ALL BCD corrections (both low and high nibble)

---

## Fix Strategy

### Principle: Minimal Surgery

The original Musashi result/carry/N/Z logic is **correct** and validated by binary tests. Only the V flag needs replacement.

### ABCD Fix

Replace the two-part `FLAG_V` approximation with WinUAE's MSB transition check. We need to capture the unadjusted full sum:

```c
M68KMAKE_OP(abcd, 8, rr, .)
{
    uint* r_dst = &DX;
    uint src = DY;
    uint dst = *r_dst;
    uint res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1();

    if(res > 9)
        res += 6;
    res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);

    /* WinUAE: unadjusted = hi+lo before any correction */
    uint unadjusted = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1()
                    + HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);

    FLAG_X = FLAG_C = (res > 0x99) << 8;
    if(FLAG_C)
        res -= 0xa0;

    res = MASK_OUT_ABOVE_8(res);

    FLAG_V = (!(unadjusted & 0x80) && (res & 0x80)) ? VFLAG_SET : VFLAG_CLEAR;
    FLAG_N = NFLAG_8(res);
    FLAG_Z |= res;

    *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}
```

### SBCD Fix

Same principle — capture unadjusted diff, replace V flag:

```c
M68KMAKE_OP(sbcd, 8, rr, .)
{
    uint* r_dst = &DX;
    uint src = DY;
    uint dst = *r_dst;
    uint res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1();

    if(res > 9)
        res -= 6;
    res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);

    uint unadjusted = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1()
                     + HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);

    FLAG_X = FLAG_C = (res > 0x99) << 8;
    if(FLAG_C)
        res += 0xa0;

    res = MASK_OUT_ABOVE_8(res);

    FLAG_V = ((unadjusted & 0x80) && !(res & 0x80)) ? VFLAG_SET : VFLAG_CLEAR;
    FLAG_N = NFLAG_8(res);
    FLAG_Z |= res;

    *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}
```

### NBCD Fix

NBCD is `0 - dst - X` (BCD negation). The unadjusted value must match WinUAE's
`tmp_newv = newv_hi + newv_lo` where `newv_hi = -(dst & 0xF0)` and
`newv_lo = -(dst & 0x0F) - X`.

Note: WinUAE computes `tmp_newv` and### Step 3: Implementation of Silicon-Accurate Logic

Based on the analysis of the `68k-bcd-verifier` by Flamewing, the bit-exact logic for 68000 BCD arithmetic is:

#### ABCD (Addition)
```c
uint8_t ss = src + dst + x;
uint8_t bc = ((src & dst) | (~ss & src) | (~ss & dst)) & 0x88;
uint8_t dc = (((ss + 0x66) ^ ss) & 0x110) >> 1;
uint8_t corf = (bc | dc) - ((bc | dc) >> 2);
uint8_t res = ss + corf;

V = (~ss & res) >> 7;
C = X = (bc | (ss & ~res)) >> 7;
Z = Z & (res == 0);
N = res >> 7;
```

#### SBCD (Subtraction)
```c
uint8_t dd = dst - src - x;
uint8_t bc = ((~dst & src) | (dd & src) | (dd & ~dst)) & 0x88; // Note: slight variation in bc for sub
uint8_t corf = bc - (bc >> 2);
uint8_t res = dd - corf;

V = (dd & ~res) >> 7;
C = X = (bc | (~dd & res)) >> 7;
Z = Z & (res == 0);
N = res >> 7;
```

### Reference: BlastEm BCD Implementation (Pavle Pavlovic)
BlastEm uses a unified JIT-based handler for `ABCD`, `SBCD`, and `NBCD`. The logic involves calculating the low nibble carry first, then the full byte, applying correction factors (`0x06` and `0x60`) based on the resulting carries.

#### BlastEm JIT Logic (Pseudocode from `m68k_core_x86.c`):
```c
// translate_m68k_abcd_sbcd
// 1. Calculate low nibble result: 
//    res_low = dst.low +/- src.low +/- X
// 2. Determine low adjustment:
//    if res_low > 9 (or borrow): adj_low = 0x06, else 0x00
// 3. Calculate full byte result with adjustment:
//    res = dst +/- src +/- X +/- adj_low
// 4. Determine high adjustment:
//    if res > 0x99 (or borrow): adj_high = 0x60, C=1, X=1
// 5. Final result: res = res +/- adj_high
```
BlastEm handles `NBCD` by setting `src = 0` and performing a `SBCD`-like subtraction.
    *r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

        FLAG_Z |= res;
        FLAG_C = CFLAG_SET;
        FLAG_X = XFLAG_SET;
    else
    {
        FLAG_C = CFLAG_CLEAR;
        FLAG_X = XFLAG_CLEAR;
    }
    FLAG_V = ((unadjusted & 0x80) && !(res & 0x80)) ? VFLAG_SET : VFLAG_CLEAR;
    FLAG_N = NFLAG_8(res);
}
```

> [!WARNING]
> The NBCD fix is the least certain. The `unadjusted` computation for NBCD is
> tricky because of unsigned/signed negation semantics. If SST still fails for
> NBCD after applying this, rewrite to WinUAE's exact algorithm structure
> (separate `newv_lo`/`newv_hi` negation with `uae_u16` types).

---

## Variant Manifest

All variants share the same core logic, differing only in operand fetch:

| Instruction | Variants | Operand Difference |
|-------------|----------|-------------------|
| ABCD | `rr .`, `mm ax7`, `mm ay7`, `mm axy7`, `mm .` | DX/DY vs -(An) memory |
| SBCD | `rr .`, `mm ax7`, `mm ay7`, `mm axy7`, `mm .` | DX/DY vs -(An) memory |
| NBCD | `. d`, `. .` | DY vs EA memory |

Total: 12 handlers to modify (5 + 5 + 2).

---

## Verification Plan

### Step 1: Build

```bash
cd /Volumes/TB4-4Tb/Projects/mister/musashi
make clean && make test_driver
```

Expected: compiles with no errors (warnings about unused `cycle_cost` are normal).

### Step 2: Binary Tests (regression gate)

```bash
make test 2>&1 | grep "fail_count" | sort | uniq -c
```

**MUST pass**: All 136 binary tests must show `test_fail_count = 0`.
This validates that result/carry/N/Z logic is unchanged. If any binary test
fails (especially `abcd.bin`, `sbcd.bin`, `nbcd.bin`), the result algorithm
has been broken — do NOT proceed.

### Step 3: SST Tests (V/N flag validation)

```bash
./test/singlestep/sst_runner ABCD SBCD NBCD --source=tomharte --verbose --summary
```

The runner resolves bare mnemonic names (e.g. `ABCD`, `ABCD.sst`) by searching
`data_dir/{tomharte,raddad}/` subdirectories. Use `--source=` to restrict to one
oracle. Full `.sst` paths also work for backward compatibility.

Per-file results are printed as:
```
[tomharte] ABCD                  NNNN/ 8065 FAIL  (X.XXXs)
[tomharte] SBCD                  NNNN/ 8065 FAIL  (X.XXXs)
[tomharte] NBCD                  NNNN/ 8065 FAIL  (X.XXXs)
```

Each file has **8065 vectors**. Total across all three: **24195 vectors**.

#### What success looks like

```
[tomharte] ABCD                  8065/ 8065 PASS  (X.XXXs)
[tomharte] SBCD                  8065/ 8065 PASS  (X.XXXs)
[tomharte] NBCD                  8065/ 8065 PASS  (X.XXXs)

Files:   3/3 passed
Vectors: 24195/24195 passed (0 FAILED)
```

### Step 2.5: GoogleTests (Unit Verification)

While SST runs millions of vectors against an oracle, GoogleTests are used for 
targeted mathematical verification of specific silicon formulas.

The GTest suite is organized as follows:
- `test/gtest/core/`: CPU state and lifecycle tests.
- `test/gtest/instructions/`: Opcode-specific logic (e.g., `bcd/`).
- `test/gtest/framework/`: Fixtures and memory mocks.

#### Building with CMake

```bash
cmake -B test/gtest/build -S .
cmake --build test/gtest/build
./test/gtest/build/test/gtest/musashi_unittests
```

#### Building with Makefile

```bash
cd test/gtest
make
./musashi_unittests
```
*(Note: Makefile assumes `libgtest` and `libgtest_main` are installed on the system).*


#### What partial success looks like

If only the V flag fix is correct but something else is off:
- Most vectors pass, but some fail on specific flag bits or result values
- Grep for patterns: `grep "FAIL" | head -20` and check if failures are
  `SR expected=... got=...` (flag mismatch) or `D0 expected=... got=...` (result mismatch)

#### Failure diagnosis

| Failure Pattern | Meaning |
|----------------|---------|
| `SR expected=0x...13 got=0x...11` | V flag wrong (bit 1 of SR = V flag) |
| `SR expected=0x...11 got=0x...13` | V flag wrong (set when should be clear) |
| `SR expected=0x...19 got=0x...11` | N and V both wrong |
| `Dx expected=0x...8D got=0x...ED` | Result off by 0x60 — carry/correction logic broken |
| `RAM@... expected=0x.. got=0x..` | Memory result wrong — same as Dx but for -(An) mode |

SR flag bit mapping (low byte):
```
bit 0 = C (carry)
bit 1 = V (overflow)  ← this is what we're fixing
bit 2 = Z (zero)
bit 3 = N (negative)
bit 4 = X (extend)
```

So `SR 0x2713 vs 0x2711` means expected V=1 (bit 1 set), got V=0 (bit 1 clear).
And `SR 0x2711 vs 0x2713` means expected V=0, got V=1.

#### Baseline — Unmodified Musashi (original code, no fixes applied)

##### TomHarte source (8065 vectors per file)

```
[tomharte] ABCD                  7293/ 8065 FAIL  (1.5s)    — 772 failures
[tomharte] SBCD                  6293/ 8065 FAIL  (1.5s)    — 1772 failures
[tomharte] NBCD                  4655/ 8065 FAIL  (1.5s)    — 3410 failures

Files:   0/3 passed
Vectors: 18241/24195 passed (5954 FAILED)  — 75.4% pass rate
```

Failure breakdown by type:

| Instruction | SR (flag) failures | Result (Dx/RAM) failures | Total |
|-------------|-------------------|--------------------------|-------|
| **ABCD** | 0 | 773 | 773 |
| **SBCD** | 19 | 1754 | 1773 |
| **NBCD** | 3291 | 119 | 3410 |

**Key observations:**
- **ABCD**: 100% of failures are result mismatches (Dx off by exactly 0x60). Zero SR/flag failures. The V flag approximation happens to produce correct results by coincidence — the real problem is the carry detection.
- **SBCD**: Dominated by result mismatches (99%). Only 19 flag-only failures.
- **NBCD**: Dominated by SR flag failures (96%). Almost all are `SR 0x2713 vs 0x2711` — V flag (bit 1) expected SET, got CLEAR. A few result mismatches (Dx/RAM).

##### Raddad source (2500 vectors per file)

```
[raddad  ] ABCD                  2254/ 2500 FAIL  (0.5s)    — 246 failures
[raddad  ] SBCD                  1944/ 2500 FAIL  (0.5s)    — 556 failures
[raddad  ] NBCD                  1463/ 2500 FAIL  (0.5s)    — 1037 failures

Files:   0/3 passed
Vectors: 5661/7500 passed (1839 FAILED)  — 75.5% pass rate
```

Same ~75% pass rate across both sources — failure patterns are consistent.

##### Interpretation

The baseline tells us **two distinct problems** exist:

1. **Result/carry problem (ABCD/SBCD)**: The original Musashi carry detection (`res > 0x99`) diverges from real silicon for certain inputs. This causes result values to be off by exactly 0x60 (the high-nibble BCD correction). This means the WinUAE algorithm (which uses different carry logic) must be adopted — not just the V flag formula.

2. **V flag problem (NBCD)**: The `~res & res` approximation fails to detect MSB transitions correctly. The WinUAE `(tmp_newv & 0x80) != 0 && (newv & 0x80) == 0` formula is needed.

> [!IMPORTANT]
> A V-flag-only fix is **not sufficient**. The result/carry algorithm must also be
> replaced with WinUAE's approach for ABCD and SBCD. For NBCD, the V flag fix is
> the primary concern (96% of failures are flag-only), but the ~4% result mismatches
> also need the WinUAE algorithm.

### Step 4: Full SST Suite (regression check)

Only run this after Step 3 passes:

```bash
./test/singlestep/sst_runner.sh --all --summary
```

This runs ALL instruction SST vectors. Failures in non-BCD instructions indicate
a regression. The BCD fix must not affect any other instruction.

### Step 5: Raddad source (cross-validation)

```bash
./test/singlestep/sst_runner ABCD SBCD NBCD --source=raddad --verbose --summary
```

Must also reach 100% pass rate — both oracle sources must agree.

