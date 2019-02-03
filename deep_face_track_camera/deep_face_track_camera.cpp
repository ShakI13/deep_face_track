#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <memory>
#include <chrono>
#include <thread>

#include "../pytorch_test/cameras_discovery.h"
#include "process.h"
#include "../opentrack-tracker-deepface/dft_image_data.h"
#include "../opentrack-tracker-deepface/dft_frame_data.h"
#include "../pytorch_test/memory_map_data.h"

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

int main(int argc, char * argv[])
{
	//if (cmdOptionExists(argv, argv + argc, "-h"))
	//{
	//	// Do stuff
	//}

	//char * filename = getCmdOption(argv, argv + argc, "-f");

	//if (filename)
	//{
	//	// Do interesting things
	//	// ...
	//}

	//return test_cameras();
	//return test_gpu_info();

	bool have_host = true;
	MemoryMapData<DeepFaceTrackMemMap> host;
	if (!host.create(FT_MM_DATA, FT_MUTEX, true))
		have_host = false;

	set_state(have_host, host, DeepFaceTrack_State::Initializing);

	CameraDiscovery cams;
	if (cams.numCameras() == 0)
	{
		std::cout << "failed to select a camera" << std::endl;
		set_state(have_host, host, DeepFaceTrack_State::CameraError);
		return -1;
	}
	cams.selectDevice(0);
	int size, width, height;
	cams.getImageSize(width, height, size);

	MemoryMapBuffer buffer;
	if (!buffer.create("dft_image_buffer", size, false))
	{
		std::cout << "failed to create memorymap buffer" << std::endl;
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		return -1;
	}

	if (!cams.getImage((unsigned char*)buffer.ptr()))
	{
		std::cout << "failed to get image from camera" << std::endl;
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		return -1;
	}

	MemoryMapData<DeepFaceTrackImageData> data;
	if (!data.create(DFT_IMAGE, DFT_IMAGE_MUTEX, false))
	{
		std::cout << "failed to create recognition mapping" << std::endl;
		set_state(have_host, host, DeepFaceTrack_State::DnnError);
		return -1;
	}
	data.lock();
	data().handshake = 0;
	data().size = size;
	data().width = width;
	data().height = height;
	data().captured = true;
	data().processed = false;
	data().dbg_show = true;
	_s(data(), buffer.mapname());
	data.unlock();

	Process reco;
	reco.create("./deep_face_track_recognition.exe");

	set_state(have_host, host, DeepFaceTrack_State::Working);
	bool is_stoped = false;
	if (have_host)
	{
		set_state(have_host, host, DeepFaceTrack_State::Stopped);
		is_stoped = true;
	}

	bool is_processed = false;
	float x, y, z, yaw, pitch, roll;
	while (reco.is_running())
	{
		reco.iterate();
	
		data.lock();
		is_processed = data().processed;
		data().handshake = 0;
		data.unlock();

		if (have_host)
		{
			host.lock();
			if (host().handshake >= MAX_TIMEOUT)
			{
				host.unlock();
				std::cout << "exceed max timeout" << std::endl;
				set_state(have_host, host, DeepFaceTrack_State::DnnError);
				return -1;
			}
			int command = host().command;
			host().handshake = host().handshake + 1;
			host.unlock();
			switch (command)
			{
			case FTNoIR_Tracker_Command::FT_CM_START:
				if (is_stoped)
				{
					is_stoped = false;
					set_state(have_host, host, DeepFaceTrack_State::Working);
				}
				break;
			case FTNoIR_Tracker_Command::FT_CM_STOP:
				if (!is_stoped)
				{
					is_stoped = true;
					set_state(have_host, host, DeepFaceTrack_State::Stopped);
				}
				break;
			case FTNoIR_Tracker_Command::FT_CM_EXIT:
				if (!is_stoped)
				{
					is_stoped = true;
					set_state(have_host, host, DeepFaceTrack_State::Stopped);
					reco.close();
				}
				break;
			}
		}

		if (is_stoped)
			continue;

		if (is_processed)
		{
			data.lock();
			x = data().x;
			y = data().y;
			z = data().z;
			yaw = data().yaw;
			pitch = data().pitch;
			roll = data().roll;
			if (!cams.getImage((unsigned char*)buffer.ptr()))
			{
				std::cout << "failed to get image from camera" << std::endl;
				data.unlock();
				return -1;
			}
			data().captured = true;
			data().processed = false;
			data.unlock();

			if (have_host)
			{
				host.lock();
				if (host().handshake >= MAX_TIMEOUT)
				{
					host.unlock();
					std::cout << "exceed max timeout" << std::endl;
					set_state(have_host, host, DeepFaceTrack_State::DnnError);
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

			//std::cout << "x: " << x << " y: " << y << " z: " << z << " yaw: " << yaw << " pitch: " << pitch << " roll: " << roll << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	}

	return 0;
}

