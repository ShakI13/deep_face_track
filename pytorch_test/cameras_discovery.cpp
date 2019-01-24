#include "cameras_discovery.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>

#define AWAIT_TIME 1000

CameraDiscovery::CameraDiscovery()
{
	_selectedId = -1;
	_tempBuffer = nullptr;
	_enumerateCameras();
}

CameraDiscovery::~CameraDiscovery()
{
	_releaseSelected();
}

int CameraDiscovery::numCameras()
{
	int numDevices = _videoInput.listDevices();
	return numDevices;
	//return _cameraIds.size();
}

void CameraDiscovery::selectDevice(int deviceId)
{
	int numCameras = this->numCameras();
	if (deviceId < numCameras && _selectedId != deviceId) {
		_releaseSelected();
		_selectedId = deviceId;
		if (deviceId >= 0)
		{
			//bool ret = _selectedCap.open(deviceId);
			bool ret = _videoInput.setupDevice(_selectedId);
			if (!ret)
			{
				std::stringstream ss;
				ss << "Failed to open camera device " << deviceId;
				_log(ss.str());
			}
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
		}
	}
	else
	{
		_releaseSelected();
		std::stringstream ss;
		ss << "Unknown camera device " << deviceId;
		_log(ss.str());
	}
}

bool CameraDiscovery::getImage(cv::Mat & img)
{
	if (_selectedId >= 0 && _videoInput.isDeviceSetup(_selectedId))
	{
		int w = _videoInput.getWidth(_selectedId);
		int h = _videoInput.getHeight(_selectedId);
		if (_tempBuffer == nullptr)
			_tempBuffer = new unsigned char[_videoInput.getSize(_selectedId)];
		cv::Mat frame = cv::Mat(h, w, CV_8UC3);
		bool ret = false;
		if (frame.isContinuous())
			ret = _videoInput.getPixels(_selectedId, frame.data, false, true);
		else
		{
			ret = _videoInput.getPixels(_selectedId, _tempBuffer, false, true);
			frame = cv::Mat(h, w, CV_8UC3, _tempBuffer);
		}
		frame.copyTo(img);
		//auto ret = _selectedCap.read(img);
		if (!ret)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
			if (frame.isContinuous())
				ret = _videoInput.getPixels(_selectedId, frame.data, false, true);
			else
			{
				ret = _videoInput.getPixels(_selectedId, _tempBuffer, false, true);
				frame = cv::Mat(h, w, CV_8UC3, _tempBuffer);
			}
			if (ret)
			{
				frame.copyTo(img);
				return true;
			}
			//if (!_selectedCap.read(img))
			return false;
		}
		return true;
	}
	return false;
}

//cv::VideoCapture& CameraDiscovery::device()
//{
//	return _selectedCap;
//}

void CameraDiscovery::_enumerateCameras()
{
	std::vector< std::string > devices = _videoInput.getDeviceList();
	for (int i = 0; i < devices.size(); i++)
	{
		std::stringstream ss;
		ss << "found " << devices[i];
		_log(ss.str());
	}

	if (devices.size() == 0)
	{
		_log("no camera devices found!");
	}

	//_cameraIds.clear();
	//struct CapDriver {
	//	int enumValue; 
	//	std::string enumName; 
	//	std::string comment;
	//};

	//// list of all CAP drivers (see highgui_c.h)
	//std::vector<CapDriver> drivers;
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_VFW, "CV_CAP_VFW", "platform native" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_FIREWARE, "CV_CAP_FIREWARE", "IEEE 1394 drivers" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_QT, "CV_CAP_QT", "QuickTime" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_UNICAP, "CV_CAP_UNICAP", "Unicap drivers" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_DSHOW, "CV_CAP_DSHOW", "DirectShow (via videoInput)" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_MSMF, "CV_CAP_MSMF", "Microsoft Media Foundation (via videoInput)" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_WINRT, "CV_CAP_WINRT", "Microsoft Windows Runtime using Media Foundation" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_PVAPI, "CV_CAP_PVAPI", "PvAPI, Prosilica GigE SDK" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI, "CV_CAP_OPENNI", "OpenNI (for Kinect)" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI_ASUS, "CV_CAP_OPENNI_ASUS", "OpenNI (for Asus Xtion)" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI2, "CV_CAP_OPENNI", "OpenNI2 (for Kinect)" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_OPENNI2_ASUS, "CV_CAP_OPENNI_ASUS", "OpenNI2 (for Asus Xtion)" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_ANDROID, "CV_CAP_ANDROID", "Android" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_XIAPI, "CV_CAP_XIAPI", "XIMEA Camera API" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_AVFOUNDATION, "CV_CAP_AVFOUNDATION", "AVFoundation framework for iOS" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_GIGANETIX, "CV_CAP_GIGANETIX", "Smartek Giganetix GigEVisionSDK" });
	//drivers.push_back({ cv::VideoCaptureAPIs::CAP_INTELPERC, "CV_CAP_INTELPERC", "Intel Perceptual Computing SDK" });

	//std::string winName, driverName, driverComment;
	//std::stringstream ss;
	//int driverEnum;
	//cv::Mat frame;
	//bool found;
	//_log("Searching for cameras IDs...");
	//
	//for (int drv = 0; drv < drivers.size(); drv++)
	//{
	//	driverName = drivers[drv].enumName;
	//	driverEnum = drivers[drv].enumValue;
	//	driverComment = drivers[drv].comment;
	//	
	//	ss << driverName << "...";
	//	_log(ss.str());
	//	found = false;

	//	int maxID = 100; //100 IDs between drivers
	//	if (driverEnum == cv::VideoCaptureAPIs::CAP_VFW)
	//		maxID = 10; //VWF opens same camera after 10 ?!?
	//	else if (driverEnum == cv::VideoCaptureAPIs::CAP_ANDROID)
	//		maxID = 98; //98 and 99 are front and back cam

	//	for (int idx = 0; idx < maxID; idx++)
	//	{
	//		cv::VideoCapture cap(driverEnum + idx);  // open the camera
	//		if (cap.isOpened())
	//		{
	//			auto w = cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH);
	//			auto h = cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT);
	//			auto format = cap.get(cv::VideoCaptureProperties::CAP_PROP_FORMAT);
	//			auto torbg = cap.get(cv::VideoCaptureProperties::CAP_PROP_CONVERT_RGB);

	//			auto ret = cap.read(frame);
	//			if (!ret)
	//			{
	//				std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	//				if (!cap.read(frame))
	//					continue;
	//			}

	//			ss.clear();
	//			ss << std::endl << driverName << " " << idx << " w: " << w << ", h: " << h << ", format: " << format << ", torgb: " << torbg;
	//			_log(ss.str());
	//		
	//			_cameraIds.push_back(idx);
	//		}
	//	}
	//	ss.clear();
	//}
	//_log("");
}

void CameraDiscovery::_releaseSelected()
{
	if (_selectedId >= 0 && _videoInput.isDeviceSetup(_selectedId))
	{
		_videoInput.stopDevice(_selectedId);
		if (_tempBuffer != nullptr)
		{
			delete[] _tempBuffer;
			_tempBuffer = nullptr;
		}
		//_selectedCap.release();
	}
	_selectedId = -1;
}

void CameraDiscovery::_log(std::string str)
{
	std::cout << str << std::endl;
}
