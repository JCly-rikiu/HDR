#include <iostream>

#include "data.h"

void load_images(std::string& image_dir) {
  if (image_dir.back() != '/')
    image_dir += "/";
  std::cout << image_dir << std::endl;
}
