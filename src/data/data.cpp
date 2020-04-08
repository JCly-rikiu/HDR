#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <opencv2/opencv.hpp>

#include "data.h"

std::vector<std::tuple<cv::Mat, double>> load_images(std::string& image_dir) {
  std::cout << "Loading images..." << std::endl;

  std::vector<std::tuple<cv::Mat, double>> images;

  std::ifstream infile(image_dir + "image_list.txt");
  if (infile.fail()) std::cerr << "fail to read file" << std::endl;

  std::string line;
  while (std::getline(infile, line)) {
    std::istringstream line_stream(line);
    std::string filename;
    double shutter_speed;
    if (!(line_stream >> filename >> shutter_speed)) break;

    cv::Mat image;
    image = cv::imread(image_dir + filename, cv::IMREAD_COLOR);
    if (!image.data) {
      std::cerr << "Could not open or find the image!" << std::endl;
      break;
    }

    images.push_back({image, 1 / shutter_speed});
  }

  std::sort(images.begin(), images.end(), [](auto const &t1, auto const &t2) {
    return std::get<1>(t1) > std::get<1>(t2);
  });

  return images;
}
