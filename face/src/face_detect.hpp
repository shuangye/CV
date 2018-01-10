#pragma once
#ifndef __FACE_DETECT_HPP__
#define __FACE_DETECT_HPP__


//
//  Created by papillon on 27/08/2017.
//  Copyright © 2017 papillon. All rights reserved.
//

#include <opencv2/opencv.hpp>

using namespace cv;

class Detector {
private:
	string _modelPath;
	CascadeClassifier _classifier;
	bool _drawClues;
	bool _valid;

public:
	Detector(string modelPath);

    bool isValid() { return _valid; }

    vector<cv::Rect> detect(const Mat &iamge);

	cv::Rect largestObject(Mat &image);
};


#endif /* __FACE_DETECT_HPP__ */
