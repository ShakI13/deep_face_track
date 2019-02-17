#include <iostream>
#include <fstream>
#include <memory>
#include <math.h>
#include <fstream>
#include <streambuf>

#include "opencv2\core.hpp"
#include "opencv2\imgproc.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\videoio.hpp"
#include "opencv2\dnn.hpp"
#include "opencv2\core\ocl.hpp"

#include "plaidml++.h"
//#include "plaidml.h"

#include "face_tracker.h"
#include "memmap/memory_map_data.h"
#include "utils/cameras_discovery.h"

void test_plaidml()
{
	std::ifstream t("./.plaidml");
	std::string config((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());

	std::shared_ptr< vertexai::ctx > context;
	context.reset(new vertexai::ctx());

	auto devices = vertexai::plaidml::enumerate_devices(context);
	for (int i = 0; i < devices.size(); i++)
	{
		auto dev = devices[i];
		std::cout << "id: " << dev.id() << std::endl;
		std::cout << "desc: " << dev.description() << std::endl;
		std::cout << "details: " << dev.details() << std::endl;
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

struct InfoPart
{
	int send;
	int receive;
	char message[256];
}; 

void test_set_str(InfoPart& info, std::string str)
{
	memset(&info.message[0], '\0', 256);
	strcat_s(&info.message[0], 256, str.c_str());
}

#define _s(a, b) test_set_str(a,b)

void test_mutex()
{
	auto client = MemoryMapData<InfoPart>();
	auto server = MemoryMapData<InfoPart>();

	int server_send = 0;
	int server_receive = 1;
	std::string server_message;
	server.create("dtf_test_memmap", "dtf_test_mutex", false);
	server.lock();
	server().send = server_send;
	server().receive = server_receive;
	_s(server(), "server: hello client");
	server.unlock();

	int send = 0;
	int receive = 0;
	std::string message;
	client.create("dtf_test_memmap", "dtf_test_mutex");
	client.lock();
	if (receive != client().receive)
	{
		receive = client().receive;
		message = client().message;

		std::cout << "Received from server " << message << std::endl;

		send++;
		client().send = send;
		_s(client(), "client: hello server");
	}
	client.unlock();

	server.lock();
	if (server_send != server().send)
	{
		server_send = server().send;
		server_message = server().message;

		std::cout << "Received from client " << server_message << std::endl;

		server_receive++;
		server().receive = server_receive;
		_s(server(), "server: ok");
	}
	server.unlock();

	client.lock();
	if (receive != client().receive)
	{
		receive = client().receive;
		message = client().message;

		std::cout << "Received from server " << message << std::endl;
	}
	client.unlock();
}

void test_image_corr()
{
	std::vector<float> fps;
	int frameNum = 0;
	CameraDiscovery cams;
	cams.numCameras();
	cams.selectDevice(0);

	int s, w, h;
	cams.getImageSize(w, h, s);
	unsigned char* buffer = new unsigned char[s];
	int keyCode = -1;

	while (cams.getImage(buffer) && keyCode != 27)
	{
		auto frame_start = time_start();
		cv::Mat img = cv::Mat(h, w, CV_8UC3, buffer);
		enhance(img);
		
		cv::namedWindow("cam");
		cv::imshow("cam", img);
		keyCode = cv::waitKey(1);

		auto frame_time = time_elapsed(frame_start);
		if (fps.size() < (frameNum % 10 + 1))
		{
			fps.push_back(0.0);
		}
		fps[frameNum % 10] = 1.0 / frame_time;
		frameNum++;
		float avr_fps = 0.0;
		for (int i = 0; i < fps.size(); i++)
		{
			avr_fps += fps[i];
		}
		if (fps.size() > 0)
		{
			avr_fps /= fps.size();
		}
		std::cout << "fps: " << avr_fps << std::endl;
	}
}

int main()
{
	//test_plaidml();
	//test_opencv();
	//test_torch();
	//test_network();
	//test_mutex();
	test_image_corr();
	std::getchar();
	return 0;

	//std::vector< cv::ocl::PlatformInfo > platformsInfo;
	//cv::ocl::getPlatfomsInfo(platformsInfo);
	//for (int i = 0; i < platformsInfo.size(); i++)
	//{
	//	auto info = platformsInfo[i];
	//	std::stringstream ss;
	//	ss << "\nPlatform name: " << info.name() << " vendor: " << info.vendor() << " version: " << info.version() << " devices: " << info.deviceNumber();
	//	cv::ocl::Device device;
	//	for (int j = 0; j < info.deviceNumber(); j++)
	//	{
	//		info.getDevice(device, j);
	//		ss << "\n device: " << device.name();
	//		ss << "\n version: " << device.version();
	//		ss << "\n vendor: " << device.vendorName();
	//		ss << "\n driver version: " << device.driverVersion();
	//		ss << "\n OpenCL version: " << device.OpenCLVersion();
	//		ss << "\n OpenCL C version: " << device.OpenCL_C_Version();
	//		ss << "\n extensions: " << device.extensions();
	//		ss << "\n globalMemSize: " << device.globalMemSize();
	//		ss << "\n localMemSize:  " << device.localMemSize();
	//	}
	//	std::cout << ss.str() << std::endl;
	//}

	////std::cout << "Context devices: " << std::endl;
	////cv::ocl::Context ctx;
	////ctx = cv::ocl::Context::getDefault();
	//////ctx.create(cv::ocl::Device::TYPE_ALL);

	////for (int i = 0; i < ctx.ndevices(); i++)
	////{
	////	cv::ocl::Device device = ctx.device(i);
	////	std::stringstream ss;
	////	ss << "\n device: " << device.name();
	////	ss << "\n version: " << device.version();
	////	ss << "\n vendor: " << device.vendorName();
	////	std::cout << ss.str() << std::endl;
	////}

	////cv::ocl::setUseOpenCL(false);
	////cv::ocl::setUseOpenCL(true);

	//FaceTracker ft;
	//return ft.debug_loop();
	//std::getchar();
}