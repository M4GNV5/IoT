import cv2
import sys

img = cv2.imread(sys.argv[1])
height, width, channel = img.shape

if width != 640 or height != 384:
	print "Image size must be 640x384"
	exit(1)

fd = open(sys.argv[2], "wb")

i = 0
val = 0
for y in range(0, 384):
	row = []
	for x in range(0, 640):
		val = val << 1
		
		b, g, r = img[y][x]
		if r != 255 or g != 255 or b != 255:
			val = val | 1

		i = i + 1
		if i == 8:
			row.append(val)
			i = 0
			val = 0

	fd.write(bytearray(row))

fd.close()
