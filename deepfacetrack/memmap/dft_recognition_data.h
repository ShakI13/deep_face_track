// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#pragma once

#define DFT_IMAGE "DeepFaceTrackImageData"
#define DFT_IMAGE_MUTEX "DeepFaceTrackImageDataMutex"
// maximum number of frame to wait for handshake counter reset
#define MAX_TIMEOUT 100
// minimum time to sleep between frame processing iterations
#define AWAIT_TIME 1
// default working directory 
#define WORK_DIR "./deepfacetrack"
// recognition app name
#define RECO_APP "./deep_face_track_recognition.exe"

#pragma pack(push, 2)
struct DeepFaceTrackImageData
{
	int handshake;
	int size; // frame buffer size in bytes
	int width; // frame buffer width 
	int height; // frame buffer height
	bool captured; // has new frame been captured
	bool processed; // is current frame processed
	bool dbg_show; // are debug frame and info shown
	bool is_stopped; // is tracking stopped
	bool is_found; // is face found on last frame
	bool has_host; // has host started a program
	char name[256]; // name of frame buffer shared memory map
	int win_x; // preview window left coordinate
	int win_y; // preview window top coordinate
	int win_w; // preview window right coordinate
	int win_h; // preview window bottom coordinate
	float x; // face center X (left-right) position
	float y; // face center Y (top-bottom) position
	float z; // face center Z (closer-further) position
	float yaw; // face yaw
	float pitch; // face pitch
	float roll; // face roll
};
#pragma pack(pop)