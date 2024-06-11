#include "podio/ROOTWriter.h"
#include "podio/CollectionBase.h"
#include "podio/DatamodelRegistry.h"
#include "podio/Frame.h"
#include "podio/GenericParameters.h"
#include "podio/podioVersion.h"

#include "rootUtils.h"

#include "TTree.h"

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
    catInfo.collsToWrite = root_utils::sortAlphabeticaly(collsToWrite);
    catInfo.tree = new TTree(category.c_str(), (category + " data tree").c_str());
    catInfo.tree->SetDirectory(m_file.get());
  }

  std::vector<StoreCollection> collections;
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
    resetBranches(catInfo.branches, collections, &const_cast<podio::GenericParameters&>(frame.getParameters()));
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

void ROOTWriter::initBranches(CategoryInfo& catInfo, const std::vector<StoreCollection>& collections,
                              /*const*/ podio::GenericParameters& parameters) {
  catInfo.branches.reserve(collections.size() + 1); // collections + parameters

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

    catInfo.branches.push_back(branches);
    catInfo.collInfo.emplace_back(catInfo.idTable.collectionID(name).value(), coll->getTypeName(),
                                  coll->isSubsetCollection(), coll->getSchemaVersion());
  }

  // Also make branches for the parameters
  root_utils::CollectionBranches branches;
  branches.data = catInfo.tree->Branch(root_utils::paramBranchName, &parameters);
  catInfo.branches.push_back(branches);
}

void ROOTWriter::resetBranches(std::vector<root_utils::CollectionBranches>& branches,
                               const std::vector<ROOTWriter::StoreCollection>& collections,
                               /*const*/ podio::GenericParameters* parameters) {
  size_t iColl = 0;
  for (auto& coll : collections) {
    const auto& collBranches = branches[iColl];
    root_utils::setCollectionAddresses(coll.second->getBuffers(), collBranches);
    iColl++;
  }

  branches.back().data->SetAddress(&parameters);
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

} // namespace podio
