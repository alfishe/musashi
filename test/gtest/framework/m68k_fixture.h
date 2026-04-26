#ifndef M68K_FIXTURE_H
#define M68K_FIXTURE_H

#include <gtest/gtest.h>
#include "m68k.h"
#include <vector>
#include <stdint.h>

class M68kTest : public ::testing::Test {
public:
    static const uint32_t RAM_SIZE = 0x100000; // 1MB
    std::vector<uint8_t> ram;

    // Helper to write memory
    void write_mem_8(uint32_t addr, uint8_t val) {
        if (addr < RAM_SIZE) ram[addr] = val;
    }
    
    void write_mem_16(uint32_t addr, uint16_t val) {
        if (addr + 1 < RAM_SIZE) {
            ram[addr] = (val >> 8) & 0xFF;
            ram[addr+1] = val & 0xFF;
        }
    }

    uint8_t read_mem_8(uint32_t addr) {
        return (addr < RAM_SIZE) ? ram[addr] : 0;
    }

    // Static bridge for Musashi callbacks
    static M68kTest* current_test;

protected:
    void SetUp() override {
        ram.assign(RAM_SIZE, 0);
        m68k_init();
        m68k_set_cpu_type(M68K_CPU_TYPE_68000);
        
        // Musashi requires memory callbacks. 
        // We'll need to define these globally or via static members 
        // because Musashi uses C callbacks.
        current_test = this;
    }

    void TearDown() override {
        current_test = nullptr;
    }
};

#endif // M68K_FIXTURE_H
