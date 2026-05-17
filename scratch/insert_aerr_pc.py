import re, sys

def patch(filename):
    with open(filename, 'r') as f:
        content = f.read()
        
    # Find MOVE templates (16 and 32 sizes)
    # Match: M68KMAKE_OP(move, 16, ai, d) ... { ... FLAG_N = 
    # We want to insert M68KI_MOVE_AERR_SAVE_PC(); right before FLAG_N
    
    # We only want to patch templates where destination is memory!
    # Destination modes: ai, pi, pd, di, ix, aw, al
    # But wait! If we patch ALL move templates, it doesn't hurt, because register destinations don't fault!
    
    pattern = re.compile(r'(M68KMAKE_OP\(move, (?:16|32), .*?, .*?\)\n\{.*?)(FLAG_N = NFLAG_)', re.DOTALL)
    
    def repl(m):
        return m.group(1) + "M68KI_MOVE_AERR_SAVE_PC();\n\t" + m.group(2)
        
    new_content, count = pattern.subn(repl, content)
    
    if count > 0:
        with open(filename, 'w') as f:
            f.write(new_content)
        print(f"Patched {count} templates.")
    else:
        print("No matches.")

patch(sys.argv[1])
