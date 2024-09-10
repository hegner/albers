#include "podio/ROOTReader.h"
#include "podio/ROOTWriter.h"
#include "read_and_write_frame.h"

int main() {
  return rewrite_frames<podio::ROOTReader, podio::ROOTWriter>("example_frame.root", "rewritten_frame.root") +
      read_rewritten_frames<podio::ROOTReader>("rewritten_frame.root");
}
