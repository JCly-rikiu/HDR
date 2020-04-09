#include <deque>
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

const double ghost_eps = 1.0;

std::vector<int> get_random_index(
    const std::vector<std::tuple<cv::Mat, double>>& image_data,
    const int pixels) {
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

    if (index != -1) random_index.push_back(index);
  }

  return random_index;
}

int weight(int z) {
  if (z < 128)
    return z + 1;
  else
    return 256 - z;
}

arma::vec solve(const std::vector<std::tuple<cv::Mat, double>>& image_data,
                int rows, int cols, int channel,
                std::vector<int> random_index) {
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

cv::Mat construct(const std::vector<std::tuple<cv::Mat, double>>& image_data,
                  std::vector<arma::vec> response_curve, const int rows,
                  const int cols, const bool ghost_removal) {
  cv::Mat radiance_map(rows, cols, CV_64FC3);

  if (ghost_removal)
    std::cout << "\tremove ghost" << std::endl;

  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      std::deque<std::tuple<double, double>> radiance_weight[3];

      for (auto [image, exposure_time] : image_data) {
        auto value = image.at<cv::Vec4b>(i, j);
        if (value[3] == 0)  // skip alpha = 0, alignment blank
          continue;

        double weight_mean = 0.0;
        for (int channel = 0; channel != 3; channel++) {
          int z = value[channel];
          weight_mean += weight(z);
        }
        weight_mean /= 3;

        for (int channel = 0; channel != 3; channel++) {
          int z = value[channel];
          radiance_weight[channel].push_back({response_curve[channel][z] - std::log(exposure_time), weight_mean});
        }
      }

      for (int channel = 0; channel != 3; channel++) {
        if (ghost_removal) {
          std::sort(radiance_weight[channel].begin(), radiance_weight[channel].end());
          auto median = std::get<0>(radiance_weight[channel][radiance_weight[channel].size() / 2]);
          while (median - std::get<0>(radiance_weight[channel].front()) > ghost_eps)
            radiance_weight[channel].pop_front();
          while (std::get<0>(radiance_weight[channel].back()) - median > ghost_eps)
            radiance_weight[channel].pop_back();
        }

        double radiance_sum = 0.0, weight_sum = 0.0;
        for (auto [radiance, weight] : radiance_weight[channel]) {
          radiance_sum += radiance * weight;
          weight_sum += weight;
        }
        radiance_map.at<cv::Vec3d>(i, j)[channel] = std::exp(radiance_sum / weight_sum);
      }
    }
  }

  return radiance_map;
}

cv::Mat hdr(const std::vector<std::tuple<cv::Mat, double>>& image_data,
            const bool ghost_removal = false) {
  std::cout << "[Constructing HDR...]" << std::endl;

  auto img = std::get<0>(image_data[0]);
  auto pixels = img.rows * img.cols;

  int rows = sample_num * image_data.size() + 255;
  int cols = 256 + sample_num;

  auto random_index = get_random_index(image_data, pixels);

  std::vector<arma::vec> response_curve;
  for (int channel = 0; channel != 3; channel++)
    response_curve.push_back(solve(image_data, rows, cols, channel, random_index));

  return construct(image_data, response_curve, img.rows, img.cols, ghost_removal);
}
