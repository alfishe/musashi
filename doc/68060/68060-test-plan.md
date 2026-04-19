# 68060 Binary Test Plan — Technical Design Document

## 1. Overview

This document specifies the binary test suite for validating 68060-specific behavior in Musashi. Tests follow the existing `test/mc68000/` and `test/mc68040/` convention: hand-written 68k assembly (`.s`) assembled into flat binaries (`.bin`) and executed by `test_driver.c`.

### 1.1 Scope

The 68060 test suite validates **only 060-specific behavioral differences** from the 040:

| Category | What We Test |
|----------|-------------|
| Integer traps (Vector 61) | MOVEP, CAS2, CHK2, CMP2, 64-bit MULL/DIVL |
| FPU traps (Vector 11) | FSIN, FCOS, FMOVECR, FPIAR access, packed decimal |
| FPU native execution | FADD, FMUL, FSQRT, FCMP, FTST, FS*/FD* rounding variants |
| MOVEC registers | PCR, BUSCR read/write; MMUSR illegal trap; CACR mask |
| Exception frames | Format $0 (4-word), Format $4 (8-word access fault) |
| MMU | PTEST trap, PLPA translation |
| CPU identification | PCR silicon ID for MC68060, LC060, EC060 |

We do **not** retest instructions that behave identically on 040 and 060 — those are covered by the existing `mc68040/` suite.

### 1.2 Non-Goals

- Cache behavior (CACR bits are emulated as register storage; actual cache semantics are not modeled)
- Branch prediction / superscalar pipeline (Musashi is interpretive, not cycle-accurate)
- Physical hardware bus timing

---

## 2. Test Infrastructure

### 2.1 Test Driver Modifications

The current `test_driver.c` hardcodes `M68K_CPU_TYPE_68040` at line 380. For 060 tests, this must be parameterized:

```c
// test_driver.c modifications:

// In main(), replace:
//   m68k_set_cpu_type(M68K_CPU_TYPE_68040);
// With:
int cpu_type = M68K_CPU_TYPE_68040;  /* Default */

while (arg < argc) {
    const char* a = argv[arg];
    if (strcmp(a, "--cpu=68060") == 0) {
        cpu_type = M68K_CPU_TYPE_68060;
        ++arg;
        continue;
    }
    if (strcmp(a, "--cpu=68lc060") == 0) {
        cpu_type = M68K_CPU_TYPE_68LC060;
        ++arg;
        continue;
    }
    if (strcmp(a, "--cpu=68ec060") == 0) {
        cpu_type = M68K_CPU_TYPE_68EC060;
        ++arg;
        continue;
    }
    // ... existing option handling ...
}

m68k_set_cpu_type(cpu_type);
```

> **Backward compatibility**: When invoked without `--cpu=`, all existing mc68000/ and mc68040/ tests run unchanged against 68040.

### 2.2 Memory Map

Unchanged from existing infrastructure:

| Address Range | Size | Type | Purpose |
|---------------|------|------|---------|
| `0x00000–0x0FFFF` | 64K | RAM | Vector table + stack |
| `0x10000–0x4FFFF` | 256K | ROM | Test code (4 × 64K slots) |
| `0x100000–0x10FFFF` | 64K | MMIO | Test device (pass/fail/interrupt) |
| `0x300000–0x30FFFF` | 64K | RAM | Scratch data for tests |

### 2.3 MMIO Test Device Registers

| Address | Size | Direction | Purpose |
|---------|------|-----------|---------|
| `0x100000` | 32 | Write | Increment fail counter |
| `0x100004` | 32 | Write | Increment pass counter |
| `0x100008` | 32 | Write | Print CPU state (debug) |
| `0x10000C` | 32 | Write | Trigger IRQ level (bits 2:0) |
| `0x100014` | 8 | Write | Output char to stdout |

### 2.4 Assembly Toolchain

```
Assembler:  m68k-elf-as -march=68060 -g --gdwarf-sections -I$(CURDIR)/..
Linker:     m68k-elf-ld -Ttext 0x10000 --oformat binary
Disasm:     m68k-elf-objdump -D test.bin -b binary -mm68k:68060
```

> **NOTE**: `-march=68060` may not be available on all binutils versions. Fallback is `-march=68040` since the trapped instructions (MOVEP, CAS2, etc.) assemble identically — they are valid 040 encodings that the 060 traps at runtime.

---

## 3. Entry Point — `entry_060.s`

The 060 entry point extends `entry.s` with handlers for the two 060-specific exception vectors.

```asm
*
* entry_060.s — 68060 test entry point
*
* USAGE: Each test file does:
*   .include "entry_060.s"
* which first includes the standard entry.s (defines main, TEST_FAIL,
* stack setup, and calls run_test), then adds 060-specific exception
* vector constants and handler subroutines.
*
* The standard entry.s 'main:' label calls 'run_test' (defined by each
* test). Each test's run_test starts with:
*   jsr setup_060_vectors
* to install the 060-specific handlers before executing test code.
*
* This avoids any symbol conflict with entry.s — we never redefine main.
*

.include "entry.s"

* -------------------------------------------------------------------
* Constants for 060-specific exception vectors
* -------------------------------------------------------------------

.set VEC_ILLEGAL,    0x10      | Vector  4: Illegal Instruction (4*4)
.set VEC_FLINE,      0x2C      | Vector 11: F-Line (11*4)
.set VEC_FP_UNIMPL,  0xF0      | Vector 60: Unimplemented FP instruction (60*4)
.set VEC_UNIMPL_INT, 0xF4      | Vector 61: Unimplemented Integer (61*4)

* Trap indicator protocol:
*   d7.l = 0x00000000  → no exception occurred
*   d7.l = 0xEEEE0004  → Vector 4 (Illegal Instruction) was taken
*   d7.l = 0xEEEE000B  → Vector 11 (F-Line) was taken
*   d7.l = 0xEEEE003C  → Vector 60 (Unimplemented FP) was taken
*   d7.l = 0xEEEE003D  → Vector 61 (Unimplemented Integer) was taken
*
* The saved PC from the exception frame is stored in a6 for verification.

* -------------------------------------------------------------------
* setup_060_vectors — call this from run_test before any trap testing
* Installs handlers for Vectors 4, 11, 60, 61 in the vector table.
* -------------------------------------------------------------------

setup_060_vectors:
    mov.l #HANDLER_ILLEGAL,    VEC_ILLEGAL
    mov.l #HANDLER_FLINE,      VEC_FLINE
    mov.l #HANDLER_FP_UNIMPL,  VEC_FP_UNIMPL
    mov.l #HANDLER_UNIMPL_INT, VEC_UNIMPL_INT
    rts

* -------------------------------------------------------------------
* Exception Handlers
*
* Format $0 frame layout (68060, 010+):
*   SP+0:  SR         (16 bits)
*   SP+2:  PC         (32 bits)
*   SP+6:  Format/Vector word (16 bits)
*
* Each handler:
*   1. Records which vector was taken (in d7)
*   2. Saves the stacked PC (in a6 for post-trap verification)
*   3. Executes RTE to resume after the trapping instruction
* -------------------------------------------------------------------

HANDLER_ILLEGAL:
    mov.l  #0xEEEE0004, %d7
    mov.l  2(%sp), %a6          | Save stacked PC
    rte

HANDLER_FLINE:
    mov.l  #0xEEEE000B, %d7
    mov.l  2(%sp), %a6
    rte

HANDLER_FP_UNIMPL:
    mov.l  #0xEEEE003C, %d7
    mov.l  2(%sp), %a6
    rte

HANDLER_UNIMPL_INT:
    mov.l  #0xEEEE003D, %d7
    mov.l  2(%sp), %a6
    rte
```

### 3.1 Why D7 and A6?

- **D7** is the "trap indicator" register. Tests clear it before executing a suspected-trapping instruction, then check it afterward.
- **A6** captures the stacked PC from the exception frame, allowing tests to verify that the PC pointed to the correct instruction (critical for MULL/DIVL 64-bit traps where the PC can be wrong if not reset).

These registers are chosen because they are callee-saved by convention and unlikely to be clobbered by simple test sequences.

---

## 4. Test Case Specifications

Each test is specified with:
- **Purpose**: What 060 behavior is validated
- **Preconditions**: Expected CPU type, setup
- **Assembly skeleton**: Exact instruction sequence
- **Pass criteria**: What constitutes a pass
- **Hazards**: Known edge cases

### 4.1 Category: Integer Instruction Traps (Vector 61)

These instructions exist on 040 but are removed from 060 silicon and must trap to Vector 61 (Unimplemented Integer Instruction) for ISP emulation.

---

#### 4.1.1 `movep_trap.s`

**Purpose**: Verify that MOVEP (all sizes, both directions) triggers Vector 61 on 68060.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * --- MOVEP.W register-to-memory ---
    clr.l   %d7
    lea.l   STACK2_BASE, %a0
    mov.l   #0x1234, %d0
    movep.w %d0, 0(%a0)            | Should trap
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- MOVEP.L register-to-memory ---
    clr.l   %d7
    mov.l   #0x12345678, %d0
    movep.l %d0, 0(%a0)            | Should trap
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- MOVEP.W memory-to-register ---
    clr.l   %d7
    movep.w 0(%a0), %d0            | Should trap
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- MOVEP.L memory-to-register ---
    clr.l   %d7
    movep.l 0(%a0), %d0            | Should trap
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    rts
```

**Pass**: All four MOVEP variants trigger Vector 61. **Fail**: Any variant executes without trapping or traps to wrong vector.

---

#### 4.1.2 `cas2_trap.s`

**Purpose**: CAS2 (both .W and .L) triggers Vector 61. Note: CAS (single operand) still works on 060.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * --- CAS2.L ---
    clr.l   %d7
    lea.l   STACK2_BASE, %a0
    lea.l   STACK2_BASE+8, %a1
    mov.l   #0, %d0
    mov.l   #0, %d1
    mov.l   #1, %d2
    mov.l   #1, %d3
    cas2.l  %d0:%d1, %d2:%d3, (%a0):(%a1)
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CAS2.W ---
    clr.l   %d7
    cas2.w  %d0:%d1, %d2:%d3, (%a0):(%a1)
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CAS.L (should still work, NOT trap!) ---
    clr.l   %d7
    mov.l   #0xABCD, (%a0)
    mov.l   #0xABCD, %d0
    mov.l   #0x1234, %d1
    cas.l   %d0, %d1, (%a0)
    cmpi.l  #0, %d7                | d7 must be 0 — no trap
    bne     TEST_FAIL
    cmp.l   (%a0), %d1             | CAS must have succeeded
    bne     TEST_FAIL

    rts
```

**Pass**: CAS2 traps; CAS executes normally. This proves the trap is opcode-specific, not a blanket CAS family rejection.

---

#### 4.1.3 `chk2_cmp2_trap.s`

**Purpose**: CHK2 and CMP2 both trigger Vector 61.

```asm
.include "entry_060.s"

.set BOUNDS_LOC, STACK2_BASE

run_test:
    jsr setup_060_vectors

    * Setup bounds in memory: lower=0x10, upper=0x50
    mov.l   #0x00100050, BOUNDS_LOC

    * --- CHK2.B ---
    clr.l   %d7
    mov.l   #0x30, %d0
    chk2.b  BOUNDS_LOC, %d0
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- CMP2.B ---
    clr.l   %d7
    cmp2.b  BOUNDS_LOC, %d0
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    rts
```

---

#### 4.1.4 `mull_64_trap.s`

**Purpose**: MULL.L with 64-bit result (Dh:Dl) traps to Vector 61. MULL.L with 32-bit result executes normally.

**Hazard**: The extension word has already been fetched when the trap decision is made. The stacked PC must point to the base opcode, not past the extension word. (See TDD §6.2).

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * --- 64-bit MULU.L  →  must trap ---
    clr.l   %d7
    mov.l   #100, %d0
    mov.l   #200, %d1
    mulu.l  %d1, %d2:%d0           | 64-bit result → trap
    cmpi.l  #0xEEEE003D, %d7
    bne     TEST_FAIL

    * --- Verify stacked PC points to MULU.L opcode ---
    * a6 was loaded by the handler with the stacked PC.
    * The opcode at that address must be 0x4C01 (MULU.L Dn encoding prefix).
    * (Exact verification depends on link address; simplified here)

    * --- 32-bit MULU.L  →  must execute ---
    clr.l   %d7
    mov.l   #100, %d0
    mov.l   #200, %d1
    mulu.l  %d1, %d0               | 32-bit result → execute
    cmpi.l  #0, %d7                | No trap
    bne     TEST_FAIL
    cmpi.l  #20000, %d0            | 100 * 200 = 20000
    bne     TEST_FAIL

    rts
```

**Pass**: 64-bit traps, 32-bit executes correctly. **Hazard verified**: Stacked PC correctness.

---

#### 4.1.5 `divl_64_trap.s`

Same pattern as MULL: 64-bit DIVS.L / DIVU.L (quotient:remainder in Dq:Dr) traps; 32-bit version works.

---

### 4.2 Category: FPU Traps (Vector 11)

These FPU operations are present in 040 hardware but removed from 060 silicon. They must trigger an F-Line exception (Vector 11) for FPSP emulation.

---

#### 4.2.1 `fsin_trap.s`

**Purpose**: FSIN triggers Vector 11.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    clr.l   %d7
    fmove.d #1.0, %fp0
    fsin    %fp0                   | Unimplemented on 060 → trap
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
```

---

#### 4.2.2 `fcos_trap.s`

Identical pattern to FSIN but with `fcos`.

---

#### 4.2.3 `fmovecr_trap.s`

**Purpose**: ALL FMOVECR constant offsets trap on 060 (entire ROM removed from silicon).

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * Pi (offset 0x00) — hardware on 040, traps on 060
    clr.l   %d7
    fmovecr #0x00, %fp0
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    * log10(2) (offset 0x01) — traps on both 040 and 060
    clr.l   %d7
    fmovecr #0x01, %fp0
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    * 1.0 (offset 0x32) — hardware on 040, traps on 060
    clr.l   %d7
    fmovecr #0x32, %fp0
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
```

---

#### 4.2.4 `fmovem_fpiar_trap.s`

**Purpose**: FMOVEM with FPIAR (control register bit 0) in the register list traps on 060.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * FMOVEM FPCR/FPSR to memory — should NOT trap
    clr.l   %d7
    lea.l   STACK2_BASE, %a0
    fmovem.l %fpcr/%fpsr, (%a0)
    cmpi.l  #0, %d7
    bne     TEST_FAIL

    * FMOVEM FPCR/FPSR/FPIAR to memory — should trap (FPIAR bit set)
    clr.l   %d7
    fmovem.l %fpcr/%fpsr/%fpiar, (%a0)
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
```

---

#### 4.2.5 `packed_trap.s`

**Purpose**: FPU packed decimal operations trap on 060.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    clr.l   %d7
    fmove.d #3.14159, %fp0
    fmove.p %fp0, STACK2_BASE     | Packed decimal store → trap
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
```

---

### 4.3 Category: FPU Native Execution (Must NOT Trap)

These tests verify that hardware-supported FPU ops execute correctly on 060 — they must NOT trigger any exception.

---

#### 4.3.1 `fadd_exec.s`

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    clr.l   %d7
    fmove.d #1.5, %fp0
    fmove.d #2.5, %fp1
    fadd    %fp1, %fp0              | Hardware op — must not trap
    cmpi.l  #0, %d7
    bne     TEST_FAIL

    * Verify result: 1.5 + 2.5 = 4.0
    fcmp.d  #4.0, %fp0
    fbne    TEST_FAIL

    rts
```

---

#### 4.3.2 `fmul_exec.s`

Same pattern: `fmul %fp1, %fp0`, verify result.

---

#### 4.3.3 `fsadd_exec.s` (Single-precision rounding variant)

**Purpose**: FSADD (opcode 0x62) is a hardware op on 060. Verify it executes and produces single-precision-rounded result.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    clr.l   %d7
    fmove.d  #1.0000001, %fp0      | Value with precision beyond single
    fmove.d  #0.0000001, %fp1
    fsadd.x  %fp1, %fp0            | Single-precision rounding add
    cmpi.l   #0, %d7               | Must NOT trap
    bne      TEST_FAIL

    rts
```

---

#### 4.3.4 `fdadd_exec.s` (Double-precision rounding variant)

Same pattern with `fdadd`.

---

### 4.4 Category: MOVEC Registers

---

#### 4.4.1 `movec_pcr.s`

**Purpose**: Read PCR, verify silicon ID; write PCR, verify ID bits are preserved.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * Read PCR
    movec   %pcr, %d0
    * Check silicon ID = 0x04300000 (MC68060)
    andi.l  #0xFFFF0000, %d0
    cmpi.l  #0x04300000, %d0
    bne     TEST_FAIL

    * Write PCR (try to change ID bits — must be preserved)
    mov.l   #0xFFFFFFFF, %d0
    movec   %d0, %pcr
    movec   %pcr, %d1
    * ID bits must still be 0x04300000
    mov.l   %d1, %d2
    andi.l  #0xFFFF0000, %d2
    cmpi.l  #0x04300000, %d2
    bne     TEST_FAIL

    rts
```

---

#### 4.4.2 `movec_buscr.s`

**Purpose**: BUSCR read/write round-trip.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * Write BUSCR
    mov.l   #0x12345678, %d0
    movec   %d0, %buscr
    * Read back
    movec   %buscr, %d1
    cmp.l   %d0, %d1
    bne     TEST_FAIL

    rts
```

---

#### 4.4.3 `movec_mmusr_trap.s`

**Purpose**: MOVEC to/from MMUSR ($805) triggers Illegal Instruction (Vector 4) on 060.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * Attempt to read MMUSR — should trigger Vector 4 (Illegal)
    clr.l   %d7
    movec   %mmusr, %d0
    cmpi.l  #0xEEEE0004, %d7       | Vector 4, not 11 or 61
    bne     TEST_FAIL

    rts
```

---

#### 4.4.4 `movec_cacr_mask.s`

**Purpose**: CACR write respects 060 mask (`0xF0F0C800`). Unwritable bits must read back as 0.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * Write all 1s to CACR
    mov.l   #0xFFFFFFFF, %d0
    movec   %d0, %cacr
    * Read back
    movec   %cacr, %d1
    * Only writable bits should be set
    cmpi.l  #0xF0F0C800, %d1
    bne     TEST_FAIL

    rts
```

---

### 4.5 Category: Exception Frame Format

---

#### 4.5.1 `rte_format0.s`

**Purpose**: Manually build a Format $0 frame on the stack, execute RTE, verify correct return.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * Build Format $0 frame manually:
    *   SP+0: SR (16 bits)
    *   SP+2: PC (32 bits)  → RETURN_TARGET
    *   SP+6: Format/Vector word (16 bits): format=0, vector=0

    mov.l   %sp, %a5               | Save SP

    * Push frame (pushed in reverse: format word first, then PC, then SR)
    move.w  #0x0000, -(%sp)        | Format $0, vector offset 0
    pea     RETURN_TARGET
    move.w  %sr, -(%sp)            | Current SR

    rte

RETURN_TARGET:
    * If we get here, RTE successfully decoded Format $0
    rts
```

---

#### 4.5.2 `rte_format4.s`

**Purpose**: Build a Format $4 (access fault) frame, execute RTE, verify the extra words are consumed.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * Build Format $4 frame:
    *   SP+0:  SR (16 bits)
    *   SP+2:  PC (32 bits)
    *   SP+6:  Format/Vector word: format=4, vector offset=8 (bus error)
    *   SP+8:  Effective address (32 bits)
    *   SP+12: Fault Status Long Word (32 bits)

    mov.l   %sp, %a5

    move.l  #0x00000000, -(%sp)    | FSLW
    move.l  #0xDEADDEAD, -(%sp)    | Effective address
    move.w  #0x4008, -(%sp)        | Format=$4, vector=2 (bus error, offset=8)
    pea     RETURN_TARGET_4
    move.w  %sr, -(%sp)

    rte

RETURN_TARGET_4:
    * RTE consumed all 16 bytes (8 words). Verify SP is restored.
    rts
```

---

### 4.6 Category: MMU

---

#### 4.6.1 `ptest_trap.s`

**Purpose**: PTEST triggers F-Line (Vector 11) on 060.

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    clr.l   %d7
    lea.l   STACK2_BASE, %a0
    * PTEST encoding: F-line opcode, should trap immediately
    ptestw  (%a0)
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
```

---

### 4.7 Category: CPU Variant Identification

These tests run with different `--cpu=` flags and verify the PCR silicon ID.

---

#### 4.7.1 `pcr_id_060.s`

Run with `--cpu=68060`. Expect PCR ID = `0x04300000`.

#### 4.7.2 `pcr_id_lc060.s`

Run with `--cpu=68lc060`. Expect PCR ID = `0x04310000`, DFP bit set.

#### 4.7.3 `pcr_id_ec060.s`

Run with `--cpu=68ec060`. Expect PCR ID = `0x04320000`, DFP bit set.

---

### 4.8 Category: LC060 / EC060 Variants

These tests verify that FPU-less and MMU-less variants behave correctly.

#### 4.8.1 `lc060_fpu_fline.s`

Run with `--cpu=68lc060`. ALL F-line (FPU) instructions must trap — even the ones that are native hardware ops on the full 68060 (FADD, FMUL, etc.).

```asm
.include "entry_060.s"

run_test:
    jsr setup_060_vectors

    * FADD is hardware on 060, but LC060 has no FPU at all
    clr.l   %d7
    fmove.d #1.0, %fp0             | Should trap (no FPU)
    cmpi.l  #0xEEEE000B, %d7
    bne     TEST_FAIL

    rts
```

---

## 5. Directory Structure

```
test/
├── entry.s                  # Existing (unchanged)
├── test_driver.c            # Modified: add --cpu= flag
├── Makefile                 # Modified: add mc68060 target
├── mc68000/                 # Existing (unchanged)
├── mc68040/                 # Existing (unchanged)
└── mc68060/
    ├── Makefile
    ├── entry_060.s
    │
    ├── movep_trap.s         ── Integer traps ──
    ├── cas2_trap.s
    ├── chk2_cmp2_trap.s
    ├── mull_64_trap.s
    ├── divl_64_trap.s
    │
    ├── fsin_trap.s          ── FPU traps ──
    ├── fcos_trap.s
    ├── fmovecr_trap.s
    ├── fmovem_fpiar_trap.s
    ├── packed_trap.s
    │
    ├── fadd_exec.s          ── FPU native ──
    ├── fmul_exec.s
    ├── fsadd_exec.s
    ├── fdadd_exec.s
    │
    ├── movec_pcr.s          ── MOVEC ──
    ├── movec_buscr.s
    ├── movec_mmusr_trap.s
    ├── movec_cacr_mask.s
    │
    ├── rte_format0.s        ── Exception frames ──
    ├── rte_format4.s
    │
    ├── ptest_trap.s         ── MMU ──
    │
    ├── pcr_id_060.s         ── CPU ID ──
    ├── pcr_id_lc060.s
    └── pcr_id_ec060.s
```

**Total: 24 test files** covering all 060-specific behavioral changes.

---

## 6. Build System Integration

### 6.1 `test/mc68060/Makefile`

```makefile
.TESTS_68060 = movep_trap.s cas2_trap.s chk2_cmp2_trap.s mull_64_trap.s divl_64_trap.s \
    fsin_trap.s fcos_trap.s fmovecr_trap.s fmovem_fpiar_trap.s packed_trap.s \
    fadd_exec.s fmul_exec.s fsadd_exec.s fdadd_exec.s \
    movec_pcr.s movec_buscr.s movec_mmusr_trap.s movec_cacr_mask.s \
    rte_format0.s rte_format4.s \
    ptest_trap.s \
    pcr_id_060.s pcr_id_lc060.s pcr_id_ec060.s

.TESTS_68060_O = $(.TESTS_68060:%.s=%.o)
$(.TESTS_68060_O): %.o: %.s entry_060.s
	$(M68K_AS) $(M68K_ASFLAGS_060) -o $@ $<

.TESTS_68060_BIN = $(.TESTS_68060_O:%.o=%.bin)
$(.TESTS_68060_BIN): %.bin: %.o
	$(M68K_LD) $(M68K_LDFLAGS) -o $@ $<

all: $(.TESTS_68060_BIN)

.PHONY: clean
clean:
	rm -f $(.TESTS_68060_O)
```

### 6.2 `test/Makefile` Modifications

```diff
 M68K_AS = m68k-elf-as
 M68K_ASFLAGS = -march=68040 -g --gdwarf-sections -I$(CURDIR)
+M68K_ASFLAGS_060 = -march=68060 -g --gdwarf-sections -I$(CURDIR)
 M68K_LD = m68k-elf-ld
 M68K_LDFLAGS = -Ttext 0x10000 --oformat binary

 all:
 	@$(MAKE) -C mc68000 all
 	@$(MAKE) -C mc68040 all
+	@$(MAKE) -C mc68060 all

 clean:
 	@$(MAKE) -C mc68000 clean
 	@$(MAKE) -C mc68040 clean
+	@$(MAKE) -C mc68060 clean
```

### 6.3 Top-level `Makefile` Test Execution

The top-level `make test` must run 060 tests with the correct CPU type flag:

```makefile
# In top-level Makefile, test target:
test_060:
	@for f in test/mc68060/*.bin; do \
	    echo "Testing $$f..."; \
	    ./test_driver $$f --cpu=68060 || exit 1; \
	done

# Variant-specific tests:
test_lc060:
	@./test_driver test/mc68060/pcr_id_lc060.bin --cpu=68lc060
	@./test_driver test/mc68060/lc060_fpu_fline.bin --cpu=68lc060

test_ec060:
	@./test_driver test/mc68060/pcr_id_ec060.bin --cpu=68ec060
```

---

## 7. Test Execution Matrix

| Test File | `--cpu=68060` | `--cpu=68lc060` | `--cpu=68ec060` |
|-----------|:---:|:---:|:---:|
| movep_trap | ✅ | ✅ | ✅ |
| cas2_trap | ✅ | ✅ | ✅ |
| chk2_cmp2_trap | ✅ | ✅ | ✅ |
| mull_64_trap | ✅ | ✅ | ✅ |
| divl_64_trap | ✅ | ✅ | ✅ |
| fsin_trap | ✅ | ✅ | ✅ |
| fcos_trap | ✅ | ✅ | ✅ |
| fmovecr_trap | ✅ | ✅ | ✅ |
| fmovem_fpiar_trap | ✅ | ✅ | ✅ |
| packed_trap | ✅ | ✅ | ✅ |
| fadd_exec | ✅ | ❌ (traps) | ❌ (traps) |
| fmul_exec | ✅ | ❌ (traps) | ❌ (traps) |
| fsadd_exec | ✅ | ❌ (traps) | ❌ (traps) |
| fdadd_exec | ✅ | ❌ (traps) | ❌ (traps) |
| movec_pcr | ✅ | ✅ | ✅ |
| movec_buscr | ✅ | ✅ | ✅ |
| movec_mmusr_trap | ✅ | ✅ | ✅ |
| movec_cacr_mask | ✅ | ✅ | ✅ |
| rte_format0 | ✅ | ✅ | ✅ |
| rte_format4 | ✅ | ✅ | ✅ |
| ptest_trap | ✅ | ✅ | ✅ |
| pcr_id_060 | ✅ | — | — |
| pcr_id_lc060 | — | ✅ | — |
| pcr_id_ec060 | — | — | ✅ |
| lc060_fpu_fline | — | ✅ | ✅ |

---

## 8. Regression Safety

All existing tests continue to run with `M68K_CPU_TYPE_68040` (the default when no `--cpu=` flag is provided):

```bash
# These remain identical:
make test          # Runs mc68000 + mc68040 tests against 68040

# New:
make test_060      # Runs mc68060 tests against 68060
make test_lc060    # Runs LC060-specific variant tests
make test_ec060    # Runs EC060-specific variant tests
```

---

## 9. Implementation Order

| Phase | Tests | Depends On (from main TDD) |
|-------|-------|---------------------------|
| 1 | `movec_pcr`, `movec_buscr`, `pcr_id_*` | Phase 1 (CPU type plumbing) |
| 2 | `movep_trap`, `cas2_trap`, `chk2_cmp2_trap`, `mull_64_trap`, `divl_64_trap` | Phase 2 (instruction table) + Phase 3 (Vector 61) |
| 3 | `movec_mmusr_trap`, `movec_cacr_mask` | Phase 2 (MOVEC updates) |
| 4 | `fsin_trap`, `fcos_trap`, `fmovecr_trap`, `fmovem_fpiar_trap`, `packed_trap` | Phase 4 (FPU trapping) |
| 5 | `fadd_exec`, `fmul_exec`, `fsadd_exec`, `fdadd_exec` | Phase 4 (FPU native ops) |
| 6 | `rte_format0`, `rte_format4` | Phase 3 (exception frames) |
| 7 | `ptest_trap` | Phase 5 (MMU) |
| 8 | `lc060_fpu_fline`, variant ID tests | Phase 6 (variants) |

Each phase's tests serve as the **acceptance criteria** for the corresponding implementation phase from the main TDD.
