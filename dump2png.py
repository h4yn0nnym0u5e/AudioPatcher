from os import path, walk
import re
from PIL import Image

fp = "S:/jonathan/Teensy/SD-copies/SanDisk Extreme Pro 64Gb/dumps"
fn = "00006.bin"

def convert(fp,fn):
    on = fn.replace(".bin",".png")
    op = path.join(fp,on)
    if not path.isfile(op):

        img = Image.new("RGB", (320, 240))

        f = open(path.join(fp,fn),"rb")
        for y in range(0,240):
            for x in range(0,320):
                bb = f.read(2)
                #print(b)
                r = bb[1] & 0xF8
                g = (bb[1] & 0x07)*0x20 + (bb[0] & 0xE0)//0x20
                b = (bb[0] & 0x1F)*0x08
                img.putpixel((x,y),(r,g,b))
        f.close()

        img.save(op)

for root,dirs,files in walk(fp):
    for file in files:
        if re.search("[.]bin$", file):
            print(file)
            convert(root,file)
