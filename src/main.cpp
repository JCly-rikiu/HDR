#include <iostream>
#include <string>

#include "data.h"

int main(int argc, char *argv[]) {
  std::string image_dir = argv[1];
  load_images(image_dir);

  return 0;
}
