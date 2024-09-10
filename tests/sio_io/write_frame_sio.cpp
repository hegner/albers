#include "podio/SIOWriter.h"
#include "write_frame.h"

int main(int, char**) {
  write_frames<podio::SIOWriter>("example_frame.sio");
  return 0;
}
