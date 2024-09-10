#include "catch2/catch_test_macros.hpp"
#include "datamodel/DatamodelDefinition.h"
#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleClusterCollectionData.h"
#include "datamodel/ExampleClusterData.h"
#include "datamodel/ExampleHitCollectionData.h"
#include "datamodel/ExampleHitData.h"
#include "datamodel/ExampleWithVectorMemberCollectionData.h"
#include "podio/CollectionBufferFactory.h"
#include "podio/CollectionBuffers.h"
#include "podio/ObjectID.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

TEST_CASE("createBuffers", "[internals][memory-management]") {
  const auto& factory = podio::CollectionBufferFactory::instance();

  SECTION("Simple type") {
    auto maybeBuffers = factory.createBuffers("ExampleHitCollection", datamodel::meta::schemaVersion, false);

    REQUIRE(maybeBuffers.has_value());
    auto buffers = maybeBuffers.value();

    // All pointers should be initialized
    REQUIRE(buffers.data);
    REQUIRE(buffers.references);
    REQUIRE(buffers.vectorMembers);

    // Cast this to something useful again
    auto dataBuffers = static_cast<ExampleHitDataContainer*>(buffers.data);
    REQUIRE(dataBuffers->empty());
    REQUIRE(buffers.references->empty());
    REQUIRE(buffers.vectorMembers->empty());

    // Do the necessary cleanup
    delete dataBuffers;
    delete buffers.references;
    delete buffers.vectorMembers;
  }

  SECTION("Type with relations") {
    auto maybeBuffers = factory.createBuffers("ExampleClusterCollection", datamodel::meta::schemaVersion, false);

    REQUIRE(maybeBuffers.has_value());
    auto buffers = maybeBuffers.value();

    // All pointers should be initialized
    REQUIRE(buffers.data);
    REQUIRE(buffers.references);
    REQUIRE(buffers.vectorMembers);

    // Cast this to something useful again
    auto dataBuffers = static_cast<ExampleClusterDataContainer*>(buffers.data);
    REQUIRE(dataBuffers->empty());
    REQUIRE(buffers.references->size() == 2);
    REQUIRE(buffers.vectorMembers->empty());

    // Do the necessary cleanup
    delete dataBuffers;
    delete buffers.references;
    delete buffers.vectorMembers;
  }

  SECTION("Type with vector members") {
    auto maybeBuffers =
        factory.createBuffers("ExampleWithVectorMemberCollection", datamodel::meta::schemaVersion, false);

    REQUIRE(maybeBuffers.has_value());
    auto buffers = maybeBuffers.value();

    // All pointers should be initialized
    REQUIRE(buffers.data);
    REQUIRE(buffers.references);
    REQUIRE(buffers.vectorMembers);

    // Cast this to something useful again
    auto dataBuffers = static_cast<ExampleWithVectorMemberDataContainer*>(buffers.data);
    REQUIRE(dataBuffers->empty());
    REQUIRE(buffers.references->empty());
    REQUIRE(buffers.vectorMembers->size() == 1);

    // Do the necessary cleanup
    delete dataBuffers;
    delete buffers.references;
    auto vecBuffer = static_cast<std::vector<int>*>((*buffers.vectorMembers)[0].second);
    delete vecBuffer;
    delete buffers.vectorMembers;
  }
}

TEST_CASE("construct CollectionData empty buffers", "[internals][memory-management]") {
  const auto& factory = podio::CollectionBufferFactory::instance();

  SECTION("Simple type") {
    auto buffers = factory.createBuffers("ExampleHitCollection", datamodel::meta::schemaVersion, false).value();
    auto collData = ExampleHitCollectionData(std::move(buffers), false);

    // These tests either get flagged by sanitizers or they work
    REQUIRE(true);
  }

  SECTION("Type with relation") {
    auto buffers = factory.createBuffers("ExampleClusterCollection", datamodel::meta::schemaVersion, false).value();
    auto collData = ExampleClusterCollectionData(std::move(buffers), false);

    // These tests either get flagged by sanitizers or they work
    REQUIRE(true);
  }

  SECTION("Type with vector members") {
    auto buffers =
        factory.createBuffers("ExampleWithVectorMemberCollection", datamodel::meta::schemaVersion, false).value();

    auto collData = ExampleWithVectorMemberCollectionData(std::move(buffers), false);

    // These tests either get flagged by sanitizers or they work
    REQUIRE(true);
  }
}

TEST_CASE("construct CollectionData non-empty buffers", "[internals][memory-management]") {
  const auto& factory = podio::CollectionBufferFactory::instance();

  SECTION("Simple type") {
    auto buffers = factory.createBuffers("ExampleHitCollection", datamodel::meta::schemaVersion, false).value();

    // Cast this to something useful again to add one data element
    auto dataBuffers = static_cast<ExampleHitDataContainer*>(buffers.data);
    dataBuffers->emplace_back(ExampleHitData{0xcaffee, 1.0, 2.0, 3.0, 125.0});

    auto collData = ExampleHitCollectionData(std::move(buffers), false);
  }

  SECTION("Type with relations") {
    auto buffers = factory.createBuffers("ExampleClusterCollection", datamodel::meta::schemaVersion, false).value();

    // Cast this to something useful again to add one data element
    auto dataBuffers = static_cast<ExampleClusterDataContainer*>(buffers.data);
    dataBuffers->emplace_back(ExampleClusterData{125.0});

    // Add an ObjectID (we don't need to resolve it in any way)
    (*buffers.references)[0]->emplace_back(podio::ObjectID{42, 42});

    auto collData = ExampleClusterCollectionData(std::move(buffers), false);
  }

  SECTION("Type with vector members") {
    auto buffers =
        factory.createBuffers("ExampleWithVectorMemberCollection", datamodel::meta::schemaVersion, false).value();

    auto vecBuffer = static_cast<std::vector<int>*>((*buffers.vectorMembers)[0].second);
    vecBuffer->emplace_back(42);

    auto collData = ExampleWithVectorMemberCollectionData(std::move(buffers), false);
  }
}
