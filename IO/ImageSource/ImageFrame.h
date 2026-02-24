#pragma once
#include <opencv2/opencv.hpp>
#include <chrono>

struct ImageFrame
{
    cv::Mat image;
    std::chrono::system_clock::time_point timestamp;
    uint64_t frameId = 0;
};