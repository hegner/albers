#include "podio/utilities/DatamodelRegistryIOHelpers.h"

#include "podio/CollectionBase.h"
#include "podio/DatamodelRegistry.h"

#include <algorithm>
#include <iostream>
#include <iterator>

namespace podio {

void DatamodelDefinitionCollector::registerDatamodelDefinition(const podio::CollectionBase* coll,
                                                               const std::string& name) {
  const auto edmIndex = coll->getDatamodelRegistryIndex();
  if (edmIndex == DatamodelRegistry::NoDefinitionAvailable) {
    std::cerr << "No EDM definition available for collection " << name << std::endl;
  } else {
    if (edmIndex != DatamodelRegistry::NoDefinitionNecessary) {
      m_edmDefRegistryIdcs.insert(edmIndex);
    }
  }
}

std::vector<std::tuple<std::string, std::string>> DatamodelDefinitionCollector::getDatamodelDefinitionsToWrite() const {
  std::vector<std::tuple<std::string, std::string>> edmDefinitions;
  edmDefinitions.reserve(m_edmDefRegistryIdcs.size());
  for (const auto& index : m_edmDefRegistryIdcs) {
    const auto& edmRegistry = podio::DatamodelRegistry::instance();
    edmDefinitions.emplace_back(edmRegistry.getDatamodelName(index), edmRegistry.getDatamodelDefinition(index));
  }

  return edmDefinitions;
}

const std::string_view DatamodelDefinitionHolder::getDatamodelDefinition(const std::string& name) const {
  const auto it = std::find_if(m_availEDMDefs.cbegin(), m_availEDMDefs.cend(),
                               [&name](const auto& entry) { return std::get<0>(entry) == name; });

  if (it != m_availEDMDefs.cend()) {
    return std::get<1>(*it);
  }

  return "{}";
}

std::vector<std::string> DatamodelDefinitionHolder::getAvailableDatamodels() const {
  std::vector<std::string> defs{};
  defs.reserve(m_availEDMDefs.size());
  std::transform(m_availEDMDefs.cbegin(), m_availEDMDefs.cend(), std::back_inserter(defs),
                 [](const auto& elem) { return std::get<0>(elem); });

  return defs;
}

} // namespace podio
