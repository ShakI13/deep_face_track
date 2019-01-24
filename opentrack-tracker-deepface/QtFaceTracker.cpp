#include "QtFaceTracker.h"

#include <qdebug.h>

QtFaceTracker::QtFaceTracker()
{
	_worker.setOwner(this);
	_stopped = true;
}

QtFaceTracker::~QtFaceTracker()
{
	_stopped = true;
	_worker.quit();
	_worker.wait();
}

void QtFaceTracker::startAsync()
{
	_stopped = false;
	_worker.start();
}

bool QtFaceTracker::start()
{
	return FaceTracker::start();
}

void QtFaceTracker::stop()
{
	FaceTracker::stop();
}

bool QtFaceTracker::isStopped()
{
	return _stopped;
}

bool QtFaceTracker::processImage(bool dbgShow)
{
	return FaceTracker::processImage(dbgShow);
}

void QtFaceTracker::_log(std::string message)
{
	qInfo(message.c_str());
}

void QtFaceTrackerWorker::setOwner(QtFaceTracker* owner)
{
	_owner = owner;
}

void QtFaceTrackerWorker::run()
{
	if (_owner == nullptr)
		return;

	bool dbgShow = true;

	_owner->start();
	auto ret = _owner->processImage(dbgShow);
	while (ret)
	{
		if (_owner->isStopped())
			break;
		ret = _owner->processImage(dbgShow);
	}

	_owner->stop();
}
