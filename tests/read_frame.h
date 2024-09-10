#ifndef PODIO_TESTS_READ_FRAME_H // NOLINT(llvm-header-guard): folder structure not suitable
#define PODIO_TESTS_READ_FRAME_H // NOLINT(llvm-header-guard): folder structure not suitable

#include "datamodel/ExampleWithInterfaceRelationCollection.h"
#include "datamodel/ExampleWithVectorMemberCollection.h"
#include "extension_model/ContainedTypeCollection.h"
#include "extension_model/ExternalComponentTypeCollection.h"
#include "extension_model/ExternalRelationTypeCollection.h"
#include "podio/Frame.h"
#include "read_test.h"

#include <iostream>

#define ASSERT(condition, msg)                                                                                         \
  if (!(condition)) {                                                                                                  \
    throw std::runtime_error(msg);                                                                                     \
  }

void processExtensions(const podio::Frame& event, int iEvent, podio::version::Version) {
  const auto& extColl = event.get<extension::ContainedTypeCollection>("extension_Contained");
  ASSERT(extColl.isValid(), "extension_Contained collection should be present")
  ASSERT(extColl.size() == 1, "extension_Contained collection should have one element")
  auto extElem = extColl[0];
  const auto& polVec = extElem.getAVector();
  ASSERT(polVec.r == iEvent * 123.f, "polVec.r value not as expected")
  ASSERT(polVec.phi == 0.15f, "polVec.phi value not as expected")
  ASSERT(polVec.rho == 3.14f, "polVec.phi value not as expected")

  const auto& extCompColl = event.get<extension::ExternalComponentTypeCollection>("extension_ExternalComponent");
  ASSERT(extCompColl.isValid(), "extension_ExternalComponent collection should be present")
  ASSERT(extCompColl.size() == 1, "extension_ExternalComponent should have one element")
  auto extCompElem = extCompColl[0];
  ASSERT((extCompElem.getAStruct().p == std::array{iEvent, iEvent - 2, iEvent + 4, iEvent * 8}),
         "aStruct.p value not as expected")
  ASSERT(extCompElem.getAComponent().aStruct.data.x == 42 * iEvent, "aComponent.aStruct.x value not as expected")
  ASSERT(extCompElem.getAComponent().nspStruct.x == iEvent, "aComponent.nspStruct.x value not as expected")
  ASSERT(extCompElem.getAComponent().nspStruct.y == iEvent * 2, "aComponent.nspStruct.y value not as expected")

  const auto& extRelColl = event.get<extension::ExternalRelationTypeCollection>("extension_ExternalRelation");
  ASSERT(extRelColl.isValid(), "extension_ExternalRelation collection should be present")
  ASSERT(extRelColl.size() == 3, "extension_ExternalRelation collection should contain 3 elements")

  const auto& hits = event.get<ExampleHitCollection>("hits");
  auto elem0 = extRelColl[0];
  ASSERT(elem0.getWeight() == iEvent * 100.f, "weight of first element not as expected")
  ASSERT(elem0.getSingleHit() == hits[0], "single hit relation not as expected")

  const auto& clusters = event.get<ExampleClusterCollection>("clusters");
  auto elem1 = extRelColl[1];
  const auto relClusters = elem1.getClusters();
  ASSERT(relClusters.size() == 2, "element should have two related clusters")
  ASSERT(relClusters[0] == clusters[1], "first related cluster not as expected")
  ASSERT(relClusters[1] == clusters[0], "first related cluster not as expected")

  auto elem2 = extRelColl[2];
  const auto& structs = elem2.getSomeStructs();
  ASSERT(structs.size() == 3, "element should have 3 struct vector members")
  ASSERT(structs[0].y == 0, "struct value not as expected")
  ASSERT(structs[1].y == iEvent, "struct value not as expected")
  ASSERT(structs[2].y == 2 * iEvent, "struct value not as expected")
}

void checkVecMemSubsetColl(const podio::Frame& event) {
  const auto& subsetColl = event.get<ExampleWithVectorMemberCollection>("VectorMemberSubsetColl");
  const auto& origColl = event.get<ExampleWithVectorMemberCollection>("WithVectorMember");
  ASSERT(subsetColl.isSubsetCollection(), "subset collection not read back as a subset collection")
  ASSERT(subsetColl.size() == 1, "subset collection should have size 1")
  ASSERT(subsetColl[0] == origColl[0], "subset coll does not have the right contents")
}

void checkInterfaceCollection(const podio::Frame& event) {
  const auto& interfaceColl = event.get<ExampleWithInterfaceRelationCollection>("interface_examples");
  ASSERT(interfaceColl.size() == 2, "interface_examples should have two elements")

  const auto& hits = event.get<ExampleHitCollection>("hits");
  const auto& particles = event.get<ExampleMCCollection>("mcparticles");
  const auto& clusters = event.get<ExampleClusterCollection>("clusters");

  const auto iface0 = interfaceColl[0];
  const auto iface1 = interfaceColl[1];

  ASSERT(iface0.aSingleEnergyType() == hits[0], "OneToOneRelation aSingleEnergy not persisted as expected")
  ASSERT(iface1.aSingleEnergyType() == clusters[0], "OneToOneRelation aSingleEnergy not persisted as expected")

  const auto iface0Rels = iface0.manyEnergies();
  ASSERT(iface0Rels.size() == 3, "OneToManyRelation to interface does not have the expected number of related elements")
  ASSERT(iface0Rels[0] == hits[0], "OneToManyRelations to interface not persisted correctly")
  ASSERT(iface0Rels[1] == clusters[0], "OneToManyRelations to interface not persisted correctly")
  ASSERT(iface0Rels[2] == particles[0], "OneToManyRelations to interface not persisted correctly")

  const auto iface1Rels = iface1.manyEnergies();
  ASSERT(iface1Rels.size() == 3, "OneToManyRelation to interface does not have the expected number of related elements")
  ASSERT(iface1Rels[0] == particles[0], "OneToManyRelations to interface not persisted correctly")
  ASSERT(iface1Rels[1] == hits[0], "OneToManyRelations to interface not persisted correctly")
  ASSERT(iface1Rels[2] == clusters[0], "OneToManyRelations to interface not persisted correctly")
}

template <typename ReaderT>
int read_frames(const std::string& filename, bool assertBuildVersion = true) {
  auto reader = ReaderT();
  try {
    reader.openFile(filename);
  } catch (const std::runtime_error& e) {
    std::cout << "File could not be opened, aborting." << std::endl;
    return 1;
  }

  if (assertBuildVersion && reader.currentFileVersion() != podio::version::build_version) {
    std::cerr << "The podio build version could not be read back correctly. "
              << "(expected:" << podio::version::build_version << ", actual: " << reader.currentFileVersion() << ")"
              << std::endl;
    return 1;
  }

  if (reader.getEntries(podio::Category::Event) != 10) {
    std::cerr << "Could not read back the number of events correctly. "
              << "(expected:" << 10 << ", actual: " << reader.getEntries(podio::Category::Event) << ")" << std::endl;
    return 1;
  }

  if (reader.getEntries(podio::Category::Event) != reader.getEntries("other_events")) {
    std::cerr << "Could not read back the number of events correctly. "
              << "(expected:" << 10 << ", actual: " << reader.getEntries("other_events") << ")" << std::endl;
    return 1;
  }

  // Read the frames in a different order than when writing them here to make
  // sure that the writing/reading order does not impose any usage requirements
  for (size_t i = 0; i < reader.getEntries(podio::Category::Event); ++i) {
    auto frame = podio::Frame(reader.readNextEntry(podio::Category::Event));
    if (reader.currentFileVersion() > podio::version::Version{0, 16, 2}) {
      if (frame.get("emptySubsetColl") == nullptr) {
        std::cerr << "Could not retrieve an empty subset collection" << std::endl;
        return 1;
      }
      if (frame.get("emptyCollection") == nullptr) {
        std::cerr << "Could not retrieve an empty collection" << std::endl;
        return 1;
      }
    }

    processEvent(frame, i, reader.currentFileVersion());

    auto otherFrame = podio::Frame(reader.readNextEntry("other_events"));
    processEvent(otherFrame, i + 100, reader.currentFileVersion());
    // The other_events category also holds external collections
    if (reader.currentFileVersion() > podio::version::Version{0, 16, 2}) {
      processExtensions(otherFrame, i + 100, reader.currentFileVersion());
    }
    // As well as a test for the vector members subset category
    if (reader.currentFileVersion() >= podio::version::Version{0, 16, 99}) {
      checkVecMemSubsetColl(otherFrame);
    }
    // Interface tests once they are present
    if (reader.currentFileVersion() >= podio::version::Version{0, 99, 99}) {
      checkInterfaceCollection(otherFrame);
    }
  }

  if (reader.readNextEntry(podio::Category::Event)) {
    std::cerr << "Trying to read more frame data than is present should return a nullptr" << std::endl;
    return 1;
  }

  std::cout << "========================================================\n" << std::endl;
  if (reader.readNextEntry("not_present")) {
    std::cerr << "Trying to read non-existant frame data should return a nullptr" << std::endl;
    return 1;
  }

  // Reading specific (jumping to) entry
  {
    auto frame = podio::Frame(reader.readEntry(podio::Category::Event, 4));
    processEvent(frame, 4, reader.currentFileVersion());
    // Reading the next entry after jump, continues from after the jump
    auto nextFrame = podio::Frame(reader.readNextEntry(podio::Category::Event));
    processEvent(nextFrame, 5, reader.currentFileVersion());

    auto otherFrame = podio::Frame(reader.readEntry("other_events", 4));
    processEvent(otherFrame, 4 + 100, reader.currentFileVersion());
    if (reader.currentFileVersion() > podio::version::Version{0, 16, 2}) {
      processExtensions(otherFrame, 4 + 100, reader.currentFileVersion());
    }

    // Jumping back also works
    auto previousFrame = podio::Frame(reader.readEntry("other_events", 2));
    processEvent(previousFrame, 2 + 100, reader.currentFileVersion());
    if (reader.currentFileVersion() > podio::version::Version{0, 16, 2}) {
      processExtensions(previousFrame, 2 + 100, reader.currentFileVersion());
    }

    // Trying to read a Frame that is not present returns a nullptr
    if (reader.readEntry(podio::Category::Event, 10)) {
      std::cerr << "Trying to read a specific entry that does not exist should return a nullptr" << std::endl;
      return 1;
    }
  }

  return 0;
}

#endif // PODIO_TESTS_READ_FRAME_H
