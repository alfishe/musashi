*
* Test case entry point for 68030 MMU tests
* Include this file in each MMU test
*

* Compile/link commands:
* m68k-elf-as -march=68030 -o file.o test.s
* m68k-elf-ld -Ttext 0x10000 --oformat binary -o test.bin file.o

* Memory layout:
* 0x0-0x10000: RAM, contains vector table and stack
* 0x10000-...: ROM, code
* 0x100000-0x100400: Special registers
* 0x300000-0x310000: RAM, page tables go here
* 0x400000-0x410000: RAM, test data area

.set TEST_FAIL_REG, 0x100000
.set TEST_PASS_REG, 0x100004
.set PRINT_REG_REG, 0x100008
.set STDOUT_REG,    0x100014
.set STACK_BASE,    0x3F0

* Page table area
.set PT_BASE,       0x300000
.set PT_ROOT,       0x300000
.set PT_LEVEL_A,    0x301000
.set PT_LEVEL_B,    0x302000
.set PT_LEVEL_C,    0x303000

* Test data area
.set TEST_DATA,     0x400000

.section .text
.globl _start
_start:

.set STACK_TOP, 0x40

main:
    mov.l #BUS_ERROR_HANDLER, 0x08
    mov.l #MMU_CONFIG_ERROR, 0xE0
    jsr run_test
    mov.l #1, TEST_PASS_REG
    stop #0x2700

TEST_FAIL:
    mov.l #0, TEST_FAIL_REG
    stop #0x2700

BUS_ERROR_HANDLER:
    mov.l #0xBE000002, %d7
    rte

MMU_CONFIG_ERROR:
    mov.l #0xCE000038, %d7
    rte

run_test:

