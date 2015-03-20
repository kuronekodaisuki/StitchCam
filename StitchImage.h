//
//
//
#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/stitching/warpers.hpp>
#include <opencv2/stitching/detail/matchers.hpp>
#include <opencv2/stitching/detail/motion_estimators.hpp>
#include <opencv2/stitching/detail/exposure_compensate.hpp>
#include <opencv2/stitching/detail/seam_finders.hpp>
#include <opencv2/stitching/detail/blenders.hpp>
#include <opencv2/stitching/detail/camera.hpp>

#include "MyWarper.h"
#include "MyCompensator.h"
#include "MyBlender.h"
#include "MySeamFinder.h"


namespace cv {

class StitchImage 
{
public:
    enum { ORIG_RESOL = -1 };
    enum Status { OK, ERR_NEED_MORE_IMGS };

    // Creates stitcher with default parameters
    static StitchImage createDefault(bool try_use_gpu = true);

    double registrationResol() const { return registr_resol_; }
    void setRegistrationResol(double resol_mpx) { registr_resol_ = resol_mpx; }

    double seamEstimationResol() const { return seam_est_resol_; }
    void setSeamEstimationResol(double resol_mpx) { seam_est_resol_ = resol_mpx; }

    double compositingResol() const { return compose_resol_; }
    void setCompositingResol(double resol_mpx) { compose_resol_ = resol_mpx; }

    double panoConfidenceThresh() const { return conf_thresh_; }
    void setPanoConfidenceThresh(double conf_thresh) { conf_thresh_ = conf_thresh; }

    bool waveCorrection() const { return do_wave_correct_; }
    void setWaveCorrection(bool flag) { do_wave_correct_ = flag; }

    detail::WaveCorrectKind waveCorrectKind() const { return wave_correct_kind_; }
    void setWaveCorrectKind(detail::WaveCorrectKind kind) { wave_correct_kind_ = kind; }

    Ptr<detail::FeaturesFinder> featuresFinder() { return features_finder_; }
    const Ptr<detail::FeaturesFinder> featuresFinder() const { return features_finder_; }
    void setFeaturesFinder(Ptr<detail::FeaturesFinder> features_finder)
        { features_finder_ = features_finder; }

    Ptr<detail::FeaturesMatcher> featuresMatcher() { return features_matcher_; }
    const Ptr<detail::FeaturesMatcher> featuresMatcher() const { return features_matcher_; }
    void setFeaturesMatcher(Ptr<detail::FeaturesMatcher> features_matcher)
        { features_matcher_ = features_matcher; }

    const cv::Mat& matchingMask() const { return matching_mask_; }
    void setMatchingMask(const cv::Mat &mask)
    {
        CV_Assert(mask.type() == CV_8U && mask.cols == mask.rows);
        matching_mask_ = mask.clone();
    }

    Ptr<detail::BundleAdjusterBase> bundleAdjuster() { return bundle_adjuster_; }
    const Ptr<detail::BundleAdjusterBase> bundleAdjuster() const { return bundle_adjuster_; }
    void setBundleAdjuster(Ptr<detail::BundleAdjusterBase> bundle_adjuster)
        { bundle_adjuster_ = bundle_adjuster; }

    Ptr<MyWarperCreator> warper() { return warper_; }
    const Ptr<MyWarperCreator> warper() const { return warper_; }
    void setWarper(Ptr<MyWarperCreator> creator) { warper_ = creator; }

    Ptr<detail::ExposureCompensator> exposureCompensator() { return exposure_comp_; }
    const Ptr<detail::ExposureCompensator> exposureCompensator() const { return exposure_comp_; }
    void setExposureCompensator(Ptr<detail::GainCompensatorGpu> exposure_comp)
        { exposure_comp_ = exposure_comp; }

    Ptr<detail::SeamFinder> seamFinder() { return seam_finder_; }
    const Ptr<detail::SeamFinder> seamFinder() const { return seam_finder_; }
    void setSeamFinder(Ptr<detail::SeamFinder> seam_finder) { seam_finder_ = seam_finder; }

    Ptr<detail::Blender> blender() { return blender_; }
    const Ptr<detail::Blender> blender() const { return blender_; }
    void setBlender(Ptr<detail::Blender> b) { blender_ = b; }

    Status estimateTransform(InputArray images);
    Status estimateTransform(InputArray images, const std::vector<std::vector<Rect> > &rois);

    Status composePanorama(OutputArray pano);
    Status composePanorama(InputArray images, OutputArray pano);
	Status composePanoramaGpu(InputArray images, OutputArray pano);
	Status composePanoramaGpu(InputArray images, OutputArray pano, Rect roi);

    Status stitch(InputArray images, OutputArray pano);
    Status stitch(InputArray images, const std::vector<std::vector<Rect> > &rois, OutputArray pano);

    std::vector<int> component() const { return indices_; }
    std::vector<detail::CameraParams> cameras() const { return cameras_; }
    double workScale() const { return work_scale_; }

	// enhanced functions
	void printUsage();

	void restore()	{
		cameras_ = cameras_save; 
		warped_image_scale_ = warped_image_scale_save;
	}

	StitchImage() {}

private:


    Status matchImages();
    void estimateCameraParams();

    double registr_resol_;
    double seam_est_resol_;
    double compose_resol_;
    double conf_thresh_;
    Ptr<detail::FeaturesFinder> features_finder_;
    Ptr<detail::FeaturesMatcher> features_matcher_;
    Ptr<MyWarperCreator> warper_;
	//Ptr<MyPlaneWarper> w_;

    Ptr<detail::MyCompensator> exposure_comp_;
    Ptr<detail::SeamFinder> seam_finder_;
    Ptr<detail::Blender> blender_;
    Ptr<detail::BundleAdjusterBase> bundle_adjuster_;
    bool do_wave_correct_;
    detail::WaveCorrectKind wave_correct_kind_;
	cv::Mat matching_mask_;

    std::vector<cv::Mat> imgs_;
    std::vector<cv::Mat> seam_est_imgs_;
	std::vector<cv::Mat> masks_;	// reuse
	//std::vector<cv::Mat> masks_warped_;	// reuse
    //std::vector<cv::Mat> images_warped_;	// reuse
	//std::vector<cv::Mat> images_warped_f_;	// reuse

    std::vector<std::vector<cv::Rect> > rois_;
    std::vector<cv::Size> full_img_sizes_;
    std::vector<detail::ImageFeatures> features_;
    std::vector<detail::MatchesInfo> pairwise_matches_;
    std::vector<int> indices_;
    std::vector<detail::CameraParams> cameras_;
	std::vector<detail::CameraParams> cameras_save;	// saved camera param
    double work_scale_;
    double seam_scale_;
    double seam_work_aspect_;
    double warped_image_scale_;
	double warped_image_scale_save;
};

}