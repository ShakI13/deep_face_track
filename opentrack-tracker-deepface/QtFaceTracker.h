#pragma once

#include <qobject.h>
#include <qthread.h>

#include "../pytorch_test/face_tracker.h"

class QtFaceTracker;
class QtFaceTrackerWorker : public QThread
{
	Q_OBJECT

public:
	void setOwner(QtFaceTracker* owner);

	void run() override;

private:
	QtFaceTracker* _owner;

signals:
	void resultReady();
};

class QtFaceTracker : public QObject, public FaceTracker
{
	Q_OBJECT

public:
	QtFaceTracker();
	~QtFaceTracker();

	bool start() override;
	void startAsync();
	void stop() override;
	bool processImage(bool dbgShow = false) override;
	bool isStopped();

protected:
	void _log(std::string message);
	bool _stopped;

	QtFaceTrackerWorker _worker;
};

