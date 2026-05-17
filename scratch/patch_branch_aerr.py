import re

with open("m68k_in.c", "r") as f:
    lines = f.readlines()

out = []
i = 0
while i < len(lines):
    line = lines[i]
    if "m68ki_check_pc_address_error_010_less()" in line:
        # Determine context by looking upwards or downwards.
        # Actually it's easier to just do it manually for the handful of cases.
        pass
    out.append(line)
    i += 1

