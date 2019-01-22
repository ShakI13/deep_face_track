#include "opencv2\core.hpp"
#include "opencv2\imgproc.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\videoio.hpp"
#include "opencv2\dnn.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <math.h>
#include "soft_max.h"

cv::dnn::Net face_detector_net;
cv::dnn::Net head_pose_net;

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

void validate_face_detector_net()
{
	if (face_detector_net.empty())
	{
		std::string model_deploy_path = "./models/deploy.prototxt.txt";
		std::string model_proto_path = "./models/res10_300x300_ssd_iter_140000.caffemodel";
		std::cout << "Trying to load face detection DNN...\n";
		face_detector_net = cv::dnn::readNetFromCaffe(
			model_deploy_path,
			model_proto_path
		);
		std::cout << "selecting opencl runtime" << std::endl;
		face_detector_net.setPreferableBackend(3);
		face_detector_net.setPreferableTarget(2);
		//std::cout << "done.\n";
	}
}

void validate_head_pose_net()
{
	if (head_pose_net.empty())
	{
		std::string model_path = ".\\models\\head_pose.onnx";
		std::cout << "Trying to load head pose DNN...\n";
		head_pose_net = cv::dnn::readNetFromONNX(model_path);
		head_pose_net.setPreferableBackend(3);
		head_pose_net.setPreferableTarget(2);
		//std::cout << "done.\n";
	}
}

void drawPred(float conf, int left, int top, int right, int bottom, cv::Mat& frame)
{
	cv::rectangle(frame, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(0, 255, 0));

	std::string label = cv::format("%.2f", conf);

	int baseLine;
	cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

	top = cv::max(top, labelSize.height);
	rectangle(frame, cv::Point(left, top - labelSize.height),
		cv::Point(left + labelSize.width, top + baseLine), cv::Scalar::all(255), cv::FILLED);
	putText(frame, label, cv::Point(left, top), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar());
}

void handle_eptr(std::exception_ptr eptr) // passing by value is ok
{
	try {
		if (eptr) {
			std::rethrow_exception(eptr);
		}
	}
	catch (const std::exception& e) {
		std::cout << "Caught exception \"" << e.what() << "\"\n";
	}
}

void drawAxis(float yaw, float pitch, float roll, cv::Mat& img, float tdx = 0.0f, float tdy = 0.0f, float size = 100)
{
	float pi = 3.14159265359f;
	pitch = pitch * pi / 180;
	yaw = -(yaw * pi / 180);
	roll = roll * pi / 180;

	if (tdx == 0.0f && tdy == 0.0f)
	{
		int h = img.rows;
		int w = img.cols;
		tdx = w / 2.0f;
		tdy = h / 2.0f;
	}

	// X - Axis pointing to right.drawn in red
	float x1 = size * (cos(yaw) * cos(roll)) + tdx;
	float y1 = size * (cos(pitch) * sin(roll) + cos(roll) * sin(pitch) * sin(yaw)) + tdy;

	// Y - Axis | drawn in green
	//        v
	float x2 = size * (-cos(yaw) * sin(roll)) + tdx;
	float y2 = size * (cos(pitch) * cos(roll) - sin(pitch) * sin(yaw) * sin(roll)) + tdy;

	// Z - Axis(out of the screen) drawn in blue
	float x3 = size * (sin(yaw)) + tdx;
	float y3 = size * (-cos(yaw) * sin(pitch)) + tdy;

	cv::line(img, cv::Point(tdx, tdy), cv::Point(x1, y1), cv::Scalar(0, 0, 255), 3);
	cv::line(img, cv::Point(tdx, tdy), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 3);
	cv::line(img, cv::Point(tdx, tdy), cv::Point(x3, y3), cv::Scalar(255, 0, 0), 3);
}

float get_fps(std::vector<float>& fps)
{
	float avr_fps = 0.0;
	for (int i = 0; i < fps.size(); i++)
	{
		avr_fps += fps[i];
	}
	if (fps.size() > 0)
	{
		avr_fps /= fps.size();
	}
	return avr_fps;
}

void str_report(int frame_num, float yaw, float pitch, float roll, std::vector<float>& fps, std::map< std::string, float >& timers)
{
	std::stringstream ss;

	//ss << "frame " << frame_num;

	ss << "yaw: " << cv::format("%.6f", yaw) << ", pitch:" << cv::format("%.6f", pitch) << ", roll: " << cv::format("%.6f", roll);


	float avr_fps = 0.0;
	for (int i = 0; i < fps.size(); i++)
	{
		avr_fps += fps[i];
	}
	if (fps.size() > 0)
	{
		avr_fps /= fps.size();
	}
	ss << ", fps " << avr_fps;

	for (auto& x : timers)
	{
		ss << ", " << x.first << ": " << cv::format("%.6f", x.second) << " msec";
	}

	std::cout << ss.str() << std::endl;
}

int main()
{
	//test_opencv();
	//test_torch();
	//test_network();

	float target_fps = 25.0f;
	int frame_pause = 1;
	int remLen = 5;

	std::vector< float > fps;
	std::map< std::string, float > timers;
	timers["load_face_det"] = 0.0f;
	timers["load_head_pose"] = 0.0f;
	timers["acquire_image"] = 0.0f;
	timers["detect"] = 0.0f;
	timers["head_pose"] = 0.0f;
	timers["frame_time"] = 0.0f;

	timers["load_face_det"] = cv::getTickCount();
	validate_face_detector_net();
	timers["load_face_det"] = (cv::getTickCount() - timers["load_face_det"]) / cv::getTickFrequency();

	timers["load_head_pose"] = cv::getTickCount();
	validate_head_pose_net();
	timers["load_head_pose"] = (cv::getTickCount() - timers["load_head_pose"]) / cv::getTickFrequency();

	std::vector<float> idx;
	for (int i = 0; i < 66; i++)
		idx.push_back(i);

	std::exception_ptr eptr;
	cv::Mat img;
	cv::Mat imgResized;
	std::cout << "Accessing webcam..." << std::endl;
	auto cap = cv::VideoCapture(0);
	auto ret = cap.read(img);
	if (!ret)
	{
		std::cout << "failed to get an image from the camera! Press any key to close the program" << std::endl;
		std::getchar();
		return -1;
	}
	int keyCode = -1;
	int frameNum = 0;

	std::cout << "Starting acquisition loop..." << std::endl;
	while (keyCode != 27)
	{
		float yaw2 = 0.0f;
		float pitch2 = 0.0f;
		float roll2 = 0.0f;

		float frame_time = cv::getTickCount();
		timers["acquire_image"] = cv::getTickCount();
		ret = cap.read(img);
		if (!ret)
		{
			std::cout << "failed to get an image from the camera! Press any key to close the program" << std::endl;
			std::getchar();
			return -1;
		}
		cv::resize(img, imgResized, cv::Size(300, 300));
		auto blob = cv::dnn::blobFromImage(imgResized, 1.0, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0));
		timers["acquire_image"] = (cv::getTickCount() - timers["acquire_image"]) / cv::getTickFrequency();

		timers["detect"] = cv::getTickCount();
		std::vector<cv::String> outNames = face_detector_net.getUnconnectedOutLayersNames();
		face_detector_net.setInput(blob);
		std::vector<cv::Mat> outs;
		face_detector_net.forward(outs, outNames);
		int h = img.rows;
		int w = img.cols;
		timers["detect"] = (cv::getTickCount() - timers["detect"]) / cv::getTickFrequency();

		timers["head_pose"] = cv::getTickCount();
		float* data = (float*)outs[0].data;
		for (size_t i = 0; i < outs[0].total(); i += 7)
		{
			float confidence = data[i + 2];
			if (confidence <= 0.25f)
				continue;

			
			int left = (int)(data[i + 3] * w);
			int top = (int)(data[i + 4] * h);
			int right = (int)(data[i + 5] * w);
			int bottom = (int)(data[i + 6] * h);
			drawPred(confidence, left, top, right, bottom, img);

			if (left < 0)
				left = 0;
			if (top < 0)
				top = 0;
			if (left >= w)
				left = w - 1;
			if (top >= h)
				top = h - 1;
			if (right < 0)
				right = 0;
			if (bottom < 0)
				bottom = 0;
			if (right >= w)
				right = w - 1;
			if (bottom >= h)
				bottom = h - 1;
			int width = right - left + 1;
			int height = bottom - top + 1;

			if (width < 10 || height < 10)
				continue;

			cv::Mat img_face = img(cv::Rect(left, top, right - left, bottom - top));
			try {
				img_face.convertTo(img_face, CV_32F, 1.0 / 255);
				cv::resize(img_face, img_face, cv::Size(224, 224));
				//cv::cvtColor(img_face, img_face, cv::COLOR_RGB2BGR);

				std::vector< cv::Mat > img_channels(3);
				cv::split(img_face, img_channels);
				img_channels[0] = img_channels[0] - 0.485f;
				img_channels[1] = img_channels[1] - 0.456f;
				img_channels[2] = img_channels[2] - 0.406f;
				img_channels[0] = img_channels[0] / 0.229f;
				img_channels[1] = img_channels[1] / 0.224f;
				img_channels[2] = img_channels[2] / 0.225f;
				cv::merge(img_channels, img_face);

				auto blob_face = cv::dnn::blobFromImage(img_face);

				//// test python image and result
				//auto img_data = load_test_image("C:\\dev\\deep_face_track\\image.txt");
				//auto res_data = load_test_image("C:\\dev\\deep_face_track\\result.txt");
				//// copy image
				//float diff = 0.0f;
				//for (int i = 0; i < img_data.size(); i++)
				//{
				//	float v1 = ((float*)blob_face.data)[i];
				//	float v2 = img_data[i];
				//	diff += abs(v1 - v2);
				//	((float*)blob_face.data)[i] = img_data[i];
				//}

				head_pose_net.setInput(blob_face);
				outNames = { "509", "510", "511" };
				outs.clear();
				head_pose_net.forward(outs, outNames);

				std::vector<float> yaw, pitch, roll;
				yaw.assign((float*)outs[0].datastart, (float*)(outs[0].datastart) + 66);
				pitch.assign((float*)outs[1].datastart, (float*)(outs[1].datastart) + 66);
				roll.assign((float*)outs[2].datastart, (float*)(outs[2].datastart) + 66);

				//// concatenate results
				//std::vector<float> all_res;
				//all_res.insert(all_res.end(), yaw.begin(), yaw.end());
				//all_res.insert(all_res.end(), pitch.begin(), pitch.end());
				//all_res.insert(all_res.end(), roll.begin(), roll.end());
				//diff = 0.0f;
				//for (int i = 0; i < all_res.size(); i++)
				//{
				//	float v1 = all_res[i];
				//	float v2 = res_data[i];
				//	diff += abs(v1 - v2);
				//}

				softmax(yaw);
				softmax(pitch);
				softmax(roll);

				float yaw_sum = 0.0f, pitch_sum = 0.0f, roll_sum = 0.0f;
				for (int i = 0; i < 66; i++)
				{
					yaw_sum += yaw[i] * idx[i];
					pitch_sum += pitch[i] * idx[i];
					roll_sum += roll[i] * idx[i];
				}
				yaw2 = yaw_sum * 3.0f - 99.0f;
				pitch2 = pitch_sum * 3.0f - 99.0f;
				roll2 = roll_sum * 3.0f - 99.0f;
				drawAxis(yaw2, pitch2, roll2, img, (left + right) / 2, (top + bottom) / 2, (bottom - top) / 2);
			}
			catch (std::exception & err) {
				std::cout << err.what() << std::endl;
			}
			catch (...)
			{
				eptr = std::current_exception();
			}
			handle_eptr(eptr);
		}
		timers["head_pose"] = (cv::getTickCount() - timers["head_pose"]) / cv::getTickFrequency();

		cv::namedWindow("webcam", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
		cv::imshow("webcam", img);
		keyCode = cv::waitKey(frame_pause);
		timers["frame_time"] = (cv::getTickCount() - frame_time) / cv::getTickFrequency();
		if (fps.size() < (frameNum % 10 + 1))
		{
			fps.push_back(0.0);
		}
		fps[frameNum % 10] = 1.0 / timers["frame_time"];
		str_report(frameNum, yaw2, pitch2, roll2, fps, timers);
		frameNum++;

		float avr_fps = get_fps(fps);
		if (avr_fps < target_fps && frame_pause > 1)
		{
			if (abs(avr_fps - target_fps) > 5)
				frame_pause -= 5;
			else
				frame_pause--;
		}
		if (avr_fps > target_fps)
		{
			if (abs(avr_fps - target_fps) > 5)
				frame_pause += 5;
			else
				frame_pause++;
		}
	}

	cv::destroyWindow("webcam");

	std::cout << "Done.\n";
	std::getchar();

	return 0;
}