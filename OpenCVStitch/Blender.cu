#include "device_launch_parameters.h"
#include <opencv2/gpu/device/saturate_cast.hpp>

#include "../MyBlender.h"

#include <stdio.h>

using namespace std;

__global__ void kernelFeed(int rows, int cols, uchar *dst, const uchar *src, const uchar *mask, int dStep, int sStep, int mStep)
{
	int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
	int offset = x + y * mStep;
	if (x < cols && y < rows && mask[offset])
	{
		int dOffset = x * 3 + y * dStep;
		offset = x * 3 + y * sStep;
		dst[dOffset] = src[offset];
		dst[dOffset + 1] = src[offset + 1];
		dst[dOffset + 2] = src[offset + 2];
	}
}

namespace cv {
namespace detail {

	void MyBlender::cudaFeed(const gpu::GpuMat &image, const gpu::GpuMat &mask, int dx, int dy)
	{
		dim3 threads(16, 16);	// 256 threads yealds better performance
		dim3 blocks(image.cols / threads.x, image.rows / threads.y);
		kernelFeed<<<blocks, threads>>>(image.rows, image.cols, 
			gpuDst_.ptr<uchar>(dy) + dx * 3, image.ptr<uchar>(), mask.ptr<uchar>(), 
			gpuDst_.step, image.step, mask.step);
	}

	void MyBlender::cudaFeed(const Mat &image, const Mat &mask, int dx, int dy)
	{
		gpu::GpuMat gpuImg;
		gpu::GpuMat gpuMask;
		gpuImg.upload(image);
		gpuMask.upload(mask);
		cudaFeed(gpuImg, gpuMask, dx, dy);
	}

}	// namespace detail
}	// namespace cv;