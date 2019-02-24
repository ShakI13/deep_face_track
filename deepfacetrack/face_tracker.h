// ----------------------------------------------------------------------------
// The MIT License
// Copyright (c) 2019 Stanislav Khain <stas.khain@gmail.com>
// ----------------------------------------------------------------------------
#pragma once

#include <memory>
#include <chrono>

#include "opencv2\core.hpp"
#include "opencv2\dnn.hpp"

std::chrono::time_point<std::chrono::steady_clock> time_start();
float time_elapsed(std::chrono::time_point<std::chrono::steady_clock> start);
void enhance(cv::Mat& img);

#define FT_WIN_NAME "DeepFaceTracker"

// Detects face in the image and calculates head pose angles
class FaceTracker
{
public:
	FaceTracker();
	~FaceTracker();

	void initialize();

	void finalize();

	/**
	 * \brief Tries to detect face on the image and head pose angles
	 * \param img frame to work with
	 * \param dbg_show show preview image and debug info
	 */
	void process_image(cv::Mat& img, bool dbg_show = false);

	/**
	 * \brief Returns head rotation angles found on last frame
	 * \param yaw angle in degrees
	 * \param pitch angle in degrees
	 * \param roll angle in degrees
	 */
	void get_rotations(float& yaw, float& pitch, float& roll);

	/**
	 * \brief Returns head position found on last frame
	 * \param x normalized onto image bigger side, relative to image center, from -1 to 1
	 * \param y normalized onto image bigger side, relative to image center, from -1 to 1 
	 * \param z relation between bigger face rect side and image smaller side, from 0 to 1
	 */
	void get_translations(float& x, float& y, float& z);

	/**
	 * \brief Enable/disable head tracking
	 * \param val true - enable, false (default) - disable
	 */
	void set_is_enabled(bool val);

	/**
	 * \brief Set preview window size for drawing scale calculation
	 * \param win_w width of the window
	 * \param win_h height of the window
	 */
	void set_display_size(int win_w, int win_h);

	/**
	 * \brief Is head tracked on last frame
	 * \return true if tracked
	 */
	bool is_found() { return _is_found; }

protected:
	cv::dnn::Net _net_detector;
	cv::dnn::Net _net_pose;
	cv::Mat _img_captured;
	cv::Mat _img_resized;

	std::vector<float> _idx;
	std::vector< float > _fps;
	std::map< std::string, float > _timers;

	bool _is_enabled = false;
	bool _is_found = false;

	int _frame_num = 0;
	float _yaw = 0.0f;
	float _pitch = 0.0f;
	float _roll = 0.0f;
	float _face_cx = 0.0f;
	float _face_cy = 0.0f;
	float _face_size = 0.0f;
	float _face_width = 0.0f;
	float _face_height = 0.0f;

	float _avr_fps = 0.0f;
	float _target_fps = 25.0f;
	int _post_frame_wait_time = 1;

	float _win_w = 0.0f;
	float _win_h = 0.0f;

	void _load_face_detector_net();

	void _load_head_pose_net();

	float _avr_fps_from_vector(std::vector<float>& fps);

	void _debug_draw_face(float conf, int left, int top, int right, int bottom, cv::Mat& frame);

	void _debug_draw_axis(float yaw, float pitch, float roll, cv::Mat& img, float tdx = 0.0f, float tdy = 0.0f, float size = 100);

	void _debug_frame_report(int frame_num, float yaw, float pitch, float roll, std::vector<float>& fps, std::map< std::string, float >& timers);

	void _handle_eptr(std::exception_ptr eptr);
};

