import struct
import sys

def parse_sst(filename):
    with open(filename, "rb") as f:
        data = f.read()

    idx = 0
    header = data[:4]
    if header == b"SST2":
        idx += 4
        # skip names
        while idx < len(data) and data[idx] != 0:
            idx += 1
        idx += 1
        
    num_aerr = 0
    
    while idx < len(data):
        vec_start = idx
        idx += 4 * 16 # D0-D7, A0-A7
        idx += 3 * 4 # USP, SSP, PC
        idx += 2 # SR
        
        # skip initial memory
        count = struct.unpack(">H", data[idx:idx+2])[0]
        idx += 2 + count * 6
        
        # Read final PC and SSP
        idx += 4 * 16
        idx += 4 # USP
        final_ssp = struct.unpack(">I", data[idx:idx+4])[0]
        idx += 4
        final_sr = struct.unpack(">H", data[idx:idx+2])[0]
        idx += 2
        final_pc = struct.unpack(">I", data[idx:idx+4])[0]
        idx += 4
        
        count = struct.unpack(">H", data[idx:idx+2])[0]
        idx += 2 + count * 6
        
        if final_ssp != 0x800: # Assuming initial SSP is 0x800 usually
            num_aerr += 1
            
    print(f"File {filename} has {num_aerr} vectors that changed SSP")

parse_sst(sys.argv[1])
