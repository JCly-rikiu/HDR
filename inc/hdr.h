#ifndef HDR_
#define HDR_

#include <vector>
#include <tuple>

#include <opencv2/opencv.hpp>

cv::Mat hdr(const std::vector<std::tuple<cv::Mat, double>>&, const bool);

#endif
