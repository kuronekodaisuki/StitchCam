#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <opencv2/stitching/detail/blenders.hpp>

namespace cv {
namespace detail {

class MyBlender : public Blender
{
public:
	MyBlender(int try_gpu = false);
    void prepare(Rect dst_roi);
    void feed(const Mat &img, const Mat &mask, Point tl);
    void blend(Mat &dst, Mat &dst_mask);

protected:
	bool use_gpu;
	gpu::GpuMat gpuDst_;

	void cudaFeed(const Mat &img, const Mat &mask, int dx, int dy);	// cuda version feed
};

} //namespace cv {
} //namespace detail {