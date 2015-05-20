#include "MyCompensator.h"

using namespace std;

namespace cv {



namespace detail {

MyCompensator::MyCompensator(bool useGpu)
{
	this->useGpu = useGpu;
}

void MyCompensator::feed(const vector<Point> &corners, const vector<Mat> &images,
                           const vector<pair<Mat,uchar> > &masks)
{
//	LOGLN("Exposure compensation...");
#if ENABLE_LOG
    int64 t = getTickCount();
#endif

    CV_Assert(corners.size() == images.size() && images.size() == masks.size());

    const int num_images = static_cast<int>(images.size());
    Mat_<int> N(num_images, num_images); N.setTo(0);
    Mat_<double> I(num_images, num_images); I.setTo(0);

    //Rect dst_roi = resultRoi(corners, images);
    Mat subimg1, subimg2;
    Mat_<uchar> submask1, submask2, intersect;

    for (int i = 0; i < num_images; ++i)
    {
        for (int j = i; j < num_images; ++j)
        {
            Rect roi;
            if (overlapRoi(corners[i], corners[j], images[i].size(), images[j].size(), roi))
            {
                subimg1 = images[i](Rect(roi.tl() - corners[i], roi.br() - corners[i]));
                subimg2 = images[j](Rect(roi.tl() - corners[j], roi.br() - corners[j]));

                submask1 = masks[i].first(Rect(roi.tl() - corners[i], roi.br() - corners[i]));
                submask2 = masks[j].first(Rect(roi.tl() - corners[j], roi.br() - corners[j]));
                intersect = (submask1 == masks[i].second) & (submask2 == masks[j].second);

                N(i, j) = N(j, i) = max(1, countNonZero(intersect));

                double Isum1 = 0, Isum2 = 0;
#ifdef	_OPENMP
#				pragma omp parallel for reduction(+: Isum1, Isum2)
#endif
                for (int y = 0; y < roi.height; ++y)
                {
                    const Point3_<uchar>* r1 = subimg1.ptr<Point3_<uchar> >(y);
                    const Point3_<uchar>* r2 = subimg2.ptr<Point3_<uchar> >(y);
                    for (int x = 0; x < roi.width; ++x)
                    {
                        if (intersect(y, x))
                        {
                            Isum1 += sqrt(static_cast<double>(sqr(r1[x].x) + sqr(r1[x].y) + sqr(r1[x].z)));
                            Isum2 += sqrt(static_cast<double>(sqr(r2[x].x) + sqr(r2[x].y) + sqr(r2[x].z)));
                        }
                    }
                }
                I(i, j) = Isum1 / N(i, j);
                I(j, i) = Isum2 / N(i, j);
            }
        }
    }

    double alpha = 0.01;
    double beta = 100;

    Mat_<double> A(num_images, num_images); A.setTo(0);
    Mat_<double> b(num_images, 1); b.setTo(0);
    for (int i = 0; i < num_images; ++i)
    {
        for (int j = 0; j < num_images; ++j)
        {
            b(i, 0) += beta * N(i, j);
            A(i, i) += beta * N(i, j);
            if (j == i) continue;
            A(i, i) += 2 * alpha * I(i, j) * I(i, j) * N(i, j);
            A(i, j) -= 2 * alpha * I(i, j) * I(j, i) * N(i, j);
        }
    }

    solve(A, b, gains_);

    LOGLN("Exposure compensation, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec");
}


void MyCompensator::apply(int index, Point corner, Mat &image, const Mat &mask)
{
	if (useGpu)
	{
		cv::gpu::device::cudaApply(image, gains_(index, 0));
	}
	else
	{
		image *= gains_(index, 0);
	}
#ifdef	_DEBUG
	cout << "GAIN" << index << ": " << gains_(index, 0) << endl;	// 
#endif
}

void MyCompensator::apply(int index, Point corner, gpu::GpuMat &image, const gpu::GpuMat &mask)
{
	if (useGpu)
	{
		cv::gpu::device::cudaApply(image, gains_(index, 0));
	}
#ifdef	_DEBUG
	cout << "GAIN" << index << ": " << gains_(index, 0) << endl;	// 
#endif
}

}	// namespace detail
}	// namespace cv
