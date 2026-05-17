#!/usr/bin/env python3
import json, sys
data = json.load(open(sys.argv[1]))
mismatches = {}
for vec in data.get('cycle_mismatches', []):
    opcode = vec.get('opcode', '????')
    exp = vec.get('expected_cycles', 0)
    got = vec.get('actual_cycles', 0)
    key = f'exp={exp} got={got}'
    if key not in mismatches:
        mismatches[key] = []
    mismatches[key].append(opcode)
for k, v in sorted(mismatches.items()):
    print(f'{k}: {len(v)} vectors, sample opcodes: {[hex(x) for x in v[:5]]}')
