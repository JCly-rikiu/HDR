#include <iostream>
#include <string>

#include "data.h"
#include "alignment.h"
#include "hdr.h"

int main(int argc, char *argv[]) {
  std::string image_dir = argv[1];
  auto image_data = load_images(argv[1]);

  image_data = alignment(image_data);

  auto radiance_map = hdr(image_data);
  cv::imwrite(image_dir + "raidance_map.hdr", radiance_map);

  return 0;
}
