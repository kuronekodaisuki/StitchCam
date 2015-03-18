#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/stitching/detail/blenders.hpp>

namespace cv {
namespace detail {

class MyBlender : public Blender
{
public:
	//void prepare(const std::vector<Point> &corners, const std::vector<Size> &sizes);
    void prepare(Rect dst_roi);
	//void setRoi(Rect roi);
    void feed(const Mat &img, const Mat &mask, Point tl);
    void blend(Mat &dst, Mat &dst_mask);
};

} //namespace cv {
} //namespace detail {