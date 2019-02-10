#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <math.h>
#include <chrono>

#include "opencv2\core.hpp"
#include "opencv2\imgproc.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\dnn.hpp"

std::chrono::time_point<std::chrono::steady_clock> time_start();
float time_elapsed(std::chrono::time_point<std::chrono::steady_clock> start);
void enhance(cv::Mat& img);

#define FT_WIN_NAME "DeepFaceTracker"

class FaceTracker
{
public:
	FaceTracker();
	~FaceTracker();

	virtual void start();
	virtual void stop();
	virtual bool processImage(cv::Mat& img, bool dbgShow = false);
	void getRotations(float& yaw, float& pitch, float& roll);
	void getTranslations(float& x, float& y, float& z);
	void setIsEnabled(bool val);

protected:
	cv::dnn::Net _net_detector;
	cv::dnn::Net _net_pose;
	cv::Mat _img_captured;
	cv::Mat _img_resized;

	std::vector<float> idx;
	std::vector< float > _fps;
	std::map< std::string, float > _timers;

	bool _is_enabled;

	float _target_fps = 25.0f;
	int _post_frame_wait_time = 1;
	int frameNum = 0;
	float yaw2 = 0.0f;
	float pitch2 = 0.0f;
	float roll2 = 0.0f;
	float px = 0.0f;
	float py = 0.0f;
	float fsize = 0.0f;
	float avr_fps = 0.0f;

	void _load_face_detector_net();

	void _load_head_pose_net();

	void _debug_draw_face(float conf, int left, int top, int right, int bottom, cv::Mat& frame);

	void _handle_eptr(std::exception_ptr eptr);

	void _debug_draw_axis(float yaw, float pitch, float roll, cv::Mat& img, float tdx = 0.0f, float tdy = 0.0f, float size = 100);

	float _avr_fps(std::vector<float>& fps);

	void _debug_frame_report(int frame_num, float yaw, float pitch, float roll, std::vector<float>& fps, std::map< std::string, float >& timers);

	virtual void _log(std::string message);
};

