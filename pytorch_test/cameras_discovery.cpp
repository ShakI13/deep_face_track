#include "cameras_discovery.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>

#define AWAIT_TIME 250

CameraDiscovery::CameraDiscovery()
{
	_selectedId = -1;
	_enumerateCameras();
}

CameraDiscovery::~CameraDiscovery()
{
	_releaseSelected();
}

int CameraDiscovery::numCameras()
{
	return _cameraIds.size();
}

void CameraDiscovery::selectDevice(int deviceId)
{
	if (deviceId >= 0 && deviceId < _cameraIds.size()) {
		_releaseSelected();
		_selectedId = deviceId;
		bool ret = _selectedCap.open(deviceId);
		if (!ret)
			std::cout << "Failed to open camera device " << deviceId << std::endl;
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	}
	else 
		std::cout << "Unknown camera device " << deviceId << std::endl;
}

bool CameraDiscovery::getImage(cv::Mat & img)
{
	if (_selectedId >= 0 && _selectedCap.isOpened())
	{
		auto ret = _selectedCap.read(img);
		if (!ret)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
			if (!_selectedCap.read(img))
				return false;
		}
		return true;
	}
	return false;
}

cv::VideoCapture& CameraDiscovery::device()
{
	return _selectedCap;
}

void CameraDiscovery::_enumerateCameras()
{
	_cameraIds.clear();
	struct CapDriver {
		int enumValue; 
		std::string enumName; 
		std::string comment;
	};

	// list of all CAP drivers (see highgui_c.h)
	std::vector<CapDriver> drivers;
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_VFW, "CV_CAP_VFW", "platform native" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_FIREWARE, "CV_CAP_FIREWARE", "IEEE 1394 drivers" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_QT, "CV_CAP_QT", "QuickTime" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_UNICAP, "CV_CAP_UNICAP", "Unicap drivers" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_DSHOW, "CV_CAP_DSHOW", "DirectShow (via videoInput)" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_MSMF, "CV_CAP_MSMF", "Microsoft Media Foundation (via videoInput)" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_WINRT, "CV_CAP_WINRT", "Microsoft Windows Runtime using Media Foundation" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_PVAPI, "CV_CAP_PVAPI", "PvAPI, Prosilica GigE SDK" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI, "CV_CAP_OPENNI", "OpenNI (for Kinect)" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI_ASUS, "CV_CAP_OPENNI_ASUS", "OpenNI (for Asus Xtion)" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI2, "CV_CAP_OPENNI", "OpenNI2 (for Kinect)" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI2_ASUS, "CV_CAP_OPENNI_ASUS", "OpenNI2 (for Asus Xtion)" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_ANDROID, "CV_CAP_ANDROID", "Android" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_XIAPI, "CV_CAP_XIAPI", "XIMEA Camera API" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_AVFOUNDATION, "CV_CAP_AVFOUNDATION", "AVFoundation framework for iOS" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_GIGANETIX, "CV_CAP_GIGANETIX", "Smartek Giganetix GigEVisionSDK" });
	drivers.push_back({ cv::VideoCaptureAPIs::CAP_INTELPERC, "CV_CAP_INTELPERC", "Intel Perceptual Computing SDK" });

	std::string winName, driverName, driverComment;
	int driverEnum;
	cv::Mat frame;
	bool found;
	std::cout << "Searching for cameras IDs..." << std::endl;
	
	for (int drv = 0; drv < drivers.size(); drv++)
	{
		driverName = drivers[drv].enumName;
		driverEnum = drivers[drv].enumValue;
		driverComment = drivers[drv].comment;
		std::cout << driverName << "...";
		found = false;

		int maxID = 100; //100 IDs between drivers
		if (driverEnum == cv::VideoCaptureAPIs::CAP_VFW)
			maxID = 10; //VWF opens same camera after 10 ?!?
		else if (driverEnum == cv::VideoCaptureAPIs::CAP_ANDROID)
			maxID = 98; //98 and 99 are front and back cam

		for (int idx = 0; idx < maxID; idx++)
		{
			cv::VideoCapture cap(driverEnum + idx);  // open the camera
			if (cap.isOpened())
			{
				auto w = cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH);
				auto h = cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT);
				auto format = cap.get(cv::VideoCaptureProperties::CAP_PROP_FORMAT);
				auto torbg = cap.get(cv::VideoCaptureProperties::CAP_PROP_CONVERT_RGB);

				auto ret = cap.read(frame);
				if (!ret)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
					if (!cap.read(frame))
						continue;
				}

				std::cout << std::endl << driverName << " " << idx << " w: " << w << ", h: " << h << ", format: " << format << ", torgb: " << torbg << std::endl;
			
				_cameraIds.push_back(idx);
			}
		}
	}
	std::cout << std::endl;
}

void CameraDiscovery::_releaseSelected()
{
	if (_selectedId >= 0 && _selectedCap.isOpened())
	{
		_selectedCap.release();
	}
	_selectedId = -1;
}
