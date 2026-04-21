# Makefile for 68030 MMU tests
# Requires m68k-elf-as and m68k-elf-ld from m68k-elf toolchain

AS = m68k-elf-as
LD = m68k-elf-ld
ASFLAGS = -march=68030
LDFLAGS = -Ttext 0x10000 --oformat binary

TESTS = pmove_tt.bin pmove_tc.bin mmu_identity.bin pflush_all.bin ptest_walk.bin ptest_wp.bin pload_atc.bin tt_match.bin \
        ptest_invalid.bin ptest_multi.bin ptest_super.bin pflush_fc.bin wp_buserr.bin

all: $(TESTS)

%.o: %.s entry_030_mmu.s
	$(AS) $(ASFLAGS) -o $@ $<

%.bin: %.o
	$(LD) $(LDFLAGS) -o $@ $<

clean:
	rm -f *.o *.bin

.PHONY: all clean
