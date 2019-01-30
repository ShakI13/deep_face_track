#pragma once

#define DFT_RECOGNITION "DeepFaceTrackRecognition"
#define DFT_RECOGNITION_MUTEX "DeepFaceTrackRecognitionMutex"

// Contains recognition result of last captured image from the camera
#pragma pack(push, 2)
struct DeepFaceTrackRecognition
{
	float x; // face center X (left-right) position
	float y; // face center Y (top-bottom) position
	float z; // face center Z (closer-further) position
	float yaw; // face angle
	float pitch; // face angle
	float roll; // face angle
};
#pragma pack(pop)