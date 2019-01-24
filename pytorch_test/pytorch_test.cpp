#include <iostream>
#include <fstream>
#include <memory>
#include <math.h>

#include "opencv2\core.hpp"
#include "opencv2\imgproc.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\videoio.hpp"
#include "opencv2\dnn.hpp"

#include "face_tracker.h"

void test_opencv()
{
	cv::Mat img;

	std::cout << "Checking imshow...\n";

	img = cv::imread("C:\\Users\\Stanislav\\Videos\\Euro Truck Simulator 2\\Euro Truck Simulator 2 Screenshot 2019.01.19 - 01.38.45.50.png");

	cv::namedWindow("image");
	cv::imshow("image", img);
	cv::waitKey(5000);

	std::cout << "Checking webcam...\n";

	auto cap = cv::VideoCapture(0);

	auto ret = cap.read(img);

	while (ret)
	{
		cv::namedWindow("webcam", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
		cv::imshow("webcam", img);
		cv::waitKey(33);

		ret = cap.read(img);
	}
}

void test_network()
{
	std::string model_deploy_path = "./models/deploy.prototxt.txt";
	std::string model_proto_path = "./models/res10_300x300_ssd_iter_140000.caffemodel";
	std::string model_path = ".\\models\\head_pose.onnx";

	std::cout << "Trying to load caffe network...\n";
	auto model = cv::dnn::readNetFromCaffe(
		model_deploy_path,
		model_proto_path
	);
	std::cout << "load done.";

	std::cout << "Trying to load caffe network...\n";
	auto module = cv::dnn::readNetFromONNX(model_path);
	std::cout << "load done.";

	std::getchar();
}

std::vector<float> load_test_image(std::string path, int batchSize = 1, int numChannels = 3, int width = 224, int height = 224)
{
	//std::cout << "creating input stream..." << std::endl;
	std::fstream in(path);
	std::string line;

	std::vector<float> data;

	//std::cout << "getting line..." << std::endl;
	while (std::getline(in, line))
	{
		//std::cout << "creating string stream..." << std::endl;
		float value;
		std::stringstream ss(line);

		while (ss >> value)
		{
			//std::cout << "got value " << value << std::endl;
			data.push_back(value);
		}
	}

	return data;

	////std::cout << "constructing mat" << std::endl;
	//cv::Mat mat = cv::Mat(cv::Size(width, height), CV_32FC3, (void*)&data[0]);
	////std::cout << "coping mat" << std::endl;
	//cv::Mat mat2 = mat.clone();
	////std::cout << "returning mat" << std::endl;
	//return mat2;
}

int main()
{
	//test_opencv();
	//test_torch();
	//test_network();

	FaceTracker ft;
	return ft.debug_loop();
}