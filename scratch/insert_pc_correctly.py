import re, sys

def patch(filename):
    with open(filename, 'r') as f:
        content = f.read()
        
    # First, remove the previously inserted M68KI_MOVE_AERR_SAVE_PC();
    content = content.replace("\tM68KI_MOVE_AERR_SAVE_PC();\n", "")
    content = content.replace("M68KI_MOVE_AERR_SAVE_PC();\n\t", "")
    content = content.replace("M68KI_MOVE_AERR_SAVE_PC();", "")
    
    # Now, find the 'uint res = ...;' and insert it immediately after.
    # But ONLY in MOVE templates!
    # A MOVE template looks like:
    # M68KMAKE_OP(move, size, dst, src)
    # {
    #     uint res = ...;
    #     uint ea = ...;
    
    def replacer(match):
        block = match.group(0)
        # Find 'uint res = ...;\n'
        res_match = re.search(r'(uint res = .*?;\n)', block)
        if res_match:
            # Insert after 'res'
            inserted = block.replace(res_match.group(1), res_match.group(1) + "\tM68KI_MOVE_AERR_SAVE_PC();\n")
            return inserted
        return block
        
    pattern = re.compile(r'M68KMAKE_OP\(move, (?:16|32), .*?, .*?\)\n\{.*?FLAG_N = NFLAG_', re.DOTALL)
    
    new_content, count = pattern.subn(replacer, content)
    
    if count > 0:
        with open(filename, 'w') as f:
            f.write(new_content)
        print(f"Patched {count} templates.")
    else:
        print("No matches.")

patch(sys.argv[1])
