#include "podio/SIOReader.h"
#include "podio/SIOWriter.h"
#include "read_and_write_frame.h"

int main() {
  return rewrite_frames<podio::SIOReader, podio::SIOWriter>("example_frame.sio", "rewritten_frame.sio") +
      read_rewritten_frames<podio::SIOReader>("rewritten_frame.sio");
}
