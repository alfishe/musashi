#include "m68k_fixture.h"

M68kTest* M68kTest::current_test = nullptr;

extern "C" {
    unsigned int m68k_read_memory_8(unsigned int address) {
        return M68kTest::current_test ? M68kTest::current_test->read_mem_8(address) : 0;
    }
    unsigned int m68k_read_memory_16(unsigned int address) {
        if (!M68kTest::current_test) return 0;
        uint32_t val = M68kTest::current_test->read_mem_8(address) << 8;
        val |= M68kTest::current_test->read_mem_8(address + 1);
        return val;
    }
    unsigned int m68k_read_memory_32(unsigned int address) {
        if (!M68kTest::current_test) return 0;
        uint32_t val = M68kTest::current_test->read_mem_8(address) << 24;
        val |= M68kTest::current_test->read_mem_8(address + 1) << 16;
        val |= M68kTest::current_test->read_mem_8(address + 2) << 8;
        val |= M68kTest::current_test->read_mem_8(address + 3);
        return val;
    }
    void m68k_write_memory_8(unsigned int address, unsigned int value) {
        if (M68kTest::current_test) M68kTest::current_test->write_mem_8(address, value & 0xFF);
    }
    void m68k_write_memory_16(unsigned int address, unsigned int value) {
        if (M68kTest::current_test) M68kTest::current_test->write_mem_16(address, value & 0xFFFF);
    }
    void m68k_write_memory_32(unsigned int address, unsigned int value) {
        if (M68kTest::current_test) {
            M68kTest::current_test->write_mem_8(address, (value >> 24) & 0xFF);
            M68kTest::current_test->write_mem_8(address + 1, (value >> 16) & 0xFF);
            M68kTest::current_test->write_mem_8(address + 2, (value >> 8) & 0xFF);
            M68kTest::current_test->write_mem_8(address + 3, value & 0xFF);
        }
    }
}
