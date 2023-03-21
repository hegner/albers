#include "podio/CollectionBufferFactory.h"
#include "podio/CollectionBuffers.h"

namespace podio {
CollectionBufferFactory& CollectionBufferFactory::mutInstance() {
  static CollectionBufferFactory factory;
  return factory;
}

CollectionBufferFactory const& CollectionBufferFactory::instance() {
  return mutInstance();
}

std::optional<podio::CollectionReadBuffers>
CollectionBufferFactory::createBuffers(const std::string& collType, SchemaVersionT version, bool subsetColl) const {
  if (const auto typeIt = m_funcMap.find(collType); typeIt != m_funcMap.end()) {
    const auto& [_, versionMap] = *typeIt;
    if (versionMap.size() >= version) {
      return versionMap[version - 1](subsetColl);
    }
  }

  return std::nullopt;
}

void CollectionBufferFactory::registerCreationFunc(const std::string& collType, SchemaVersionT version,
                                                   const CreationFuncT& creationFunc) {
  // Try to create an entry for this collection type with a vector that is sized
  // to hold all creation functions for all versions up to the passed schema
  // version
  auto [typeIt, inserted] = m_funcMap.try_emplace(collType, version);
  auto& versionMap = typeIt->second;

  // If we already have something for this type, make sure to handle all
  // versions correctly, assuming that all present creation functions are
  // unchanged and that all non-present creation functions behave the same as
  // this (assumed latest) version
  if (!inserted) {
    const auto prevSize = versionMap.size();
    if (prevSize < version) {
      versionMap.resize(version);
      for (auto i = prevSize; i < version; ++i) {
        versionMap[i] = creationFunc;
      }
    } else {
      // In this case we are explicitly updating one specific version
      versionMap[version - 1] = creationFunc;
    }
  } else {
    // If we have a completely new map, than we simply populate all versions
    // with this creation function
    versionMap.reserve(version);
    for (size_t i = 0; i < version; ++i) {
      versionMap.emplace_back(creationFunc);
    }
  }
}

} // namespace podio
