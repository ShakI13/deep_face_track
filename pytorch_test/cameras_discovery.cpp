#include "cameras_discovery.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>

#define AWAIT_TIME 1000

CameraDiscovery::CameraDiscovery()
{
	_videoInput.setVerbose(false);
	_selectedId = -1;
	//_tempBuffer = nullptr;
	_enumerateCameras();
}

CameraDiscovery::~CameraDiscovery()
{
	_releaseSelected();
}

int CameraDiscovery::numCameras()
{
	int numDevices = _videoInput.listDevices(true);
	return numDevices;
}

void CameraDiscovery::selectDevice(int deviceId)
{
	int numCameras = this->numCameras();
	if (deviceId < numCameras && _selectedId != deviceId) {
		_releaseSelected();
		_selectedId = deviceId;
		if (deviceId >= 0)
		{
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

void CameraDiscovery::getImageSize(int& width, int& height, int& size)
{
	width = height = size = -1;
	if (_selectedId >= 0 && _videoInput.isDeviceSetup(_selectedId))
	{
		width = _videoInput.getWidth(_selectedId);
		height = _videoInput.getHeight(_selectedId);
		size = _videoInput.getSize(_selectedId);
	}
}

bool CameraDiscovery::getImage(unsigned char* buffer)
{
	if (_selectedId >= 0 && _videoInput.isDeviceSetup(_selectedId))
	{
		int w = _videoInput.getWidth(_selectedId);
		int h = _videoInput.getHeight(_selectedId);
		//if (_tempBuffer == nullptr)
		//	_tempBuffer = new unsigned char[_videoInput.getSize(_selectedId)];
		//cv::Mat frame = cv::Mat(h, w, CV_8UC3);
		bool ret = false;
		ret = _videoInput.getPixels(_selectedId, buffer, false, true);
		//if (frame.isContinuous())
		//	ret = _videoInput.getPixels(_selectedId, frame.data, false, true);
		//else
		//{
		//	ret = _videoInput.getPixels(_selectedId, _tempBuffer, false, true);
		//	frame = cv::Mat(h, w, CV_8UC3, _tempBuffer);
		//}
		//frame.copyTo(img);
		//auto ret = _selectedCap.read(img);
		if (!ret)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
			ret = _videoInput.getPixels(_selectedId, buffer, false, true);
			//if (frame.isContinuous())
			//	ret = _videoInput.getPixels(_selectedId, frame.data, false, true);
			//else
			//{
			//	ret = _videoInput.getPixels(_selectedId, _tempBuffer, false, true);
			//	frame = cv::Mat(h, w, CV_8UC3, _tempBuffer);
			//}
			//if (ret)
			//{
			//	frame.copyTo(img);
			//	return true;
			//}
			//if (!_selectedCap.read(img))
			return false;
		}
		return true;
	}
	return false;
}

bool CameraDiscovery::haveImage()
{
	if (_selectedId >= 0 && _videoInput.isDeviceSetup(_selectedId))
	{
		return _videoInput.isFrameNew(_selectedId);
	}

	return true;
}

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
}

void CameraDiscovery::_releaseSelected()
{
	if (_selectedId >= 0 && _videoInput.isDeviceSetup(_selectedId))
	{
		_videoInput.stopDevice(_selectedId);
		//if (_tempBuffer != nullptr)
		//{
		//	delete[] _tempBuffer;
		//	_tempBuffer = nullptr;
		//}
		//_selectedCap.release();
	}
	_selectedId = -1;
}

void CameraDiscovery::_log(std::string str)
{
	std::cout << str << std::endl;
}
