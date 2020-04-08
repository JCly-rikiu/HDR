#include <iostream>
#include <random>
#include <tuple>
#include <vector>

#include <opencv2/opencv.hpp>

#define ARMA_DONT_USE_WRAPPER
#include <armadillo>

#include "hdr.h"

const int lambda = 10;
const int sample_num = 100;

int weight(int z) {
  if (z < 128)
    return z + 1;
  else
    return 256 - z;
}

arma::vec solve(const std::vector<std::tuple<cv::Mat, double>>& image_data, int rows, int cols, int channel, std::vector<int> random_index) {
  arma::mat A = arma::zeros<arma::mat>(rows, cols);
  arma::vec b = arma::zeros<arma::vec>(rows);

  int it = 0;
  for (auto [image, exposure_time] : image_data) {
    for (int i = 0; i != sample_num; i++) {
      int index = random_index[i];
      int z = image.data[image.channels() * index + channel];
      A(it, z) = weight(z);
      A(it, 256 + i) = -weight(z);

      b(it) = weight(z) * std::log(exposure_time);

      it++;
    }
  }

  A(it++, 127) = 1;

  for (int i = 1; i != 255; i++) {
    A(it, i - 1) = lambda * weight(i);
    A(it, i) = -2 * lambda * weight(i);
    A(it, i + 1) = lambda * weight(i);
    it++;
  }

  return solve(A, b);
}

cv::Mat construct(const std::vector<std::tuple<cv::Mat, double>>& image_data, std::vector<arma::vec> response_curve, int rows, int cols) {
  cv::Mat radiance_map(rows, cols, CV_64FC3);

  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      double radiance_sum[3] = { 0.0, 0.0, 0.0 };
      int weight_sum[3] = { 0, 0, 0 };

      for (auto [image, exposure_time] : image_data) {
        auto value = image.at<cv::Vec4b>(i, j);
        if (value[3] == 0)  // skip alpha = 0, alignment blank
          continue;
        for (int channel = 0; channel != 3; channel++) {
          int z = value[channel];
          radiance_sum[channel] += weight(z) * (response_curve[channel][z] - std::log(exposure_time));
          weight_sum[channel] += weight(z);
        }
      }

      for (int channel = 0; channel != 3; channel++) {
        radiance_map.at<cv::Vec3d>(i, j)[channel] = std::exp(radiance_sum[channel] / weight_sum[channel]);
      }
    }
  }

  return radiance_map;
}

std::vector<int> get_random_index(const std::vector<std::tuple<cv::Mat, double>>& image_data, int pixels) {
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<> dist(0, pixels - 1);

  std::vector<int> random_index;
  while (random_index.size() < sample_num) {
    auto index = dist(random_engine);
    for (auto [image, time] : image_data) {
      if (image.data[image.channels() * index + 3] == 0) {  // skip alpha = 0, alignment blank
        index = -1;
        break;
      }
    }

    if (index != -1)
      random_index.push_back(index);
  }

  return random_index;
}

cv::Mat hdr(const std::vector<std::tuple<cv::Mat, double>>& image_data) {
  std::cout << "Constructing HDR..." << std::endl;

  auto img = std::get<0>(image_data[0]);
  auto pixels = img.rows * img.cols;

  int rows = sample_num * image_data.size() + 255;
  int cols = 256 + sample_num;

  auto random_index = get_random_index(image_data, pixels);

  std::vector<arma::vec> response_curve;
  for (int channel = 0; channel != 3; channel++)
    response_curve.push_back(solve(image_data, rows, cols, channel, random_index));

  return construct(image_data, response_curve, img.rows, img.cols);
}
