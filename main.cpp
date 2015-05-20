/*
 * main.cpp
 *
 *  Created on: 2015/03/19
 *      Author: Hiroshi Matsuoka
 */

#include <stdio.h>
#include <sys/time.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/gpumat.hpp>
#include <opencv2/stitching/stitcher.hpp>
//#include <opencv2/highgui/highgui.hpp>

#include "WebCam.h"
#include "MyStitcher.h"
#include "MyBlender.h"
#include "MyCompensator.h"
#include "MySeamFinder.h"
#include "MyWarper.h"

using namespace std;
using namespace cv;

uint32_t getTick() {
    struct timespec ts;
    unsigned theTick = 0U;
    clock_gettime( CLOCK_REALTIME, &ts );
    theTick  = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}

static void callback(void *pItem, char *pDeviceName)
{
	printf("%s\n", (char*)pDeviceName);
}

const float WIDTH = 640.0;
const float HEIGHT = 480.0;
const float FPS = 30.0;

int main(int argc, char *aargv[])
{
	int count = gpu::getCudaEnabledDeviceCount();
	printf("CUDA device count:%d\n", count);
	for (int i = 0; i < count; i++)
	{
		gpu::DeviceInfo info(i);
		cout << info.name() << endl;
		//char *name = info.name();
		//printf("NAME: %s\n", name);
	}
	count = WebCam::EnumDevices((PENUMDEVICE)&callback, NULL);
	printf("Camera device count:%d\n", count);

	MyStitcher stitcher = MyStitcher::createDefault(true);
	stitcher.setSeamFinder(new detail::MyVoronoiSeamFinder());
	stitcher.setBlender(new detail::MyBlender(true));
	stitcher.setExposureCompensator(new detail::MyCompensator(true));
	
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
					cam0 >> image;
					if (image.empty() == false) {
						image.copyTo(left);
						images.push_back(left);
						imshow("cam0", image);
					}
				}
				if (cam1.isOpened()) {
					cam1 >> image;
					if (image.empty() == false) {
						image.copyTo(right);
						images.push_back(right);
						imshow("cam1", image);
					}
				}
				uint32_t start;
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
					start = getTick();
					switch (stitcher.stitch(images, panorama))
					{
					case MyStitcher::OK:
						printf("%d msec\n", getTick() - start);
						imshow("Panorama", panorama);
						break;

					case MyStitcher::ERR_NEED_MORE_IMGS:
						printf("Need more IMAGES\n");
						break;

					default:
						printf("Failed to stitch\n");
					}
					break;
				case 'n': // NoSeamFinder
					stitcher.setSeamFinder(new detail::NoSeamFinder());
					printf("NoSeamFinder\n");
					break;
				case 'v': // VoronoiSeamFinder
					stitcher.setSeamFinder(new detail::MyVoronoiSeamFinder());
					printf("VoronoiSeamFinder\n");
					break;
				case 'd': // DpSeamFinder
					stitcher.setSeamFinder(new detail::MySeamFinder());
					printf("DpSeamFinder\n");
					break;
				}

				if (continuous) {
					start = getTick();
					if (MyStitcher::OK == stitcher.composePanorama(images, panorama)) {
						printf("%d msec\n", getTick() - start);
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


