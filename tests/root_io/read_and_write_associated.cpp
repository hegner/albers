#include "datamodel/EventInfoCollection.h"
#include "datamodel/ExampleCluster.h"
#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/MutableEventInfo.h"
#include "datamodel/MutableExampleCluster.h"
#include "datamodel/MutableExampleHit.h"
#include "podio/Frame.h"
#include "podio/FrameCategories.h"
#include "podio/ROOTReader.h"
#include "podio/ROOTWriter.h"
#include "podio/RelationRange.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void writeCollection() {
  podio::ROOTWriter writer("associations.root");

  std::cout << "start writing collections...\n";

  unsigned nevents = 2;

  for (unsigned i = 0; i < nevents; ++i) {
    auto event = podio::Frame{};

    auto info = EventInfoCollection{};
    auto item1 = MutableEventInfo();
    item1.Number(i);
    info.push_back(item1);

    event.putParameter("UserEventWeight", 100.f * i);
    std::cout << " event number: " << i << std::endl;
    event.putParameter("UserEventName", std::to_string(i));

    auto hits = ExampleHitCollection{};
    auto hit1 = hits.create(0xbadULL, 0., 0., 0., 23. + i);
    auto hit2 = hits.create(0xcaffeeULL, 1., 0., 0., 12. + i);

    // Clusters
    auto clusters = ExampleClusterCollection{};
    auto cluster = MutableExampleCluster();
    auto clu0 = MutableExampleCluster();
    auto clu1 = MutableExampleCluster();

    clu0.addHits(hit1);
    clu0.energy(hit1.energy());
    clu1.addHits(hit2);
    clu1.energy(hit2.energy());
    cluster.addHits(hit1);
    cluster.addHits(hit2);
    cluster.energy(hit1.energy() + hit2.energy());
    cluster.addClusters(clu0);
    cluster.addClusters(clu1);

    // Add tracked hits to subset hits collection
    auto hits_subset = ExampleHitCollection{};
    hits_subset.setSubsetCollection();
    hits_subset.push_back(hit1);
    hits_subset.push_back(hit2);

    // Push all clusters
    clusters.push_back(clu0);
    clusters.push_back(clu1);
    clusters.push_back(cluster);

    // event.put(std::move(hits), "hits");
    event.put(std::move(info), "info");
    event.put(std::move(hits_subset), "hits_subset");
    event.put(std::move(clusters), "clusters");

    writer.writeFrame(event, podio::Category::Event);
  }
}

void readCollection() {
  // Start reading the input
  auto reader = podio::ROOTReader();
  reader.openFile("associations.root");

  const auto nEvents = reader.getEntries(podio::Category::Event);

  for (unsigned i = 0; i < nEvents; ++i) {
    auto store = podio::Frame(reader.readNextEntry(podio::Category::Event));

    auto& clusters = store.get<ExampleClusterCollection>("clusters");
    if (clusters.isValid()) {
      for (const auto& cluster : clusters) {
        if (cluster.isAvailable()) {
          for (const auto& hit : cluster.Hits()) {
            if (hit.isAvailable()) {
              throw std::runtime_error("Hit is available, although it has not been written");
            }
          }
        }
      }
    } else {
      throw std::runtime_error("Collection 'clusters' should be present");
    }

    // Test for subset collections
    auto& hits_subset = store.get<ExampleHitCollection>("hits_subset");
    if (hits_subset.isValid()) {
      if (!hits_subset.isSubsetCollection()) {
        throw std::runtime_error("hits_subset should be a subset collection");
      }

      if (hits_subset.size() != 2) {
        throw std::runtime_error("subset collection should have original size");
      }

      for (const auto& hit : hits_subset) {
        if (hit.isAvailable()) {
          throw std::runtime_error("Hit is available, although it has not been written");
        }
      }
    } else {
      throw std::runtime_error("Collection 'hits_subset' should be present");
    }
  }
}

int main() {

  writeCollection();
  readCollection();

  return 0;
}
