// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "opencv2\core.hpp"
#include "opencv2\core\ocl.hpp"

#define AWAIT_TIME 1000

std::string _bytesToStr(unsigned long bytes)
{
	float bytes2 = bytes;
	int num_div = -1;
	while (bytes2 > 1.0f)
	{
		bytes = (unsigned long)bytes2;
		bytes2 /= 1024.0f;
		num_div++;
	}

	std::stringstream ss;

	switch (num_div)
	{
	case 1:
		ss << (unsigned long)bytes << " KB";
		break;
	case 2:
		ss << (unsigned long)bytes << " MB";
		break;
	case 3:
		ss << (unsigned long)bytes << " GB";
		break;
	default:
		ss << bytes << " B";
		break;
	}

	return ss.str();
}

int main()
{
	//std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));

	std::vector< cv::ocl::PlatformInfo > platformsInfo;
	cv::ocl::getPlatfomsInfo(platformsInfo);
	for (int i = 0; i < platformsInfo.size(); i++)
	{
		auto info = platformsInfo[i];
		std::stringstream ss;
		ss << "\n\nPlatform name: " << info.name() << "\nvendor: " << info.vendor() << "\nversion: " << info.version() << "\ndevices: " << info.deviceNumber();
		cv::ocl::Device device;
		for (int j = 0; j < info.deviceNumber(); j++)
		{
			info.getDevice(device, j);
			ss << "\n device: " << device.name();
			ss << "\n version: " << device.version();
			ss << "\n vendor: " << device.vendorName();
			ss << "\n driver version: " << device.driverVersion();
			ss << "\n OpenCL version: " << device.OpenCLVersion();
			ss << "\n OpenCL C version: " << device.OpenCL_C_Version();
			ss << "\n extensions: " << device.extensions();
			ss << "\n globalMemSize: " << _bytesToStr(device.globalMemSize());
			ss << "\n localMemSize:  " << _bytesToStr(device.localMemSize());
		}
		std::cout << ss.str() << std::endl;
		//printf(ss.str().c_str());
	}

	std::getchar();
}

