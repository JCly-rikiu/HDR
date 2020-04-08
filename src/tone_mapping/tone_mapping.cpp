#include <numeric>
#include <vector>

#include <opencv2/opencv.hpp>

#include "tone_mapping.h"

const double a = 0.18;
const double eps = 0.05;

cv::Mat global_operator(const cv::Mat& radiance_map, const cv::Mat& Lm,
                        const double Lwhite) {
  auto rows = radiance_map.rows;
  auto cols = radiance_map.cols;
  cv::Mat Ld(rows, cols, CV_64FC1);

  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      double Lm_l = Lm.at<double>(i, j);
      Ld.at<double>(i, j) =
          (Lm_l * (1 + Lm_l / std::pow(Lwhite, 2))) / (1 + Lm_l);
    }
  }

  return Ld;
}

cv::Mat local_operator(const cv::Mat& radiance_map, const cv::Mat& Lm,
                       const double Lwhite) {
  auto rows = radiance_map.rows;
  auto cols = radiance_map.cols;
  cv::Mat Ld(rows, cols, CV_64FC1);

  double alpha = 0.35, s = 1.0;
  std::vector<cv::Mat> blur_lum;
  for (int i = 0; i != 8; i++) {
    cv::Mat blur(rows, cols, CV_64FC1);
    cv::GaussianBlur(Lm, blur, cv::Size(), alpha * s);
    blur_lum.push_back(blur);

    alpha *= 1.6;
    s *= 1.6;
  }

  s = 1.0;
  for (int i = 0; i != rows; i++) {
    for (int j = 0; j != cols; j++) {
      int smax = 7;
      for (int k = 0; k != 7; k++) {
        auto v =
            (blur_lum[k].at<double>(i, j) - blur_lum[k + 1].at<double>(i, j)) /
            (std::pow(2.0, 8.0) * a / (s * s));
        if (std::abs(v) < eps) {
          smax = k;
          break;
        }

        s *= 1.6;
      }

      auto Lm_l = Lm.at<double>(i, j);
      Ld.at<double>(i, j) = Lm_l * (1 + Lm_l / std::pow(Lwhite, 2)) /
                            (1 + blur_lum[smax].at<double>(i, j));
    }
  }

  return Ld;
}

cv::Mat tone_mapping(const cv::Mat& radiance_map) {
  std::cout << "Tone mapping..." << std::endl;

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
  std::cout << "lum_mean: " << lum_mean << std::endl;
  std::cout << "Lwhite: " << Lwhite << std::endl;

  cv::Mat Lm(rows, cols, CV_64FC1);
  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      Lm.at<double>(i, j) = Lw.at<double>(i, j) * a / lum_mean;

  cv::Mat tonemap(rows, cols, CV_8UC3);
  auto Ld_g = global_operator(radiance_map, Lm, Lwhite);
  auto Ld_l = local_operator(radiance_map, Lm, Lwhite);
  double blend = 0.5;
  for (int i = 0; i != rows; i++)
    for (int j = 0; j != cols; j++)
      for (int channel = 0; channel != 3; channel++) {
        auto Ld = Ld_g.at<double>(i, j) * (blend) +
                  Ld_l.at<double>(i, j) * (1.0 - blend);
        auto value = radiance_map.at<cv::Vec3d>(i, j)[channel] * Ld /
                     Lw.at<double>(i, j) * 255;
        tonemap.at<cv::Vec3b>(i, j)[channel] =
            static_cast<unsigned char>(std::clamp(value, 0.0, 255.0));
      }
  return tonemap;
}
