#ifndef DATA
#define DATA

#include <string>
#include <tuple>
#include <vector>

#include <opencv2/opencv.hpp>

std::vector<std::tuple<cv::Mat, double>> load_images(std::string&&);

#endif
