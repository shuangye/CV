#if SUPPORT_CPP_EXCEPTION
#include <stdexcept>
#endif
#include <osa/osa.h>
#include <dscv/dscv_matutils.hpp>

unsigned long long MatUtils::sum(const Mat &image)
{
	unsigned int sum = 0;
	//int channels = image.channels();
	cv::Size size = image.size();

	if (image.channels() > 1) {
#if SUPPORT_CPP_EXCEPTION
		throw std::invalid_argument("This class supports gray image only.");
#else
		cerr << "This class supports gray image only." << endl;
		return OSA_STATUS_EINVAL;
#endif /* SUPPORT_CPP_EXCEPTION */
	}
	
	// TODO: support multiple channels
	for (int col = 0; col < size.width; ++col) {
		for (int row = 0; row < size.height; ++row) {			
			sum += image.at<uchar>(row, col);
		}
	}

	return sum;
}


unsigned int MatUtils::average(const Mat &image)
{
	unsigned int sum;
	unsigned int elementCount = image.size().width * image.size().height;
	
	if (0 == elementCount) {
		return 0;
	}

	sum = this->sum(image);
	return (unsigned int)(sum / (float)elementCount);
}


unsigned int MatUtils::sumOfAbsDiff(const Mat &image)
{
	unsigned int result = 0;
	int average;
	unsigned int elementCount = image.size().width * image.size().height;

	if (0 == elementCount) {
		return 0;
	}

	if (image.channels() > 1) {
#if SUPPORT_CPP_EXCEPTION
		throw std::invalid_argument("This class supports gray image only.");
#else
		cerr << "This class supports gray image only." << endl;
		return OSA_STATUS_EINVAL;
#endif /* SUPPORT_CPP_EXCEPTION */
	}

	average = this->average(image);
	for (int col = 0; col < image.cols; ++col) {
		for (int row = 0; row < image.rows; ++row) {
			result += std::abs(int((int)image.at<uchar>(row, col) - average));			
		}
	}

	return result;
}


int MatUtils::neutralizeGray(Mat &image)
{
	const float kNeutralValue = 128.0;  /* neutral gray  */	
	float scale;


	if (image.channels() > 1) {
#if SUPPORT_CPP_EXCEPTION
		throw std::invalid_argument("This class supports gray image only.");
#else
		cerr << "This operation supports gray image only." << endl;
		return OSA_STATUS_EINVAL;
#endif /* SUPPORT_CPP_EXCEPTION */
	}
			
	scale = kNeutralValue / this->average(image);
	image *= scale;	

	return 0;
}

void MatUtils::neutralizeIfNeeded(Mat &image)
{
	unsigned int average;
	float scale;


	average = this->average(image);
	if (average < 128) {  /* 128 as the neutral gray  */
		scale = 128.0 / average;
		image *= scale;
	}
}


int MatUtils::levelDiff(const Mat &image0, const Mat &image1, Mat &diff)
{
	Mat gray0, gray1;


	if (image0.size() != image1.size()) {
		cerr << "The two images must be in the same size.\n" << endl;
		return -1;
	}

	gray0 = toGray(image0);
	gray1 = toGray(image1);

#if 0
	neutralizeIfNeeded(gray0);
	neutralizeIfNeeded(gray1);
//#else
	neutralizeGray(gray0);
	neutralizeGray(gray1);
#endif

#if 1
	unsigned int average0, average1;
	float scale;
	Mat scaledGray1;

	average0 = this->average(gray0);
	average1 = this->average(gray1);
	scale = float(average1) / average0;
	scaledGray1 = scale * gray0;
	cv::absdiff(gray0, scaledGray1, diff);
#else
	cv::absdiff(gray0, gray1, diff);
#endif

	return 0;
}


cv::Point MatUtils::findLightestRegion(const Mat &image, const cv::Size windowSize)
{
	cv::Point upperLeft;
	Mat gray;
	cv::Rect region;
	int lightSum;
	int lightestSum = 0;

	if (image.channels() > 1) {
		cv::cvtColor(image, gray, COLOR_RGB2GRAY);
	}
	else {
		gray = image;
	}

	region.width = windowSize.width;
	region.height = windowSize.height;

	for (int col = 0; col < gray.cols - windowSize.width; ++col) {
		for (int row = 0; row < gray.rows - windowSize.height; ++row) {
			region.x = col;
			region.y = row;			
			lightSum = this->sum(Mat(image, region));
			if (lightestSum < lightSum) {
				lightestSum = lightSum;
				upperLeft.x = region.x;
				upperLeft.y = region.y;
			}
		}
	}

	return upperLeft;
}


cv::Point MatUtils::findDarkestRegion(const Mat &image, const cv::Size windowSize)
{
	cv::Point upperLeft;
	Mat gray;
	cv::Rect region;
	int lightSum;
	int darkestSum = INT_MAX;

	if (image.channels() > 1) {
		cv::cvtColor(image, gray, COLOR_RGB2GRAY);
	}
	else {
		gray = image;
	}

	region.width = windowSize.width;
	region.height = windowSize.height;

	for (int col = 0; col < gray.cols - windowSize.width; ++col) {
		for (int row = 0; row < gray.rows - windowSize.height; ++row) {
			region.x = col;
			region.y = row;
			lightSum = this->sum(Mat(image, region));
			if (darkestSum > lightSum) {
				darkestSum = lightSum;
				upperLeft.x = region.x;
				upperLeft.y = region.y;
			}
		}
	}

	return upperLeft;
}


void MatUtils::rotate(Mat &src, Mat &dst, int degree)
{
	degree = -degree;//warpAffine默认的旋转方向是逆时针，所以加负号表示转化为顺时针
	double angle = degree  * CV_PI / 180.; // 弧度  
	double a = sin(angle), b = cos(angle);
	int width = src.cols;
	int height = src.rows;
	int width_rotate = int(height * fabs(a) + width * fabs(b));
	int height_rotate = int(width * fabs(a) + height * fabs(b));
	//旋转数组map
	// [ m0  m1  m2 ] ===>  [ A11  A12   b1 ]
	// [ m3  m4  m5 ] ===>  [ A21  A22   b2 ]
	float map[6];
	Mat map_matrix = Mat(2, 3, CV_32F, map);
	// 旋转中心
	CvPoint2D32f center = cvPoint2D32f(width / 2, height / 2);
	CvMat map_matrix2 = map_matrix;
	cv2DRotationMatrix(center, degree, 1.0, &map_matrix2);//计算二维旋转的仿射变换矩阵
	map[2] += (width_rotate - width) / 2;
	map[5] += (height_rotate - height) / 2;
	Mat img_rotate;
	//对图像做仿射变换
	//CV_WARP_FILL_OUTLIERS - 填充所有输出图像的象素。
	//如果部分象素落在输入图像的边界外，那么它们的值设定为 fillval.
	//CV_WARP_INVERSE_MAP - 指定 map_matrix 是输出图像到输入图像的反变换，
	warpAffine(src, img_rotate, map_matrix, Size(width_rotate, height_rotate), 1, 0, 0);

	dst = img_rotate;
}


Mat MatUtils::toGray(Mat image)
{
	Mat gray;

	if (image.channels() > 1) {
		cv::cvtColor(image, gray, COLOR_RGB2GRAY);		
	}
	else {
		gray = image;
	}

	return gray;
}


double MatUtils::calcSimilarity(const Mat A, const Mat B)
{
	if (A.size() != B.size()) {
		return 10000000.0;  // Return a bad value
	}

	// Calculate the L2 relative error between the 2 images.
	double errorL2 = norm(A, B, CV_L2);
	// Scale the value since L2 is summed across all pixels.
	double similarity = errorL2 / (double)(A.rows * A.cols);
	return similarity;
}


void MatUtils::calcLbp(const Mat srcImage, Mat &lbp)
{
		const int nRows = srcImage.rows;
		const int nCols = srcImage.cols;
		cv::Mat resultMat(srcImage.size(), srcImage.type());
		// 遍历图像，生成LBP特征  
		for (int y = 1; y < nRows - 1; y++)
		{
			for (int x = 1; x < nCols - 1; x++)
			{
				// 定义邻域  
				uchar neighbor[8] = { 0 };
				neighbor[0] = srcImage.at<uchar>(y - 1, x - 1);
				neighbor[1] = srcImage.at<uchar>(y - 1, x);
				neighbor[2] = srcImage.at<uchar>(y - 1, x + 1);
				neighbor[3] = srcImage.at<uchar>(y, x + 1);
				neighbor[4] = srcImage.at<uchar>(y + 1, x + 1);
				neighbor[5] = srcImage.at<uchar>(y + 1, x);
				neighbor[6] = srcImage.at<uchar>(y + 1, x - 1);
				neighbor[7] = srcImage.at<uchar>(y, x - 1);
				// 当前图像的处理中心   
				uchar center = srcImage.at<uchar>(y, x);
				uchar temp = 0;
				// 计算LBP的值   
				for (int k = 0; k < 8; k++)
				{
					// 遍历中心点邻域  
					temp += (neighbor[k] >= center)* (1 << k);
				}
				resultMat.at<uchar>(y, x) = temp;
			}
		}

		lbp = resultMat;
}

