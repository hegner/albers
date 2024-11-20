#include "podio/CollectionBufferFactory.h"
#include "podio/CollectionBuffers.h"
#include "podio/ROOTFrameData.h"
#include "rootUtils.h"

// podio specific includes
#include "podio/CollectionBase.h"
#include "podio/CollectionIDTable.h"
#include "podio/GenericParameters.h"
#include "podio/ROOTLegacyReader.h"

// ROOT specific includes
#include "TChain.h"
#include "TClass.h"
#include "TFile.h"
#include "TTree.h"

namespace podio {

std::unique_ptr<ROOTFrameData> ROOTLegacyReader::readNextEntry(const std::string& name) {
  if (name != m_categoryName) {
    return nullptr;
  }
  return readEntry();
}

std::unique_ptr<podio::ROOTFrameData> ROOTLegacyReader::readEntry(const std::string& name, unsigned entry) {
  if (name != m_categoryName) {
    return nullptr;
  }
  m_eventNumber = entry;
  return readEntry();
}

std::unique_ptr<podio::ROOTFrameData> ROOTLegacyReader::readEntry() {
  ROOTFrameData::BufferMap buffers;
  for (const auto& collInfo : m_storedClasses) {
    buffers.emplace(collInfo.first, getCollectionBuffers(collInfo));
  }

  auto parameters = readEventMetaData();

  m_eventNumber++;
  return std::make_unique<ROOTFrameData>(std::move(buffers), m_table, std::move(parameters));
}

podio::CollectionReadBuffers
ROOTLegacyReader::getCollectionBuffers(const std::pair<std::string, detail::CollectionInfo>& collInfo) {
  const auto& name = collInfo.first;
  const auto& [collType, isSubsetColl, schemaVersion, index] = collInfo.second;
  auto& branches = m_collectionBranches[index];

  const auto& bufferFactory = podio::CollectionBufferFactory::instance();
  auto maybeBuffers = bufferFactory.createBuffers(collType, schemaVersion, isSubsetColl);

  // TODO: Error handling of empty optional
  auto collBuffers = maybeBuffers.value_or(podio::CollectionReadBuffers{});

  const auto localEntry = m_chain->LoadTree(m_eventNumber);
  // After switching trees in the chain, branch pointers get invalidated so
  // they need to be reassigned.
  // NOTE: root 6.22/06 requires that we get completely new branches here,
  // with 6.20/04 we could just re-set them
  if (localEntry == 0) {
    branches.data = root_utils::getBranch(m_chain.get(), name.c_str());

    // reference collections
    if (auto* refCollections = collBuffers.references) {
      for (size_t i = 0; i < refCollections->size(); ++i) {
        const auto brName = root_utils::refBranch(name, i);
        branches.refs[i] = root_utils::getBranch(m_chain.get(), brName.c_str());
      }
    }

    // vector members
    if (auto* vecMembers = collBuffers.vectorMembers) {
      for (size_t i = 0; i < vecMembers->size(); ++i) {
        const auto brName = root_utils::vecBranch(name, i);
        branches.vecs[i] = root_utils::getBranch(m_chain.get(), brName.c_str());
      }
    }
  }

  // set the addresses and read the data
  root_utils::setCollectionAddresses(collBuffers, branches);
  root_utils::readBranchesData(branches, localEntry);

  collBuffers.recast(collBuffers);

  return collBuffers;
}

podio::GenericParameters ROOTLegacyReader::readEventMetaData() {
  GenericParameters params;
  auto [tree, entry] = getLocalTreeAndEntry("evt_metadata");
  auto* branch = root_utils::getBranch(tree, "evtMD");
  auto* emd = &params;
  branch->SetAddress(&emd);
  branch->GetEntry(entry);
  return params;
}

void ROOTLegacyReader::openFile(const std::string& filename) {
  openFiles({filename});
}

void ROOTLegacyReader::openFiles(const std::vector<std::string>& filenames) {
  m_chain = std::make_unique<TChain>("events");
  for (const auto& filename : filenames) {
    //-1 forces the headers to be read so that
    // the validity of the files can be checked
    if (!m_chain->Add(filename.c_str(), -1)) {
      throw std::runtime_error("File " + filename + " couldn't be found or the \"events\" tree couldn't be read.");
    }
  }

  // read the meta data and build the collectionBranches cache
  // NOTE: This is a small pessimization, if we do not read all collections
  // afterwards, but it makes the handling much easier in general
  auto metadatatree = static_cast<TTree*>(m_chain->GetFile()->Get("metadata"));
  m_table = std::make_shared<CollectionIDTable>();
  auto* table = m_table.get();
  auto* tableBranch = root_utils::getBranch(metadatatree, "CollectionIDs");
  tableBranch->SetAddress(&table);
  tableBranch->GetEntry(0);

  podio::version::Version* versionPtr{nullptr};
  if (auto* versionBranch = root_utils::getBranch(metadatatree, "PodioVersion")) {
    versionBranch->SetAddress(&versionPtr);
    versionBranch->GetEntry(0);
  }
  m_fileVersion = versionPtr ? *versionPtr : podio::version::Version{0, 0, 0};
  delete versionPtr;

  // Check if the CollectionTypeInfo branch is there and assume that the file
  // has been written with podio pre #197 (<0.13.1) if that is not the case
  if (auto* collInfoBranch = root_utils::getBranch(metadatatree, "CollectionTypeInfo")) {
    auto collectionInfo = new std::vector<root_utils::CollectionWriteInfoT>;

    if (m_fileVersion < podio::version::Version{0, 16, 4}) {
      auto oldCollInfo = new std::vector<root_utils::CollectionInfoWithoutSchemaT>();
      collInfoBranch->SetAddress(&oldCollInfo);
      collInfoBranch->GetEntry(0);
      collectionInfo->reserve(oldCollInfo->size());
      for (auto&& [collID, collType, isSubsetColl] : *oldCollInfo) {
        collectionInfo->emplace_back(collID, std::move(collType), isSubsetColl, 1u);
      }
      delete oldCollInfo;
    } else {
      collInfoBranch->SetAddress(&collectionInfo);
      collInfoBranch->GetEntry(0);
    }
    std::vector<root_utils::CollectionWriteInfo> collInfo;
    collInfo.reserve(collectionInfo->size());
    for (auto& [id, typeName, isSubsetColl, schemaVersion] : *collectionInfo) {
      collInfo.emplace_back(id, std::move(typeName), isSubsetColl, schemaVersion);
    }
    createCollectionBranches(collInfo);
    delete collectionInfo;
  } else {
    std::cout << "PODIO: Reconstructing CollectionTypeInfo branch from other sources in file: \'"
              << m_chain->GetFile()->GetName() << "\'" << std::endl;
    metadatatree->GetEntry(0);
    const auto collectionInfo = root_utils::reconstructCollectionInfo(m_chain.get(), *m_table);
    createCollectionBranches(collectionInfo);
  }
}

unsigned ROOTLegacyReader::getEntries(const std::string& name) const {
  if (name != m_categoryName) {
    return 0;
  }
  return m_chain->GetEntries();
}

void ROOTLegacyReader::createCollectionBranches(const std::vector<root_utils::CollectionWriteInfo>& collInfo) {
  size_t collectionIndex{0};

  for (const auto& [collID, collType, isSubsetColl, collSchemaVersion] : collInfo) {
    // We only write collections that are in the collectionIDTable, so no need
    // to check here
    const auto name = m_table->name(collID).value();

    root_utils::CollectionBranches branches{};
    const auto collectionClass = TClass::GetClass(collType.c_str());

    // Need the collection here to setup all the branches. Have to manage the
    // temporary collection ourselves
    auto collection =
        std::unique_ptr<podio::CollectionBase>(static_cast<podio::CollectionBase*>(collectionClass->New()));
    collection->setSubsetCollection(isSubsetColl);

    if (!isSubsetColl) {
      // This branch is guaranteed to exist since only collections that are
      // also written to file are in the info metadata that we work with here
      branches.data = root_utils::getBranch(m_chain.get(), name.c_str());
    }

    const auto buffers = collection->getBuffers();
    for (size_t i = 0; i < buffers.references->size(); ++i) {
      const auto brName = root_utils::refBranch(name, i);
      branches.refs.push_back(root_utils::getBranch(m_chain.get(), brName.c_str()));
    }

    for (size_t i = 0; i < buffers.vectorMembers->size(); ++i) {
      const auto brName = root_utils::vecBranch(name, i);
      branches.vecs.push_back(root_utils::getBranch(m_chain.get(), brName.c_str()));
    }

    m_storedClasses.emplace_back(name, std::make_tuple(collType, isSubsetColl, collSchemaVersion, collectionIndex++));

    m_collectionBranches.emplace_back(std::move(branches));
  }
}

std::pair<TTree*, unsigned> ROOTLegacyReader::getLocalTreeAndEntry(const std::string& treename) {
  auto localEntry = m_chain->LoadTree(m_eventNumber);
  auto* tree = static_cast<TTree*>(m_chain->GetFile()->Get(treename.c_str()));
  return {tree, localEntry};
}

std::vector<std::string_view> ROOTLegacyReader::getAvailableCategories() const {
  return {m_categoryName};
}

} // namespace podio
