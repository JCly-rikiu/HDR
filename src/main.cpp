#include <iostream>
#include <string>

#include "data.h"
#include "alignment.h"
#include "hdr.h"
#include "tone_mapping.h"

int main(int argc, char *argv[]) {
  std::string image_dir = argv[1];
  if (image_dir.back() != '/') image_dir += "/";

  bool skip_alignment = false;
  bool ghost_removal = false;
  int tone = 2;
  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.compare("--skip-alignment") == 0)
      skip_alignment = true;
    if (arg.compare("--global-tone") == 0)
      tone = 1;
    if (arg.compare("--blend-tone") == 0)
      tone = 0;
    if (arg.compare("--contrast-tone") == 0)
      tone = 3;
    if (arg.compare("--remove-ghost") == 0)
      ghost_removal = true;
  }

  auto image_data = load_images(image_dir);

  image_data = alignment(image_data, skip_alignment);

  auto radiance_map = hdr(image_data, ghost_removal);
  cv::imwrite(image_dir + "radiance_map.hdr", radiance_map);
  std::cout << "\tSave radiance map to " + image_dir + "radiance_map.hdr" << std::endl;

  auto tonemap_image = tone_mapping(radiance_map, tone);
  cv::imwrite(image_dir + "tone.jpg", tonemap_image);
  std::cout << "\tSave tone mapped image to " + image_dir + "tone.jpg" << std::endl;

  return 0;
}
