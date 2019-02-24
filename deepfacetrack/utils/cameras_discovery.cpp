// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#include "cameras_discovery.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>

#include "log.h"

// time to wait on get image failure
#define AWAIT_TIME 1000

CameraDiscovery::CameraDiscovery()
{
	_videoInput.setVerbose(false);
	_selected_id = -1;
	_enumerate_cameras();
}

CameraDiscovery::~CameraDiscovery()
{
	_release_selected();
}

int CameraDiscovery::num_cameras()
{
	int num_devices = _videoInput.listDevices(true);
	return num_devices;
}

void CameraDiscovery::select_device(int deviceId)
{
	int num_cameras = this->num_cameras();
	if (deviceId < num_cameras && _selected_id != deviceId) {
		_release_selected();
		_selected_id = deviceId;
		if (deviceId >= 0)
		{
			bool ret = _videoInput.setupDevice(_selected_id);
			if (!ret)
			{
				std::stringstream ss;
				ss << "Failed to open camera device " << deviceId;
				write_log(ss.str());
			}
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
		}
	}
	else
	{
		_release_selected();
		std::stringstream ss;
		ss << "Unknown camera device " << deviceId;
		write_log(ss.str());
	}
}

void CameraDiscovery::get_image_size(int& width, int& height, int& size)
{
	width = height = size = -1;
	if (_selected_id >= 0 && _videoInput.isDeviceSetup(_selected_id))
	{
		width = _videoInput.getWidth(_selected_id);
		height = _videoInput.getHeight(_selected_id);
		size = _videoInput.getSize(_selected_id);
	}
}

bool CameraDiscovery::get_image(unsigned char* buffer)
{
	if (_selected_id >= 0 && _videoInput.isDeviceSetup(_selected_id))
	{
		int w = _videoInput.getWidth(_selected_id);
		int h = _videoInput.getHeight(_selected_id);
		bool ret = false;
		ret = _videoInput.getPixels(_selected_id, buffer, false, true);
		if (!ret)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
			ret = _videoInput.getPixels(_selected_id, buffer, false, true);
			return false;
		}
		return true;
	}
	return false;
}

bool CameraDiscovery::have_image()
{
	if (_selected_id >= 0 && _videoInput.isDeviceSetup(_selected_id))
	{
		return _videoInput.isFrameNew(_selected_id);
	}

	return true;
}

void CameraDiscovery::_enumerate_cameras()
{
	std::vector< std::string > devices = _videoInput.getDeviceList();
	for (int i = 0; i < devices.size(); i++)
	{
		std::stringstream ss;
		ss << "found " << devices[i];
		write_log(ss.str());
	}

	if (devices.size() == 0)
	{
		write_log("no camera devices found!");
	}
}

void CameraDiscovery::_release_selected()
{
	if (_selected_id >= 0 && _videoInput.isDeviceSetup(_selected_id))
	{
		_videoInput.stopDevice(_selected_id);
	}
	_selected_id = -1;
}
