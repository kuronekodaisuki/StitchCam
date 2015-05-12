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
#include "WebCam.h"
#include "StitchImage.h"

using namespace std;
using namespace cv;

static void callback(void *pItem, char *pDeviceName)
{
	printf("%s\n", (char*)pDeviceName);
}

const float WIDTH = 320.0;
const float HEIGHT = 240.0;
const float FPS = 15.0;

int main(int argc, char *aargv[])
{
	int count = cudaDeviceCount();
	printf("CUDA device count:%d\n", count);
	count = WebCam::EnumDevices((PENUMDEVICE)&callback, NULL);
	printf("Camera device count:%d\n", count);

	//StitchImage stitcher = StitchImage::createDefault();

	if (count == 1)
	{
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
	}
	else if (2 <= count)
	{
		VideoCapture cam0, cam1;
		cam0.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
		cam0.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
		cam0.set(CV_CAP_PROP_FPS, FPS);
		cam0.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));
		cam1.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
		cam1.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
		cam1.set(CV_CAP_PROP_FPS, FPS);
		cam1.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));
		
		vector<Mat> images;
		try
		{
			bool res;
			res = cam0.open(CV_CAP_V4L2 + 0);
			res = cam0.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
			res = cam0.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
			res = cam0.set(CV_CAP_PROP_FPS, FPS);
			res = cam0.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));
			res = cam1.open(CV_CAP_V4L2 + 1);
			res = cam1.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
			res = cam1.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
			res = cam1.set(CV_CAP_PROP_FPS, FPS);
			res = cam1.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));

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
				if (cam0.isOpened()) {
					cam0 >> image;
					if (image.empty() == false) {
						images.push_back(image);
						imshow("cam0", image);
					}
				}
				if (cam1.isOpened()) {
					cam1 >> image;
					if (image.empty() == false) {
						images.push_back(image);
						imshow("cam1", image);
					}
				}
				images.clear();
			}
			cam0.release();
			cam1.release();
		}
		catch (Exception e)
		{
		}
	}

	return 0;
}


