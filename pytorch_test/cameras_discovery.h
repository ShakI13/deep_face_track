#pragma once

#include <vector>
#include "opencv2\core.hpp"
#include "opencv2\videoio.hpp"
#include "../../videoInput/videoInputSrcAndDemos/libs/videoInput/videoInput.h"

class CameraDiscovery
{
public:
	CameraDiscovery();
	~CameraDiscovery();

	int numCameras();
	void selectDevice(int deviceId);
	bool getImage(cv::Mat& img);
	//cv::VideoCapture& device();

protected:
	//std::vector<int> _cameraIds;
	int _selectedId;
	//cv::VideoCapture _selectedCap;
	videoInput _videoInput;
	unsigned char* _tempBuffer;

	void _enumerateCameras();
	void _releaseSelected();
	virtual void _log(std::string str);
};