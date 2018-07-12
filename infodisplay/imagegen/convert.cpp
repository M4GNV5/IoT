#include <stdio.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		printf("Usage: %s <in file> <out file>\n", argv[0]);
		return 1;
	}

	Mat in = imread(argv[1]);

	if(in.cols != 640 || in.rows != 384 || in.channels() != 3)
	{
		printf("Image is %dx%d not 640x384 pixel\n", in.rows, in.cols);
		return 1;
	}

	FILE *out = fopen(argv[2], "w");

	uint8_t byte = 0;
	int i = 0;

	for(int y = 0; y < 384; y++)
	{
		for(int x = 0; x < 640; x++)
		{
			byte <<= 1;
			Vec3b colour = in.at<Vec3b>(y, x);
			if(colour[0] != 255 || colour[1] != 255 || colour[2] != 255)
				byte |= 1;

			i++;
			if(i == 8)
			{
				fputc(byte, out);

				i = 0;
				byte = 0;
			}
		}
	}

	fclose(out);
	return 0;
}
