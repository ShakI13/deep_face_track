#include "torch\torch.h"
#include "torch\script.h"
#include "torch\csrc\jit\argument_spec.h"
#include "opencv2\core.hpp"
#include "opencv2\imgproc.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\videoio.hpp"
#include "opencv2\dnn.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <math.h>

cv::dnn::Net face_detector_net;
std::shared_ptr< torch::jit::script::Module > head_pose_net;

void test_torch()
{
	bool is_enabled = torch::cuda::is_available();
	if (is_enabled)
	{
		auto device_count = torch::cuda::device_count();
		std::cout << "Torch cuda enabled\nnumber of devices: " << device_count << "\n";

		is_enabled = torch::cuda::cudnn_is_available();
		if (is_enabled)
		{
			std::cout << "Torch cudnn enabled\n";
		}
		else
		{
			std::cout << "Torch cudnn NOT enabled\n";
		}
	}
	else
	{
	}
}

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
	std::string model_deploy_path = "C:/dev/deep_face_track/ThirdParty/awesome-face-detection-master/models/deploy.prototxt.txt";
	std::string model_proto_path = "C:/dev/deep_face_track/ThirdParty/awesome-face-detection-master/models/res10_300x300_ssd_iter_140000.caffemodel";
	std::string model_path = "C:\\dev\\deep_face_track\\model.pt";

	std::cout << "Trying to load caffe network...\n";
	auto model = cv::dnn::readNetFromCaffe(
		model_deploy_path,
		model_proto_path
	);
	std::cout << "load done.";

	std::cout << "Trying to load torch network...\n";
	std::ifstream in(model_path, std::ios_base::binary);
	if (in.fail()) {
		std::cout << "failed to open model" << std::endl;
	}
	else {
		std::cout << "successed to open model" << std::endl;
	}
	std::shared_ptr< torch::jit::script::Module > module = torch::jit::load(in);
	std::cout << "load done.";

	std::getchar();
	//module->forward()
}

std::vector<float> cvmat_to_vector(cv::Mat& mat)
{
	std::vector<float> data;

	return data;
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
		//std::cout << "Trying to load caffe network...\n";
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
	if (head_pose_net.get() == nullptr)
	{
		std::string model_path = ".\\models\\model.pt";
		//std::cout << "Trying to load torch network...\n";
		std::ifstream in(model_path, std::ios_base::binary);
		head_pose_net = torch::jit::load(in);
		if (torch::cuda::is_available() && torch::cuda::device_count() > 0)
		{
			std::cout << "selecting CUDA runtime" << std::endl;
			head_pose_net->to(at::kCUDA);
		}
		if (torch::hasHIP())
		{
			std::cout << "selecting HIP runtime" << std::endl;
			head_pose_net->to(at::kHIP);
		}
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

	ss << "frame " << frame_num;

	ss << ", yaw: " << cv::format("%.6f", yaw) << ", pitch:" << cv::format("%.6f", pitch) << ", roll: " << cv::format("%.6f", roll);


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
	bool has_cuda = (torch::cuda::is_available() && torch::cuda::device_count() > 0);
	bool has_hip = (torch::hasHIP());

	//test_opencv();
	//test_torch();
	//test_network();

	float target_fps = 7.0f;
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
	torch::Tensor idx_tensor = torch::from_blob(&idx[0], { 66 }, at::kFloat);
	idx_tensor.to(at::kCPU);

	std::exception_ptr eptr;
	cv::Mat img;
	cv::Mat imgResized;
	auto cap = cv::VideoCapture(0);
	auto ret = cap.read(img);
	int keyCode = -1;
	int frameNum = 0;

	while (keyCode != 27)
	{
		float yaw2 = 0.0f;
		float pitch2 = 0.0f;
		float roll2 = 0.0f;

		float frame_time = cv::getTickCount();
		timers["acquire_image"] = cv::getTickCount();
		ret = cap.read(img);
		cv::resize(img, imgResized, cv::Size(300, 300));
		auto blob = cv::dnn::blobFromImage(imgResized, 1.0, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0));
		timers["acquire_image"] = (cv::getTickCount() - timers["acquire_image"]) / cv::getTickFrequency();

		timers["detect"] = cv::getTickCount();
		std::vector<std::string> outNames = face_detector_net.getUnconnectedOutLayersNames();
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
			int width = right - left + 1;
			int height = bottom - top + 1;
			drawPred(confidence, left, top, right, bottom, img);

			if (left < 0)
				left = 0;
			if (top < 0)
				top = 0;
			if (right >= w)
				right = w - 1;
			if (bottom >= h)
				bottom = h - 1;

			cv::Mat img_face = img(cv::Rect(left, top, right - left, bottom - top));
			try {
				img_face.convertTo(img_face, CV_32F, 1.0 / 255);
				cv::resize(img_face, img_face, cv::Size(224, 224));
				//cv::cvtColor(img_face, img_face, cv::COLOR_RGB2BGR);

				torch::Tensor tensor_face = torch::from_blob(img_face.data, { 1, img_face.rows, img_face.cols, 3 }, at::kFloat);
				if (has_cuda)
				{
					tensor_face = tensor_face.to(at::kCUDA);
				}
				if (has_hip)
				{
					tensor_face = tensor_face.to(at::kHIP);
				}

				tensor_face = tensor_face.permute({ 0, 3, 1, 2 });
				tensor_face[0][0] = tensor_face[0][0].sub(0.485).div(0.229);
				tensor_face[0][1] = tensor_face[0][1].sub(0.456).div(0.224);
				tensor_face[0][2] = tensor_face[0][2].sub(0.406).div(0.225);

				std::vector<torch::jit::IValue> input;
				input.emplace_back(tensor_face);

				// Execute the model and turn its output into a tensor.
				auto output = head_pose_net->forward(input).toTuple()->elements();
				auto yaw = output[0].toTensor();
				auto pitch = output[1].toTensor();
				auto roll = output[2].toTensor();

				yaw = yaw.softmax(1).to(at::kCPU).view({ 66 });
				pitch = pitch.softmax(1).to(at::kCPU).view({ 66 });
				roll = roll.softmax(1).to(at::kCPU).view({ 66 });

				yaw2 = ((yaw * idx_tensor).sum() * 3 - 99).item().toFloat();
				pitch2 = ((pitch * idx_tensor).sum() * 3 - 99).item().toFloat();
				roll2 = ((roll * idx_tensor).sum() * 3 - 99).item().toFloat();

				//std::cout << "yaw:\n " << yaw2 << "\npitch:\n" << pitch2 << "\nroll:\n" << roll2 << std::endl;
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
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
