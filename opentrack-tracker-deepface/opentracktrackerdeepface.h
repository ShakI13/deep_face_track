#pragma once

#include "opentracktrackerdeepface_global.h"
#include "ui_DeepFaceTrackSettingsDialog.h"
#include "api/plugin-api.hpp"
#include "compat/timer.hpp"
#include "compat/macros.hpp"

#include <cmath>

class DeepFaceTracker : public ITracker
{
public:
	DeepFaceTracker();
	~DeepFaceTracker() override;
	module_status start_tracker(QFrame *) override;
	void data(double *data) override;

private:
	static const double incr[6];
	double last_x[6];
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

