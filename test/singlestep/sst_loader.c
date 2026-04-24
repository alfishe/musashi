/*
 * sst_loader.c — Unified SST binary format loader implementation
 */
#include "sst_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *sst_source_name(uint8_t source_id) {
    switch (source_id) {
        case SST_SOURCE_TOMHARTE: return "tomharte";
        case SST_SOURCE_RADDAD:   return "raddad";
        default:                  return "unknown";
    }
}

sst_test_file_t *sst_load(const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;

    /* Read magic */
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "SST1", 4) != 0) {
        fclose(f);
        return NULL;
    }

    sst_test_file_t *tf = calloc(1, sizeof(sst_test_file_t));
    if (!tf) { fclose(f); return NULL; }

    /* Read header */
    uint32_t num_vectors;
    uint8_t source_id, name_len;
    fread(&num_vectors, 4, 1, f);
    fread(&source_id, 1, 1, f);
    fread(&name_len, 1, 1, f);

    tf->source_id = source_id;
    tf->num_vectors = (int)num_vectors;

    if (name_len >= SST_MAX_MNEMONIC) name_len = SST_MAX_MNEMONIC - 1;
    fread(tf->mnemonic, 1, name_len, f);
    tf->mnemonic[name_len] = '\0';

    /* Allocate vectors */
    tf->vectors = calloc(num_vectors, sizeof(sst_vector_t));
    if (!tf->vectors) {
        free(tf);
        fclose(f);
        return NULL;
    }

    for (uint32_t i = 0; i < num_vectors; i++) {
        sst_vector_t *vec = &tf->vectors[i];

        /* Vector name */
        uint8_t vname_len;
        fread(&vname_len, 1, 1, f);
        if (vname_len >= SST_MAX_NAME) vname_len = SST_MAX_NAME - 1;
        fread(vec->name, 1, vname_len, f);
        vec->name[vname_len] = '\0';

        /* Initial registers */
        fread(vec->initial.regs, 4, SST_NUM_REGS, f);
        /* Final registers */
        fread(vec->final_state.regs, 4, SST_NUM_REGS, f);

        /* RAM counts */
        uint16_t num_ram_initial, num_ram_final;
        fread(&num_ram_initial, 2, 1, f);
        fread(&num_ram_final, 2, 1, f);

        /* Initial RAM */
        vec->initial.num_ram = (num_ram_initial < SST_MAX_RAM) ? num_ram_initial : SST_MAX_RAM;
        for (int j = 0; j < num_ram_initial; j++) {
            uint32_t addr;
            uint8_t val;
            fread(&addr, 4, 1, f);
            fread(&val, 1, 1, f);
            if (j < SST_MAX_RAM) {
                vec->initial.ram[j].addr = addr;
                vec->initial.ram[j].value = val;
            }
        }

        /* Final RAM */
        vec->final_state.num_ram = (num_ram_final < SST_MAX_RAM) ? num_ram_final : SST_MAX_RAM;
        for (int j = 0; j < num_ram_final; j++) {
            uint32_t addr;
            uint8_t val;
            fread(&addr, 4, 1, f);
            fread(&val, 1, 1, f);
            if (j < SST_MAX_RAM) {
                vec->final_state.ram[j].addr = addr;
                vec->final_state.ram[j].value = val;
            }
        }
    }

    fclose(f);
    return tf;
}

void sst_free(sst_test_file_t *tf) {
    if (tf) {
        free(tf->vectors);
        free(tf);
    }
}
