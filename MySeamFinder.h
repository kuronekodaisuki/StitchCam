#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/stitching/detail/util.hpp>
#include <opencv2/stitching/detail/seam_finders.hpp>
#include <set>

using namespace std;
namespace cv {
namespace detail {

class MySeamFinder : public SeamFinder
{
public:
    enum CostFunction { COLOR, COLOR_GRAD };

    MySeamFinder(CostFunction costFunc = COLOR);

    CostFunction costFunction() const { return costFunc_; }
    void setCostFunction(CostFunction val) { costFunc_ = val; }

    virtual void find(const std::vector<Mat> &src, const std::vector<Point> &corners,
                      std::vector<Mat> &masks);

private:
    enum ComponentState
    {
        FIRST = 1, SECOND = 2, INTERS = 4,
        INTERS_FIRST = INTERS | FIRST,
        INTERS_SECOND = INTERS | SECOND
    };

    class ImagePairLess
    {
    public:
        ImagePairLess(const std::vector<Mat> &images, const std::vector<Point> &corners)
            : src_(&images[0]), corners_(&corners[0]) {}

        bool operator() (const std::pair<size_t, size_t> &l, const std::pair<size_t, size_t> &r) const
        {
            Point c1 = corners_[l.first] + Point(src_[l.first].cols / 2, src_[l.first].rows / 2);
            Point c2 = corners_[l.second] + Point(src_[l.second].cols / 2, src_[l.second].rows / 2);
            int d1 = (c1 - c2).dot(c1 - c2);

            c1 = corners_[r.first] + Point(src_[r.first].cols / 2, src_[r.first].rows / 2);
            c2 = corners_[r.second] + Point(src_[r.second].cols / 2, src_[r.second].rows / 2);
            int d2 = (c1 - c2).dot(c1 - c2);

            return d1 < d2;
        }

    private:
        const Mat *src_;
        const Point *corners_;
    };

    class ClosePoints
    {
    public:
        ClosePoints(int minDist) : minDist_(minDist) {}

        bool operator() (const Point &p1, const Point &p2) const
        {
            int dist2 = (p1.x-p2.x) * (p1.x-p2.x) + (p1.y-p2.y) * (p1.y-p2.y);
            return dist2 < minDist_ * minDist_;
        }

    private:
        int minDist_;
    };

    void process(
            const Mat &image1, const Mat &image2, Point tl1, Point tl2, Mat &mask1, Mat &mask2);

    void findComponents();

    void findEdges();

    void resolveConflicts(
            const Mat &image1, const Mat &image2, Point tl1, Point tl2, Mat &mask1, Mat &mask2);

    void computeGradients(const Mat &image1, const Mat &image2);

    bool hasOnlyOneNeighbor(int comp);

    bool closeToContour(int y, int x, const Mat_<uchar> &contourMask);

    bool getSeamTips(int comp1, int comp2, Point &p1, Point &p2);

    void computeCosts(
            const Mat &image1, const Mat &image2, Point tl1, Point tl2,
            int comp, Mat_<float> &costV, Mat_<float> &costH);

    bool estimateSeam(
            const Mat &image1, const Mat &image2, Point tl1, Point tl2, int comp,
            Point p1, Point p2, std::vector<Point> &seam, bool &isHorizontal);

    void updateLabelsUsingSeam(
            int comp1, int comp2, const std::vector<Point> &seam, bool isHorizontalSeam);

    CostFunction costFunc_;

    // processing images pair data
    Point unionTl_, unionBr_;
    Size unionSize_;
    Mat_<uchar> mask1_, mask2_;
    Mat_<uchar> contour1mask_, contour2mask_;
    Mat_<float> gradx1_, grady1_;
    Mat_<float> gradx2_, grady2_;

    // components data
    int ncomps_;
    Mat_<int> labels_;
    std::vector<ComponentState> states_;
    std::vector<Point> tls_, brs_;
    std::vector<std::vector<Point> > contours_;
    std::set<std::pair<int, int> > edges_;
};

class MyVoronoiSeamFinder : public PairwiseSeamFinder
{
public:
    virtual void find(const std::vector<Size> &size, const std::vector<Point> &corners,
                      std::vector<Mat> &masks);
private:
    void findInPair(size_t first, size_t second, Rect roi);
};

}
}