#include <fstream>
#include <osa/osa.h>
#include "face_recognize.hpp"
#include "UtlMat.hpp"



Recognizer::Recognizer(string modelPath, string configPath) : _modelPath(modelPath), _configPath(configPath)
{
}

Recognizer::~Recognizer()
{
}


static int read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') 
{
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file) {
		OSA_error("No valid input file was given, please check the given filename.");
		return OSA_STATUS_EINVAL;
	}

	string line, path, classlabel;
	Mat image;
	bool isCommnet;

	while (!file.eof()) {
		getline(file, line);
		if (line.empty()) {
			continue;
		}

		for (size_t i = 0; i < line.length(); ++i) {
			if (iswspace(line[i])) {
				continue;
			}
			isCommnet = '#' == line[i];
			break;
		}

		if (isCommnet) {
			continue;
		}

		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, classlabel);
		if (!path.empty() && !classlabel.empty()) {
			image = imread(path, 0);
			if (image.empty()) {
				continue;
			}
			images.push_back(image);
			labels.push_back(atoi(classlabel.c_str()));
		}
	}

	return OSA_STATUS_OK;
}


int Recognizer::recognize(const Mat &face, int &label, double &confidence)
{
	int ret;	
	static bool isModelValid = false;
	vector<Mat> images;
	vector<int> labels;	
	ifstream testStream(_modelPath.c_str(), ifstream::in);


	if (NULL == _faceRecModel) {
		// _faceRecModel = createLBPHFaceRecognizer();
		_faceRecModel = createEigenFaceRecognizer();
	}

	if (!isModelValid) {
		if (testStream.good()) {
			OSA_info("Loading existing model...\n");
			_faceRecModel->load(_modelPath);
		}
		else {
			ret = read_csv(_configPath, images, labels);
			if (OSA_isFailed(ret)) {
				return ret;
			}

			if (images.size() < 2) {
				OSA_error("This demo needs at least 2 images to work.Please add more images to your data set!");
				return OSA_STATUS_EINVAL;
			}

			OSA_info("There are %u samples in the training set. Now will start training...\n", images.size());
			_faceRecModel->train(images, labels);
			_faceRecModel->save(_modelPath);
		}

		isModelValid = true;
	}
	
	_faceRecModel->predict(UtlMat::toGray(face), label, confidence);
	return OSA_STATUS_OK;
}


// Generate an approximately reconstructed face by back-projecting the eigenvectors & eigenvalues of the given (preprocessed) face.
double Recognizer::calcReliability(const Mat preprocessedFace)
{
	Mat reconstructedFace;
	double similarity;

	// Since we can only reconstruct the face for some types of FaceRecognizer models (ie: Eigenfaces or Fisherfaces),
	// we should surround the OpenCV calls by a try/catch block so we don't crash for other models.
#if SUPPORT_CPP_EXCEPTION
	try
#endif
	{
		// Get some required data from the FaceRecognizer model.
		Mat eigenvectors = _faceRecModel->get<Mat>("eigenvectors");
		Mat averageFaceRow = _faceRecModel->get<Mat>("mean");

		int faceHeight = preprocessedFace.rows;

		// Project the input image onto the PCA subspace.
		Mat projection = subspaceProject(eigenvectors, averageFaceRow, preprocessedFace.reshape(1, 1));
		//printMatInfo(projection, "projection");

		// Generate the reconstructed face back from the PCA subspace.
		Mat reconstructionRow = subspaceReconstruct(eigenvectors, averageFaceRow, projection);
		//printMatInfo(reconstructionRow, "reconstructionRow");

		// Convert the float row matrix to a regular 8-bit image. Note that we
		// shouldn't use "getImageFrom1DFloatMat()" because we don't want to normalize
		// the data since it is already at the perfect scale.

		// Make it a rectangular shaped image instead of a single row.
		Mat reconstructionMat = reconstructionRow.reshape(1, faceHeight);
		// Convert the floating-point pixels to regular 8-bit uchar pixels.
		reconstructedFace = Mat(reconstructionMat.size(), CV_8U);
		reconstructionMat.convertTo(reconstructedFace, CV_8U, 1, 0);
		//printMatInfo(reconstructedFace, "reconstructedFace");

	}

#if SUPPORT_CPP_EXCEPTION
	catch (cv::Exception e) {
		//cout << "WARNING: Missing FaceRecognizer properties." << endl;		
		return -1;
	}
#endif

	imshow("Reconstruction", reconstructedFace);
	similarity = UtlMat::calcSimilarity(preprocessedFace, reconstructedFace);
	return similarity;
}

