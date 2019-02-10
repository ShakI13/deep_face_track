#include "face_tracker.h"
#include "soft_max.h"
#include "opencv2/core/ocl.hpp"
#include <chrono>

//float time_start()
//{
//	return cv::getTickCount();
//}
//
//float time_elapsed(float start)
//{
//	return (cv::getTickCount() - start) / cv::getTickFrequency();
//}

std::chrono::time_point<std::chrono::steady_clock> time_start()
{
	return std::chrono::high_resolution_clock::now();
}

float time_elapsed(std::chrono::time_point<std::chrono::steady_clock> start)
{
	using milli = std::chrono::milliseconds;
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<milli>(now - start).count() / 1000.0f;
}

void enhance(cv::Mat& img)
{
	auto h = img.rows;
	auto w = img.cols;
	unsigned char min_r = 256, min_g = 256, min_b = 256;
	unsigned char max_r = 0, max_g = 0, max_b = 0;

	for (int i = 0; i < h; i++)
	{
		unsigned char* src = img.data + i * img.step;
		for (int j = 0; j < w; j++)
		{
			auto srcP = src + 3 * j;
			if (min_r > srcP[0])
				min_r = srcP[0];
			if (max_r < srcP[0])
				max_r = srcP[0];

			if (min_g > srcP[1])
				min_g = srcP[1];
			if (max_g < srcP[1])
				max_g = srcP[1];

			if (min_b > srcP[2])
				min_b = srcP[2];
			if (max_b < srcP[2])
				max_b = srcP[2];
		}
	}

	for (int i = 0; i < h; i++)
	{
		unsigned char* src = img.data + i * img.step;
		for (int j = 0; j < w; j++)
		{
			auto srcP = src + 3 * j;

			srcP[0] = (unsigned char)(255.0f * (srcP[0] - min_r) / (float)(max_r - min_r));
			srcP[1] = (unsigned char)(255.0f * (srcP[1] - min_g) / (float)(max_g - min_g));
			srcP[2] = (unsigned char)(255.0f * (srcP[2] - min_b) / (float)(max_b - min_b));
		}
	}
}

FaceTracker::FaceTracker()
{
	_target_fps = 25.0f;
	_post_frame_wait_time = 1;
	frameNum = 0;
	yaw2 = 0.0f;
	pitch2 = 0.0f;
	roll2 = 0.0f;
}

FaceTracker::~FaceTracker()
{
}

void FaceTracker::start()
{
	_timers["load_face_det"] = 0.0f;
	_timers["load_head_pose"] = 0.0f;
	_timers["acquire_image"] = 0.0f;
	_timers["detect"] = 0.0f;
	_timers["head_pose"] = 0.0f;
	_timers["frame_time"] = 0.0f;

	auto load_face_det = time_start();
	_load_face_detector_net();
	_timers["load_face_det"] = time_elapsed(load_face_det);

	auto load_head_pose = time_start();
	_load_head_pose_net();
	_timers["load_head_pose"] = time_elapsed(load_head_pose);

	for (int i = 0; i < 66; i++)
		idx.push_back(i);

	std::cout << "context devices: " << std::endl;
	cv::ocl::Context ctx;
	ctx = cv::ocl::Context::getDefault();
	//ctx.create(cv::ocl::device::type_all);

	for (int i = 0; i < ctx.ndevices(); i++)
	{
		cv::ocl::Device device = ctx.device(i);
		std::stringstream ss;
		ss << "\n device: " << device.name();
		ss << "\n version: " << device.version();
		ss << "\n vendor: " << device.vendorName();
		std::cout << ss.str() << std::endl;
	}
}

void FaceTracker::stop()
{
	cv::destroyWindow(FT_WIN_NAME);
	idx.clear();
}

bool FaceTracker::processImage(cv::Mat& img, bool dbgShow)
{
	_timers["acquire_image"] = 0.0f;
	_timers["detect"] = 0.0f;
	_timers["head_pose"] = 0.0f;
	_timers["frame_time"] = 0.0f;

	std::exception_ptr eptr;
	int keyCode = -1;
	int keyNumPlus = 45;
	int keyNumMinus = 43;

	auto frame_time = time_start();
	auto acquire_image = time_start();
	_img_captured = img;
	enhance(_img_captured);
	cv::resize(_img_captured, _img_resized, cv::Size(300, 300));

	if (_is_enabled)
	{
		auto blob = cv::dnn::blobFromImage(_img_resized, 1.0, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0));
		_timers["acquire_image"] = time_elapsed(acquire_image);

		auto det_time = time_start();
		std::vector<cv::String> outNames = _net_detector.getUnconnectedOutLayersNames();
		_net_detector.setInput(blob);
		std::vector<cv::Mat> outs;
		_net_detector.forward(outs, outNames);
		int h = _img_captured.rows;
		int w = _img_captured.cols;
		_timers["detect"] = time_elapsed(det_time);

		auto head_time = time_start();
		float* data = (float*)outs[0].data;
		for (size_t i = 0; i < outs[0].total(); i += 7)
		{
			float confidence = data[i + 2];
			if (confidence <= 0.25f)
				continue;

			int left = (int)(data[i + 3] * w);
			int top = (int)(data[i + 4] * h);
			int right = (int)(data[i + 5] * w);
			int bottom = (int)(data[i + 6] * h);
			if (dbgShow)
				_debug_draw_face(avr_fps, left, top, right, bottom, _img_captured);

			if (left < 0)
				left = 0;
			if (top < 0)
				top = 0;
			if (left >= w)
				left = w - 1;
			if (top >= h)
				top = h - 1;
			if (right < 0)
				right = 0;
			if (bottom < 0)
				bottom = 0;
			if (right >= w)
				right = w - 1;
			if (bottom >= h)
				bottom = h - 1;
			int width = right - left + 1;
			int height = bottom - top + 1;

			if (width < 10 || height < 10)
				continue;

			px = left + width / 2.0f;
			py = top + height / 2.0f;
			fsize = width * height;

			cv::Mat img_face = _img_captured(cv::Rect(left, top, right - left, bottom - top));
			try {
				img_face.convertTo(img_face, CV_32F, 1.0 / 255);
				cv::resize(img_face, img_face, cv::Size(224, 224));
				//cv::cvtColor(img_face, img_face, cv::COLOR_RGB2BGR);

				std::vector< cv::Mat > img_channels(3);
				cv::split(img_face, img_channels);
				img_channels[0] = img_channels[0] - 0.485f;
				img_channels[1] = img_channels[1] - 0.456f;
				img_channels[2] = img_channels[2] - 0.406f;
				img_channels[0] = img_channels[0] / 0.229f;
				img_channels[1] = img_channels[1] / 0.224f;
				img_channels[2] = img_channels[2] / 0.225f;
				cv::merge(img_channels, img_face);

				auto blob_face = cv::dnn::blobFromImage(img_face);

				//// test python image and result
				//auto img_data = load_test_image("C:\\dev\\deep_face_track\\image.txt");
				//auto res_data = load_test_image("C:\\dev\\deep_face_track\\result.txt");
				//// copy image
				//float diff = 0.0f;
				//for (int i = 0; i < img_data.size(); i++)
				//{
				//	float v1 = ((float*)blob_face.data)[i];
				//	float v2 = img_data[i];
				//	diff += abs(v1 - v2);
				//	((float*)blob_face.data)[i] = img_data[i];
				//}

				_net_pose.setInput(blob_face);
				//outNames = { "509", "510", "511" };
				outNames = { "139", "140", "141" };
				outs.clear();
				_net_pose.forward(outs, outNames);

				std::vector<float> yaw, pitch, roll;
				yaw.assign((float*)outs[0].datastart, (float*)(outs[0].datastart) + 66);
				pitch.assign((float*)outs[1].datastart, (float*)(outs[1].datastart) + 66);
				roll.assign((float*)outs[2].datastart, (float*)(outs[2].datastart) + 66);

				//// concatenate results
				//std::vector<float> all_res;
				//all_res.insert(all_res.end(), yaw.begin(), yaw.end());
				//all_res.insert(all_res.end(), pitch.begin(), pitch.end());
				//all_res.insert(all_res.end(), roll.begin(), roll.end());
				//diff = 0.0f;
				//for (int i = 0; i < all_res.size(); i++)
				//{
				//	float v1 = all_res[i];
				//	float v2 = res_data[i];
				//	diff += abs(v1 - v2);
				//}

				softmax(yaw);
				softmax(pitch);
				softmax(roll);

				float yaw_sum = 0.0f, pitch_sum = 0.0f, roll_sum = 0.0f;
				for (int i = 0; i < 66; i++)
				{
					yaw_sum += yaw[i] * idx[i];
					pitch_sum += pitch[i] * idx[i];
					roll_sum += roll[i] * idx[i];
				}

				yaw2 = yaw_sum * 3.0f - 99.0f;
				pitch2 = pitch_sum * 3.0f - 99.0f;
				roll2 = roll_sum * 3.0f - 99.0f;
				if (dbgShow)
					_debug_draw_axis(yaw2, pitch2, roll2, _img_captured, (left + right) / 2, (top + bottom) / 2, (bottom - top) / 2);
			}
			catch (std::exception & err) {
				_log(err.what());
			}
			catch (...)
			{
				eptr = std::current_exception();
			}
			_handle_eptr(eptr);
			break;
		}
		_timers["head_pose"] = time_elapsed(head_time);
	}

	if (dbgShow)
	{
		//cv::namedWindow(FT_WIN_NAME, cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
		cv::imshow(FT_WIN_NAME, _img_captured);
		keyCode = cv::waitKey(_post_frame_wait_time);
		keyCode = -1;
	}

	//std::cout << "keyCode: " << keyCode << std::endl;
/*
	if (keyCode == 45)
	{
		std::cout << "UP" << std::endl;
		auto contrast = cap.device().get(cv::VideoCaptureProperties::CAP_PROP_CONTRAST);
		contrast *= 1.3f;
		cap.device().set(cv::VideoCaptureProperties::CAP_PROP_CONTRAST, contrast);
		contrast = cap.device().get(cv::VideoCaptureProperties::CAP_PROP_BRIGHTNESS);
		contrast *= 1.3f;
		cap.device().set(cv::VideoCaptureProperties::CAP_PROP_BRIGHTNESS, contrast);
		contrast = cap.device().get(cv::VideoCaptureProperties::CAP_PROP_SATURATION);
		contrast *= 1.3f;
		cap.device().set(cv::VideoCaptureProperties::CAP_PROP_SATURATION, contrast);
	}
	else if (keyCode == 43)
	{
		std::cout << "DOWN" << std::endl;
		auto contrast = cap.device().get(cv::VideoCaptureProperties::CAP_PROP_CONTRAST);
		contrast *= 0.7f;
		cap.device().set(cv::VideoCaptureProperties::CAP_PROP_CONTRAST, contrast);
		contrast = cap.device().get(cv::VideoCaptureProperties::CAP_PROP_BRIGHTNESS);
		contrast *= 0.7f;
		cap.device().set(cv::VideoCaptureProperties::CAP_PROP_BRIGHTNESS, contrast);
		contrast = cap.device().get(cv::VideoCaptureProperties::CAP_PROP_SATURATION);
		contrast *= 0.7f;
		cap.device().set(cv::VideoCaptureProperties::CAP_PROP_SATURATION, contrast);
	}
*/
	_timers["frame_time"] = time_elapsed(frame_time);
	if (_fps.size() < (frameNum % 10 + 1))
	{
		_fps.push_back(0.0);
	}
	_fps[frameNum % 10] = 1.0 / _timers["frame_time"];
	if (dbgShow)
		_debug_frame_report(frameNum, yaw2, pitch2, roll2, _fps, _timers);
	//std::cout << "Z: " << 100.0f * fsize / (float)(_img_captured.cols * _img_captured.rows);
	frameNum++;

	avr_fps = _avr_fps(_fps);
	//if (avr_fps < _target_fps && _post_frame_wait_time > 1)
	//{
	//	if (abs(avr_fps - _target_fps) > 5)
	//		_post_frame_wait_time -= 5;
	//	else
	//		_post_frame_wait_time--;
	//}
	//if (avr_fps > _target_fps)
	//{
	//	if (abs(avr_fps - _target_fps) > 5)
	//		_post_frame_wait_time += 5;
	//	else
	//		_post_frame_wait_time++;
	//}

	return keyCode != 27;
}

void FaceTracker::getRotations(float & yaw, float & pitch, float & roll)
{	
	yaw = yaw2;
	pitch = pitch2;
	roll = roll2;
}

void FaceTracker::getTranslations(float & x, float & y, float & z)
{
	if (_img_captured.empty())
		x = y = z = 0.0f;
	else
	{
		float h = _img_captured.rows;
		float w = _img_captured.cols;
		float imSize = w*h;

		x = px;
		y = py;
		z = fsize / imSize;
	}
}

void FaceTracker::setIsEnabled(bool val)
{
	_is_enabled = val;
}

void FaceTracker::_load_face_detector_net()
{
	if (_net_detector.empty())
	{
		std::string model_deploy_path = "./models/deploy.prototxt.txt";
		std::string model_proto_path = "./models/res10_300x300_ssd_iter_140000.caffemodel";
		_log("Trying to load face detection DNN...");
		_net_detector = cv::dnn::readNetFromCaffe(
			model_deploy_path,
			model_proto_path
		);
		_net_detector.setPreferableBackend(cv::dnn::Backend::DNN_BACKEND_OPENCV);
		_net_detector.setPreferableTarget(cv::dnn::Target::DNN_TARGET_OPENCL);
		//_net_detector.setPreferableTarget(cv::dnn::Target::DNN_TARGET_CPU);
		//std::cout << "done.\n";
	}
}

void FaceTracker::_load_head_pose_net()
{
	if (_net_pose.empty())
	{
		std::string model_path = ".\\models\\head_pose.onnx";
		_log("Trying to load head pose DNN...");
		_net_pose = cv::dnn::readNetFromONNX(model_path);
		_net_pose.setPreferableBackend(cv::dnn::Backend::DNN_BACKEND_OPENCV);
		_net_pose.setPreferableTarget(cv::dnn::Target::DNN_TARGET_OPENCL);
		//_net_pose.setPreferableTarget(cv::dnn::Target::DNN_TARGET_CPU);
		//std::cout << "done.\n";
	}
}

#define my_max(a, b) (((a) > (b)) ? (a) : (b))

void FaceTracker::_debug_draw_face(float conf, int left, int top, int right, int bottom, cv::Mat & frame)
{
	cv::rectangle(frame, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(0, 255, 0));

	std::string label = cv::format("%.2f", conf);

	int baseLine;
	cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

	top = my_max(top, labelSize.height);
	rectangle(frame, cv::Point(left, top - labelSize.height),
		cv::Point(left + labelSize.width, top + baseLine), cv::Scalar::all(255), cv::FILLED);
	putText(frame, label, cv::Point(left, top), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar());
}

void FaceTracker::_handle_eptr(std::exception_ptr eptr) // passing by value is ok
{
	try {
		if (eptr) {
			std::rethrow_exception(eptr);
		}
	}
	catch (const std::exception& e) {
		std::stringstream ss;
		ss << "Caught exception:\n" << e.what();
		_log(ss.str());
	}
}

void FaceTracker::_debug_draw_axis(float yaw, float pitch, float roll, cv::Mat & img, float tdx, float tdy, float size)
{
	float pi = 3.14159265359f;
	pitch = pitch * pi / 180;
	yaw = -(yaw * pi / 180);
	roll = roll * pi / 180;

	if (tdx == 0.0f && tdy == 0.0f)
	{
		int h = img.rows;
		int w = img.cols;
		tdx = w / 2.0f;
		tdy = h / 2.0f;
	}

	// X - Axis pointing to right.drawn in red
	float x1 = size * (cos(yaw) * cos(roll)) + tdx;
	float y1 = size * (cos(pitch) * sin(roll) + cos(roll) * sin(pitch) * sin(yaw)) + tdy;

	// Y - Axis | drawn in green
	//        v
	float x2 = size * (-cos(yaw) * sin(roll)) + tdx;
	float y2 = size * (cos(pitch) * cos(roll) - sin(pitch) * sin(yaw) * sin(roll)) + tdy;

	// Z - Axis(out of the screen) drawn in blue
	float x3 = size * (sin(yaw)) + tdx;
	float y3 = size * (-cos(yaw) * sin(pitch)) + tdy;

	cv::line(img, cv::Point(tdx, tdy), cv::Point(x1, y1), cv::Scalar(0, 0, 255), 3);
	cv::line(img, cv::Point(tdx, tdy), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 3);
	cv::line(img, cv::Point(tdx, tdy), cv::Point(x3, y3), cv::Scalar(255, 0, 0), 3);
}

float FaceTracker::_avr_fps(std::vector<float>& fps)
{
	float avr_fps = 0.0;
	for (int i = 0; i < fps.size(); i++)
	{
		avr_fps += fps[i];
	}
	if (fps.size() > 0)
	{
		avr_fps /= fps.size();
	}
	return avr_fps;
}

void FaceTracker::_debug_frame_report(int frame_num, float yaw, float pitch, float roll, std::vector<float>& fps, std::map<std::string, float>& timers)
{
	std::stringstream ss;

	//ss << "frame " << frame_num;

	ss << "yaw: " << cv::format("%.6f", yaw) << ", pitch:" << cv::format("%.6f", pitch) << ", roll: " << cv::format("%.6f", roll);


	float avr_fps = 0.0;
	for (int i = 0; i < fps.size(); i++)
	{
		avr_fps += fps[i];
	}
	if (fps.size() > 0)
	{
		avr_fps /= fps.size();
	}
	ss << ", fps " << avr_fps;

	for (auto& x : timers)
	{
		ss << ", " << x.first << ": " << cv::format("%.6f", x.second) << " msec";
	}

	_log(ss.str());
}

void FaceTracker::_log(std::string message)
{
	std::cout << message << std::endl;
}
