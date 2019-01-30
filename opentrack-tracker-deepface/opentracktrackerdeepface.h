#pragma once

#include "opentracktrackerdeepface_global.h"
#include "ui_DeepFaceTrackSettingsDialog.h"

#include <cmath>

#include "api/plugin-api.hpp"
#include "compat/timer.hpp"
#include "compat/macros.hpp"

#include "..\pytorch_test\memory_map_data.h"
#include "..\deep_face_track_camera\process.h"
#include "dft_frame_data.h"

class DeepFaceTracker : public ITracker
{
public:
	DeepFaceTracker();
	~DeepFaceTracker() override;
	module_status start_tracker(QFrame *) override;
	void data(double *data) override;

protected:
	Process reco;
	MemoryMapData< DeepFaceTrackMemMap > dat;
	DeepFaceTrack_State state;

private:
	Timer t;
};

class DeepFaceTrackerDialog : public ITrackerDialog
{
	Q_OBJECT

	Ui::Dialog ui;

public:
	DeepFaceTrackerDialog();
	void register_tracker(ITracker *) override {}
	void unregister_tracker() override {}

private slots:
	void doOK();
	void doCancel();
};

class DeepFaceTrackerMeta : public Metadata
{
	Q_OBJECT

	QString name() { return tr("DeepFaceTrack (dev)"); }
	QIcon icon() { return QIcon(); }
};

