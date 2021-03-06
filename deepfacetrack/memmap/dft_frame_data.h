// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#pragma once

static const char* FT_MM_DATA = "FT_DFT_SharedMem";
static const char* FT_MUTEX = "FT_DFT_Mutex";
static const char* FT_WIN_NAME = "DeepFaceTracker"; // camera preview window name

// Contains recognition result of last captured image from the camera
#pragma pack(push, 2)
struct DeepFaceTrackMemMap
{
	int handshake;
	int command; // Command from FaceTrackNoIR
	int state; // DeepFaceTrack internal state
	float x; // face center X (left-right) position
	float y; // face center Y (top-bottom) position
	float z; // face center Z (closer-further) position
	float yaw; // face yaw angle
	float pitch; // face pitch angle
	float roll; // face roll angle
	int win_x; // window position
	int win_y; // window position
	int win_w; // window size
	int win_h; // window size
	int frame_w; // camera image width
	int frame_h; // camera image height
};
#pragma pack(pop)

enum FTNoIR_Tracker_Command {
	FT_CM_START = 10,
	FT_CM_STOP = 20,
	FT_CM_SHOW_CAM = 30,
	FT_CM_SET_PAR_FILTER = 50,
	FT_CM_EXIT = 100,
	FT_CM_SHOWWINDOW = 110,
	FT_CM_HIDEWINDOW = 111
};

enum DeepFaceTrack_State {
	Normal = 0,
	Initializing = 1,
	CameraError = 2,
	DnnError = 3,
	Working = 4,
	Stopped = 5
};