#pragma once

#define DFT_IMAGE "DeepFaceTrackImageData"
#define DFT_IMAGE_MUTEX "DeepFaceTrackImageDataMutex"

#pragma pack(push, 2)
struct DeepFaceTrackImageData
{
	int handshake;
	int size;
	int width;
	int height;
	bool captured;
	bool processed;
	bool dbg_show;
	bool is_stopped;
	char name[256];
	int win_x;
	int win_y;
	int win_w;
	int win_h;
	float x; // face center X (left-right) position
	float y; // face center Y (top-bottom) position
	float z; // face center Z (closer-further) position
	float yaw; // face angle
	float pitch; // face angle
	float roll; // face angle
};
#pragma pack(pop)