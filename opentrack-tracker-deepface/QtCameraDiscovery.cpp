#include "QtCameraDiscovery.h"

#include <qdebug.h>

QtCameraDiscovery::QtCameraDiscovery()
{
}


QtCameraDiscovery::~QtCameraDiscovery()
{
}

void QtCameraDiscovery::_log(std::string message)
{
	qInfo(message.c_str());
}
