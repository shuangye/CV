#include <osa/osa.h>
#include "face_stereo.hpp"


Stereo::Stereo()
{
}


Stereo::~Stereo()
{
}


int Stereo::calcDisparity(Mat lImage, Mat rImage, Mat& disparity)
{
	Mat disparity8;
	int64 t1, t2;


	if (lImage.size() != rImage.size()) {
		return OSA_STATUS_EINVAL;
	}
    
	int channelCount = lImage.channels();
	Size imageSize = lImage.size();
	int numberOfDisparities = ((imageSize.width / 8) + 15) & -16;

	t1 = getTickCount();

#if 1
	StereoSGBM sgbm;
	sgbm.preFilterCap = 63;
	sgbm.SADWindowSize = 5;
	sgbm.P1 = 8 * channelCount * sgbm.SADWindowSize * sgbm.SADWindowSize;
	sgbm.P2 = 32 * channelCount * sgbm.SADWindowSize * sgbm.SADWindowSize;
	sgbm.minDisparity = 0;
	sgbm.numberOfDisparities = numberOfDisparities;
	sgbm.uniquenessRatio = 5;
	sgbm.speckleWindowSize = 100;
	sgbm.speckleRange = 32;
	sgbm.disp12MaxDiff = 1;
	sgbm.fullDP = false;
	sgbm(lImage, rImage, disparity);
#else
	StereoBM bm;
	Rect roi1, roi2;
	roi1.x = 0;
	roi1.y = 0;
	roi1.width = img1.size().width;
	roi1.height = img1.size().height;
	roi2 = roi1;
	bm.state->roi1 = roi1;
	bm.state->roi2 = roi2;
	bm.state->preFilterCap = 31;
	bm.state->SADWindowSize = 9;
	bm.state->minDisparity = 0;
	bm.state->numberOfDisparities = numberOfDisparities;
	bm.state->textureThreshold = 10;
	bm.state->uniquenessRatio = 15;
	bm.state->speckleWindowSize = 100;
	bm.state->speckleRange = 32;
	bm.state->disp12MaxDiff = 1;
	cvtColor(img1, img1, COLOR_BGR2GRAY);  /* BG algorithm supports gray images only */
	cvtColor(img2, img2, COLOR_BGR2GRAY);
	bm(img1, img2, disp);
#endif

	_disparity = disparity;
	disparity.convertTo(disparity8, CV_8U, 255 / (numberOfDisparities * 16.));

	disparity = disparity8;

	t2 = getTickCount();

	OSA_debug("Calculated disparity. %.1fms elapsed.\n", (t2 - t1) * 1000 / getTickFrequency());

	return 0;
}


#if 0
int Stereo::calcDepth(const string stereoCalibrationPath, cv::Rect region)
{
	int ret;
	Mat disp_compute;
	Mat pointcloud;	


	if (_disparity.empty()) {
		OSA_error("Please calc disparity first.\n");
		return OSA_STATUS_EPERM;
	}

	if (region.area() == 0) {
		return OSA_STATUS_EPERM;
	}

	if (NULL == _pStereoRectifier) {
		_pStereoRectifier = new StereoRectifier(stereoCalibrationPath, _disparity.size());
		ret = _pStereoRectifier->getQ(_Q);
		if (OSA_isFailed(ret) || _Q.empty()) {
			return OSA_STATUS_EINVAL;
		}
	}

	_disparity.convertTo(disp_compute, CV_32F, 1.f / 16.f);

	// Calculate 3D co-ordinates from disparity image
	reprojectImageTo3D(disp_compute, pointcloud, _Q, true);

	// Extract depth of the ROI
	pointcloud = pointcloud(region);
	Mat z_roi(pointcloud.size(), CV_32FC1);
	int from_to[] = { 2, 0 };
	mixChannels(&pointcloud, 1, &z_roi, 1, from_to, 1);

	cout << "Depth: " << mean(z_roi) << " mm" << endl;

	return OSA_STATUS_OK;
}

#endif