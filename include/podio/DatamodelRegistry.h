#ifndef PODIO_DATAMODELREGISTRY_H
#define PODIO_DATAMODELREGISTRY_H

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace podio {

/// Type alias for storing the names of all Relations and VectorMembers for all
/// datatypes of an EDM. Populated for each EDM at code generation time.
/// The structure is of each element in the outer vector is:
/// - get<0>: The name of the datatype
/// - get<1>: The names of all Relations, where OneToManyRelations comes before
///   OneToOneRelations (in the order as they appear in the YAML file)
/// - get<2>: The names of all VectorMembers (in the order of the file YAML)
using RelationNameMapping =
    std::vector<std::tuple<std::string_view, std::vector<std::string_view>, std::vector<std::string_view>>>;

/// Information on the names of the OneTo[One|Many]Relations as well as the
/// VectorMembers of a datatype
///
/// The contents are populated by the code generation, where we simply generate
/// static vectors that we make available as const& here.
struct RelationNames {
  /// The names of the relations (OneToMany before OneToOne)
  const std::vector<std::string_view>& relations;
  /// The names of the vector members
  const std::vector<std::string_view>& vectorMembers;
};

/// Global registry holding information about datamodels and datatypes defined
/// therein that are currently known by podio (i.e. which have been dynamically
/// loaded).
///
/// This is a singleton which is (statically) populated during dynamic loading
/// of generated EDMs. In this context an **EDM refers to the shared library**
/// that is compiled from the generated code from a datamodel definition in YAML
/// format. When we refer to a **datamodel** in this context we talk about the
/// entity as a whole, i.e. its definition in a YAML file, but also the concrete
/// implementation as an EDM, as well as all other information that is related
/// to it. In the API of this registry this will be used, unless we want to
/// highlight that we are referring to a specific part of a datamodel.
class DatamodelRegistry {
public:
  /// Get the registry
  static const DatamodelRegistry& instance();

  // Mutable instance only used for the initial registration!
  static DatamodelRegistry& mutInstance();

  ~DatamodelRegistry() = default;
  DatamodelRegistry(const DatamodelRegistry&) = delete;
  DatamodelRegistry& operator=(const DatamodelRegistry&) = delete;
  DatamodelRegistry(DatamodelRegistry&&) = delete;
  DatamodelRegistry& operator=(const DatamodelRegistry&&) = delete;

  /// Dedicated index value for collections that don't have a datamodel
  /// definition (e.g. UserDataCollection)
  static constexpr size_t NoDefinitionNecessary = -1;
  /// Dedicated index value for error checking, used to default init the generated RegistryIndex
  static constexpr size_t NoDefinitionAvailable = -2;

  /// Get the definition (in JSON format) of the datamodel with the given
  /// edmName.
  ///
  /// If no datamodel with the given name can be found, an empty datamodel
  /// definition, i.e. an empty JSON object ("{}"), is returned.
  ///
  /// @param name The name of the datamodel
  ///
  /// @returns The high level definition of the datamodel in JSON format
  const std::string_view getDatamodelDefinition(std::string_view name) const;

  /// Get the definition (in JSON format) of the datamodel with the given
  /// index.
  ///
  /// If no datamodel is found under the given index, an empty datamodel
  /// definition, i.e. an empty JSON object ("{}"), is returned.
  ///
  /// @param index The datamodel definition index that can be obtained from each
  ///              collection
  ///
  /// @returns The high level definition of the datamodel in JSON format
  const std::string_view getDatamodelDefinition(size_t index) const;

  /// Get the name of the datamodel that is stored under the given index.
  ///
  /// If no datamodel is found under the given index, an empty string is returned
  ///
  /// @param index The datamodel definition index that can be obtained from each
  ///              collection
  ///
  /// @returns The name of the datamodel
  const std::string& getDatamodelName(size_t index) const;

  /// Register a datamodel and return its index in the registry.
  ///
  /// This is the hook that is called during dynamic loading of an EDM to
  /// register information for this EDM. If an EDM has already been registered
  /// under this name, than the index to the existing EDM in the registry will be
  /// returned.
  ///
  /// @param name The name of the EDM that should be registered
  /// @param definition The datamodel definition from which this EDM has been
  /// generated in JSON format
  /// @param relationNames the names of the relations and vector members for all
  /// datatypes that are defined for this EDM
  ///
  /// @returns The index of this datamodel in the registry
  size_t registerDatamodel(std::string name, std::string_view definition,
                           const podio::RelationNameMapping& relationNames);

  /// Get the names of the relations and vector members of a datatype
  RelationNames getRelationNames(std::string_view typeName) const;

private:
  DatamodelRegistry() = default;
  /// The stored definitions
  std::vector<std::pair<std::string, std::string_view>> m_definitions{};

  std::unordered_map<std::string_view, RelationNames> m_relations{};
};
} // namespace podio

#endif // PODIO_DATAMODELREGISTRY_H
