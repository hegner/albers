#include "podio/Writer.h"
#include "write_interface.h"

int main(int, char**) {

  auto writer = podio::makeWriter("example_frame_interface.root");
  write_frames(writer);

  return 0;
}
