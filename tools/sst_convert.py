#!/usr/bin/env python3
"""
Unified SingleStepTests converter for Musashi test harness.

Converts test vectors from TWO independent sources into a common .sst format.
These files are consumed by the `sst_runner` (C binary) to perform high-speed 
regression testing of the Musashi 68k core. The recommended way to run this 
entire pipeline is via the `./test/singlestep/sst_runner.sh` bootstrap script.

  Source A — TomHarte / 680x0 (preferred, ~1M vectors)
    Repository: https://github.com/SingleStepTests/680x0
    Format:     gzipped JSON (.json.gz), byte-addressed RAM
    Location:   test/singlestep/data/680x0/68000/v1/*.json.gz

  Source B — raddad772 / m68000 (~100K vectors)
    Repository: https://github.com/SingleStepTests/m68000
    Format:     proprietary binary (.json.bin), 16-bit word RAM
    Location:   test/singlestep/data/m68000/v1/*.json.bin

Both sources provide single-instruction execution tests for the Motorola 68000.
Each test vector contains initial CPU state (registers + RAM), and expected
final state after executing exactly one instruction. Bus cycle traces and
prefetch data are stripped during conversion (Musashi is not cycle-exact).

IMPORTANT: The .sst output format uses HOST ENDIANNESS for all multi-byte values.
It is NOT cross-platform portable. If you move vectors between different
architectures (e.g., x86_64 to ARM64), you MUST re-run the conversion script.

Usage:
    python3 sst_convert.py --all
    python3 sst_convert.py --source=tomharte
    python3 sst_convert.py --source=raddad
    python3 sst_convert.py --file=ADD.b --source=tomharte

Unified SST binary format (.sst):
    Header:
        char[4]  magic = "SST1"
        uint32   num_vectors
        uint8    source_id       (0=tomharte, 1=raddad)
        uint8    name_len
        char[]   mnemonic

    Per vector:
        uint8    vec_name_len
        char[]   vec_name
        uint32   initial_regs[19]  (D0-D7, A0-A6, USP, SSP, SR, PC)
        uint32   final_regs[19]
        uint16   num_ram_initial
        uint16   num_ram_final
        Per RAM entry: uint32 addr, uint8 value
"""

import os
import sys
import glob
import gzip
import json
import struct
import argparse
from pathlib import Path

SST_MAGIC = b'SST1'
SOURCE_TOMHARTE = 0
SOURCE_RADDAD = 1

REG_ORDER = ['d0', 'd1', 'd2', 'd3', 'd4', 'd5', 'd6', 'd7',
             'a0', 'a1', 'a2', 'a3', 'a4', 'a5', 'a6', 'usp',
             'ssp', 'sr', 'pc']


# ---------------------------------------------------------------------------
# TomHarte / 680x0 loader (gzipped JSON)
# ---------------------------------------------------------------------------

def load_tomharte(filepath):
    """Load a .json.gz file from SingleStepTests/680x0.

    The TomHarte format stores the instruction opcode in the 'prefetch'
    array rather than in RAM. We inject the prefetch words at the PC
    address into the initial RAM so the emulator can fetch them.
    """
    with gzip.open(filepath, 'rt', encoding='utf-8') as f:
        tests = json.load(f)

    vectors = []
    for t in tests:
        ini = t['initial']
        fin = t['final']

        # Build initial RAM from explicit entries
        initial_ram = [(addr, val) for addr, val in ini.get('ram', [])]

        # Inject prefetch words at PC address (big-endian 68000)
        pc = ini['pc']
        prefetch = ini.get('prefetch', [])
        for i, word in enumerate(prefetch):
            addr = pc + i * 2
            initial_ram.append((addr,     (word >> 8) & 0xFF))
            initial_ram.append((addr + 1, word & 0xFF))

        vectors.append({
            'name': t.get('name', ''),
            'initial_regs': [ini[r] for r in REG_ORDER],
            'initial_ram': initial_ram,
            'final_regs': [fin[r] for r in REG_ORDER],
            'final_ram': [(addr, val) for addr, val in fin.get('ram', [])],
        })
    return vectors


# ---------------------------------------------------------------------------
# raddad772 / m68000 loader (proprietary binary)
# ---------------------------------------------------------------------------

def _read_raddad_name(content, ptr):
    numbytes, magic = struct.unpack_from('<II', content, ptr)
    ptr += 8
    assert magic == 0x89ABCDEF, f"Bad name magic: {magic:#x}"
    strlen = struct.unpack_from('<I', content, ptr)[0]
    ptr += 4
    bs = struct.unpack_from(f'{strlen}s', content, ptr)[0]
    ptr += strlen
    return ptr, bs.decode('utf-8')


def _read_raddad_state(content, ptr):
    numbytes, magic = struct.unpack_from('<II', content, ptr)
    ptr += 8
    assert magic == 0x01234567, f"Bad state magic: {magic:#x}"

    regs = []
    for _ in REG_ORDER:
        regs.append(struct.unpack_from('<I', content, ptr)[0])
        ptr += 4

    # skip prefetch (2 x uint32)
    ptr += 8

    # RAM: 16-bit words split into byte pairs
    num_rams = struct.unpack_from('<I', content, ptr)[0]
    ptr += 4
    ram = []
    for _ in range(num_rams):
        addr, data = struct.unpack_from('<IH', content, ptr)
        ptr += 6
        assert addr < 0x1000000
        ram.append((addr, data >> 8))
        ram.append((addr | 1, data & 0xFF))

    return ptr, regs, ram


def _skip_raddad_transactions(content, ptr):
    numbytes, magic = struct.unpack_from('<II', content, ptr)
    assert magic == 0x456789AB
    ptr += 8
    num_cycles, num_transactions = struct.unpack_from('<II', content, ptr)
    ptr += 8
    for _ in range(num_transactions):
        tw, cycles = struct.unpack_from('<BI', content, ptr)
        ptr += 5
        if tw != 0:
            ptr += 20
    return ptr


def load_raddad(filepath):
    """Load a .json.bin file from SingleStepTests/m68000.

    raddad PC is 'm_au' = next prefetch address = instruction_start + 4.
    We subtract 4 to get the true instruction PC that Musashi expects.
    The instruction bytes are already in the RAM array at the correct addresses.
    """
    with open(filepath, 'rb') as f:
        content = f.read()

    ptr = 0
    magic, num_tests = struct.unpack_from('<II', content, ptr)
    assert magic == 0x1A3F5D71, f"Bad file magic: {magic:#x}"
    ptr += 8

    vectors = []
    for _ in range(num_tests):
        numbytes, test_magic = struct.unpack_from('<II', content, ptr)
        assert test_magic == 0xABC12367
        ptr += 8

        ptr, name = _read_raddad_name(content, ptr)
        ptr, initial_regs, initial_ram = _read_raddad_state(content, ptr)
        ptr, final_regs, final_ram = _read_raddad_state(content, ptr)
        ptr = _skip_raddad_transactions(content, ptr)

        # Adjust PC: raddad stores m_au (next prefetch addr = insn_start + 4)
        initial_regs[18] = (initial_regs[18] - 4) & 0xFFFFFFFF
        final_regs[18] = (final_regs[18] - 4) & 0xFFFFFFFF

        vectors.append({
            'name': name,
            'initial_regs': initial_regs,
            'initial_ram': initial_ram,
            'final_regs': final_regs,
            'final_ram': final_ram,
        })
    return vectors


# ---------------------------------------------------------------------------
# Unified .sst writer
# ---------------------------------------------------------------------------

def write_sst(outpath, mnemonic, vectors, source_id):
    """Write vectors to unified .sst binary format."""
    mnem_bytes = mnemonic.encode('utf-8')

    with open(outpath, 'wb') as f:
        # Header
        f.write(SST_MAGIC)
        f.write(struct.pack('<I', len(vectors)))
        f.write(struct.pack('BB', source_id, len(mnem_bytes)))
        f.write(mnem_bytes)

        for vec in vectors:
            # Vector name
            vname = vec['name'].encode('utf-8')[:63]
            f.write(struct.pack('B', len(vname)))
            f.write(vname)

            # Registers
            for val in vec['initial_regs']:
                f.write(struct.pack('<I', val & 0xFFFFFFFF))
            for val in vec['final_regs']:
                f.write(struct.pack('<I', val & 0xFFFFFFFF))

            # RAM
            f.write(struct.pack('<HH',
                                len(vec['initial_ram']),
                                len(vec['final_ram'])))
            for addr, val in vec['initial_ram']:
                f.write(struct.pack('<IB', addr, val & 0xFF))
            for addr, val in vec['final_ram']:
                f.write(struct.pack('<IB', addr, val & 0xFF))


# ---------------------------------------------------------------------------
# Conversion orchestration
# ---------------------------------------------------------------------------

DEFAULT_TOMHARTE_DIR = 'test/singlestep/data/680x0/68000/v1'
DEFAULT_RADDAD_DIR = 'test/singlestep/data/m68000/v1'
DEFAULT_OUTPUT_DIR = 'test/singlestep/unified'


def convert_source(source_name, input_dir, output_subdir, loader_fn,
                   glob_pattern, source_id, strip_suffix):
    """Convert all files from one source."""
    os.makedirs(output_subdir, exist_ok=True)
    files = sorted(glob.glob(os.path.join(input_dir, glob_pattern)))

    if not files:
        print(f"  [{source_name}] No files found in {input_dir}")
        return 0, 0

    total_vectors = 0
    for filepath in files:
        basename = Path(filepath).name
        # Strip suffix to get mnemonic: "ADD.b.json.gz" → "ADD.b"
        mnemonic = basename
        for suffix in strip_suffix:
            if mnemonic.endswith(suffix):
                mnemonic = mnemonic[:-len(suffix)]
                break

        outname = mnemonic.replace('.', '_') + '.sst'
        outpath = os.path.join(output_subdir, outname)

        print(f"  [{source_name:8s}] {mnemonic:20s} -> {outname:25s}", end='', flush=True)
        vectors = loader_fn(filepath)
        write_sst(outpath, mnemonic, vectors, source_id)
        total_vectors += len(vectors)
        print(f"  [{len(vectors):6d} vectors]")

    return len(files), total_vectors


def main():
    parser = argparse.ArgumentParser(
        description='Convert SingleStepTests to unified .sst format')
    parser.add_argument('--all', action='store_true',
                        help='Convert both sources')
    parser.add_argument('--source', choices=['tomharte', 'raddad'],
                        help='Convert only one source')
    parser.add_argument('--file', help='Convert single mnemonic (e.g., ADD.b)')
    parser.add_argument('--tomharte-dir', default=DEFAULT_TOMHARTE_DIR)
    parser.add_argument('--raddad-dir', default=DEFAULT_RADDAD_DIR)
    parser.add_argument('--output-dir', default=DEFAULT_OUTPUT_DIR)
    args = parser.parse_args()

    if not args.all and not args.source and not args.file:
        args.all = True

    print("SingleStepTests Unified Converter")
    print(f"  TomHarte dir: {args.tomharte_dir}")
    print(f"  raddad dir:   {args.raddad_dir}")
    print(f"  Output dir:   {args.output_dir}")
    print()

    total_files = 0
    total_vectors = 0

    do_tomharte = args.all or args.source == 'tomharte'
    do_raddad = args.all or args.source == 'raddad'

    if do_tomharte and os.path.isdir(args.tomharte_dir):
        nf, nv = convert_source(
            'tomharte', args.tomharte_dir,
            os.path.join(args.output_dir, 'tomharte'),
            load_tomharte, '*.json.gz', SOURCE_TOMHARTE,
            ['.json.gz'])
        total_files += nf
        total_vectors += nv

    if do_raddad and os.path.isdir(args.raddad_dir):
        nf, nv = convert_source(
            'raddad', args.raddad_dir,
            os.path.join(args.output_dir, 'raddad'),
            load_raddad, '*.json.bin', SOURCE_RADDAD,
            ['.json.bin'])
        total_files += nf
        total_vectors += nv

    print(f"\nDone: {total_files} files, {total_vectors:,} total vectors")


if __name__ == '__main__':
    main()
