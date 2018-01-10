#include <osa/osa.h>
#include <dscv/dscv.hpp>
#include "face_detect.hpp"


using namespace std;

Detector::Detector(string modelPath) : _modelPath(modelPath), _drawClues(false)
{
	_classifier = CascadeClassifier(_modelPath);
	_valid = !_classifier.empty();

	if (!_valid) {		
        OSA_error("The model path %s is invalid.\n", _modelPath.c_str());
	}
};

vector<cv::Rect> Detector::detect(const Mat &image)
{
    vector<cv::Rect> objects;
	// Size minSize = Size(image.size().width / 4, image.size().height / 4);
    
    _classifier.detectMultiScale(image, objects, 1.1, 3, 0);
 
    return objects;
}


cv::Rect Detector::largestObject(Mat &image)
{
    RectUtils rectUtils;
	cv::Rect largest(0, 0, 0, 0);
	vector<cv::Rect> objects = this->detect(image);


	// _classifier.detectMultiScale(image, objects, 1.1, 3, CASCADE_FIND_BIGGEST_OBJECT | CASCADE_DO_ROUGH_SEARCH);
	objects = this->detect(image);
	for (size_t i = 0; i < objects.size(); ++i) {
		if (rectUtils.CompareRect(objects[i], largest) > 0) {
			largest = objects[i];
		}
	}
	
	if (_drawClues && largest.area() > 0) {
		cv::rectangle(image, largest, Scalar(255, 0, 0), 2);
	}

	return largest;
}



