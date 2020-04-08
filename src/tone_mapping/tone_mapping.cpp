#include <numeric>
#include <vector>

#include <opencv2/opencv.hpp>

#include "tone_mapping.h"
#include "contrast.h"

const double a = 0.34;
const double eps = 0.05;
const double phi = 8.0;

cv::Mat global_operator(const cv::Mat& radiance_map, const cv::Mat& Lm,
                        const double Lwhite) {
  auto rows = radiance_map.rows;
  auto cols = radiance_map.cols;
  cv::Mat Ld(rows, cols, CV_64FC1);

  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      double Lm_l = Lm.at<double>(i, j);
      Ld.at<double>(i, j) = (Lm_l * (1 + Lm_l / std::pow(Lwhite, 2))) / (1 + Lm_l);
    }
  }

  return Ld;
}

cv::Mat local_operator(const cv::Mat& radiance_map, const cv::Mat& Lm,
                       const double Lwhite) {
  auto rows = radiance_map.rows;
  auto cols = radiance_map.cols;
  cv::Mat Ld(rows, cols, CV_64FC1);

  const double alpha_1 = 0.35, alpha_2 = 0.35 * 1.6;
  double s = 1.0;
  std::vector<cv::Mat> v1s, v2s;
  for (int i = 0; i != 8; i++) {
    cv::Mat v1(rows, cols, CV_64FC1);
    cv::Mat v2(rows, cols, CV_64FC1);
    cv::GaussianBlur(Lm, v1, cv::Size(), alpha_1 * s, alpha_1 * s,
                     cv::BORDER_REPLICATE);
    cv::GaussianBlur(Lm, v2, cv::Size(), alpha_2 * s, alpha_2 * s,
                     cv::BORDER_REPLICATE);
    v1s.push_back(v1);
    v2s.push_back(v2);

    s *= 1.6;
  }

  s = 1.0;
  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      int smax = 7;
      for (int k = 0; k != 8; k++) {
        auto v = (v1s[k].at<double>(i, j) - v2s[k].at<double>(i, j)) /
                 (std::pow(2.0, phi) * a / (s * s) + v1s[k].at<double>(i, j));
        if (std::abs(v) < eps) {
          smax = k;
          break;
        }

        s *= 1.6;
      }

      auto Lm_l = Lm.at<double>(i, j);
      Ld.at<double>(i, j) = Lm_l * (1 + Lm_l / std::pow(Lwhite, 2)) /
                            (1 + v1s[smax].at<double>(i, j));
    }
  }

  return Ld;
}

cv::Mat tone_mapping(const cv::Mat& radiance_map, const int tone = 2) {
  std::cout << "[Tone mapping...]" << std::endl;

  if (tone == 3)
    return contrast(radiance_map);

  if (tone == 0)
    std::cout << "blend global and local operator" << std::endl;
  else if (tone == 1)
    std::cout << "global operator" << std::endl;
  else
    std::cout << "local operator" << std::endl;

  auto rows = radiance_map.rows;
  auto cols = radiance_map.cols;

  cv::Mat Lw(rows, cols, CV_64FC1);
  double lum_mean = 0.0;
  double Lwhite = 0.0;

  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      auto value = radiance_map.at<cv::Vec3d>(i, j);
      auto lum = Lw.at<double>(i, j) =
          0.27 * value[0] + 0.67 * value[1] + 0.06 * value[2];
      for (int c = 0; c != 3; c++) Lwhite = std::max(Lwhite, value[c]);
      lum_mean += std::log(0.000001 + lum);
    }
  }
  lum_mean = std::exp(lum_mean / (rows * cols));
  std::cout << "\tLwhite: " << Lwhite << std::endl;

  cv::Mat Lm(rows, cols, CV_64FC1);
  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      Lm.at<double>(i, j) = Lw.at<double>(i, j) * a / lum_mean;

  cv::Mat tonemap(rows, cols, CV_64FC3);
  double blend = 0.5;
  if (tone == 1) blend = 1.0;
  if (tone == 2) blend = 0.0;
  auto Ld_g = tone != 2 ? global_operator(radiance_map, Lm, Lwhite)
                        : cv::Mat::zeros(radiance_map.size(), CV_64FC1);
  auto Ld_l = tone != 1 ? local_operator(radiance_map, Lm, Lwhite)
                        : cv::Mat::zeros(radiance_map.size(), CV_64FC1);
  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      for (int channel = 0; channel != 3; channel++) {
        auto Ld = Ld_g.at<double>(i, j) * (blend) +
                  Ld_l.at<double>(i, j) * (1.0 - blend);
        auto value = radiance_map.at<cv::Vec3d>(i, j)[channel] * Ld /
                     Lw.at<double>(i, j);
        // value = std::pow(value, 1 / 1.2);
        tonemap.at<cv::Vec3d>(i, j)[channel] = value * 255;
      }

  return tonemap;
}
