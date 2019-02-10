#include <chrono>
#include <thread>
#include <Windows.h>
#include <direct.h>

#include "opencv2\opencv.hpp"

#include "../pytorch_test/face_tracker.h"
#include "../pytorch_test/memory_map_data.h"
#include "../opentrack-tracker-deepface/dft_image_data.h"

#define MAX_TIMEOUT 100
#define AWAIT_TIME 1

int main()
{
	//SetEnvironmentVariableA("PATH", "%PATH%;./deepfacetrack");
	_chdir(".\\deepfacetrack");

	MemoryMapBuffer buffer;
	MemoryMapData<DeepFaceTrackImageData> data;
	if (!data.create(DFT_IMAGE, DFT_IMAGE_MUTEX, true))
	{
		std::cout << "failed to open recognition mapping" << std::endl;
		//return -1;
	}

	std::string bufferName;
	cv::Mat image;
	int size, width, height;
	bool dbg_show;
	float x, y, z;
	float yaw, pitch, roll;
	bool win_show = false;
	int win_x = -1, win_y = -1, win_w = -1, win_h = -1;

	FaceTracker ft;
	ft.start();

	while (true)
	{
		bool haveNewImg = false;
		bool is_stopped = false;

		data.lock();
		if (data().handshake >= MAX_TIMEOUT)
		{
			data.unlock();
			std::cout << "exceed max timeout" << std::endl;
			return -1;
		}
		dbg_show = data().dbg_show;
		if (win_x != data().win_x || win_y != data().win_y)
		{
			win_x = data().win_x;
			win_y = data().win_y;
			if (win_x >= 0 && win_y >= 0)
			{
				cv::moveWindow(FT_WIN_NAME, win_x, win_y);
			}
		}
		if (win_w != data().win_w || win_h != data().win_h)
		{
			win_w = data().win_w;
			win_h = data().win_h;
			if (win_w >= 50 && win_h >= 50)
			{
				cv::resizeWindow(FT_WIN_NAME, cv::Size(win_w, win_h));
			}
		}

		if (win_show != dbg_show)
		{
			win_show = dbg_show;
			if (win_show)
			{
				cv::namedWindow(FT_WIN_NAME, cv::WINDOW_NORMAL);
				//HWND Handle = FindWindowA(0, FT_WIN_NAME);
				//LONG lStyle = GetWindowLong(Handle, GWL_STYLE);
				//lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
				//SetWindowLong(Handle, GWL_STYLE, lStyle);
				//LONG lExStyle = GetWindowLong(Handle, GWL_EXSTYLE);
				//lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
				//SetWindowLong(Handle, GWL_EXSTYLE, lExStyle);
			}
			else
			{
				cv::destroyWindow(FT_WIN_NAME);
			}
		}
		data().handshake = data().handshake + 1;

		if (data().captured)
		{
			size = data().size;
			width = data().width;
			height = data().height;
			if (bufferName.size() == 0)
			{
				bufferName = data().name;
				if (!buffer.create(bufferName, size, true))
				{
					std::cout << "failed to open buffer mapping" << std::endl;
				}
			}
			image = cv::Mat(height, width, CV_8UC3, (unsigned char*)buffer.ptr());
			haveNewImg = true;
		}
		is_stopped = data().is_stopped;
		data.unlock();

		if (haveNewImg)
		{
			ft.setIsEnabled(!is_stopped);
			if (!ft.processImage(image, dbg_show))
			{
				std::cout << "requested quit, finalizing..." << std::endl;
				return 0;
			}
			ft.getRotations(yaw, pitch, roll);
			ft.getTranslations(x, y, z);

			data.lock();
			data().x = x;
			data().y = y;
			data().z = z;
			data().yaw = yaw;
			data().pitch = pitch;
			data().roll = roll;
			data().captured = false;
			data().processed = true;
			data.unlock();
			haveNewImg = false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	}

    return 0;
}

