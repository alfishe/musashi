with open("m68kcpu.h", "r") as f:
    content = f.read()

import re
matches = re.findall(r"static inline uint m68ki_ea_ax_pd_32\(void\)[\s\S]+?\}", content)
for m in matches:
    print(m)
