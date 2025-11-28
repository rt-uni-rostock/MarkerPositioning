#pragma once
#include <opencv2/opencv.hpp>
#include <chrono>

struct FrameData
{
    cv::Mat frame;
    std::chrono::system_clock::time_point timestamp;
};