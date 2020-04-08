#include <iostream>

#include <opencv2/opencv.hpp>

#include "contrast.h"

void normalize(cv::Mat& image, double gamma_value) {
  auto rows = image.rows;
  auto cols = image.cols;

  double min_v = std::numeric_limits<double>::infinity();
  double max_v = -std::numeric_limits<double>::infinity();
  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      for (int channel = 0; channel != 3; ++channel) {
        min_v = std::min(min_v, static_cast<double>(image.at<cv::Vec3d>(i, j)[channel]));
        max_v = std::max(max_v, static_cast<double>(image.at<cv::Vec3d>(i, j)[channel]));
      }

  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      for (int channel = 0; channel != 3; ++channel) {
        if (max_v - min_v > DBL_EPSILON)
          image.at<cv::Vec3d>(i, j)[channel] = (image.at<cv::Vec3d>(i, j)[channel] - min_v) / (max_v - min_v);
        image.at<cv::Vec3d>(i, j)[channel] = std::pow(image.at<cv::Vec3d>(i, j)[channel], 1 / gamma_value);
      }
}

cv::Mat contrast(const cv::Mat& radiance_map) {
  std::cout << "\tcontrast" << std::endl;

  cv::Mat tonemap = radiance_map;
  normalize(tonemap, 1.0);

  auto rows = tonemap.rows;
  auto cols = tonemap.cols;

  double log_lum_min = std::numeric_limits<double>::infinity();
  double log_lum_max = -std::numeric_limits<double>::infinity();
  double log_lum_mean = 0.0;

  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      auto value = tonemap.at<cv::Vec3d>(i, j);
      auto lum = 0.114 * value[0] + 0.587 * value[1] + 0.299 * value[2];
      auto log_lum = std::log(0.000001 + lum);
      log_lum_min = std::min(log_lum_min, log_lum);
      log_lum_max = std::max(log_lum_max, log_lum);
      log_lum_mean += log_lum;
    }
  }
  log_lum_mean /= (rows * cols);
  double lum_mean = std::exp(log_lum_mean);

  double contrast = 0.3 + 0.7 * std::pow((log_lum_max - log_lum_mean) / (log_lum_max - log_lum_min), 1.4);

  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      for (int channel = 0; channel != 3; channel++) {
        auto value = tonemap.at<cv::Vec3d>(i, j)[channel];
        tonemap.at<cv::Vec3d>(i, j)[channel] = value / (value + std::pow(lum_mean, contrast));
      }

  normalize(tonemap, 1.5);

  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      for (int channel = 0; channel != 3; channel++)
        tonemap.at<cv::Vec3d>(i, j)[channel] *= 255.0;

  return tonemap;
}
