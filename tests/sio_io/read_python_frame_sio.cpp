#include "podio/SIOReader.h"
#include "read_python_frame.h"

int main() {
  return read_frame<podio::SIOReader>("example_frame_with_py.sio");
}
