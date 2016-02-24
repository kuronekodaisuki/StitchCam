/*
 * main.cpp
 *
 *  Created on: 2015/03/19
 *      Author: mao
 */

#include <stdio.h>
//#include <sys/time.h>

#include <opencv2/opencv.hpp>
#include <opencv2/stitching/stitcher.hpp>
//#include <opencv2/highgui/highgui.hpp>

#include "cuda.h"
#include "WebCam.h"
#include "StitchImage.h"

using namespace std;
using namespace cv;

#ifndef WIN32
uint32_t getTick() {
    struct timespec ts;
    unsigned theTick = 0U;
    clock_gettime( CLOCK_REALTIME, &ts );
    theTick  = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}
#endif

static void callback(void *pItem, char *pDeviceName)
{
	printf("%s\n", (char*)pDeviceName);
}

const float WIDTH = 320.0; // 640.0
const float HEIGHT = 240.0; // 480.0
const float FPS = 30.0;

int main(int argc, char *aargv[])
{
	int count = gpu::getCudaEnabledDeviceCount(); //cudaDeviceCount();
	printf("CUDA device count:%d\n", count);
	count = WebCam::EnumDevices((PENUMDEVICE)&callback, NULL);
	printf("Camera device count:%d\n", count);

	Stitcher stitcher = Stitcher::createDefault();

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
		try
		{
			bool res, continuous = false;
			VideoCapture cam0, cam1;	
			vector<Mat> images;
			Mat left, right;
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
				Mat image, panorama;
				if (cam0.isOpened()) {
					if (cam0.read(image))
					{
						image.copyTo(left);
						images.push_back(left);
						imshow("cam0", image);
					}
				}
				if (cam1.isOpened()) {
					if (cam1.read(image))
					{
						image.copyTo(right);
						images.push_back(right);
						imshow("cam1", image);
					}
				}
#ifndef WIN32
				uint32_t start;
#endif
				char key = (char)waitKey(10);
				switch (key)
				{
				case 'q':
				case 'Q':
					loop = false;
					break;

				case 's':
				case 'S':
					continuous = !continuous;
					break;

				case 'c':
				case 'C':
#ifndef WIN32
					start = getTick();
#endif
					switch (stitcher.stitch(images, panorama))
					{
					case Stitcher::OK:
#ifndef WIN32
						printf("%d msec\n", getTick() - start);
#endif
						imshow("Panorama", panorama);
						break;

					case Stitcher::ERR_NEED_MORE_IMGS:
						printf("Need more IMAGES\n");
						break;

					//case Stitcher::ORIG_RESOL:
					//	printf("Resolution\n");
					//	break;

					default:
						printf("Failed to stitch\n");
					}
				}

				if (continuous) {
#ifndef WIN32
					start = getTick();
#endif
					if (Stitcher::OK == stitcher.composePanorama(images, panorama)) {
#ifndef WIN32
						printf("%d msec\n", getTick() - start);
#endif
						imshow("Panorama", panorama);
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

