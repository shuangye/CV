#include <osa/osa.h>
#include <dscv/dscv_rectutils.hpp>


cv::Point RectUtils::bottomRight(const cv::Rect &rect)
{
	return Point(rect.x + rect.width, rect.y + rect.height);
}

int RectUtils::CompareRect(const cv::Rect &rect1, const cv::Rect &rect2) {
	int area1 = rect1.size().width * rect1.size().height;
	int area2 = rect2.size().width * rect2.size().height;
	if (area1 > area2) {
		return 1;
	}
	else if (area1 < area2) {
		return -1;
	}
	else {
		return 0;
	}
}


cv::Rect RectUtils::CoveredRect(const vector<cv::Rect> rects)
{
	cv::Rect result(0, 0, 0, 0);
	Point refPoints[2], targetPoints[2];
	Point p1, p2;

	switch (rects.size()) {
	case 0:
		break;
	case 1:
		result = rects[0];
		break;
	default:
		refPoints[0].x = rects[0].x;
		refPoints[0].y = rects[0].y;
		refPoints[1] = bottomRight(rects[0]);
		for (size_t i = 1; i < rects.size(); ++i) {
			targetPoints[0].x = rects[i].x;
			targetPoints[0].y = rects[i].y;
			targetPoints[1] = bottomRight(rects[i]);
			p1.x = OSA_min(refPoints[0].x, targetPoints[0].x);
			p1.y = OSA_min(refPoints[0].y, targetPoints[0].y);
			p2.x = OSA_max(refPoints[1].x, targetPoints[1].x);
			p2.y = OSA_max(refPoints[1].y, targetPoints[1].y);
		}
		result = Rect(p1, p2);
		break;
	}

	return result;
}
