# 68060 Implementation Plan

## Overview

This document outlines the phased implementation plan for adding Motorola 68060 support to Musashi. The approach follows Musashi's existing patterns for CPU variant support.

> [!IMPORTANT]
> **Post-Implementation Notes**:
> - `M68K_060_TRAP_UNIMPLEMENTED` was removed — real hardware always traps, no flag needed.
> - `FREM` is NOT hardware on the 060 (traps to FPSP). Corrected in implementation.
> - See `walkthrough.md` for the authoritative final state.

## Implementation Flow

```mermaid
flowchart LR
    subgraph Phase0[Phase 0: Recon]
        P0A[Review 68060 docs]
        P0B[Analyze MAME diff]
        P0C[Decision: approach]
    end
    
    subgraph Phase1[Phase 1: Infrastructure]
        P1A[CPU types]
        P1B[Config flags]
        P1C[New registers]
    end
    
    subgraph Phase2[Phase 2: Instructions]
        P2A[Table column]
        P2B[Parser update]
        P2C[Mark trapped ops]
    end
    
    subgraph Phase3[Phase 3: Exceptions]
        P3A[Frame $0]
        P3B[Frame $4]
        P3C[RTE update]
    end
    
    subgraph Phase4[Phase 4: FPU]
        P4A[Partition ops]
        P4B[Trap handler]
        P4C[Fallback mode]
    end
    
    subgraph Phase5[Phase 5: MMU]
        P5A[PLPA]
        P5B[060 PTEST]
        P5C[EC060 path]
    end
    
    subgraph Phase6[Phase 6: Test]
        P6A[Unit tests]
        P6B[Boot tests]
        P6C[Regression]
    end
    
    Phase0 --> Phase1 --> Phase2 --> Phase3 --> Phase4 --> Phase5 --> Phase6
```

## Phase 0: Reconnaissance (1-2 days)

### Objectives
- Gather authoritative documentation
- Assess existing work in MAME
- Make strategic decisions

### Tasks

#### P0.1: Documentation Gathering
- [ ] Obtain MC68060 User's Manual (MC68060UM/AD)
- [ ] Obtain M68060 Software Package documentation
- [ ] Review M68000 Programmer's Reference Manual 68060 sections

#### P0.2: MAME Analysis
- [ ] Diff MAME `src/devices/cpu/m68000/` against upstream Musashi
- [ ] Catalog 68060-specific changes in MAME
- [ ] Note any useful implementations (avoid direct copying due to GPL)

#### P0.3: Strategic Decisions
- [ ] **Decision 1**: Clean-room vs MAME port (recommend clean-room for MIT license)
- [ ] **Decision 2**: Trap-only vs native fallback (recommend both, configurable)
- [ ] **Decision 3**: Cycle timing fidelity (recommend approximate, document limits)

### Deliverables
- Decision document with rationale
- Reference documentation inventory

---

## Phase 1: Infrastructure Plumbing (2-3 days)

### Objectives
- Add CPU type definitions
- Add configuration flags
- Add new control registers
- Ensure clean compilation

### Tasks

#### P1.1: CPU Type Definitions

**File: `m68kcpu.h`**
```c
// Add after CPU_TYPE_SCC070
#define CPU_TYPE_EC060  (0x00000800)
#define CPU_TYPE_LC060  (0x00001000)
#define CPU_TYPE_060    (0x00002000)
```

**File: `m68k.h`**
```c
// Add to m68k_cpu_type enum
M68K_CPU_TYPE_68EC060,
M68K_CPU_TYPE_68LC060,
M68K_CPU_TYPE_68060
```

#### P1.2: Configuration Flags

**File: `m68kconf.h`**
```c
#ifndef M68K_EMULATE_060
#define M68K_EMULATE_060            M68K_OPT_ON
#endif

#ifndef M68K_060_TRAP_UNIMPLEMENTED
#define M68K_060_TRAP_UNIMPLEMENTED M68K_OPT_ON
#endif
```

#### P1.3: Control Registers

**File: `m68kcpu.h` (CPU state structure)**
```c
uint32 pcr;    // Processor Configuration Register
uint32 buscr;  // Bus Control Register
```

**File: `m68kcpu.c` (MOVEC handling)**
- Add PCR ($808) to MOVEC register table
- Add BUSCR ($008) to MOVEC register table
- Update CACR handling for 68060 layout

#### P1.4: CPU Type Initialization

**File: `m68kcpu.c` - `m68k_set_cpu_type()`**
```c
case M68K_CPU_TYPE_68060:
    CPU_TYPE         = CPU_TYPE_060;
    CPU_ADDRESS_MASK = 0xffffffff;
    CPU_SR_MASK      = 0xf71f;
    // ... additional initialization
```

### Deliverables
- Compiling codebase with new CPU types
- `m68k_set_cpu_type(M68K_CPU_TYPE_68060)` callable

### Verification
```c
m68k_set_cpu_type(M68K_CPU_TYPE_68060);
assert(m68k_get_reg(NULL, M68K_REG_CPU_TYPE) == M68K_CPU_TYPE_68060);
```

---

## Phase 2: Instruction Coverage Matrix (3-5 days)

### Objectives
- Extend instruction table for 68060
- Update code generator
- Mark trapped instructions

### Tasks

#### P2.1: Table Structure Extension

**File: `m68k_in.c`**

Change table header from:
```
name    size  proc   ea   bit pattern       A+-DXWLdxI  0 1 2 3 4  000 010 020 030 040
```

To:
```
name    size  proc   ea   bit pattern       A+-DXWLdxI  0 1 2 3 4 5  000 010 020 030 040 060
```

#### P2.2: Code Generator Update

**File: `m68kmake.c`**

```c
#define NUM_CPU_TYPES 6  // Was 5

// Update cycle parsing to read 6 columns
// Update table generation for 6 CPU types
```

**File: `m68k_in.c` (table footer)**
```c
unsigned char m68ki_cycles[NUM_CPU_TYPES][0x10000];
```

#### P2.3: Mark Trapped Instructions

For each instruction that traps on 68060, update its table entry:

| Instruction | Current 040 | 68060 Value | Meaning |
|-------------|-------------|-------------|---------|
| movep | 12 | T | Trap to vector 61 |
| cas2 | 12 | T | Trap to vector 61 |
| chk2cmp2 | 18 | T | Trap to vector 61 |
| mull (64-bit) | 43 | T | Trap to vector 61 |
| divl (64-bit) | 84 | T | Trap to vector 61 |

Convention: Use "T" or special marker for trapped ops, "." for not present.

#### P2.4: Implement Vector 61 Handler

**File: `m68kcpu.h`**
```c
#define EXCEPTION_UNIMPLEMENTED_INTEGER 61
```

**File: `m68kcpu.c`**
```c
void m68ki_exception_unimplemented_integer(void)
{
    // Push 060-style frame format $0
    // Jump to vector 61
}
```

### Deliverables
- Extended instruction table
- Updated code generator
- Trapped instructions raise vector 61

---

## Phase 3: Exception Frames & RTE (2-3 days)

### Objectives
- Implement 68060 exception frame formats
- Update RTE instruction handler

### Tasks

#### P3.1: Frame Format $0 (4-word)

**File: `m68kcpu.c`**

```c
void m68ki_push_frame_0(uint vector)
{
    // 68060 normal exception frame
    m68ki_push_16(vector | 0x0000);  // Format/Vector
    m68ki_push_32(REG_PC);           // PC
    m68ki_push_16(m68ki_get_sr());   // SR
}
```

#### P3.2: Frame Format $4 (8-word Access Fault)

```c
void m68ki_push_frame_4(uint vector, uint ea, uint fault_status)
{
    // 68060 access fault frame
    m68ki_push_32(fault_status);     // Fault Status
    m68ki_push_32(ea);               // Effective Address
    m68ki_push_16(vector | 0x4000);  // Format/Vector ($4xxx)
    m68ki_push_32(REG_PC);           // PC
    m68ki_push_16(m68ki_get_sr());   // SR
}
```

#### P3.3: Update RTE Handler

**File: `m68k_in.c` - `M68KMAKE_OP(rte, 32, ., .)`**

Add 68060 frame format decoding:
```c
case 0:  // Format $0 - 68060 4-word normal
    if (CPU_TYPE_IS_060_PLUS()) {
        new_pc = m68ki_pull_32();
        m68ki_set_sr(m68ki_pull_16());
        // No additional data to pull
    }
    break;
    
case 4:  // Format $4 - 68060 8-word access fault
    if (CPU_TYPE_IS_060_PLUS()) {
        new_pc = m68ki_pull_32();
        m68ki_set_sr(m68ki_pull_16());
        m68ki_pull_32();  // Effective Address (discard)
        m68ki_pull_32();  // Fault Status (discard)
    }
    break;
```

### Deliverables
- Correct frame pushing for 68060 exceptions
- RTE properly decodes 68060 frames

---

## Phase 4: FPU Split (3-4 days)

### Objectives
- Partition FPU operations into hardware vs trapped
- Implement trap mechanism for unimplemented ops
- Add optional native fallback

### Tasks

#### P4.1: Define Hardware Operations Set

**File: `m68kfpu.c`**

```c
// Operations remaining in 68060 hardware
#define FPU_060_HW_OPS ( \
    (1 << 0x00) |  /* FMOVE    */ \
    (1 << 0x01) |  /* FINT     */ \
    (1 << 0x02) |  /* FSINH - actually trapped */ \
    ...
)

static int fpu_op_is_060_hardware(int opcode) {
    // Return true if this op executes in 060 hardware
}
```

#### P4.2: Add FPU Trap Path

**File: `m68kfpu.c`**

```c
static void fpgen_rm_reg(uint16 w2)
{
    // ... existing decode ...
    
    if (CPU_TYPE_IS_060_PLUS() && !fpu_op_is_060_hardware(secondary)) {
#if M68K_060_TRAP_UNIMPLEMENTED
        m68ki_exception_1111();  // F-line trap
        return;
#endif
    }
    
    // ... existing implementation ...
}
```

#### P4.3: Packed Decimal Handling

Packed decimal FMOVE operations trap on 68060:

```c
if (CPU_TYPE_IS_060_PLUS() && (source_format == 3)) {
    // Packed decimal - trap
    m68ki_exception_1111();
    return;
}
```

#### P4.4: Configuration Flag

**File: `m68kconf.h`**

```c
// When ON: unimplemented FPU ops trap to vector 11
// When OFF: execute natively (for systems without FPSP)
#define M68K_060_TRAP_UNIMPLEMENTED M68K_OPT_ON
```

### Deliverables
- FPU operations correctly partitioned
- Unimplemented ops trap to vector 11
- Optional native fallback mode

---

## Phase 5: MMU & New Instructions (2-3 days)

### Objectives
- Implement PLPA instruction
- Implement LPSTOP instruction
- Adjust MMU behavior for 68060
- Handle EC060 variant

### Tasks

#### P5.1: PLPA Instruction

**File: `m68k_in.c`**

```c
M68KMAKE_OP(plpa, 32, ., .)
{
    if (CPU_TYPE_IS_060_PLUS()) {
        if (!(FLAG_S)) {
            m68ki_exception_privilege_violation();
            return;
        }
        // Perform address translation probe
        uint ea = AY;
        uint pa = pmmu_translate_addr_060(ea);
        // Result handling per 68060 PLPA semantics
    }
}
```

**Table entry:**
```
plpa      32  .     .     1111010100...000  ..........  . . . . . S   .   .   .   .   . 4
```

#### P5.2: LPSTOP Instruction

**File: `m68k_in.c`**

```c
M68KMAKE_OP(lpstop, 0, ., .)
{
    if (CPU_TYPE_IS_060_PLUS()) {
        if (!(FLAG_S)) {
            m68ki_exception_privilege_violation();
            return;
        }
        uint new_sr = OPER_I_16();
        m68ki_set_sr(new_sr);
        CPU_STOPPED |= STOP_LEVEL_STOP;
        // Low-power mode signaling (callback?)
    }
}
```

#### P5.3: 68060 PTEST Semantics

**File: `m68kmmu.h`**

The 68060 PTEST differs from 030/040:
- No MMUSR register updates
- Different status reporting

```c
void ptest_060(uint ea, int rw, int fc)
{
    // 68060-specific PTEST implementation
}
```

#### P5.4: EC060 MMU Disable

**File: `m68kcpu.c`**

```c
case M68K_CPU_TYPE_68EC060:
    CPU_TYPE = CPU_TYPE_EC060;
    HAS_PMMU = 0;  // No MMU
    HAS_FPU = 0;   // No FPU
    // ...
```

### Deliverables
- Working PLPA instruction
- Working LPSTOP instruction
- Correct MMU behavior per variant

---

## Phase 6: Variants (1 day)

### Objectives
- Configure LC060 and EC060 variants
- Ensure correct feature detection

### Tasks

#### P6.1: MC68LC060

- FPU disabled via PCR.DFP
- All FPU ops trap to vector 11
- Full MMU support

#### P6.2: MC68EC060

- FPU disabled
- MMU disabled
- Embedded controller mode

#### P6.3: PCR Identification

```c
// PCR values for identification
#define PCR_68060   0x04300000
#define PCR_LC060   0x04310000
#define PCR_EC060   0x04320000
```

### Deliverables
- All three variants functional
- Correct PCR identification

---

## Phase 7: Testing (1-2 weeks)

### Objectives
- Unit test new functionality
- Boot test with real software
- Regression testing

### Tasks

#### P7.1: Unit Tests

**File: `test/test_68060.c`**

- Test PCR read/write
- Test MOVEC with 060 registers
- Test trapped instructions raise correct vectors
- Test FPU operation partitioning
- Test exception frame formats
- Test RTE with 060 frames

#### P7.2: Boot Tests

| Test Target | Description |
|-------------|-------------|
| AmigaOS 3.x + 68060.library | Full system with ISP/FPSP |
| NetBSD/amiga 68060 | Unix with 68060 support |
| Linux/m68k | Linux with ifpsp060 |

#### P7.3: Regression Tests

- Ensure 68000-68040 still work correctly
- Run existing Musashi test suite
- Run MAME 68k test suites if available

### Deliverables
- Comprehensive test suite
- Boot test results documented
- No regressions in existing functionality

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| License contamination from MAME | Medium | High | Clean-room implementation |
| Subtle timing differences | High | Low | Document as approximate |
| Missing edge cases | Medium | Medium | Extensive boot testing |
| FPU accuracy issues | Low | Medium | Use existing softfloat |

## Success Criteria

1. All three variants boot AmigaOS with 68060.library
2. No regressions in existing CPU types
3. MIT license preserved
4. Documentation complete
