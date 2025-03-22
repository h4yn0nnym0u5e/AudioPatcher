"""
Turn a 256-sample binary file into a C source file
"""
import struct
from os.path import join

filePath = "D:/Jonathan/Teensy/AudioPatcher/SD/arbWAVs"
fileName = "sax.snd"

# Read the data in; quite small so get it all at once
file = open(join(filePath,fileName),"rb")
fileData = file.read(512) # must be this big to be an arbitrary wave
file.close()

# Write a C source file
outFile = open(join(filePath,fileName.replace(".snd",".c")),"w")
outFile.write(f"""#include <Arduino.h>
//
// {fileName}
PROGMEM const int16_t arbWAV_{fileName.replace(".snd","")}[] = 
\t{{
\t""")
n=0
for i in range(0,256):
    vt = struct.unpack_from("<h",fileData,i*2)
    if i == 255:
        outFile.write(f"{vt[0]:6d}")
    else:
        outFile.write(f"{vt[0]:6d}, ")
    n += 1
    if n == 8:
        outFile.write("\n\t")
        n = 0

outFile.write("};\n")


