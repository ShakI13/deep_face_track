#pragma once

#include <vector>
#include "../../videoInput/videoInputSrcAndDemos/libs/videoInput/videoInput.h"

class CameraDiscovery
{
public:
	CameraDiscovery();
	~CameraDiscovery();

	int numCameras();
	void selectDevice(int deviceId);
	void getImageSize(int& width, int& height, int& size);
	bool getImage(unsigned char* buffer);
	bool haveImage();

protected:
	int _selectedId;
	videoInput _videoInput;

	void _enumerateCameras();
	void _releaseSelected();
	virtual void _log(std::string str);
};