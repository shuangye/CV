#include <chrono>
#include <fstream>
#include <osa/osa.h>
#include <dscv/dscv.hpp>
#include <face/face.h>
#include "face_living.hpp"
#include "face_stereo.hpp"
#include "face_config.h"
#if FACE_OPENCV_SUPPORT_NON_FREE
#include <opencv2/nonfree/features2d.hpp>
#endif

using namespace std;


Living::Living()
{
	_showDisparity = true;
}


Living::~Living()
{	
}



int Living::determineLivingFace(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{
	int ret;
    bool isFaceQualified;

	possibility = FACE_LIVING_FACE_IMPOSSIBLE;

	if (lFaceRegion.area() == 0 || rFaceRegion.area() == 0) {		
		return OSA_STATUS_OK;
	}

#if 1
	/* if the two faces' areas have large differences */
	if (OSA_abs(lFaceRegion.area() - rFaceRegion.area()) > OSA_min(lFaceRegion.area(), rFaceRegion.area())) {
        return OSA_STATUS_OK;
	}
#endif

#if 1
    /* if the face is too small */
    isFaceQualified = (lFaceRegion.area() * _kMinFaceRatio) >= lImage.size().area();
	if (!isFaceQualified) {
		return OSA_STATUS_OK;
	}
#endif

	ret = determineLivingFaceTopHalf(lImage, rImage, lFaceRegion, rFaceRegion, possibility);
	if (OSA_isFailed(ret)) {
		return ret;
	}

	ret = determineLivingFace1(lImage, rImage, lFaceRegion, rFaceRegion, possibility);
	if (OSA_isFailed(ret)) {
		return ret;
	}

	ret = determineLivingFaceBottomHalf(lImage, rImage, lFaceRegion, rFaceRegion, possibility);

	return ret;
}


/*
 WARNING: this method stores previous face region, and contains logic about time dimension, so it is neither reentrant nor thread safe!
 */
void Living::livingFacePenalty(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{
    static cv::Rect prevLFace = cv::Rect(0, 0, 0, 0);
    static chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
    chrono::steady_clock::time_point t2;
    chrono::duration<double> timeSpan;
    int64 distance2;
    double speed;
    double percentage;
    
    t2 = chrono::steady_clock::now();            

    if (0 != prevLFace.area()) {
        distance2 = (prevLFace.x - lFaceRegion.x) * (prevLFace.x - lFaceRegion.x) + (prevLFace.y - lFaceRegion.y) * (prevLFace.y - lFaceRegion.y);
        timeSpan = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        speed = sqrt(distance2) / timeSpan.count();  /* pixels / second */
        
        percentage = speed / sqrt(lImage.cols * lImage.cols + lImage.rows * lImage.rows) * 100;  /* percentage of diagonal */        
        OSA_debug("Speed = %f, %f during %f seconds.\n", speed, percentage, timeSpan.count());

        possibility -= percentage;    /* how many percentages you moved, how many percentages I discount you. */
        OSA_lowerLimit(possibility, 0);

#if FACE_OPENCV_SUPPORTS_GUI
        Char buffer[64];
        snprintf(buffer, sizeof(buffer), "Speed = %.3f, %f%%", speed, percentage);
        cv::putText(lImage, string(buffer), cv::Point(0, lImage.size().height - 50), FONT_HERSHEY_PLAIN, 2, cv::Scalar(0, 0, 255), 1);
#endif
    }

    t1 = chrono::steady_clock::now();
    prevLFace = lFaceRegion;
}


int Living::determineLivingFaceTopHalf(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{
	int ret;
	Stereo stereo;
	Mat lFace, dFace;
    Mat _lImage, _rImage;
    vector<cv::Rect> faces(2);
    cv::Rect roundingFaceRegion;    /* the rect which covers both left face region and right face region */


    (void)rFaceRegion;

	if (lImage.size() != rImage.size()) {
		cerr << "The two images must be of same size." << endl;
		return OSA_STATUS_EINVAL;
	}

    _detectedFaceInDisparity = lFaceRegion;  /* left side as the reference */
    
    /* stereo disparity involves heavy calculations, so try to calc the disparity of the covered region only */
    faces[0] = lFaceRegion;
    faces[1] = rFaceRegion;
    roundingFaceRegion = RectUtils::CoveredRect(faces);
    _lImage = Mat(lImage, roundingFaceRegion);
    _rImage = Mat(rImage, roundingFaceRegion);
    ret = stereo.calcDisparity(_lImage, _rImage, _disparity);
	if (ret != 0) {
		cerr << "Failed to calc disparity map: " << ret << endl;
		return OSA_STATUS_EINVAL;
	}
	
	lFace = Mat(lImage, lFaceRegion);

    /* extract the corresponding face region in disparity map */
    _detectedFaceInDisparity.x -= roundingFaceRegion.x;
    _detectedFaceInDisparity.y -= roundingFaceRegion.y;
    dFace = Mat(_disparity, _detectedFaceInDisparity);

	_lFaceGray = MatUtils::toGray(lFace);
	_dFaceGray = MatUtils::toGray(dFace);

#if 1
	cv::equalizeHist(_lFaceGray, _lFaceGray);
	cv::equalizeHist(_dFaceGray, _dFaceGray);
#endif
    
	return OSA_STATUS_OK;
}


int Living::determineLivingFaceBottomHalf(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{    
    (void)possibility;

#if 0
	Detector detector("D:/CV/OpenCV2.4.13/sources/data/haarcascades/haarcascade_frontalface_default.xml");
	_detectedFaceInDisparity = detector.largestObject(_disparity);

	if (_detectedFaceInDisparity.area() > 0) {		
		OSA_debug("Face area %d = %dx%d in image %dx%d vs. %d = %dx%d in disparity %dx%d.\n", 
			_detectedFaceInDisparity.area(), _detectedFaceInDisparity.width, _detectedFaceInDisparity.height, _disparity.cols, _disparity.rows,
			lFaceRegion.area(), lFaceRegion.width, lFaceRegion.height, lImage.cols, lImage.rows);
		rectangle(_disparity, _detectedFaceInDisparity, Scalar(255));

		if (_detectedFaceInDisparity.area() * 16 >= _disparity.size().area()) {
#if 0
			int dist2 = (_detectedFaceInDisparity.x - lFaceRegion.x) * (_detectedFaceInDisparity.x - lFaceRegion.x)
				+ (_detectedFaceInDisparity.y - lFaceRegion.y) * (_detectedFaceInDisparity.y - lFaceRegion.y);
			int diagonal2 = lFaceRegion.x * lFaceRegion.x + lFaceRegion.y * lFaceRegion.y;
			OSA_info("distance %d, %d\n", dist2, diagonal2);
			if (OSA_abs(dist2 - diagonal2) <= diagonal2 / 100) {
				
			}
#endif
			OSA_info("Face matches.\n");
			*pPossibility /= 2;  // TODO: different alrogithms have different meanings
		}
	}
#endif

    livingFacePenalty(lImage, rImage, lFaceRegion, rFaceRegion, possibility);
    
#if FACE_OPENCV_SUPPORTS_GUI    
	if (_showDisparity) {
        rectangle(lImage, lFaceRegion, Scalar(0, 0, 255));
        rectangle(rImage, rFaceRegion, Scalar(0, 0, 255));

        /* black and white rectangles to make it obvious */
        rectangle(_disparity, _detectedFaceInDisparity, Scalar(0));
        _detectedFaceInDisparity.width += 2;
        _detectedFaceInDisparity.height += 2;
        _detectedFaceInDisparity.x -= 1;
        _detectedFaceInDisparity.y -= 1;
        rectangle(_disparity, _detectedFaceInDisparity, Scalar(255));
		imshow("Face::Disparity", _disparity);
	}
#endif

	return OSA_STATUS_OK;
}


int Living::determineLivingFace1(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{
	Mat diff;
	MatUtils utlMat;
	unsigned int average;


    (void)lImage;
    (void)rImage;
    (void)lFaceRegion;
    (void)rFaceRegion;

#if 1
	Mat mean, stdDev;
	cv::meanStdDev(_dFaceGray, mean, stdDev);
    possibility = stdDev.at<double>(0, 0);
#else
	utlMat.levelDiff(_lFaceGray, _dFaceGray, diff);  /* left side as the reference */
	average = utlMat.average(diff);
	possibility = average;  /* percentage */	
#endif

	return OSA_STATUS_OK;
}


int Living::determineLivingFace2(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{
    (void)lImage;
    (void)rImage;
    (void)lFaceRegion;
    (void)rFaceRegion;

    const float kSimilarityThreshold = 1.5F;
    double similarity;

    similarity = MatUtils::calcSimilarity(_lFaceGray, _dFaceGray);
    if (similarity > kSimilarityThreshold) {
        possibility = FACE_LIVING_FACE_IMPOSSIBLE;
    }
    else {
        possibility = 100 - similarity * 100 / kSimilarityThreshold;    /* scale to 0~100% */
    }

    OSA_info("Similarity = %f, possibility = %d. Face size left %dx%d, right %dx%d.\n", 
        similarity, possibility, lFaceRegion.size().width, lFaceRegion.size().height, rFaceRegion.size().width, rFaceRegion.size().height);
    return OSA_STATUS_OK;
}


#if FACE_OPENCV_SUPPORT_NON_FREE

static double sumMatches(vector<DMatch> matches)
{
	double sum = 0.0;

	for (size_t i = 0; i < matches.size(); ++i) {
		sum += matches[i].distance;
	}

	return sum;
}


/* using feature match */
int Living::determineLivingFace4(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{
	const int kMinHessian = 400;


    (void)lImage;
    (void)rImage;
    (void)lFaceRegion;
    (void)rFaceRegion;		

	//-- Step 1: Detect the keypoints using SURF Detector
	SurfFeatureDetector detector(kMinHessian);
	std::vector<KeyPoint> lFaceKeypoints, dFaceKeypoints;
	detector.detect(_lFaceGray, lFaceKeypoints);
	detector.detect(_dFaceGray, dFaceKeypoints);

	//-- Step 2: Calculate descriptors (feature vectors)
	SurfDescriptorExtractor extractor;
	Mat lFaceDescriptors, dFaceDescriptors;
	extractor.compute(_lFaceGray, lFaceKeypoints, lFaceDescriptors);
	extractor.compute(_dFaceGray, dFaceKeypoints, dFaceDescriptors);

	//-- Step 3: Matching descriptor vectors using FLANN matcher
	FlannBasedMatcher matcher;
	std::vector< DMatch > matches;
	matcher.match(lFaceDescriptors, dFaceDescriptors, matches);
	
	//-- Quick calculation of max and min distances between keypoints
	double maxDist = 0; double minDist = 100;
	for (int i = 0; i < lFaceDescriptors.rows; i++) {
		double dist = matches[i].distance;
		if (dist < minDist) minDist = dist;
		if (dist > maxDist) maxDist = dist;
	}

	// Filter only "good" matches (i.e. whose distance is less than 3*min_dist )
	std::vector< DMatch > good_matches;
	for (int i = 0; i < lFaceDescriptors.rows; i++) {
		if (matches[i].distance < 3 * minDist) {
			good_matches.push_back(matches[i]);
		}
	}

	double matchesSum = sumMatches(matches);
	double goodMatchesSum = sumMatches(good_matches);
	OSA_info("matches %f / %u = %f; good matches %f / %u = %f.\n",
		matchesSum, matches.size(), matchesSum / matches.size(),
		goodMatchesSum, good_matches.size(), goodMatchesSum / good_matches.size());

	return OSA_STATUS_OK;
}

#endif


/* using contours match */
int Living::determineLivingFace5(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility)
{
    (void)lImage;
    (void)rImage;
    (void)lFaceRegion;
    (void)rFaceRegion;

	double similarity = MatUtils::calcSimilarity(_lFaceGray, _dFaceGray);
	possibility = (int)(similarity * 100);
	return OSA_STATUS_OK;
}
