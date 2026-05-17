import sys, struct

def find_pd(filename):
    with open(filename, "rb") as f:
        data = f.read(4)
        if len(data) < 4: return
        count = struct.unpack(">I", data)[0]
        
        for i in range(count):
            name_bytes = bytearray()
            while True:
                b = f.read(1)
                if not b or b == b'\0': break
                name_bytes += b
            name = name_bytes.decode('ascii')
            
            # Read vector length
            len_bytes = f.read(4)
            if not len_bytes: break
            length = struct.unpack(">I", len_bytes)[0]
            
            if "-(A" in name:
                print(f"Found vector {i}: {name}")
                break
                
            f.seek(length, 1)

find_pd(sys.argv[1])
