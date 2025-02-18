#include "podio/ROOTReader.h"
#include "podio/CollectionBase.h"
#include "podio/CollectionBufferFactory.h"
#include "podio/CollectionBuffers.h"
#include "podio/CollectionIDTable.h"
#include "podio/DatamodelRegistry.h"
#include "podio/GenericParameters.h"
#include "podio/utilities/RootHelpers.h"
#include "rootUtils.h"

// ROOT specific includes
#include "TChain.h"
#include "TClass.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace podio {

std::tuple<std::vector<root_utils::CollectionBranches>, std::vector<detail::NamedCollInfo>>
createCollectionBranches(TChain* chain, const podio::CollectionIDTable& idTable,
                         const std::vector<root_utils::CollectionWriteInfo>& collInfo);

std::tuple<std::vector<root_utils::CollectionBranches>, std::vector<detail::NamedCollInfo>>
createCollectionBranchesIndexBased(TChain* chain, const podio::CollectionIDTable& idTable,
                                   const std::vector<root_utils::CollectionWriteInfo>& collInfo);

template <typename T>
void ROOTReader::readParams(ROOTReader::CategoryInfo& catInfo, podio::GenericParameters& params, bool reloadBranches,
                            unsigned int localEntry) {
  const auto collBranchIdx = catInfo.branches.size() - root_utils::nParamBranches - 1;
  constexpr auto brOffset = root_utils::getGPBranchOffsets<T>();

  if (reloadBranches) {
    auto& keyBranch = catInfo.branches[collBranchIdx + brOffset.keys].data;
    keyBranch = root_utils::getBranch(catInfo.chain.get(), root_utils::getGPKeyName<T>());
    auto& valueBranch = catInfo.branches[collBranchIdx + brOffset.values].data;
    valueBranch = root_utils::getBranch(catInfo.chain.get(), root_utils::getGPValueName<T>());
  }

  auto keyBranch = catInfo.branches[collBranchIdx + brOffset.keys].data;
  auto valueBranch = catInfo.branches[collBranchIdx + brOffset.values].data;

  root_utils::ParamStorage<T> storage;
  keyBranch->SetAddress(storage.keysPtr());
  keyBranch->GetEntry(localEntry);
  valueBranch->SetAddress(storage.valuesPtr());
  valueBranch->GetEntry(localEntry);

  params.loadFrom(std::move(storage.keys), std::move(storage.values));
}

GenericParameters ROOTReader::readEntryParameters(ROOTReader::CategoryInfo& catInfo, bool reloadBranches,
                                                  unsigned int localEntry) {
  GenericParameters params;

  if (m_fileVersion < podio::version::Version{0, 99, 99}) {
    // Parameter branch is always the last one
    auto& paramBranches = catInfo.branches.back();

    // Make sure to have a valid branch pointer after switching trees in the chain
    // as well as on the first event
    if (reloadBranches) {
      paramBranches.data = root_utils::getBranch(catInfo.chain.get(), root_utils::paramBranchName);
    }
    auto* branch = paramBranches.data;

    auto* emd = &params;
    branch->SetAddress(&emd);
    branch->GetEntry(localEntry);
  } else {
    readParams<int>(catInfo, params, reloadBranches, localEntry);
    readParams<float>(catInfo, params, reloadBranches, localEntry);
    readParams<double>(catInfo, params, reloadBranches, localEntry);
    readParams<std::string>(catInfo, params, reloadBranches, localEntry);
  }

  return params;
}

std::unique_ptr<ROOTFrameData> ROOTReader::readNextEntry(const std::string& name,
                                                         const std::vector<std::string>& collsToRead) {
  auto& catInfo = getCategoryInfo(name);
  return readEntry(catInfo, collsToRead);
}

std::unique_ptr<ROOTFrameData> ROOTReader::readEntry(const std::string& name, const unsigned entNum,
                                                     const std::vector<std::string>& collsToRead) {
  auto& catInfo = getCategoryInfo(name);
  catInfo.entry = entNum;
  return readEntry(catInfo, collsToRead);
}

std::unique_ptr<ROOTFrameData> ROOTReader::readEntry(ROOTReader::CategoryInfo& catInfo,
                                                     const std::vector<std::string>& collsToRead) {
  if (!catInfo.chain) {
    return nullptr;
  }
  if (catInfo.entry >= catInfo.chain->GetEntries()) {
    return nullptr;
  }

  // Make sure to not silently ignore non-existant but requested collections
  if (!collsToRead.empty()) {
    for (const auto& name : collsToRead) {
      if (std::ranges::find(catInfo.storedClasses, name, &detail::NamedCollInfo::name) == catInfo.storedClasses.end()) {
        throw std::invalid_argument(name + " is not available from Frame");
      }
    }
  }

  // After switching trees in the chain, branch pointers get invalidated so
  // they need to be reassigned.
  // NOTE: root 6.22/06 requires that we get completely new branches here,
  // with 6.20/04 we could just re-set them
  const auto preTreeNo = catInfo.chain->GetTreeNumber();
  const auto localEntry = catInfo.chain->LoadTree(catInfo.entry);
  const auto treeChange = catInfo.chain->GetTreeNumber() != preTreeNo;
  // Also need to make sure to handle the first event
  const auto reloadBranches = treeChange || localEntry == 0;

  ROOTFrameData::BufferMap buffers;
  for (size_t i = 0; i < catInfo.storedClasses.size(); ++i) {
    if (!collsToRead.empty() && std::ranges::find(collsToRead, catInfo.storedClasses[i].name) == collsToRead.end()) {
      continue;
    }
    buffers.emplace(catInfo.storedClasses[i].name, getCollectionBuffers(catInfo, i, reloadBranches, localEntry));
  }

  auto parameters = readEntryParameters(catInfo, reloadBranches, localEntry);

  catInfo.entry++;
  return std::make_unique<ROOTFrameData>(std::move(buffers), catInfo.table, std::move(parameters));
}

podio::CollectionReadBuffers ROOTReader::getCollectionBuffers(ROOTReader::CategoryInfo& catInfo, size_t iColl,
                                                              bool reloadBranches, unsigned int localEntry) {
  const auto& name = catInfo.storedClasses[iColl].name;
  const auto& [collType, isSubsetColl, schemaVersion, index] = catInfo.storedClasses[iColl].info;
  auto& branches = catInfo.branches[index];

  const auto& bufferFactory = podio::CollectionBufferFactory::instance();
  auto maybeBuffers = bufferFactory.createBuffers(collType, schemaVersion, isSubsetColl);

  // TODO: Error handling of empty optional
  auto collBuffers = maybeBuffers.value_or(podio::CollectionReadBuffers{});

  if (reloadBranches) {
    root_utils::resetBranches(catInfo.chain.get(), branches, name);
  }

  // set the addresses and read the data
  root_utils::setCollectionAddresses(collBuffers, branches);
  root_utils::readBranchesData(branches, localEntry);

  collBuffers.recast(collBuffers);

  return collBuffers;
}

ROOTReader::CategoryInfo& ROOTReader::getCategoryInfo(const std::string& category) {
  if (auto it = m_categories.find(category); it != m_categories.end()) {
    // Use the id table as proxy to check whether this category has been
    // initialized already
    if (it->second.table == nullptr) {
      initCategory(it->second, category);
    }
    return it->second;
  }

  // Use a nullptr TChain to signify an invalid category request
  // TODO: Warn / log
  static auto invalidCategory = CategoryInfo{nullptr};

  return invalidCategory;
}

void ROOTReader::initCategory(CategoryInfo& catInfo, const std::string& category) {
  catInfo.table = std::make_shared<podio::CollectionIDTable>();
  auto* table = catInfo.table.get();
  auto* tableBranch = root_utils::getBranch(m_metaChain.get(), root_utils::idTableName(category));
  tableBranch->SetAddress(&table);
  tableBranch->GetEntry(0);

  auto* collInfoBranch = root_utils::getBranch(m_metaChain.get(), root_utils::collInfoName(category));

  auto collInfo = new std::vector<root_utils::CollectionWriteInfo>();

  if (m_fileVersion >= podio::version::Version{1, 1, 0}) {
    collInfoBranch->SetAddress(&collInfo);
    collInfoBranch->GetEntry(0);
  } else {
    auto collInfoOld = new std::vector<root_utils::CollectionWriteInfoT>();
    if (m_fileVersion < podio::version::Version{0, 16, 4}) {
      auto collInfoReallyOld = new std::vector<root_utils::CollectionInfoWithoutSchemaT>();
      collInfoBranch->SetAddress(&collInfoReallyOld);
      collInfoBranch->GetEntry(0);
      collInfoOld->reserve(collInfoReallyOld->size());
      for (auto& [collID, collType, isSubsetColl] : *collInfoReallyOld) {
        // Manually set the schema version to 1
        collInfo->emplace_back(collID, std::move(collType), isSubsetColl, 1u);
      }
      delete collInfoReallyOld;
    } else {
      collInfoBranch->SetAddress(&collInfoOld);
      collInfoBranch->GetEntry(0);
    }
    // "Convert" to new style
    collInfo->reserve(collInfoOld->size());
    for (auto& [id, typeName, isSubsetColl, schemaVersion] : *collInfoOld) {
      collInfo->emplace_back(id, std::move(typeName), isSubsetColl, schemaVersion);
    }
    delete collInfoOld;
  }

  // For backwards compatibility make it possible to read the index based files
  // from older versions
  if (m_fileVersion < podio::version::Version{0, 16, 99}) {
    std::tie(catInfo.branches, catInfo.storedClasses) =
        createCollectionBranchesIndexBased(catInfo.chain.get(), *catInfo.table, *collInfo);
  } else {
    std::tie(catInfo.branches, catInfo.storedClasses) =
        createCollectionBranches(catInfo.chain.get(), *catInfo.table, *collInfo);
  }
  delete collInfo;

  // Finally set up the branches for the parameters
  if (m_fileVersion < podio::version::Version{0, 99, 99}) {
    root_utils::CollectionBranches paramBranches{};
    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::paramBranchName));
  } else {
    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::intKeyName));
    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::intValueName));

    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::floatKeyName));
    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::floatValueName));

    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::doubleKeyName));
    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::doubleValueName));

    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::stringKeyName));
    catInfo.branches.emplace_back(root_utils::getBranch(catInfo.chain.get(), root_utils::stringValueName));
  }
}

std::vector<std::string> getAvailableCategories(TChain* metaChain) {
  auto* branches = metaChain->GetListOfBranches();
  std::vector<std::string> brNames;
  brNames.reserve(branches->GetEntries());

  for (int i = 0; i < branches->GetEntries(); ++i) {
    const std::string name = branches->At(i)->GetName();
    const auto fUnder = name.find(root_utils::idTableName(""));
    if (fUnder != std::string::npos) {
      brNames.emplace_back(name.substr(0, fUnder));
    }
  }

  std::ranges::sort(brNames);
  brNames.erase(std::unique(brNames.begin(), brNames.end()), brNames.end());
  return brNames;
}

void ROOTReader::openFile(const std::string& filename) {
  openFiles({filename});
}

void ROOTReader::openFiles(const std::vector<std::string>& filenames) {
  m_metaChain = std::make_unique<TChain>(root_utils::metaTreeName);
  // NOTE: We simply assume that the meta data doesn't change throughout the
  // chain! This essentially boils down to the assumption that all files that
  // are read this way were written with the same settings.
  // Reading all files is done to check that all file exists
  for (const auto& filename : filenames) {
    if (!m_metaChain->Add(filename.c_str(), -1)) {
      throw std::runtime_error("File " + filename + " couldn't be found or the \"" + root_utils::metaTreeName +
                               "\" tree couldn't be read.");
    }
  }

  podio::version::Version* versionPtr{nullptr};
  if (auto* versionBranch = root_utils::getBranch(m_metaChain.get(), root_utils::versionBranchName)) {
    versionBranch->SetAddress(&versionPtr);
    versionBranch->GetEntry(0);
  }
  m_fileVersion = versionPtr ? *versionPtr : podio::version::Version{0, 0, 0};
  delete versionPtr;

  if (auto* edmDefBranch = root_utils::getBranch(m_metaChain.get(), root_utils::edmDefBranchName)) {
    auto* datamodelDefs = new DatamodelDefinitionHolder::MapType{};
    edmDefBranch->SetAddress(&datamodelDefs);
    edmDefBranch->GetEntry(0);

    DatamodelDefinitionHolder::VersionList edmVersions{};
    for (const auto& [name, _] : *datamodelDefs) {
      if (auto* edmVersionBranch = root_utils::getBranch(m_metaChain.get(), root_utils::edmVersionBranchName(name))) {
        auto* edmVersion = new podio::version::Version{};
        edmVersionBranch->SetAddress(&edmVersion);
        edmVersionBranch->GetEntry(0);
        edmVersions.emplace_back(name, *edmVersion);
        delete edmVersion;
      }
    }

    m_datamodelHolder = DatamodelDefinitionHolder(std::move(*datamodelDefs), std::move(edmVersions));
    delete datamodelDefs;
  }

  // Do some work up front for setting up categories and setup all the chains
  // and record the available categories. The rest of the setup follows on
  // demand when the category is first read
  m_availCategories = ::podio::getAvailableCategories(m_metaChain.get());
  for (const auto& cat : m_availCategories) {
    auto [it, _] = m_categories.try_emplace(cat, std::make_unique<TChain>(cat.c_str()));
    for (const auto& fn : filenames) {
      it->second.chain->Add(fn.c_str());
    }
  }
}

unsigned ROOTReader::getEntries(const std::string& name) const {
  if (auto it = m_categories.find(name); it != m_categories.end()) {
    return it->second.chain->GetEntries();
  }

  return 0;
}

std::vector<std::string_view> ROOTReader::getAvailableCategories() const {
  std::vector<std::string_view> cats;
  cats.reserve(m_categories.size());
  for (const auto& [cat, _] : m_categories) {
    cats.emplace_back(cat);
  }
  return cats;
}

std::tuple<std::vector<root_utils::CollectionBranches>, std::vector<detail::NamedCollInfo>>
createCollectionBranchesIndexBased(TChain* chain, const podio::CollectionIDTable& idTable,
                                   const std::vector<root_utils::CollectionWriteInfo>& collInfo) {

  size_t collectionIndex{0};
  std::vector<root_utils::CollectionBranches> collBranches;
  collBranches.reserve(collInfo.size() + 1);
  std::vector<detail::NamedCollInfo> storedClasses;
  storedClasses.reserve(collInfo.size());

  for (const auto& [collID, collType, isSubsetColl, collSchemaVersion] : collInfo) {
    // We only write collections that are in the collectionIDTable, so no need
    // to check here
    const auto name = idTable.name(collID).value();

    const auto collectionClass = TClass::GetClass(collType.c_str());
    // Need the collection here to setup all the branches. Have to manage the
    // temporary collection ourselves
    auto collection =
        std::unique_ptr<podio::CollectionBase>(static_cast<podio::CollectionBase*>(collectionClass->New()));
    root_utils::CollectionBranches branches{};
    if (isSubsetColl) {
      // Only one branch will exist and we can trivially get its name
      auto brName = root_utils::refBranch(name, 0);
      branches.refs.push_back(root_utils::getBranch(chain, brName.c_str()));
      branches.refNames.emplace_back(std::move(brName));
    } else {
      // This branch is guaranteed to exist since only collections that are
      // also written to file are in the info metadata that we work with here
      branches.data = root_utils::getBranch(chain, name.c_str());

      const auto buffers = collection->getBuffers();
      for (size_t i = 0; i < buffers.references->size(); ++i) {
        auto brName = root_utils::refBranch(name, i);
        branches.refs.push_back(root_utils::getBranch(chain, brName.c_str()));
        branches.refNames.emplace_back(std::move(brName));
      }

      for (size_t i = 0; i < buffers.vectorMembers->size(); ++i) {
        auto brName = root_utils::vecBranch(name, i);
        branches.vecs.push_back(root_utils::getBranch(chain, brName.c_str()));
        branches.vecNames.emplace_back(std::move(brName));
      }
    }

    storedClasses.emplace_back(name, std::make_tuple(collType, isSubsetColl, collSchemaVersion, collectionIndex++));
    collBranches.emplace_back(std::move(branches));
  }

  return {std::move(collBranches), storedClasses};
}

std::tuple<std::vector<root_utils::CollectionBranches>, std::vector<detail::NamedCollInfo>>
createCollectionBranches(TChain* chain, const podio::CollectionIDTable& idTable,
                         const std::vector<root_utils::CollectionWriteInfo>& collInfo) {

  size_t collectionIndex{0};
  std::vector<root_utils::CollectionBranches> collBranches;
  collBranches.reserve(collInfo.size() + 1);
  std::vector<detail::NamedCollInfo> storedClasses;
  storedClasses.reserve(collInfo.size());

  for (const auto& [collID, collType, isSubsetColl, collSchemaVersion] : collInfo) {
    // We only write collections that are in the collectionIDTable, so no need
    // to check here
    const auto name = idTable.name(collID).value();

    root_utils::CollectionBranches branches{};
    if (isSubsetColl) {
      // Only one branch will exist and we can trivially get its name
      auto brName = root_utils::subsetBranch(name);
      branches.refs.push_back(root_utils::getBranch(chain, brName.c_str()));
      branches.refNames.emplace_back(std::move(brName));
    } else {
      // This branch is guaranteed to exist since only collections that are
      // also written to file are in the info metadata that we work with here
      branches.data = root_utils::getBranch(chain, name.c_str());

      const auto relVecNames = podio::DatamodelRegistry::instance().getRelationNames(collType);
      for (const auto& relName : relVecNames.relations) {
        auto brName = root_utils::refBranch(name, relName);
        branches.refs.push_back(root_utils::getBranch(chain, brName.c_str()));
        branches.refNames.emplace_back(std::move(brName));
      }
      for (const auto& vecName : relVecNames.vectorMembers) {
        auto brName = root_utils::refBranch(name, vecName);
        branches.vecs.push_back(root_utils::getBranch(chain, brName.c_str()));
        branches.vecNames.emplace_back(std::move(brName));
      }
    }

    storedClasses.emplace_back(name, std::make_tuple(collType, isSubsetColl, collSchemaVersion, collectionIndex++));
    collBranches.emplace_back(std::move(branches));
  }

  return {std::move(collBranches), storedClasses};
}

} // namespace podio
