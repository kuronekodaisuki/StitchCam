#include <opencv2/stitching/warpers.hpp>

namespace cv {
namespace detail {

class RotationWarperGpu : public RotationWarper
{
    Point warp(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
       gpu::GpuMat &dst);
};

} // namespace detail {
} // namespace cv
