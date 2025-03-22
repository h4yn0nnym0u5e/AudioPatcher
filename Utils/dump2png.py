from os import path, walk
import re
from PIL import Image

filePath = "S:/jonathan/Teensy/SD-copies/SanDisk Extreme Pro 64Gb/dumps"
fileName = "00006.bin"

def convert(filePath,fileName):
    outputName = fileName.replace(".bin",".png")
    outputPath = path.join(filePath,outputName)
    if not path.isfile(outputPath):

        img = Image.new("RGB", (320, 240))

        inFile = open(path.join(filePath,fileName),"rb")
        for y in range(0,240):
            for x in range(0,320):
                thePixel = inFile.read(2)
                #print(b)
                r = thePixel[1] & 0xF8
                g = (thePixel[1] & 0x07)*0x20 + (thePixel[0] & 0xE0)//0x20
                b = (thePixel[0] & 0x1F)*0x08
                img.putpixel((x,y),(r,g,b))
        inFile.close()

        img.save(outputPath)

for root,dirs,files in walk(filePath):
    for file in files:
        if re.search("[.]bin$", file):
            print(file)
            convert(root,file)
