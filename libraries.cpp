//
// separeted opencv library linkage
//
#ifdef _MSC_VER

extern "C"
{
#include <cuda_runtime_api.h>
#pragma comment(lib, "cudart.lib")
}

#pragma comment(lib, "OpenCL.lib")


#	ifdef _DEBUG
#pragma comment(lib, "opencv_calib3d2410d.lib")
#pragma comment(lib, "opencv_core2410d.lib")
#pragma comment(lib, "opencv_features2d2410d.lib")
#pragma comment(lib, "opencv_flann2410d.lib")
#pragma comment(lib, "opencv_gpu2410d.lib")
#pragma comment(lib, "opencv_highgui2410d.lib")
#pragma comment(lib, "opencv_imgproc2410d.lib")
#pragma comment(lib, "opencv_legacy2410d.lib")
#pragma comment(lib, "opencv_ml2410d.lib")
#pragma comment(lib, "opencv_nonfree2410d.lib")
#pragma comment(lib, "opencv_objdetect2410d.lib")
#pragma comment(lib, "opencv_ocl2410d.lib")
#pragma comment(lib, "opencv_stitching2410d.lib")
#pragma comment(lib, "opencv_video2410d.lib")
#	else
#pragma comment(lib, "opencv_calib3d2410.lib")
#pragma comment(lib, "opencv_core2410.lib")
#pragma comment(lib, "opencv_features2d2410.lib")
#pragma comment(lib, "opencv_flann2410.lib")
#pragma comment(lib, "opencv_gpu2410.lib")
#pragma comment(lib, "opencv_highgui2410.lib")
#pragma comment(lib, "opencv_imgproc2410.lib")
#pragma comment(lib, "opencv_legacy2410.lib")
#pragma comment(lib, "opencv_ml2410.lib")
#pragma comment(lib, "opencv_nonfree2410.lib")
#pragma comment(lib, "opencv_objdetect2410.lib")
#pragma comment(lib, "opencv_ocl2410.lib")
#pragma comment(lib, "opencv_stitching2410.lib")
#pragma comment(lib, "opencv_video2410.lib")
#	endif
//#pragma comment(lib, "freeglut.lib")
#endif

