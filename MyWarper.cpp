#include "MyWarper.h"

namespace cv {

namespace detail {

//////////////////////////////////////////////////////////////////////////
// MyWarperGpu
Point2f MyWarperGpu::warpPoint(const Point2f &pt, const Mat &K, const Mat &R, const Mat &T)
{
    projector_.setCameraParams(K, R, T);
    Point2f uv;
    projector_.mapForward(pt.x, pt.y, uv.x, uv.y);
    return uv;
}

Rect MyWarperGpu::warpRoi(Size src_size, const Mat &K, const Mat &R, const Mat &T)
{
    projector_.setCameraParams(K, R, T);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    return Rect(dst_tl, Point(dst_br.x + 1, dst_br.y + 1));
}


void MyWarperGpu::detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br)
{
    float tl_uf = numeric_limits<float>::max();
    float tl_vf = numeric_limits<float>::max();
    float br_uf = -numeric_limits<float>::max();
    float br_vf = -numeric_limits<float>::max();

    float u, v;

    projector_.mapForward(0, 0, u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    projector_.mapForward(0, static_cast<float>(src_size.height - 1), u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    projector_.mapForward(static_cast<float>(src_size.width - 1), 0, u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    projector_.mapForward(static_cast<float>(src_size.width - 1), static_cast<float>(src_size.height - 1), u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    dst_tl.x = static_cast<int>(tl_uf);
    dst_tl.y = static_cast<int>(tl_vf);
    dst_br.x = static_cast<int>(br_uf);
    dst_br.y = static_cast<int>(br_vf);
}

Rect MyWarperGpu::buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap)
{
    projector_.setCameraParams(K, R, T);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    gpu::buildWarpPlaneMaps(src_size, Rect(dst_tl, Point(dst_br.x + 1, dst_br.y + 1)),
                            K, R, T, projector_.scale, xmap, ymap);

    return Rect(dst_tl, dst_br);
}

Point MyWarperGpu::warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
                        gpu::GpuMat &dst)
{
    //Mat xmap, ymap;
    Rect dst_roi = buildMaps(src.size(), K, R, T, d_xmap_, d_ymap_);
    dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
    gpu::remap(src, dst, d_xmap_, d_ymap_, interp_mode, border_mode);
    return dst_roi.tl();
}

//////////////////////////////////////////////////////////////////////////
// MyCylindricalWarperGpu

void MyCylindricalWarperGpu::detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br)
{
    float tl_uf = std::numeric_limits<float>::max();
    float tl_vf = std::numeric_limits<float>::max();
    float br_uf = -std::numeric_limits<float>::max();
    float br_vf = -std::numeric_limits<float>::max();

    float u, v;
    for (float x = 0; x < src_size.width; ++x)
    {
        projector_.mapForward(static_cast<float>(x), 0, u, v);
        tl_uf = std::min(tl_uf, u); tl_vf = std::min(tl_vf, v);
        br_uf = std::max(br_uf, u); br_vf = std::max(br_vf, v);

        projector_.mapForward(static_cast<float>(x), static_cast<float>(src_size.height - 1), u, v);
        tl_uf = std::min(tl_uf, u); tl_vf = std::min(tl_vf, v);
        br_uf = std::max(br_uf, u); br_vf = std::max(br_vf, v);
    }
    for (int y = 0; y < src_size.height; ++y)
    {
        projector_.mapForward(0, static_cast<float>(y), u, v);
        tl_uf = std::min(tl_uf, u); tl_vf = std::min(tl_vf, v);
        br_uf = std::max(br_uf, u); br_vf = std::max(br_vf, v);

        projector_.mapForward(static_cast<float>(src_size.width - 1), static_cast<float>(y), u, v);
        tl_uf = std::min(tl_uf, u); tl_vf = std::min(tl_vf, v);
        br_uf = std::max(br_uf, u); br_vf = std::max(br_vf, v);
    }

    dst_tl.x = static_cast<int>(tl_uf);
    dst_tl.y = static_cast<int>(tl_vf);
    dst_br.x = static_cast<int>(br_uf);
    dst_br.y = static_cast<int>(br_vf);
}

Rect MyCylindricalWarperGpu::buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap)
{
    projector_.setCameraParams(K, R);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    gpu::buildWarpCylindricalMaps(src_size, Rect(dst_tl, Point(dst_br.x + 1, dst_br.y + 1)),
                                  K, R, projector_.scale, xmap, ymap);

    return Rect(dst_tl, dst_br);
}

/*
Point MyCylindricalWarperGpu::warp(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                                 gpu::GpuMat &dst)
{
    Rect dst_roi = buildMaps(src.size(), K, R, d_xmap_, d_ymap_);
    dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
    gpu::remap(src, dst, d_xmap_, d_ymap_, interp_mode, border_mode);
    return dst_roi.tl();
}
*/

Point MyCylindricalWarperGpu::warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
                        gpu::GpuMat &dst)
{
    Rect dst_roi = buildMaps(src.size(), K, R, T, d_xmap_, d_ymap_);
    dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
    gpu::remap(src, dst, d_xmap_, d_ymap_, interp_mode, border_mode);
    return dst_roi.tl();
}


//////////////////////////////////////////////////////////////////////////
// MyPlaneWarper
Point2f MyPlaneWarper::warpPoint(const Point2f &pt, const Mat &K, const Mat &R, const Mat &T)
{
    projector_.setCameraParams(K, R, T);
    Point2f uv;
    projector_.mapForward(pt.x, pt.y, uv.x, uv.y);
    return uv;
}


Rect MyPlaneWarper::buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, Mat &xmap, Mat &ymap)
{
    projector_.setCameraParams(K, R, T);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    xmap.create(dst_br.y - dst_tl.y + 1, dst_br.x - dst_tl.x + 1, CV_32F);
    ymap.create(dst_br.y - dst_tl.y + 1, dst_br.x - dst_tl.x + 1, CV_32F);

    float x, y;
    for (int v = dst_tl.y; v <= dst_br.y; ++v)
    {
        for (int u = dst_tl.x; u <= dst_br.x; ++u)
        {
            projector_.mapBackward(static_cast<float>(u), static_cast<float>(v), x, y);
            xmap.at<float>(v - dst_tl.y, u - dst_tl.x) = x;
            ymap.at<float>(v - dst_tl.y, u - dst_tl.x) = y;
        }
    }

    return Rect(dst_tl, dst_br);
}

Rect MyPlaneWarper::buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap)
{
    projector_.setCameraParams(K, R, T);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    xmap.create(dst_br.y - dst_tl.y + 1, dst_br.x - dst_tl.x + 1, CV_32F);
    ymap.create(dst_br.y - dst_tl.y + 1, dst_br.x - dst_tl.x + 1, CV_32F);

    float x, y;
    for (int v = dst_tl.y; v <= dst_br.y; ++v)
    {
        for (int u = dst_tl.x; u <= dst_br.x; ++u)
        {
            projector_.mapBackward(static_cast<float>(u), static_cast<float>(v), x, y);
            //xmap.at<float>(v - dst_tl.y, u - dst_tl.x) = x;
            //ymap.at<float>(v - dst_tl.y, u - dst_tl.x) = y;
        }
    }

    return Rect(dst_tl, dst_br);
}


Point MyPlaneWarper::warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
                        Mat &dst)
{
    Mat xmap, ymap;
    Rect dst_roi;// = buildMaps(src.size(), K, R, T, xmap, ymap);

    dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
    remap(src, dst, xmap, ymap, interp_mode, border_mode);

    return dst_roi.tl();
}

Point MyPlaneWarper::warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
	gpu::GpuMat &dst)
{
    Mat xmap, ymap;
    Rect dst_roi;// = buildMaps(src.size(), K, R, T, xmap, ymap);

    dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
    remap(src, dst, xmap, ymap, interp_mode, border_mode);

    return dst_roi.tl();
}

Rect MyPlaneWarper::warpRoi(Size src_size, const Mat &K, const Mat &R, const Mat &T)
{
    projector_.setCameraParams(K, R, T);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    return Rect(dst_tl, Point(dst_br.x + 1, dst_br.y + 1));
}


void MyPlaneWarper::detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br)
{
    float tl_uf = numeric_limits<float>::max();
    float tl_vf = numeric_limits<float>::max();
    float br_uf = -numeric_limits<float>::max();
    float br_vf = -numeric_limits<float>::max();

    float u, v;

    projector_.mapForward(0, 0, u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    projector_.mapForward(0, static_cast<float>(src_size.height - 1), u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    projector_.mapForward(static_cast<float>(src_size.width - 1), 0, u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    projector_.mapForward(static_cast<float>(src_size.width - 1), static_cast<float>(src_size.height - 1), u, v);
    tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
    br_uf = max(br_uf, u); br_vf = max(br_vf, v);

    dst_tl.x = static_cast<int>(tl_uf);
    dst_tl.y = static_cast<int>(tl_vf);
    dst_br.x = static_cast<int>(br_uf);
    dst_br.y = static_cast<int>(br_vf);
}

//////////////////////////////////////////////////////////////////////////
// MyPlaneWarperGpu
Rect MyPlaneWarperGpu::buildMaps(Size src_size, const Mat &K, const Mat &R, gpu::GpuMat &xmap, gpu::GpuMat &ymap)
{
    return buildMaps(src_size, K, R, Mat::zeros(3, 1, CV_32F), xmap, ymap);
}

Rect MyPlaneWarperGpu::buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap)
{
    projector_.setCameraParams(K, R, T);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    gpu::buildWarpPlaneMaps(src_size, Rect(dst_tl, Point(dst_br.x + 1, dst_br.y + 1)),
                            K, R, T, projector_.scale, xmap, ymap);

    return Rect(dst_tl, dst_br);
}

Point MyPlaneWarperGpu::warp(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                           gpu::GpuMat &dst)
{
    return warp(src, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode, dst);
}


Point MyPlaneWarperGpu::warp(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
                           gpu::GpuMat &dst)
{
    Rect dst_roi = buildMaps(src.size(), K, R, T, d_xmap_, d_ymap_);
    dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
    gpu::remap(src, dst, d_xmap_, d_ymap_, interp_mode, border_mode);
    return dst_roi.tl();
}


}	// namespace detail
}	// namespace cv