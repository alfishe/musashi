/*
 * sst_loader.h — Unified SST binary format loader
 *
 * Reads .sst files produced by tools/sst_convert.py from:
 *   - SingleStepTests/680x0  (TomHarte, ~1M vectors)
 *   - SingleStepTests/m68000 (raddad772, ~100K vectors)
 *
 * Both sources are normalized to the same binary layout.
 */
#ifndef SST_LOADER_H
#define SST_LOADER_H

/**
 * sst_loader.h
 *
 * Compact loader for .sst (SingleStepTests) binary format.
 * 
 * NOTE: The .sst format uses HOST ENDIANNESS for speed. If moving data
 * between machines with different architectures, you must re-run 
 * tools/sst_convert.py.
 */

#include <stdint.h>

#define SST_MAX_RAM      512
#define SST_MAX_NAME      64
#define SST_MAX_MNEMONIC  32
#define SST_NUM_REGS      19   /* D0-D7, A0-A6, USP, SSP, SR, PC */

#define SST_SOURCE_TOMHARTE  0
#define SST_SOURCE_RADDAD    1

typedef struct {
    uint32_t addr;
    uint8_t  value;
} sst_ram_entry_t;

typedef struct {
    uint32_t regs[SST_NUM_REGS];      /* D0-D7, A0-A6, USP, SSP, SR, PC */
    sst_ram_entry_t ram[SST_MAX_RAM];
    int num_ram;
} sst_cpu_state_t;

typedef struct {
    char name[SST_MAX_NAME];
    sst_cpu_state_t initial;
    sst_cpu_state_t final_state;
} sst_vector_t;

typedef struct {
    char mnemonic[SST_MAX_MNEMONIC];
    uint8_t source_id;                 /* SST_SOURCE_TOMHARTE or SST_SOURCE_RADDAD */
    int num_vectors;
    sst_vector_t *vectors;             /* heap-allocated array */
} sst_test_file_t;

/* Load a .sst file. Returns NULL on failure. Caller must sst_free(). */
sst_test_file_t *sst_load(const char *filepath);

/* Free a loaded test file */
void sst_free(sst_test_file_t *tf);

/* Return source name string */
const char *sst_source_name(uint8_t source_id);

#endif /* SST_LOADER_H */
