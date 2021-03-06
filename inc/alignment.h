#ifndef ALIGNMENT_
#define ALIGNMENT_

#include <vector>
#include <tuple>

#include <opencv2/opencv.hpp>

std::vector<std::tuple<cv::Mat, double>> alignment(const std::vector<std::tuple<cv::Mat, double>>&, bool);

#endif
