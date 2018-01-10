#pragma once
#ifndef __DS_FACE_RECOGNIZE_HPP__
#define __DS_FACE_RECOGNIZE_HPP__

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class Recognizer
{
public:
    Recognizer(string modelPath, string configPath);
	~Recognizer();

	int recognize(const Mat &face, int &label, double &confidence);	

    double calcReliability(const Mat preprocessedFace);

private:
	Ptr<FaceRecognizer> _faceRecModel;
	string _modelPath;
	string _configPath;
};


#endif  /* __DS_FACE_RECOGNIZE_HPP__ */
