#include "datamodel/ExampleMC.h"
#include "datamodel/ExampleMCCollection.h"
#include "datamodel/ExampleReferencingType.h"
#include "datamodel/MutableExampleMC.h"
#include "datamodel/MutableExampleReferencingType.h"
#include "datamodel/MutableExampleWithVectorMember.h"
#include "podio/Frame.h"
#include "podio/FrameCategories.h"
#include "podio/ROOTReader.h"
#include "podio/ROOTWriter.h"
#include "podio/RelationRange.h"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#define LOCATION()                                                                                                     \
  std::string(__FILE__) + std::string(":") + std::to_string(__LINE__) + std::string(" in ") +                          \
      std::string(__PRETTY_FUNCTION__)

#define ASSERT_CONDITION(condition, message)                                                                           \
  {                                                                                                                    \
    if (!(condition)) {                                                                                                \
      throw std::runtime_error(LOCATION() + std::string(": ") + message);                                              \
    }                                                                                                                  \
  }

#define ASSERT_EQUAL(left, right, message)                                                                             \
  {                                                                                                                    \
    std::stringstream msg;                                                                                             \
    msg << message << " | expected: " << right << " - actual: " << left;                                               \
    ASSERT_CONDITION(left == right, msg.str())                                                                         \
  }

void fillExampleMCCollection(ExampleMCCollection& collection) {
  for (int i = 0; i < 10; ++i) {
    MutableExampleMC mcp;
    mcp.PDG(i);
    collection.push_back(mcp);
  }

  // only daughters
  auto mcp = collection[0];
  mcp.adddaughters(collection[1]);
  mcp.adddaughters(collection[5]);
  mcp.adddaughters(collection[4]);

  // daughters and parents
  mcp = collection[2];
  mcp.adddaughters(collection[1]);
  mcp.adddaughters(collection[9]);
  mcp.addparents(collection[8]);
  mcp.addparents(collection[6]);

  // realistic case with possibly cyclical references
  mcp = collection[6];
  mcp.adddaughters(collection[2]);
  mcp.addparents(collection[3]);
  collection[3].adddaughters(collection[6]);
}

// TODO: break up into smaller cases
void doTestExampleMC(ExampleMCCollection const& collection) {
  // Empty
  ASSERT_CONDITION(collection[7].daughters().empty() && collection[7].parents().empty(),
                   "RelationRange of empty collection is not empty")
  // Equivalent but potentially quicker way of checking an empty collection
  ASSERT_CONDITION(collection[7].daughters().empty() && collection[7].parents().empty(),
                   "RelationRange of empty collection is not empty")

  // alternatively check if a loop is entered
  for (const auto& p [[maybe_unused]] : collection[7].daughters()) {
    throw std::runtime_error("Range based for loop entered on a supposedly empty range");
  }

  ASSERT_EQUAL(collection[0].daughters().size(), 3, "Range has wrong size")
  // check daughter relations are OK
  std::vector<int> expectedPDG = {1, 5, 4};
  int index = 0;
  for (const auto& p : collection[0].daughters()) {
    ASSERT_EQUAL(p.PDG(), expectedPDG[index], "ExampleMC daughters range points to wrong particle (by PDG)")
    index++;
  }

  // Check indexed access
  const auto daughters = collection[0].daughters();
  for (size_t i = 0; i < expectedPDG.size(); ++i) {
    const auto daughter = daughters[i];
    ASSERT_EQUAL(daughter.PDG(), expectedPDG[i], "ExampleMC daughter points to the wrong particle (by PDG)")
  }

  // mothers and daughters
  ASSERT_EQUAL(collection[2].daughters().size(), 2, "Range has wrong size")
  ASSERT_EQUAL(collection[2].parents().size(), 2, "Range has wrong size")

  expectedPDG = {1, 9};
  index = 0;
  for (const auto& p : collection[2].daughters()) {
    ASSERT_EQUAL(p.PDG(), expectedPDG[index], "ExampleMC daughters range points to wrong particle (by PDG)")
    index++;
  }
  expectedPDG = {8, 6};
  index = 0;
  for (const auto& p : collection[2].parents()) {
    ASSERT_EQUAL(p.PDG(), expectedPDG[index], "ExampleMC parents range points to wrong particle (by PDG)")
    index++;
  }

  // Indexed access with range check
  const auto parents = collection[2].parents();
  for (size_t i = 0; i < expectedPDG.size(); ++i) {
    const auto parent = parents.at(i);
    ASSERT_EQUAL(parent.PDG(), expectedPDG[i], "ExampleMC parents points to the wrong particle (by PDG)")
  }

  try {
    const auto parent [[maybe_unused]] = parents.at(3);
    throw std::runtime_error("Trying to access out of bounds in a RelationRange::at should throw");
  } catch (const std::out_of_range& err) {
    ASSERT_EQUAL(err.what(), std::string("index out of bounds for RelationRange"),
                 "Access out of bounds throws wrong exception")
  }

  // realistic case
  auto mcp6 = collection[6];
  ASSERT_EQUAL(mcp6.daughters().size(), 1, "Wrong number of daughters")
  auto parent = mcp6.parents(0);
  ASSERT_EQUAL(parent.daughters().size(), 1, "Wrong number of daughters")

  for (const auto& p : mcp6.parents()) {
    // loop will only run once as per the above assertion
    ASSERT_EQUAL(p, parent, "parent-daughter relation is not as expected")
  }
  for (const auto& p : parent.daughters()) {
    // loop will only run once as per the above assertion
    ASSERT_EQUAL(p, mcp6, "daughter-parent relation is not as expected")
  }
}

void testExampleMC() {
  ExampleMCCollection mcps;
  fillExampleMCCollection(mcps);
  doTestExampleMC(mcps);
}

void testExampleWithVectorMember() {
  MutableExampleWithVectorMember ex;
  ex.addcount(1);
  ex.addcount(2);
  ex.addcount(10);

  ASSERT_EQUAL(ex.count().size(), 3, "vector member range has wrong size")

  std::vector<int> expected = {1, 2, 10};
  int index = 0;
  for (const int c : ex.count()) {
    ASSERT_EQUAL(c, expected[index], "wrong content in range-based for loop")
    index++;
  }
}

void testExampleReferencingType() {
  MutableExampleReferencingType ex;
  MutableExampleReferencingType ex1;
  MutableExampleReferencingType ex2;

  ex.addRefs(ex1);
  ex.addRefs(ex2);

  ASSERT_EQUAL(ex.Refs().size(), 2, "Wrong number of references")

  int index = 0;
  for (const auto& e : ex.Refs()) {
    if (index == 0) {
      ASSERT_EQUAL(e, ex1, "First element of range is not as expected")
    } else {
      ASSERT_EQUAL(e, ex2, "Second element of range is not as expected")
    }

    index++;
  }

  ASSERT_CONDITION(!ex.Refs().empty(), "Relation range of element with relations should not be empty")
  ASSERT_CONDITION(ex1.Refs().empty(), "Relation range of element with no relations should be empty")
}

void testWithIO() {
  podio::ROOTWriter writer("relation_range_io_test.root");

  for (int i = 0; i < 10; ++i) {
    auto collection = ExampleMCCollection();
    fillExampleMCCollection(collection);
    doTestExampleMC(collection);
    auto event = podio::Frame();
    event.put(std::move(collection), "mcparticles");
    writer.writeFrame(event, podio::Category::Event);
  }
  writer.finish();

  podio::ROOTReader reader;
  reader.openFile("relation_range_io_test.root");

  for (int i = 0; i < 10; ++i) {
    const auto event = podio::Frame(reader.readNextEntry(podio::Category::Event));
    auto& readColl = event.get<ExampleMCCollection>("mcparticles");
    ASSERT_CONDITION(readColl.isValid(), "Collection 'mcparticles' should be present")
    ASSERT_EQUAL(readColl.size(), 10, "'mcparticles' should have 10 entries")

    doTestExampleMC(readColl);
  }
}

int main(int, char**) {
  try {
    testExampleMC();
    testExampleWithVectorMember();
    testExampleReferencingType();
    testWithIO();
  } catch (std::runtime_error const& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
