#ifndef PODIO_TESTS_SCHEMAEVOLUTION_READNEWDATA_H // NOLINT(llvm-header-guard): folder structure not suitable
#define PODIO_TESTS_SCHEMAEVOLUTION_READNEWDATA_H // NOLINT(llvm-header-guard): folder structure not suitable

#include "datamodel/ExampleHitCollection.h"
#include "datamodel/ExampleWithARelationCollection.h"
#include "datamodel/ExampleWithArrayComponentCollection.h"
#include "datamodel/ExampleWithNamespaceCollection.h"
#include "podio/Frame.h"

#include <iostream>
#include <string>

#define ASSERT_EQUAL(actual, expected, msg)                                                                            \
  if ((expected) != (actual)) {                                                                                        \
    std::cerr << __PRETTY_FUNCTION__ << ": " << msg << " (expected: " << expected << ", actual: " << actual << ")";    \
    return 1;                                                                                                          \
  }

int readSimpleStruct(const podio::Frame& event) {
  const auto& coll = event.get<ExampleWithArrayComponentCollection>("simpleStructTest");
  auto elem = coll[0];
  const auto sstruct = elem.s();

  ASSERT_EQUAL(sstruct.y, 0, "New component member not 0 initialized")
  ASSERT_EQUAL(sstruct.x, 42, "Existing component member changed")
  ASSERT_EQUAL(sstruct.z, 123, "Existing component member changed")

  return 0;
}

int readExampleHit(const podio::Frame& event) {
  const auto& coll = event.get<ExampleHitCollection>("datatypeMemberAdditionTest");
  auto elem = coll[0];

  ASSERT_EQUAL(elem.t(), 0, "New datatype member variable not 0 initialized")
  ASSERT_EQUAL(elem.x(), 1.23, "Member variables unrelated to schema evolution have changed")
  ASSERT_EQUAL(elem.y(), 1.23, "Member variables unrelated to schema evolution have changed")
  ASSERT_EQUAL(elem.z(), 1.23, "Member variables unrelated to schema evolution have changed")
  ASSERT_EQUAL(elem.energy(), 0, "Member variables unrelated to schema evolution have changed")
  ASSERT_EQUAL(elem.cellID(), 0xcaffee, "Member variables unrelated to schema evolution have changed")

  return 0;
}

int readExampleWithNamespace(const podio::Frame& event) {
  const auto& coll = event.get<ex42::ExampleWithNamespaceCollection>("componentMemberRenameTest");
  auto elem = coll[0];

  ASSERT_EQUAL(elem.y_new(), 42, "Renamed component member variable does not have the expected value")
  ASSERT_EQUAL(elem.x(), 123, "Member variables unrelated to schema evolution have changed")

  return 0;
}

int readExampleWithARelation(const podio::Frame& event) {
  const auto& coll = event.get<ex42::ExampleWithARelationCollection>("floatToDoubleMemberTest");
  auto elem = coll[0];

  ASSERT_EQUAL(elem.number(), static_cast<double>(3.14f),
               "Conversion from float to double member does not work as expected")

  return 0;
}

template <typename ReaderT>
int read_new_data(const std::string& filename) {
  ReaderT reader{};
  reader.openFile(filename);

  const auto event = podio::Frame(reader.readEntry("events", 0));

  int result = 0;
  result += readSimpleStruct(event);
  result += readExampleHit(event);
  result += readExampleWithNamespace(event);
  result += readExampleWithARelation(event);

  return result;
}

#endif // PODIO_TESTS_SCHEMAEVOLUTION_WRITEOLDDATA_H
