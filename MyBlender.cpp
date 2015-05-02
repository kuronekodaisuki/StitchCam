//
//
//

//#include <nmmintrin.h>
#include "MyBlender.h"

namespace cv {
namespace detail {

MyBlender::MyBlender(int try_gpu)
{
	use_gpu = false;
/*	
	if (try_gpu)
	{
		if (gpu::getCudaEnabledDeviceCount() > 0)
		{
			use_gpu = true;
		}
	}
*/
}

void MyBlender::prepare(Rect dst_roi)
{
	dst_.create(dst_roi.size(), CV_8UC3);
    dst_.setTo(Scalar::all(0));
	if (use_gpu)
	{
		gpuDst_.upload(dst_);
	}
    dst_mask_.create(dst_roi.size(), CV_8U);
    dst_mask_.setTo(Scalar::all(0));
    dst_roi_ = dst_roi;
}

void MyBlender::feed(const Mat &img, const Mat &mask, Point tl)
{
	CV_Assert(img.type() == CV_8UC3);
    CV_Assert(mask.type() == CV_8U);
    int dx = tl.x - dst_roi_.x;
    int dy = tl.y - dst_roi_.y;

	if (use_gpu)
	{
		cudaFeed(img, mask, dx, dy);
	}
	else
	{
#ifdef	_OPENMP
#		pragma omp parallel for
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
}

void MyBlender::blend(Mat &dst, Mat &dst_mask)
{
	if (use_gpu)
	{
		gpuDst_.download(dst);
		gpuDst_.release();
	}
	else 
	{
		dst_.setTo(Scalar::all(0), dst_mask_ == 0);
		dst = dst_;
	}
    dst_mask = dst_mask_;
    dst_.release();
    dst_mask_.release();
}

} //namespace cv {
} //namespace detail {