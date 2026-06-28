

import math


desiredPlanetRadiusPixels = 85         # max 1/2 of scanline count (198)

print("const int pix" + str(desiredPlanetRadiusPixels) + "[] = {")

textureWidth = 24
pixelRatio = 6.9 #6.23

full180 = desiredPlanetRadiusPixels * 2
for line in range(0, full180, 3):
    v = line - desiredPlanetRadiusPixels
    offsetText = math.sqrt(desiredPlanetRadiusPixels * desiredPlanetRadiusPixels - (v * v)) / pixelRatio
    offset = int(offsetText+0.5)

    bits = 0
    for subPix in range(0, offset):

        realAngle = math.degrees(math.asin((subPix + 0.5)/offset))
        texturePix = realAngle * (textureWidth) / 90
        bits |= (1 << (19-int(texturePix )))

    print(format(bits, '#032b') + ', //', line)
print("};")


print()
print("const short int line" + str(desiredPlanetRadiusPixels) + "[] = {")

planetHeight = 20 * 24

for line in range(0, full180, 3):
    v = line - desiredPlanetRadiusPixels
    angle = math.degrees(math.acos(-v/desiredPlanetRadiusPixels))
    length = int(angle * planetHeight / 180)
    length = int(length / 3) * 3

    row = int(length/24)
    col = (length - row * 24) / 3
    col = int(col) * 3
    length = (row << 5) + col

    print(int(length), ', //', line)
print("-1,")
print("};")
