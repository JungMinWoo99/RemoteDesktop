#include <opencv2/opencv.hpp>

using namespace cv;

void main() {
	Mat Image = imread("lena.jpg", IMREAD_COLOR);
	namedWindow("display", WINDOW_AUTOSIZE);
	imshow("display", Image);
	imwrite("lena_new.jpg", Image);

	waitKey(0);
}