# Musashi Opcode Generation

This document explains how Musashi generates 1,967 opcode handler functions from
523 template definitions, the constraints that shape the system, and how
instruction-specific behavior is injected into the generation pipeline.

---

## 1. The Generation Pipeline

```
┌──────────────┐     m68kmake      ┌──────────────┐     compiler    ┌──────────┐
│  m68k_in.c   │ ──────────────►   │  m68kops.c   │ ─────────────►  │ .o file  │
│  (templates) │   (C generator)   │  (generated) │                 │          │
│  m68kmake.c  │                   │  m68kops.h   │                 │          │
└──────────────┘                   └──────────────┘                 └──────────┘
```

### Source files

| File | Role |
|------|------|
| `m68k_in.c` | Contains 523 handler templates (`M68KMAKE_OP(...)`) and a 518-row instruction table |
| `m68kmake.c` | Standalone C program that reads `m68k_in.c` and writes `m68kops.c` / `m68kops.h` |
| `m68kops.c` | **Generated** — 1,967 concrete handler functions, ~38K lines |
| `m68kops.h` | **Generated** — function prototypes and jump table declarations |

`m68kmake` is compiled and run as a **build step** (see `Makefile` or `CMakeLists.txt`).
It is never linked into the emulator binary.

---

## 2. 68000 ISA Instruction Groups

The 68000 ISA has ~56 instruction mnemonics for the base 68000 CPU (growing to 115
in the full 68000–68060 table in Musashi). They fall into six categories by memory
access pattern — this categorization is critical because it determines which
instructions need special address error handling.

### Category 1: Register-Only (no memory data access)

**Instructions**: ADD Dn,Dn, SUB Dn,Dn, AND Dn,Dn, OR Dn,Dn, EOR Dn,Dn,
ABCD Dn,Dn, SBCD Dn,Dn, ADDX Dn,Dn, SUBX Dn,Dn, EXG, SWAP, EXT, MOVEQ,
CMP Dn,Dn, NEG Dn, NOT Dn, shift/rotate register variants,
BCHG/BCLR/BSET/BTST register variants

**Memory access pattern**: None — all operands are in registers.

**AERR behavior**: No data-memory address errors are possible. AERR can only
occur during opcode fetch (if PC is odd), which is handled generically by the
execution loop, not by individual instruction templates.

**Template pattern**: Specific EA mode (`d` or `a`), no dot expansion. These
templates contain no tokens and no AERR code.

### Category 2: Single Memory Read

**Instructions**: ADDA, SUBA, CMPA, TST, CMP, CMPI, CMPM, BTST #imm,EA

**Memory access pattern**: Read one operand from memory, write result to a
register (or just set flags). Only one data-memory access.

**AERR behavior**: If the read faults, REG_PC has advanced past the source
extension words (for modes that have them). The default `REG_PC - 2` stacked PC
is correct because REG_PC points to the next instruction word — which is the
right recovery point. No per-mode adjustment needed.

**Template pattern**: Dot expansion for the source EA, but all expanded variants
use the default AERR behavior. The `OPER_AY_*` read functions are sufficient —
no wrapper needed.

**Why no exception**: Single-access instructions have a simple relationship
between REG_PC and the fault point. The default offset works for all EA modes.

### Category 3: Read-Modify-Write at Same Address (RMW)

**Instructions**: ADD EA, SUB EA, AND EA, OR EA, EOR EA, ADDI, SUBI, ANDI,
ORI, EORI, ADDQ, SUBQ, NEG, NOT, CLR, TAS, BCHG/BCLR/BSET #imm,EA,
shift/rotate memory variants, ABCD/SBCD/ADDX/SUBX memory-to-memory, NBCD

**Memory access pattern**: Read data from EA, modify it, write result back to
the **same** EA address.

**AERR behavior**: If the read succeeds, the address is properly aligned (the
68000 would have faulted on the read if it weren't). The subsequent write goes
to the same address — so it cannot cause an alignment fault that the read didn't.
Since the write can't fault if the read didn't, no special AERR handling is needed
for the write phase.

**Template pattern**: Dot expansion, one token for the OPER read. The RMW write
uses `m68ki_write_*()` directly — no wrapper needed because the write can't
fail independently.

**Why no exception**: Source and destination are the same address. If the read
succeeds, the write succeeds. Self-correcting by design.

### Category 4: Read Source, Write to Different Destination

**Instructions**: **MOVE** (all 107 templates), MOVEM (register→memory)

**Memory access pattern**: Read data from source EA, write to destination EA —
two **different** addresses. The destination write can fault even when the source
read succeeded, because they're different addresses.

**AERR behavior**: This is the **only category** that needs per-EA-mode AERR
specialization. When the destination write faults:
- REG_PC has been advanced by the source extension words (if any)
- REG_PC has been advanced by the destination extension words (if any)
- The correct stacked PC depends on the source EA mode (which determines
  how far REG_PC advanced before the fault)

**Template pattern**: MOVE has per-destination-mode templates (via `spec proc`),
and uses dot expansion for the source. The token substitution system was built
specifically for this case.

**Why exception needed**: Two different addresses means the write can fail
independently of the read. The stacked PC at the fault point depends on which
source extension words were consumed — and this varies per EA mode within the
dot template.

### Category 5: Stack Operations and Branches

**Instructions**: BCC, BRA, BSR, DBCC, JMP, JSR, RTR, RTS, RTE, LINK, UNLK,
MOVEM (memory→register)

**Memory access pattern**: Stack pushes/pulls or no data access. Stack operations
use supervisor memory space with its own fault handling.

**AERR behavior**: Branches with odd target addresses cause a separate exception
type. Stack operations in supervisor mode have different FC codes. These are
handled by the branch/exception template code, not by per-EA-mode tokens.

### Category 6: System Control

**Instructions**: MOVE to/from SR, MOVE to/from CCR, MOVE USP, ANDI/ORI/EORI
to SR/CCR, RESET, STOP, NOP, TRAP, TRAPV, TRAPCC

**Memory access pattern**: Mostly register-only or system-space accesses.

**AERR behavior**: System control instructions either don't access data memory
or use specific system-space mechanisms. No per-EA-mode handling needed.

---

## 3. The Instruction Table

Near the top of `m68k_in.c`, after `M68KMAKE_TABLE_BODY`, lives a table that tells
m68kmake which opcodes exist and how they map to 68000 bit patterns:

```
              spec  spec                    allowed ea              cpu cycles
name    size  proc   ea   bit pattern       A+-DXWLdxI  0 1 2 3 4  000 010 020 030 040
======  ====  ====  ====  ================  ==========  = = = = =  === === === === ===
adda      32  .     d     1101...111000...  ..........  U U U U U   6   6   2   2   2
adda      32  .     a     1101...111001...  ..........  U U U U U   6   6   2   2   2
adda      32  .     .     1101...111......  A+-DXWLdxI  U U U U U   6   6   2   2   2
```

Key columns:

- **spec proc** — special processing mode (`.` = normal, `er` = EA→reg, `rr` = reg→reg, etc.)
  For MOVE instructions, this field is the *destination* EA mode.
- **spec ea** — source effective addressing mode:
  - `d`, `a`, `ai`, `pi`, `pd`, etc. → generates one specific handler
  - `.` (dot) → **expands into multiple handlers** (see §4)
- **allowed ea** — which EA modes the `.` expands to:
  `A+-DXWLdxI` = ai + pi + pd + di + ix + aw + al + pcdi + pcix + i (10 modes)

---

## 4. The Dot (`.`) Expander — How One Template Becomes Many Handlers

When a table entry has `spec ea = .`, m68kmake generates **one handler per allowed
addressing mode** from a single template body.

### Standard case: ADDA.l (no per-mode specialization needed)

Table entry:
```
adda      32  .     .     1101...111......  A+-DXWLdxI  U U U U U   6   6   2   2   2
```

Template body:
```c
M68KMAKE_OP(adda, 32, ., .)
{
    uint src = M68KMAKE_GET_OPER_AY_32;   // ← substitution token
    uint* r_dst = &AX;
    *r_dst = MASK_OUT_ABOVE_32(*r_dst + src);
}
```

m68kmake expands this into 10 concrete handlers by substituting the token:

| Generated handler | Token resolves to |
|---|---|
| `m68k_op_adda_32_ai()` | `OPER_AY_AI_32()` |
| `m68k_op_adda_32_pi()` | `OPER_AY_PI_32()` |
| `m68k_op_adda_32_pd()` | `OPER_AY_PD_32()` |
| `m68k_op_adda_32_di()` | `OPER_AY_DI_32()` |
| `m68k_op_adda_32_ix()` | `OPER_AY_IX_32()` |
| `m68k_op_adda_32_aw()` | `OPER_AW_32()` |
| `m68k_op_adda_32_al()` | `OPER_AL_32()` |
| `m68k_op_adda_32_pcdi()` | `OPER_PCDI_32()` |
| `m68k_op_adda_32_pcix()` | `OPER_PCIX_32()` |
| `m68k_op_adda_32_i()` | `OPER_I_32()` |

This is the **vast majority** of dot templates — all expanded variants share
the same AERR behavior, so a simple token substitution is sufficient.

### Exception case: MOVE al (per-mode specialization required)

Table entry:
```
move      16  al    .     0011001111......  A+-DXWLdxI  U U U U U  16  16   6   6   6
```

Template body:
```c
M68KMAKE_OP(move, 16, al, .)
{
    uint res = M68KMAKE_GET_OPER_AY_16;   // read source
    uint ea = EA_AL_16();                  // read dest extension words → advances REG_PC

    FLAG_N = NFLAG_16(res);
    FLAG_Z = res;
    FLAG_V = VFLAG_CLEAR;
    FLAG_C = CFLAG_CLEAR;
    M68KMAKE_WRITE_AL_16(ea, res);         // write to dest — may fault
}
```

The `M68KMAKE_WRITE_AL_16` token resolves **differently** per source EA mode:

| Generated handler | WRITE_AL_16 resolves to |
|---|---|
| `m68k_op_move_16_al_ai()` | `m68ki_write_16_al_dest(ea, res)` — offset=-2 |
| `m68k_op_move_16_al_pi()` | `m68ki_write_16_al_dest(ea, res)` — offset=-2 |
| ... | ... |
| `m68k_op_move_16_al_pcix()` | `m68ki_write_16_al_dest(ea, res)` — offset=-2 |
| `m68k_op_move_16_al_i()` | `m68ki_write_16(ea, res)` — no offset |

Only the **immediate source** variant gets plain `m68ki_write_16`. All other
source modes get the wrapper with `aerr_pc_offset = -2`.

### Substitution tokens

m68kmake recognizes these tokens in template bodies and replaces them with
EA-mode-specific code:

| Token | Replaced with | Used by | Since |
|-------|--------------|---------|-------|
| `M68KMAKE_GET_EA_AY_8/16/32` | `EA_AY_AI_32()`, `EA_AY_PI_32()`, etc. | All dot templates | Original |
| `M68KMAKE_GET_OPER_AY_8/16/32` | `OPER_AY_AI_32()`, `OPER_AY_PI_32()`, etc. | All dot templates | Original |
| `M68KMAKE_CC` / `M68KMAKE_NOT_CC` | Condition code test (`FLAG_Z`, etc.) | BCC, SCC, etc. | Original |
| `M68KMAKE_WRITE_AL_16/32` | Write with or without AERR offset | MOVE al only | Added 2025 |

All tokens are defined in `m68kmake.c` as `#define ID_OPHANDLER_*` macros
and resolved in `generate_opcode_handler()`.

### The key constraint

**A `.` template produces one body shared across all EA modes.**
The only per-mode variation comes from token substitution. You cannot write
different code for mode AI vs mode PI within a single `.` template — only the
token values change.

This is why instructions like MOVE use **per-mode templates** for their
*destination* (MOVE's `spec proc` field), while using `.` for their *source*.

---

## 5. Specific vs Dot Templates

### Specific templates (213 table entries)

When the table specifies an exact EA mode, m68kmake generates exactly one handler
with no token substitution:

```
adda      32  .     d     1101...111000...  ..........  U U U U U   6   6   2   2   2
```

The template `M68KMAKE_OP(adda, 32, ., d)` is written verbatim — no tokens
are present or needed.

### Dot templates (305 table entries, 115 unique instructions)

The `.` in `spec ea` causes multi-mode expansion. 305 table entries expand
into ~1,754 handlers (the bulk of the 1,967 total).

### Why both exist

Specific entries serve two purposes:
1. **Opcodes that only work with one mode** (e.g., register-to-register `abcd`)
2. **Opcodes where the general case needs special handling** (e.g., MOVE to
   register source vs memory source have different flag behavior)

### Breakdown by ISA group

| ISA Group | Table entries | Dot (`.`) | Specific | Unique instructions |
|-----------|:---:|:---:|:---:|:---:|
| Data Movement | 115 | 54 | 61 | 12 |
| Arithmetic | 88 | 44 | 44 | 14 |
| Logic | 54 | 33 | 21 | 7 |
| Shift/Rotate | 56 | 56 | 0 | 8 |
| Bit Manipulation | 16 | 8 | 8 | 4 |
| BCD | 12 | 5 | 7 | 3 |
| Compare/Test | 55 | 17 | 38 | 7 |
| Branch/Jump | 12 | 12 | 0 | 8 |
| Condition/Trap | 6 | 5 | 1 | 3 |
| System Control | 4 | 4 | 0 | 4 |
| 68020+ extensions | 98 | 65 | 33 | 43 |
| **Total** | **518** | **305** | **213** | **115** |

Note: Shift/Rotate and Branch/Jump use dot templates exclusively — every variant
shares the same body with no per-mode specialization needed.

---

## 6. Why Only MOVE Needs Per-Mode AERR Specialization

### The architectural reason

The 68000 instruction execution sequence is:

```
1. Fetch opcode word(s)          → REG_PC += 2 per word
2. Fetch source EA extension     → REG_PC += 2..4 (varies by EA mode)
3. Read source data from EA      → may fault here
4. Fetch dest EA extension       → REG_PC += 2..4 (MOVE only)
5. Write result to dest EA       → may fault here
```

Most instructions stop at step 3 or combine steps 3+5 at the same address.
MOVE is unique because:

1. **It has two separate addresses** — source and destination are different EAs
2. **Both addresses are independently faultable** — the write can fail even when
   the read succeeded
3. **The dest write occurs AFTER source PC advancement** — so REG_PC reflects
   source extension consumption, not just opcode consumption
4. **Source EA mode determines REG_PC at fault time** — different source modes
   consume different numbers of extension words

### Why each other category doesn't need this

| Category | Why no specialization needed |
|----------|------------------------------|
| Register-only | No data memory access at all |
| Single read | One access, REG_PC state is deterministic |
| RMW (same address) | Write goes to same address as read — if read succeeds, write can't fault on alignment |
| Branches | No data write (or stack push in supervisor space) |
| System control | Register-only or system-space operations |

### MOVE's AERR-affected templates

Out of 107 MOVE templates, 17 have AERR-specific behavior:

| AERR concern | Templates | Mechanism |
|-------------|-----------|-----------|
| PC offset for pre-decrement dest (`pd`) | 6 | Inline `aerr_pc_offset = 2` |
| PC offset for absolute-long dest (`al`) | 2 | `M68KMAKE_WRITE_AL_*` token |
| Register restore for post-increment dest (`pi`) | 6 | Inline `aerr_restore_reg` |
| Register restore for pre-decrement dest (`pd`, 32-bit) | 3 | Inline `aerr_restore_reg` |

The **absolute-long dest** case (2 templates) is the only one that uses the
token substitution system, because it's the only case where the needed behavior
changes per source EA mode within a dot template.

All other AERR-affected MOVE templates use **specific** source modes (`d`, `a`),
not dot expansion — so they can set the offset inline.

---

## 7. Address Error (AERR) Mechanism

### The 68000 address error stack frame

When a word or longword access targets an odd address, the 68000 pushes a
14-byte stack frame:

```
Offset  Size  Content
0       4     PC value (instruction address at fault time)
4       2     Status Register (SR)
6       2     Instruction Register (IR, first opcode word)
8       4     Access address (the odd address)
12      2     Status word: [IR high bits | FC | R/W | I/N]
```

### How the stacked PC is computed

```c
// In m68kcpu.h — m68ki_stack_frame_buserr()
m68ki_push_32(REG_PC - 2 + m68ki_aerr_pc_offset);
m68ki_aerr_pc_offset = 0;  // reset after use
```

The default is `REG_PC - 2` (offset = 0), which is correct for most instructions
because REG_PC points to the first word of the next instruction, and subtracting
2 gives the last word of the current instruction.

### When the default is wrong

For MOVE with absolute-long destination and memory source:

```
MOVE.W  (A0), $ABCDEF00    ; A0 source (no extension), $ABCDEF00 dest (4-byte extension)

Execution:
  1. REG_PC = opcode_start + 2  (after opcode fetch)
  2. Source = (A0)              (no PC advancement — register indirect)
  3. EA_AL = read_imm_32()      (REG_PC += 4, now = opcode_start + 6)
  4. Write to $ABCDEF00         ← FAULT (odd address)

At fault: REG_PC = opcode_start + 6
Default stacked PC = REG_PC - 2 = opcode_start + 4  ← WRONG
Correct stacked PC = opcode_start                   ← points at the opcode itself
Needed offset = -4 from REG_PC-2 = -2 additional    → aerr_pc_offset = -2
```

For MOVE with absolute-long destination and immediate source:

```
MOVE.W  #$1234, $ABCDEF00   ; immediate source (2 bytes), dest (4-byte extension)

Execution:
  1. REG_PC = opcode_start + 2  (after opcode fetch)
  2. Source = read_imm_16()     (REG_PC += 2, now = opcode_start + 4)
  3. EA_AL = read_imm_32()      (REG_PC += 4, now = opcode_start + 8)
  4. Write to $ABCDEF00         ← FAULT (odd address)

At fault: REG_PC = opcode_start + 8
Default stacked PC = REG_PC - 2 = opcode_start + 6  ← WRONG
Correct stacked PC = opcode_start + 6               ← actually REG_PC-2 is correct!
Needed offset = 0                                  → default behavior is fine
```

The difference: immediate source consumes its extension words from the instruction
stream *before* the destination extension, so REG_PC is already fully advanced by
the time the write occurs. Memory source modes don't advance REG_PC (or advance
it less), leaving the destination extension words as the last PC-consuming step.

---

## 8. Three Approaches to Per-Mode Specialization

### Approach 1: Inline in template (MOVE pd, pi, al d/a)

For templates that target a **specific** EA mode, the offset is set inline:

```c
M68KMAKE_OP(move, 16, pd, d)
{
    uint res = MASK_OUT_ABOVE_16(DY);
    uint ea = EA_AX_PD_16();
    FLAG_N = NFLAG_16(res);
    FLAG_Z = res;
    FLAG_V = VFLAG_CLEAR;
    FLAG_C = CFLAG_CLEAR;
    m68ki_aerr_pc_offset = 2;
    m68ki_write_16(ea, res);
    m68ki_aerr_pc_offset = 0;
}
```

**Works when**: The template targets one specific EA mode (source is `d`, `a`, etc.).

**Limitation**: Cannot be used inside a `.` template because the offset would
apply to ALL expanded modes — some of which may need a different value.

**Coverage**: 15 of 17 AERR-affected MOVE templates (all use specific source modes).

### Approach 2: Post-generation Python patching (removed)

A Python script (`tools/patch_move_al_i.py`) ran after m68kmake to modify
specific generated functions in `m68kops.c`:

```python
# Removed — was fragile and order-sensitive
patch_move_al_i.py m68kops.c
```

The template had `aerr_pc_offset = -2` applied to ALL source modes, then the
patcher stripped it from the `_al_i` (immediate source) handlers.

**Why removed**:
- Fragile regex-based modifications on generated code
- Build order dependency (patcher must run after m68kmake, before compiler)
- Hidden logic — the template in `m68k_in.c` doesn't reflect actual behavior
- Would grow linearly as more instructions need patching

**Coverage**: Was used for 2 templates (`move 16/32 al .`).

### Approach 3: m68kmake token substitution (current, recommended)

Two new tokens (`M68KMAKE_WRITE_AL_16`, `M68KMAKE_WRITE_AL_32`) resolve to
different function calls depending on the EA mode being generated:

| EA mode | Token resolves to |
|---------|------------------|
| Memory source (AI through PCIX) | `m68ki_write_16_al_dest()` / `m68ki_write_32_al_dest()` |
| Immediate source (I) | `m68ki_write_16()` / `m68ki_write_32()` |

The wrapper functions (`m68ki_write_*_al_dest`) are inline functions in
`m68kcpu.h` that wrap the write with `aerr_pc_offset = -2`.

**Why this approach**:
- All behavior is expressed in C, not in external scripts
- The template in `m68k_in.c` shows intent: "use the AL write token"
- m68kmake resolves the token at generation time — zero runtime overhead
- Extensible: same pattern can add more tokens for other instructions
- No post-generation steps in the build

**Coverage**: 2 templates (`move 16/32 al .`). Replaces Approach 2 entirely.

### Summary: which approach for which template

| Template | Source modes | Approach | Why |
|----------|-------------|----------|-----|
| `move 16/32 pd, d` | `d` (data register) | Inline (Approach 1) | Specific source mode |
| `move 16/32 pd, a` | `a` (address register) | Inline (Approach 1) | Specific source mode |
| `move 16/32 pd, .` | All 10 modes | Inline (Approach 1) | Same offset for all modes |
| `move 16/32 pi, d/a/.` | Varies | Inline (Approach 1) | restore_reg, not offset |
| `move 16/32 al, .` | All 10 modes | **Token (Approach 3)** | Offset differs between memory and immediate source |
| `move 16/32 al, d` | `d` | None needed | Register source, no offset |
| `move 16/32 al, a` | `a` | None needed | Register source, no offset |

The `move al .` template is the **only** case where Approach 3 is needed —
the only case where the AERR offset must differ between expanded modes of a
dot template.

---

## 9. The MOVE al Case Study: Before and After

### The problem

MOVE.W with absolute-long destination generates handlers for 10 source EA modes
from one dot template. When the destination write faults on an odd address:

- **Memory source modes** (AI through PCIX): REG_PC has NOT consumed the source's
  extension words from the instruction stream (source data came from memory).
  The destination extension (4 bytes) has been consumed, but the overall REG_PC
  position is such that `REG_PC - 4` gives the correct stacked PC. Offset = `-2`
  on top of the default `-2`.

- **Immediate source** (I mode): REG_PC HAS consumed the immediate data AND the
  destination extension. `REG_PC - 2` is correct. Offset = `0`.

### Before: Python patcher (Approach 2)

```c
// m68k_in.c template — inline offset applied to ALL modes
M68KMAKE_OP(move, 16, al, .)
{
    ...
    m68ki_aerr_pc_offset = -2;       // ← wrong for immediate source
    m68ki_write_16(ea, res);
    m68ki_aerr_pc_offset = 0;
}
```
```bash
# Build step: patch m68kops.c after generation
python3 tools/patch_move_al_i.py m68kops.c    # strips offset from _al_i handlers
```

### After: Token substitution (Approach 3)

```c
// m68k_in.c template — token resolves per-mode at generation time
M68KMAKE_OP(move, 16, al, .)
{
    ...
    M68KMAKE_WRITE_AL_16(ea, res);   // ← resolves to wrapper or plain write
}
```

```c
// m68kcpu.h — inline wrapper (used for memory-source modes only)
static inline void m68ki_write_16_al_dest(uint ea, uint val)
{
    m68ki_aerr_pc_offset = -2;
    m68ki_write_16(ea, val);
    m68ki_aerr_pc_offset = 0;
}
```

```c
// m68kmake.c — token resolution in generate_opcode_handler()
if(ea_mode == EA_MODE_I)
    add_replace_string(replace, ID_OPHANDLER_WRITE_AL_16, "m68ki_write_16");
else
    add_replace_string(replace, ID_OPHANDLER_WRITE_AL_16, "m68ki_write_16_al_dest");
```

### Generated output

```
move_16_al_ai:   m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_pi:   m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_pd:   m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_di:   m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_ix:   m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_aw:   m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_al:   m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_pcdi: m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_pcix: m68ki_write_16_al_dest(ea, res)  // offset=-2 ✓
move_16_al_i:    m68ki_write_16(ea, res)           // no offset ✓
```

### How the issue was found

The bug was discovered through SST (SingleStepTests) comparing Musashi's output
against two reference test suites:

- **tomharte** (~1M test vectors) — caught the `move al i` case where the offset
  was incorrectly applied to immediate source
- **raddad** (~150K test vectors) — caught the `move al` memory-source case where
  the offset was needed but missing

The contradiction between the two suites on other instructions (ADDA, SUBA)
confirmed that the issue was specific to MOVE's two-address nature.

---

## 10. Extending the System

To add per-EA-mode behavior for a new instruction:

### Step 1: Define inline wrappers in `m68kcpu.h`

```c
static inline uint m68ki_read_32_with_offset(void)
{
    uint ea = EA_AY_32();
    m68ki_aerr_pc_offset = 2;
    uint val = m68ki_read_32(ea);
    m68ki_aerr_pc_offset = 0;
    return val;
}
```

### Step 2: Add tokens in `m68kmake.c`

```c
// Token ID
#define ID_OPHANDLER_READ_OFFSET_32 ID_BASE "_READ_OFFSET_32"

// Resolution in generate_opcode_handler()
if(ea_mode == EA_MODE_I)
    add_replace_string(replace, ID_OPHANDLER_READ_OFFSET_32, "OPER_I_32()");
else
    add_replace_string(replace, ID_OPHANDLER_READ_OFFSET_32, "m68ki_read_32_with_offset()");
```

### Step 3: Use token in `m68k_in.c` template

```c
M68KMAKE_OP(adda, 32, ., .)
{
    uint src = M68KMAKE_READ_OFFSET_32;   // ← new token
    uint* r_dst = &AX;
    *r_dst = MASK_OUT_ABOVE_32(*r_dst + src);
}
```

### Cost of adding a token

- ~3 lines in `m68kmake.c` (ID define + resolution logic)
- ~5 lines in `m68kcpu.h` (inline wrapper)
- No changes to existing templates unless they opt in
- Tokens not referenced by a template are harmlessly added to the substitution
  table but never matched — zero impact on generated code

---

## 11. Reference: Key Numbers

| Metric | Value |
|--------|-------|
| 68000 ISA instruction mnemonics (base CPU) | ~56 |
| Total instruction mnemonics (68000–68060) | 115 |
| Template definitions (`M68KMAKE_OP`) | 523 |
| Generated handler functions | 1,967 |
| Table entries | 518 |
| Entries with dot EA (`.`) | 305 |
| Specific EA entries | 213 |
| EA modes (including PI7, PD7) | 13 |
| m68kmake substitution tokens | 9 (7 original + 2 AERR) |
| AERR-related inline wrappers | 2 |
| MOVE templates total | 107 |
| MOVE templates with AERR behavior | 17 |
| MOVE templates using token substitution | 2 |
| Lines in generated m68kops.c | ~38,000 |
| ISA categories needing per-mode AERR | 1 of 6 (only MOVE) |
