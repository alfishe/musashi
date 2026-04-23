/*
    m68kmmu.h - PMMU implementation for 68851/68030/68040/68060

    Original implementation by R. Belmont
    Enhanced implementation ported from MAME by:
    R. Belmont, Hans Ostermeyer, Sven Schnelle

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

    Ported to standalone Musashi for MiSTer FPGA co-simulation.
*/

#ifndef M68KMMU_H
#define M68KMMU_H

/* ======================================================================== */
/* ========================== MMU CONSTANTS =============================== */
/* ======================================================================== */

/* MMU SR register fields */
#define M68K_MMU_SR_BUS_ERROR       0x8000
#define M68K_MMU_SR_LIMIT           0x4000
#define M68K_MMU_SR_SUPERVISOR_ONLY 0x2000
#define M68K_MMU_SR_WRITE_PROTECT   0x0800
#define M68K_MMU_SR_INVALID         0x0400
#define M68K_MMU_SR_MODIFIED        0x0200
#define M68K_MMU_SR_TRANSPARENT     0x0040

/* MMU translation table descriptor field definitions */
#define M68K_MMU_DF_DT              0x00000003
#define M68K_MMU_DF_DT_INVALID      0x00000000
#define M68K_MMU_DF_DT_PAGE         0x00000001
#define M68K_MMU_DF_DT_TABLE_4BYTE  0x00000002
#define M68K_MMU_DF_DT_TABLE_8BYTE  0x00000003
#define M68K_MMU_DF_WP              0x00000004
#define M68K_MMU_DF_USED            0x00000008
#define M68K_MMU_DF_MODIFIED        0x00000010
#define M68K_MMU_DF_CI              0x00000040
#define M68K_MMU_DF_SUPERVISOR      0x00000100
#define M68K_MMU_DF_ADDR_MASK       0xfffffff0
#define M68K_MMU_DF_IND_ADDR_MASK   0xfffffffc

/* MMU ATC Fields */
#define M68K_MMU_ATC_BUSERROR       0x08000000
#define M68K_MMU_ATC_CACHE_IN       0x04000000
#define M68K_MMU_ATC_WRITE_PR       0x02000000
#define M68K_MMU_ATC_MODIFIED       0x01000000
#define M68K_MMU_ATC_MASK           0x00ffffff
#define M68K_MMU_ATC_SHIFT          8
#define M68K_MMU_ATC_VALID          0x08000000

/* MMU Translation Control register */
#define M68K_MMU_TC_SRE             0x02000000
#define M68K_MMU_TC_FCL             0x01000000

/* TT register */
#define M68K_MMU_TT_ENABLE          0x8000

/* Number of ATC entries (030) */
#ifndef MMU_ATC_ENTRIES
#define MMU_ATC_ENTRIES 22
#endif

/* ======================================================================== */
/* ========================== HELPER FUNCTIONS ============================ */
/* ======================================================================== */

/* Decode effective address for PMMU instructions */
static uint32 pmmu_decode_ea_32(int ea)
{
	int mode = (ea >> 3) & 0x7;
	int reg = (ea & 0x7);

	switch (mode)
	{
		case 2:     /* (An) */
			return REG_A[reg];

		case 3:     /* (An)+ */
			return EA_AY_PI_32();

		case 5:     /* (d16, An) */
			return EA_AY_DI_32();

		case 6:     /* (An) + (Xn) + d8 */
			return EA_AY_IX_32();

		case 7:
			switch (reg)
			{
				case 0:     /* (xxx).W */
					return MAKE_INT_16(OPER_I_16());

				case 1:     /* (xxx).L */
				{
					uint32 d1 = OPER_I_16();
					uint32 d2 = OPER_I_16();
					return (d1 << 16) | d2;
				}

				case 2:     /* (d16, PC) */
					return EA_PCDI_32();

				default:
					break;
			}
			break;

		default:
			break;
	}

	return 0;
}

/* ======================================================================== */
/* ======================== BUS ERROR HANDLING ============================ */
/* ======================================================================== */

static void pmmu_set_buserror(uint32 addr_in)
{
	if (++m68ki_cpu.mmu_tmp_buserror_occurred == 1)
	{
		m68ki_cpu.mmu_tmp_buserror_address = addr_in;
		m68ki_cpu.mmu_tmp_buserror_rw = m68ki_cpu.mmu_tmp_rw;
		m68ki_cpu.mmu_tmp_buserror_fc = m68ki_cpu.mmu_tmp_fc;
		m68ki_cpu.mmu_tmp_buserror_sz = m68ki_cpu.mmu_tmp_sz;

		/* Co-sim: notify access fault */
		m68ki_mmu_cosim_fault(addr_in, m68ki_cpu.mmu_tmp_fc,
			m68ki_cpu.mmu_tmp_rw ? 0 : 1, M68K_MMU_FAULT_BUS_ERROR);
	}
}

/* ======================================================================== */
/* =================== ADDRESS TRANSLATION CACHE (ATC) ==================== */
/* ======================================================================== */

/* pmmu_atc_update_history: update PLRU history for entry N */
static void pmmu_atc_update_history(int entry)
{
	uint32 bit = 1u << entry;
	uint32 all_bits = (1u << MMU_ATC_ENTRIES) - 1;

	m68ki_cpu.mmu_atc_history |= bit;

	/* If all bits would be set, clear others and keep only this entry */
	if (m68ki_cpu.mmu_atc_history == all_bits)
	{
		m68ki_cpu.mmu_atc_history = bit;
	}
}

/* pmmu_atc_find_victim: find entry to evict using PLRU */
static int pmmu_atc_find_victim(void)
{
	int i;
	for (i = 0; i < MMU_ATC_ENTRIES; i++)
	{
		if (!(m68ki_cpu.mmu_atc_history & (1u << i)))
		{
			return i;
		}
	}
	/* All bits set (shouldn't happen), return 0 */
	return 0;
}

/* pmmu_atc_add: adds this address to the ATC */
static void pmmu_atc_add(uint32 logical, uint32 physical, int fc, int rw)
{
	int ps = (m68ki_cpu.mmu_tc >> 24) & 0xf;  /* PS: bits 27-24 */
	uint32 atc_tag = M68K_MMU_ATC_VALID | ((fc & 7) << 24) | ((logical >> ps) << (ps - 8));
	uint32 atc_data = (physical >> ps) << (ps - 8);
	int i, found;

	if (m68ki_cpu.mmu_tmp_sr & (M68K_MMU_SR_BUS_ERROR | M68K_MMU_SR_INVALID | M68K_MMU_SR_SUPERVISOR_ONLY))
	{
		atc_data |= M68K_MMU_ATC_BUSERROR;
	}

	if (m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_WRITE_PROTECT)
	{
		atc_data |= M68K_MMU_ATC_WRITE_PR;
	}

	if (!rw && !(m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_WRITE_PROTECT))
	{
		atc_data |= M68K_MMU_ATC_MODIFIED;
	}

	/* First see if this is already in the cache */
	for (i = 0; i < MMU_ATC_ENTRIES; i++)
	{
		if (m68ki_cpu.mmu_atc_tag[i] == atc_tag)
		{
			m68ki_cpu.mmu_atc_data[i] = atc_data;
			pmmu_atc_update_history(i);
			return;
		}
	}

	/* Find an empty entry */
	found = -1;
	for (i = 0; i < MMU_ATC_ENTRIES; i++)
	{
		if (!(m68ki_cpu.mmu_atc_tag[i] & M68K_MMU_ATC_VALID))
		{
			found = i;
			break;
		}
	}

	/* No empty entry? Use PLRU to find victim */
	if (found == -1)
	{
		found = pmmu_atc_find_victim();
	}

	m68ki_cpu.mmu_atc_tag[found] = atc_tag;
	m68ki_cpu.mmu_atc_data[found] = atc_data;
	pmmu_atc_update_history(found);

	/* Co-sim: notify ATC insert */
	m68ki_mmu_cosim_atc(M68K_MMU_ATC_INSERT, logical, physical);
}

static void pmmu_atc_flush(void)
{
	int i;
	for (i = 0; i < MMU_ATC_ENTRIES; i++)
	{
		m68ki_cpu.mmu_atc_tag[i] = 0;
	}
	m68ki_cpu.mmu_atc_history = 0;

	/* Co-sim: notify ATC flush all */
	m68ki_mmu_cosim_atc(M68K_MMU_ATC_FLUSH_ALL, 0, 0);
}

/* ======================================================================== */
/* ====================== FC (FUNCTION CODE) DECODING ==================== */
/* ======================================================================== */

static int pmmu_get_fc(uint16 modes)
{
	if ((modes & 0x1f) == 0)
	{
		return m68ki_cpu.sfc;
	}

	if ((modes & 0x1f) == 1)
	{
		return m68ki_cpu.dfc;
	}

	if (CPU_TYPE & CPU_TYPE_030)
	{
		/* 68030 has 3 bits FC */
		if (((modes >> 3) & 3) == 1)
		{
			return REG_D[(modes & 7)] & 0x7;
		}

		if (((modes >> 3) & 3) == 2)
		{
			return modes & 7;
		}
	}
	else
	{
		/* 68851 has 4 bits FC */
		if (((modes >> 3) & 3) == 1)
		{
			return REG_D[(modes & 7)] & 0xf;
		}

		if (modes & 0x10)
		{
			return modes & 0xf;
		}
	}

	return 0;
}

/* pmmu_atc_flush_fc_ea: flush ATC entries matching FC and/or EA */
static void pmmu_atc_flush_fc_ea(uint16 modes)
{
	int fcmask = (modes >> 5) & 7;
	int fc = pmmu_get_fc(modes) & fcmask;
	int ps = (m68ki_cpu.mmu_tc >> 24) & 0xf;  /* PS: bits 27-24 */
	int mode = (modes >> 10) & 7;
	uint32 ea;
	int i;

	switch (mode)
	{
	case 1: /* PFLUSHA */
		pmmu_atc_flush();
		break;

	case 4: /* flush by fc */
		for (i = 0; i < MMU_ATC_ENTRIES; i++)
		{
			if ((m68ki_cpu.mmu_atc_tag[i] & M68K_MMU_ATC_VALID) &&
			    ((m68ki_cpu.mmu_atc_tag[i] >> 24) & fcmask) == (uint32)fc)
			{
				m68ki_cpu.mmu_atc_tag[i] = 0;
			}
		}
		break;

	case 6: /* flush by fc + ea */
	{
		uint32 ea_page;
		ea = pmmu_decode_ea_32(m68ki_cpu.ir);
		ea_page = ((ea >> ps) << (ps - 8)) & 0x00FFFFFF;
		for (i = 0; i < MMU_ATC_ENTRIES; i++)
		{
			if ((m68ki_cpu.mmu_atc_tag[i] & M68K_MMU_ATC_VALID) &&
			    (((m68ki_cpu.mmu_atc_tag[i] >> 24) & fcmask) == (uint32)fc) &&
			    ((m68ki_cpu.mmu_atc_tag[i] & 0x00FFFFFF) == ea_page))
			{
				m68ki_cpu.mmu_atc_tag[i] = 0;
			}
		}
		break;
	}

	default:
		break;
	}
}

/* pmmu_atc_lookup: look up address in ATC
 * Returns 1 if found, 0 if not found
 * ptest parameter: 1 for PTEST, 0 for normal translation
 */
static int pmmu_atc_lookup(uint32 addr_in, int fc, int rw, int ptest, uint32 *addr_out)
{
	int ps = (m68ki_cpu.mmu_tc >> 24) & 0xf;  /* PS: bits 27-24 */
	uint32 atc_tag = M68K_MMU_ATC_VALID | ((fc & 7) << 24) | ((addr_in >> ps) << (ps - 8));
	int i;

	for (i = 0; i < MMU_ATC_ENTRIES; i++)
	{
		uint32 atc_data;

		if (m68ki_cpu.mmu_atc_tag[i] != atc_tag)
		{
			continue;
		}

		atc_data = m68ki_cpu.mmu_atc_data[i];

		if (!ptest && !rw)
		{
			/* If the M bit is clear and a write access is attempted,
			 * invalidate the ATC entry and do a table search */
			if (!(atc_data & M68K_MMU_ATC_MODIFIED))
			{
				m68ki_cpu.mmu_atc_tag[i] = 0;
				continue;
			}
		}

		m68ki_cpu.mmu_tmp_sr = 0;
		if (atc_data & M68K_MMU_ATC_MODIFIED)
		{
			m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_MODIFIED;
		}

		if (atc_data & M68K_MMU_ATC_WRITE_PR)
		{
			m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_WRITE_PROTECT;
		}

		if (atc_data & M68K_MMU_ATC_BUSERROR)
		{
			m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_BUS_ERROR | M68K_MMU_SR_INVALID;
		}

		/* Update PLRU history on hit */
		pmmu_atc_update_history(i);

		*addr_out = (atc_data << 8) | (addr_in & ~(~0u << ps));
		return 1;
	}

	if (ptest)
	{
		m68ki_cpu.mmu_tmp_sr = M68K_MMU_SR_INVALID;
	}
	return 0;
}

/* ======================================================================== */
/* =================== TRANSPARENT TRANSLATION (TT) ======================= */
/* ======================================================================== */

static int pmmu_match_tt(uint32 addr_in, int fc, uint32 tt, int rw)
{
	uint32 address_base, address_mask;
	uint32 fcmask, fcbits;
	int s_field;

	if (!(tt & M68K_MMU_TT_ENABLE))
	{
		return 0;
	}

	/* Check S field (supervisor/user matching) - bits 14-13 */
	s_field = (tt >> 13) & 3;
	if (s_field == 0 && (fc & 4))  /* 00 = match user only, but FC is supervisor */
	{
		return 0;
	}
	if (s_field == 1 && !(fc & 4))  /* 01 = match supervisor only, but FC is user */
	{
		return 0;
	}
	/* s_field 2 or 3 = match both */

	/* Transparent translation enabled */
	address_base = tt & 0xff000000;
	address_mask = ((tt << 8) & 0xff000000) ^ 0xff000000;
	fcmask = (~tt) & 7;
	fcbits = (tt >> 4) & 7;

	if ((addr_in & address_mask) != (address_base & address_mask))
	{
		return 0;
	}

	/* FC matching - only check if mask allows */
	if ((fc & fcmask) != (fcbits & fcmask))
	{
		return 0;
	}

	/* R/W matching - bit 8 is RW, bit 9 is RWM (enable) */
	if (tt & 0x100)  /* RWM set - check R/W */
	{
		int tt_rw = (tt & 0x200) ? 1 : 0;
		if (rw != tt_rw)
		{
			return 0;
		}
	}

	m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_TRANSPARENT;
	return 1;
}

/* ======================================================================== */
/* ====================== DESCRIPTOR UPDATES ============================== */
/* ======================================================================== */

static void pmmu_update_descriptor(uint32 tptr, int type, uint32 entry, int rw)
{
	if (type == M68K_MMU_DF_DT_PAGE && !rw &&
	    !(entry & M68K_MMU_DF_MODIFIED) &&
	    !(entry & M68K_MMU_DF_WP))
	{
		/* Set M+U bits on write to unmodified page */
		m68k_write_memory_32(tptr, entry | M68K_MMU_DF_USED | M68K_MMU_DF_MODIFIED);
	}
	else if (type != M68K_MMU_DF_DT_INVALID && !(entry & M68K_MMU_DF_USED))
	{
		/* Set U bit */
		m68k_write_memory_32(tptr, entry | M68K_MMU_DF_USED);
	}
}

static void pmmu_update_sr(int type, uint32 tbl_entry, int fc, int is_long)
{
	switch (type)
	{
	case M68K_MMU_DF_DT_INVALID:
		/* Invalid has no flags */
		break;

	case M68K_MMU_DF_DT_PAGE:
		if (tbl_entry & M68K_MMU_DF_MODIFIED)
		{
			m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_MODIFIED;
		}
		/* fall through */

	case M68K_MMU_DF_DT_TABLE_4BYTE:
	case M68K_MMU_DF_DT_TABLE_8BYTE:
		if (tbl_entry & M68K_MMU_DF_WP)
		{
			m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_WRITE_PROTECT;
		}

		if (is_long && !(fc & 4) && (tbl_entry & M68K_MMU_DF_SUPERVISOR))
		{
			m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_SUPERVISOR_ONLY;
		}
		break;

	default:
		break;
	}
}

/* ======================================================================== */
/* ========================= TABLE WALK (030) ============================= */
/* ======================================================================== */

/*
 * pmmu_walk_tables: perform 68030 page table walk
 * Returns 1 if resolved, 0 if not
 * ptest parameter: 1 for PTEST, 0 for normal translation
 */
static int pmmu_walk_tables(uint32 addr_in, int type, uint32 table, int fc,
                            int limit, int rw, int ptest, uint32 *addr_out)
{
	int level = 0;
	uint32 bits = m68ki_cpu.mmu_tc & 0xffff;   /* TIA/TIB/TIC/TID */
	int pagesize = (m68ki_cpu.mmu_tc >> 24) & 0xf;  /* PS: bits 27-24 */
	int is = (m68ki_cpu.mmu_tc >> 20) & 0xf;        /* IS: bits 23-20 */
	int bitpos = 12;
	int resolved = 0;
	int pageshift = is;

	addr_in <<= is;

	m68ki_cpu.mmu_tablewalk = 1;

	if (m68ki_cpu.mmu_tc & M68K_MMU_TC_FCL)
	{
		bitpos = 16;
	}

	do
	{
		int indexbits = (bits >> bitpos) & 0xf;
		int table_index = (bitpos == 16) ? fc : (addr_in >> (32 - indexbits));
		int indirect;
		uint32 tbl_entry, tbl_entry2;

		bitpos -= 4;
		indirect = (!bitpos || !(bits >> bitpos)) && indexbits;

		switch (type)
		{
		case M68K_MMU_DF_DT_INVALID:   /* Invalid, will cause MMU exception */
			m68ki_cpu.mmu_tmp_sr = M68K_MMU_SR_INVALID;
			resolved = 1;
			break;

		case M68K_MMU_DF_DT_PAGE:   /* Page descriptor, direct mapping */
			if (!ptest)
			{
				table &= ~0u << pagesize;
				*addr_out = table + (addr_in >> pageshift);
			}
			resolved = 1;
			break;

		case M68K_MMU_DF_DT_TABLE_4BYTE:   /* Valid 4-byte descriptors */
			level++;
			*addr_out = table + (table_index << 2);
			tbl_entry = m68k_read_memory_32(*addr_out);
			type = tbl_entry & M68K_MMU_DF_DT;

			if (indirect && (type == 2 || type == 3))
			{
				/* Indirect descriptor */
				level++;
				*addr_out = tbl_entry & M68K_MMU_DF_IND_ADDR_MASK;
				tbl_entry = m68k_read_memory_32(*addr_out);
				type = tbl_entry & M68K_MMU_DF_DT;
			}

			table = tbl_entry & M68K_MMU_DF_ADDR_MASK;
			pmmu_update_sr(type, tbl_entry, fc, 0);
			if (!ptest)
			{
				pmmu_update_descriptor(*addr_out, type, tbl_entry, rw);
			}
			break;

		case M68K_MMU_DF_DT_TABLE_8BYTE:   /* Valid 8-byte descriptors */
			level++;
			*addr_out = table + (table_index << 3);
			tbl_entry = m68k_read_memory_32(*addr_out);
			tbl_entry2 = m68k_read_memory_32(*addr_out + 4);
			type = tbl_entry & M68K_MMU_DF_DT;

			if (indirect && (type == 2 || type == 3))
			{
				/* Indirect descriptor */
				level++;
				*addr_out = tbl_entry2 & M68K_MMU_DF_IND_ADDR_MASK;
				tbl_entry = m68k_read_memory_32(*addr_out);
				tbl_entry2 = m68k_read_memory_32(*addr_out + 4);
				type = tbl_entry & M68K_MMU_DF_DT;
			}

			table = tbl_entry2 & M68K_MMU_DF_ADDR_MASK;
			pmmu_update_sr(type, tbl_entry, fc, 1);
			if (!ptest)
			{
				pmmu_update_descriptor(*addr_out, type, tbl_entry, rw);
			}
			break;
		}

		if (m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_BUS_ERROR)
		{
			/* Bus error during page table walking is always fatal */
			resolved = 1;
			break;
		}

		if (!ptest)
		{
			if (!rw && (m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_WRITE_PROTECT))
			{
				resolved = 1;
				break;
			}

			if (!(fc & 4) && (m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_SUPERVISOR_ONLY))
			{
				resolved = 1;
				break;
			}
		}

		addr_in <<= indexbits;
		pageshift += indexbits;
	} while (level < limit && !resolved);

	m68ki_cpu.mmu_tmp_sr &= 0xfff0;
	m68ki_cpu.mmu_tmp_sr |= level;
	m68ki_cpu.mmu_tablewalk = 0;

	return resolved;
}

/* ======================================================================== */
/* =================== 030 TRANSLATION WITH FC ============================ */
/* ======================================================================== */

/*
 * pmmu_translate_addr_with_fc: perform 68851/68030-style PMMU address translation
 * ptest: 1 for PTEST operation, 0 for normal
 * pload: 1 for PLOAD operation, 0 for normal
 */
static uint32 pmmu_translate_addr_with_fc(uint32 addr_in, uint8 fc, int rw, int ptest, int pload, int limit)
{
	uint32 addr_out = 0;
	int type;
	uint32 tbl_addr;

	m68ki_cpu.mmu_tmp_sr = 0;
	m68ki_cpu.mmu_last_logical_addr = addr_in;

	/* Check transparent translation first */
	if (pmmu_match_tt(addr_in, fc, m68ki_cpu.mmu_tt0, rw) ||
	    pmmu_match_tt(addr_in, fc, m68ki_cpu.mmu_tt1, rw) ||
	    fc == 7)
	{
		return addr_in;
	}

	/* PTEST with level 0 just checks ATC */
	if (ptest && limit == 0)
	{
		pmmu_atc_lookup(addr_in, fc, rw, 1, &addr_out);
		return addr_out;
	}

	/* For non-PTEST/PLOAD, try ATC lookup first */
	if (!ptest && !pload && pmmu_atc_lookup(addr_in, fc, rw, 0, &addr_out))
	{
		if ((m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_BUS_ERROR) ||
		    (!rw && (m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_WRITE_PROTECT)))
		{
			pmmu_set_buserror(addr_in);
		}

		/* Co-sim: notify translation complete (ATC hit path) */
		m68ki_mmu_cosim_translate(addr_in, addr_out, fc, rw ? 0 : 1, 0, 1,
			m68ki_cpu.mmu_tmp_sr);

		return addr_out;
	}

	/* Select root pointer based on supervisor mode and SRE bit */
	if ((m68ki_cpu.mmu_tc & M68K_MMU_TC_SRE) && (fc & 4))
	{
		tbl_addr = m68ki_cpu.mmu_srp_aptr & M68K_MMU_DF_ADDR_MASK;
		type = m68ki_cpu.mmu_srp_limit & M68K_MMU_DF_DT;
	}
	else
	{
		tbl_addr = m68ki_cpu.mmu_crp_aptr & M68K_MMU_DF_ADDR_MASK;
		type = m68ki_cpu.mmu_crp_limit & M68K_MMU_DF_DT;
	}

	/* Do the table walk */
	if (!pmmu_walk_tables(addr_in, type, tbl_addr, fc, limit, rw, ptest, &addr_out))
	{
		/* Should not happen - walk should always resolve */
		m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_INVALID;
	}

	if (ptest)
	{
		return addr_out;
	}

	/* Check for access violations */
	if ((m68ki_cpu.mmu_tmp_sr & (M68K_MMU_SR_INVALID | M68K_MMU_SR_SUPERVISOR_ONLY)) ||
	    ((m68ki_cpu.mmu_tmp_sr & M68K_MMU_SR_WRITE_PROTECT) && !rw))
	{
		if (!pload)
		{
			pmmu_set_buserror(addr_in);
		}
	}

	/* Add to ATC */
	pmmu_atc_add(addr_in, addr_out, fc, rw && type != 1);

	/* Co-sim: notify translation complete (table walk path) */
	m68ki_mmu_cosim_translate(addr_in, addr_out, fc, rw ? 0 : 1, 0, 0,
		m68ki_cpu.mmu_tmp_sr);

	return addr_out;
}

/* ======================================================================== */
/* ======================== 040 MMU FUNCTIONS ============================= */
/* ======================================================================== */

/*
 * pmmu_translate_addr_with_fc_040: perform 68040-style PMMU address translation
 */
static uint32 pmmu_translate_addr_with_fc_040(uint32 addr_in, uint8 fc, int ptest)
{
	uint32 addr_out, tt0, tt1;
	uint32 root_idx, ptr_idx, page_idx, page;
	uint32 root_ptr, pointer_ptr, page_ptr;
	uint32 root_entry, pointer_entry, page_entry;

	addr_out = addr_in;
	m68ki_cpu.mmu_tmp_sr = 0;
	m68ki_cpu.mmu_last_logical_addr = addr_in;

	/* Select TT registers based on access type */
	if (fc & 1)  /* Data access, use DTT0/DTT1 */
	{
		tt0 = m68ki_cpu.mmu040_dtt0;
		tt1 = m68ki_cpu.mmu040_dtt1;
	}
	else if (fc & 2)  /* Program access, use ITT0/ITT1 */
	{
		tt0 = m68ki_cpu.mmu040_itt0;
		tt1 = m68ki_cpu.mmu040_itt1;
	}
	else
	{
		tt0 = 0;
		tt1 = 0;
	}

	/* Check transparent translation registers */
	if (tt0 & M68K_MMU_TT_ENABLE)
	{
		static const int fcmask[4] = { 4, 4, 0, 0 };
		static const int fcmatch[4] = { 0, 4, 0, 0 };
		uint32 mask = (tt0 >> 16) & 0xff;
		mask ^= 0xff;
		mask <<= 24;

		if ((addr_in & mask) == (tt0 & mask) &&
		    (fc & fcmask[(tt0 >> 13) & 3]) == fcmatch[(tt0 >> 13) & 3])
		{
			if ((tt0 & 4) && !m68ki_cpu.mmu_tmp_rw && !ptest)
			{
				pmmu_set_buserror(addr_in);
			}
			return addr_in;
		}
	}

	if (tt1 & M68K_MMU_TT_ENABLE)
	{
		static const int fcmask[4] = { 4, 4, 0, 0 };
		static const int fcmatch[4] = { 0, 4, 0, 0 };
		uint32 mask = (tt1 >> 16) & 0xff;
		mask ^= 0xff;
		mask <<= 24;

		if ((addr_in & mask) == (tt1 & mask) &&
		    (fc & fcmask[(tt1 >> 13) & 3]) == fcmatch[(tt1 >> 13) & 3])
		{
			if ((tt1 & 4) && !m68ki_cpu.mmu_tmp_rw && !ptest)
			{
				pmmu_set_buserror(addr_in);
			}
			return addr_in;
		}
	}

	if (m68ki_cpu.pmmu_enabled)
	{
		root_idx = (addr_in >> 25) & 0x7f;
		ptr_idx = (addr_in >> 18) & 0x7f;

		/* Select supervisor or user root pointer */
		if (fc & 4)
		{
			root_ptr = m68ki_cpu.mmu040_srp + (root_idx << 2);
		}
		else
		{
			root_ptr = m68ki_cpu.mmu040_urp + (root_idx << 2);
		}

		/* Get the root entry */
		root_entry = m68k_read_memory_32(root_ptr);

		/* Is UDT marked valid? */
		if (root_entry & 2)
		{
			/* Set U bit if not already set */
			if (!(root_entry & 0x8) && !ptest)
			{
				root_entry |= 0x8;
				m68k_write_memory_32(root_ptr, root_entry);
			}

			/* PTEST: check write protect */
			if (ptest && (root_entry & 4))
			{
				m68ki_cpu.mmu_tmp_sr |= 4;
			}

			pointer_ptr = (root_entry & ~0x1ff) + (ptr_idx << 2);
			pointer_entry = m68k_read_memory_32(pointer_ptr);

			/* PTEST: check write protect */
			if (ptest && (pointer_entry & 4))
			{
				m68ki_cpu.mmu_tmp_sr |= 4;
			}

			/* Update U bit */
			if (!(pointer_entry & 0x8) && !ptest)
			{
				pointer_entry |= 0x8;
				m68k_write_memory_32(pointer_ptr, pointer_entry);
			}

			/* Write protected by root or pointer entries? */
			if ((((root_entry & 4) && !m68ki_cpu.mmu_tmp_rw) ||
			     ((pointer_entry & 4) && !m68ki_cpu.mmu_tmp_rw)) && !ptest)
			{
				pmmu_set_buserror(addr_in);
				return addr_in;
			}

			/* Is UDT valid on the pointer entry? */
			if (!(pointer_entry & 2) && !ptest)
			{
				pmmu_set_buserror(addr_in);
				return addr_in;
			}
		}
		else
		{
			if (!ptest)
			{
				pmmu_set_buserror(addr_in);
			}
			return addr_in;
		}

		/* Now do the page lookup */
		if (m68ki_cpu.mmu040_tc & 0x4000)  /* 8k pages */
		{
			page_idx = (addr_in >> 13) & 0x1f;
			page = addr_in & 0x1fff;
			pointer_entry &= ~0x7f;
		}
		else  /* 4k pages */
		{
			page_idx = (addr_in >> 12) & 0x3f;
			page = addr_in & 0xfff;
			pointer_entry &= ~0xff;
		}

		page_ptr = pointer_entry + (page_idx << 2);
		page_entry = m68k_read_memory_32(page_ptr);

		/* Resolve indirect page pointers */
		while ((page_entry & 3) == 2)
		{
			page_entry = m68k_read_memory_32(page_entry & ~0x3);
		}

		/* Is the page write protected or supervisor protected? */
		if ((((page_entry & 4) && !m68ki_cpu.mmu_tmp_rw) ||
		     ((page_entry & 0x80) && !(fc & 4))) && !ptest)
		{
			pmmu_set_buserror(addr_in);
			return addr_in;
		}

		switch (page_entry & 3)
		{
		case 0:  /* Invalid */
			if (!ptest)
			{
				pmmu_set_buserror(addr_in);
			}
			return addr_in;

		case 1:
		case 3:  /* Resident */
			if (m68ki_cpu.mmu040_tc & 0x4000)  /* 8k pages */
			{
				addr_out = (page_entry & ~0x1fff) | page;
			}
			else
			{
				addr_out = (page_entry & ~0xfff) | page;
			}

			if (!ptest)
			{
				page_entry |= 0x8;  /* Set U bit */

				/* If writing, set M bit */
				if (!m68ki_cpu.mmu_tmp_rw)
				{
					page_entry |= 0x10;
				}

				/* Write back if changed */
				m68k_write_memory_32(page_ptr, page_entry);
			}
			else
			{
				/* PTEST: populate SR */
				m68ki_cpu.mmu_tmp_sr |= ((addr_out & ~0xfff) | (page_entry & 0x7f4));
			}
			break;

		case 2:  /* Indirect - should not happen at this point */
			break;
		}
	}

	return addr_out;
}

/* ======================================================================== */
/* ==================== MAIN TRANSLATION FUNCTION ========================= */
/* ======================================================================== */

/*
 * pmmu_translate_addr: perform PMMU address translation
 * This is the main entry point called from memory access functions
 */
uint pmmu_translate_addr(uint addr_in)
{
	uint32 addr_out;
	int rw = m68ki_cpu.mmu_tmp_rw;
	uint8 fc = m68ki_cpu.mmu_tmp_fc;

	/* During table walk, memory accesses are physical (no translation) */
	if (m68ki_cpu.mmu_tablewalk)
	{
		return addr_in;
	}

	if (CPU_TYPE_IS_040_PLUS(CPU_TYPE))
	{
		addr_out = pmmu_translate_addr_with_fc_040(addr_in, fc, 0);
	}
	else
	{
		addr_out = pmmu_translate_addr_with_fc(addr_in, fc, rw, 0, 0, 7);
	}

	return addr_out;
}

/* ======================================================================== */
/* ======================== PMMU INSTRUCTIONS ============================= */
/* ======================================================================== */

/* m68851_pload: PLOAD instruction */
static void m68851_pload(uint32 ea, uint16 modes)
{
	uint32 ltmp = pmmu_decode_ea_32(ea);
	int fc = pmmu_get_fc(modes);
	int rw = (modes & 0x200) ? 1 : 0;

	/* MC68851 traps if MMU is not enabled, 030 does not */
	if (m68ki_cpu.pmmu_enabled || (CPU_TYPE & CPU_TYPE_030))
	{
		if (CPU_TYPE_IS_040_PLUS(CPU_TYPE))
		{
			pmmu_translate_addr_with_fc_040(ltmp, fc, 0);
		}
		else
		{
			pmmu_translate_addr_with_fc(ltmp, fc, rw, 0, 1, 7);
		}
	}
	else
	{
		m68ki_exception_trap(57);
	}
}

/* m68851_ptest: PTEST instruction */
static void m68851_ptest(uint32 ea, uint16 modes)
{
	uint32 v_addr = pmmu_decode_ea_32(ea);
	uint32 p_addr;
	int level = (modes >> 10) & 7;
	int rw = (modes & 0x200) ? 1 : 0;
	int fc = pmmu_get_fc(modes);

	if (CPU_TYPE_IS_040_PLUS(CPU_TYPE))
	{
		p_addr = pmmu_translate_addr_with_fc_040(v_addr, fc, 1);
	}
	else
	{
		p_addr = pmmu_translate_addr_with_fc(v_addr, fc, rw, 1, 0, level);
	}

	m68ki_cpu.mmu_sr = m68ki_cpu.mmu_tmp_sr;

	/* If A-register field specified, store last descriptor address */
	if (modes & 0x100)
	{
		int areg = (modes >> 5) & 7;
		REG_A[areg] = p_addr;
	}
}

/* m68851_pmove_get: PMOVE from MMU register */
static void m68851_pmove_get(uint32 ea, uint16 modes)
{
	switch ((modes >> 10) & 0x3f)
	{
	case 0x02:  /* Transparent translation register 0 */
		WRITE_EA_32(ea, m68ki_cpu.mmu_tt0);
		break;

	case 0x03:  /* Transparent translation register 1 */
		WRITE_EA_32(ea, m68ki_cpu.mmu_tt1);
		break;

	case 0x10:  /* Translation control register */
		WRITE_EA_32(ea, m68ki_cpu.mmu_tc);
		break;

	case 0x12:  /* Supervisor root pointer */
		WRITE_EA_64(ea, (uint64)m68ki_cpu.mmu_srp_limit << 32 | (uint64)m68ki_cpu.mmu_srp_aptr);
		break;

	case 0x13:  /* CPU root pointer */
		WRITE_EA_64(ea, (uint64)m68ki_cpu.mmu_crp_limit << 32 | (uint64)m68ki_cpu.mmu_crp_aptr);
		break;

	default:
		break;
	}

	/* Note: PMOVE from register does NOT flush ATC - only PMOVE to register can flush */
}

/* m68851_pmove_put: PMOVE to MMU register */
static void m68851_pmove_put(uint32 ea, uint16 modes)
{
	uint64 temp64;
	uint32 temp;

	switch ((modes >> 13) & 7)
	{
	case 0:
		temp = READ_EA_32(ea);
		if (((modes >> 10) & 7) == 2)
		{
			m68ki_cpu.mmu_tt0 = temp;
		}
		else if (((modes >> 10) & 7) == 3)
		{
			m68ki_cpu.mmu_tt1 = temp;
		}
		if (!(modes & 0x100))
		{
			pmmu_atc_flush();
		}
		break;

	case 2:
		switch ((modes >> 10) & 7)
		{
		case 0:  /* Translation control register */
		{
			uint32 tc_val = READ_EA_32(ea);

			if (tc_val & 0x80000000)
			{
				/* Validate TC: IS + TIA + TIB + TIC + TID + PS must equal 32 */
				uint ps  = (tc_val >> 24) & 0x0F;
				uint is  = (tc_val >> 20) & 0x0F;
				uint tia = (tc_val >> 16) & 0x0F;
				uint tib = (tc_val >> 12) & 0x0F;
				uint tic = (tc_val >> 8)  & 0x0F;
				uint tid = (tc_val >> 4)  & 0x0F;

				/* PS must be 8-15 (256B to 32KB pages), and sum must equal 32 */
				if (ps < 8 || is + tia + tib + tic + tid + ps != 32)
				{
					/* Configuration error - vector 56 (0xE0) */
					m68ki_exception_trap(56);
					return;
				}

				m68ki_cpu.mmu_tc = tc_val;
				m68ki_cpu.pmmu_enabled = 1;
			}
			else
			{
				m68ki_cpu.mmu_tc = tc_val;
				m68ki_cpu.pmmu_enabled = 0;
			}

			if (!(modes & 0x100))
			{
				pmmu_atc_flush();
			}
			break;
		}

		case 2:  /* Supervisor root pointer */
			temp64 = READ_EA_64(ea);
			m68ki_cpu.mmu_srp_limit = (temp64 >> 32) & 0xffffffff;
			m68ki_cpu.mmu_srp_aptr = temp64 & 0xffffffff;
			/* SRP type 0 is not allowed */
			if ((m68ki_cpu.mmu_srp_limit & 3) == 0)
			{
				m68ki_exception_trap(56);
				return;
			}
			if (!(modes & 0x100))
			{
				pmmu_atc_flush();
			}
			break;

		case 3:  /* CPU root pointer */
			temp64 = READ_EA_64(ea);
			m68ki_cpu.mmu_crp_limit = (temp64 >> 32) & 0xffffffff;
			m68ki_cpu.mmu_crp_aptr = temp64 & 0xffffffff;
			/* CRP type 0 is not allowed */
			if ((m68ki_cpu.mmu_crp_limit & 3) == 0)
			{
				m68ki_exception_trap(56);
				return;
			}
			if (!(modes & 0x100))
			{
				pmmu_atc_flush();
			}
			break;

		default:
			break;
		}
		break;

	case 3:  /* MMU status */
		/* Writing to MMUSR is not typically done */
		break;
	}
}

/* m68851_pmove: PMOVE instruction */
static void m68851_pmove(uint32 ea, uint16 modes)
{
	switch ((modes >> 13) & 0x7)
	{
	case 0:  /* MC68030/040 form with FD bit */
	case 2:  /* MC68851 form, FD never set */
		if (modes & 0x200)
		{
			m68851_pmove_get(ea, modes);
		}
		else
		{
			m68851_pmove_put(ea, modes);
		}
		break;

	case 3:  /* MC68030 to/from status reg */
		if (modes & 0x200)
		{
			WRITE_EA_16(ea, m68ki_cpu.mmu_sr);
		}
		else
		{
			m68ki_cpu.mmu_sr = READ_EA_16(ea);
		}
		break;

	default:
		break;
	}
}

/* ======================================================================== */
/* =================== MAIN MMU INSTRUCTION DISPATCHER ==================== */
/* ======================================================================== */

/*
 * m68881_mmu_ops: COP 0 MMU opcode handling
 * Called for F-line instructions that are PMMU operations
 */
void m68881_mmu_ops(void)
{
	uint16 modes;
	uint32 ea = m68ki_cpu.ir & 0x3f;

	/* Catch the 2 "weird" encodings up front (PBcc) */
	if ((m68ki_cpu.ir & 0xffc0) == 0xf0c0)
	{
		fprintf(stderr, "680x0: unhandled PBcc\n");
		return;
	}
	else if ((m68ki_cpu.ir & 0xffc0) == 0xf080)
	{
		fprintf(stderr, "680x0: unhandled PBcc\n");
		return;
	}
	else if ((m68ki_cpu.ir & 0xffe0) == 0xf500)
	{
		/* 68040 PFLUSH */
		pmmu_atc_flush();
	}
	else  /* the rest are 1111000xxxXXXXXX where xxx is the instruction family */
	{
		switch ((m68ki_cpu.ir >> 9) & 0x7)
		{
		case 0:
			modes = OPER_I_16();

			if ((modes & 0xfde0) == 0x2000)  /* PLOAD */
			{
				m68851_pload(ea, modes);
				return;
			}
			else if ((modes & 0xe200) == 0x2000)  /* PFLUSH */
			{
				pmmu_atc_flush_fc_ea(modes);
				return;
			}
			else if (modes == 0xa000)  /* PFLUSHR */
			{
				pmmu_atc_flush();
				return;
			}
			else if (modes == 0x2800)  /* PVALID (FORMAT 1) */
			{
				fprintf(stderr, "680x0: unhandled PVALID1\n");
				return;
			}
			else if ((modes & 0xfff8) == 0x2c00)  /* PVALID (FORMAT 2) */
			{
				fprintf(stderr, "680x0: unhandled PVALID2\n");
				return;
			}
			else if ((modes & 0xe000) == 0x8000)  /* PTEST */
			{
				m68851_ptest(ea, modes);
				return;
			}
			else
			{
				m68851_pmove(ea, modes);
			}
			break;

		default:
			fprintf(stderr, "680x0: unknown PMMU instruction group %d\n", (m68ki_cpu.ir >> 9) & 0x7);
			break;
		}
	}
}

/* ======================================================================== */
/* ========================= BUS ERROR SUPPORT ============================ */
/* ======================================================================== */

/*
 * m68851_buserror: called when a bus error occurs during PMMU operation
 * Returns 1 if the bus error was during a table walk, 0 otherwise
 */
int m68851_buserror(uint32 *addr)
{
	if (!m68ki_cpu.pmmu_enabled)
	{
		return 0;
	}

	if (m68ki_cpu.mmu_tablewalk)
	{
		m68ki_cpu.mmu_tmp_sr |= M68K_MMU_SR_BUS_ERROR | M68K_MMU_SR_INVALID;
		return 1;
	}

	*addr = m68ki_cpu.mmu_last_logical_addr;
	return 0;
}

#endif /* M68KMMU_H */
