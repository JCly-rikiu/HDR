#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

#include <opencv2/opencv.hpp>

#include "alignment.h"

const int eps = 4;

std::tuple<cv::Mat, cv::Mat> create_bitmap(const cv::Mat& gray) {
  cv::Mat threshold_bitmap(gray.size(), CV_8U);
  cv::Mat exclude_bitmap(gray.size(), CV_8U);

  std::vector<uchar> values;
  for (auto it = gray.begin<uchar>(), end = gray.end<uchar>(); it != end; ++it)
    values.push_back(*it);
  std::sort(values.begin(), values.end());
  auto median = values[values.size() / 2];

  for (int row = 0; row < gray.rows; row++) {
    const unsigned char* gray_pixel = gray.ptr(row);
    unsigned char* threshold_pixel = threshold_bitmap.ptr(row);
    unsigned char* exclude_pixel = exclude_bitmap.ptr(row);
    for (int col = 0; col < gray.cols;
         col++, gray_pixel++, threshold_pixel++, exclude_pixel++) {
      *threshold_pixel = (*gray_pixel > median) ? 255 : 0;
      *exclude_pixel =
          (*gray_pixel > median - eps && *gray_pixel < median + eps) ? 0 : 255;
    }
  }

  return {threshold_bitmap, exclude_bitmap};
}

std::tuple<cv::Mat, cv::Mat> shift_bitmap(const cv::Mat& threshold,
                                          const cv::Mat& exclude, int rs,
                                          int cs) {
  cv::Mat shift_threshold(threshold.size(), CV_8U);
  cv::Mat shift_exclude(exclude.size(), CV_8U);

  cv::Mat mat = (cv::Mat_<double>(2, 3) << 1, 0, rs, 0, 1, cs);
  cv::warpAffine(threshold, shift_threshold, mat, shift_threshold.size(),
                 cv::INTER_NEAREST);
  cv::warpAffine(exclude, shift_exclude, mat, shift_exclude.size(),
                 cv::INTER_NEAREST);

  return {shift_threshold, shift_exclude};
}

int get_diff_error(const cv::Mat& center_threshold,
                   const cv::Mat& shift_threshold,
                   const cv::Mat& center_exclude,
                   const cv::Mat& shift_exclude) {
  cv::Mat diff(center_threshold.size(), CV_8U);
  cv::bitwise_xor(center_threshold, shift_threshold, diff);
  cv::bitwise_and(diff, center_exclude, diff);
  cv::bitwise_and(diff, shift_exclude, diff);

  return cv::countNonZero(diff);
}

std::tuple<int, int> get_exp_shift(const cv::Mat& center_image,
                                   const cv::Mat& image, int shift_bits) {
  int row_shift = 0, col_shift = 0;

  if (shift_bits > 0) {
    cv::Mat small_center_image, small_image;
    cv::resize(center_image, small_center_image, cv::Size(), 0.5, 0.5,
               cv::INTER_NEAREST);
    cv::resize(image, small_image, cv::Size(), 0.5, 0.5, cv::INTER_NEAREST);
    auto [r, c] =
        get_exp_shift(small_center_image, small_image, shift_bits - 1);
    row_shift = r * 2;
    col_shift = c * 2;
  }

  auto [center_threshold, center_exclude] = create_bitmap(center_image);
  auto [threshold, exclude] = create_bitmap(image);

  int min_error = center_image.rows * center_image.cols;
  int ret_row_shift = row_shift, ret_col_shift = col_shift;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      int rs = row_shift + i, cs = col_shift + j;

      auto [shift_threshold, shift_exclude] =
          shift_bitmap(threshold, exclude, rs, cs);

      int error = get_diff_error(center_threshold, shift_threshold,
                                 center_exclude, shift_exclude);
      if (min_error > error) {
        ret_row_shift = rs;
        ret_col_shift = cs;
        min_error = error;
      }
    }
  }

  return {ret_row_shift, ret_col_shift};
}

cv::Mat mtb_alignment(const cv::Mat& center_image, const cv::Mat& image) {
  cv::Mat center_gray, gray;
  cv::cvtColor(center_image, center_gray, cv::COLOR_RGB2GRAY);
  cv::cvtColor(image, gray, cv::COLOR_RGB2GRAY);

  auto [row_shift, col_shift] = get_exp_shift(center_gray, gray, 5);

  cv::Mat shift_image = cv::Mat::zeros(image.size(), CV_8UC4);
  cv::cvtColor(image, shift_image, cv::COLOR_RGB2RGBA, 4);
  cv::Mat mat = (cv::Mat_<double>(2, 3) << 1, 0, row_shift, 0, 1, col_shift);
  cv::warpAffine(shift_image, shift_image, mat, shift_image.size(),
                 cv::INTER_NEAREST);

  return shift_image;
}

std::vector<std::tuple<cv::Mat, double>> alignment(
    const std::vector<std::tuple<cv::Mat, double>>& image_data,
    bool skip = false) {
  std::vector<std::tuple<cv::Mat, double>> ret_image_data;
  if (skip) {
    for (auto [image, exposure_time] : image_data) {
      cv::Mat temp_image = cv::Mat::zeros(image.size(), CV_8UC4);
      cv::cvtColor(image, temp_image, cv::COLOR_RGB2RGBA, 4);
      ret_image_data.push_back({temp_image, exposure_time});
    }
    return ret_image_data;
  }

  std::cout << "Alignment..." << std::endl;

  auto middle = image_data.size() / 2;

  auto& [middle_image, exposure_time] = image_data[middle];

  for (size_t i = 0; i != image_data.size(); i++) {
    if (i == middle) {
      cv::Mat temp_image = cv::Mat::zeros(middle_image.size(), CV_8UC4);
      cv::cvtColor(middle_image, temp_image, cv::COLOR_RGB2RGBA, 4);
      ret_image_data.push_back({temp_image, exposure_time});
      continue;
    }
    auto [image, exposure_time] = image_data[i];
    auto shift_image = mtb_alignment(middle_image, image);

    ret_image_data.push_back({shift_image, exposure_time});
  }

  return ret_image_data;
}
