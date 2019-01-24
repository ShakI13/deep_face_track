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

#include <cmath>
#include <QDebug>

const double DeepFaceTracker::incr[6] =
{
	50, 40, 80,
	70, 5, 3
};

DeepFaceTracker::DeepFaceTracker() :
	last_x{ 0, 0, 0, 0, 0, 0 }
{
}

DeepFaceTracker::~DeepFaceTracker()
{
}

module_status DeepFaceTracker::start_tracker(QFrame*)
{
	t.start();
	ft.startAsync();

	return status_ok();
}

#ifdef EMIT_NAN
#   include <cstdlib>
#endif

void DeepFaceTracker::data(double *data)
{
	const double dt = t.elapsed_seconds();
	t.start();
/*
#ifdef EMIT_NAN
	if ((rand() % 4) == 0)
	{
		for (int i = 0; i < 6; i++)
			data[i] = 0. / 0.;
	}
	else\*/
//#endif
		float x, y, z, yaw, pitch, roll;

		ft.getTranslations(x, y, z);
		ft.getRotations(yaw, pitch, roll);

		data[0] = x / 5.0f;
		data[1] = y / 5.0f;
		data[2] = z * 100.0f;
		data[3] = yaw * 1.0f;
		data[4] = pitch * 1.0f;
		data[5] = roll * 1.0f;
		/*
		for (int i = 0; i < 6; i++)
		{
			double x = last_x[i] + incr[i] * dt;
			if (x > 180)
				x = -360 + x;
			else if (x < -180)
				x = 360 + x;
			x = copysign(fmod(fabs(x), 360), x);
			last_x[i] = x;

			if (i >= 3)
			{
				data[i] = x;
			}
			else
			{
				data[i] = x * 100 / 180.;
			}
		}*/
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
