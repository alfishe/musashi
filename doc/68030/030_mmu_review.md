# 030 MMU Implementation Review

This document summarizes the progress against the `task.md` and `mmu_implementation_plan.md` for the 68030 MMU implementation.

## ✅ Covered (Phases 1 & 2 Core)

The core translation, infrastructure, and instructions have been successfully implemented:

*   **ATC Infrastructure:** `mmu_atc_tag`, `mmu_atc_data`, and TT0/TT1 registers added to `m68ki_cpu_core` and cleared on reset.
*   **Translation & TT Matching:** `pmmu_match_tt` correctly handles TT0/TT1 overrides. Table walking correctly threads the function code (FC) through `pmmu_translate_addr_with_fc`.
*   **Instruction Emulation:** `PFLUSH`, `PTEST`, and `PLOAD` have been successfully ported from MAME to Musashi. PMOVE for TT0/TT1 is implemented.
*   **MMU Walking details:** `pmmu_update_descriptor` correctly updates U/M bits. `pmmu_update_sr` populates the MMUSR correctly.
*   **Bus Error Triggering:** `pmmu_set_buserror()` replaces fatal error aborts.

## ❌ Missing / Incomplete

However, there are several key items from Milestone 1 (Phases 1, 2, 5.1, and 6) that are missing or incomplete:


### 1. Phase 2: TC Register Validation
*   **Missing:** The PMOVE instruction handler does not validate the Translation Control (TC) register.
*   **Required:** The sum of the Page Size (PS) and Initial Shift (IS) bits, plus the Table Index (TIA, TIB, TIC, TID) bits must equal 32. If not, writing to TC should throw a configuration error (or just bus error, depending on spec).

### 2. Phase 5.1: 030 Bus Error Frames
*   **Missing:** `m68ki_exception_bus_error()` in `m68kcpu.h` still hardcodes the 68010 Format $8 (29-word) stack frame (`m68ki_stack_frame_1000`).
*   **Required:** It must branch based on `CPU_TYPE` and build Format $A (short) and Format $B (long, 46-word) frames with correct SSW, Fault Address, and padded internal pipeline words.

### 3. Phase 1: ATC Replacement Policy - Not designed properly yet!
*   **Missing:** The replacement policy is hardcoded to Round-Robin in `pmmu_atc_add()`. In real 68030 hardware, the 22-entry ATC uses a pseudo-LRU (Least Recently Used) replacement algorithm.
*   **Required:** A pseudo-LRU default implementation should be provided for standard emulation.
*   **Detailed PLRU Algorithm (Bit-PLRU / MRU):**
    1. Maintain a 22-bit History Register (one bit per ATC entry), initialized to all `0`s.
    2. On every ATC access (both hits and fills), set the accessed entry's corresponding history bit to `1`.
    3. If setting this bit would cause all 22 bits to become `1`, instead set *only* the currently accessed entry's bit to `1`, and clear the other 21 bits to `0`.
    4. When the ATC is full and an eviction is required, scan the History Register (e.g., from index 0 to 21) and select the first entry whose history bit is `0` as the victim.

    ```mermaid
    flowchart TD
        A[Translation Requested] --> B{Hits in ATC?}
        B -- Yes (Hit) --> C["Locate Hit Entry N"]
        B -- No (Miss) --> D{ATC Full?}
        D -- No --> E["Fill empty Entry N"]
        D -- Yes --> F["Find lowest index N where History[N] == 0"]
        F --> G["Evict Entry N, Fill new descriptor"]
        
        C --> H
        E --> H
        G --> H
        
        H["Set History[N] = 1"] --> I{"Are all 22 bits == 1?"}
        I -- Yes --> J["Clear all bits except History[N]"]
        I -- No --> K[Done]
    ```

    ```mermaid
    stateDiagram-v2
        state "Initial State" as s1
        state "After Accessing Entry 3" as s2
        state "History Full (21 bits set)" as s3
        state "Accessing last 0 bit (Entry 7)" as s4
        
        s1: History = [0,0,0,0...0]
        s2: History = [0,0,0,1...0]
        s3: History = [1,1,1,1,1,1,1,0,1...1]
        s4: History = [0,0,0,0,0,0,0,1,0...0]
        
        s1 --> s2 : Access Entry 3
        s2 --> s3 : Access all other entries except 7
        s3 --> s4 : Access Entry 7\n(All would be 1, so clear others)
    ```

### 4. Phase 1: Co-Simulation Replacement Hook (Optional)
*   **Missing:** No mechanism exists for an external testbench to dictate ATC replacement choices.
*   **Required (Optional):** A callback hook (e.g., `m68k_atc_replace_callback`) can optionally be implemented to bypass the pseudo-LRU algorithm. This allows the co-simulation testbench to orchestrate deterministic evictions and ensure 100% sync with the FPGA RTL during tests that thrash the ATC.

---

## ⚠️ Additional Defects Found in Code Review

### 5. Read paths do not check `mmu_tmp_buserror_occurred`
*   **Location:** `m68ki_read_8_fc`, `m68ki_read_16_fc`, `m68ki_read_32_fc` in `m68kcpu.h`
*   **Bug:** After calling `pmmu_translate_addr()`, the write-path functions correctly check `mmu_tmp_buserror_occurred` and call `m68ki_exception_bus_error()`. The read-path functions do **not** — they return the translated address (or garbage) and continue the memory access silently on a fault.
*   **Required:** All read FC functions must check `mmu_tmp_buserror_occurred` after translation and raise `m68ki_exception_bus_error()` in the same way the write paths do.

### 6. `mmu_tmp_buserror_occurred` never cleared on read paths
*   **Related to #5:** Even if a read translation faults, `mmu_tmp_buserror_occurred` is left non-zero. This means a subsequent write on the same instruction (e.g. a RMW instruction) will trigger a *second*, spurious bus error. The counter must be cleared after each translation attempt.

### 7. Instruction-fetch path bypasses MMU translation
*   **Location:** `m68ki_read_imm_16()` and `m68ki_read_imm_32()` in `m68kcpu.h`
*   **Bug:** The `M68K_EMULATE_PMMU` guard block calls `pmmu_translate_addr(address)` but `address` is **undefined** in this context (the non-`M68K_SEPARATE_READS` path uses `REG_PC` directly). The translation is effectively a no-op and instruction fetches are never translated through the MMU.
*   **Required:** The PC-relative fetch should set `mmu_tmp_fc` / `mmu_tmp_rw` and translate `REG_PC` before fetching, or the `M68K_SEPARATE_READS` path must be made the mandatory path when PMMU is enabled.

### 8. Debug `fprintf` left in 040 translation path
*   **Location:** `pmmu_translate_addr_with_fc_040()` in `m68kmmu.h`, lines ~878–929
*   **Bug:** There are unconditional `fprintf(stderr, ...)` calls with a `static int debug_count` limiter still present in the production code path. These will fire during normal emulation whenever the 040 MMU is active, producing noise on stderr and potentially slowing execution.
*   **Required:** Remove or gate these behind a `#if defined(M68K_MMU_DEBUG)` compile-time flag.

### 9. `pmmu_atc_flush()` does not reset the PLRU history
*   **Location:** `pmmu_atc_flush()` in `m68kmmu.h`
*   **Bug:** `pmmu_atc_flush()` only clears the `mmu_atc_tag[]` entries and resets the round-robin counter `mmu_atc_rr`. When PLRU is implemented, the history register (the 22-bit `mmu_atc_history` field) must also be zeroed on every flush. Failing to do so will cause the PLRU to select incorrect victims after a `PFLUSHA`.
*   **Required:** Add `m68ki_cpu.mmu_atc_history = 0;` to `pmmu_atc_flush()` and also on reset in `m68k_pulse_reset()`.