// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#pragma once

#include "videoInput.h"

/*
Gives access to connected web-cameras via VideoInput functionality.
*/
class CameraDiscovery
{
public:
	CameraDiscovery();
	~CameraDiscovery();

	/**
	 * \brief returns number of connected cameras
	 * \return number of connected cameras
	 */
	int num_cameras();

	/**
	 * \brief Selects specified camera
	 * \param deviceId id of camera from 0 to numCameras()-1
	 */
	void select_device(int deviceId);

	/**
	 * \brief Returns selected camera frame size and dimensions
	 * \param width width of the frame in pixels
	 * \param height height of the frame in pixels
	 * \param size size of the frame in bytes
	 */
	void get_image_size(int& width, int& height, int& size);

	/**
	 * \brief Returns selected camera last captured frame buffer data
	 * \param buffer pointer to allocated buffer
	 * \return true on success, false on fail
	 */
	bool get_image(unsigned char* buffer);

	/**
	 * \brief Checks if selected camera has new frame
	 * \return if selected camera has new frame, returns true otherwise false
	 */
	bool have_image();

protected:
	int _selected_id;
	videoInput _videoInput;

	void _enumerate_cameras();
	void _release_selected();
};