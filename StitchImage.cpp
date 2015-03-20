// StitchImage.cpp : 
//

//#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
//#include <timer.h>	// $(CudaToolkitDir)\common\inc

#include "opencv2/opencv_modules.hpp"
#include <opencv2/gpu/gpu.hpp>
//#include <opencv2/core/gpumat.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/stitching/stitcher.hpp>
#include "opencv2/stitching/detail/autocalib.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/stitching/detail/camera.hpp"
#include "opencv2/stitching/detail/exposure_compensate.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/detail/seam_finders.hpp"
#include "opencv2/stitching/detail/util.hpp"
#include "opencv2/stitching/detail/warpers.hpp"
#include "opencv2/stitching/warpers.hpp"

#include "StitchImage.h"

#pragma warning(disable:4996)

using namespace std;
using namespace cv;
using namespace cv::detail;
using namespace cv::gpu;

// Default command line args
//vector<string> img_names;
bool preview = false;
bool try_gpu = true;
static double work_megapix = 0.08;	// 0.08
static double seam_megapix = 0.08;	// 0.08
double compose_megapix = -1;
static float conf_thresh = 0.5f;	// 0.5f
string features_type = "surf";
string ba_cost_func = "ray";
string ba_refine_mask = "xxxxx";
bool do_wave_correct = true;
WaveCorrectKind wave_correct = WAVE_CORRECT_HORIZ;
bool save_graph = false;
std::string save_graph_to;
string warp_type = "cylindrical";	// "cylindrical"
int expos_comp_type = ExposureCompensator::GAIN_BLOCKS;	//GAIN
float match_conf = 0.3f;
string seam_find_type = "gc_color";	// "dp_colorgrad"
int blend_type = Blender::MULTI_BAND;
float blend_strength = 5;
string result_name = "result.jpg";

namespace cv {
StitchImage StitchImage::createDefault(bool try_use_gpu)
{
    StitchImage stitcher;
    stitcher.setRegistrationResol(work_megapix);
    stitcher.setSeamEstimationResol(seam_megapix);
    stitcher.setCompositingResol(ORIG_RESOL);
    stitcher.setPanoConfidenceThresh(conf_thresh);
    stitcher.setWaveCorrection(true);
    stitcher.setWaveCorrectKind(detail::WAVE_CORRECT_HORIZ);
    stitcher.setFeaturesMatcher(new detail::BestOf2NearestMatcher(try_use_gpu));
    stitcher.setBundleAdjuster(new detail::BundleAdjusterRay());
	//stitcher.w_ = new MyPlaneWarper();

#if defined(HAVE_OPENCV_GPU) && !defined(DYNAMIC_CUDA_SUPPORT)
    if (try_use_gpu && gpu::getCudaEnabledDeviceCount() > 0)
    {
#if defined(HAVE_OPENCV_NONFREE)
        stitcher.setFeaturesFinder(new detail::SurfFeaturesFinderGpu());
#else
        stitcher.setFeaturesFinder(new detail::OrbFeaturesFinder());
#endif
		stitcher.setWarper(new cv::MyCylindricalWarperGpu());
        stitcher.setSeamFinder(new detail::MySeamFinder(detail::MySeamFinder::COLOR_GRAD));
    }
    else
#endif
    {
#ifdef HAVE_OPENCV_NONFREE
        stitcher.setFeaturesFinder(new detail::SurfFeaturesFinder());
#else
        stitcher.setFeaturesFinder(new detail::OrbFeaturesFinder());
#endif
        stitcher.setWarper(new cv::MyCylindricalWarperGpu());
        stitcher.setSeamFinder(new detail::GraphCutSeamFinder(detail::GraphCutSeamFinderBase::COST_COLOR));
    }

    stitcher.setExposureCompensator(new MyCompensator(try_use_gpu));
    stitcher.setBlender(new MyBlender());

    return stitcher;
}


StitchImage::Status StitchImage::estimateTransform(InputArray images)
{
    return estimateTransform(images, vector<vector<Rect> >());
}


StitchImage::Status StitchImage::estimateTransform(InputArray images, const vector<vector<Rect> > &rois)
{
    images.getMatVector(imgs_);
    rois_ = rois;

    Status status;

    if ((status = matchImages()) != OK)
        return status;

    estimateCameraParams();
	cameras_save = cameras_;
	warped_image_scale_save = warped_image_scale_;
    return OK;
}


StitchImage::Status StitchImage::stitch(InputArray images, OutputArray pano)
{
    Status status = estimateTransform(images);
    if (status != OK)
        return status;
    return composePanorama(pano);
}


StitchImage::Status StitchImage::stitch(InputArray images, const vector<vector<Rect> > &rois, OutputArray pano)
{
    Status status = estimateTransform(images, rois);
    if (status != OK)
        return status;
    return composePanorama(pano);
}


StitchImage::Status StitchImage::matchImages()
{
    if ((int)imgs_.size() < 2)
    {
        LOGLN("Need more images");
        return ERR_NEED_MORE_IMGS;
    }

    work_scale_ = 1;
    seam_work_aspect_ = 1;
    seam_scale_ = 1;
    bool is_work_scale_set = false;
    bool is_seam_scale_set = false;
    Mat full_img, img;
    features_.resize(imgs_.size());
    seam_est_imgs_.resize(imgs_.size());
    full_img_sizes_.resize(imgs_.size());

    LOGLN("Finding features...");
#if ENABLE_LOG
    int64 t = getTickCount();
#endif

    for (size_t i = 0; i < imgs_.size(); ++i)
    {
        full_img = imgs_[i];
        full_img_sizes_[i] = full_img.size();

        if (registr_resol_ < 0)
        {
            img = full_img;
            work_scale_ = 1;
            is_work_scale_set = true;
        }
        else
        {
            if (!is_work_scale_set)
            {
                work_scale_ = min(1.0, sqrt(registr_resol_ * 1e6 / full_img.size().area()));
                is_work_scale_set = true;
            }
            resize(full_img, img, Size(), work_scale_, work_scale_);
        }
        if (!is_seam_scale_set)
        {
            seam_scale_ = min(1.0, sqrt(seam_est_resol_ * 1e6 / full_img.size().area()));
            seam_work_aspect_ = seam_scale_ / work_scale_;
            is_seam_scale_set = true;
        }

        if (rois_.empty())
            (*features_finder_)(img, features_[i]);
        else
        {
            vector<Rect> rois(rois_[i].size());
            for (size_t j = 0; j < rois_[i].size(); ++j)
            {
                Point tl(cvRound(rois_[i][j].x * work_scale_), cvRound(rois_[i][j].y * work_scale_));
                Point br(cvRound(rois_[i][j].br().x * work_scale_), cvRound(rois_[i][j].br().y * work_scale_));
                rois[j] = Rect(tl, br);
            }
            (*features_finder_)(img, features_[i], rois);
        }
        features_[i].img_idx = (int)i;
        LOGLN("Features in image #" << i+1 << ": " << features_[i].keypoints.size());

        resize(full_img, img, Size(), seam_scale_, seam_scale_);
        seam_est_imgs_[i] = img.clone();
		//img.copyTo(seam_est_imgs_[i]);
    }

    // Do it to save memory
    features_finder_->collectGarbage();
    full_img.release();
    img.release();

    LOGLN("Finding features, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec");

    LOG("Pairwise matching");
#if ENABLE_LOG
    t = getTickCount();
#endif
    (*features_matcher_)(features_, pairwise_matches_, matching_mask_);
    features_matcher_->collectGarbage();
    LOGLN("Pairwise matching, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec");

    // Leave only images we are sure are from the same panorama
    indices_ = detail::leaveBiggestComponent(features_, pairwise_matches_, (float)conf_thresh_);
	vector<Mat> seam_est_imgs_subset;
    vector<Mat> imgs_subset;
    vector<Size> full_img_sizes_subset;
    for (size_t i = 0; i < indices_.size(); ++i)
    {
        imgs_subset.push_back(imgs_[indices_[i]]);
        seam_est_imgs_subset.push_back(seam_est_imgs_[indices_[i]]);
        full_img_sizes_subset.push_back(full_img_sizes_[indices_[i]]);
    }
    seam_est_imgs_ = seam_est_imgs_subset;
    imgs_ = imgs_subset;
    full_img_sizes_ = full_img_sizes_subset;

    if ((int)imgs_.size() < 2)
    {
        LOGLN("Need more images");
        return ERR_NEED_MORE_IMGS;
    }

    return OK;
}

void StitchImage::estimateCameraParams()
{
    detail::HomographyBasedEstimator estimator;
    estimator(features_, pairwise_matches_, cameras_);

    for (size_t i = 0; i < cameras_.size(); ++i)
    {
        Mat R;
        cameras_[i].R.convertTo(R, CV_32F);
        cameras_[i].R = R;
        LOGLN("Initial intrinsic parameters #" << indices_[i] + 1 << ":\n " << cameras_[i].K());
    }

    bundle_adjuster_->setConfThresh(conf_thresh_);
    (*bundle_adjuster_)(features_, pairwise_matches_, cameras_);

    // Find median focal length and use it as final image scale
    vector<double> focals;
    for (size_t i = 0; i < cameras_.size(); ++i)
    {
        LOGLN("Camera #" << indices_[i] + 1 << ":\n" << cameras_[i].K());
        focals.push_back(cameras_[i].focal);
    }

    std::sort(focals.begin(), focals.end());
    if (focals.size() % 2 == 1)
        warped_image_scale_ = static_cast<float>(focals[focals.size() / 2]);
    else
        warped_image_scale_ = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;

    if (do_wave_correct_)
    {
        vector<Mat> rmats;
        for (size_t i = 0; i < cameras_.size(); ++i)
            rmats.push_back(cameras_[i].R);
        detail::waveCorrect(rmats, wave_correct_kind_);
        for (size_t i = 0; i < cameras_.size(); ++i)
            cameras_[i].R = rmats[i];
    }
}

StitchImage::Status StitchImage::composePanorama(InputArray images, OutputArray pano)
{
    vector<Mat> imgs;
    images.getMatVector(imgs);
    if (!imgs.empty())
    {
        CV_Assert(imgs.size() == imgs_.size());

        Mat img;
        seam_est_imgs_.resize(imgs.size());

        for (size_t i = 0; i < imgs.size(); ++i)
        {
            imgs_[i] = imgs[i];
            resize(imgs[i], img, Size(), seam_scale_, seam_scale_);
            seam_est_imgs_[i] = img.clone();
        }

        vector<Mat> seam_est_imgs_subset;
        vector<Mat> imgs_subset;

        for (size_t i = 0; i < indices_.size(); ++i)
        {
            imgs_subset.push_back(imgs_[indices_[i]]);
            seam_est_imgs_subset.push_back(seam_est_imgs_[indices_[i]]);
        }

        seam_est_imgs_ = seam_est_imgs_subset;
        imgs_ = imgs_subset;
    }
	return composePanorama(pano);
}


StitchImage::Status StitchImage::composePanorama(OutputArray pano)
{
    LOGLN("Warping images (auxiliary)... ");

    Mat &pano_ = pano.getMatRef();

#if ENABLE_LOG
    int64 t = getTickCount();
#endif

    vector<Point> corners(imgs_.size());
    vector<Size> sizes(imgs_.size());
	vector<Mat> masks_warped(imgs_.size());	// reuse
    vector<Mat> images_warped(imgs_.size());	// reuse
    vector<Mat> masks(imgs_.size());	// reuse

    // Prepare image masks
    for (size_t i = 0; i < imgs_.size(); ++i)
    {
        masks[i].create(seam_est_imgs_[i].size(), CV_8U);
        masks[i].setTo(Scalar::all(255));
    }

    // Warp images and their masks
    Ptr<detail::MyRotationWarper> w = warper_->create(float(warped_image_scale_ * seam_work_aspect_));
	//w = warper_->create(float(warped_image_scale_ * seam_work_aspect_));
    for (size_t i = 0; i < imgs_.size(); ++i)
    {
        Mat_<float> K;
        cameras_[i].K().convertTo(K, CV_32F);
        K(0,0) *= (float)seam_work_aspect_;
        K(0,2) *= (float)seam_work_aspect_;
        K(1,1) *= (float)seam_work_aspect_;
        K(1,2) *= (float)seam_work_aspect_;

        corners[i] = w->warp(seam_est_imgs_[i], K, cameras_[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
        sizes[i] = images_warped[i].size();

        w->warp(masks[i], K, cameras_[i].R, INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
    }

    vector<Mat> images_warped_f(imgs_.size());	// reuse
    for (size_t i = 0; i < imgs_.size(); ++i)
        images_warped[i].convertTo(images_warped_f[i], CV_32F);

    LOGLN("Warping images, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec");

    // Find seams
    exposure_comp_->feed(corners, images_warped, masks_warped);
    seam_finder_->find(images_warped_f, corners, masks_warped);

    // Release unused memory
    seam_est_imgs_.clear();
    images_warped.clear();	// reuse
    images_warped_f.clear();	// reuse
    masks.clear();	// reuse

    LOGLN("Compositing...");
#if ENABLE_LOG
    t = getTickCount();
#endif

    Mat img_warped, img_warped_s;	// reuse
    Mat dilated_mask, seam_mask, mask, mask_warped;	// reuse

    //double compose_seam_aspect = 1;
    double compose_work_aspect = 1;
    bool is_blender_prepared = false;

    double compose_scale = 1;
    bool is_compose_scale_set = false;

    Mat full_img, img;	// reuse
    for (size_t img_idx = 0; img_idx < imgs_.size(); ++img_idx)
    {
        LOGLN("Compositing image #" << indices_[img_idx] + 1);

        // Read image and resize it if necessary
        full_img = imgs_[img_idx];
        if (!is_compose_scale_set)
        {
            if (compose_resol_ > 0)
                compose_scale = min(1.0, sqrt(compose_resol_ * 1e6 / full_img.size().area()));
            is_compose_scale_set = true;

            // Compute relative scales
            //compose_seam_aspect = compose_scale / seam_scale_;
            compose_work_aspect = compose_scale / work_scale_;

            // Update warped image scale
            warped_image_scale_ *= static_cast<float>(compose_work_aspect);
            w = warper_->create((float)warped_image_scale_);

            // Update corners and sizes
            for (size_t i = 0; i < imgs_.size(); ++i)
            {
                // Update intrinsics
                cameras_[i].focal *= compose_work_aspect;
                cameras_[i].ppx *= compose_work_aspect;
                cameras_[i].ppy *= compose_work_aspect;

                // Update corner and size
                Size sz = full_img_sizes_[i];
                if (std::abs(compose_scale - 1) > 1e-1)
                {
                    sz.width = cvRound(full_img_sizes_[i].width * compose_scale);
                    sz.height = cvRound(full_img_sizes_[i].height * compose_scale);
                }

                Mat K;
                cameras_[i].K().convertTo(K, CV_32F);
                Rect roi = w->warpRoi(sz, K, cameras_[i].R);
                corners[i] = roi.tl();
                sizes[i] = roi.size();
            }
        }
        if (std::abs(compose_scale - 1) > 1e-1)
            resize(full_img, img, Size(), compose_scale, compose_scale);
        else
            img = full_img;
        full_img.release();
        Size img_size = img.size();

        Mat K;
        cameras_[img_idx].K().convertTo(K, CV_32F);

        // Warp the current image
        w->warp(img, K, cameras_[img_idx].R, INTER_LINEAR, BORDER_REFLECT, img_warped);

        // Warp the current image mask
        mask.create(img_size, CV_8U);
        mask.setTo(Scalar::all(255));
        w->warp(mask, K, cameras_[img_idx].R, INTER_NEAREST, BORDER_CONSTANT, mask_warped);

        // Compensate exposure
        exposure_comp_->apply((int)img_idx, corners[img_idx], img_warped, mask_warped);

//        img_warped.convertTo(img_warped_s, CV_16S);
//        img_warped.release();
        img.release();
        mask.release();

        // Make sure seam mask has proper size
        dilate(masks_warped[img_idx], dilated_mask, Mat());
        resize(dilated_mask, seam_mask, mask_warped.size());

        mask_warped = seam_mask & mask_warped;

        if (!is_blender_prepared)
        {
            blender_->prepare(corners, sizes);
            is_blender_prepared = true;
        }

        // Blend the current image
        blender_->feed(img_warped, mask_warped, corners[img_idx]);
		img_warped.release();
    }

    Mat result, result_mask;	// reuse
    blender_->blend(result, result_mask);

    LOGLN("Compositing, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec");

    // Preliminary result is in CV_16SC3 format, but all values are in [0,255] range,
    // so convert it to avoid user confusing
    result.convertTo(pano_, CV_8U);

    return OK;
}


//////////////////////////////////////////////////////////////////////////
// composePanoramaGpu
StitchImage::Status StitchImage::composePanoramaGpu(InputArray images, OutputArray pano)
{
	return composePanoramaGpu(images, pano, Rect());
}

StitchImage::Status StitchImage::composePanoramaGpu(InputArray images, OutputArray pano, Rect roi)
{
    LOGLN("Warping images (auxiliary)... ");
#ifdef _DEBUG
	char buffer[200];
#endif

	restore();
	vector<Mat> imgs;
    images.getMatVector(imgs);
    if (!imgs.empty())
    {
        CV_Assert(imgs.size() == imgs_.size());

        Mat img;
        seam_est_imgs_.resize(imgs.size());

        for (size_t i = 0; i < imgs.size(); ++i)
        {
            imgs_[i] = imgs[i];
            resize(imgs[i], img, Size(), seam_scale_, seam_scale_);
            seam_est_imgs_[i] = img.clone();
			//img.copyTo(seam_est_imgs_gpu_[i]);
        }

        vector<Mat> seam_est_imgs_subset;
        vector<Mat> imgs_subset;

        for (size_t i = 0; i < indices_.size(); ++i)
        {
            imgs_subset.push_back(imgs_[indices_[i]]);
            seam_est_imgs_subset.push_back(seam_est_imgs_[indices_[i]]);
        }

        seam_est_imgs_ = seam_est_imgs_subset;
        imgs_ = imgs_subset;
    }

#if ENABLE_LOG
    int64 t = getTickCount();
#endif

	//////////////////////////////////////////////////////////////////////////
	// 

    vector<Point> corners(imgs_.size());
    vector<Size> sizes(imgs_.size());
	vector<Mat> masks_warped(imgs_.size());
    vector<Mat> images_warped(imgs_.size());
    //vector<Mat> masks(imgs_.size());

	if (masks_.size() != imgs_.size())
	{
		masks_.resize(imgs_.size());
		// Prepare image masks
		for (size_t i = 0; i < imgs_.size(); ++i)
		{
			masks_[i].create(seam_est_imgs_[i].size(), CV_8U);
			masks_[i].setTo(Scalar::all(255));
		}
	}
	if (masks_warped.size() != imgs_.size())
	{
		masks_warped.resize(imgs_.size());
	}
	if (images_warped.size() != imgs_.size())
	{
		images_warped.resize(imgs_.size());
	}

    // Warp images and their masks
    Ptr<detail::MyRotationWarper> w = warper_->create(float(warped_image_scale_ * seam_work_aspect_));

    for (int i = 0; i < (int)imgs_.size(); ++i)
    {
        Mat_<float> K;
        cameras_[i].K().convertTo(K, CV_32F);
        K(0,0) *= (float)seam_work_aspect_;
        K(0,2) *= (float)seam_work_aspect_;
        K(1,1) *= (float)seam_work_aspect_;
        K(1,2) *= (float)seam_work_aspect_;

		corners[i] = w->warp(seam_est_imgs_[i], K, cameras_[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
        sizes[i] = images_warped[i].size();

        w->warp(masks_[i], K, cameras_[i].R, INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
    }

    LOGLN("Warping images, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec");

#ifdef _DEBUG
	for (int i = 0; i < (int)masks_warped.size(); i++)
	{
		sprintf(buffer, "mask_%d.jpg", i);
		bool res = imwrite(buffer, masks_warped[i]);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	//
	// 
    exposure_comp_->feed(corners, images_warped, masks_warped);

	// Release unused memory
    seam_est_imgs_.clear();
    images_warped.clear();	// reuse
	masks_.clear();

    LOGLN("Compositing...");
#if ENABLE_LOG
    t = getTickCount();
#endif

    Mat img_warped, img_warped_s;
	GpuMat	img_gpu, mask_gpu, img_warped_gpu, mask_warped_gpu;
    Mat dilated_mask, seam_mask, mask, mask_warped;	// reuse

    //double compose_seam_aspect = 1;
    double compose_work_aspect = 1;
    bool is_blender_prepared = false;

    double compose_scale = 1;
    bool is_compose_scale_set = false;

    for (int img_idx = 0; img_idx < (int)imgs_.size(); ++img_idx)
    {
        LOGLN("Compositing image #" << indices_[img_idx] + 1);
		Mat full_img, img;	// reuse
        // Read image and resize it if necessary
        full_img = imgs_[img_idx];
        if (!is_compose_scale_set)
        {
            if (compose_resol_ > 0)
                compose_scale = min(1.0, sqrt(compose_resol_ * 1e6 / full_img.size().area()));
            is_compose_scale_set = true;

            // Compute relative scales
            //compose_seam_aspect = compose_scale / seam_scale_;
            compose_work_aspect = compose_scale / work_scale_;

            // Update warped image scale
            warped_image_scale_ *= static_cast<float>(compose_work_aspect);
            w = warper_->create((float)warped_image_scale_);

            // Update corners and sizes
            for (size_t i = 0; i < imgs_.size(); ++i)
            {
                // Update intrinsics
                cameras_[i].focal *= compose_work_aspect;
                cameras_[i].ppx *= compose_work_aspect;
                cameras_[i].ppy *= compose_work_aspect;

                // Update corner and size
                Size sz = full_img_sizes_[i];
                if (std::abs(compose_scale - 1) > 1e-1)
                {
                    sz.width = cvRound(full_img_sizes_[i].width * compose_scale);
                    sz.height = cvRound(full_img_sizes_[i].height * compose_scale);
                }

                Mat K;
                cameras_[i].K().convertTo(K, CV_32F);
                Rect roi = w->warpRoi(sz, K, cameras_[i].R);
                corners[i] = roi.tl();
                sizes[i] = roi.size();
            }
        }
        if (std::abs(compose_scale - 1) > 1e-1)
            resize(full_img, img, Size(), compose_scale, compose_scale);
        else
            img = full_img;
        full_img.release();
        Size img_size = img.size();

        Mat K;
        cameras_[img_idx].K().convertTo(K, CV_32F);
#ifdef _DEBUG
		printf("aspect:%f, focal:%f, ppx:%f, ppy:%f\n", 
			cameras_[img_idx].aspect, cameras_[img_idx].focal, cameras_[img_idx].ppx, cameras_[img_idx].ppy);
		cout << "K: " << K << endl;
		cout << "R: " << cameras_[img_idx].R << endl;
		cout << "t: " << cameras_[img_idx].t << endl;
#endif
        // Warp the current image
#if 1
		img_gpu.upload(img);
        w->warp_gpu(img_gpu, K, cameras_[img_idx].R, INTER_LINEAR, BORDER_REFLECT, img_warped_gpu);	// 

        // Warp the current image mask
        mask_gpu.create(img_size, CV_8U);
        mask_gpu.setTo(Scalar::all(255));
        w->warp_gpu(mask_gpu, K, cameras_[img_idx].R, INTER_NEAREST, BORDER_CONSTANT, mask_warped_gpu);	// 
#else
        w->warp(img, K, cameras_[img_idx].R, INTER_LINEAR, BORDER_REFLECT, img_warped);	// 

        // Warp the current image mask
        mask.create(img_size, CV_8U);
        mask.setTo(Scalar::all(255));
        w->warp(mask, K, cameras_[img_idx].R, INTER_NEAREST, BORDER_CONSTANT, mask_warped);	// 
#endif

#if ENABLE_LOG
//		Warping, time:  4.82922 msec
//		Warping, time: 29.5079 msec
		cout << "Warping, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec" << endl;
#endif
        // 
 #if 1
		exposure_comp_->apply((int)img_idx, corners[img_idx], img_warped_gpu, mask_warped_gpu);	// 
		img_warped_gpu.download(img_warped);
		mask_warped_gpu.download(mask_warped);

		img_gpu.release();
		img_warped_gpu.release();
		mask_warped_gpu.release();
#else
		exposure_comp_->apply((int)img_idx, corners[img_idx], img_warped, mask_warped);	// 
#endif
        img.release();
        mask.release();

        // Make sure seam mask has proper size
        dilate(masks_warped[img_idx], dilated_mask, Mat());
        resize(dilated_mask, seam_mask, mask_warped.size());

        mask_warped = seam_mask & mask_warped;

        if (!is_blender_prepared)
        {
            blender_->prepare(corners, sizes);
            is_blender_prepared = true;
        }

		// if roi is valid 
		if (roi.size() != Size(0, 0))
		{
		//	blender_->setRoi(roi);
		}
#ifdef	_DEBUG
//		sprintf(buffer, "image_warped_%d.jpg", img_idx);
//		imwrite(buffer, img_warped);
//		sprintf(buffer, "mask_warped_%d.jpg", img_idx);
//		imwrite(buffer, mask_warped);
		cout << corners[img_idx] << endl;
#endif
        // 
        blender_->feed(img_warped, mask_warped, corners[img_idx]);	// 
		img_warped.release();
		mask_warped.release();
    }

#if ENABLE_LOG
//	Preblend, time: 47.0378 msec
		cout << "Preblend, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec" << endl;
#endif

    Mat &pano_ = pano.getMatRef();
    Mat result, result_mask;	// reuse
    blender_->blend(pano_, result_mask);	// 

#if ENABLE_LOG
//	Compositing, time: 63.345 msec
	cout << "Compositing, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec" << endl;
#endif
    LOGLN("Compositing, time: " << ((getTickCount() - t) / getTickFrequency()) << " sec");

    return OK;
}
}

void StitchImage::printUsage()
{
    cout <<
	"Type 'S' to start stitching\n"
	"Type 'e' or 'E' to stop stitching\n"
	"Type 't' to oneshot stitch\n"
	"Type 'h' or 'H' to help\n"
	"Type 'q' or 'Q' to quit\n";
}
