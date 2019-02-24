/* Copyright (c) 2014, Stanislaw Halik <sthalik@misaki.pl>

 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#include "opentracktrackerdeepface.h"
#include "api/plugin-api.hpp"
#include "compat/math-imports.hpp"

#include <QPushButton>
#include <qmessagebox.h>

#include <cmath>
#include <QDebug>

#include <iostream>
#include <cstdio>
#include <string>

void open_log()
{
	using namespace std;
	//freopen("output.txt", "w", stdout);
	freopen("dft_error.txt", "w", stderr);
}

void write_log(std::string line)
{
	using namespace std;
	cout << line << endl;
}

DeepFaceTracker::DeepFaceTracker()
{
	open_log();
	write_log("creating memmap...");
	state = DeepFaceTrack_State::Initializing;
	if (!dat.create(FT_MM_DATA, FT_MUTEX, false))
	{
		//QMessageBox msgBox;
		//msgBox.setText("Failed to create memmap!");
		//msgBox.setInformativeText("Attemp to create memmap failed");
		//msgBox.setStandardButtons(QMessageBox::Ok);
		//msgBox.show();
		state = DeepFaceTrack_State::CameraError;
		write_log("creating memmap failed");
		return;
	}

	dat.lock();
	dat().handshake = dat().command = dat().state = state;
	dat().x = dat().y = dat().z = dat().yaw = dat().pitch = dat().roll = 0.0f;
	dat.unlock();
	write_log("creating memmap ok");
}

DeepFaceTracker::~DeepFaceTracker()
{
	if (reco.is_running())
	{
		dat.lock();
		dat().handshake = 0;
		dat().command = FTNoIR_Tracker_Command::FT_CM_EXIT;
		dat.unlock();
	}
}

module_status DeepFaceTracker::start_tracker(QFrame*)
{
	t.start();

	if (state == DeepFaceTrack_State::Initializing)
		if (!reco.is_running())
		{
			write_log("creating reco...");
			reco.create("./deepfacetrack/deep_face_track_camera.exe");
			write_log("creating reco done");
		}

	if (!reco.is_running())
	{
		write_log("reco not started");
		//QMessageBox msgBox;
		//msgBox.setText("Failed to start reco!");
		//msgBox.setInformativeText("Attemp to start reco failed");
		//msgBox.setStandardButtons(QMessageBox::Ok);
		//msgBox.show();
		state = DeepFaceTrack_State::CameraError;
		return error("Attemp to start reco failed");
	}

	//write_log("starting recognition");
	dat.lock();
	dat().command = FTNoIR_Tracker_Command::FT_CM_START;
	dat().handshake = 0;
	dat.unlock();

	return status_ok();
}

#ifdef EMIT_NAN
#   include <cstdlib>
#endif

void DeepFaceTracker::data(double *data)
{
	const double dt = t.elapsed_seconds();
	t.start();

	float x, y, z, yaw, pitch, roll;
	x = y = z = yaw = pitch = roll = 0.0f;
	if (reco.is_running() && reco.iterate())
	{
		dat.lock();
		dat().handshake = 0;
		state = (DeepFaceTrack_State)dat().state;
		if (state == DeepFaceTrack_State::Working)
		{
			x = dat().x;
			y = dat().y;
			z = dat().z;
			yaw = dat().yaw;
			pitch = dat().pitch;
			roll = dat().roll;
		}
		dat.unlock();
	}
	else
		state = DeepFaceTrack_State::DnnError;

	data[0] = x / 5.0f;
	data[1] = y / 5.0f;
	data[2] = z * 100.0f;
	data[3] = yaw * 1.0f;
	data[4] = pitch * 1.0f;
	data[5] = roll * 1.0f;
	//write_log("data: done");
	write_log("state: " + std::to_string(state));
}

DeepFaceTrackerDialog::DeepFaceTrackerDialog()
{
	ui.setupUi(this);

	connect(ui.buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton* btn) {
		if (btn == ui.buttonBox->button(QDialogButtonBox::Abort))
			*(volatile int*)0 = 0;
	});

	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(doOK()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(doCancel()));
}

void DeepFaceTrackerDialog::doOK()
{
	//s.b->save();
	close();
}

void DeepFaceTrackerDialog::doCancel()
{
	close();
}

OPENTRACK_DECLARE_TRACKER(DeepFaceTracker, DeepFaceTrackerDialog, DeepFaceTrackerMeta)
