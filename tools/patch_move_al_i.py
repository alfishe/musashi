#!/usr/bin/env python3
"""Post-generation patch for m68kops.c.

The m68kmake code generator produces MOVE al_i handlers with
m68ki_aerr_pc_offset = -2, but immediate-source variants already have
REG_PC fully advanced (all extension words consumed before the write),
so no offset correction is needed.

This script removes the offset lines from m68k_op_move_{16,32}_al_i().
"""
import re, sys

def patch(path):
    t = open(path).read()
    for fn in ('m68k_op_move_16_al_i', 'm68k_op_move_32_al_i'):
        pat = (
            r'(static void ' + fn + r'\(void\)\s*\{.*?)'
            r'm68ki_aerr_pc_offset\s*=\s*-2;\s*'
            r'(m68ki_write_\d+\(ea,\s*res\);)\s*'
            r'm68ki_aerr_pc_offset\s*=\s*0;'
        )
        t, n = re.subn(pat, r'\1\2', t, flags=re.DOTALL)
        print(f'  {fn}: {n} substitution(s)')
    open(path, 'w').write(t)

if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'm68kops.c'
    patch(path)
