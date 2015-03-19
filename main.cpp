/*
 * main.cpp
 *
 *  Created on: 2015/03/19
 *      Author: mao
 */
#include <stdio.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "cuda.h"

using namespace std;
using namespace cv;

int main(int argc, char *aargv[])
{
	int count = cudaDeviceCount();
	printf("CUDA device count:%d\n", count);
	VideoCapture cam;
	cam.open(0);
	for (bool loop = true; loop; )
	{
		char key = (char)waitKey(10);
		switch (key)
		{
		case 'q':
		case 'Q':
			loop = false;
			break;
		}
		Mat image;
		cam >> image;
		imshow("cam", image);
	}
	cam.release();
	return 0;
}


