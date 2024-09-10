#include "podio/Frame.h"
#include "podio/SIOLegacyReader.h"
#include "read_test.h"

#include <iostream>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: read_frame_legacy_sio inputfile" << std::endl;
    return 1;
  }

  const auto inputFile = argv[1];
  auto reader = podio::SIOLegacyReader();
  try {
    reader.openFile(inputFile);
  } catch (const std::runtime_error& e) {
    std::cout << "File (" << inputFile << ") could not be opened, aborting." << std::endl;
    return 1;
  }

  if (reader.getEntries("events") != 2000) {
    std::cerr << "Could not read back the number of events correctly. "
              << "(expected:" << 2000 << ", actual: " << reader.getEntries("events") << ")" << std::endl;
    return 1;
  }

  for (size_t i = 0; i < reader.getEntries("events"); ++i) {
    const auto frame = podio::Frame(reader.readNextEntry("events"));
    processEvent(frame, i, reader.currentFileVersion());
  }

  // Reading specific entries
  {
    auto frame = podio::Frame(reader.readEntry("events", 4));
    processEvent(frame, 4, reader.currentFileVersion());

    auto nextFrame = podio::Frame(reader.readNextEntry("events"));
    processEvent(nextFrame, 5, reader.currentFileVersion());

    auto previousFrame = podio::Frame(reader.readEntry("events", 2));
    processEvent(previousFrame, 2, reader.currentFileVersion());
  }

  return 0;
}
