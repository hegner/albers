#ifndef PODIO_UTILITIES_DATAMODELREGISTRYIOHELPERS_H
#define PODIO_UTILITIES_DATAMODELREGISTRYIOHELPERS_H

#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace podio {
class CollectionBase;

/// Helper class to collect the datamodel (JSON) definitions that should be
/// written.
class DatamodelDefinitionCollector {
public:
  /// Register the datamodel definition of the EDM this collection is from to be
  /// written.
  ///
  /// @param coll A collection of an EDM
  /// @param name The name under which this collection is stored on file
  void registerDatamodelDefinition(const podio::CollectionBase* coll, const std::string& name);

  /// Get all the names and JSON definitions that need to be written
  std::vector<std::tuple<std::string, std::string>> getDatamodelDefinitionsToWrite() const;

private:
  std::set<size_t> m_edmDefRegistryIdcs{}; ///< The indices in the EDM definition registry that need to be written
};

/// Helper class to hold and provide the datamodel (JSON) definitions for reader
/// classes.
class DatamodelDefinitionHolder {
public:
  /// The "map" type that is used internally
  using MapType = std::vector<std::tuple<std::string, std::string>>;
  /// Constructor from an existing collection of names and datamodel definitions
  DatamodelDefinitionHolder(MapType&& definitions) : m_availEDMDefs(std::move(definitions)) {
  }

  DatamodelDefinitionHolder() = default;
  ~DatamodelDefinitionHolder() = default;
  DatamodelDefinitionHolder(const DatamodelDefinitionHolder&) = delete;
  DatamodelDefinitionHolder& operator=(const DatamodelDefinitionHolder&) = delete;
  DatamodelDefinitionHolder(DatamodelDefinitionHolder&&) = default;
  DatamodelDefinitionHolder& operator=(DatamodelDefinitionHolder&&) = default;

  /// Get the datamodel definition for the given datamodel name.
  ///
  /// Returns an empty model definition if no model is stored under the given
  /// name.
  ///
  /// @param name The name of the datamodel
  const std::string_view getDatamodelDefinition(const std::string& name) const;

  /// Get all names of the datamodels that have been read from file
  std::vector<std::string> getAvailableDatamodels() const;

protected:
  MapType m_availEDMDefs{};
};

} // namespace podio

#endif // PODIO_UTILITIES_DATAMODELREGISTRYIOHELPERS_H
