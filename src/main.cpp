#include <iostream>
#include <string>

#include "data.h"
#include "alignment.h"

int main(int argc, char *argv[]) {
  std::string image_dir = argv[1];
  auto image_data = load_images(argv[1]);

  image_data = alignment(image_data);

  return 0;
}
