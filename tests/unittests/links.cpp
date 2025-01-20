#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_vector.hpp"

#include "podio/LinkCollection.h"
#include "podio/LinkNavigator.h"

#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/TestInterfaceLinkCollection.h"
#include "datamodel/TypeWithEnergy.h"

#ifdef PODIO_JSON_OUTPUT
  #include "nlohmann/json.hpp"
#endif

#include <type_traits>

// Test datatypes (spelling them out here explicitly to make sure that
// assumptions about typedefs actually hold)
using TestL = podio::Link<ExampleHit, ExampleCluster>;
using TestMutL = podio::MutableLink<ExampleHit, ExampleCluster>;
using TestLColl = podio::LinkCollection<ExampleHit, ExampleCluster>;
using TestLIter = podio::LinkCollectionIterator<ExampleHit, ExampleCluster>;
using TestLMutIter = podio::LinkMutableCollectionIterator<ExampleHit, ExampleCluster>;

TEST_CASE("Link constness", "[links][static-checks]") {
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestMutL>().getFrom()), const ExampleHit>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestMutL>().getTo()), const ExampleCluster>);

  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestL>().getFrom()), const ExampleHit>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestL>().getTo()), const ExampleCluster>);
}

TEST_CASE("Link basics", "[links]") {
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks): There are quite a few
  // false positives here from clang-tidy that we are confident are false
  // positives, because we don't see issues in our builds with sanitizers
  auto cluster = MutableExampleCluster();
  auto hit = MutableExampleHit();

  auto mutLink = TestMutL();
  mutLink.setWeight(3.14f);
  mutLink.setFrom(hit);
  mutLink.setTo(cluster);

  REQUIRE(mutLink.getWeight() == 3.14f);
  REQUIRE(mutLink.getFrom() == hit);
  REQUIRE(mutLink.getTo() == cluster);

  SECTION("Copying") {
    auto otherLink = mutLink;
    REQUIRE(otherLink.getWeight() == 3.14f);
    REQUIRE(otherLink.getFrom() == hit);
    REQUIRE(otherLink.getTo() == cluster);

    auto otherCluster = ExampleCluster();
    auto otherHit = ExampleHit();
    otherLink.setFrom(otherHit);
    otherLink.setTo(otherCluster);
    otherLink.setWeight(42.0f);
    REQUIRE(otherLink.getWeight() == 42.0f);
    REQUIRE(otherLink.getFrom() == otherHit);
    REQUIRE(otherLink.getTo() == otherCluster);

    // Make sure original link changes as well
    REQUIRE(mutLink.getWeight() == 42.0f);
    REQUIRE(mutLink.getFrom() == otherHit);
    REQUIRE(mutLink.getTo() == otherCluster);
  }

  SECTION("Assignment") {
    auto otherLink = TestMutL();
    otherLink = mutLink;
    REQUIRE(otherLink.getWeight() == 3.14f);
    REQUIRE(otherLink.getFrom() == hit);
    REQUIRE(otherLink.getTo() == cluster);

    auto otherCluster = ExampleCluster();
    auto otherHit = ExampleHit();
    otherLink.setFrom(otherHit);
    otherLink.setTo(otherCluster);
    otherLink.setWeight(42.0f);
    REQUIRE(otherLink.getWeight() == 42.0f);
    REQUIRE(otherLink.getFrom() == otherHit);
    REQUIRE(otherLink.getTo() == otherCluster);

    // Make sure original link changes as well
    REQUIRE(mutLink.getWeight() == 42.0f);
    REQUIRE(mutLink.getFrom() == otherHit);
    REQUIRE(mutLink.getTo() == otherCluster);
  }

  SECTION("Implicit conversion") {
    // Use an immediately invoked lambda to check that the implicit conversion
    // is working as desired
    [hit, cluster](TestL link) { // NOLINT(performance-unnecessary-value-param):
                                 // We want the value here to force the
                                 // conversion
      REQUIRE(link.getWeight() == 3.14f);
      REQUIRE(link.getFrom() == hit);
      REQUIRE(link.getTo() == cluster);
    }(mutLink);
  }

  SECTION("Cloning") {
    auto otherLink = mutLink.clone();
    REQUIRE(otherLink.getWeight() == 3.14f);
    REQUIRE(otherLink.getFrom() == hit);
    REQUIRE(otherLink.getTo() == cluster);

    auto otherCluster = ExampleCluster();
    auto otherHit = ExampleHit();
    otherLink.setFrom(otherHit);
    otherLink.setTo(otherCluster);
    otherLink.setWeight(42.0f);
    REQUIRE(otherLink.getWeight() == 42.0f);
    REQUIRE(otherLink.getFrom() == otherHit);
    REQUIRE(otherLink.getTo() == otherCluster);

    // Make sure original link is unchanged
    REQUIRE(mutLink.getWeight() == 3.14f);
    REQUIRE(mutLink.getFrom() == hit);
    REQUIRE(mutLink.getTo() == cluster);

    // Check cloning from an immutable one
    TestL link = mutLink;
    auto anotherLink = link.clone();
    anotherLink.setFrom(otherHit);
    anotherLink.setTo(otherCluster);
    anotherLink.setWeight(42.0f);
    REQUIRE(anotherLink.getWeight() == 42.0f);
    REQUIRE(anotherLink.getFrom() == otherHit);
    REQUIRE(anotherLink.getTo() == otherCluster);

    // Cloning without relations
    auto linkNoRel = link.clone(false);
    REQUIRE_FALSE(linkNoRel.getFrom().isAvailable());
    REQUIRE_FALSE(linkNoRel.getTo().isAvailable());
    REQUIRE(linkNoRel.getWeight() == 3.14f);
  }

  SECTION("Equality operator") {
    auto otherLink = mutLink; // NOLINT(performance-unnecessary-copy-initialization)
    REQUIRE(otherLink == mutLink);

    // Mutable and immutable links should be comparable
    TestL link = mutLink;
    REQUIRE(link == mutLink);

    // operator!= is also defined and working
    auto newLink = TestL{};
    REQUIRE(otherLink != newLink);
    REQUIRE(link != newLink);
  }
}
// NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

TEST_CASE("Links templated accessors", "[links]") {
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks): There are quite a few
  // false positives here from clang-tidy that we are confident are false
  // positives, because we don't see issues in our builds with sanitizers
  ExampleHit hit;
  ExampleCluster cluster;

  TestMutL link;
  link.set(hit);
  link.set(cluster);
  link.setWeight(1.0);

  SECTION("Mutable Link") {
    REQUIRE(hit == link.get<ExampleHit>());
    REQUIRE(cluster == link.get<ExampleCluster>());

    const auto& [h, c, w] = link;
    REQUIRE(h == hit);
    REQUIRE(c == cluster);
    REQUIRE(w == 1.0);
  }

  SECTION("Immutable link") {
    TestL a{link};

    REQUIRE(hit == a.get<ExampleHit>());
    REQUIRE(cluster == a.get<ExampleCluster>());

    const auto& [h, c, w] = a;
    REQUIRE(h == hit);
    REQUIRE(c == cluster);
    REQUIRE(w == 1.0);
  }
}
// NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

TEST_CASE("LinkCollection constness", "[links][static-checks][const-correctness]") {
  // Test type-aliases in LinkCollection
  STATIC_REQUIRE(std::is_same_v<TestLColl::const_iterator, TestLIter>);
  STATIC_REQUIRE(std::is_same_v<TestLColl::iterator, TestLMutIter>);

  SECTION("const collections with const iterators") {
    const auto coll = TestLColl();
    // this essentially checks the whole "chain" from begin() / end() through
    // iterator operators
    for (auto link : coll) {
      STATIC_REQUIRE(std::is_same_v<decltype(link), TestL>); // const collection iterators should only return
                                                             // immutable objects
    }

    // check the individual steps again from above, to see where things fail if they fail
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const TestLColl>().begin()),
                                  TestLColl::const_iterator>); // const collectionb begin() should return a
                                                               // LinkCollectionIterator

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const TestLColl>().end()),
                                  TestLColl::const_iterator>); // const collectionb end() should return a
                                                               // LinkCollectionIterator

    STATIC_REQUIRE(std::is_same_v<decltype(*std::declval<const TestLColl>().begin()),
                                  TestL>); // LinkCollectionIterator should only give access to immutable
                                           // objects

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestLIter>().operator->()),
                                  TestL*>); // LinkCollectionIterator should only give access to immutable
                                            // objects
  }

  SECTION("non-const collections with mutable iterators") {
    auto coll = TestLColl();
    // this essentially checks the whole "chain" from begin() / end() through
    // iterator operators
    for (auto link : coll) {
      STATIC_REQUIRE(std::is_same_v<decltype(link), TestMutL>); // collection iterators should return return
                                                                // mutable objects
    }

    // check the individual steps again from above, to see where things fail if they fail
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestLColl>().begin()),
                                  TestLColl::iterator>); // collection begin() should return a
                                                         // MutableCollectionIterator

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestLColl>().end()),
                                  TestLColl::iterator>); // collectionb end() should return a
                                                         // MutableCollectionIterator

    STATIC_REQUIRE(std::is_same_v<decltype(*std::declval<TestLColl>().begin()),
                                  TestMutL>); // MutableCollectionIterator should give access to immutable
                                              // mutable objects

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestLMutIter>().operator->()),
                                  TestMutL*>); // MutableCollectionIterator should give access to immutable
                                               // mutable objects
  }

  SECTION("const correct indexed access to const collections") {
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const TestLColl>()[0]),
                                  TestL>); // const collections should only have indexed indexed access to immutable
                                           // objects

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const TestLColl>().at(0)),
                                  TestL>); // const collections should only have indexed indexed access to immutable
                                           // objects
  }

  SECTION("const correct indexed access to collections") {
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestLColl>()[0]),
                                  TestMutL>); // collections should have indexed indexed access to mutable objects

    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<TestLColl>().at(0)),
                                  TestMutL>); // collections should have indexed indexed access to mutable objects
  }
}

TEST_CASE("LinkCollection subset collection", "[links][subset-colls]") {
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks): There are quite a few
  // false positives here from clang-tidy that we are confident are false
  // positives, because we don't see issues in our builds with sanitizers
  auto links = TestLColl();
  auto link1 = links.create();
  link1.setWeight(1.0f);
  auto link2 = links.create();
  link2.setWeight(2.0f);

  auto linkRefs = TestLColl();
  linkRefs.setSubsetCollection();
  for (const auto a : links) {
    linkRefs.push_back(a);
  }

  SECTION("Collection iterators work with subset collections") {

    // index-based looping / access
    for (size_t i = 0; i < linkRefs.size(); ++i) {
      REQUIRE(linkRefs[i].getWeight() == i + 1);
    }

    // range-based for loop
    int index = 1;
    for (const auto a : linkRefs) {
      REQUIRE(a.getWeight() == index++);
    }
  }

  SECTION("Conversion failures") {
    // Cannot convert into a subset collection if elements already present
    REQUIRE_THROWS_AS(links.setSubsetCollection(), std::logic_error);

    // Cannot convert a subset collection into a normal collection
    REQUIRE_THROWS_AS(linkRefs.setSubsetCollection(false), std::logic_error);
  }

  SECTION("Subset collection only handles tracked objects") {
    auto link = TestL();
    REQUIRE_THROWS_AS(linkRefs.push_back(link), std::invalid_argument);
    REQUIRE_THROWS_AS(linkRefs.create(), std::logic_error);
  }
}
// NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

TEST_CASE("LinkCollection basics", "[links]") {
  REQUIRE(podio::detail::linkCollTypeName<ExampleCluster, ExampleHit>() ==
          "podio::LinkCollection<ExampleCluster,ExampleHit>");

  auto links = TestLColl{};
  auto link = links.create();
  REQUIRE(link.id().collectionID == 0);

  links.setID(42);
  for (auto l : links) {
    REQUIRE(l.id().collectionID == 42);
  }
}

auto createLinkCollections(const size_t nElements = 3u) {
  auto colls = std::make_tuple(TestLColl(), ExampleHitCollection(), ExampleClusterCollection());

  auto& [linkColl, hitColl, clusterColl] = colls;
  for (auto i = 0u; i < nElements; ++i) {
    auto hit = hitColl.create();
    auto cluster = clusterColl.create();
  }

  for (auto i = 0u; i < nElements; ++i) {
    auto link = linkColl.create();
    link.setWeight(i);
    // Fill the relations in opposite orders to at least uncover issues that
    // could be hidden by running the indices in parallel
    link.setFrom(hitColl[i]);
    link.setTo(clusterColl[nElements - i - 1]);
  }

  return colls;
}

void checkCollections(const TestLColl& links, const ExampleHitCollection& hits,
                      const ExampleClusterCollection& clusters, const size_t nElements = 3u) {
  REQUIRE(links.size() == 3);
  REQUIRE(hits.size() == 3);
  REQUIRE(clusters.size() == 3);

  size_t index = 0;
  for (auto link : links) {
    REQUIRE(link.getWeight() == index);
    REQUIRE(link.getFrom() == hits[index]);
    REQUIRE(link.getTo() == clusters[nElements - index - 1]);

    index++;
  }
}

TEST_CASE("LinkCollection looping", "[links][basics]") {
  const auto [linkColl, hitColl, clusterColl] = createLinkCollections();

  int i = 0;
  const auto collSize = linkColl.size();
  for (const auto& [from, to, weight] : linkColl) {
    STATIC_REQUIRE(std::is_same_v<decltype(from), const ExampleHit>);
    STATIC_REQUIRE(std::is_same_v<decltype(to), const ExampleCluster>);
    STATIC_REQUIRE(std::is_same_v<decltype(weight), const float>);

    REQUIRE(from == hitColl[i]);
    REQUIRE(to == clusterColl[collSize - 1 - i]);
    REQUIRE(weight == i);
    i++;
  }
}

TEST_CASE("LinkCollection movability", "[links][move-semantics][collections]") {
  // Setup a few collections for testing
  auto [linkColl, hitColl, clusterColl] = createLinkCollections();

  // Check that after the setup everything is as expected
  checkCollections(linkColl, hitColl, clusterColl);

  SECTION("Move constructor and assignment") {
    // NOTE: moving datatype collections is already covered by respective tests
    auto newLinks = std::move(linkColl);
    checkCollections(newLinks, hitColl, clusterColl);

    auto newerLinks = TestLColl();
    newerLinks = std::move(newLinks);
    checkCollections(newerLinks, hitColl, clusterColl);
  }

  SECTION("Prepared collections can be move assigned/constructed") {
    linkColl.prepareForWrite();
    auto newLinks = std::move(linkColl);
    // checkCollections(newLinks, hitColl, clusterColl);

    newLinks.prepareForWrite();
    auto newerLinks = TestLColl();
    newerLinks = std::move(newLinks);
    // checkCollections(newLinks, hitColl, clusterColl);
  }

  SECTION("Subset collections can be moved") {
    // Create a subset collection to move from
    auto subsetLinks = TestLColl();
    subsetLinks.setSubsetCollection();
    for (auto a : linkColl) {
      subsetLinks.push_back(a);
    }
    checkCollections(subsetLinks, hitColl, clusterColl);

    // Move constructor
    auto newSubsetLinks = std::move(subsetLinks);
    checkCollections(newSubsetLinks, hitColl, clusterColl);
    REQUIRE(newSubsetLinks.isSubsetCollection());

    // Move assignment
    auto evenNewerLinks = TestLColl();
    evenNewerLinks = std::move(newSubsetLinks);
    checkCollections(evenNewerLinks, hitColl, clusterColl);
    REQUIRE(evenNewerLinks.isSubsetCollection());
  }
}

TEST_CASE("Links with interfaces", "[links][interface-types]") {
  auto coll = TestInterfaceLinkCollection{};
  auto cluster = ExampleCluster();
  auto hit = ExampleHit();
  auto iface = TypeWithEnergy(hit);

  auto link = coll.create();
  link.setFrom(cluster);
  link.setTo(iface);

  REQUIRE(link.getFrom() == cluster);
  REQUIRE(link.get<ExampleCluster>() == cluster);
  REQUIRE(link.getTo() == iface);
  REQUIRE(link.get<TypeWithEnergy>() == iface);

  link.set(TypeWithEnergy(cluster));
  REQUIRE(link.get<ExampleCluster>() == cluster); // unchanged
  REQUIRE(link.get<TypeWithEnergy>() == cluster);

  // Setting with implicit conversion to interface
  auto hit2 = ExampleHit();
  link.setTo(hit2);
  REQUIRE(link.get<TypeWithEnergy>() == hit2);

  link.set(hit);
  REQUIRE(link.get<TypeWithEnergy>() == hit);
  REQUIRE(link.get<ExampleCluster>() == cluster);
}

TEST_CASE("Links reverse iterators", "[links][iterator]") {
  const auto [linkColl, hitColl, clusterColl] = createLinkCollections();
  REQUIRE(linkColl.size() > 1);
  auto it = --linkColl.rend();
  REQUIRE(*it == *linkColl.begin());
  it = linkColl.rbegin();
  REQUIRE(*it == *--linkColl.end());
  auto cit = --linkColl.rend();
  REQUIRE(*cit == *linkColl.cbegin());
  cit = linkColl.crbegin();
  REQUIRE(*cit == *--linkColl.cend());
}

#ifdef PODIO_JSON_OUTPUT
TEST_CASE("Link JSON conversion", "[links][json]") {
  const auto& [links, hits, clusters] = createLinkCollections();
  const nlohmann::json json{{"links", links}};

  REQUIRE(json["links"].size() == 3);

  int i = 0;
  for (const auto& link : json["links"]) {
    REQUIRE(link["weight"] == i);
    REQUIRE(link["from"]["index"] == i);
    i++;
  }

  i = 2;
  for (const auto& link : json["links"]) {
    REQUIRE(link["to"]["index"] == i);
    i--;
  }
}

#endif

TEST_CASE("LinkNavigator basics", "[links]") {
  TestLColl coll{};
  std::vector<ExampleHit> hits(11);
  std::vector<ExampleCluster> clusters(3);

  for (size_t i = 0; i < 10; ++i) {
    auto a = coll.create();
    a.set(hits[i]);
    a.set(clusters[i % 3]);
    a.setWeight(i * 0.1f);
  }

  auto a = coll.create();
  a.set(hits[10]);

  podio::LinkNavigator nav{coll};

  for (size_t i = 0; i < 10; ++i) {
    const auto& hit = hits[i];
    const auto linkedClusters = nav.getLinked(hit);
    REQUIRE(linkedClusters.size() == 1);
    const auto& [cluster, weight] = linkedClusters[0];
    REQUIRE(cluster == clusters[i % 3]);
    REQUIRE(weight == i * 0.1f);
  }

  using Catch::Matchers::UnorderedEquals;
  using podio::detail::links::WeightedObject;
  using WeightedHits = std::vector<WeightedObject<ExampleHit>>;

  auto linkedHits = nav.getLinked(clusters[0]);
  REQUIRE_THAT(linkedHits,
               UnorderedEquals(WeightedHits{WeightedObject{hits[0], 0.f}, WeightedObject{hits[3], 3 * 0.1f},
                                            WeightedObject{hits[6], 6 * 0.1f}, WeightedObject{hits[9], 9 * 0.1f}}));

  linkedHits = nav.getLinked(clusters[1]);
  REQUIRE_THAT(linkedHits,
               UnorderedEquals(WeightedHits{WeightedObject{hits[1], 0.1f}, WeightedObject{hits[4], 0.4f},
                                            WeightedObject{hits[7], 0.7f}}));

  const auto [noCluster, noWeight] = nav.getLinked(hits[10])[0];
  REQUIRE_FALSE(noCluster.isAvailable());
}

TEST_CASE("LinkNavigator same types", "[links]") {
  std::vector<ExampleCluster> clusters(3);
  auto linkColl = podio::LinkCollection<ExampleCluster, ExampleCluster>{};
  auto link = linkColl.create();
  link.setFrom(clusters[0]);
  link.setTo(clusters[1]);
  link.setWeight(0.5f);

  link = linkColl.create();
  link.setFrom(clusters[0]);
  link.setTo(clusters[2]);
  link.setWeight(0.25f);

  link = linkColl.create();
  link.setFrom(clusters[1]);
  link.setTo(clusters[2]);
  link.setWeight(0.66f);

  auto navigator = podio::LinkNavigator{linkColl};
  auto linkedClusters = navigator.getLinked(clusters[1], podio::ReturnTo);
  REQUIRE(linkedClusters.size() == 1);
  REQUIRE(linkedClusters[0].o == clusters[2]);
  REQUIRE(linkedClusters[0].weight == 0.66f);

  linkedClusters = navigator.getLinked(clusters[1], podio::ReturnFrom);
  REQUIRE(linkedClusters.size() == 1);
  REQUIRE(linkedClusters[0].o == clusters[0]);
  REQUIRE(linkedClusters[0].weight == 0.5f);

  using Catch::Matchers::UnorderedEquals;
  using podio::detail::links::WeightedObject;
  using WeightedObjVec = std::vector<WeightedObject<ExampleCluster>>;
  linkedClusters = navigator.getLinked(clusters[0], podio::ReturnTo);
  REQUIRE_THAT(linkedClusters,
               UnorderedEquals(WeightedObjVec{WeightedObject(clusters[1], 0.5f), WeightedObject{clusters[2], 0.25f}}));

  linkedClusters = navigator.getLinked(clusters[2], podio::ReturnFrom);
  REQUIRE_THAT(linkedClusters,
               UnorderedEquals(WeightedObjVec{WeightedObject{clusters[0], 0.25f}, WeightedObject{clusters[1], 0.66f}}));
}
