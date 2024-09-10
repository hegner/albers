#ifndef PODIO_TESTS_READ_PYTHON_FRAME_H // NOLINT(llvm-header-guard): folder structure not suitable
#define PODIO_TESTS_READ_PYTHON_FRAME_H // NOLINT(llvm-header-guard): folder structure not suitable

#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleHitCollection.h"
#include "podio/Frame.h"

#include <iostream>

int checkHits(const ExampleHitCollection& hits) {
  if (hits.size() != 2) {
    std::cerr << "There should be two hits in the collection (actual size: " << hits.size() << ")" << std::endl;
    return 1;
  }

  auto hit1 = hits[0];
  if (hit1.cellID() != 0xbad || hit1.x() != 0.0 || hit1.y() != 0.0 || hit1.z() != 0.0 || hit1.energy() != 23.0) {
    std::cerr << "Could not retrieve the correct hit[0]: (expected: " << MutableExampleHit(0xbad, 0.0, 0.0, 0.0, 23.0)
              << ", actual: " << hit1 << ")" << std::endl;
    return 1;
  }

  auto hit2 = hits[1];
  if (hit2.cellID() != 0xcaffee || hit2.x() != 1.0 || hit2.y() != 0.0 || hit2.z() != 0.0 || hit2.energy() != 12.0) {
    std::cerr << "Could not retrieve the correct hit[1]: (expected: "
              << MutableExampleHit(0xcaffee, 1.0, 0.0, 0.0, 12.0) << ", actual: " << hit2 << ")" << std::endl;
    return 1;
  }

  return 0;
}

int checkClusters(const ExampleClusterCollection& clusters) {
  if (clusters.size() != 2) {
    std::cerr << "There should be two clusters in the collection (actual size: " << clusters.size() << ")" << std::endl;
    return 1;
  }

  if (clusters[0].energy() != 3.14 || clusters[1].energy() != 1.23) {
    std::cerr << "Energies of the clusters is wrong: (expected: 3.14 and 1.23, actual " << clusters[0].energy()
              << " and " << clusters[1].energy() << ")" << std::endl;
    return 1;
  }

  return 0;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& vec) {
  auto delim = "[";
  for (const auto& v : vec) {
    o << std::exchange(delim, ", ") << v;
  }
  return o << "]";
}

int checkParameters(const podio::Frame& frame) {
  const auto iVal = frame.getParameter<int>("an_int").value();
  if (iVal != 42) {
    std::cerr << "Parameter an_int was not stored correctly (expected 42, actual " << iVal << ")" << std::endl;
    return 1;
  }

  const auto dVal = frame.getParameter<std::vector<double>>("some_floats").value();
  if (dVal.size() != 3 || dVal[0] != 1.23 || dVal[1] != 7.89 || dVal[2] != 3.14) {
    std::cerr << "Parameter some_floats was not stored correctly (expected [1.23, 7.89, 3.14], actual " << dVal << ")"
              << std::endl;
    return 1;
  }

  const auto strVal = frame.getParameter<std::vector<std ::string>>("greetings").value();
  if (strVal.size() != 2 || strVal[0] != "from" || strVal[1] != "python") {
    std::cerr << "Parameter greetings was not stored correctly (expected [from, python], actual " << strVal << ")"
              << std::endl;
    return 1;
  }

  const auto realFloat = frame.getParameter<float>("real_float").value_or(-1.f);
  if (realFloat != 3.14f) {
    std::cerr << "Parameter real_float was not stored correctly (expected 3.14, actual " << realFloat << ")"
              << std::endl;
    return 1;
  }

  const auto realFloats = frame.getParameter<std::vector<float>>("more_real_floats").value();
  if (realFloats.size() != 3 || realFloats[0] != 1.23f || realFloats[1] != 4.56f || realFloats[2] != 7.89f) {
    std::cerr << "Parameter more_real_floats was not stored as correctly (expected [1.23, 4.56, 7.89], actual"
              << realFloats << ")" << std::endl;
    return 1;
  }

  return 0;
}

template <typename ReaderT>
int read_frame(const std::string& filename) {
  auto reader = ReaderT();
  reader.openFile(filename);

  auto event = podio::Frame(reader.readEntry("events", 0));

  return checkHits(event.get<ExampleHitCollection>("hits_from_python")) +
      checkClusters(event.get<ExampleClusterCollection>("clusters_from_python")) + checkParameters(event);
}

#endif // PODIO_TESTS_READ_PYTHON_FRAME_H
