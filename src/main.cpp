#include <iostream>
#include <string>

#include "data.h"

int main(int argc, char *argv[]) {
  std::string image_dir = argv[1];
  auto images = load_images(argv[1]);

  return 0;
}
