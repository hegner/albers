#include "podio/SchemaEvolution.h"

#include "podio/CollectionBuffers.h"

#include <iostream>
#include <tuple>
#include <utility>

namespace podio {

SchemaEvolution& SchemaEvolution::mutInstance() {
  static SchemaEvolution instance;
  return instance;
}

SchemaEvolution const& SchemaEvolution::instance() {
  return mutInstance();
}

podio::CollectionReadBuffers SchemaEvolution::evolveBuffers(const podio::CollectionReadBuffers& oldBuffers,
                                                            SchemaVersionT fromVersion,
                                                            const std::string& collType) const {
  if (const auto typeIt = m_versionMapIndices.find(collType); typeIt != m_versionMapIndices.end()) {
    const auto [currentVersion, mapIndex] = typeIt->second;
    if (fromVersion == currentVersion) {
      return oldBuffers; // Nothing to do here
    }

    const auto& typeEvolFuncs = m_evolutionFuncs[mapIndex];
    if (fromVersion < typeEvolFuncs.size()) {
      // Do we need this check? In principle we could ensure at registration
      // time that this is always guaranteed
      return typeEvolFuncs[fromVersion - 1](oldBuffers, fromVersion);
    }
  }

  std::cerr << "PODIO WARNING: evolveBuffers has no knowledge of how to evolve buffers for " << collType
            << " from version " << fromVersion << std::endl; // TODO: exception
  return oldBuffers;
}

void SchemaEvolution::registerEvolutionFunc(const std::string& collType, SchemaVersionT fromVersion,
                                            SchemaVersionT currentVersion, const EvolutionFuncT& evolutionFunc,
                                            Priority priority) {
  auto typeIt = m_versionMapIndices.find(collType);
  if (typeIt == m_versionMapIndices.end()) {
    // Create an entry for this type
    std::tie(typeIt, std::ignore) =
        m_versionMapIndices.emplace(collType, MapIndex{currentVersion, MapIndex::NoEvolutionAvailable});
  }

  // If we do not have any evolution funcs yet, create the necessary mapping
  // structure and update the index
  if (typeIt->second.index == MapIndex::NoEvolutionAvailable) {
    typeIt->second.index = m_evolutionFuncs.size();
    m_evolutionFuncs.emplace_back();
  }

  // From here on out we don't need the mutable any longer
  const auto& [_, mapIndex] = typeIt->second;

  auto& versionMap = m_evolutionFuncs[mapIndex];
  const auto prevSize = versionMap.size();
  if (prevSize < currentVersion) {
    versionMap.resize(currentVersion);
  }

  versionMap[fromVersion - 1] = evolutionFunc;
  if (priority != Priority::UserDefined) {
    //  std::cerr << "Not updating evolution function because priority is not UserDefined" << std::endl;
  }
}

podio::CollectionReadBuffers SchemaEvolution::noOpSchemaEvolution(podio::CollectionReadBuffers&& buffers,
                                                                  SchemaVersionT) {
  return buffers;
}

} // namespace podio
