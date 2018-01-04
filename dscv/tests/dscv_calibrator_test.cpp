#include <osa/osa.h>
#include <dscv/dscv.hpp>
#include <cvio/cvio.hpp>
#include "../src/dscv_config.h"
#include "dscv_tests.hpp"

#define DSCV_CALIBRATOR_USE_CAMERA 1

static const cv::Size kPatternSize(9, 6);


static void loadImagePairsFromFile(vector<Mat>& _lPatternImages, vector<Mat>& _rPatternImages)
{
    int i;
    const int kCount = 20;
    vector<Mat> lPatternImages(kCount);
    vector<Mat> rPatternImages(kCount);
    Char fileName[128];

    for (i = 0; i < kCount; ++i) {
        snprintf(fileName, sizeof(fileName), "D:/CV/Supporting/calibration/DumpPatternImages_%d_1359006840.jpg", i);
        lPatternImages[i] = cv::imread(fileName);        

        snprintf(fileName, sizeof(fileName), "D:/CV/Supporting/calibration/DumpPatternImages_%d_1359006840.jpg", i + kCount);
        rPatternImages[i] = cv::imread(fileName);
    }

    _lPatternImages = lPatternImages;
    _rPatternImages = rPatternImages;
}


/**********************************************************************/

#pragma region Calibrator

static int calibrator_test(Mat &frame, int key)
{
    static size_t patternImagesCount = 0;
    static vector<Mat> patternImages;
    static Calibrator calibrator(kPatternSize);
    Mat drawnImage;
       

    if ('c' == key) {
        if (calibrator.peek(frame, &drawnImage)) {
            patternImages.push_back(frame);    /* save the original image */
            frame = drawnImage;                        /* display the drawn image */
            ++patternImagesCount;
            OSA_info("Current pattern images count = %u.\n", patternImagesCount);
        }
        return OSA_STATUS_OK;
    }

    if ('s' == key && patternImagesCount >= Calibrator::kHardLeastImagesCount) {
        calibrator.setPatternImages(patternImages);

        OSA_info("Calibrating...\n");
        calibrator.calibrate();

        OSA_info("Dumping result...\n");
        calibrator.dumpResult(string(DSCV_LEFT_CALIBRATION_PATH));

        OSA_info("Done. Calibration file saved to %s.\n", DSCV_LEFT_CALIBRATION_PATH);
    }

    return OSA_STATUS_OK;
}


static int DSCV_calibrator_Camera_Test()
{
    vector<int> cameras;
    cameras.push_back(0);
    Camera camera(cameras);	
    camera.preview(25, calibrator_test, NULL);
    return OSA_STATUS_OK;
}


static int DSCV_calibrator_File_Test(vector<Mat> patternImages)
{ 
    size_t i;

    for (i = 0; i < patternImages.size(); ++i) {        
        calibrator_test(patternImages[i], 'c');
    }

    calibrator_test(patternImages[0], 's');
    return 0;
}

#pragma endregion

/**********************************************************************/

#pragma region Stereo Calibrator

static int stereoCalibrator_test(vector<Mat> &frames, int key)
{
    static unsigned int index = 0;
    static size_t patternImagesCount = 0;
    static vector<Mat> lPatternImages, rPatternImages;
    static Calibrator calibrator(kPatternSize);
    const size_t kCount = frames.size();
    bool patternFound = true;
    Mat lDrawnImage, rDrawnImage;


    assert(2 == kCount);

    ++index;

#if DSCV_CALIBRATOR_USE_CAMERA
    if (index % 64 == 0) {
#else
    if ('c' == key) {
#endif
        patternFound = calibrator.peek(frames[0], &lDrawnImage) && calibrator.peek(frames[1], &rDrawnImage);
        if (patternFound) {
            lPatternImages.push_back(frames[0]);
            rPatternImages.push_back(frames[1]);            
            frames[0] = lDrawnImage;
            frames[1] = rDrawnImage;
            OSA_info("Current pattern images count = %u; L @ %p, R @ %p.\n", patternImagesCount, lPatternImages[patternImagesCount].data, rPatternImages[patternImagesCount].data);
            ++patternImagesCount;
        }        
        return OSA_STATUS_OK;
    }

    if ('s' == key && patternImagesCount >= Calibrator::kHardLeastImagesCount) {
        StereoCalibrator stereoCalibrator(kPatternSize);
        stereoCalibrator.setPatternImages(lPatternImages, rPatternImages);

#if 0
        Char _fileName[128];
        for (size_t i = 0; i < lPatternImages.size(); ++i) {
            snprintf(_fileName, sizeof(_fileName), "D:/CV/Supporting/calibration/l/L_%u.jpg", i);
            imwrite(_fileName, lPatternImages[i]);
            snprintf(_fileName, sizeof(_fileName), "D:/CV/Supporting/calibration/r/R_%u.jpg", i);
            imwrite(_fileName, rPatternImages[i]);
        }
#endif

        OSA_info("Calibrating...\n");
        stereoCalibrator.calibrate();

        /* after this, left or right camera position can be determined by the coordinates of _lImagePoints and _rImagePoints */

        OSA_info("Dumping result...\n");
        stereoCalibrator.dumpResult(string(DSCV_STEREO_CALIBRATION_PATH));

        OSA_info("Done. Calibration file saved to %s.\n", DSCV_STEREO_CALIBRATION_PATH);
    }

    return OSA_STATUS_OK;
}


static int DSCV_stereoCalibrator_Camera_Test()
{
    vector<int> cameras;
    cameras.push_back(0);
    cameras.push_back(1);
    Camera camera(cameras);	
    camera.overlayDatetime = true;
    camera.preview(25, NULL, stereoCalibrator_test);
    return OSA_STATUS_OK;
}


static int DSCV_stereoCalibrator_File_Test(vector<Mat> lPatternImages, vector<Mat> rPatternImages)
{ 
    vector<Mat> patternPair(2);
    
    for (size_t i = 0; i < lPatternImages.size(); ++i) {
        patternPair[0] = lPatternImages[i];
        patternPair[1] = rPatternImages[i];
        stereoCalibrator_test(patternPair, 'c');
    }

    stereoCalibrator_test(patternPair, 's');
    return 0;
}

#pragma endregion

/**********************************************************************/

#pragma region Rectifier

static int rectifier_test(vector<Mat> &frames, int key)
{
    static Rectifier *pRectifier = NULL;
    Mat lFrame;

    assert(1 == frames.size());

    if (NULL == pRectifier) {
        pRectifier = new Rectifier(string(DSCV_LEFT_CALIBRATION_PATH), cv::Size(frames[0].cols, frames[0].rows));
    }

    pRectifier->rectify(frames[0], lFrame);    

    imshow("Rectified Image", lFrame);

    // delete pStereoRectifier;
    // pStereoRectifier = NULL;
    return OSA_STATUS_OK;
}


static int DSCV_rectifier_Camera_Test()
{
    vector<int> cameras;
    cameras.push_back(0);
    Camera camera(cameras);	
    camera.preview(25, NULL, rectifier_test);
    return OSA_STATUS_OK;
}


static int DSCV_rectifier_File_Test(vector<Mat>& patternImages)
{
    vector<Mat> frames(1);

    for (size_t i = 0; i < patternImages.size(); ++i) {
        frames[0] = patternImages[i];
        rectifier_test(frames, 0);
        waitKey(3000);
    }

    return 0;
}


#pragma endregion

/**********************************************************************/

#pragma region Stereo Rectifier

static int stereoRectifier_test(vector<Mat> &frames, int key)
{
    static StereoRectifier *pStereoRectifier = NULL;
    Mat lFrame, rFrame, combinedFrame;

    assert(2 == frames.size());

    if (NULL == pStereoRectifier) {
        pStereoRectifier = new StereoRectifier();
    }
    assert(NULL != pStereoRectifier);

    if (!pStereoRectifier->getValidity()) {
        pStereoRectifier->load(string(DSCV_STEREO_CALIBRATION_PATH), cv::Size(frames[0].cols, frames[0].rows));
    }

    if (pStereoRectifier->getValidity()) {
        pStereoRectifier->rectify(frames[0], frames[1], lFrame, rFrame, &combinedFrame);
        imshow("Combined Image", combinedFrame);
    }
    
    // delete pStereoRectifier;
    // pStereoRectifier = NULL;
    return OSA_STATUS_OK;
}


static int DSCV_stereoRectifier_Camera_Test()
{
    vector<int> cameras;
    cameras.push_back(0);
    cameras.push_back(1);
    Camera camera(cameras);	
    camera.preview(25, NULL, stereoRectifier_test);
    return OSA_STATUS_OK;
}


static int DSCV_stereoRectifier_File_Test(vector<Mat>& lPatternImages, vector<Mat>& rPatternImages)
{
    vector<Mat> patternPair(2);

    for (size_t i = 0; i < lPatternImages.size(); ++i) {
        patternPair[0] = lPatternImages[i];
        patternPair[1] = rPatternImages[i];
        stereoRectifier_test(patternPair, 0);
        waitKey(3000);
    }

    return 0;
}

#pragma endregion

/**********************************************************************/


int DSCV_calibrator_Tests()
{
#if DSCV_CALIBRATOR_USE_CAMERA
    DSCV_calibrator_Camera_Test();
    DSCV_rectifier_Camera_Test();
#else
    vector<Mat> lPatternImages, rPatternImages;
    loadImagePairsFromFile(lPatternImages, rPatternImages);
    DSCV_calibrator_File_Test(rPatternImages);
    DSCV_rectifier_File_Test(rPatternImages);
#endif
    return 0;
}


int DSCV_stereoCalibrator_Tests()
{
#if DSCV_CALIBRATOR_USE_CAMERA
    DSCV_stereoCalibrator_Camera_Test();
    DSCV_stereoRectifier_Camera_Test();
#else
    vector<Mat> lPatternImages, rPatternImages;
    loadImagePairsFromFile(lPatternImages, rPatternImages);
    DSCV_stereoCalibrator_File_Test(lPatternImages, rPatternImages);
    DSCV_stereoRectifier_File_Test(lPatternImages, rPatternImages);
#endif

    return 0;
}
