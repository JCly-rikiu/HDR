#ifndef ALIGNMENT_
#define ALIGNMENT_

#include <vector>
#include <tuple>

#include <opencv2/opencv.hpp>

std::vector<std::tuple<cv::Mat, double>> alignment(std::vector<std::tuple<cv::Mat, double>>&);

#endif
