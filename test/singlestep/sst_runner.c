/*
 * sst_runner.c — SingleStepTests harness for Musashi
 *
 * Loads .sst files, runs each vector against Musashi, compares results.
 * Produces console output and reports in markdown, HTML, JSON, YAML.
 *
 * Build: see Makefile target 'sst_runner'
 */
#include "sst_loader.h"
#include "m68k.h"
#include "m68kcpu.h"  /* For direct access to m68ki_cpu internals (stopped, run_mode) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

/* Wall-clock helper — returns seconds since epoch as double */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static double g_run_start = 0.0;  /* set in main before first file */
static double g_run_end   = 0.0;  /* set after last file */

/* ------------------------------------------------------------------ */
/* Flat 16MB memory for SST tests                                     */
/* ------------------------------------------------------------------ */
#define MEM_SIZE (16 * 1024 * 1024)
static uint8_t g_mem[MEM_SIZE];

unsigned int m68k_read_memory_8(unsigned int addr)  { return g_mem[addr & 0xFFFFFF]; }
unsigned int m68k_read_memory_16(unsigned int addr) {
    addr &= 0xFFFFFF;
    return (g_mem[addr] << 8) | g_mem[addr + 1];
}
unsigned int m68k_read_memory_32(unsigned int addr) {
    addr &= 0xFFFFFF;
    return (g_mem[addr] << 24) | (g_mem[addr+1] << 16) |
           (g_mem[addr+2] << 8) | g_mem[addr+3];
}
void m68k_write_memory_8(unsigned int addr, unsigned int val)  { g_mem[addr & 0xFFFFFF] = val; }
void m68k_write_memory_16(unsigned int addr, unsigned int val) {
    addr &= 0xFFFFFF;
    g_mem[addr] = (val >> 8) & 0xFF;
    g_mem[addr+1] = val & 0xFF;
}
void m68k_write_memory_32(unsigned int addr, unsigned int val) {
    addr &= 0xFFFFFF;
    g_mem[addr]   = (val >> 24) & 0xFF;
    g_mem[addr+1] = (val >> 16) & 0xFF;
    g_mem[addr+2] = (val >> 8)  & 0xFF;
    g_mem[addr+3] = val & 0xFF;
}
unsigned int m68k_read_disassembler_16(unsigned int addr) { return m68k_read_memory_16(addr); }
unsigned int m68k_read_disassembler_32(unsigned int addr) { return m68k_read_memory_32(addr); }

/* ------------------------------------------------------------------ */
/* Register names                                                     */
/* ------------------------------------------------------------------ */
static const char *g_reg_names[SST_NUM_REGS] = {
    "D0","D1","D2","D3","D4","D5","D6","D7",
    "A0","A1","A2","A3","A4","A5","A6",
    "USP","SSP","SR","PC"
};
static const m68k_register_t g_reg_ids[SST_NUM_REGS] = {
    M68K_REG_D0, M68K_REG_D1, M68K_REG_D2, M68K_REG_D3,
    M68K_REG_D4, M68K_REG_D5, M68K_REG_D6, M68K_REG_D7,
    M68K_REG_A0, M68K_REG_A1, M68K_REG_A2, M68K_REG_A3,
    M68K_REG_A4, M68K_REG_A5, M68K_REG_A6,
    M68K_REG_USP, M68K_REG_ISP, M68K_REG_SR, M68K_REG_PC
};

/* ------------------------------------------------------------------ */
/* Per-file result tracking                                           */
/* ------------------------------------------------------------------ */
#define MAX_FAILS_DETAIL 10

typedef struct {
    char detail[256];
} fail_detail_t;

typedef struct {
    char mnemonic[SST_MAX_MNEMONIC];
    char source[16];
    int total;
    int passed;
    int failed;
    fail_detail_t first_fails[MAX_FAILS_DETAIL];
    int num_details;
    double elapsed_sec;   /* wall-clock time for this file */
} file_result_t;

#define MAX_FILE_RESULTS 300
static file_result_t g_results[MAX_FILE_RESULTS];
static int g_num_results = 0;

/* ------------------------------------------------------------------ */
/* Run one vector                                                     */
/* ------------------------------------------------------------------ */
static int run_vector(const sst_vector_t *vec, file_result_t *res, int verbose) {
    /* Clear memory */
    memset(g_mem, 0, MEM_SIZE);

    /* Load initial RAM */
    for (int i = 0; i < vec->initial.num_ram; i++) {
        g_mem[vec->initial.ram[i].addr & 0xFFFFFF] = vec->initial.ram[i].value;
    }

    /* Clear stopped/halted state from prior vectors (e.g. STOP instruction).
     * We don't use pulse_reset here because it imposes a 132-cycle reset
     * penalty that would consume our entire execution budget. */
    m68ki_cpu.stopped = 0;
    m68ki_cpu.run_mode = RUN_MODE_NORMAL;

    /* Set initial registers. Order matters due to stack pointer aliasing:
     * 1. SR first (establishes supervisor/user mode, affects A7 aliasing)
     * 2. Data and address registers
     * 3. USP and SSP (must be set after SR to properly alias)
     * 4. PC last (triggers prefetch reload via m68ki_jump) */
    m68k_set_reg(M68K_REG_SR, vec->initial.regs[17]);

    /* D0-D7 (indices 0-7) */
    for (int i = 0; i < 8; i++)
        m68k_set_reg(g_reg_ids[i], vec->initial.regs[i]);
    /* A0-A6 (indices 8-14) */
    for (int i = 8; i < 15; i++)
        m68k_set_reg(g_reg_ids[i], vec->initial.regs[i]);
    /* USP (15), SSP (16) */
    m68k_set_reg(M68K_REG_USP, vec->initial.regs[15]);
    m68k_set_reg(M68K_REG_ISP, vec->initial.regs[16]);
    /* PC (18) — this calls m68ki_jump() internally to set up prefetch */
    m68k_set_reg(M68K_REG_PC, vec->initial.regs[18]);

    /* Execute one instruction. Budget of 1 cycle — Musashi always finishes
     * the current instruction even when the budget is exceeded. */
    int cycles_used = m68k_execute(1);
    (void)cycles_used;  /* available for future cycle-accuracy reporting */

    /* Compare final registers */
    int ok = 1;
    for (int i = 0; i < SST_NUM_REGS; i++) {
        uint32_t expected = vec->final_state.regs[i];
        uint32_t actual = m68k_get_reg(NULL, g_reg_ids[i]);

        /* SR: mask to 16 bits */
        if (i == 17) { expected &= 0xFFFF; actual &= 0xFFFF; }

        if (actual != expected) {
            ok = 0;
            if (res->num_details < MAX_FAILS_DETAIL) {
                snprintf(res->first_fails[res->num_details].detail,
                         sizeof(res->first_fails[0].detail),
                         "%s: %s %s exp=0x%08X got=0x%08X",
                         vec->name, res->mnemonic, g_reg_names[i],
                         expected, actual);
                res->num_details++;
            }
            if (verbose) {
                printf("    FAIL %s: %s expected=0x%08X got=0x%08X\n",
                       vec->name, g_reg_names[i], expected, actual);
            }
            break; /* report first mismatch only */
        }
    }

    /* Compare final RAM */
    if (ok) {
        for (int i = 0; i < vec->final_state.num_ram; i++) {
            uint32_t addr = vec->final_state.ram[i].addr & 0xFFFFFF;
            uint8_t expected = vec->final_state.ram[i].value;
            uint8_t actual = g_mem[addr];
            if (actual != expected) {
                ok = 0;
                if (res->num_details < MAX_FAILS_DETAIL) {
                    snprintf(res->first_fails[res->num_details].detail,
                             sizeof(res->first_fails[0].detail),
                             "%s: %s RAM@0x%06X exp=0x%02X got=0x%02X",
                             vec->name, res->mnemonic, addr, expected, actual);
                    res->num_details++;
                }
                if (verbose) {
                    printf("    FAIL %s: RAM@0x%06X expected=0x%02X got=0x%02X\n",
                           vec->name, addr, expected, actual);
                }
                break;
            }
        }
    }

    return ok;
}

/* ------------------------------------------------------------------ */
/* Run one .sst file                                                  */
/* ------------------------------------------------------------------ */
static void run_file(const char *filepath, int max_vectors, int verbose,
                     int stop_on_fail) {
    sst_test_file_t *tf = sst_load(filepath);
    if (!tf) {
        fprintf(stderr, "ERROR: cannot load %s\n", filepath);
        return;
    }

    if (g_num_results >= MAX_FILE_RESULTS) {
        fprintf(stderr, "ERROR: too many files\n");
        sst_free(tf);
        return;
    }

    file_result_t *res = &g_results[g_num_results++];
    memset(res, 0, sizeof(*res));
    strncpy(res->mnemonic, tf->mnemonic, SST_MAX_MNEMONIC - 1);
    strncpy(res->source, sst_source_name(tf->source_id), sizeof(res->source) - 1);

    int count = tf->num_vectors;
    if (max_vectors > 0 && count > max_vectors) count = max_vectors;
    res->total = count;

    double t0 = now_sec();
    for (int i = 0; i < count; i++) {
        if (run_vector(&tf->vectors[i], res, verbose))
            res->passed++;
        else {
            res->failed++;
            if (stop_on_fail) break;
        }
    }
    res->elapsed_sec = now_sec() - t0;

    /* Console progress line */
    const char *status = (res->failed == 0) ? "PASS" : "FAIL";
    printf("[%-8s] %-20s %5d/%5d %s  (%.3fs)\n",
           res->source, res->mnemonic, res->passed, res->total, status,
           res->elapsed_sec);

    sst_free(tf);
}

/* ------------------------------------------------------------------ */
/* Scan directory for .sst files                                      */
/* ------------------------------------------------------------------ */
static void scan_dir(const char *dirpath, int max_vectors, int verbose,
                     int stop_on_fail) {
    DIR *d = opendir(dirpath);
    if (!d) return;

    struct dirent *ent;
    /* Collect filenames and sort */
    char paths[300][512];
    int n = 0;
    while ((ent = readdir(d)) != NULL && n < 300) {
        size_t len = strlen(ent->d_name);
        if (len > 4 && strcmp(ent->d_name + len - 4, ".sst") == 0) {
            snprintf(paths[n], sizeof(paths[0]), "%s/%s", dirpath, ent->d_name);
            n++;
        }
    }
    closedir(d);

    /* Simple sort */
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (strcmp(paths[i], paths[j]) > 0) {
                char tmp[512];
                strcpy(tmp, paths[i]);
                strcpy(paths[i], paths[j]);
                strcpy(paths[j], tmp);
            }

    for (int i = 0; i < n; i++)
        run_file(paths[i], max_vectors, verbose, stop_on_fail);
}

/* ------------------------------------------------------------------ */
/* Report writers                                                     */
/* ------------------------------------------------------------------ */
static void compute_totals(int *total_files, int *total_pass, int *total_fail,
                           int *total_vectors) {
    *total_files = g_num_results;
    *total_pass = 0; *total_fail = 0; *total_vectors = 0;
    for (int i = 0; i < g_num_results; i++) {
        *total_vectors += g_results[i].total;
        *total_pass += g_results[i].passed;
        *total_fail += g_results[i].failed;
    }
}

typedef struct {
    double total_sec;    /* sum of per-file elapsed */
    double wall_sec;     /* overall wall clock (start→end) */
    double min_sec;      /* fastest file */
    double max_sec;      /* slowest file */
    double avg_sec;      /* mean per file */
    double vecs_per_sec; /* throughput based on wall time */
    char   min_mnemonic[SST_MAX_MNEMONIC];
    char   max_mnemonic[SST_MAX_MNEMONIC];
} timing_stats_t;

static void compute_timing(timing_stats_t *ts, int total_vectors) {
    ts->wall_sec = (g_run_end > 0.0) ? (g_run_end - g_run_start) : 0.0;
    ts->total_sec = 0.0;
    ts->min_sec = 1e30; ts->max_sec = 0.0;
    ts->min_mnemonic[0] = ts->max_mnemonic[0] = '\0';
    for (int i = 0; i < g_num_results; i++) {
        double e = g_results[i].elapsed_sec;
        ts->total_sec += e;
        if (e < ts->min_sec) {
            ts->min_sec = e;
            strncpy(ts->min_mnemonic, g_results[i].mnemonic, SST_MAX_MNEMONIC-1);
        }
        if (e > ts->max_sec) {
            ts->max_sec = e;
            strncpy(ts->max_mnemonic, g_results[i].mnemonic, SST_MAX_MNEMONIC-1);
        }
    }
    ts->avg_sec = g_num_results > 0 ? ts->total_sec / g_num_results : 0.0;
    ts->vecs_per_sec = ts->wall_sec > 0.0 ? total_vectors / ts->wall_sec : 0.0;
    if (ts->min_sec > 1e29) ts->min_sec = 0.0;
}

static void print_summary(void) {
    int tf, tp, tfail, tv;
    compute_totals(&tf, &tp, &tfail, &tv);
    int files_pass = 0;
    for (int i = 0; i < g_num_results; i++)
        if (g_results[i].failed == 0) files_pass++;

    timing_stats_t ts;
    compute_timing(&ts, tv);

    printf("\n=== Summary ===\n");
    printf("Files:   %d/%d passed\n", files_pass, tf);
    printf("Vectors: %d/%d passed", tp, tv);
    if (tfail > 0) printf(" (%d FAILED)", tfail);
    printf("\n");
    printf("\n=== Timing ===\n");
    printf("Total wall time : %.3f s\n", ts.wall_sec);
    printf("Throughput      : %.0f vectors/s\n", ts.vecs_per_sec);
    printf("Per-file avg    : %.3f s\n", ts.avg_sec);
    printf("Fastest file    : %.3f s  (%s)\n", ts.min_sec, ts.min_mnemonic);
    printf("Slowest file    : %.3f s  (%s)\n", ts.max_sec, ts.max_mnemonic);
}

static void write_report_json(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    int tf, tp, tfail, tv;
    compute_totals(&tf, &tp, &tfail, &tv);

    timing_stats_t ts;
    compute_timing(&ts, tv);

    fprintf(f, "{\n  \"summary\": {\n");
    fprintf(f, "    \"total_files\": %d,\n", tf);
    fprintf(f, "    \"total_vectors\": %d,\n", tv);
    fprintf(f, "    \"passed\": %d,\n", tp);
    fprintf(f, "    \"failed\": %d\n  },\n", tfail);
    fprintf(f, "  \"timing\": {\n");
    fprintf(f, "    \"wall_sec\": %.3f,\n", ts.wall_sec);
    fprintf(f, "    \"vectors_per_sec\": %.0f,\n", ts.vecs_per_sec);
    fprintf(f, "    \"per_file_avg_sec\": %.3f,\n", ts.avg_sec);
    fprintf(f, "    \"fastest_sec\": %.3f,\n", ts.min_sec);
    fprintf(f, "    \"fastest_file\": \"%s\",\n", ts.min_mnemonic);
    fprintf(f, "    \"slowest_sec\": %.3f,\n", ts.max_sec);
    fprintf(f, "    \"slowest_file\": \"%s\"\n  },\n", ts.max_mnemonic);
    fprintf(f, "  \"files\": [\n");
    for (int i = 0; i < g_num_results; i++) {
        file_result_t *r = &g_results[i];
        fprintf(f, "    {\"source\":\"%s\",\"mnemonic\":\"%s\","
                "\"total\":%d,\"passed\":%d,\"failed\":%d,\"elapsed_sec\":%.4f",
                r->source, r->mnemonic, r->total, r->passed, r->failed, r->elapsed_sec);
        if (r->num_details > 0) {
            fprintf(f, ",\"first_failures\":[");
            for (int j = 0; j < r->num_details; j++) {
                fprintf(f, "\"%s\"%s", r->first_fails[j].detail,
                        j < r->num_details - 1 ? "," : "");
            }
            fprintf(f, "]");
        }
        fprintf(f, "}%s\n", i < g_num_results - 1 ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
    printf("JSON report: %s\n", path);
}

static void write_report_yaml(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    int tf, tp, tfail, tv;
    compute_totals(&tf, &tp, &tfail, &tv);

    timing_stats_t ts;
    compute_timing(&ts, tv);

    fprintf(f, "summary:\n  total_files: %d\n  total_vectors: %d\n", tf, tv);
    fprintf(f, "  passed: %d\n  failed: %d\n\n", tp, tfail);
    fprintf(f, "timing:\n");
    fprintf(f, "  wall_sec: %.3f\n", ts.wall_sec);
    fprintf(f, "  vectors_per_sec: %.0f\n", ts.vecs_per_sec);
    fprintf(f, "  per_file_avg_sec: %.3f\n", ts.avg_sec);
    fprintf(f, "  fastest_sec: %.3f\n", ts.min_sec);
    fprintf(f, "  fastest_file: %s\n", ts.min_mnemonic);
    fprintf(f, "  slowest_sec: %.3f\n", ts.max_sec);
    fprintf(f, "  slowest_file: %s\n\n", ts.max_mnemonic);
    fprintf(f, "files:\n");
    for (int i = 0; i < g_num_results; i++) {
        file_result_t *r = &g_results[i];
        fprintf(f, "  - source: %s\n    mnemonic: %s\n", r->source, r->mnemonic);
        fprintf(f, "    total: %d\n    passed: %d\n    failed: %d\n    elapsed_sec: %.4f\n",
                r->total, r->passed, r->failed, r->elapsed_sec);
        if (r->num_details > 0) {
            fprintf(f, "    first_failures:\n");
            for (int j = 0; j < r->num_details; j++)
                fprintf(f, "      - \"%s\"\n", r->first_fails[j].detail);
        }
    }
    fclose(f);
    printf("YAML report: %s\n", path);
}

static void write_report_markdown(const char *path, int brief) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    int tf, tp, tfail, tv;
    compute_totals(&tf, &tp, &tfail, &tv);
    int files_pass = 0;
    for (int i = 0; i < g_num_results; i++)
        if (g_results[i].failed == 0) files_pass++;

    timing_stats_t ts;
    compute_timing(&ts, tv);

    fprintf(f, "# SST Test Report\n\n");
    fprintf(f, "| Metric | Value |\n|--------|------:|\n");
    fprintf(f, "| Files | %d/%d passed |\n", files_pass, tf);
    fprintf(f, "| Vectors | %d/%d passed |\n", tp, tv);
    if (tfail > 0) fprintf(f, "| **Failures** | **%d** |\n", tfail);
    fprintf(f, "\n## Timing\n\n");
    fprintf(f, "| Metric | Value |\n|--------|------:|\n");
    fprintf(f, "| Wall time | %.3f s |\n", ts.wall_sec);
    fprintf(f, "| Throughput | %.0f vec/s |\n", ts.vecs_per_sec);
    fprintf(f, "| Per-file avg | %.3f s |\n", ts.avg_sec);
    fprintf(f, "| Fastest | %.3f s (%s) |\n", ts.min_sec, ts.min_mnemonic);
    fprintf(f, "| Slowest | %.3f s (%s) |\n", ts.max_sec, ts.max_mnemonic);
    fprintf(f, "\n## Per-File Results\n\n");
    fprintf(f, "| Source | Mnemonic | Pass | Total | Status | Time (s) |\n");
    fprintf(f, "|--------|----------|-----:|------:|--------|--------:|\n");
    for (int i = 0; i < g_num_results; i++) {
        file_result_t *r = &g_results[i];
        fprintf(f, "| %s | %s | %d | %d | %s | %.3f |\n",
                r->source, r->mnemonic, r->passed, r->total,
                r->failed == 0 ? "✅" : "❌", r->elapsed_sec);
    }
    if (!brief && tfail > 0) {
        fprintf(f, "\n## Failure Details\n\n");
        for (int i = 0; i < g_num_results; i++) {
            file_result_t *r = &g_results[i];
            if (r->num_details > 0) {
                fprintf(f, "### %s (%s)\n\n", r->mnemonic, r->source);
                for (int j = 0; j < r->num_details; j++)
                    fprintf(f, "- `%s`\n", r->first_fails[j].detail);
                fprintf(f, "\n");
            }
        }
    }
    fclose(f);
    printf("Markdown report: %s\n", path);
}

static void write_report_html(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    int tf, tp, tfail, tv;
    compute_totals(&tf, &tp, &tfail, &tv);
    int files_pass = 0;
    for (int i = 0; i < g_num_results; i++)
        if (g_results[i].failed == 0) files_pass++;

    timing_stats_t ts;
    compute_timing(&ts, tv);

    fprintf(f, "<!DOCTYPE html><html><head><meta charset=\"utf-8\">\n");
    fprintf(f, "<title>SST Report</title>\n");
    fprintf(f, "<style>body{font-family:monospace;margin:2em}"
            "table{border-collapse:collapse}td,th{border:1px solid #ccc;"
            "padding:4px 8px}th{background:#f0f0f0}.pass{color:green}"
            ".fail{color:red;font-weight:bold}.num{text-align:right}"
            ".timing td{background:#f8f8f8}</style></head><body>\n");
    fprintf(f, "<h1>SST Test Report</h1>\n");
    fprintf(f, "<p>Files: %d/%d passed | Vectors: %d/%d passed", files_pass, tf, tp, tv);
    if (tfail > 0) fprintf(f, " | <span class=\"fail\">%d FAILED</span>", tfail);
    fprintf(f, "</p>\n");
    fprintf(f, "<h2>Timing</h2>\n<table class=\"timing\">\n");
    fprintf(f, "<tr><th>Metric</th><th>Value</th></tr>\n");
    fprintf(f, "<tr><td>Wall time</td><td class=\"num\">%.3f s</td></tr>\n", ts.wall_sec);
    fprintf(f, "<tr><td>Throughput</td><td class=\"num\">%.0f vec/s</td></tr>\n", ts.vecs_per_sec);
    fprintf(f, "<tr><td>Per-file avg</td><td class=\"num\">%.3f s</td></tr>\n", ts.avg_sec);
    fprintf(f, "<tr><td>Fastest file</td><td class=\"num\">%.3f s &nbsp; %s</td></tr>\n",
            ts.min_sec, ts.min_mnemonic);
    fprintf(f, "<tr><td>Slowest file</td><td class=\"num\">%.3f s &nbsp; %s</td></tr>\n",
            ts.max_sec, ts.max_mnemonic);
    fprintf(f, "</table>\n");
    fprintf(f, "<h2>Per-File Results</h2>\n"
            "<table><tr><th>Source</th><th>Mnemonic</th>"
            "<th>Pass</th><th>Total</th><th>Status</th><th>Time (s)</th></tr>\n");
    for (int i = 0; i < g_num_results; i++) {
        file_result_t *r = &g_results[i];
        fprintf(f, "<tr><td>%s</td><td>%s</td><td class=\"num\">%d</td>"
                "<td class=\"num\">%d</td><td class=\"%s\">%s</td>"
                "<td class=\"num\">%.3f</td></tr>\n",
                r->source, r->mnemonic, r->passed, r->total,
                r->failed == 0 ? "pass" : "fail",
                r->failed == 0 ? "PASS" : "FAIL",
                r->elapsed_sec);
    }
    fprintf(f, "</table>\n");
    if (tfail > 0) {
        fprintf(f, "<h2>Failure Details</h2><ul>\n");
        for (int i = 0; i < g_num_results; i++) {
            file_result_t *r = &g_results[i];
            for (int j = 0; j < r->num_details; j++)
                fprintf(f, "<li><code>%s</code></li>\n", r->first_fails[j].detail);
        }
        fprintf(f, "</ul>\n");
    }
    fprintf(f, "</body></html>\n");
    fclose(f);
    printf("HTML report: %s\n", path);
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */
static void usage(void) {
    printf("Usage: sst_runner [OPTIONS] [FILE...]\n\n"
           "  FILE...             .sst files or mnemonic names\n\n"
           "Options:\n"
           "  --all               Run all .sst files in data dirs\n"
           "  --source=NAME       tomharte, raddad, or both (default)\n"
           "  --data-dir=PATH     Override unified data dir\n"
           "  --max-vectors=N     Limit per file (smoke testing)\n"
           "  --stop-on-fail      Stop at first failure\n"
           "  --verbose           Print each failure\n"
           "  --summary           Print summary\n"
           "  --report-json=PATH  Write JSON report\n"
           "  --report-yaml=PATH  Write YAML report\n"
           "  --report-md=PATH    Write full Markdown report\n"
           "  --report-md-brief=PATH  Write brief Markdown report\n"
           "  --report-html=PATH  Write HTML report\n"
           "  --report-all=DIR    Write all report formats to DIR\n"
           );
}

int main(int argc, char **argv) {
    int do_all = 0, verbose = 0, summary = 0, stop_on_fail = 0;
    int max_vectors = 0;
    const char *source_filter = NULL;
    const char *data_dir = "test/singlestep/unified";
    const char *report_json = NULL, *report_yaml = NULL;
    const char *report_md = NULL, *report_md_brief = NULL;
    const char *report_html = NULL, *report_all_dir = NULL;
    char **files = NULL;
    int num_files = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) do_all = 1;
        else if (strcmp(argv[i], "--verbose") == 0) verbose = 1;
        else if (strcmp(argv[i], "--summary") == 0) summary = 1;
        else if (strcmp(argv[i], "--stop-on-fail") == 0) stop_on_fail = 1;
        else if (strncmp(argv[i], "--source=", 9) == 0) source_filter = argv[i] + 9;
        else if (strncmp(argv[i], "--data-dir=", 11) == 0) data_dir = argv[i] + 11;
        else if (strncmp(argv[i], "--max-vectors=", 14) == 0) max_vectors = atoi(argv[i] + 14);
        else if (strncmp(argv[i], "--report-json=", 14) == 0) report_json = argv[i] + 14;
        else if (strncmp(argv[i], "--report-yaml=", 14) == 0) report_yaml = argv[i] + 14;
        else if (strncmp(argv[i], "--report-md=", 12) == 0) report_md = argv[i] + 12;
        else if (strncmp(argv[i], "--report-md-brief=", 18) == 0) report_md_brief = argv[i] + 18;
        else if (strncmp(argv[i], "--report-html=", 14) == 0) report_html = argv[i] + 14;
        else if (strncmp(argv[i], "--report-all=", 13) == 0) report_all_dir = argv[i] + 13;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(); return 0;
        }
        else {
            /* Positional: treat as .sst file path */
            if (!files) files = malloc(argc * sizeof(char*));
            files[num_files++] = argv[i];
        }
    }

    if (!do_all && num_files == 0) {
        usage();
        return 1;
    }

    /* Init Musashi once */
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);

    /* Check if default data_dir exists; if not, try local 'unified' (for running from test/singlestep) */
    if (access(data_dir, F_OK) != 0 && strcmp(data_dir, "test/singlestep/unified") == 0) {
        if (access("unified", F_OK) == 0) {
            data_dir = "unified";
        }
    }

    /* Prime internal pipeline: place NOP at 0, execute it once.
     * First execute after pulse_reset has stale prefetch state. */
    memset(g_mem, 0, MEM_SIZE);
    g_mem[0] = 0x4E; g_mem[1] = 0x71;  /* NOP at address 0 */
    m68k_pulse_reset();
    m68k_execute(1);

    printf("SST Runner — Musashi SingleStepTests Harness\n");
    printf("Data dir: %s\n\n", data_dir);

    g_run_start = now_sec();

    if (do_all) {
        int do_tomharte = !source_filter || strcmp(source_filter, "tomharte") == 0
                          || strcmp(source_filter, "both") == 0;
        int do_raddad = !source_filter || strcmp(source_filter, "raddad") == 0
                        || strcmp(source_filter, "both") == 0;
        char path[1024];
        if (do_tomharte) {
            snprintf(path, sizeof(path), "%s/tomharte", data_dir);
            scan_dir(path, max_vectors, verbose, stop_on_fail);
        }
        if (do_raddad) {
            snprintf(path, sizeof(path), "%s/raddad", data_dir);
            scan_dir(path, max_vectors, verbose, stop_on_fail);
        }
    } else {
        /* Resolve each positional arg: accept full path, NAME.sst, or bare NAME.
         * Search data_dir/{tomharte,raddad}/ subdirectories for matches. */
        const char *subdirs[] = { "tomharte", "raddad", NULL };
        for (int i = 0; i < num_files; i++) {
            const char *arg = files[i];

            /* If arg contains a path separator, treat as literal path */
            if (strchr(arg, '/') || strchr(arg, '\\')) {
                run_file(arg, max_vectors, verbose, stop_on_fail);
                continue;
            }

            /* Extract base mnemonic: strip .sst extension if present */
            char mnemonic[256];
            strncpy(mnemonic, arg, sizeof(mnemonic) - 1);
            mnemonic[sizeof(mnemonic) - 1] = '\0';
            size_t mlen = strlen(mnemonic);
            if (mlen > 4 && strcmp(mnemonic + mlen - 4, ".sst") == 0)
                mnemonic[mlen - 4] = '\0';

            /* Probe data_dir subdirectories for MNEMONIC.sst */
            int found = 0;
            for (int s = 0; subdirs[s]; s++) {
                char probe[1024];

                /* Apply source filter if --source= was given */
                if (source_filter && strcmp(source_filter, "both") != 0
                    && strcmp(source_filter, subdirs[s]) != 0)
                    continue;

                snprintf(probe, sizeof(probe), "%s/%s/%s.sst",
                         data_dir, subdirs[s], mnemonic);
                FILE *fp = fopen(probe, "rb");
                if (fp) {
                    fclose(fp);
                    run_file(probe, max_vectors, verbose, stop_on_fail);
                    found++;
                }
            }
            if (!found)
                fprintf(stderr, "ERROR: cannot resolve '%s' "
                        "(tried data_dir subdirs)\n", arg);
        }
    }

    g_run_end = now_sec();

    if (summary) print_summary();

    /* ---- Report output ---- */
    /* If no explicit report path was given, auto-generate a timestamped
     * subdirectory under test/singlestep/reports/ for tracking progress. */
    char auto_dir[1024];
    int auto_reports = 0;
    if (!report_json && !report_yaml && !report_md && !report_md_brief
        && !report_html && !report_all_dir) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        /* Derive reports base from data_dir: strip trailing /unified* */
        char reports_base[1024];
        snprintf(reports_base, sizeof(reports_base), "%s", data_dir);
        char *p = strstr(reports_base, "unified");
        if (p) {
            if (p > reports_base && (*(p-1) == '/' || *(p-1) == '\\'))
                *(p-1) = '\0';
            else
                *p = '\0';
        }
        if (reports_base[0] == '\0' || strcmp(reports_base, ".") == 0) {
            strcpy(reports_base, ".");
        } else if (access(reports_base, F_OK) != 0) {
            /* If the derived base doesn't exist, fallback to current dir
             * to avoid creating nested 'test/singlestep/test/singlestep' paths. */
            strcpy(reports_base, ".");
        }
        snprintf(auto_dir, sizeof(auto_dir),
                 "%s/reports/%04d-%02d-%02d_%02d%02d%02d",
                 reports_base,
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
        report_all_dir = auto_dir;
        auto_reports = 1;
    }

    /* Write reports */
    if (report_json) write_report_json(report_json);
    if (report_yaml) write_report_yaml(report_yaml);
    if (report_md) write_report_markdown(report_md, 0);
    if (report_md_brief) write_report_markdown(report_md_brief, 1);
    if (report_html) write_report_html(report_html);
    if (report_all_dir) {
        /* Recursive mkdir: create parent and child directories */
        char tmp[1024];
        snprintf(tmp, sizeof(tmp), "%s", report_all_dir);
        for (char *q = tmp + 1; *q; q++) {
            if (*q == '/') { *q = '\0'; mkdir(tmp, 0755); *q = '/'; }
        }
        mkdir(tmp, 0755);

        char rp[1024];
        snprintf(rp, sizeof(rp), "%s/report.json", report_all_dir); write_report_json(rp);
        snprintf(rp, sizeof(rp), "%s/report.yaml", report_all_dir); write_report_yaml(rp);
        snprintf(rp, sizeof(rp), "%s/report.md", report_all_dir); write_report_markdown(rp, 0);
        snprintf(rp, sizeof(rp), "%s/report_brief.md", report_all_dir); write_report_markdown(rp, 1);
        snprintf(rp, sizeof(rp), "%s/report.html", report_all_dir); write_report_html(rp);

        if (auto_reports)
            printf("\nReports auto-saved to: %s/\n", report_all_dir);
    }

    free(files);

    int tf, tp, tfail, tv;
    compute_totals(&tf, &tp, &tfail, &tv);
    return tfail > 0 ? 1 : 0;
}
