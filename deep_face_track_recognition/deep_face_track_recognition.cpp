#include <chrono>
#include <thread>

#include "opencv2\opencv.hpp"

#include "../pytorch_test/face_tracker.h"
#include "../pytorch_test/memory_map_data.h"
#include "../opentrack-tracker-deepface/dft_image_data.h"

#define MAX_TIMEOUT 100
#define AWAIT_TIME 1

int main()
{
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

	FaceTracker ft;
	ft.start();

	while (true)
	{
		bool haveNewImg = false;

		data.lock();
		if (data().handshake >= MAX_TIMEOUT)
		{
			data.unlock();
			std::cout << "exceed max timeout" << std::endl;
			return -1;
		}
		dbg_show = data().dbg_show;
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
		data.unlock();

		if (haveNewImg)
		{
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
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(AWAIT_TIME));
	}

    return 0;
}

