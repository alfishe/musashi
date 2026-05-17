import struct, sys, os

def find(filename):
    with open(filename, "rb") as f:
        data = f.read(4)
        if not data: return
        num_vectors = struct.unpack("I", data)[0]
        for i in range(num_vectors):
            vname = f.read(64).decode("latin-1").strip("\0")
            vmnem = f.read(32).decode("latin-1").strip("\0")
            
            # skip initial regs
            f.seek(19*4, os.SEEK_CUR)
            # read initial ram
            num_ram = struct.unpack("I", f.read(4))[0]
            f.seek(num_ram * 5, os.SEEK_CUR)
            
            # read final regs
            f.seek(16*4, os.SEEK_CUR)
            ssp = struct.unpack("I", f.read(4))[0]
            if ssp < 0x800:
                print(f"Vector {i} ({vname}) has AERR! SSP={hex(ssp)}")
            
            f.seek(2*4, os.SEEK_CUR)
            # read final ram
            num_ram = struct.unpack("I", f.read(4))[0]
            f.seek(num_ram * 5, os.SEEK_CUR)

find(sys.argv[1])
