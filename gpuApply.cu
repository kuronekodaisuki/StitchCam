#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <opencv2/gpu/device/saturate_cast.hpp>

#include "MyCompensator.h"

//#include <stdio.h>

using namespace std;

namespace cv {
namespace gpu {
namespace device {

//////////////////////////////////////////////////////////////////////////
// Exposure compensate kernel
template<typename T>
__global__ void applyKernel(int rows, int cols, T *ptr, int step, float scale)
{
	int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
	int offset = x * 3 + y * step;
	if (x < cols && y < rows)
    {
		ptr[offset] = saturate_cast<T>(scale * ptr[offset]);
		ptr[offset + 1] = saturate_cast<T>(scale * ptr[offset + 1]);
		ptr[offset + 2] = saturate_cast<T>(scale * ptr[offset + 2]);
    }
}


void cudaApply(gpu::GpuMat &image, double scale)
{
	int rows = image.rows;
	int cols = image.cols;
	dim3 threads(16, 16);	// 256 threads yealds better performance
	dim3 grid(cols / threads.x, rows / threads.y);

	switch (image.type())
	{
	case CV_8UC3:
		applyKernel<<<grid, threads>>>(rows, cols, (uchar *)image.datastart, image.step, scale);
		cudaDeviceSynchronize();
		break;
	case CV_16SC3:
		applyKernel<<<grid, threads>>>(rows, cols, (short *)image.datastart, image.step, scale);
		cudaDeviceSynchronize();
		break;
	}
}

void cudaApply(Mat &image, double scale)
{
	gpu::GpuMat gpuMat;
	gpuMat.upload(image);
	cudaApply(gpuMat, scale);
	gpuMat.download(image);
}
}	// namespace device
}	// namespace gpu
}	// namespace cv;
