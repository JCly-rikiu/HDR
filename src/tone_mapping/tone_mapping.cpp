#include <numeric>

#include <opencv2/opencv.hpp>

#include "tone_mapping.h"

cv::Mat tone_mapping(cv::Mat radiance_map) {
  auto rows = radiance_map.rows;
  auto cols = radiance_map.cols;

  std::vector<double> lum(rows * cols);
  double lum_mean = 0.0;
  double Lwhite = 0.0;

  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      auto value = radiance_map.at<cv::Vec3f>(i, j);
      lum[i * cols + j] = 0.27 * value[0] + 0.67 * value[1] + 0.06 * value[2];
      Lwhite = std::max(Lwhite, lum[i * cols + j]);
      lum_mean += std::log(0.000001 + lum[i * cols + j]);
    }
  }
  std::cout << "Lwhite: " << Lwhite << std::endl;
  lum_mean = std::exp(lum_mean / lum.size());
  std::cout << "lum_mean: " << lum_mean << std::endl;

  double a = 0.18;
  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      double Lw = lum[i * cols + j];
      double L = a * Lw / lum_mean;
      double Ld = (L * (1 + L / std::pow(Lwhite, 2))) / (1 + L);
      for (int channel = 0; channel != 3; channel++) {
        radiance_map.at<cv::Vec3f>(i, j)[channel] *= Ld / Lw * 255;
      }
    }
  }

  return radiance_map;
}
