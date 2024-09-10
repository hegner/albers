#include "podio/Frame.h"

#include "catch2/catch_test_macros.hpp"
#include "datamodel/ExampleCluster.h"
#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/MutableExampleCluster.h"
#include "podio/RelationRange.h"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

TEST_CASE("Frame collections", "[frame][basics]") {
  auto event = podio::Frame();
  auto clusters = ExampleClusterCollection();
  clusters.create(3.14f);
  clusters.create(42.0f);

  event.put(std::move(clusters), "clusters");

  auto& coll = event.get<ExampleClusterCollection>("clusters");
  REQUIRE(coll[0].energy() == 3.14f);
  REQUIRE(coll[1].energy() == 42.0f);

  REQUIRE_FALSE(event.get("non-existant"));
}

TEST_CASE("Frame parameters", "[frame][basics]") {
  auto event = podio::Frame();

  event.putParameter("aString", "from a string literal");
  REQUIRE(event.getParameter<std::string>("aString") == "from a string literal");

  event.putParameter("someInts", {42, 123});
  const auto ints = event.getParameter<std::vector<int>>("someInts").value();
  REQUIRE(ints.size() == 2);
  REQUIRE(ints[0] == 42);
  REQUIRE(ints[1] == 123);

  event.putParameter("someStrings", {"one", "two", "three"});
  const auto strings = event.getParameter<std::vector<std::string>>("someStrings").value();
  REQUIRE(strings.size() == 3);
  REQUIRE(strings[0] == "one");
  REQUIRE(strings[1] == "two");
  REQUIRE(strings[2] == "three");

  const auto stringKeys = event.getParameterKeys<std::string>();
  REQUIRE(stringKeys.size() == 2);
  // Can't rely on an insertion order here
  REQUIRE(std::find(stringKeys.begin(), stringKeys.end(), "aString") != stringKeys.end());
  REQUIRE(std::find(stringKeys.begin(), stringKeys.end(), "someStrings") != stringKeys.end());
}

// NOTE: Due to the extremely small tasks that are done in these tests, they will
// most likely succeed with a very high probability and only running with a
// ThreadSanitizer will detect race conditions, so make sure to have that
// enabled (-DUSE_SANITIZER=Thread) when working on these tests

TEST_CASE("Frame collections multithreaded insert", "[frame][basics][multithread]") {
  constexpr int nThreads = 10;
  std::vector<std::thread> threads;
  threads.reserve(10);

  auto frame = podio::Frame();

  // Fill collections from different threads
  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back([&frame, i]() {
      auto clusters = ExampleClusterCollection();
      clusters.create(i * 3.14);
      clusters.create(i * 3.14);
      frame.put(std::move(clusters), "clusters_" + std::to_string(i));

      auto hits = ExampleHitCollection();
      hits.create(i * 100ULL);
      hits.create(i * 100ULL);
      hits.create(i * 100ULL);
      frame.put(std::move(hits), "hits_" + std::to_string(i));
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Check the frame contents after all threads have finished
  for (int i = 0; i < nThreads; ++i) {
    auto& hits = frame.get<ExampleHitCollection>("hits_" + std::to_string(i));
    REQUIRE(hits.size() == 3);
    for (const auto h : hits) {
      REQUIRE(h.cellID() == i * 100ULL);
    }

    auto& clusters = frame.get<ExampleClusterCollection>("clusters_" + std::to_string(i));
    REQUIRE(clusters.size() == 2);
    for (const auto c : clusters) {
      REQUIRE(c.energy() == i * 3.14);
    }
  }
}

// Helper function to create a frame in the tests below
auto createFrame() {
  auto frame = podio::Frame();

  frame.put(ExampleClusterCollection(), "emptyClusters");

  // Create a few hits inline (to avoid having to have two identifiers)
  auto& hits = frame.put(
      []() {
        auto coll = ExampleHitCollection();
        auto hit = coll.create(0x42ULL, 0., 0., 0., 0.);
        auto hit2 = coll.create(0x123ULL, 1., 1., 1., 1.);
        return coll;
      }(),
      "hits");

  auto clusters = ExampleClusterCollection();
  auto cluster = clusters.create(3.14f);
  cluster.addHits(hits[0]);
  auto cluster2 = clusters.create(42.0f);
  cluster2.addHits(hits[1]);
  cluster2.addClusters(cluster);

  // Create a few clusters inline and relate them to the hits from above
  frame.put(std::move(clusters), "clusters");

  frame.putParameter("anInt", 42);
  frame.putParameter("someFloats", {1.23f, 2.34f, 3.45f});

  return frame;
}

// Helper function to get names easily below
std::string makeName(const std::string& prefix, int index) {
  return prefix + "_" + std::to_string(index);
}

// The Catch2 assertions are not threadsafe
// https://github.com/catchorg/Catch2/blob/devel/docs/limitations.md#thread-safe-assertions
// This is a poor-mans implementation where it is our responsibility to only
// pass in unshared counters
void CHECK_INCREASE(const bool condition, int& counter) {
  if (condition) {
    counter++;
  }
}

TEST_CASE("Frame collections multithreaded insert and read", "[frame][basics][multithread]") {
  constexpr int nThreads = 10;
  std::vector<std::thread> threads;
  threads.reserve(10);

  // create a pre-populated frame
  auto frame = createFrame();

  // The Catch2 assertions are not threadsafe:
  // https://github.com/catchorg/Catch2/blob/devel/docs/limitations.md#thread-safe-assertions
  // Count the successes in this array here and check them outside
  // Once the Catch2 deficiencies are resolved, this can be changed again
  std::array<int, nThreads> successes{};

  // Fill collections from different threads
  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back([&frame, i, &successes]() {
      auto clusters = ExampleClusterCollection();
      clusters.create(i * 3.14);
      clusters.create(i * 3.14);
      frame.put(std::move(clusters), makeName("clusters", i));

      // Retrieve a few collections in between and do just a very basic testing
      auto& existingClu = frame.get<ExampleClusterCollection>("clusters");
      CHECK_INCREASE(existingClu.size() == 2, successes[i]);
      auto& existingHits = frame.get<ExampleHitCollection>("hits");
      CHECK_INCREASE(existingHits.size() == 2, successes[i]);

      auto hits = ExampleHitCollection();
      hits.create(i * 100ULL);
      hits.create(i * 100ULL);
      hits.create(i * 100ULL);
      frame.put(std::move(hits), makeName("hits", i));

      // Fill in a lot of new collections to trigger a rehashing of the
      // internal map, which invalidates iterators
      constexpr int nColls = 100;
      for (int k = 0; k < nColls; ++k) {
        frame.put(ExampleHitCollection(), "h_" + std::to_string(i) + "_" + std::to_string(k));
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Check the frame contents after all threads have finished
  for (int i = 0; i < nThreads; ++i) {
    // Check whether the insertions are as expected
    REQUIRE(successes[i] == 2);

    auto& hits = frame.get<ExampleHitCollection>(makeName("hits", i));
    REQUIRE(hits.size() == 3);
    for (const auto h : hits) {
      REQUIRE(h.cellID() == i * 100ULL);
    }

    auto& clusters = frame.get<ExampleClusterCollection>(makeName("clusters", i));
    REQUIRE(clusters.size() == 2);
    for (const auto c : clusters) {
      REQUIRE(c.energy() == i * 3.14);
    }
  }
}

// Helper function to keep the tests below a bit easier to read and not having
// to repeat this bit all the time. This checks that the contents are the ones
// that would be expected from the createFrame above
void checkFrame(const podio::Frame& frame) {
  auto& hits = frame.get<ExampleHitCollection>("hits");
  REQUIRE(hits.size() == 2);
  REQUIRE(hits[0].energy() == 0);
  REQUIRE(hits[0].cellID() == 0x42ULL);
  REQUIRE(hits[1].energy() == 1);
  REQUIRE(hits[1].cellID() == 0x123ULL);

  REQUIRE(frame.get<ExampleClusterCollection>("emptyClusters").empty());

  auto& clusters = frame.get<ExampleClusterCollection>("clusters");
  REQUIRE(clusters.size() == 2);
  REQUIRE(clusters[0].energy() == 3.14f);
  REQUIRE(clusters[0].Hits().size() == 1);
  REQUIRE(clusters[0].Hits()[0] == hits[0]);
  REQUIRE(clusters[0].Clusters().empty());

  REQUIRE(clusters[1].energy() == 42.f);
  REQUIRE(clusters[1].Hits().size() == 1);
  REQUIRE(clusters[1].Hits()[0] == hits[1]);
  REQUIRE(clusters[1].Clusters()[0] == clusters[0]);

  REQUIRE(frame.getParameter<int>("anInt") == 42);
  const auto floats = frame.getParameter<std::vector<float>>("someFloats").value();
  REQUIRE(floats.size() == 3);
  REQUIRE(floats[0] == 1.23f);
  REQUIRE(floats[1] == 2.34f);
  REQUIRE(floats[2] == 3.45f);
}

TEST_CASE("Frame movability", "[frame][move-semantics]") {
  auto frame = createFrame();
  checkFrame(frame); // just to ensure that the setup is as expected

  SECTION("Move constructor") {
    auto otherFrame = std::move(frame);
    checkFrame(otherFrame);
  }

  SECTION("Move assignment operator") {
    auto otherFrame = podio::Frame();
    otherFrame = std::move(frame);
    checkFrame(otherFrame);
  }

  SECTION("Use after move construction") {
    auto otherFrame = std::move(frame); // NOLINT(clang-analyzer-cplusplus.Move) clang-tidy and the Catch2 sections
                                        // setup do not go along here
    otherFrame.putParameter("aString", "Can add strings after move-constructing");
    REQUIRE(otherFrame.getParameter<std::string>("aString") == "Can add strings after move-constructing");

    otherFrame.put(
        []() {
          auto coll = ExampleHitCollection();
          coll.create();
          coll.create();
          coll.create();
          return coll;
        }(),
        "moreHits");

    auto& hits = otherFrame.get<ExampleHitCollection>("moreHits");
    REQUIRE(hits.size() == 3);
    checkFrame(otherFrame);
  }
}

TEST_CASE("Frame parameters multithread insert", "[frame][basics][multithread]") {
  // Test that parameter access is thread safe
  constexpr int nThreads = 10;
  std::vector<std::thread> threads;
  threads.reserve(nThreads);

  auto frame = podio::Frame();

  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back([&frame, i]() {
      frame.putParameter(makeName("int_par", i), i);

      frame.putParameter(makeName("float_par", i), static_cast<float>(i));

      frame.putParameter(makeName("string_par", i), std::to_string(i));
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  for (int i = 0; i < nThreads; ++i) {
    REQUIRE(frame.getParameter<int>(makeName("int_par", i)) == i);
    REQUIRE(frame.getParameter<float>(makeName("float_par", i)) == static_cast<float>(i));
    REQUIRE(frame.getParameter<std::string>(makeName("string_par", i)) == std::to_string(i));
  }
}

TEST_CASE("Frame parameters multithread insert and read", "[frame][basics][multithread]") {
  constexpr int nThreads = 10;
  std::vector<std::thread> threads;
  threads.reserve(nThreads);

  auto frame = podio::Frame();
  frame.putParameter("int_par", 42);
  frame.putParameter("string_par", "some string");
  frame.putParameter("float_pars", {1.23f, 4.56f, 7.89f});

  // The Catch2 assertions are not threadsafe:
  // https://github.com/catchorg/Catch2/blob/devel/docs/limitations.md#thread-safe-assertions
  // Count the successes in this array here and check them outside
  // Once the Catch2 deficiencies are resolved, this can be changed again
  std::array<int, nThreads> successes{};

  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back([&frame, i, &successes]() {
      frame.putParameter(makeName("int", i), i);
      frame.putParameter(makeName("float", i), static_cast<float>(i));

      CHECK_INCREASE(frame.getParameter<int>("int_par") == 42, successes[i]);
      CHECK_INCREASE(frame.getParameter<float>(makeName("float", i)) == static_cast<float>(i), successes[i]);

      frame.putParameter(makeName("string", i), std::to_string(i));
      CHECK_INCREASE(frame.getParameter<std::string>("string_par") == "some string", successes[i]);

      const auto floatPars = frame.getParameter<std::vector<float>>("float_pars").value();
      CHECK_INCREASE(floatPars.size() == 3, successes[i]);
      CHECK_INCREASE(floatPars[0] == 1.23f, successes[i]);
      CHECK_INCREASE(floatPars[1] == 4.56f, successes[i]);
      CHECK_INCREASE(floatPars[2] == 7.89f, successes[i]);

      // Fill in a lot of new parameters to trigger rehashing of the internal
      // map, which invalidates iterators
      constexpr int nParams = 100;
      for (int k = 0; k < nParams; ++k) {
        frame.putParameter(makeName("intPar", i) + std::to_string(k), i * k);
        frame.putParameter(makeName("floatPar", i) + std::to_string(k), static_cast<float>(i) * k);
        frame.putParameter(makeName("stringPar", i) + std::to_string(k), std::to_string(i * k));
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  for (int i = 0; i < nThreads; ++i) {
    // Check the insertion successes
    REQUIRE(successes[i] == 7);

    REQUIRE(frame.getParameter<int>(makeName("int", i)) == i);
    REQUIRE(frame.getParameter<float>(makeName("float", i)) == static_cast<float>(i));
    REQUIRE(frame.getParameter<std::string>(makeName("string", i)) == std::to_string(i));
  }
}

TEST_CASE("Frame double insert", "[frame][basics]") {
  auto event = podio::Frame();
  auto clusters = ExampleClusterCollection();
  clusters.create(3.14f);
  clusters.create(42.0f);
  auto other_clusters = ExampleClusterCollection();
  other_clusters.create(23.0f);

  event.put(std::move(clusters), "clusters");
  REQUIRE_THROWS_AS(event.put(std::move(other_clusters), "clusters"), std::invalid_argument);
}

TEST_CASE("Frame destructor ASanFail") {
  std::map<std::string, std::pair<ExampleClusterCollection, ExampleHitCollection>> hitClusterMap{};
  podio::Frame frame{};

  // Introduce a scope to mimic the DD4hep structure
  {
    for (const auto& name : {"foo", "bar", "baz"}) {
      auto& hitClusters = hitClusterMap[name];

      for (int i = 0; i < 3; ++i) {
        auto cluster = hitClusters.first->create();

        for (int j = 0; j < 5; ++j) {
          auto hit = hitClusters.second->create();
          cluster.addHits(hit);
        }
      }
    }
  }

  // Introduce a commit scope
  {
    for (auto& [name, collections] : hitClusterMap) {
      frame.put(std::move(collections.first), name + "Clusters");
      frame.put(std::move(collections.second), name + "Hits");
    }
    frame = {};
    hitClusterMap.clear();
  }
}

TEST_CASE("Frame getName", "[frame][basics]") {
  const auto frame = createFrame();

  const auto& hits = frame.get<ExampleHitCollection>("hits");
  REQUIRE(frame.getName(hits).value() == "hits");

  REQUIRE_FALSE(frame.getName(0xfffffff).has_value());
}

TEST_CASE("EIC-Jana2 cleanup use case", "[memory-management][492][174]") {
  // Test case that only triggers in ASan builds if memory-management / cleanup
  // has a bug
  ExampleCluster* clone;
  {
    podio::Frame frame;
    {
      ExampleClusterCollection coll;
      coll.create();
      const ExampleClusterCollection& moved = frame.put(std::move(coll), "mycoll");
      clone = new ExampleCluster(moved[0]);
    }
  }
  delete clone;
}
