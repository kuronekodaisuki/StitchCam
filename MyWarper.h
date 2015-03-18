#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <opencv2/core/gpumat.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/stitching/warpers.hpp>

extern "C"
{
//#include <cuda_runtime_api.h>
//#pragma comment(lib, "cudart.lib")
}

using namespace std;
namespace cv {

namespace detail {

// gpu::Matを出力とするインタフェース
class MyRotationWarper : public RotationWarper
{
public:
    virtual ~MyRotationWarper() {}

    virtual Point2f warpPoint(const Point2f &pt, const Mat &K, const Mat &R) = 0;

    virtual void warpBackward(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                              Size dst_size, Mat &dst) = 0;

    virtual Rect warpRoi(Size src_size, const Mat &K, const Mat &R) = 0;

	///////////////////////////
    virtual Rect buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap) = 0;

    virtual Rect buildMaps(Size src_size, const Mat &K, const Mat &R, gpu::GpuMat &xmap, gpu::GpuMat &ymap) = 0;

	virtual Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap) = 0;

	//////////////////////////
    virtual Point warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                       Mat &dst) = 0;

	virtual Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               gpu::GpuMat &dst) = 0;
			   
	virtual Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               gpu::GpuMat &dst) = 0;

    float getScale() const { return 1.f; }
    void setScale(float) {}
};

template <class P>
class MyRotationWarperBase : public MyRotationWarper
{
public:
    Point2f warpPoint(const Point2f &pt, const Mat &K, const Mat &R);

    void warpBackward(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                      Size dst_size, Mat &dst);

    Rect warpRoi(Size src_size, const Mat &K, const Mat &R);

	///////////////////////////
	Rect buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap);

	Rect buildMaps(Size src_size, const Mat &K, const Mat &R, gpu::GpuMat &xmap, gpu::GpuMat &ymap)
	{
	    Point dst_tl, dst_br;
		return Rect(dst_tl, dst_br);
	}

	Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap)
	{
		return buildMaps(src_size, K, R, Mat::zeros(3, 1, CV_32F), xmap, ymap);
	}

	///////////////////////////
	Point warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                       Mat &dst);

    Point warp_gpu(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
		gpu::GpuMat &dst) { return warp_gpu(src, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode);  }

	 Point warp_gpu(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
		gpu::GpuMat &dst) { return Point(); }

	float getScale() const { return projector_.scale; }
    void setScale(float val) { projector_.scale = val; }

protected:

    // Detects ROI of the destination image. It's correct for any projection.
    virtual void detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br);

    // Detects ROI of the destination image by walking over image border.
    // Correctness for any projection isn't guaranteed.
    void detectResultRoiByBorder(Size src_size, Point &dst_tl, Point &dst_br);

    P projector_;
};

template <class P>
Point2f MyRotationWarperBase<P>::warpPoint(const Point2f &pt, const Mat &K, const Mat &R)
{
    projector_.setCameraParams(K, R);
    Point2f uv;
    projector_.mapForward(pt.x, pt.y, uv.x, uv.y);
    return uv;
}


template <class P>
Rect MyRotationWarperBase<P>::buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap)
{
    projector_.setCameraParams(K, R);

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


template <class P>
Point MyRotationWarperBase<P>::warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                                  Mat &dst)
{
    Mat xmap, ymap;
    Rect dst_roi = buildMaps(src.size(), K, R, xmap, ymap);

    dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
    remap(src, dst, xmap, ymap, interp_mode, border_mode);

    return dst_roi.tl();
}


template <class P>
void MyRotationWarperBase<P>::warpBackward(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
                                         Size dst_size, Mat &dst)
{
    projector_.setCameraParams(K, R);

    Point src_tl, src_br;
    detectResultRoi(dst_size, src_tl, src_br);
    CV_Assert(src_br.x - src_tl.x + 1 == src.cols && src_br.y - src_tl.y + 1 == src.rows);

    Mat xmap(dst_size, CV_32F);
    Mat ymap(dst_size, CV_32F);

    float u, v;
    for (int y = 0; y < dst_size.height; ++y)
    {
        for (int x = 0; x < dst_size.width; ++x)
        {
            projector_.mapForward(static_cast<float>(x), static_cast<float>(y), u, v);
            xmap.at<float>(y, x) = u - src_tl.x;
            ymap.at<float>(y, x) = v - src_tl.y;
        }
    }

    dst.create(dst_size, src.type());
    remap(src, dst, xmap, ymap, interp_mode, border_mode);
}


template <class P>
Rect MyRotationWarperBase<P>::warpRoi(Size src_size, const Mat &K, const Mat &R)
{
    projector_.setCameraParams(K, R);

    Point dst_tl, dst_br;
    detectResultRoi(src_size, dst_tl, dst_br);

    return Rect(dst_tl, Point(dst_br.x + 1, dst_br.y + 1));
}


template <class P>
void MyRotationWarperBase<P>::detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br)
{
    float tl_uf = std::numeric_limits<float>::max();
    float tl_vf = std::numeric_limits<float>::max();
    float br_uf = -std::numeric_limits<float>::max();
    float br_vf = -std::numeric_limits<float>::max();

    float u, v;
    for (int y = 0; y < src_size.height; ++y)
    {
        for (int x = 0; x < src_size.width; ++x)
        {
            projector_.mapForward(static_cast<float>(x), static_cast<float>(y), u, v);
            tl_uf = std::min(tl_uf, u); tl_vf = std::min(tl_vf, v);
            br_uf = std::max(br_uf, u); br_vf = std::max(br_vf, v);
        }
    }

    dst_tl.x = static_cast<int>(tl_uf);
    dst_tl.y = static_cast<int>(tl_vf);
    dst_br.x = static_cast<int>(br_uf);
    dst_br.y = static_cast<int>(br_vf);
}


template <class P>
void MyRotationWarperBase<P>::detectResultRoiByBorder(Size src_size, Point &dst_tl, Point &dst_br)
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


//////////////////////////////////////////////////////////////////////////
// MyWarperGpu
class MyWarperGpu : public MyRotationWarperBase<PlaneProjector>
{
public:
    MyWarperGpu(float scale = 1.f) { projector_.scale = scale; }

    void setScale(float scale) { projector_.scale = scale; }

    Rect warpRoi(Size src_size, const Mat &K, const Mat &R, const Mat &T);

	Rect warpRoi(Size src_size, const Mat &K, const Mat &R)
	{
		return warpRoi(src_size, K, R,  Mat::zeros(3, 1, CV_32F));
	}

    Point2f warpPoint(const Point2f &pt, const Mat &K, const Mat &R, const Mat &T);

	///////////////////////////
	Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap);

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, Mat &xmap, Mat &ymap)
    {
        Rect result = buildMaps(src_size, K, R, T, d_xmap_, d_ymap_);
        //d_xmap_.download(xmap);	// GPUのみで処理する場合、これは不要
        //d_ymap_.download(ymap);	// GPUのみで処理する場合、これは不要
        return result;
    }

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap)
    {
        return buildMaps(src_size, K, R, Mat::zeros(3, 1, CV_32F), xmap, ymap);
    }

	/////////////////////////
    Point warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp_gpu(d_src_, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }

    Point warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp_gpu(d_src_, K, R, T, interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }
	
	Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               gpu::GpuMat &dst)
	{
		return warp_gpu(src, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode, dst);
	}
	
    Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               gpu::GpuMat &dst);

protected:
    void detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br);

	gpu::GpuMat d_xmap_, d_ymap_, d_src_, d_dst_;
};

//////////////////////////////////////////////////////////////////////////
// MyCylindricalWarperGpu
// Projects image onto x * x + z * z = 1 cylinder
// CylindricalWarperGpuの作業用変数がprivateのため、CylindricalWarperから派生させる
class MyCylindricalWarperGpu : public MyRotationWarperBase<CylindricalProjector>
{
public:
    MyCylindricalWarperGpu(float scale = 1.f)
	{	projector_.scale = scale;
		//cudaGetDeviceCount(&cudaDeviceCount);
	}

	/////////////////////////
	Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap);

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, Mat &xmap, Mat &ymap)
    {
        Rect result = buildMaps(src_size, K, R, T, d_xmap_, d_ymap_);
        //d_xmap_.download(xmap);	// GPUのみで処理する場合、これは不要
        //d_ymap_.download(ymap);	// GPUのみで処理する場合、これは不要
        return result;
    }

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap)
    {
        return buildMaps(src_size, K, R, Mat::zeros(3, 1, CV_32F), xmap, ymap);
    }

	/////////////////////////
    Point warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp_gpu(d_src_, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }

    Point warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp_gpu(d_src_, K, R, T, interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }
	
	Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               gpu::GpuMat &dst)
	{
		return warp_gpu(src, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode, dst);
	}
	
    Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               gpu::GpuMat &dst);

protected:
	void detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br);

	int cudaDeviceCount;
	
private:
    gpu::GpuMat d_xmap_, d_ymap_, d_src_, d_dst_;
};


//////////////////////////////////////////////////////////////////////////
// MySphericalWarper
// Projects image onto unit sphere with origin at (0, 0, 0).
// Poles are located at (0, -1, 0) and (0, 1, 0) points.
class MySphericalWarperGpu : public MyRotationWarperBase<SphericalProjector>
{
public:
    MySphericalWarperGpu(float scale) { projector_.scale = scale; }

	/////////////////////////
	Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap);

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, Mat &xmap, Mat &ymap)
    {
        Rect result = buildMaps(src_size, K, R, T, d_xmap_, d_ymap_);
        //d_xmap_.download(xmap);	// GPUのみで処理する場合、これは不要
        //d_ymap_.download(ymap);	// GPUのみで処理する場合、これは不要
        return result;
    }

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap)
    {
        return buildMaps(src_size, K, R, Mat::zeros(3, 1, CV_32F), xmap, ymap);
    }

	/////////////////////////
    Point warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp_gpu(d_src_, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }

    Point warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp_gpu(d_src_, K, R, T, interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }
	
	Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               gpu::GpuMat &dst)
	{
		return warp_gpu(src, K, R, Mat::zeros(3, 1, CV_32F), interp_mode, border_mode, dst);
	}
	
    Point warp_gpu(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               gpu::GpuMat &dst);

protected:
    void detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br);

private:
    gpu::GpuMat d_xmap_, d_ymap_, d_src_, d_dst_;
};
//////////////////////////////////////////////////////////////////////////
// MyPlaneWarper
class MyPlaneWarper : public MyRotationWarperBase<PlaneProjector>
{
public:
    MyPlaneWarper(float scale = 1.f) { projector_.scale = scale; }

    void setScale(float scale) { projector_.scale = scale; }

    Point2f warpPoint(const Point2f &pt, const Mat &K, const Mat &R, const Mat &T);

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, Mat &xmap, Mat &ymap);

    Point warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               Mat &dst);

	Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap);

    Point warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               gpu::GpuMat &dst);

    Rect warpRoi(Size src_size, const Mat &K, const Mat &R, const Mat &T);

protected:
    void detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br);
};

//////////////////////////////////////////////////////////////////////////
// MyPlaneWarperGpu
// PlaneWarperGpuの作業用変数がprivateのため、PlaneWarperから派生させる
class MyPlaneWarperGpu : public MyPlaneWarper//RotationWarperBase<PlaneProjector>
{
public:
    MyPlaneWarperGpu(float scale = 1.f)
	{
		projector_.scale = scale;
	}

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap)
    {
        Rect result = buildMaps(src_size, K, R, d_xmap_, d_ymap_);
        d_xmap_.download(xmap);
        d_ymap_.download(ymap);
        return result;
    }

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, Mat &xmap, Mat &ymap)
    {
        Rect result = buildMaps(src_size, K, R, T, d_xmap_, d_ymap_);
        d_xmap_.download(xmap);
        d_ymap_.download(ymap);
        return result;
    }

    Point warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp(d_src_, K, R, interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }

    Point warp(const Mat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               Mat &dst)
    {
        d_src_.upload(src);
        Point result = warp(d_src_, K, R, T, interp_mode, border_mode, d_dst_);
        d_dst_.download(dst);
        return result;
    }

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, gpu::GpuMat &xmap, gpu::GpuMat &ymap);

    Rect buildMaps(Size src_size, const Mat &K, const Mat &R, const Mat &T, gpu::GpuMat &xmap, gpu::GpuMat &ymap);

    Point warp(const gpu::GpuMat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
               gpu::GpuMat &dst);

    Point warp(const gpu::GpuMat &src, const Mat &K, const Mat &R, const Mat &T, int interp_mode, int border_mode,
               gpu::GpuMat &dst);
protected:
	// copy from projector for remapping
	float scale;		// mapBackward
    float k[9];
    float rinv[9];
    float r_kinv[9];
    float k_rinv[9];	// mapBackward
    float t[3];			// mapBackward

private:
    gpu::GpuMat d_xmap_, d_ymap_, d_src_, d_dst_;
};



}	// namespace detail

//////////////////////////////////////////////////////////////////////////
// WarperCreators
class MyWarperCreator
{
public:
    virtual ~MyWarperCreator() {}
    virtual Ptr<detail::MyRotationWarper> create(float scale) const = 0;
};

class MyWarperGpu: public MyWarperCreator
{
public:
    Ptr<detail::MyRotationWarper> create(float scale) const 
	{
#ifdef	_DEBUG
		cout << "MyWarperGpu" << endl;
#endif
		return new detail::MyWarperGpu(scale); 
	}
};

class MyCylindricalWarperGpu: public MyWarperCreator
{
public:
    Ptr<detail::MyRotationWarper> create(float scale) const 
	{
#ifdef	_DEBUG
		cout << "MyCylindricalWarperGpu" << endl;
#endif
		return new detail::MyCylindricalWarperGpu(scale); 
	}
};

class MySphericalWarperGpu: public MyWarperCreator
{
public:
    Ptr<detail::MyRotationWarper> create(float scale) const 
	{
#ifdef	_DEBUG
		cout << "MyCylindricalWarperGpu" << endl;
#endif
		return new detail::MySphericalWarperGpu(scale); 
	}
};

/*
class MyPlaneWarper: public MyWarperCreator
{
public:
    Ptr<detail::MyRotationWarper> create(float scale) const 
	{
#ifdef	_DEBUG
		cout << "MyPlaneWarper" << endl;
#endif
		return new detail::MyPlaneWarper(scale); 
	}
};

class MyPlaneWarperGpu: public MyWarperCreator
{
public:
    Ptr<detail::MyRotationWarper> create(float scale) const 
	{
#ifdef	_DEBUG
		cout << "MyPlaneWarperGpu" << endl;
#endif
		return new detail::MyPlaneWarperGpu(scale); 
	}
};
*/


}	// namespace cv