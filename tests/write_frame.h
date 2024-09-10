#ifndef PODIO_TESTS_WRITE_FRAME_H // NOLINT(llvm-header-guard): folder structure not suitable
#define PODIO_TESTS_WRITE_FRAME_H // NOLINT(llvm-header-guard): folder structure not suitable

#include "datamodel/EventInfoCollection.h"
#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/ExampleMCCollection.h"
#include "datamodel/ExampleReferencingTypeCollection.h"
#include "datamodel/ExampleWithARelationCollection.h"
#include "datamodel/ExampleWithArrayCollection.h"
#include "datamodel/ExampleWithFixedWidthIntegersCollection.h"
#include "datamodel/ExampleWithInterfaceRelationCollection.h"
#include "datamodel/ExampleWithNamespaceCollection.h"
#include "datamodel/ExampleWithOneRelationCollection.h"
#include "datamodel/ExampleWithVectorMemberCollection.h"
#include "extension_model/ContainedTypeCollection.h"
#include "extension_model/ExternalComponentTypeCollection.h"
#include "extension_model/ExternalRelationTypeCollection.h"
#include "frame_test_common.h"
#include "podio/Frame.h"
#include "podio/UserDataCollection.h"

#include <string>
#include <tuple>

auto createMCCollection() {
  auto mcps = ExampleMCCollection();

  // ---- add some MC particles ----
  auto mcp0 = mcps.create();
  auto mcp1 = mcps.create();
  auto mcp2 = mcps.create();
  auto mcp3 = mcps.create();
  auto mcp4 = mcps.create();
  auto mcp5 = mcps.create();
  auto mcp6 = mcps.create();
  auto mcp7 = mcps.create();
  auto mcp8 = mcps.create();
  auto mcp9 = mcps.create();

  auto mcp = mcps[0];
  mcp.adddaughters(mcps[2]);
  mcp.adddaughters(mcps[3]);
  mcp.adddaughters(mcps[4]);
  mcp.adddaughters(mcps[5]);
  mcp = mcps[1];
  mcp.adddaughters(mcps[2]);
  mcp.adddaughters(mcps[3]);
  mcp.adddaughters(mcps[4]);
  mcp.adddaughters(mcps[5]);
  mcp = mcps[2];
  mcp.adddaughters(mcps[6]);
  mcp.adddaughters(mcps[7]);
  mcp.adddaughters(mcps[8]);
  mcp.adddaughters(mcps[9]);
  mcp = mcps[3];
  mcp.adddaughters(mcps[6]);
  mcp.adddaughters(mcps[7]);
  mcp.adddaughters(mcps[8]);
  mcp.adddaughters(mcps[9]);

  //--- now fix the parent relations
  // use a range-based for loop here to see if we get mutable objects from the
  // begin/end iterators
  for (auto mc : mcps) {
    for (auto p : mc.daughters()) {
      int dIndex = p.getObjectID().index;
      auto d = mcps[dIndex];
      d.addparents(p);
    }
  }

  return mcps;
}

auto createArrayCollection(int i) {
  auto arrays = ExampleWithArrayCollection();

  std::array<int, 4> arrayTest = {0, 0, 2, 3};
  std::array<int, 4> arrayTest2 = {4, 4, 2 * static_cast<int>(i)};
  NotSoSimpleStruct a;
  a.data.p = arrayTest2;
  ex2::NamespaceStruct nstruct;
  nstruct.x = static_cast<int>(i);
  std::array<ex2::NamespaceStruct, 4> structArrayTest = {nstruct, nstruct, nstruct, nstruct};
  auto array = MutableExampleWithArray(a, arrayTest, arrayTest, arrayTest, arrayTest, structArrayTest);
  array.myArray(1, i);
  array.arrayStruct(a);
  arrays.push_back(array);

  return arrays;
}

auto createMCRefCollection(const ExampleMCCollection& mcps, const ExampleMCCollection& moreMCs) {
  auto mcpsRefs = ExampleMCCollection();
  mcpsRefs.setSubsetCollection();
  // ----------------- add all "odd" mc particles into a subset collection
  for (auto p : mcps) {
    if (p.id().index % 2) {
      mcpsRefs.push_back(p);
    }
  }
  // ----------------- add the "even" counterparts from a different collection
  for (auto p : moreMCs) {
    if (p.id().index % 2 == 0) {
      mcpsRefs.push_back(p);
    }
  }

  if (mcpsRefs.size() != mcps.size()) {
    throw std::runtime_error(
        "The mcParticleRefs collection should now contain as many elements as the mcparticles collection");
  }

  return mcpsRefs;
}

auto createHitCollection(int i) {
  ExampleHitCollection hits;

  auto hit1 = hits.create(0xbadULL, 0., 0., 0., 23. + i);
  auto hit2 = hits.create(0xcaffeeULL, 1., 0., 0., 12. + i);

  return hits;
}

auto createHitRefCollection(const ExampleHitCollection& hits) {
  ExampleHitCollection hitRefs;
  hitRefs.setSubsetCollection();

  hitRefs.push_back(hits[1]);
  hitRefs.push_back(hits[0]);

  return hitRefs;
}

auto createClusterCollection(const ExampleHitCollection& hits) {
  ExampleClusterCollection clusters;

  auto cluster = MutableExampleCluster();
  auto clu0 = MutableExampleCluster();
  auto clu1 = MutableExampleCluster();

  auto hit1 = hits[0];
  auto hit2 = hits[1];

  clu0.addHits(hit1);
  clu0.energy(hit1.energy());
  clu1.addHits(hit2);
  clu1.energy(hit2.energy());
  cluster.addHits(hit1);
  cluster.addHits(hit2);
  cluster.energy(hit1.energy() + hit2.energy());
  cluster.addClusters(clu0);
  cluster.addClusters(clu1);

  clusters.push_back(clu0);
  clusters.push_back(clu1);
  clusters.push_back(cluster);

  return clusters;
}

auto createReferencingCollections(const ExampleClusterCollection& clusters) {
  auto retType = std::tuple<ExampleReferencingTypeCollection, ExampleReferencingTypeCollection>();
  auto& [refs, refs2] = retType;

  auto ref = MutableExampleReferencingType();
  refs.push_back(ref);

  auto ref2 = refs2.create();

  ref.addClusters(clusters[2]);
  ref.addRefs(ref2);

  auto cyclic = MutableExampleReferencingType();
  cyclic.addRefs(cyclic);
  refs.push_back(cyclic);

  return retType;
}

auto createOneRelCollection(const ExampleClusterCollection& clusters) {
  ExampleWithOneRelationCollection oneRels;

  auto oneRel = MutableExampleWithOneRelation();
  oneRel.cluster(clusters[2]);
  oneRels.push_back(oneRel);

  // write non-filled relation
  oneRels.create();

  return oneRels;
}

auto createVectorMemberCollection(int i) {
  ExampleWithVectorMemberCollection vecs;

  auto vec = MutableExampleWithVectorMember();
  vec.addcount(i);
  vec.addcount(i + 10);
  vecs.push_back(vec);
  auto vec1 = MutableExampleWithVectorMember();
  vec1.addcount(i + 1);
  vec1.addcount(i + 11);
  vecs.push_back(vec1);

  return vecs;
}

auto createVectorMemberSubsetCollection(const ExampleWithVectorMemberCollection& coll) {
  ExampleWithVectorMemberCollection refs;
  refs.setSubsetCollection();

  refs.push_back(coll[0]);

  return refs;
}

auto createInfoCollection(int i) {
  EventInfoCollection info;

  auto item1 = MutableEventInfo();
  item1.Number(i);
  info.push_back(item1);

  return info;
}

auto createFixedWidthCollection() {
  auto fixedWidthInts = ExampleWithFixedWidthIntegersCollection();

  auto maxValues = fixedWidthInts.create();
  maxValues.fixedI16(std::numeric_limits<std::int16_t>::max());  // 2^(16 - 1) - 1 == 32767
  maxValues.fixedU32(std::numeric_limits<std::uint32_t>::max()); // 2^32 - 1 == 4294967295
  maxValues.fixedU64(std::numeric_limits<std::uint64_t>::max()); // 2^64 - 1 == 18446744073709551615
  auto& maxComp = maxValues.fixedWidthStruct();
  maxComp.fixedUnsigned16 = std::numeric_limits<std::uint16_t>::max(); // 2^16 - 1 == 65535
  maxComp.fixedInteger64 = std::numeric_limits<std::int64_t>::max();   // 2^(64 -1) - 1 == 9223372036854775807
  maxComp.fixedInteger32 = std::numeric_limits<std::int32_t>::max();   // 2^(32 - 1) - 1 == 2147483647

  auto minValues = fixedWidthInts.create();
  minValues.fixedI16(std::numeric_limits<std::int16_t>::min());  // -2^(16 - 1) == -32768
  minValues.fixedU32(std::numeric_limits<std::uint32_t>::min()); // 0
  minValues.fixedU64(std::numeric_limits<std::uint64_t>::min()); // 0
  auto& minComp = minValues.fixedWidthStruct();
  minComp.fixedUnsigned16 = std::numeric_limits<std::uint16_t>::min(); // 0
  minComp.fixedInteger64 = std::numeric_limits<std::int64_t>::min();   // -2^(64 - 1) == -9223372036854775808
  minComp.fixedInteger32 = std::numeric_limits<std::int32_t>::min();   // -2^(32 - 1) == -2147483648

  auto arbValues = fixedWidthInts.create();
  arbValues.fixedI16(-12345);
  arbValues.fixedU32(1234567890);
  arbValues.fixedU64(1234567890123456789);
  auto& arbComp = arbValues.fixedWidthStruct();
  arbComp.fixedUnsigned16 = 12345;
  arbComp.fixedInteger32 = -1234567890;
  arbComp.fixedInteger64 = -1234567890123456789ll;

  return fixedWidthInts;
}

auto createUserDataCollections(int i) {
  auto retType = std::tuple<podio::UserDataCollection<uint64_t>, podio::UserDataCollection<double>>();
  auto& [usrInts, usrDoubles] = retType;

  // add some plain ints as user data
  usrInts.resize(i + 1);
  int myInt = 0;
  for (auto& iu : usrInts) {
    iu = myInt++;
  }

  // and some user double values
  unsigned nd = 100;
  usrDoubles.resize(nd);
  for (unsigned id = 0; id < nd; ++id) {
    usrDoubles[id] = 42.;
  }

  return retType;
}

auto createNamespaceRelationCollection(int i) {
  auto retVal = std::tuple<ex42::ExampleWithNamespaceCollection, ex42::ExampleWithARelationCollection,
                           ex42::ExampleWithARelationCollection>{};
  auto& [namesps, namesprels, cpytest] = retVal;

  for (int j = 0; j < 5; j++) {
    auto rel = ex42::MutableExampleWithARelation();
    rel.number(0.5 * j);
    auto exWithNamesp = ex42::MutableExampleWithNamespace();
    exWithNamesp.component().x = i;
    exWithNamesp.component().y = 1000 * i;
    namesps.push_back(exWithNamesp);
    if (j != 3) { // also check for empty relations
      rel.ref(exWithNamesp);
      for (int k = 0; k < 5; k++) {
        auto namesp = ex42::MutableExampleWithNamespace();
        namesp.x(3 * k);
        namesp.component().y = k;
        namesps.push_back(namesp);
        rel.addrefs(namesp);
      }
    }
    namesprels.push_back(rel);
  }
  for (auto&& namesprel : namesprels) {
    cpytest.push_back(namesprel.clone());
  }

  return retVal;
}

auto createExtensionContainedCollection(int i) {
  auto coll = extension::ContainedTypeCollection();
  // The ContainedType only has a polar vector
  auto contElem = coll.create();
  contElem.setAVector({i * 123.f, 0.15f, 3.14f});

  return coll;
}

auto createExtensionExternalComponentCollection(int i) {
  auto coll = extension::ExternalComponentTypeCollection();

  // Set the upstream components only here
  auto extCompElem = coll.create();
  extension::ExtComponent extComp{};
  extComp.aStruct.data.x = 42 * i;
  extComp.nspStruct = ex2::NamespaceStruct{i, 2 * i};
  extCompElem.setAComponent(extComp);
  SimpleStruct simpleS{};
  simpleS.p = {i, i - 2, i + 4, i * 8};
  extCompElem.setAStruct(simpleS);

  return coll;
}

auto createExtensionExternalRelationCollection(int i, const ExampleHitCollection& hits,
                                               const ExampleClusterCollection& clusters) {
  auto coll = extension::ExternalRelationTypeCollection();

  auto elem0 = coll.create();
  elem0.setWeight(i * 100.f);
  elem0.setSingleHit(hits[0]);

  auto elem1 = coll.create();
  elem1.addToClusters(clusters[1]);
  elem1.addToClusters(clusters[0]);

  auto elem2 = coll.create();
  for (int j = 0; j < 3; j++) {
    auto s = SimpleStruct();
    s.y = j * i;
    elem2.addToSomeStructs(s);
  }

  return coll;
}

auto createExampleWithInterfaceCollection(const ExampleHitCollection& hits, const ExampleClusterCollection& clusters,
                                          const ExampleMCCollection& particles) {
  auto coll = ExampleWithInterfaceRelationCollection{};
  auto elem = coll.create();
  elem.aSingleEnergyType(hits[0]);
  elem.addmanyEnergies(hits[0]);
  elem.addmanyEnergies(clusters[0]);
  elem.addmanyEnergies(particles[0]);

  elem = coll.create();
  elem.aSingleEnergyType(clusters[0]);
  elem.addmanyEnergies(particles[0]);
  elem.addmanyEnergies(hits[0]);
  elem.addmanyEnergies(clusters[0]);

  return coll;
}

podio::Frame makeFrame(int iFrame) {
  podio::Frame frame{};

  frame.put(createArrayCollection(iFrame), "arrays");
  const auto& vecMemColl = frame.put(createVectorMemberCollection(iFrame), "WithVectorMember");
  frame.put(createVectorMemberSubsetCollection(vecMemColl), "VectorMemberSubsetColl");
  frame.put(createInfoCollection(iFrame), "info");
  frame.put(createFixedWidthCollection(), "fixedWidthInts");

  auto& mcps = frame.put(createMCCollection(), "mcparticles");

  ExampleMCCollection moreMCs{};
  for (const auto&& mc : mcps) {
    moreMCs.push_back(mc.clone());
  }
  auto& otherMCs = frame.put(std::move(moreMCs), "moreMCs");
  frame.put(createMCRefCollection(mcps, otherMCs), "mcParticleRefs");

  const auto& hits = frame.put(createHitCollection(iFrame), "hits");
  frame.put(createHitRefCollection(hits), "hitRefs");

  const auto& clusters = frame.put(createClusterCollection(hits), "clusters");

  auto [refs, refs2] = createReferencingCollections(clusters);
  frame.put(std::move(refs), "refs");
  frame.put(std::move(refs2), "refs2");

  frame.put(createOneRelCollection(clusters), "OneRelation");

  auto [usrInts, usrDoubles] = createUserDataCollections(iFrame);
  frame.put(std::move(usrInts), "userInts");
  frame.put(std::move(usrDoubles), "userDoubles");

  auto [namesps, namespsrels, cpytest] = createNamespaceRelationCollection(iFrame);
  frame.put(std::move(namesps), "WithNamespaceMember");
  frame.put(std::move(namespsrels), "WithNamespaceRelation");
  frame.put(std::move(cpytest), "WithNamespaceRelationCopy");

  // Parameters
  frame.putParameter("anInt", 42 + iFrame);
  frame.putParameter("UserEventWeight", 100.f * iFrame);
  frame.putParameter("UserEventName", " event_number_" + std::to_string(iFrame));
  frame.putParameter("SomeVectorData", {1, 2, 3, 4});
  frame.putParameter("SomeVectorData", {"just", "some", "strings"});
  frame.putParameter("SomeVectorData", {iFrame * 1.1, iFrame * 2.2});
  frame.putParameter("SomeValue", "string value");
  frame.putParameter("SomeValue", 42);

  // An empty collection
  frame.put(ExampleClusterCollection(), "emptyCollection");
  // An empty subset collection
  auto emptySubsetColl = ExampleHitCollection();
  emptySubsetColl.setSubsetCollection();
  frame.put(std::move(emptySubsetColl), "emptySubsetColl");

  frame.put(createExtensionContainedCollection(iFrame), "extension_Contained");
  frame.put(createExtensionExternalComponentCollection(iFrame), "extension_ExternalComponent");
  frame.put(createExtensionExternalRelationCollection(iFrame, hits, clusters), "extension_ExternalRelation");

  frame.put(createExampleWithInterfaceCollection(hits, clusters, mcps), "interface_examples");

  return frame;
}

template <typename WriterT>
void write_frames(const std::string& filename) {
  WriterT writer(filename);

  for (int i = 0; i < 10; ++i) {
    auto frame = makeFrame(i);
    writer.writeFrame(frame, podio::Category::Event, collsToWrite);
  }

  for (int i = 100; i < 110; ++i) {
    auto frame = makeFrame(i);
    writer.writeFrame(frame, "other_events");
  }

  writer.finish();
}

#endif // PODIO_TESTS_WRITE_FRAME_H
