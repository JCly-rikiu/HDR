#ifndef HDR_
#define HDR_

#include <vector>
#include <tuple>

#include <opencv2/opencv.hpp>

cv::Mat hdr(std::vector<std::tuple<cv::Mat, double>>&);

#endif