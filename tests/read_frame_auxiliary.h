#ifndef PODIO_TESTS_READ_FRAME_AUXILIARY_H // NOLINT(llvm-header-guard): folder structure not suitable
#define PODIO_TESTS_READ_FRAME_AUXILIARY_H // NOLINT(llvm-header-guard): folder structure not suitable

#include "frame_test_common.h"
#include "podio/Frame.h"
#include "podio/podioVersion.h"

#include <iostream>
#include <string>
#include <vector>

bool present(const std::string& elem, const std::vector<std::string>& vec) {
  return std::find(vec.begin(), vec.end(), elem) != vec.end();
}

int testGetAvailableCollections(const podio::Frame& frame, const std::vector<std::string>& expected) {
  const auto& collNames = frame.getAvailableCollections();
  int result = 0;
  for (const auto& name : expected) {
    if (!present(name, collNames)) {
      std::cerr << "Cannot find expected collection " << name << " in collections of Frame" << std::endl;
      result = 1;
    }
  }

  // Get a few collections and make sure that the results are unchanged (apart
  // from ordering)
  frame.get("hitRefs");
  frame.get("mcparticles");

  const auto& newCollNames = frame.getAvailableCollections();
  for (const auto& name : newCollNames) {
    if (!present(name, collNames)) {
      std::cerr << "getAvailableCollections returns different collections after getting collections" << std::endl;
      return 1;
    }
  }

  return result;
}

/**
 * Test function for testing some auxiliary functionality of the Frame.
 * Encapsulates everything, such that a corresponding main function boils down
 * to including the reader to test and defining a main that invokes and returns
 * this function.
 *
 * @param fileName the name of the file to read from
 * @tparam ReaderT a Frame based I/O capable reader
 * @return 0 if all checks pass, non-zero otherwise
 * */
template <typename ReaderT>
int test_frame_aux_info(const std::string& fileName) {
  auto reader = ReaderT{};
  reader.openFile(fileName);

  // Test on the first event only here. Additionally, also only testing the
  // events category, since that is the one where not all collections are
  // written
  auto event = podio::Frame(reader.readEntry(podio::Category::Event, 0));

  auto collsToRead = collsToWrite;
  if (reader.currentFileVersion() < podio::version::Version{0, 16, 3}) {
    collsToRead.erase(collsToRead.end() - 2, collsToRead.end());
  }

  return testGetAvailableCollections(event, collsToRead);
}

#endif // PODIO_TESTS_READ_FRAME_AUXILIARY_H
