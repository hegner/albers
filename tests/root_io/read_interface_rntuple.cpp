#include "podio/Reader.h"
#include "read_interface.h"

int main(int, char**) {
  auto reader = podio::makeReader("example_from_rntuple_interface.root");
  if (read_frames(reader)) {
    return 1;
  }

  return 0;
}
