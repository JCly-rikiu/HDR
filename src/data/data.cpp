#include "data.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <opencv2/opencv.hpp>

std::vector<std::tuple<cv::Mat, double>> load_images(std::string&& image_dir) {
  std::vector<std::tuple<cv::Mat, double>> images;

  if (image_dir.back() != '/') image_dir += "/";
  std::ifstream infile(image_dir + "image_list.txt");
  if (infile.fail()) std::cerr << "fail to read file" << std::endl;

  std::string line;
  while (std::getline(infile, line)) {
    std::istringstream line_stream(line);
    std::string filename;
    double t;
    if (!(line_stream >> filename >> t)) break;

    cv::Mat image;
    image = cv::imread(image_dir + filename, cv::IMREAD_COLOR);
    if (!image.data) {
      std::cerr << "Could not open or find the image!" << std::endl;
      break;
    }

    images.push_back({image, 1 / t});
  }

  std::sort(images.begin(), images.end(), [](auto const &t1, auto const &t2) {
    return std::get<1>(t1) > std::get<1>(t2);
  });

  return images;
}
