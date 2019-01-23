#pragma once

#include <vector>
#include "opencv2\core.hpp"
#include "opencv2\videoio.hpp"

class CameraDiscovery
{
public:
	CameraDiscovery();
	~CameraDiscovery();

	int numCameras();
	void selectDevice(int deviceId);
	bool getImage(cv::Mat& img);
	cv::VideoCapture& device();

protected:
	std::vector<int> _cameraIds;
	int _selectedId;
	cv::VideoCapture _selectedCap;

	void _enumerateCameras();
	void _releaseSelected();
};