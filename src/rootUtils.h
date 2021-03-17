#ifndef PODIO_ROOT_UTILS_H // NOLINT: Cannot be easily handled by clang-tidy
#define PODIO_ROOT_UTILS_H // NOLINT: Cannot be easily handled by clang-tidy

#include "podio/CollectionBase.h"
#include "podio/CollectionBranches.h"

#include "TBranch.h"
#include "TClass.h"

#include <string>
#include <vector>

namespace podio::root_utils {
// Workaround slow branch retrieval for 6.22/06 performance degradation
// see: https://root-forum.cern.ch/t/serious-degradation-of-i-o-performance-from-6-20-04-to-6-22-06/43584/10
template <class Tree>
TBranch* getBranch(Tree* chain, const char* name) {
  return static_cast<TBranch*>(chain->GetListOfBranches()->FindObject(name));
}

inline std::string refBranch(const std::string& name, size_t index) { return name + "#" + std::to_string(index); }

inline std::string vecBranch(const std::string& name, size_t index) { return name + "_" + std::to_string(index); }

inline void setCollectionAddresses(podio::CollectionBase* collection, const CollectionBranches& branches) {
  if (auto buffer = collection->getBufferAddress()) { branches.data->SetAddress(buffer); }

  if (auto refCollections = collection->referenceCollections()) {
    for (size_t i = 0; i < refCollections->size(); ++i) {
      branches.refs[i]->SetAddress(&(*refCollections)[i]);
    }
  }

  if (auto vecMembers = collection->vectorMembers()) {
    for (size_t i = 0; i < vecMembers->size(); ++i) {
      branches.vecs[i]->SetAddress((*vecMembers)[i].second);
    }
  }
}

inline CollectionBase* prepareCollection(const TClass* dataClass, const TClass* collectionClass) {
  auto* buffer = dataClass->New();
  auto* collection = static_cast<CollectionBase*>(collectionClass->New());
  collection->setBuffer(buffer);
  return collection;
}

} // namespace podio::root_utils

#endif
