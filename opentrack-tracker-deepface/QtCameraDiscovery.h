#pragma once

#include "../pytorch_test/cameras_discovery.h"

class QtCameraDiscovery :
	protected CameraDiscovery
{
public:
	QtCameraDiscovery();
	~QtCameraDiscovery();
protected:
	void _log(std::string message) override;
};

