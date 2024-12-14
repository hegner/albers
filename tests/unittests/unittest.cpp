// STL
#include <cstdint>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"
#include "catch2/matchers/catch_matchers_vector.hpp"

// podio specific includes
#include "podio/Frame.h"
#include "podio/GenericParameters.h"
#include "podio/ROOTLegacyReader.h"
#include "podio/ROOTReader.h"
#include "podio/ROOTWriter.h"
#include "podio/podioVersion.h"

#ifndef PODIO_ENABLE_SIO
  #define PODIO_ENABLE_SIO 0
#endif
#if PODIO_ENABLE_SIO
  #include "podio/SIOLegacyReader.h"
  #include "podio/SIOReader.h"
  #include "podio/SIOWriter.h"
#endif

#if PODIO_ENABLE_RNTUPLE
  #include "podio/RNTupleReader.h"
  #include "podio/RNTupleWriter.h"
#endif

// Test data types
#include "datamodel/EventInfoCollection.h"
#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleForCyclicDependency1Collection.h"
#include "datamodel/ExampleForCyclicDependency2Collection.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/ExampleWithArray.h"
#include "datamodel/ExampleWithArrayComponent.h"
#include "datamodel/ExampleWithComponent.h"
#include "datamodel/ExampleWithExternalExtraCode.h"
#include "datamodel/ExampleWithFixedWidthIntegers.h"
#include "datamodel/ExampleWithOneRelationCollection.h"
#include "datamodel/ExampleWithUserInitCollection.h"
#include "datamodel/ExampleWithVectorMemberCollection.h"
#include "datamodel/MutableExampleCluster.h"
#include "datamodel/MutableExampleWithArray.h"
#include "datamodel/MutableExampleWithComponent.h"
#include "datamodel/MutableExampleWithExternalExtraCode.h"
#include "datamodel/StructWithExtraCode.h"

#include "podio/UserDataCollection.h"

TEST_CASE("AutoDelete", "[basics][memory-management]") {
  auto coll = EventInfoCollection();
  auto hit1 = MutableEventInfo();
  auto hit2 = MutableEventInfo();
  auto hit3 = MutableEventInfo();
  coll.push_back(hit1);
  hit3 = hit2;
}

TEST_CASE("Assignment-operator ref count", "[basics][memory-management]") {
  // Make sure that the assignment operator handles the reference count
  // correctly. (Will trigger in an ASan build if it is not the case)
  // See https://github.com/AIDASoft/podio/issues/200
  std::map<int, MutableExampleHit> hitMap;
  for (int i = 0; i < 10; ++i) {
    auto hit = MutableExampleHit(0x42ULL, i, i, i, i);
    hitMap[i] = hit;
  }

  // hits should still be valid here
  for (int i = 0; i < 10; ++i) {
    const auto hit = hitMap[i];
    REQUIRE(hit.energy() == i);
  }
}

TEST_CASE("ostream-operator", "[basics]") {
  // Make sure that trying to print an object that is not available does not crash
  auto hit = ExampleHit::makeEmpty();
  REQUIRE_FALSE(hit.isAvailable());
  std::stringstream sstr;
  sstr << hit;
  REQUIRE(sstr.str() == "[not available]");
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
  auto mutable_hit = MutableExampleHit();
  auto const_hit = ExampleHit(mutable_hit);
  auto const_hit_clone = const_hit.clone();
  const_hit_clone.energy(30);
  REQUIRE(success);
}

TEST_CASE("Component", "[basics]") {
  auto info = MutableExampleWithComponent();
  info.component().data.x = 3;
  REQUIRE(3 == info.component().data.x);
}

TEST_CASE("makeEmpty", "[basics]") {
  auto hit = ExampleHit::makeEmpty();
  // Any access to such a handle is a crash
  REQUIRE_FALSE(hit.isAvailable());

  hit = MutableExampleHit{};
  REQUIRE(hit.isAvailable());
  REQUIRE(hit.energy() == 0);
}

TEST_CASE("Cyclic", "[basics][relations][memory-management]") {
  auto coll1 = ExampleForCyclicDependency1Collection();
  auto start = coll1.create();
  REQUIRE_FALSE(start.ref().isAvailable());
  auto coll2 = ExampleForCyclicDependency2Collection();
  auto end = coll2.create();
  start.ref(end);
  REQUIRE(start.ref().isAvailable());
  end.ref(start);
  REQUIRE(start == end.ref());
  auto end_eq = start.ref();
  auto start_eq = end_eq.ref();
  REQUIRE(start == start_eq);
  REQUIRE(start == start.ref().ref());
}

TEST_CASE("Cyclic w/o collection", "[LEAK-FAIL][basics][relations][memory-management]") {
  auto start = MutableExampleForCyclicDependency1{};
  REQUIRE_FALSE(start.ref().isAvailable());
  auto end = MutableExampleForCyclicDependency2{};
  start.ref(end);
  REQUIRE(start.ref().isAvailable());
  end.ref(start);
  REQUIRE(start == end.ref());
  auto end_eq = start.ref();
  auto start_eq = end_eq.ref();
  REQUIRE(start == start_eq);
  REQUIRE(start == start.ref().ref());
}

TEST_CASE("Container lifetime", "[basics][memory-management]") {
  std::vector<ExampleHit> hits;
  {
    MutableExampleHit hit;
    hit.energy(3.14f);
    hits.push_back(hit); // NOLINT(modernize-use-emplace)
  }
  auto hit = hits[0];
  REQUIRE(hit.energy() == 3.14f);
}

TEST_CASE("Invalid_refs", "[basics][relations]") {
  auto hits = ExampleHitCollection();
  auto hit1 = hits.create(0xcaffeeULL, 0., 0., 0., 0.);
  auto hit2 = ExampleHit();
  auto clusters = ExampleClusterCollection();
  auto cluster = clusters.create();
  cluster.addHits(hit1);
  cluster.addHits(hit2);
  REQUIRE_THROWS_AS(clusters.prepareForWrite(), std::runtime_error);
}

TEST_CASE("Looping", "[basics]") {
  auto coll = ExampleHitCollection();
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

  const auto& constColl = coll;
  int index = 0;
  for (auto hit : constColl) {
    auto energy = hit.energy();
    REQUIRE(energy == index++);
  }
}

TEST_CASE("Reverse iterators", "[basics]") {
  auto coll = ExampleHitCollection();
  coll.create();
  coll.create();
  auto it = std::rbegin(coll);
  (*it).energy(43);
  (*++it).energy(42);
  REQUIRE((*it).energy() == 42);
  REQUIRE((*--it).energy() == 43);
  it = std::rend(coll);
  REQUIRE((*--it).energy() == 42);
  REQUIRE((*--it).energy() == 43);
  auto cit = std::crbegin(coll);
  REQUIRE((*cit).energy() == 43);
  REQUIRE((*++cit).energy() == 42);
  cit = std::crend(coll);
  REQUIRE((*--cit).energy() == 42);
  REQUIRE((*--cit).energy() == 43);
}

TEST_CASE("Notebook", "[basics]") {
  auto hits = ExampleHitCollection();
  for (unsigned i = 0; i < 12; ++i) {
    auto hit = hits.create(0xcaffeeULL, 0., 0., 0., double(i));
  }

  // Request only subset
  auto energies = hits.energy(10);
  REQUIRE(energies.size() == 10);
  int index = 0;
  for (auto energy : energies) {
    REQUIRE(double(index) == energy);
    ++index;
  }

  // Make sure there are no "invented" values
  REQUIRE(hits.energy(24).size() == hits.size());
  REQUIRE(hits.energy().size() == hits.size());
}

TEST_CASE("OneToOneRelations", "[basics][relations]") {
  bool success = true;
  auto cluster = MutableExampleCluster();
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
  auto hits = ExampleHitCollection();
  auto hit1 = hits.create(0x42ULL, 0., 0., 0., 0.);
  auto hit2 = hits.create(0x42ULL, 1., 1., 1., 1.);
  auto clusters = ExampleClusterCollection();
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
  auto clusters = ExampleClusterCollection();

  auto variadic_cluster = clusters.create(3.14f);
  auto normal_cluster = clusters.create();
  normal_cluster.energy(42);

  variadic_cluster.addClusters(normal_cluster);
  REQUIRE(variadic_cluster.Clusters_size() == 1);
  REQUIRE(variadic_cluster.Clusters(0) == normal_cluster);
}

TEST_CASE("write_buffer", "[basics][io]") {
  auto coll = ExampleHitCollection();
  auto hit1 = coll.create(0x42ULL, 0., 0., 0., 0.);
  auto hit2 = coll.create(0x42ULL, 1., 1., 1., 1.);
  auto clusters = ExampleClusterCollection();
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

  auto ref_coll = ExampleWithOneRelationCollection();
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

TEST_CASE("UserDataCollection print", "[basics]") {
  auto coll = podio::UserDataCollection<int32_t>();
  coll.push_back(1);
  coll.push_back(2);
  coll.push_back(3);

  std::stringstream sstr;
  coll.print(sstr);

  REQUIRE(sstr.str() == "[1, 2, 3]");
}

/*
TEST_CASE("Arrays") {
  auto obj = ExampleWithArray();
  obj.array({1,2,3});
  REQUIRE( obj.array()[0] == 1);
}
*/

TEST_CASE("member getter return types", "[basics][code-gen][const-correctness]") {
  // Check that the return types of the getter functions are as expected
  // Builtin member types are returned by value, including fixed width integers
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleHit>().energy()), double>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleWithFixedWidthIntegers>().fixedU64()), std::uint64_t>);
  // Arrays are returned by const reference
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleWithArray>().myArray()), const std::array<int, 4>&>);
  // But if we index into that array we get back a value
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleWithArray>().myArray(0)), int>);
  // Non-builtin member types are returned by const reference
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleWithArrayComponent>().s()), const SimpleStruct&>);
  // Accessing sub members also works as expected: builtin types by value,
  // everything else by const reference
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleWithArrayComponent>().x()), int>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleWithArrayComponent>().p()), const std::array<int, 4>&>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<ExampleWithArray>().data()), const SimpleStruct&>);

  // Mutable types also give access to mutable references
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<MutableExampleCluster>().energy()), double&>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<MutableExampleWithArray>().data()), SimpleStruct&>);
  // However if they are const the usual rules apply
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const MutableExampleCluster>().energy()), double>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const MutableExampleWithArray>().data()), const SimpleStruct&>);
}

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

TEST_CASE("ExtraCode declarationFile and implementationFile", "[basics][code-gen]") {
  auto mutable_number = MutableExampleWithExternalExtraCode();
  REQUIRE(mutable_number.reset() == 0);
  REQUIRE(mutable_number.add(2) == 2);
  REQUIRE(mutable_number.add_inplace(1) == 1);
  REQUIRE(mutable_number.gt(-1));
  REQUIRE(mutable_number.lt(100));
  ExampleWithExternalExtraCode number = mutable_number;
  REQUIRE(number.add(1) == 2);
  REQUIRE(number.gt(-1));
  REQUIRE(number.lt(100));
}

TEST_CASE("ExtraCode declarationFile in component", "[basics][code-gen]") {
  auto value = StructWithExtraCode();
  value.x = 1;
  REQUIRE(value.negate() == -1);
  REQUIRE(value.reset() == 0);
}

TEST_CASE("AssociativeContainer", "[basics]") {
  auto clu1 = MutableExampleCluster();
  auto clu2 = MutableExampleCluster();
  auto clu3 = MutableExampleCluster();
  auto clu4 = MutableExampleCluster();
  auto clu5 = MutableExampleCluster();

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
  auto cluster = MutableExampleCluster();
  auto rel = MutableExampleWithOneRelation();
  rel.cluster(cluster);
  auto returned_cluster = rel.cluster();
  REQUIRE(cluster == returned_cluster);
  REQUIRE(returned_cluster == cluster);

  auto clu = ExampleCluster::makeEmpty();
  auto clu2 = ExampleCluster::makeEmpty();
  // Empty handles always compare equal
  REQUIRE(clu == clu2);
  // They never compare equal to a non-empty handle
  REQUIRE(clu != cluster);
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
  auto ex = MutableExampleWithUserInit{};
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

TEST_CASE("Collection size and empty", "[basics][collections]") {
  ExampleClusterCollection coll{};
  REQUIRE(coll.empty());

  coll.create();
  coll.create();
  REQUIRE(coll.size() == 2u);
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
  auto collection = ExampleHitCollection();

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
  for (auto hit [[maybe_unused]] : collection) {
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
  // Can only collect things that already live in a different collection
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
  // Also in the other direction
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

TEST_CASE("Collection iterators work with subset collections", "[subset-colls]") {
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

TEST_CASE("Cannot convert a normal collection into a subset collection", "[subset-colls]") {
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
  auto cluster = MutableExampleCluster();

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
    cluster.energy(150.f * i);

    auto vecMem = vecMemColl.create();
    vecMem.addcount(i);
    vecMem.addcount(42 + i);

    userDataColl.push_back(3.14f * i);
  }

  vecMemColl.prepareForWrite();
  auto buffers = vecMemColl.getBuffers();
  auto vecBuffers = buffers.vectorMembers;
  auto thisVec = (*vecBuffers)[0].second;

  // const auto floatVec = podio::CollectionWriteBuffers::asVector<float>(thisVec);
  const auto floatVec2 = podio::CollectionReadBuffers::asVector<float>(thisVec);

  // std::cout << floatVec->size() << '\n';
  std::cout << "** " << floatVec2->size() << " vs " << vecMemColl.size() << '\n';

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
      REQUIRE(hitID.index == static_cast<int>(iHit));
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
    for (int iC = 0; iC < static_cast<int>(nElements); ++iC) {
      REQUIRE((*countBuffer)[iC * 2] == iC);
      REQUIRE((*countBuffer)[iC * 2 + 1] == 42 + iC);
    }
  }
}

template <typename>
struct TD;

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

  SECTION("Moved collections can be prepared") {
    auto newHits = std::move(hitColl);
    newHits.prepareForWrite();

    auto newClusters = std::move(clusterColl);
    newClusters.prepareForWrite();

    auto newVecMems = std::move(vecMemColl);
    newVecMems.prepareForWrite();

    auto newUserData = std::move(userDataColl);
    newUserData.prepareForWrite();

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
    // Check that preprocessor comparisons work by actually invoking the
    // preprocessor
#if PODIO_BUILD_VERSION == PODIO_VERSION(podio_VERSION_MAJOR, podio_VERSION_MINOR, podio_VERSION_PATCH)
    STATIC_REQUIRE(true);
#else
    STATIC_REQUIRE(false);
#endif

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

  gp.set("anInt", 42);
  REQUIRE(*gp.get<int>("anInt") == 42);
  // Make sure that resetting a value with the same key works
  gp.set("anInt", -42);
  REQUIRE(*gp.get<int>("anInt") == -42);

  // Make sure that passing a string literal is converted to a string on the fly
  gp.set("aString", "const char initialized");
  REQUIRE(*gp.get<std::string>("aString") == "const char initialized");

  gp.set("aStringVec", {"init", "from", "const", "chars"});
  const auto stringVec = gp.get<std::vector<std::string>>("aStringVec").value();
  REQUIRE(stringVec.size() == 4);
  REQUIRE(stringVec[0] == "init");
  REQUIRE(stringVec[3] == "chars");

  // Check that storing double values works
  gp.set("double", 1.234);
  gp.set("manyDoubles", {1.23, 4.56, 7.89});
  REQUIRE(gp.get<double>("double") == 1.234);
  const auto storedDoubles = gp.get<std::vector<double>>("manyDoubles").value();
  REQUIRE(storedDoubles.size() == 3);
  REQUIRE(storedDoubles[0] == 1.23);
  REQUIRE(storedDoubles[1] == 4.56);
  REQUIRE(storedDoubles[2] == 7.89);

  // Check that passing an initializer_list creates the vector on the fly
  gp.set("manyInts", {1, 2, 3, 4});
  const auto ints = gp.get<std::vector<int>>("manyInts").value();
  REQUIRE(ints.size() == 4);
  for (int i = 0; i < 4; ++i) {
    REQUIRE(ints[i] == i + 1);
  }

  auto floats = std::vector<float>{3.14f, 2.718f};
  // This stores a copy of the current value
  gp.set("someFloats", floats);
  // Hence, modifying the original vector will not be reflected
  floats.push_back(42.f);
  REQUIRE(floats.size() == 3);

  const auto storedFloats = gp.get<std::vector<float>>("someFloats").value();
  REQUIRE(storedFloats.size() == 2);
  REQUIRE(storedFloats[0] == 3.14f);
  REQUIRE(storedFloats[1] == 2.718f);

  // We can at this point reset this to a single value with the same key even if
  // it has been a vector before
  gp.set("someFloats", 12.34f);
  REQUIRE(*gp.get<float>("someFloats") == 12.34f);

  // Missing values return an empty optional
  REQUIRE_FALSE(gp.get<int>("Missing"));
  REQUIRE_FALSE(gp.get<float>("Missing"));
  REQUIRE_FALSE(gp.get<std::string>("Missing"));

  // Same for vectors
  REQUIRE_FALSE(gp.get<std::vector<int>>("Missing"));
  REQUIRE_FALSE(gp.get<std::vector<float>>("Missing"));
  REQUIRE_FALSE(gp.get<std::vector<std::string>>("Missing"));
}

TEST_CASE("GenericParameters empty vector single value access", "[generic-parameters]") {
  auto gp = podio::GenericParameters();
  gp.set("empty-ints", std::vector<int>{});

  // Getting the whole vector works
  const auto maybeVec = gp.get<std::vector<int>>("empty-ints");
  REQUIRE(maybeVec.has_value());
  const auto& vec = maybeVec.value();
  REQUIRE(vec.empty());

  // Trying to get a single (i.e. the first) value will not work
  const auto maybeVal = gp.get<int>("empty-ints");
  REQUIRE_FALSE(maybeVal.has_value());
}

TEST_CASE("GenericParameters constructors", "[generic-parameters]") {
  // Tests for making sure that generic parameters can be moved / copied correctly
  auto originalParams = podio::GenericParameters{};
  originalParams.set("int", 42);
  originalParams.set("ints", {1, 2});
  originalParams.set("float", 3.14f);
  originalParams.set("double", 2 * 3.14);
  originalParams.set("strings", {"one", "two", "three"});

  SECTION("Copy constructor") {
    auto copiedParams = originalParams;
    REQUIRE(*copiedParams.get<int>("int") == 42);
    REQUIRE(copiedParams.get<std::vector<int>>("ints").value()[1] == 2);
    REQUIRE(*copiedParams.get<float>("float") == 3.14f);
    REQUIRE(*copiedParams.get<double>("double") == 2 * 3.14);
    REQUIRE(copiedParams.get<std::vector<std::string>>("strings").value()[0] == "one");

    // Make sure these are truly independent copies now
    copiedParams.set("anotherDouble", 1.2345);
    REQUIRE_FALSE(originalParams.get<double>("anotherDouble"));
  }

  SECTION("Move constructor") {
    auto copiedParams = std::move(originalParams);
    REQUIRE(copiedParams.get<int>("int") == 42);
    REQUIRE(copiedParams.get<std::vector<int>>("ints").value()[1] == 2);
    REQUIRE(copiedParams.get<float>("float") == 3.14f);
    REQUIRE(copiedParams.get<double>("double") == 2 * 3.14);
    REQUIRE(copiedParams.get<std::vector<std::string>>("strings").value()[0] == "one");
  }

  SECTION("Move assignment") {
    auto copiedParams = podio::GenericParameters{};
    copiedParams = std::move(originalParams);
    REQUIRE(copiedParams.get<int>("int") == 42);
    REQUIRE(copiedParams.get<std::vector<int>>("ints").value()[1] == 2);
    REQUIRE(copiedParams.get<float>("float") == 3.14f);
    REQUIRE(copiedParams.get<double>("double") == 2 * 3.14);
    REQUIRE(copiedParams.get<std::vector<std::string>>("strings").value()[0] == "one");
  }
}

TEST_CASE("Missing files (ROOT readers)", "[basics]") {
  auto root_legacy_reader = podio::ROOTLegacyReader();
  REQUIRE_THROWS_AS(root_legacy_reader.openFile("NonExistentFile.root"), std::runtime_error);

  auto root_frame_reader = podio::ROOTReader();
  REQUIRE_THROWS_AS(root_frame_reader.openFile("NonExistentFile.root"), std::runtime_error);
}

#if PODIO_ENABLE_SIO
TEST_CASE("Missing files (SIO readers)", "[basics]") {
  auto sio_legacy_reader = podio::SIOLegacyReader();
  REQUIRE_THROWS_AS(sio_legacy_reader.openFile("NonExistentFile.sio"), std::runtime_error);

  auto sio_frame_reader = podio::SIOReader();
  REQUIRE_THROWS_AS(sio_frame_reader.openFile("NonExistentFile.root"), std::runtime_error);
}
#endif

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

TEST_CASE("subset collection JSON", "[json]") {
  auto clusters = ExampleClusterCollection();
  clusters.setID(42);
  for (int i = 0; i < 5; ++i) {
    clusters.create(i * 1.0);
  }

  auto subsetClusters = ExampleClusterCollection();
  subsetClusters.setID(123);
  subsetClusters.setSubsetCollection();
  for (int i = clusters.size() - 1; i >= 0; i--) {
    subsetClusters.push_back(clusters[i]);
  }

  const nlohmann::json json{{"subset", subsetClusters}};
  REQUIRE(json["subset"].size() == 5);

  for (int i = 0; i < 5; ++i) {
    REQUIRE(json["subset"][i]["collectionID"] == clusters.getID());
    REQUIRE(json["subset"][i]["index"] == 4 - i);
  }
}

#endif

// Write a template function that can be used with different writers in order to
// be able to tag the unittests differently. This is necessary because the
// ROOTWriter fails with ASan, but the ROOTNTuple writer doesn't
template <typename WriterT>
void runConsistentFrameTest(const std::string& filename) {
  using Catch::Matchers::ContainsSubstring;

  podio::Frame frame;

  frame.put(ExampleClusterCollection(), "clusters");
  frame.put(ExampleClusterCollection(), "clusters2");
  frame.put(ExampleHitCollection(), "hits");

  WriterT writer(filename);
  writer.writeFrame(frame, "full");

  // Write a frame with more collections
  frame.put(ExampleHitCollection(), "hits2");
  REQUIRE_THROWS_WITH(writer.writeFrame(frame, "full"),
                      ContainsSubstring("Trying to write category") &&
                          ContainsSubstring("inconsistent collection content") &&
                          ContainsSubstring("superfluous: [hits2]"));

  // Write a frame with less collections
  podio::Frame frame2;
  frame2.put(ExampleClusterCollection(), "clusters");
  frame2.put(ExampleClusterCollection(), "clusters2");
  REQUIRE_THROWS_WITH(writer.writeFrame(frame2, "full"),
                      ContainsSubstring("Collection 'hits' in category") &&
                          ContainsSubstring("not available in Frame"));

  // Write only a subset of collections
  const std::vector<std::string> collsToWrite = {"clusters", "hits"};
  writer.writeFrame(frame, "subset", collsToWrite);

  // Frame is missing a collection
  REQUIRE_THROWS_AS(writer.writeFrame(frame2, "subset", collsToWrite), std::runtime_error);

  // Don't throw if frame contents are different, but the subset that is written
  // is consistent
  const std::vector<std::string> otherCollsToWrite = {"clusters", "clusters2"};
  writer.writeFrame(frame, "subset2", otherCollsToWrite);
  REQUIRE_NOTHROW(writer.writeFrame(frame2, "subset2", otherCollsToWrite));

  // Make sure that restricting the second frame works.
  // See https://github.com/AIDASoft/podio/issues/382 for the original issue
  writer.writeFrame(frame2, "full_frame2");
  REQUIRE_NOTHROW(writer.writeFrame(frame, "full_frame2", frame2.getAvailableCollections()));
}

template <typename WriterT>
void runCheckConsistencyTest(const std::string& filename) {
  using Catch::Matchers::UnorderedEquals;

  WriterT writer(filename);
  podio::Frame frame;
  frame.put(ExampleClusterCollection(), "clusters");
  frame.put(ExampleClusterCollection(), "clusters2");
  frame.put(ExampleHitCollection(), "hits");
  writer.writeFrame(frame, "frame");

  // Cumbersome way to get the collections that are used for this category
  const auto& [categoryColls, emptyVec] = writer.checkConsistency({}, "frame");
  REQUIRE_THAT(categoryColls, UnorderedEquals<std::string>({"clusters", "clusters2", "hits"}));
  REQUIRE(emptyVec.empty());

  const std::vector<std::string> collsToWrite = {"clusters", "clusters2", "non-existant"};
  const auto& [missing, superfluous] = writer.checkConsistency(collsToWrite, "frame");
  REQUIRE_THAT(missing, UnorderedEquals<std::string>({"hits"}));
  REQUIRE_THAT(superfluous, UnorderedEquals<std::string>({"non-existant"}));
}

template <typename ReaderT, typename WriterT>
void runRelationAfterCloneCheck(const std::string& filename = "unittest_relations_after_cloning.root") {
  auto [hitColl, clusterColl, vecMemColl, userDataColl] = createCollections();
  auto frame = podio::Frame();
  // Empty relations
  auto emptyColl = ExampleClusterCollection();
  emptyColl.create();
  emptyColl.create();
  frame.put(std::move(emptyColl), "emptyClusters");
  // OneToOne relations
  auto oneToOneColl = ExampleWithOneRelationCollection();
  auto obj = oneToOneColl.create();
  obj.cluster(clusterColl[1]);

  frame.put(std::move(oneToOneColl), "oneToOne");
  frame.put(std::move(hitColl), "hits");
  frame.put(std::move(clusterColl), "clusters");
  frame.put(std::move(vecMemColl), "vectors");

  auto writer = WriterT(filename);
  writer.writeFrame(frame, podio::Category::Event);
  writer.finish();
  auto reader = ReaderT();
  reader.openFile(filename);
  auto readFrame = podio::Frame(reader.readNextEntry(podio::Category::Event));

  auto& clusters = readFrame.get<ExampleClusterCollection>("clusters");

  auto nCluster = clusters[0].clone();
  REQUIRE(nCluster.Hits().size() == 1);

  auto hit = MutableExampleHit(420, {}, {}, {}, {});
  nCluster.addHits(hit);
  REQUIRE(nCluster.Hits().size() == 2);
  REQUIRE(nCluster.Hits()[1].cellID() == 420);

  auto nCluster2 = nCluster.clone();
  REQUIRE(nCluster2.Hits().size() == 2);
  auto anotherHit = MutableExampleHit(421, {}, {}, {}, {});
  nCluster2.addHits(anotherHit);
  REQUIRE(nCluster2.Hits().size() == 3);
  REQUIRE(nCluster2.Hits()[2].cellID() == 421);

  auto& vectors = readFrame.get<ExampleWithVectorMemberCollection>("vectors");
  auto nVec = vectors[0].clone();
  REQUIRE(nVec.count().size() == 2);
  nVec.addcount(420);
  REQUIRE(nVec.count().size() == 3);
  REQUIRE(nVec.count()[2] == 420);

  auto newClusterCollection = ExampleClusterCollection();
  auto newHitCollection = ExampleHitCollection();
  auto& emptyClusters = readFrame.get<ExampleClusterCollection>("emptyClusters");
  auto nEmptyCluster = emptyClusters[0].clone();
  REQUIRE(nEmptyCluster.Hits().empty());
  nEmptyCluster.addHits(hit);
  REQUIRE(nEmptyCluster.Hits().size() == 1);
  REQUIRE(nEmptyCluster.Hits()[0].cellID() == 420);
  nEmptyCluster.addHits(anotherHit);
  REQUIRE(nEmptyCluster.Hits().size() == 2);
  REQUIRE(nEmptyCluster.Hits()[1].cellID() == 421);
  newClusterCollection.push_back(nEmptyCluster);
  newHitCollection.push_back(hit);
  newHitCollection.push_back(anotherHit);

  auto& collWithOneRelation = readFrame.get<ExampleWithOneRelationCollection>("oneToOne");
  REQUIRE(collWithOneRelation.size() == 1);
  auto nObj = collWithOneRelation[0].clone();
  REQUIRE(collWithOneRelation[0].cluster().energy() == 150.);
  REQUIRE(nObj.cluster().energy() == 150.);

  // Test cloned objects after writing and reading
  auto newName = std::filesystem::path(filename)
                     .replace_extension("_cloned" + std::filesystem::path(filename).extension().string())
                     .string();
  auto newWriter = WriterT(newName);
  auto newFrame = podio::Frame();
  newFrame.put(std::move(newClusterCollection), "emptyClusters");
  newFrame.put(std::move(newHitCollection), "hits");
  newWriter.writeFrame(newFrame, podio::Category::Event);
  newWriter.finish();
  auto newReader = ReaderT();
  newReader.openFile(newName);
  auto afterCloneFrame = podio::Frame(newReader.readNextEntry(podio::Category::Event));

  auto& newEmptyClusters = afterCloneFrame.get<ExampleClusterCollection>("emptyClusters");
  auto oneHitCluster = newEmptyClusters[0].clone();
  auto newHit = ExampleHit(422, 0., 0., 0., 0.);
  auto newAnotherHit = ExampleHit(423, 0., 0., 0., 0.);
  REQUIRE(nEmptyCluster.Hits().size() == 2);
  REQUIRE(nEmptyCluster.Hits()[0].cellID() == 420);
  REQUIRE(nEmptyCluster.Hits()[1].cellID() == 421);
  nEmptyCluster.addHits(newHit);
  REQUIRE(nEmptyCluster.Hits().size() == 3);
  REQUIRE(nEmptyCluster.Hits()[2].cellID() == 422);
  nEmptyCluster.addHits(newAnotherHit);
  REQUIRE(nEmptyCluster.Hits().size() == 4);
  REQUIRE(nEmptyCluster.Hits()[3].cellID() == 423);
}

TEST_CASE("Relations after cloning with TTrees", "[ASAN-FAIL][UBSAN-FAIL][relations][basics]") {
  runRelationAfterCloneCheck<podio::ROOTReader, podio::ROOTWriter>("unittests_relations_after_cloning.root");
}

TEST_CASE("ROOTWriter consistent frame contents", "[ASAN-FAIL][UBSAN-FAIL][THREAD-FAIL][basics][root]") {
  // The UBSAN-FAIL and TSAN-FAIL only happens on clang12 in CI.
  runConsistentFrameTest<podio::ROOTWriter>("unittests_frame_consistency.root");
}

TEST_CASE("ROOTWriter check consistency", "[ASAN-FAIL][UBSAN-FAIL][basics][root]") {
  runCheckConsistencyTest<podio::ROOTWriter>("unittests_frame_check_consistency.root");
}

#if PODIO_ENABLE_RNTUPLE

TEST_CASE("Relations after cloning with RNTuple", "[THREAD-FAIL][UBSAN-FAIL][relations][basics]") {
  runRelationAfterCloneCheck<podio::RNTupleReader, podio::RNTupleWriter>(
      "unittests_relations_after_cloning_rntuple.root");
}

TEST_CASE("RNTupleWriter consistent frame contents", "[UBSAN-FAIL][basics][root]") {
  runConsistentFrameTest<podio::RNTupleWriter>("unittests_frame_consistency_rntuple.root");
}

TEST_CASE("RNTupleWriter check consistency", "[UBSAN-FAIL][basics][root]") {
  runCheckConsistencyTest<podio::RNTupleWriter>("unittests_frame_check_consistency_rntuple.root");
}

#endif

#if PODIO_ENABLE_SIO

TEST_CASE("Relations after cloning with SIO", "[relations][basics]") {
  runRelationAfterCloneCheck<podio::SIOReader, podio::SIOWriter>("unittests_relations_after_cloning.sio");
}

#endif

TEST_CASE("Clone empty relations", "[relations][basics]") {
  auto coll = ExampleClusterCollection();
  coll.create();
  coll.create();
  coll[0].addHits(ExampleHit());
  coll[0].addHits(ExampleHit());
  auto newColl = ExampleClusterCollection();
  newColl.push_back(coll.at(0).clone(false));
  newColl.push_back(coll.at(1).clone(false));
  REQUIRE(newColl[0].Hits().empty());
  REQUIRE(newColl[1].Hits().empty());
  newColl[0].addHits(ExampleHit());
  REQUIRE(newColl[0].Hits().size() == 1);
  newColl[0].addHits(ExampleHit());
  REQUIRE(newColl[0].Hits().size() == 2);

  auto immCluster = ExampleCluster(coll.at(0));
  auto immCluster2 = ExampleCluster(coll.at(1));
  auto clonedImmCluster = immCluster.clone(false);
  auto clonedImmCluster2 = immCluster2.clone(false);
  REQUIRE(clonedImmCluster.Hits().empty());
  REQUIRE(clonedImmCluster2.Hits().empty());
  clonedImmCluster.addHits(ExampleHit());
  REQUIRE(clonedImmCluster.Hits().size() == 1);
  clonedImmCluster.addHits(ExampleHit());
  REQUIRE(clonedImmCluster.Hits().size() == 2);
}
