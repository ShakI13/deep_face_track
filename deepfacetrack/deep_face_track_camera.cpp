// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#include <chrono>
#include <thread>
#include <direct.h>
#include <sstream>

#include "utils/command_line.h"
#include "utils/cameras_discovery.h"
#include "utils/process.h"
#include "utils/log.h"
#include "memmap/dft_recognition_data.h"
#include "memmap/dft_frame_data.h"
#include "memmap/memory_map_data.h"

// safely set name in shared memory
void set_name(DeepFaceTrackImageData& data, std::string str)
{
	memset(&data.name[0], '\0', 256);
	strcat_s(&data.name[0], 256, str.c_str());
}

// set state in shared memory with locking
void set_state(bool have_host, MemoryMapData<DeepFaceTrackMemMap>& host, DeepFaceTrack_State state)
{
	if (have_host)
	{
		host.lock();
		host().state = state;
		host.unlock();
	}
}

int main(int argc, char * argv[])
{
	// point to directory where all dlls and models should be
	_chdir(WORK_DIR);

	bool show_log = false; // save all stdout to file
	bool have_host = true; // is started from external app
	int cam_id = 0; // requested camera id to work with
	int size, width, height; // frame size and dimensions of selected camera
	int win_x = -1, win_y = -1, win_w = -1, win_h = -1; // requested preview window position and size
	bool win_show = true; // shows preview window
	bool is_stopped = false; // is recognition paused
	bool is_processed = false; // is last frame processed
	float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f, pitch = 0.0f, roll = 0.0f; // last results from recognition

	// parse show log command-line option
	if (arg_exists(argv, argv + argc, "-l"))
	{
		show_log = true;
	}
	// start logging if enabled
	open_log(show_log);

	// print to log other command-line options for debug
	write_log("Command line options:");
	for (int i = 0; i < argc; i++)
	{
		write_log(argv[i]);
	}

	// parse camera id command-line option
	char * camStr = get_arg(argv, argv + argc, "-c");
	if (camStr != nullptr)
	{
		bool is_a_number = true;
		int c = 0;
		while (camStr[c] != '\0')
		{
			if (camStr[c] < '0' || camStr[c] > '9')
			{
				is_a_number = false;
				break;
			}
			c++;
		}
		if (is_a_number)
		{
			cam_id = atoi(camStr);
		}
		else
		{
			write_log("failed to parse camera id");
			return -1;
		}
	}

	// tries to open host shared memory, if fails then works in standalone mode
	MemoryMapData<DeepFaceTrackMemMap> host;
	if (!host.create(FT_MM_DATA, FT_MUTEX, true))
		have_host = false;
	else
		write_log("started from host app, host memmap opened");

	write_log("initializing");
	set_state(have_host, host, DeepFaceTrack_State::Initializing);

	CameraDiscovery cam;
	if (cam.num_cameras() == 0)
	{
		write_log("camera error: failed to select a camera");
		set_state(have_host, host, DeepFaceTrack_State::CameraError);
		close_log();
		return -1;
	}

	std::stringstream ss;
	ss << "selecting camera " << cam_id << "...";
	write_log(ss.str());
	cam.select_device(cam_id);
	cam.get_image_size(width, height, size);
	if (have_host)
	{
		host.lock();
		host().frame_w = width;
		host().frame_h = height;
		host.unlock();
	}

	// creates shared memory for image buffer 
	MemoryMapBuffer buffer;
	if (!buffer.create("dft_image_buffer", size, false))
	{
		write_log("dnn error: failed to create image buffer for recognition");
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		close_log();
		return -1;
	}

	if (!cam.get_image((unsigned char*)buffer.ptr()))
	{
		write_log("dnn error: failed to get image from camera");
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		close_log();
		return -1;
	}

	// creates and initializes recognition shared memory
	MemoryMapData<DeepFaceTrackImageData> data;
	if (!data.create(DFT_IMAGE, DFT_IMAGE_MUTEX, false))
	{
		write_log("dnn error: failed to create mapping for recognition");
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		close_log();
		return -1;
	}
	data.lock();
	data().handshake = 0;
	data().captured = true;
	data().processed = false;
	data().is_found = false;
	data().is_stopped = true;
	data().size = size;
	data().width = width;
	data().height = height;
	data().dbg_show = win_show;
	data().has_host = have_host;
	set_name(data(), buffer.name());
	data.unlock();

	write_log("starting recognition app...");
	Process reco;
	if (!reco.create(RECO_APP))
	{
		write_log("dnn error: failed to start recognition app");
		close_log();
		return -1;
	}

	write_log("working");
	set_state(have_host, host, DeepFaceTrack_State::Working);
	if (have_host)
	{
		write_log("stopped");
		set_state(have_host, host, DeepFaceTrack_State::Stopped);
		is_stopped = true;
	}

	// while recognition app is running iterate the loop
	write_log("going to listening loop...");
	while (reco.is_running())
	{
		// 1. check recognition app and their stdin stdout
		reco.iterate();

		// 2. check if last frame recognition is done
		data.lock();
		is_processed = data().processed;
		data().handshake = 0;
		data.unlock();

		// 3. check host status, commands and requested preview window size and position
		if (have_host)
		{
			host.lock();
			if (host().handshake >= MAX_TIMEOUT)
			{
				host.unlock();
				write_log("error: exceed host memmap handshake timeout");
				set_state(have_host, host, DeepFaceTrack_State::DnnError);
				close_log();
				return -1;
			}
			int command = host().command;
			win_x = host().win_x;
			win_y = host().win_y;
			win_w = host().win_w;
			win_h = host().win_h;
			host().handshake = host().handshake + 1;
			host.unlock();
			switch (command)
			{
			case FTNoIR_Tracker_Command::FT_CM_START:
				if (is_stopped)
				{
					is_stopped = false;
					set_state(have_host, host, DeepFaceTrack_State::Working);
					write_log("received command to start, set state working");
				}
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_STOP:
				if (!is_stopped)
				{
					is_stopped = true;
					set_state(have_host, host, DeepFaceTrack_State::Stopped);
					write_log("received command to stop, set state stopped");
				}
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_EXIT:
				if (!is_stopped)
				{
					is_stopped = true;
					set_state(have_host, host, DeepFaceTrack_State::Stopped);
					reco.close();
					write_log("received command to exit, exiting...");
				}
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_SHOWWINDOW:
				win_show = true;
				write_log("received command to show the window");
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_HIDEWINDOW:
				win_show = false;
				write_log("received command to hide a window");
				command = 0;
				break;
			default:
				command = 0;
				break;
			}
			host.lock();
			host().command = command;
			host.unlock();
		}

		// 4. update params for recognition app
		data.lock();
		data().dbg_show = win_show;
		data().win_x = win_x;
		data().win_y = win_y;
		data().win_w = win_w;
		data().win_h = win_h;
		data().is_stopped = is_stopped;
		data.unlock();

		if (is_processed)
		{
			// 5. get last results from recognition app
			bool is_found = false;
			data.lock();
			x = data().x;
			y = data().y;
			z = data().z;
			yaw = data().yaw;
			pitch = data().pitch;
			roll = data().roll;
			is_found = data().is_found;
			data.unlock();

			if (have_host)
			{
				host.lock();
				host().x = x;
				host().y = y;
				host().z = z;
				host().yaw = yaw;
				host().pitch = pitch;
				host().roll = roll;
				host.unlock();
			}

			// 6. update current state
			if (is_stopped)
			{
				set_state(have_host, host, DeepFaceTrack_State::Stopped);
			}
			else
			{
				if (is_found)
				{
					set_state(have_host, host, DeepFaceTrack_State::Working);
				}
				else
				{
					set_state(have_host, host, DeepFaceTrack_State::Initializing);
				}
			}

			// 7. get new image if any and send to recognition app
			if (!cam.have_image())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME * 2));
				continue;
			}
			if (!cam.get_image((unsigned char*)buffer.ptr()))
			{
				write_log("failed to get image from camera");
				set_state(have_host, host, DeepFaceTrack_State::CameraError);
				close_log();
				return -1;
			}
			data.lock();
			data().captured = true;
			data().processed = false;
			data.unlock();
			is_processed = false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	}

	write_log("recognition is not running, exiting...");
	return 0;
}

