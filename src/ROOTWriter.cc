#include "podio/ROOTWriter.h"
#include "podio/DatamodelRegistry.h"
#include "podio/Frame.h"
#include "podio/GenericParameters.h"
#include "podio/podioVersion.h"

#include "podio/utilities/DatamodelRegistryIOHelpers.h"
#include "rootUtils.h"

#include "TTree.h"
#include <tuple>

namespace podio {

ROOTWriter::ROOTWriter(const std::string& filename) {
  m_file = std::make_unique<TFile>(filename.c_str(), "recreate");
}

ROOTWriter::~ROOTWriter() {
  if (!m_finished) {
    finish();
  }
}

void ROOTWriter::writeFrame(const podio::Frame& frame, const std::string& category) {
  writeFrame(frame, category, frame.getAvailableCollections());
}

void ROOTWriter::writeFrame(const podio::Frame& frame, const std::string& category,
                            const std::vector<std::string>& collsToWrite) {
  auto& catInfo = getCategoryInfo(category);
  // Use the TTree as proxy here to decide whether this category has already
  // been initialized
  if (catInfo.tree == nullptr) {
    catInfo.idTable = frame.getCollectionIDTableForWrite();
    catInfo.collsToWrite = podio::utils::sortAlphabeticaly(collsToWrite);
    catInfo.tree = new TTree(category.c_str(), (category + " data tree").c_str());
    catInfo.tree->SetDirectory(m_file.get());
  }

  std::vector<root_utils::StoreCollection> collections;
  collections.reserve(catInfo.collsToWrite.size());
  for (const auto& name : catInfo.collsToWrite) {
    auto* coll = frame.getCollectionForWrite(name);
    if (!coll) {
      // Make sure all collections that we want to write are actually available
      // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
      throw std::runtime_error("Collection '" + name + "' in category '" + category + "' is not available in Frame");
    }
    collections.emplace_back(name, const_cast<podio::CollectionBase*>(coll));
  }

  // We will at least have a parameters branch, even if there are no
  // collections
  if (catInfo.branches.empty()) {
    initBranches(catInfo, collections, const_cast<podio::GenericParameters&>(frame.getParameters()));

  } else {
    // Make sure that the category contents are consistent with the initial
    // frame in the category
    if (!root_utils::checkConsistentColls(catInfo.collsToWrite, collsToWrite)) {
      throw std::runtime_error("Trying to write category '" + category + "' with inconsistent collection content. " +
                               root_utils::getInconsistentCollsMsg(catInfo.collsToWrite, collsToWrite));
    }
    fillParams(catInfo, frame.getParameters());
    resetBranches(catInfo, collections);
  }

  catInfo.tree->Fill();
}

ROOTWriter::CategoryInfo& ROOTWriter::getCategoryInfo(const std::string& category) {
  if (auto it = m_categories.find(category); it != m_categories.end()) {
    return it->second;
  }

  auto [it, _] = m_categories.try_emplace(category, CategoryInfo{});
  return it->second;
}

void ROOTWriter::initBranches(CategoryInfo& catInfo, const std::vector<root_utils::StoreCollection>& collections,
                              /*const*/ podio::GenericParameters& parameters) {
  catInfo.branches.reserve(collections.size() + root_utils::nParamBranches); // collections + parameters

  // First collections
  for (auto& [name, coll] : collections) {
    // For the first entry in each category we also record the datamodel
    // definition
    m_datamodelCollector.registerDatamodelDefinition(coll, name);

    root_utils::CollectionBranches branches;
    const auto buffers = coll->getBuffers();
    // For subset collections we only fill one references branch
    if (coll->isSubsetCollection()) {
      auto& refColl = (*buffers.references)[0];
      const auto brName = root_utils::subsetBranch(name);
      branches.refs.push_back(catInfo.tree->Branch(brName.c_str(), refColl.get()));
    } else {
      // For "proper" collections we populate all branches, starting with the data
      const auto bufferDataType = "vector<" + std::string(coll->getDataTypeName()) + ">";
      branches.data = catInfo.tree->Branch(name.c_str(), bufferDataType.c_str(), buffers.data);

      const auto relVecNames = podio::DatamodelRegistry::instance().getRelationNames(coll->getValueTypeName());
      if (auto refColls = buffers.references) {
        int i = 0;
        for (auto& c : (*refColls)) {
          const auto brName = root_utils::refBranch(name, relVecNames.relations[i++]);
          branches.refs.push_back(catInfo.tree->Branch(brName.c_str(), c.get()));
        }
      }

      if (auto vmInfo = buffers.vectorMembers) {
        int i = 0;
        for (auto& [type, vec] : (*vmInfo)) {
          const auto typeName = "vector<" + type + ">";
          const auto brName = root_utils::vecBranch(name, relVecNames.vectorMembers[i++]);
          branches.vecs.push_back(catInfo.tree->Branch(brName.c_str(), typeName.c_str(), vec));
        }
      }
    }

    catInfo.branches.emplace_back(std::move(branches));
    catInfo.collInfo.emplace_back(catInfo.idTable.collectionID(name).value(), std::string(coll->getTypeName()),
                                  coll->isSubsetCollection(), coll->getSchemaVersion());
  }

  fillParams(catInfo, parameters);
  // NOTE: The order in which these are created is codified for later use in
  // root_utils::getGPBranchOffsets
  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::intKeyName, &catInfo.intParams.keys));
  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::intValueName, &catInfo.intParams.values));

  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::floatKeyName, &catInfo.floatParams.keys));
  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::floatValueName, &catInfo.floatParams.values));

  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::doubleKeyName, &catInfo.doubleParams.keys));
  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::doubleValueName, &catInfo.doubleParams.values));

  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::stringKeyName, &catInfo.stringParams.keys));
  catInfo.branches.emplace_back(catInfo.tree->Branch(root_utils::stringValueName, &catInfo.stringParams.values));
}

void ROOTWriter::resetBranches(CategoryInfo& categoryInfo,
                               const std::vector<root_utils::StoreCollection>& collections) {
  size_t iColl = 0;
  for (auto& [_, coll] : collections) {
    const auto& collBranches = categoryInfo.branches[iColl];
    root_utils::setCollectionAddresses(coll->getBuffers(), collBranches);
    iColl++;
  }
  // Correct index to point to the last branch of collection data for symmetric
  // handling of the offsets in reading and writing
  iColl--;

  constexpr auto intOffset = root_utils::getGPBranchOffsets<int>();
  categoryInfo.branches[iColl + intOffset.keys].data->SetAddress(categoryInfo.intParams.keysPtr());
  categoryInfo.branches[iColl + intOffset.values].data->SetAddress(categoryInfo.intParams.valuesPtr());

  constexpr auto floatOffset = root_utils::getGPBranchOffsets<float>();
  categoryInfo.branches[iColl + floatOffset.keys].data->SetAddress(categoryInfo.floatParams.keysPtr());
  categoryInfo.branches[iColl + floatOffset.values].data->SetAddress(categoryInfo.floatParams.valuesPtr());

  constexpr auto doubleOffset = root_utils::getGPBranchOffsets<double>();
  categoryInfo.branches[iColl + doubleOffset.keys].data->SetAddress(categoryInfo.doubleParams.keysPtr());
  categoryInfo.branches[iColl + doubleOffset.values].data->SetAddress(categoryInfo.doubleParams.valuesPtr());

  constexpr auto stringOffset = root_utils::getGPBranchOffsets<std::string>();
  categoryInfo.branches[iColl + stringOffset.keys].data->SetAddress(categoryInfo.stringParams.keysPtr());
  categoryInfo.branches[iColl + stringOffset.values].data->SetAddress(categoryInfo.stringParams.valuesPtr());
}

void ROOTWriter::finish() {
  auto* metaTree = new TTree(root_utils::metaTreeName, "metadata tree for podio I/O functionality");
  metaTree->SetDirectory(m_file.get());

  // Store the collection id table and collection info for reading in the meta tree
  for (/*const*/ auto& [category, info] : m_categories) {
    metaTree->Branch(root_utils::idTableName(category).c_str(), &info.idTable);
    metaTree->Branch(root_utils::collInfoName(category).c_str(), &info.collInfo);
  }

  // Store the current podio build version into the meta data tree
  auto podioVersion = podio::version::build_version;
  metaTree->Branch(root_utils::versionBranchName, &podioVersion);

  auto edmDefinitions = m_datamodelCollector.getDatamodelDefinitionsToWrite();
  metaTree->Branch(root_utils::edmDefBranchName, &edmDefinitions);

  // Collect the (build) versions of the generated datamodels where available
  DatamodelDefinitionHolder::VersionList edmVersions;
  for (const auto& [name, _] : edmDefinitions) {
    auto edmVersion = podio::DatamodelRegistry::instance().getDatamodelVersion(name);
    if (edmVersion) {
      edmVersions.emplace_back(name, edmVersion.value());
    }
  }
  for (auto& [name, version] : edmVersions) {
    metaTree->Branch(root_utils::edmVersionBranchName(name).c_str(), &version);
  }

  metaTree->Fill();

  m_file->Write();
  m_file->Close();

  m_finished = true;
}

std::tuple<std::vector<std::string>, std::vector<std::string>>
ROOTWriter::checkConsistency(const std::vector<std::string>& collsToWrite, const std::string& category) const {
  if (const auto it = m_categories.find(category); it != m_categories.end()) {
    return root_utils::getInconsistentColls(it->second.collsToWrite, collsToWrite);
  }

  return {std::vector<std::string>{}, collsToWrite};
}

void ROOTWriter::fillParams(CategoryInfo& catInfo, const GenericParameters& params) {
  catInfo.intParams = params.getKeysAndValues<int>();
  catInfo.floatParams = params.getKeysAndValues<float>();
  catInfo.doubleParams = params.getKeysAndValues<double>();
  catInfo.stringParams = params.getKeysAndValues<std::string>();
}

} // namespace podio
