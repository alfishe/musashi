# Just a basic makefile to quickly test that everyting is working, it just
# compiles the .o and the generator

MUSASHIFILES     = m68kcpu.c m68kdasm.c softfloat/softfloat.c
MUSASHIGENCFILES = m68kops.c
MUSASHIGENHFILES = m68kops.h
MUSASHIGENERATOR = m68kmake

EXE =
EXEPATH = ./

.CFILES   = $(MAINFILES) $(OSDFILES) $(MUSASHIFILES) $(MUSASHIGENCFILES)
.OFILES   = $(.CFILES:%.c=%.o)

CC        = gcc
WARNINGS  = -Wall -Wextra -pedantic -g
CFLAGS    = $(WARNINGS)
LFLAGS    = $(WARNINGS) -Wl,-w

DELETEFILES = $(MUSASHIGENCFILES) $(MUSASHIGENHFILES) $(.OFILES) $(TARGET) $(MUSASHIGENERATOR)$(EXE) test_driver$(EXE)


all: $(.OFILES)

clean:
	rm -f $(DELETEFILES)
	@$(MAKE) -C test clean


m68kcpu.o: $(MUSASHIGENHFILES) m68kfpu.c m68kmmu.h softfloat/softfloat.c softfloat/softfloat.h

$(MUSASHIGENCFILES) $(MUSASHIGENHFILES): $(MUSASHIGENERATOR)$(EXE)
	$(EXEPATH)$(MUSASHIGENERATOR)$(EXE)

$(MUSASHIGENERATOR)$(EXE):  $(MUSASHIGENERATOR).c
	$(CC) -o  $(MUSASHIGENERATOR)$(EXE)  $(MUSASHIGENERATOR).c

test_driver$(EXE): test/test_driver.c $(.OFILES)
	$(CC) $(CFLAGS) $(LFLAGS) -o test_driver$(EXE) test/test_driver.c $(.OFILES) -I. -lm


TESTS_68000 = abcd adda add_i addq add addx andi_to_ccr andi_to_sr and \
               bcc bchg bclr bool_i bset bsr btst \
               chk cmpa cmpm cmp dbcc divs divu eori_to_ccr eori_to_sr eor exg ext \
               lea_pea lea_tas lea_tst links \
               movem movep moveq move move_usp move_xxx_flags muls mulu \
               nbcd negs op_cmp_i ori_to_ccr ori_to_sr or \
               rox roxx rtr sbcd scc shifts2 shifts suba sub_i subq sub subx swap trapv

TESTS_68040 = bfchg bfclr bfext bfffo bfins bfset bftst cas chk2 cmp2 \
	divs_long divu_long interrupt jmp mul_long rtd shifts3 trapcc

# 68040 MMU tests
TESTS_68040_MMU = movec_tc movec_urp_srp mmu_simple tt_data walk_4k walk_8k \
	ptest_wp ptest_super ptest_modified ptest_global pflusha pflush_page \
	pflushn invalid_page cinv_cpush indirect_desc um_bits

TESTS_68000_RUN = $(TESTS_68000:%=%.bin)
$(TESTS_68000_RUN): test_driver$(EXE)
	./test_driver$(EXE) test/mc68000/$@

TESTS_68040_RUN = $(TESTS_68040:%=%.bin)
$(TESTS_68040_RUN): test_driver$(EXE)
	./test_driver$(EXE) test/mc68040/$@

TESTS_68040_MMU_RUN = $(TESTS_68040_MMU:%=%.040.bin)
$(TESTS_68040_MMU_RUN): test_driver$(EXE)
	./test_driver$(EXE) --cpu=68040 test/mc68040/$(patsubst %.040.bin,%.bin,$@)

TESTS_68060 = movep_trap cas2_trap chk2_cmp2_trap mull_64_trap divl_64_trap \
	movec_pcr movec_cacr_mask movec_buscr pcr_write_mask mmusr_trap \
	fsin_trap fadd_exec fmovecr_trap fpiar_trap packed_trap \
	fpu_hw_ops fpu_sw_traps fpu_rounding_ops \
	ptest_trap plpa_exec lpstop_exec \
	mmu_regs

TESTS_68060_RUN = $(TESTS_68060:%=%.060.bin)
$(TESTS_68060_RUN): test_driver$(EXE)
	./test_driver$(EXE) --cpu=68060 test/mc68060/$(patsubst %.060.bin,%.bin,$@)

# LC060 tests run with --cpu=68lc060 (no FPU variant)
TESTS_LC060 = lc060_fpu_trap lc060_pcr
TESTS_LC060_RUN = $(TESTS_LC060:%=%.lc060.bin)
$(TESTS_LC060_RUN): test_driver$(EXE)
	./test_driver$(EXE) --cpu=68lc060 test/mc68060/$(patsubst %.lc060.bin,%.bin,$@)

# EC060 tests run with --cpu=68ec060 (no FPU/MMU variant)
TESTS_EC060 = ec060_pcr
TESTS_EC060_RUN = $(TESTS_EC060:%=%.ec060.bin)
$(TESTS_EC060_RUN): test_driver$(EXE)
	./test_driver$(EXE) --cpu=68ec060 test/mc68060/$(patsubst %.ec060.bin,%.bin,$@)

# 68010 regression tests
TESTS_68010 = movec_caar_bug
TESTS_68010_RUN = $(TESTS_68010:%=%.010.bin)
$(TESTS_68010_RUN): test_driver$(EXE)
	./test_driver$(EXE) --cpu=68010 test/mc68010/$(patsubst %.010.bin,%.bin,$@)

# 68030 MMU tests
TESTS_68030 = pmove_tt pmove_tc mmu_identity pflush_all pload_atc ptest_walk ptest_wp tt_match \
              ptest_invalid ptest_multi ptest_super pflush_fc wp_buserr
TESTS_68030_RUN = $(TESTS_68030:%=%.030.bin)
$(TESTS_68030_RUN): test_driver$(EXE)
	./test_driver$(EXE) --cpu=68030 test/mc68030/$(patsubst %.030.bin,%.bin,$@)

build_tests:
	@$(MAKE) -C test all
test: $(TESTS_68000_RUN) $(TESTS_68010_RUN) $(TESTS_68030_RUN) $(TESTS_68040_RUN) $(TESTS_68040_MMU_RUN) $(TESTS_68060_RUN) $(TESTS_LC060_RUN) $(TESTS_EC060_RUN)
