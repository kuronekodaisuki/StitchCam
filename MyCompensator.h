//
//
//
#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/ocl/ocl.hpp>
#include <opencv2/gpu/gpumat.hpp>
#include <opencv2/stitching/detail/util.hpp>
#include <opencv2/stitching/detail/exposure_compensate.hpp>


using namespace std;

namespace cv {

namespace gpu {
namespace device {
void cudaApply(gpu::GpuMat &image, double scale);
void cudaApply(Mat &image, double scale);
}	// namespace device
}	// namespace gpu

namespace detail {

class GainCompensatorGpu : public GainCompensator
{
	virtual void apply(int index, Point corner, gpu::GpuMat &image, const gpu::GpuMat &mask) = 0;
};

class MyCompensator : public GainCompensatorGpu
{
public:
	void feed(const vector<Point> &corners, const vector<Mat> &images,
                               const vector<Mat> &masks)
	{
		vector<pair<Mat,uchar> > level_masks;
		for (size_t i = 0; i < masks.size(); ++i)
			level_masks.push_back(make_pair(masks[i], 255));
		feed(corners, images, level_masks);
	}

	void feed(const vector<Point> &corners, const vector<gpu::GpuMat> &images,
                               const vector<Mat> &masks)
	{
		vector<pair<Mat,uchar> > level_masks;
		for (size_t i = 0; i < masks.size(); ++i)
			level_masks.push_back(make_pair(masks[i], 255));
		feed(corners, images, level_masks);
	}

    void feed(const vector<Point> &corners, const vector<Mat> &images,
              const vector<pair<Mat,uchar> > &masks);
	
    void feed(const vector<Point> &corners, const vector<gpu::GpuMat> &images,
              const vector<pair<Mat,uchar> > &masks);

	void feed(const vector<Point> &corners, const vector<Mat> &images,
              const vector<pair<gpu::GpuMat,uchar> > &masks);

    void apply(int index, Point corner, Mat &image, const Mat &mask);

	void apply(int index, Point corner, gpu::GpuMat &image, const gpu::GpuMat &mask);

	MyCompensator(bool useGpu = false);	// constructor

protected:
	Mat_<double> gains_;	// ï‚ê≥íl
	bool useGpu;
	bool openCL;
};

}
}