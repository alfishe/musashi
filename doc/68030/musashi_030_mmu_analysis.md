# Musashi 68030 MMU (PMMU) Analysis

## Verdict: ✅ Confirmed — Your assessment is correct

PFLUSH, PTEST, PLOAD, PFLUSHR, PBcc, and PVALID are all **stub-only** — they print `"unhandled"` to stderr and return without doing anything. PMOVE is the only PMMU coprocessor instruction with a real implementation.

---

## Detailed Breakdown

### Source File: [m68kmmu.h](../../m68kmmu.h)

The entire Musashi PMMU lives in this single 322-line header. It contains exactly **two functions**:

| Function | Purpose | Status |
|---|---|---|
| `pmmu_translate_addr()` | 68851/68030 page table walk | ✅ **Implemented** |
| `m68881_mmu_ops()` | COP 0 MMU instruction dispatch | ⚠️ **Partial** (only PMOVE works) |

---

### Instruction-by-Instruction Status

#### ❌ PFLUSH — Not Implemented
```c
// m68kmmu.h:209-212
else if ((modes & 0xe200) == 0x2000)  // PFLUSH
{
    fprintf(stderr,"680x0: unhandled PFLUSH PC=%x\n", REG_PC);
    return;
}
```
The encoding is correctly **detected** (`001xxx0xxxxxxxxx`) but execution is a no-op with a stderr warning.

Additionally, the 68040-specific PFLUSH in [m68k_in.c:8622-8646](../../m68k_in.c#L8622-L8646) is also stubbed:
```c
fprintf(stderr,"68040: unhandled PFLUSH\n");
```

#### ❌ PFLUSHR — Not Implemented
```c
// m68kmmu.h:214-217
else if (modes == 0xa000)  // PFLUSHR
{
    fprintf(stderr,"680x0: unhandled PFLUSHR\n");
    return;
}
```

#### ❌ PTEST — Not Implemented
```c
// m68kmmu.h:229-232
else if ((modes & 0xe000) == 0x8000)  // PTEST
{
    fprintf(stderr,"680x0: unhandled PTEST\n");
    return;
}
```
Encoding match (`100xxxxxxxxxxxxx`) is correct, but there is zero implementation — no table walk is performed, no MMUSR bits are set, no level count is evaluated.

#### ❌ PLOAD — Not Implemented
```c
// m68kmmu.h:204-207
if ((modes & 0xfde0) == 0x2000)  // PLOAD
{
    fprintf(stderr,"680x0: unhandled PLOAD\n");
    return;
}
```

#### ❌ PBcc — Not Implemented
```c
// m68kmmu.h:187-196
if ((m68ki_cpu.ir & 0xffc0) == 0xf0c0)  // PBcc (word)
{
    fprintf(stderr,"680x0: unhandled PBcc\n");
    return;
}
else if ((m68ki_cpu.ir & 0xffc0) == 0xf080)  // PBcc (long)
{
    fprintf(stderr,"680x0: unhandled PBcc\n");
    return;
}
```

#### ❌ PVALID — Not Implemented
```c
// m68kmmu.h:219-227
// Both FORMAT 1 (0x2800) and FORMAT 2 (0x2c00) are stubbed
fprintf(stderr,"680x0: unhandled PVALID1\n");
fprintf(stderr,"680x0: unhandled PVALID2\n");
```

#### ✅ PMOVE — **Implemented**
PMOVE is the **only** fully functional PMMU instruction. It handles:

| PMOVE Direction | Registers Supported | Location |
|---|---|---|
| **To MMU** (write) | TC, SRP, CRP | [m68kmmu.h:262-293](../../m68kmmu.h#L262-L293) |
| **From MMU** (read) | TC, SRP, CRP | [m68kmmu.h:240-259](../../m68kmmu.h#L240-L259) |
| **MMUSR** (to/from) | MMU Status Register | [m68kmmu.h:297-306](../../m68kmmu.h#L297-L306) |

Both MC68030/040 form (case 0) and MC68881 form (case 2) are handled. The MC68030 MMUSR access (case 3) is also present.

Key PMOVE behaviors:
- Writing to TC with bit 31 set enables the PMMU (`pmmu_enabled = 1`)
- SRP and CRP are stored as 64-bit split (limit + address pointer)
- The FD (flush disable) bit in MC68030 form is parsed but not acted on

---

## Address Translation: `pmmu_translate_addr()`

The page table walker **does work** and is hooked into every memory access path:

```c
// m68kcpu.h — hooked into all read/write_fc functions
#if M68K_EMULATE_PMMU
if (PMMU_ENABLED)
    address = pmmu_translate_addr(address);
#endif
```

It implements the 68030 3-level (A→B→C) table walk with:
- ✅ IS (Initial Shift) field
- ✅ A/B/C bit-width fields from TC register
- ✅ 4-byte and 8-byte descriptor formats
- ✅ Early termination descriptors at any level
- ✅ SRP vs CRP selection based on supervisor mode and TC bit 25

### What the Translation **Lacks**

| Missing Feature | Impact |
|---|---|
| **No TLB/ATC** | Every memory access does a full table walk — no caching |
| **No Transparent Translation (TT0/TT1)** | No 030 TT registers exist in the CPU struct at all |
| **No bus error on invalid descriptors** | Uses `fatalerror()` (hard abort) instead of bus error exception |
| **No write-protect checking** | The W bit in descriptors is ignored |
| **No modified/used bit updates** | Descriptor M/U bits are never written back |
| **No function code matching** | FC bits in descriptors are ignored |
| **No MMUSR population** | Translation results don't set status bits |
| **No page size from TC** | The PS field is ignored (hardcoded masking) |

---

## CPU State: MMU Registers

From [m68kcpu.h:1026-1030](../../m68kcpu.h#L1026-L1030):

```c
/* PMMU registers (030-style) */
uint mmu_crp_aptr, mmu_crp_limit;   // CPU Root Pointer
uint mmu_srp_aptr, mmu_srp_limit;   // Supervisor Root Pointer
uint mmu_tc;                         // Translation Control
uint16 mmu_sr;                       // MMU Status Register
```

> [!WARNING]
> **Missing 030 registers**: There are no TT0/TT1 (Transparent Translation) registers in the CPU structure. These are critical for 68030 operation — AmigaOS and other real operating systems use them extensively to map I/O space.

The 040/060 MMU registers exist separately:
```c
uint mmu040_tc, mmu040_itt0, mmu040_itt1;
uint mmu040_dtt0, mmu040_dtt1;
uint mmu040_urp, mmu040_srp;
```

---

## Summary Table

| Instruction | Detected? | Implemented? | Notes |
|---|---|---|---|
| **PMOVE** | ✅ | ✅ | TC, SRP, CRP, MMUSR — both directions |
| **PFLUSH** | ✅ | ❌ | Stub only — `"unhandled PFLUSH"` |
| **PFLUSHR** | ✅ | ❌ | Stub only — `"unhandled PFLUSHR"` |
| **PTEST** | ✅ | ❌ | Stub only — `"unhandled PTEST"` |
| **PLOAD** | ✅ | ❌ | Stub only — `"unhandled PLOAD"` |
| **PBcc** | ✅ | ❌ | Stub only — `"unhandled PBcc"` |
| **PVALID** | ✅ | ❌ | Stub only — both formats |
| **Table Walk** | — | ✅ | Works but missing TLB, TT, protection, and status |

> [!IMPORTANT]
> The practical consequence: Musashi can **enable** the PMMU via PMOVE and perform basic address translation through page tables, but it cannot **flush** the (nonexistent) TLB, **test** translations without performing them, or **preload** ATC entries. For bare-metal OS emulation (AmigaOS, etc.), the missing PFLUSH is relatively benign (no TLB to flush), but missing PTEST would break any OS that probes page validity before access.
