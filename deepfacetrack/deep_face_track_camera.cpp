#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <memory>
#include <chrono>
#include <thread>
#include <direct.h>
#include <sstream>

#include "utils/cameras_discovery.h"
#include "utils/process.h"
#include "memmap/dft_image_data.h"
#include "memmap/dft_frame_data.h"
#include "memmap/memory_map_data.h"

#define MAX_TIMEOUT 100
#define AWAIT_TIME 1

int test_cameras()
{
	CameraDiscovery cams;

	std::cout << "Found " << cams.numCameras() << std::endl;
	if (cams.numCameras() > 0)
	{
		cams.selectDevice(0);
		std::cout << "Selected camera " << 0 << std::endl;

		int w = 0, h = 0, s = 0;
		cams.getImageSize(w, h, s);
		if (w > 0 && h > 0)
		{
			unsigned char* buffer = new unsigned char[s];
			bool ret = cams.getImage(buffer);
			if (!ret)
			{
				std::cout << "Failed to get an image from the camera" << std::endl;
				return -1;
			}
			delete[] buffer;
		}

		std::cout << "All ok" << std::endl;
		return 0;
	}

	std::cout << "Failed to find a camera" << std::endl;

	return -1;
}

int test_gpu_info()
{
	std::cout << "Starting gpu_info.exe..." << std::endl;
	Process reco;
	reco.create("./gpu_info.exe");
	while (reco.iterate())
	{

	}
	reco.close();
	std::cout << "done." << std::endl;
	std::getchar();
	return 0;
}

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
	{
		return *itr;
	}
	return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}

void test_set_str(DeepFaceTrackImageData& data, std::string str)
{
	memset(&data.name[0], '\0', 256);
	strcat_s(&data.name[0], 256, str.c_str());
}

#define _s(a, b) test_set_str(a,b)

void set_state(bool have_host, MemoryMapData<DeepFaceTrackMemMap>& host, DeepFaceTrack_State state)
{
	if (have_host)
	{
		host.lock();
		host().state = state;
		host.unlock();
	}
}

FILE* _log;

void _open_log(bool show_log)
{
	if (show_log)
		_log = fopen("dft_camera.log", "w");
	else
		_log = NULL;
}

void _close_log()
{
	if (_log != NULL)
		fclose(_log);
}

void _write_log(std::string line)
{
	using namespace std;
	cout << line << endl;
	if (_log != NULL)
	{
		fputs(line.c_str(), _log);
		fputs("\n", _log);
	}
}

int main(int argc, char * argv[])
{
	_chdir(".\\deepfacetrack");

	bool show_log = false;
	int cam_id = 0;
	if (cmdOptionExists(argv, argv + argc, "-l"))
	{
		show_log = true;
	}
	_open_log(show_log);

	_write_log("Command line options:");
	for (int i = 0; i < argc; i++)
	{
		_write_log(argv[i]);
	}

	char * camStr = getCmdOption(argv, argv + argc, "-c");
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
			_write_log("failed to parse camera id");
			return -1;
		}
	}

	bool have_host = true;
	MemoryMapData<DeepFaceTrackMemMap> host;
	if (!host.create(FT_MM_DATA, FT_MUTEX, true))
		have_host = false;
	else
		_write_log("started from host app, host memmap opened");

	bool win_show = true;// !have_host;
	int win_x = -1, win_y = -1, win_w = -1, win_h = -1;

	_write_log("initializing");
	set_state(have_host, host, DeepFaceTrack_State::Initializing);

	CameraDiscovery cams;
	if (cams.numCameras() == 0)
	{
		_write_log("camera error: failed to select a camera");
		set_state(have_host, host, DeepFaceTrack_State::CameraError);
		_close_log();
		return -1;
	}

	std::stringstream ss;
	ss << "selection camera " << cam_id << "...";
	_write_log(ss.str());
	cams.selectDevice(cam_id);
	int size, width, height;
	cams.getImageSize(width, height, size);
	if (have_host)
	{
		host.lock();
		host().frame_w = width;
		host().frame_h = height;
		host.unlock();
	}

	MemoryMapBuffer buffer;
	if (!buffer.create("dft_image_buffer", size, false))
	{
		//std::cout << "failed to create memorymap buffer" << std::endl;
		_write_log("dnn error: failed to create image buffer for recognition");
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		_close_log();
		return -1;
	}

	if (!cams.getImage((unsigned char*)buffer.ptr()))
	{
		//std::cout << "failed to get image from camera" << std::endl;
		_write_log("dnn error: failed to get image from camera");
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		_close_log();
		return -1;
	}

	MemoryMapData<DeepFaceTrackImageData> data;
	if (!data.create(DFT_IMAGE, DFT_IMAGE_MUTEX, false))
	{
		_write_log("dnn error: failed to create mapping for recognition");
		//std::cout << "failed to create recognition mapping" << std::endl;
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		_close_log();
		return -1;
	}

	data.lock();
	data().handshake = 0;
	data().size = size;
	data().width = width;
	data().height = height;
	data().captured = true;
	data().processed = false;
	data().dbg_show = win_show;
	data().is_found = false;
	data().is_stopped = true;
	data().has_host = have_host;
	_s(data(), buffer.mapname());
	data.unlock();

	_write_log("starting recognition app...");
	Process reco;
	if (!reco.create("./deep_face_track_recognition.exe"))
	{
		_write_log("dnn error: failed to start recognition app");
		_close_log();
		return -1;
	}

	_write_log("working");
	set_state(have_host, host, DeepFaceTrack_State::Working);
	bool is_stoped = false;
	if (have_host)
	{
		_write_log("stopped");
		set_state(have_host, host, DeepFaceTrack_State::Stopped);
		is_stoped = true;
	}

	_write_log("going to listening loop...");
	bool is_processed = false;
	float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	int frame_num = 0;
	while (reco.is_running())
	{
		reco.iterate();
	
		data.lock();
		is_processed = data().processed;
		data().handshake = 0;
		data.unlock();

		if (have_host)
		{
			//_write_log("checking host memmap...");
			host.lock();
			if (host().handshake >= MAX_TIMEOUT)
			{
				host.unlock();
				_write_log("error: exceed host memmap handshake timeout");
				//std::cout << "exceed max timeout" << std::endl;
				set_state(have_host, host, DeepFaceTrack_State::DnnError);
				_close_log();
				return -1;
			}
			int command = host().command;
			//win_show = host().win_show;
			win_x = host().win_x;
			win_y = host().win_y;
			win_w = host().win_w;
			win_h = host().win_h;
			host().handshake = host().handshake + 1;
			host.unlock();
			switch (command)
			{
			case FTNoIR_Tracker_Command::FT_CM_START:
				if (is_stoped)
				{
					is_stoped = false;
					set_state(have_host, host, DeepFaceTrack_State::Working);
					_write_log("received command to start, set state working");
				}
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_STOP:
				if (!is_stoped)
				{
					is_stoped = true;
					set_state(have_host, host, DeepFaceTrack_State::Stopped);
					_write_log("received command to stop, set state stopped");
				}
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_EXIT:
				if (!is_stoped)
				{
					is_stoped = true;
					set_state(have_host, host, DeepFaceTrack_State::Stopped);
					reco.close();
					_write_log("received command to exit, exiting...");
				}
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_SHOWWINDOW:
				win_show = true;
				_write_log("received command to show the window");
				command = 0;
				break;
			case FTNoIR_Tracker_Command::FT_CM_HIDEWINDOW:
				win_show = false;
				_write_log("received command to hide a window");
				command = 0;
				break;
			default:
				//_write_log("waiting for command from host");
				command = 0;
				break;
			}
			host.lock();
			host().command = command;
			host.unlock();
		}

		data.lock();
		data().dbg_show = win_show;
		data().win_x = win_x;
		data().win_y = win_y;
		data().win_w = win_w;
		data().win_h = win_h;
		data().is_stopped = is_stoped;
		data.unlock();

		//if (frame_num % (15 * 10) == 0)
		//{
		//	//win_show = !win_show;
		//	win_x += 20;
		//	win_y += 20;
		//}

		frame_num++;

		if (is_processed)
		{
			bool is_found = false;
			//_write_log("frame recognized, getting results");
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
				//_write_log("sending results to host...");
				host.lock();
				if (host().handshake >= MAX_TIMEOUT)
				{
					host.unlock();
					_write_log("error: exceed host memmap handshake timeout");
					set_state(have_host, host, DeepFaceTrack_State::DnnError);
					_close_log();
					return -1;
				}
				host().x = x;
				host().y = y;
				host().z = z;
				host().yaw = yaw;
				host().pitch = pitch;
				host().roll = roll;
				host.unlock();
			}

			if (is_stoped)
			{
				set_state(have_host, host, DeepFaceTrack_State::Stopped);
				_write_log("stopped");
			}
			else
			{
				if (is_found)
				{
					set_state(have_host, host, DeepFaceTrack_State::Working);
					//_write_log("working");
				}
				else
				{
					set_state(have_host, host, DeepFaceTrack_State::Initializing);
					//_write_log("initializing");
				}
			}

			if (!cams.haveImage())
			{
				//_write_log("no new frame from image");
				std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME * 2));
				continue;
			}
			if (!cams.getImage((unsigned char*)buffer.ptr()))
			{
				//std::cout << "failed to get image from camera" << std::endl;
				//data.unlock();
				_write_log("failed to get image from camera");
				set_state(have_host, host, DeepFaceTrack_State::CameraError);
				_close_log();
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

	_write_log("recognition is not running, exiting...");
	return 0;
}

