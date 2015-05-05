#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <opencv2/stitching/detail/blenders.hpp>
#include <opencv2/stitching/detail/util.hpp>

namespace cv {
namespace detail {

class MyBlender : public Blender
{
public:
	MyBlender(int try_gpu = false);
	void prepare(const vector<Point> &corners, const vector<Size> &sizes)
	{
		prepare(resultRoi(corners, sizes));
	}

    void prepare(Rect dst_roi);
    void feed(const Mat &img, const Mat &mask, Point tl);
	void feed(const gpu::GpuMat &img, const gpu::GpuMat &mask, Point tl);
    void blend(Mat &dst, Mat &dst_mask);

protected:
	bool use_gpu;
	gpu::GpuMat gpuDst_;

	void cudaFeed(const gpu::GpuMat &image, const gpu::GpuMat &mask, int dx, int dy); // cuda version feed
	void cudaFeed(const Mat &img, const Mat &mask, int dx, int dy);	// cuda version feed
};

} //namespace cv {
} //namespace detail {