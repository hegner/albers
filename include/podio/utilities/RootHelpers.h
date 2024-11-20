#ifndef PODIO_UTILITIES_ROOTHELPERS_H
#define PODIO_UTILITIES_ROOTHELPERS_H

#include "podio/GenericParameters.h"

#include "ROOT/RVec.hxx"
#include "TBranch.h"

#include <string>
#include <tuple>
#include <vector>

namespace podio {
class CollectionBase;

namespace root_utils {

  // A collection of additional information that describes the collection: the
  // collectionID, the collection (data) type, whether it is a subset
  // collection, and its schema version
  struct CollectionWriteInfoT {
    uint32_t collectionID{static_cast<uint32_t>(-1)};
    std::string dataType{};
    bool isSubset{false};
    unsigned int schemaVersion{0};
  };

  // for backwards compatibility
  using CollectionInfoWithoutSchemaT = std::tuple<int, std::string, bool>;

  /// A collection name and a base pointer grouped together for writing
  using StoreCollection = std::tuple<const std::string&, podio::CollectionBase*>;

  /// Small helper struct to collect all branches that are necessary to read or
  /// write a collection. Needed to cache the branch pointers and avoid having to
  /// get them from a TTree/TChain for every event.
  struct CollectionBranches {
    CollectionBranches() = default;
    ~CollectionBranches() = default;
    CollectionBranches(const CollectionBranches&) = delete;
    CollectionBranches& operator=(const CollectionBranches&) = delete;
    CollectionBranches(CollectionBranches&&) = default;
    CollectionBranches& operator=(CollectionBranches&&) = default;

    CollectionBranches(TBranch* dataBranch) : data(dataBranch) {
    }

    TBranch* data{nullptr};
    std::vector<TBranch*> refs{};
    std::vector<TBranch*> vecs{};
    std::vector<std::string> refNames{}; ///< The names of the relation branches
    std::vector<std::string> vecNames{}; ///< The names of the vector member branches
  };

  /// Pair of keys and values for one type of the ones that can be stored in
  /// GenericParameters
  template <typename T>
  struct ParamStorage {
    ParamStorage() = default;
    ~ParamStorage() = default;
    ParamStorage(const ParamStorage&) = delete;
    ParamStorage& operator=(const ParamStorage&) = delete;
    ParamStorage(ParamStorage&&) = default;
    ParamStorage& operator=(ParamStorage&&) = default;

    ParamStorage(std::tuple<std::vector<std::string>, std::vector<std::vector<T>>> keysValues) :
        keys(std::move(std::get<0>(keysValues))), values(std::move(std::get<1>(keysValues))) {
    }

    /// Get a pointer to the stored keys for binding it to a TBranch
    auto keysPtr() {
      m_keysPtr = &keys;
      return &m_keysPtr;
    }

    /// Get a pointer to the stored vectors for binding it to a TBranch
    auto valuesPtr() {
      m_valuesPtr = &values;
      return &m_valuesPtr;
    }

    std::vector<std::string> keys{};      ///< The keys for this type
    std::vector<std::vector<T>> values{}; ///< The values for this type

  private:
    std::vector<std::string>* m_keysPtr{nullptr};
    std::vector<std::vector<T>>* m_valuesPtr{nullptr};
  };

  GenericParameters
  loadParamsFrom(ROOT::VecOps::RVec<std::string> intKeys, ROOT::VecOps::RVec<std::vector<int>> intValues,
                 ROOT::VecOps::RVec<std::string> floatKeys, ROOT::VecOps::RVec<std::vector<float>> floatValues,
                 ROOT::VecOps::RVec<std::string> doubleKeys, ROOT::VecOps::RVec<std::vector<double>> doubleValues,
                 ROOT::VecOps::RVec<std::string> stringKeys, ROOT::VecOps::RVec<std::vector<std::string>> stringValues);

} // namespace root_utils
} // namespace podio

#endif // PODIO_UTILITIES_ROOTHELPERS_H
