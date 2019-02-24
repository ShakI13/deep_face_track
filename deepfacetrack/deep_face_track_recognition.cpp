// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#include <chrono>
#include <thread>
#include <Windows.h>
#include <direct.h>
#include <tlhelp32.h>
#include <WinUser.h>

#include "opencv2\opencv.hpp"

#include "face_tracker.h"
#include "memmap/dft_recognition_data.h"
#include "memmap/memory_map_data.h"

// change window stype, position and size if started from host application
void _validate_window(int x, int y, int w, int h, bool show, bool has_host)
{
	if (!has_host)
		return;
		
	HWND Handle = FindWindowA(0, FT_WIN_NAME);
	
	LONG lStyle = GetWindowLong(Handle, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_BORDER | WS_OVERLAPPED | WS_CHILD);
	SetWindowLong(Handle, GWL_STYLE, lStyle);
	
	LONG lExStyle = GetWindowLong(Handle, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	lExStyle &= (WS_EX_TOPMOST);
	SetWindowLong(Handle, GWL_EXSTYLE, lExStyle);

	RECT rect;
	GetWindowRect(Handle, &rect);
	if (x >= 0)
		rect.left = x;
	if (y >= 0)
		rect.top = y;
	if (w >= 50)
		rect.right = w;
	if (h >= 50)
		rect.bottom = h;
	SetWindowPos(Handle, HWND_TOPMOST, rect.left, rect.top, rect.right, rect.bottom, 0);

	if (!show)
		ShowWindow(Handle, SW_SHOWMINIMIZED);
	else
		ShowWindow(Handle, SW_SHOWNOACTIVATE);
}

int main()
{
	std::string bufferName; // shared buffer name that holds the image
	cv::Mat image; // mapping to image from shared buffer
	int size, width, height; // image size and dimensions
	bool dbg_show; // showing preview image and debug info or not
	bool has_host;
	bool has_new_img;
	bool is_stopped;

	int win_x = -1, win_y = -1, win_w = -1, win_h = -1;

	float x = 0.0f, y = 0.0f, z = 0.0f;
	float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;

	FaceTracker ft; // face tracker instance
	MemoryMapBuffer buffer; // shared buffer that holds the image
	MemoryMapData<DeepFaceTrackImageData> data; // shared tracking params

	// point to directory where all dlls and models should be
	_chdir(WORK_DIR);

	// tries to open camera shared memory, if fails then works without camera
	if (!data.create(DFT_IMAGE, DFT_IMAGE_MUTEX, true))
	{
		std::cout << "failed to open recognition mapping" << std::endl;
		return -1;
	}

	// load models
	ft.initialize();

	while (true)
	{
		has_new_img = false;

		data.lock();
		if (data().handshake >= MAX_TIMEOUT)
		{
			data.unlock();
			std::cout << "exceed max timeout" << std::endl;
			return -1;
		}

		// read params from camera app
		dbg_show = data().dbg_show;
		has_host = data().has_host;
		is_stopped = data().is_stopped;
		if (win_x != data().win_x || win_y != data().win_y)
		{
			if (data().win_x >= 0 && data().win_y >= 0)
			{
				win_x = data().win_x;
				win_y = data().win_y;
			}
		}
		if (win_w != data().win_w || win_h != data().win_h)
		{
			if (data().win_w >= 50 && data().win_h >= 50)
			{
				win_w = data().win_w;
				win_h = data().win_h;
			}
		}
		data().handshake = data().handshake + 1;

		// if new image captured validate local mapping to image
		if (data().captured)
		{
			// get image dimensions
			size = data().size;
			width = data().width;
			height = data().height;

			// if shared buffer not setup, try to open it
			if (bufferName.empty())
			{
				bufferName = data().name;
				if (!buffer.create(bufferName, size, true))
				{
					std::cout << "failed to open buffer mapping" << std::endl;
					return -1;
				}
			}

			// if local mapping image properties and buffer ones are different, reinitialize local mapping
			if (image.cols != width || image.rows != height || image.datastart != (unsigned char*)buffer.ptr())
				image = cv::Mat(height, width, CV_8UC3, (unsigned char*)buffer.ptr());

			// signal that there is new image
			has_new_img = true;
		}
		data.unlock();

		// validate preview window
		cv::namedWindow(FT_WIN_NAME, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
		_validate_window(win_x, win_y, win_w, win_h, dbg_show, has_host);

		// if new frame is arrived, try to recognize head on it and update output params
		if (has_new_img)
		{
			ft.set_is_enabled(!is_stopped);
			ft.set_display_size(win_w, win_h);
			ft.process_image(image, dbg_show);
			ft.get_rotations(yaw, pitch, roll);
			ft.get_translations(x, y, z);

			data.lock();
			data().x = x;
			data().y = y;
			data().z = z;
			data().yaw = yaw;
			data().pitch = pitch;
			data().roll = roll;
			data().captured = false;
			data().processed = true;
			data().is_found = ft.is_found();
			data.unlock();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	}

    return 0;
}

