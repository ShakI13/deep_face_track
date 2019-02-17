#include <chrono>
#include <thread>
#include <Windows.h>
#include <direct.h>
#include <tlhelp32.h>
#include <WinUser.h>

#include "opencv2\opencv.hpp"

#include "face_tracker.h"
#include "memmap/dft_image_data.h"
#include "memmap/memory_map_data.h"

#define MAX_TIMEOUT 100
#define AWAIT_TIME 1

// ref: https://stackoverflow.com/questions/11711417/get-hwnd-by-process-id-c
HWND ParentHandle = NULL;
BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == lParam)
	{
		ParentHandle = hwnd;
		return FALSE;
	}
	return TRUE;
}

void _validate_embed(HWND Handle, int x, int y, int w, int h, bool show)
{
	if (ParentHandle == NULL)
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				if (_wcsicmp(entry.szExeFile, L"FaceTrackNoIR.exe") == 0)
				{
					HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
					DWORD processId = GetProcessId(hProcess);
					EnumWindows(EnumWindowsProcMy, processId);
					CloseHandle(hProcess);
				}
			}
		}

		CloseHandle(snapshot);
	}

	// ref: https://stackoverflow.com/questions/7611103/embedding-window-into-another-process/7612436
	if (ParentHandle == NULL)
		return;

	if (!IsWindow(ParentHandle))
		return;

	RECT rect;
	WINDOWPLACEMENT place;
	GetWindowPlacement(ParentHandle, &place);
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

	//HWND PrevParent = SetParent(Handle, ParentHandle);
	///// Attach container app input thread to the running app input thread, so that
	/////  the running app receives user input.
	//DWORD threadId = GetWindowThreadProcessId(ParentHandle, 0);
	//DWORD childId = GetWindowThreadProcessId(Handle, 0);
	////FAppThreadID: = GetWindowThreadProcessId(WindowHandle, nil)
	//AttachThreadInput(childId, threadId, TRUE);
	// //AttachThreadInput(GetCurrentThreadId, FAppThreadID, True);

	//// /// Changing parent of the running app to our provided container control
	//// Windows.SetParent(WindowHandle, Container.Handle);
	//SendMessageA(ParentHandle, WM_UPDATEUISTATE, UIS_INITIALIZE, 0);
	//UpdateWindow(Handle);
	////SendMessage(Container.Handle, WM_UPDATEUISTATE, UIS_INITIALIZE, 0);
	////UpdateWindow(WindowHandle);

	///// This prevents the parent control to redraw on the area of its child windows (the running app)
	//SetWindowLong(ParentHandle, GWL_STYLE, GetWindowLong(ParentHandle, GWL_STYLE) | WS_CLIPCHILDREN);
	///// Make the running app to fill all the client area of the container
	////SetWindowPos(WindowHandle, 0, 0, 0, Container.ClientWidth, Container.ClientHeight, SWP_NOZORDER);
	//ShowWindow(Handle, SW_SHOW);
	//SetForegroundWindow(Handle);
}

void _validate_window(int x, int y, int w, int h, bool show, bool has_host)
{
	if (!has_host)
		return;
		
	HWND Handle = FindWindowA(0, FT_WIN_NAME);
	LONG lStyle = GetWindowLong(Handle, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_BORDER | WS_OVERLAPPED | WS_CHILD); // WS_BORDER WS_OVERLAPPED
	SetWindowLong(Handle, GWL_STYLE, lStyle);
	LONG lExStyle = GetWindowLong(Handle, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	lExStyle &= (WS_EX_TOPMOST);
	SetWindowLong(Handle, GWL_EXSTYLE, lExStyle);

	_validate_embed(Handle, x, y, w, h, show);
}

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
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	bool win_show = false;
	bool has_host = false;
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
		has_host = data().has_host;
		if (win_x != data().win_x || win_y != data().win_y)
		{
			if (data().win_x >= 0 && data().win_y >= 0)
			{
				win_x = data().win_x;
				win_y = data().win_y;
				//cv::moveWindow(FT_WIN_NAME, win_x, win_y);
			}
		}
		if (win_w != data().win_w || win_h != data().win_h)
		{
			if (data().win_w >= 50 && data().win_h >= 50)
			{
				win_w = data().win_w;
				win_h = data().win_h;
				//cv::resizeWindow(FT_WIN_NAME, cv::Size(win_w, win_h));
			}
		}

		//if (win_x >= 0 && win_y >= 0)
		{
			cv::namedWindow(FT_WIN_NAME, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
			_validate_window(win_x, win_y, win_w, win_h, dbg_show, has_host);
		}
		/*else
		{

		}*/

		//if (win_show != dbg_show)
		//{
		//	win_show = dbg_show;
		//	if (win_show)
		//	{
		//		cv::namedWindow(FT_WIN_NAME, cv::WINDOW_NORMAL);
		//		_validate_window();
		//		//cv::destroyWindow(FT_WIN_NAME);
		//	}
		//	else
		//	{
		//		cv::destroyWindow(FT_WIN_NAME);
		//	}
		//}
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
			ft.setDisplaySize(win_w, win_h);
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
			data().is_found = ft.isFound();
			data.unlock();
			haveNewImg = false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	}

    return 0;
}

