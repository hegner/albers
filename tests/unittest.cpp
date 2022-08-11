// STL
#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>
#ifdef __has_include
  #if __has_include(<version>)
    #include <version>
  #endif
  #if __has_include(<ranges>)
    #include <ranges>
  #endif
#endif

#include "catch2/catch_test_macros.hpp"

// podio specific includes
#include "podio/EventStore.h"
#include "podio/GenericParameters.h"
#include "podio/podioVersion.h"

// Test data types
#include "datamodel/EventInfoCollection.h"
#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleForCyclicDependency1Collection.h"
#include "datamodel/ExampleForCyclicDependency2Collection.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/ExampleWithOneRelationCollection.h"
#include "datamodel/ExampleWithUserInitCollection.h"
#include "datamodel/ExampleWithVectorMemberCollection.h"
#include "datamodel/MutableExampleWithComponent.h"
#include "podio/UserDataCollection.h"

TEST_CASE("AutoDelete", "[basics][memory-management]") {
  auto store = podio::EventStore();
  auto hit1 = MutableEventInfo();
  auto hit2 = MutableEventInfo();
  auto hit3 = MutableEventInfo();
  auto& coll = store.create<EventInfoCollection>("info");
  coll.push_back(hit1);
  hit3 = hit2;
}

TEST_CASE("Basics", "[basics][memory-management]") {
  auto store = podio::EventStore();
  // Adding
  auto& collection = store.create<ExampleHitCollection>("name");
  auto hit1 = collection.create(0xcaffeeULL, 0., 0., 0., 0.); // initialize w/ value
  auto hit2 = collection.create();                            // default initialize
  hit2.energy(12.5);
  // Retrieving
  const ExampleHitCollection* coll2(nullptr);
  REQUIRE(store.get("name", coll2));
  const ExampleHitCollection* coll3(nullptr);
  REQUIRE_FALSE(store.get("wrongName", coll3));
}

TEST_CASE("Assignment-operator ref count", "[basics][memory-management]") {
  // Make sure that the assignment operator handles the reference count
  // correctly. (Will trigger in an ASan build if it is not the case)
  // See https://github.com/AIDASoft/podio/issues/200
  std::map<int, ExampleHit> hitMap;
  for (int i = 0; i < 10; ++i) {
    auto hit = ExampleHit(0x42ULL, i, i, i, i);
    hitMap[i] = hit;
  }

  // hits should still be valid here
  for (int i = 0; i < 10; ++i) {
    const auto hit = hitMap[i];
    REQUIRE(hit.energy() == i);
  }
}

TEST_CASE("Clearing", "[ASAN-FAIL][THREAD-FAIL][basics][memory-management]") {
  bool success = true;
  auto store = podio::EventStore();
  auto& hits = store.create<ExampleHitCollection>("hits");
  auto& clusters = store.create<ExampleClusterCollection>("clusters");
  auto& oneRels = store.create<ExampleWithOneRelationCollection>("OneRelation");
  auto nevents = unsigned(1000);
  for (unsigned i = 0; i < nevents; ++i) {
    hits.clear();
    clusters.clear();
    auto hit1 = hits.create();
    auto hit2 = ExampleHit();
    hit1.energy(double(i));
    auto cluster = clusters.create();
    cluster.addHits(hit1);
    cluster.addHits(hit2);
    hits.push_back(hit2);
    auto oneRel = MutableExampleWithOneRelation();
    oneRel.cluster(cluster);
    oneRel.cluster(cluster);
    oneRels.push_back(oneRel);
  }
  hits.clear();
  if (hits.size() != 0) {
    success = false;
  }
  REQUIRE(success);
}

TEST_CASE("Cloning", "[basics][memory-management]") {
  bool success = true;
  auto hit = MutableExampleHit();
  hit.energy(30);
  auto hit2 = hit.clone();
  hit2.energy(20);
  if (hit.energy() == hit2.energy()) {
    success = false;
  }
  auto cluster = MutableExampleCluster();
  cluster.addHits(hit);
  auto cluster2 = cluster.clone();
  cluster.addHits(hit2);
  // check that the clone of a const object is mutable
  auto const_hit = ExampleHit();
  auto const_hit_clone = const_hit.clone();
  const_hit_clone.energy(30);
  REQUIRE(success);
}

TEST_CASE("Component", "[basics]") {
  auto info = MutableExampleWithComponent();
  info.component().data.x = 3;
  REQUIRE(3 == info.component().data.x);
}

TEST_CASE("Cyclic", "[LEAK-FAIL][basics][relations][memory-management]") {
  auto start = MutableExampleForCyclicDependency1();
  auto isAvailable = start.ref().isAvailable();
  REQUIRE_FALSE(isAvailable);
  auto end = MutableExampleForCyclicDependency2();
  start.ref(end);
  isAvailable = start.ref().isAvailable();
  REQUIRE(isAvailable);
  end.ref(start);
  REQUIRE(start == end.ref());
  auto end_eq = start.ref();
  auto start_eq = end_eq.ref();
  REQUIRE(start == start_eq);
  REQUIRE(start == start.ref().ref());
}

TEST_CASE("Invalid_refs", "[LEAK-FAIL][basics][relations]") {
  auto store = podio::EventStore();
  auto& hits = store.create<ExampleHitCollection>("hits");
  auto hit1 = hits.create(0xcaffeeULL, 0., 0., 0., 0.);
  auto hit2 = ExampleHit();
  auto& clusters = store.create<ExampleClusterCollection>("clusters");
  auto cluster = clusters.create();
  cluster.addHits(hit1);
  cluster.addHits(hit2);
  REQUIRE_THROWS_AS(clusters.prepareForWrite(), std::runtime_error);
}

TEST_CASE("Looping", "[basics]") {
  auto store = podio::EventStore();
  auto& coll = store.create<ExampleHitCollection>("name");
  auto hit1 = coll.create(0xbadULL, 0., 0., 0., 0.);
  auto hit2 = coll.create(0xcaffeeULL, 1., 1., 1., 1.);
  for (auto&& i : coll) {
    i.energy(42); // make sure that we can indeed change the energy here for
                  // non-const collections
  }
  REQUIRE(hit1.energy() == 42);
  REQUIRE(hit2.energy() == 42);

  for (int i = 0, end = coll.size(); i != end; ++i) {
    coll[i].energy(i); // reset it back to the original value
  }

  REQUIRE(coll[0].energy() == 0);
  REQUIRE(coll[1].energy() == 1);

  auto& constColl = store.get<ExampleHitCollection>("name");
  int index = 0;
  for (auto hit : constColl) {
    auto energy = hit.energy();
    REQUIRE(energy == index++);
  }
}

TEST_CASE("Notebook", "[basics]") {
  bool success = true;
  auto store = podio::EventStore();
  auto& hits = store.create<ExampleHitCollection>("hits");
  for (unsigned i = 0; i < 12; ++i) {
    auto hit = hits.create(0xcaffeeULL, 0., 0., 0., double(i));
  }
  auto energies = hits.energy<10>();
  int index = 0;
  for (auto energy : energies) {
    if (double(index) != energy) {
      success = false;
    }
    ++index;
  }
  REQUIRE(success);
}

TEST_CASE("OneToOneRelations", "[basics][relations]") {
  bool success = true;
  auto cluster = ExampleCluster();
  auto rel = MutableExampleWithOneRelation();
  rel.cluster(cluster);
  REQUIRE(success);
}

TEST_CASE("Podness", "[basics][code-gen]") {
  // fail this already at compile time
  STATIC_REQUIRE(std::is_standard_layout_v<ExampleClusterData>); // Generated data classes do not have standard layout
  STATIC_REQUIRE(std::is_trivially_copyable_v<ExampleClusterData>); // Generated data classes are not trivially copyable
  STATIC_REQUIRE(std::is_standard_layout_v<ExampleHitData>);    // Generated data classes do not have standard layout
  STATIC_REQUIRE(std::is_trivially_copyable_v<ExampleHitData>); // Generated data classes are not trivially copyable
  STATIC_REQUIRE(std::is_standard_layout_v<ExampleWithOneRelationData>); // Generated data classes do not have standard
                                                                         // layout
  STATIC_REQUIRE(std::is_trivially_copyable_v<ExampleWithOneRelationData>); // Generated data classes are not trivially
                                                                            // copyable
}

TEST_CASE("Referencing", "[basics][relations]") {
  auto store = podio::EventStore();
  auto& hits = store.create<ExampleHitCollection>("hits");
  auto hit1 = hits.create(0x42ULL, 0., 0., 0., 0.);
  auto hit2 = hits.create(0x42ULL, 1., 1., 1., 1.);
  auto& clusters = store.create<ExampleClusterCollection>("clusters");
  auto cluster = clusters.create();
  cluster.addHits(hit1);
  cluster.addHits(hit2);
  int index = 0;
  for (auto i = cluster.Hits_begin(), end = cluster.Hits_end(); i != end; ++i) {
    REQUIRE(i->energy() == index);
    ++index;
  }
}

TEST_CASE("VariadicCreate", "[basics]") {
  // Test that objects created via the variadic create template function handle relations correctly
  auto store = podio::EventStore();
  auto& clusters = store.create<ExampleClusterCollection>("clusters");

  auto variadic_cluster = clusters.create(3.14f);
  auto normal_cluster = clusters.create();
  normal_cluster.energy(42);

  variadic_cluster.addClusters(normal_cluster);
  REQUIRE(variadic_cluster.Clusters_size() == 1);
  REQUIRE(variadic_cluster.Clusters(0) == normal_cluster);
}

TEST_CASE("write_buffer", "[basics][io]") {
  auto store = podio::EventStore();
  auto& coll = store.create<ExampleHitCollection>("data");
  auto hit1 = coll.create(0x42ULL, 0., 0., 0., 0.);
  auto hit2 = coll.create(0x42ULL, 1., 1., 1., 1.);
  auto& clusters = store.create<ExampleClusterCollection>("clusters");
  auto cluster = clusters.create();
  // add a few related objects to also exercise relation writing
  cluster.addHits(hit1);
  cluster.addHits(hit2);

  REQUIRE_NOTHROW(clusters.prepareForWrite());
  auto buffers = clusters.getBuffers();
  REQUIRE(buffers.dataAsVector<ExampleClusterData>()->size() == clusters.size());

  // a second call should not crash the whole thing and leave everything untouched
  REQUIRE_NOTHROW(clusters.prepareForWrite());
  REQUIRE(clusters.getBuffers().data == buffers.data);

  auto& ref_coll = store.create<ExampleWithOneRelationCollection>("onerel");
  auto withRef = ref_coll.create();
  REQUIRE_NOTHROW(ref_coll.prepareForWrite());
}

TEST_CASE("thread-safe prepareForWrite", "[basics][multithread]") {
  // setup a collection that we can then prepareForWrite from multiple threads
  constexpr auto nElements = 100u;
  ExampleHitCollection hits{};
  podio::UserDataCollection<std::uint64_t> userInts{};
  for (size_t i = 0; i < nElements; ++i) {
    hits.create(i, i * 0.5, i * 1.5, i * 2.5, 3.14);
    userInts.push_back(i);
  }

  constexpr int nThreads = 10;
  std::vector<std::thread> threads{};
  threads.reserve(nThreads);

  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back([&hits, &userInts]() {
      hits.prepareForWrite();
      userInts.prepareForWrite();
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // NOTE: doing this on a single thread, because getBuffers is not threadsafe
  auto buffers = hits.getBuffers();
  auto* dataVec = buffers.dataAsVector<ExampleHitData>();
  REQUIRE(dataVec->size() == nElements);

  auto intBuffers = userInts.getBuffers();
  auto* intVec = intBuffers.dataAsVector<std::uint64_t>();
  REQUIRE(intVec->size() == nElements);

  size_t i = 0;
  for (const auto& h : *dataVec) {
    REQUIRE(h.energy == 3.14);
    REQUIRE(h.cellID == i);
    REQUIRE(h.x == i * 0.5);
    REQUIRE(h.y == i * 1.5);
    REQUIRE(h.z == i * 2.5);

    REQUIRE((*intVec)[i] == i);

    i++;
  }
}

/*
TEST_CASE("Arrays") {
  auto obj = ExampleWithArray();
  obj.array({1,2,3});
  REQUIRE( obj.array()[0] == 1);
}
*/

TEST_CASE("Extracode", "[basics][code-gen]") {
  auto ev = MutableEventInfo();
  ev.setNumber(42);
  REQUIRE(ev.getNumber() == 42);

  int ia[3] = {1, 2, 3};
  auto simple = SimpleStruct(ia);
  REQUIRE(simple.x == 1);
  REQUIRE(simple.y == 2);
  REQUIRE(simple.z == 3);
}

TEST_CASE("AssociativeContainer", "[basics]") {
  auto clu1 = ExampleCluster();
  auto clu2 = ExampleCluster();
  auto clu3 = ExampleCluster();
  auto clu4 = ExampleCluster();
  auto clu5 = ExampleCluster();

  std::set<ExampleCluster> cSet;
  cSet.insert(clu1);
  cSet.insert(clu2);
  cSet.insert(clu3);
  cSet.insert(clu4);
  cSet.insert(clu5);
  cSet.insert(clu1);
  cSet.insert(clu2);
  cSet.insert(clu3);
  cSet.insert(clu4);
  cSet.insert(clu5);

  REQUIRE(cSet.size() == 5);

  std::map<ExampleCluster, int> cMap;
  cMap[clu1] = 1;
  cMap[clu2] = 2;
  cMap[clu3] = 3;
  cMap[clu4] = 4;
  cMap[clu5] = 5;

  REQUIRE(cMap[clu3] == 3);

  cMap[clu3] = 42;

  REQUIRE(cMap[clu3] == 42);
}

TEST_CASE("Equality", "[basics]") {
  auto cluster = ExampleCluster();
  auto rel = MutableExampleWithOneRelation();
  rel.cluster(cluster);
  auto returned_cluster = rel.cluster();
  REQUIRE(cluster == returned_cluster);
  REQUIRE(returned_cluster == cluster);
}

TEST_CASE("UserInitialization", "[basics][code-gen]") {
  ExampleWithUserInitCollection coll;
  // Default initialization values should work even through the create factory
  auto elem = coll.create();
  REQUIRE(elem.i16Val() == 42);
  REQUIRE(elem.floats()[0] == 3.14f);
  REQUIRE(elem.floats()[1] == 1.23f);
  REQUIRE(elem.s().x == 10);
  REQUIRE(elem.s().y == 11);
  REQUIRE(elem.d() == 9.876e5);
  REQUIRE(elem.comp().i == 42);
  REQUIRE(elem.comp().arr[0] == 1.2);
  REQUIRE(elem.comp().arr[1] == 3.4);

  // And obviously when initialized directly
  auto ex = ExampleWithUserInit{};
  REQUIRE(ex.i16Val() == 42);
  REQUIRE(ex.floats()[0] == 3.14f);
  REQUIRE(ex.floats()[1] == 1.23f);
  REQUIRE(ex.s().x == 10);
  REQUIRE(ex.s().y == 11);
  REQUIRE(ex.d() == 9.876e5);
  REQUIRE(ex.comp().i == 42);
  REQUIRE(ex.comp().arr[0] == 1.2);
  REQUIRE(ex.comp().arr[1] == 3.4);
}

TEST_CASE("NonPresentCollection", "[basics][event-store]") {
  auto store = podio::EventStore();
  REQUIRE_THROWS_AS(store.get<ExampleHitCollection>("NonPresentCollection"), std::runtime_error);
}

TEST_CASE("const correct indexed access to const collections", "[const-correctness]") {
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const ExampleClusterCollection>()[0]),
                                ExampleCluster>); // const collections should only have indexed access to mutable
                                                  // objects
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const ExampleClusterCollection>().at(0)),
                                ExampleCluster>); // const collections should only have indexed access to mutable
                                                  // objects
}

TEST_CASE("const correct indexed access to collections", "[const-correctness]") {
  auto store = podio::EventStore();
  auto& collection = store.create<ExampleHitCollection>("irrelevant name");

  STATIC_REQUIRE(std::is_same_v<decltype(collection), ExampleHitCollection&>); // collection created by store should not
                                                                               // be const

  STATIC_REQUIRE(std::is_same_v<decltype(collection[0]), MutableExampleHit>); // non-const collections should have
                                                                              // indexed access to mutable objects

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleClusterCollection>()[0]),
                                MutableExampleCluster>); // collections should have indexed access to mutable objects

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleClusterCollection>().at(0)),
                                MutableExampleCluster>); // collections should have indexed access to mutable objects
}

TEST_CASE("const correct iterators on const collections", "[const-correctness]") {
  const auto collection = ExampleHitCollection();
  // this essentially checks the whole "chain" from begin() / end() through
  // iterator operators
  for (auto hit : collection) {
    STATIC_REQUIRE(std::is_same_v<decltype(hit), ExampleHit>); // const collection iterators should only return
                                                               // immutable objects
  }

  // but we can exercise it in a detailed fashion as well to make it easier to
  // spot where things fail, should they fail
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const ExampleHitCollection>().begin()),
                                ExampleHitCollectionIterator>); // const collection begin() should return a
                                                                // CollectionIterator

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const ExampleHitCollection>().end()),
                                ExampleHitCollectionIterator>); // const collection end() should return a
                                                                // CollectionIterator

  STATIC_REQUIRE(std::is_same_v<decltype(*std::declval<const ExampleHitCollection>().begin()),
                                ExampleHit>); // CollectionIterator should only give access to immutable objects

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleHitCollectionIterator>().operator->()),
                                ExampleHit*>); // CollectionIterator should only give access to immutable objects
}

TEST_CASE("const correct iterators on collections", "[const-correctness]") {
  auto collection = ExampleClusterCollection();
  for (auto cluster : collection) {
    STATIC_REQUIRE(std::is_same_v<decltype(cluster), MutableExampleCluster>); // collection iterators should return
                                                                              // mutable objects
    cluster.energy(42);                                                       // this will necessarily also compile
  }

  // check the individual steps again from above, to see where things fail if they fail
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleClusterCollection>().end()),
                                ExampleClusterMutableCollectionIterator>); // non const collection end() should return a
                                                                           // MutableCollectionIterator

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleClusterCollection>().end()),
                                ExampleClusterMutableCollectionIterator>); // non const collection end() should return a
                                                                           // MutableCollectionIterator

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleClusterCollection>().end()),
                                ExampleClusterMutableCollectionIterator>); // collection end() should return a
                                                                           // MutableCollectionIterator

  STATIC_REQUIRE(std::is_same_v<decltype(*std::declval<ExampleClusterCollection>().begin()),
                                MutableExampleCluster>); // MutableCollectionIterator should give access to mutable
                                                         // objects

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleClusterMutableCollectionIterator>().operator->()),
                                MutableExampleCluster*>); // CollectionIterator should only give access to mutable
                                                          // objects
}

TEST_CASE("Iterator dereferencing", "[!shouldfail][iterators]") {
  // a collection to play with
  auto collection = ExampleHitCollection();

  // NOTE: the following unit tests will fail because even the mutable iterator
  // does not return an lvalue on dereference that writes to the collection: i.e.
  // for (auto i = collection.begin(); i != collection.end(); i++) {
  //   *i = MutableExampleHit(0x42ULL, 0., 0., 0., 0.);
  // }
  // This does not write into the collection, but just to a temporary copy.

  // fill from mutable
  collection.clear();
  collection.create();
  auto mutable_hit = MutableExampleHit(0x42ULL, 0., 0., 0., 0.);
  std::fill(collection.begin(), collection.end(), mutable_hit);
  REQUIRE(collection.begin()->cellID() == 0x42ULL);
}

TEST_CASE("Iterator concepts", "[iterator-concepts][iterators]") {
  // a collection to play with
  auto collection = ExampleHitCollection();

  // input iterator traits
  STATIC_REQUIRE(
      std::is_same_v<std::iterator_traits<decltype(collection)::iterator>::iterator_category, std::input_iterator_tag>);
  STATIC_REQUIRE(std::is_same_v<std::iterator_traits<decltype(collection)::const_iterator>::iterator_category,
                                std::input_iterator_tag>);
  STATIC_REQUIRE(std::is_same_v<std::iterator_traits<decltype(collection)::reverse_iterator>::iterator_category,
                                std::input_iterator_tag>);
  STATIC_REQUIRE(std::is_same_v<std::iterator_traits<decltype(collection)::const_reverse_iterator>::iterator_category,
                                std::input_iterator_tag>);

  STATIC_REQUIRE_FALSE(std::is_same_v<std::iterator_traits<decltype(collection)::iterator>::iterator_category,
                                      std::forward_iterator_tag>);
  STATIC_REQUIRE_FALSE(std::is_same_v<std::iterator_traits<decltype(collection)::const_iterator>::iterator_category,
                                      std::forward_iterator_tag>);
  STATIC_REQUIRE_FALSE(std::is_same_v<std::iterator_traits<decltype(collection)::reverse_iterator>::iterator_category,
                                      std::forward_iterator_tag>);
  STATIC_REQUIRE_FALSE(
      std::is_same_v<std::iterator_traits<decltype(collection)::const_reverse_iterator>::iterator_category,
                     std::forward_iterator_tag>);

#if __cpp_concepts
  // input iterator concepts (C++20)
  STATIC_REQUIRE(std::input_iterator<decltype(collection)::iterator>);
  STATIC_REQUIRE(std::input_iterator<decltype(collection)::const_iterator>);
  STATIC_REQUIRE(std::input_iterator<decltype(collection)::reverse_iterator>);
  STATIC_REQUIRE(std::input_iterator<decltype(collection)::const_reverse_iterator>);
  // but not foward iterator concept
  STATIC_REQUIRE_FALSE(std::forward_iterator<decltype(collection)::iterator>);
  STATIC_REQUIRE_FALSE(std::forward_iterator<decltype(collection)::const_iterator>);
  STATIC_REQUIRE_FALSE(std::forward_iterator<decltype(collection)::reverse_iterator>);
  STATIC_REQUIRE_FALSE(std::forward_iterator<decltype(collection)::const_reverse_iterator>);
#endif

  // iterator loops
  auto cellID = 0;
  collection.clear();
  collection.create();
  collection.create(0x42ULL, 0., 0., 0., 0.);
  // forward loop with post-increment
  for (auto c = collection.begin(); c != collection.end(); c++) {
    STATIC_REQUIRE(std::is_same_v<decltype(c), ExampleHitMutableCollectionIterator>);
    cellID = c->cellID();
  }
  REQUIRE(cellID == 0x42ULL);
  // reverse const loop with pre-increment
  for (auto c = collection.crbegin(); c != collection.crend(); ++c) {
    STATIC_REQUIRE(std::is_same_v<decltype(c), ExampleHitCollectionReverseIterator>);
    cellID = c->cellID();
  }
  REQUIRE(cellID == 0);
  // std::for_each loop with reference capture
  std::for_each(collection.begin(), collection.end(), [&cellID](const auto& c) {
    STATIC_REQUIRE(std::is_same_v<decltype(c), const MutableExampleHit&>);
    cellID = c.cellID();
  });
  REQUIRE(cellID == 0x42ULL);

  // find in collection
  collection.clear();
  collection.create();
  collection.create(0x42ULL, 1., 1., 1., 1.);
  // forward find with const iterators
  auto found_const =
      std::find_if(collection.cbegin(), collection.cend(), [](const auto& h) { return h.cellID() == 0x42ULL; });
  STATIC_REQUIRE(std::is_same_v<decltype(found_const), ExampleHitCollectionIterator>);
  REQUIRE(found_const != collection.cend());
  REQUIRE(found_const->cellID() == 0x42ULL);
  // forward find with mutable iterators
  auto found = std::find_if(collection.begin(), collection.end(), [](const auto& h) { return h.cellID() == 0x42ULL; });
  STATIC_REQUIRE(std::is_same_v<decltype(found), ExampleHitMutableCollectionIterator>);
  REQUIRE(found != collection.end());
  REQUIRE(found->cellID() == 0x42ULL);
  // reverse find with mutable iterators
  auto found_reverse =
      std::find_if(collection.rbegin(), collection.rend(), [](const auto& h) { return h.cellID() == 0x42ULL; });
  STATIC_REQUIRE(std::is_same_v<decltype(found_reverse), ExampleHitMutableCollectionReverseIterator>);
  REQUIRE(found_reverse != collection.rend());
  REQUIRE(found_reverse->cellID() == 0x42ULL);
  // forward find that fails
  auto not_found =
      std::find_if(collection.begin(), collection.end(), [](const auto& h) { return h.cellID() == 0x1ULL; });
  REQUIRE(not_found == collection.end());
  // reverse find that fails
  auto not_found_reverse =
      std::find_if(collection.rbegin(), collection.rend(), [](const auto& h) { return h.cellID() == 0x1ULL; });
  REQUIRE(not_found_reverse == collection.rend());
}

#ifdef __cpp_lib_ranges
TEST_CASE("Ranges (C++20)", "[ranges][iterators]") {
  // a collection to play with
  auto collection = ExampleHitCollection();

  STATIC_REQUIRE(std::ranges::input_range<decltype(collection)>);

  // ranged views (C++20)
  auto is_non_zero = [](const auto& p) { return p.cellID() > 0; };
  auto half_energy = [](const auto& p) {
    auto q = p.clone();
    q.energy(q.energy() / 2);
    return q;
  };
  collection.clear();
  collection.create();
  collection.create(0x42ULL, 1., 1., 1., 1.);
  // pipe syntax
  int count_pipe_syntax = 0;
  for (auto c : collection | std::views::filter(is_non_zero)) {
    STATIC_REQUIRE(std::is_same_v<decltype(c), MutableExampleHit>);
    ++count_pipe_syntax;
  }
  REQUIRE(count_pipe_syntax == 1);
  // composing syntax
  auto count_composing = 0;
  for (auto c : std::views::filter(collection, is_non_zero)) {
    STATIC_REQUIRE(std::is_same_v<decltype(c), MutableExampleHit>);
    ++count_composing;
  }
  REQUIRE(count_composing == 1);
  // take_while
  auto count_take_while = 0;
  for (auto c : collection | std::views::take_while(is_non_zero)) {
    STATIC_REQUIRE(std::is_same_v<decltype(c), MutableExampleHit>);
    ++count_take_while;
  }
  REQUIRE(count_take_while == 0);
  // ref_view
  const std::ranges::ref_view collection_ref_view{collection};
  for (auto c : collection_ref_view) {
    STATIC_REQUIRE(std::is_same_v<decltype(c), MutableExampleHit>);
  }
  // for_each with pipe syntax
  float energy = 0;
  auto add_to_energy = [&e = energy](const auto& p) { e += p.energy(); };
  std::ranges::for_each(collection | std::views::filter(is_non_zero) | std::views::transform(half_energy),
                        add_to_energy);
  REQUIRE(energy > 0.49);
  REQUIRE(energy < 0.51);
  #ifdef __cpp_lib_ranges_zip
  // zip range view over two different collections (C++23)
  auto collection1 = ExampleHitCollection();
  auto hit1 = collection1.create();
  auto collection2 = ExampleClusterCollection();
  auto cluster1 = collection2.create();
  REQUIRE(collection1.begin().cellID() == 0);
  for (auto c: std::views::zip(collection1, collection2) {
    STATIC_REQUIRE(std::is_same_v<decltype(std::get<0>(c)), MutableExampleHit>);
    STATIC_REQUIRE(std::is_same_v<decltype(std::get<1>(c)), MutableExampleCluster>);
    // set all cellIDs in hit collection to 1
    std::get<0>(c).cellID(1);
  }
  // views are references
  REQUIRE(collection1.begin().cellID() == 1);
  #endif
}
#endif

TEST_CASE("Subset collection basics", "[subset-colls]") {
  auto clusterRefs = ExampleClusterCollection();
  clusterRefs.setSubsetCollection();

  // The following will always be true
  REQUIRE(clusterRefs.isSubsetCollection());
  const auto refCollBuffers = clusterRefs.getBuffers();
  REQUIRE(refCollBuffers.data == nullptr);
  REQUIRE(refCollBuffers.vectorMembers->empty());
  REQUIRE(refCollBuffers.references->size() == 1u);
}

TEST_CASE("Subset collection can handle subsets", "[subset-colls]") {
  // Can only collect things that already live in a different colection
  auto clusters = ExampleClusterCollection();
  auto cluster = clusters.create();

  auto clusterRefs = ExampleClusterCollection();
  clusterRefs.setSubsetCollection();
  clusterRefs.push_back(cluster);

  auto clusterRef = clusterRefs[0];
  STATIC_REQUIRE(std::is_same_v<decltype(clusterRef), decltype(cluster)>); // Elements that can be obtained from a
                                                                           // collection and a subset collection should
                                                                           // have the same type

  REQUIRE(clusterRef == cluster);

  // These are "true" subsets, so changes should propagate
  cluster.energy(42);
  REQUIRE(clusterRef.energy() == 42);
  // Also in the other directon
  clusterRef.energy(-42);
  REQUIRE(cluster.energy() == -42);

  clusters.setID(42);
  for (auto c : clusters) {
    REQUIRE(c.getObjectID().collectionID == 42);
  }

  // Setting the ID on a subset collection should not change the IDs of the
  // reference objects as otherwise we cannot use them in I/O
  clusterRefs.setID(314);
  REQUIRE(clusterRefs.getID() == 314);
  for (auto c : clusterRefs) {
    REQUIRE(c.getObjectID().collectionID == 42);
  }
}

TEST_CASE("Collection iterators work with subset collections", "[LEAK-FAIL][subset-colls]") {
  auto hits = ExampleHitCollection();
  auto hit1 = hits.create(0x42ULL, 0., 0., 0., 0.);
  auto hit2 = hits.create(0x42ULL, 1., 1., 1., 1.);

  auto hitRefs = ExampleHitCollection();
  hitRefs.setSubsetCollection();
  for (const auto h : hits) {
    hitRefs.push_back(h);
  }

  // index-based looping / access
  for (size_t i = 0; i < hitRefs.size(); ++i) {
    REQUIRE(hitRefs[i].energy() == i);
  }

  // range-based for loop
  int index = 0;
  for (const auto h : hitRefs) {
    REQUIRE(h.energy() == index++);
  }
}

TEST_CASE("Canont convert a normal collection into a subset collection", "[subset-colls]") {
  auto clusterRefs = ExampleClusterCollection();
  auto cluster = clusterRefs.create();

  REQUIRE_THROWS_AS(clusterRefs.setSubsetCollection(), std::logic_error);
}

TEST_CASE("Cannot convert a subset collection into a normal collection", "[subset-colls]") {
  auto clusterRefs = ExampleClusterCollection();
  clusterRefs.setSubsetCollection();

  auto clusters = ExampleClusterCollection();
  auto cluster = clusters.create();
  clusterRefs.push_back(cluster);

  REQUIRE_THROWS_AS(clusterRefs.setSubsetCollection(false), std::logic_error);
}

TEST_CASE("Subset collection only handles tracked objects", "[subset-colls]") {
  auto clusterRefs = ExampleClusterCollection();
  clusterRefs.setSubsetCollection();
  auto cluster = ExampleCluster();

  REQUIRE_THROWS_AS(clusterRefs.push_back(cluster), std::invalid_argument);
  REQUIRE_THROWS_AS(clusterRefs.create(), std::logic_error);
}

// Helper functionality to keep the tests below with a common setup a bit shorter
auto createCollections(const size_t nElements = 3u) {
  auto colls = std::make_tuple(ExampleHitCollection(), ExampleClusterCollection(), ExampleWithVectorMemberCollection(),
                               podio::UserDataCollection<float>());
  auto& [hitColl, clusterColl, vecMemColl, userDataColl] = colls;

  for (auto i = 0u; i < nElements; ++i) {
    auto hit = hitColl.create();
    hit.cellID(i);
    hit.energy(100.f * i);
    auto cluster = clusterColl.create();
    // create a few relations as well
    cluster.addHits(hit);

    auto vecMem = vecMemColl.create();
    vecMem.addcount(i);
    vecMem.addcount(42 + i);

    userDataColl.push_back(3.14f * i);
  }

  return colls;
}

// Helper functionality to keep the tests below with a common setup a bit shorter
void checkCollections(/*const*/ ExampleHitCollection& hits, /*const*/ ExampleClusterCollection& clusters,
                      /*const*/ ExampleWithVectorMemberCollection& vectors,
                      /*const*/ podio::UserDataCollection<float>& userData, const size_t nElements = 3u) {
  // Basics
  REQUIRE(hits.size() == nElements);
  REQUIRE(clusters.size() == nElements);
  REQUIRE(vectors.size() == nElements);
  REQUIRE(userData.size() == nElements);

  int i = 0;
  for (auto cluster : clusters) {
    REQUIRE(cluster.Hits(0) == hits[i++]);
  }

  i = 0;
  for (const auto vec : vectors) {
    const auto counts = vec.count();
    REQUIRE(counts.size() == 2);
    REQUIRE(counts[0] == i);
    REQUIRE(counts[1] == i + 42);
    i++;
  }

  i = 0;
  for (const auto v : userData) {
    REQUIRE(v == 3.14f * i++);
  }

  // Also check that the buffers we get are valid and have the expected contents
  hits.prepareForWrite();
  auto hitBuffers = hits.getBuffers();
  if (!hits.isSubsetCollection()) {
    auto* hitPODBuffers = hitBuffers.dataAsVector<ExampleHitData>();
    REQUIRE(hitPODBuffers->size() == nElements);
    for (size_t iHit = 0; iHit < nElements; ++iHit) {
      const auto& hitPOD = (*hitPODBuffers)[iHit];
      REQUIRE(hitPOD.cellID == static_cast<unsigned long long>(iHit));
      REQUIRE(hitPOD.energy == 100.f * iHit);
    }
  }

  clusters.prepareForWrite();
  auto clusterBuffers = clusters.getBuffers();
  auto* clusterRelationBuffers = clusterBuffers.references;

  if (!clusters.isSubsetCollection()) {
    REQUIRE(clusterRelationBuffers->size() == 2);                 // We have two relations, to hits and to clusters
    const auto& hitRelationBuffer = (*clusterRelationBuffers)[0]; // Index is an implementation detail
    REQUIRE(hitRelationBuffer->size() == nElements * 1);          // Each cluster has one hit in these tests
    for (size_t iHit = 0; iHit < nElements * 1; ++iHit) {
      const auto& hitID = (*hitRelationBuffer)[iHit];
      REQUIRE(hitID.index == (int)iHit);
      REQUIRE(static_cast<unsigned>(hitID.collectionID) == hits.getID());
    }
  }

  vectors.prepareForWrite();
  auto vecMemBuffers = vectors.getBuffers();
  if (!vectors.isSubsetCollection()) {
    auto* vecMemVecBuffers = vecMemBuffers.vectorMembers;
    REQUIRE(vecMemVecBuffers->size() == 1); // Only one vector member here
    // Now we are really descending into implementation details
    const auto* countBuffer = *static_cast<std::vector<int>**>((*vecMemVecBuffers)[0].second);
    REQUIRE(countBuffer->size() == nElements * 2);
    for (int iC = 0; iC < (int)nElements; ++iC) {
      REQUIRE((*countBuffer)[iC * 2] == iC);
      REQUIRE((*countBuffer)[iC * 2 + 1] == 42 + iC);
    }
  }
}

TEST_CASE("Move-only collections", "[collections][move-semantics]") {
  // Setup a few collections that will be used throughout below
  auto [hitColl, clusterColl, vecMemColl, userDataColl] = createCollections();

  // Hopefully redundant check for setup
  checkCollections(hitColl, clusterColl, vecMemColl, userDataColl);

  SECTION("Move constructor") {
    // Move-construct collections and make sure the size is as expected
    auto newHits = std::move(hitColl);
    auto newClusters = std::move(clusterColl);
    auto newVecMems = std::move(vecMemColl);
    auto newUserData = std::move(userDataColl);

    checkCollections(newHits, newClusters, newVecMems, newUserData);
  }

  SECTION("Move assignment") {
    // Move assign collections and make sure everything is as expected
    auto newHits = ExampleHitCollection();
    newHits = std::move(hitColl);

    auto newClusters = ExampleClusterCollection();
    newClusters = std::move(clusterColl);

    auto newVecMems = ExampleWithVectorMemberCollection();
    newVecMems = std::move(vecMemColl);

    auto newUserData = podio::UserDataCollection<float>();
    newUserData = std::move(userDataColl);

    checkCollections(newHits, newClusters, newVecMems, newUserData);
  }

  SECTION("Prepared collections can be move constructed") {
    hitColl.prepareForWrite();
    auto newHits = std::move(hitColl);

    clusterColl.prepareForWrite();
    auto newClusters = std::move(clusterColl);

    vecMemColl.prepareForWrite();
    auto newVecMems = std::move(vecMemColl);

    userDataColl.prepareForWrite();
    auto newUserData = std::move(userDataColl);

    checkCollections(newHits, newClusters, newVecMems, newUserData);
  }

  SECTION("Prepared collections can be move assigned") {
    hitColl.prepareForWrite();
    clusterColl.prepareForWrite();
    vecMemColl.prepareForWrite();

    auto newHits = ExampleHitCollection();
    newHits = std::move(hitColl);

    auto newClusters = ExampleClusterCollection();
    newClusters = std::move(clusterColl);

    auto newVecMems = ExampleWithVectorMemberCollection();
    newVecMems = std::move(vecMemColl);

    auto newUserData = podio::UserDataCollection<float>();
    newUserData = std::move(userDataColl);

    checkCollections(newHits, newClusters, newVecMems, newUserData);
  }

  SECTION("Subset collections can be moved") {
    // NOTE: Does not apply to UserDataCollections!
    auto subsetHits = ExampleHitCollection();
    subsetHits.setSubsetCollection();
    for (auto hit : hitColl) {
      subsetHits.push_back(hit);
    }
    checkCollections(subsetHits, clusterColl, vecMemColl, userDataColl);

    auto newSubsetHits = std::move(subsetHits);
    REQUIRE(newSubsetHits.isSubsetCollection());
    checkCollections(newSubsetHits, clusterColl, vecMemColl, userDataColl);

    auto subsetClusters = ExampleClusterCollection();
    subsetClusters.setSubsetCollection();
    for (auto cluster : clusterColl) {
      subsetClusters.push_back(cluster);
    }
    checkCollections(newSubsetHits, subsetClusters, vecMemColl, userDataColl);

    // Test move-assignment here as well
    auto newSubsetClusters = ExampleClusterCollection();
    newSubsetClusters = std::move(subsetClusters);
    REQUIRE(newSubsetClusters.isSubsetCollection());
    checkCollections(newSubsetHits, newSubsetClusters, vecMemColl, userDataColl);

    auto subsetVecs = ExampleWithVectorMemberCollection();
    subsetVecs.setSubsetCollection();
    for (auto vec : vecMemColl) {
      subsetVecs.push_back(vec);
    }
    checkCollections(newSubsetHits, newSubsetClusters, subsetVecs, userDataColl);

    auto newSubsetVecs = std::move(subsetVecs);
    REQUIRE(newSubsetVecs.isSubsetCollection());
    checkCollections(hitColl, clusterColl, newSubsetVecs, userDataColl);
  }
}

TEST_CASE("Version tests", "[versioning]") {
  using namespace podio::version;
  // all of these comparisons should be possible at compile time -> STATIC_REQUIRE

  // major version checks
  constexpr Version ver_1{1};
  constexpr Version ver_2{2};
  constexpr Version ver_1_1{1, 1};
  constexpr Version ver_2_1{2, 1};
  constexpr Version ver_1_1_1{1, 1, 1};
  constexpr Version ver_1_0_2{1, 0, 2};
  constexpr Version ver_2_0_2{2, 0, 2};

  SECTION("Equality") {
    STATIC_REQUIRE(ver_1 == Version{1, 0, 0});
    STATIC_REQUIRE(ver_1 != ver_2);
    STATIC_REQUIRE(ver_1_1_1 == Version{1, 1, 1});
    STATIC_REQUIRE(ver_2_1 != ver_1_1);
    STATIC_REQUIRE(ver_1_0_2 != ver_2_0_2);
  }

  SECTION("Major version") {
    STATIC_REQUIRE(ver_1 < ver_2);
    STATIC_REQUIRE(Version{3} > ver_2);
  }

  SECTION("Minor version") {
    STATIC_REQUIRE(ver_1 < ver_1_1);
    STATIC_REQUIRE(ver_2_1 > ver_2);
    STATIC_REQUIRE(ver_1_1 < ver_2);
  }

  SECTION("Patch version") {
    STATIC_REQUIRE(ver_1 < ver_1_0_2);
    STATIC_REQUIRE(ver_1 < ver_1_1_1);
    STATIC_REQUIRE(ver_1_1_1 > ver_1_1);
    STATIC_REQUIRE(ver_2_0_2 < ver_2_1);
  }
}

TEST_CASE("Preprocessor version tests", "[versioning]") {
  SECTION("Basic functionality") {
    using namespace podio::version;
    // Check that preprocessor comparisons work
    STATIC_REQUIRE(PODIO_BUILD_VERSION == PODIO_VERSION(build_version.major, build_version.minor, build_version.patch));

    // Make sure that we can actually decode 64 bit versions
    STATIC_REQUIRE(decode_version(PODIO_BUILD_VERSION) == build_version);

    STATIC_REQUIRE(PODIO_MAJOR_VERSION(PODIO_BUILD_VERSION) == build_version.major);
    STATIC_REQUIRE(PODIO_MINOR_VERSION(PODIO_BUILD_VERSION) == build_version.minor);
    STATIC_REQUIRE(PODIO_PATCH_VERSION(PODIO_BUILD_VERSION) == build_version.patch);

    // Make a few checks where other versions are "maxed out"
    STATIC_REQUIRE(PODIO_MAJOR_VERSION(PODIO_VERSION(10000, 65535, 65535)) == 10000);
    STATIC_REQUIRE(PODIO_MINOR_VERSION(PODIO_VERSION(65535, 20000, 65535)) == 20000);
    STATIC_REQUIRE(PODIO_PATCH_VERSION(PODIO_VERSION(65535, 65535, 30000)) == 30000);
  }

  SECTION("Comparing") {
    // Using some large numbers here to check what happens if we start to
    // actually use the 16 available bits
    // patch version
    STATIC_REQUIRE(PODIO_VERSION(10000, 20000, 39999) < PODIO_VERSION(10000, 20000, 40000));

    // minor version
    STATIC_REQUIRE(PODIO_VERSION(10000, 30000, 33333) > PODIO_VERSION(10000, 29999, 33333));
    STATIC_REQUIRE(PODIO_VERSION(10000, 30000, 33333) < PODIO_VERSION(10000, 30001, 44444));

    // major version
    STATIC_REQUIRE(PODIO_VERSION(20000, 40000, 0) < PODIO_VERSION(20001, 40000, 0));
    STATIC_REQUIRE(PODIO_VERSION(20000, 40000, 10000) < PODIO_VERSION(20001, 30000, 0));
    STATIC_REQUIRE(PODIO_VERSION(20001, 40000, 10000) > PODIO_VERSION(20000, 40000, 20000));
    STATIC_REQUIRE(PODIO_VERSION(20000, 40000, 10000) > PODIO_VERSION(19999, 50000, 30000));
  }
}

TEST_CASE("GenericParameters", "[generic-parameters]") {
  // Check that GenericParameters work as intended
  auto gp = podio::GenericParameters{};

  gp.setValue("anInt", 42);
  REQUIRE(gp.getValue<int>("anInt") == 42);
  // Make sure that resetting a value with the same key works
  gp.setValue("anInt", -42);
  REQUIRE(gp.getValue<int>("anInt") == -42);

  // Make sure that passing a string literal is converted to a string on the fly
  gp.setValue("aString", "const char initialized");
  REQUIRE(gp.getValue<std::string>("aString") == "const char initialized");

  // Check that passing an initializer_list creates the vector on the fly
  gp.setValue("manyInts", {1, 2, 3, 4});
  const auto& ints = gp.getValue<std::vector<int>>("manyInts");
  REQUIRE(ints.size() == 4);
  for (int i = 0; i < 4; ++i) {
    REQUIRE(ints[i] == i + 1);
  }

  auto floats = std::vector<float>{3.14f, 2.718f};
  // This stores a copy of the current value
  gp.setValue("someFloats", floats);
  // Hence, modifying the original vector will not be reflected
  floats.push_back(42.f);
  REQUIRE(floats.size() == 3);

  const auto& storedFloats = gp.getValue<std::vector<float>>("someFloats");
  REQUIRE(storedFloats.size() == 2);
  REQUIRE(storedFloats[0] == 3.14f);
  REQUIRE(storedFloats[1] == 2.718f);

  // We can at this point reset this to a single value with the same key even if
  // it has been a vector before
  gp.setValue("someFloats", 12.34f);
  REQUIRE(gp.getValue<float>("someFloats") == 12.34f);

  // Missing values return the default initialized ones
  REQUIRE(gp.getValue<int>("MissingValue") == int{});
  REQUIRE(gp.getValue<float>("MissingValue") == float{});
  REQUIRE(gp.getValue<std::string>("MissingValue") == std::string{}); // NOLINT(readability-container-size-empty): We
                                                                      // want the explicit comparison here

  // Same for vectors
  REQUIRE(gp.getValue<std::vector<int>>("MissingValue").empty());
  REQUIRE(gp.getValue<std::vector<float>>("MissingValue").empty());
  REQUIRE(gp.getValue<std::vector<std::string>>("MissingValue").empty());
}

// Helper alias template "macro" to get the return type of calling
// GenericParameters::getValue with the desired template type
template <typename T>
using GPGetValue = decltype(std::declval<podio::GenericParameters>().getValue<T>(std::declval<std::string>()));

TEST_CASE("GenericParameters return types", "[generic-parameters][static-checks]") {
  // Tests for checking that the getValue returns return by value resp. by const
  // reference as expected
  STATIC_REQUIRE(std::is_same_v<GPGetValue<int>, int>); // int and float are returned by value
  STATIC_REQUIRE(std::is_same_v<GPGetValue<std::vector<int>>, const std::vector<int>&>); // vectors are const
                                                                                         // references

  STATIC_REQUIRE(std::is_same_v<GPGetValue<std::string>, const std::string&>); // std::strings are returned by const
                                                                               // reference as well
  STATIC_REQUIRE(std::is_same_v<GPGetValue<std::vector<std::string>>, const std::vector<std::string>&>); // as are
                                                                                                         // vectors of
                                                                                                         // strings
}

#ifdef PODIO_JSON_OUTPUT
  #include "nlohmann/json.hpp"

TEST_CASE("JSON", "[json]") {
  const auto& [hitColl, clusterColl, vecMemColl, userDataColl] = createCollections();
  const nlohmann::json json{
      {"clusters", clusterColl}, {"hits", hitColl}, {"vectors", vecMemColl}, {"userData", userDataColl}};

  REQUIRE(json["clusters"].size() == 3);

  int i = 0;
  for (const auto& clu : json["clusters"]) {
    REQUIRE(clu["Hits"][0]["index"] == i++);
  }

  i = 0;
  REQUIRE(json["hits"].size() == 3);
  for (const auto& hit : json["hits"]) {
    REQUIRE(hit["cellID"] == i);
    REQUIRE(hit["energy"] == 100.f * i);
    i++;
  }

  i = 0;
  REQUIRE(json["vectors"].size() == 3);
  for (const auto& vec : json["vectors"]) {
    REQUIRE(vec["count"].size() == 2);
    REQUIRE(vec["count"][0] == i);
    REQUIRE(vec["count"][1] == i + 42);
    i++;
  }

  REQUIRE(json["userData"].size() == 3);
  for (size_t j = 0; j < 3; ++j) {
    REQUIRE(json["userData"][j] == 3.14f * j);
  }
}
#endif
