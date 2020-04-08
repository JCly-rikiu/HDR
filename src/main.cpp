#include <iostream>
#include <string>

#include "data.h"
#include "alignment.h"
#include "hdr.h"
#include "tone_mapping.h"

int main(int argc, char *argv[]) {
  std::string image_dir = argv[1];
  if (image_dir.back() != '/') image_dir += "/";
  auto image_data = load_images(image_dir);

  bool skip_alignment = false;
  if (argc > 2) {
    std::string arg = argv[2];
    if (arg.compare("--skip-alignment") == 0)
      skip_alignment = true;
  }
  image_data = alignment(image_data, skip_alignment);

  auto radiance_map = hdr(image_data);
  cv::imwrite(image_dir + "raidance_map.jpg", radiance_map);

  auto tonemap_image = tone_mapping(radiance_map);
  cv::imwrite(image_dir + "tone.jpg", tonemap_image);

  return 0;
}
