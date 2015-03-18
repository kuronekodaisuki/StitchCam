//
//
//

#include <nmmintrin.h>
#include "MyBlender.h"

namespace cv {
namespace detail {

void MyBlender::prepare(Rect dst_roi)
{
	dst_.create(dst_roi.size(), CV_8UC3);
    dst_.setTo(Scalar::all(0));
    dst_mask_.create(dst_roi.size(), CV_8U);
    dst_mask_.setTo(Scalar::all(0));
    dst_roi_ = dst_roi;
}

/*
void MyBlender::setRoi(Rect dst_roi)
{
//	dst_.create(dst_roi.size(), CV_8UC3);
//    dst_.setTo(Scalar::all(0));
//    dst_mask_.create(dst_roi.size(), CV_8U);
//    dst_mask_.setTo(Scalar::all(0));
    dst_roi_ = dst_roi;
}
*/

void MyBlender::feed(const Mat &img, const Mat &mask, Point tl)
{
	CV_Assert(img.type() == CV_8UC3);
    CV_Assert(mask.type() == CV_8U);
    int dx = tl.x - dst_roi_.x;
    int dy = tl.y - dst_roi_.y;
#ifdef	_OPENMP
#pragma omp parallel for
#endif
    for (int y = 0; y < img.rows; ++y)
    {
        const Point3_<uchar> *src_row = img.ptr<Point3_<uchar> >(y);
        Point3_<uchar> *dst_row = dst_.ptr<Point3_<uchar> >(dy + y);
        const uchar *mask_row = mask.ptr<uchar>(y);
        uchar *dst_mask_row = dst_mask_.ptr<uchar>(dy + y);

        for (int x = 0; x < img.cols; ++x)
        {
            if (mask_row[x])
                dst_row[dx + x] = src_row[x];
            dst_mask_row[dx + x] |= mask_row[x];
        }
    }
}

void MyBlender::blend(Mat &dst, Mat &dst_mask)
{
	dst_.setTo(Scalar::all(0), dst_mask_ == 0);
    dst = dst_;
    dst_mask = dst_mask_;
    dst_.release();
    dst_mask_.release();
}

} //namespace cv {
} //namespace detail {