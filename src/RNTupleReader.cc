#include "podio/RNTupleReader.h"

#include "podio/CollectionBufferFactory.h"
#include "podio/CollectionBuffers.h"
#include "podio/CollectionIDTable.h"
#include "podio/DatamodelRegistry.h"
#include "podio/GenericParameters.h"
#include "podio/ObjectID.h"
#include "rootUtils.h"
#include <ROOT/REntry.hxx>
#include <ROOT/RError.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleView.hxx>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <tuple>
#include <utility>

namespace podio {

template <typename T>
void RNTupleReader::readParams(const std::string& name, unsigned entNum, GenericParameters& params) {
  auto keyView = m_readers[name][0]->GetView<std::vector<std::string>>(root_utils::getGPKeyName<T>());
  auto valueView = m_readers[name][0]->GetView<std::vector<std::vector<T>>>(root_utils::getGPValueName<T>());

  params.loadFrom(keyView(entNum), valueView(entNum));
}

GenericParameters RNTupleReader::readEventMetaData(const std::string& name, unsigned entNum) {
  GenericParameters params;

  readParams<int>(name, entNum, params);
  readParams<float>(name, entNum, params);
  readParams<double>(name, entNum, params);
  readParams<std::string>(name, entNum, params);

  return params;
}

bool RNTupleReader::initCategory(const std::string& category) {
  if (std::find(m_availableCategories.begin(), m_availableCategories.end(), category) == m_availableCategories.end()) {
    return false;
  }
  // Assume that the metadata is the same in all files
  auto filename = m_filenames[0];

  auto id = m_metadata_readers[filename]->GetView<std::vector<unsigned int>>(root_utils::idTableName(category));
  m_collectionInfo[category].id = id(0);

  auto collectionName =
      m_metadata_readers[filename]->GetView<std::vector<std::string>>(root_utils::collectionName(category));
  m_collectionInfo[category].name = collectionName(0);

  auto collectionType =
      m_metadata_readers[filename]->GetView<std::vector<std::string>>(root_utils::collInfoName(category));
  m_collectionInfo[category].type = collectionType(0);

  auto subsetCollection =
      m_metadata_readers[filename]->GetView<std::vector<short>>(root_utils::subsetCollection(category));
  m_collectionInfo[category].isSubsetCollection = subsetCollection(0);

  auto schemaVersion = m_metadata_readers[filename]->GetView<std::vector<SchemaVersionT>>("schemaVersion_" + category);
  m_collectionInfo[category].schemaVersion = schemaVersion(0);

  m_idTables[category] =
      std::make_shared<CollectionIDTable>(m_collectionInfo[category].id, m_collectionInfo[category].name);

  return true;
}

void RNTupleReader::openFile(const std::string& filename) {
  openFiles({filename});
}

void RNTupleReader::openFiles(const std::vector<std::string>& filenames) {

  m_filenames.insert(m_filenames.end(), filenames.begin(), filenames.end());
  for (auto& filename : filenames) {
    if (m_metadata_readers.find(filename) == m_metadata_readers.end()) {
      m_metadata_readers[filename] = ROOT::Experimental::RNTupleReader::Open(root_utils::metaTreeName, filename);
    }
  }

  m_metadata = ROOT::Experimental::RNTupleReader::Open(root_utils::metaTreeName, filenames[0]);

  auto versionView = m_metadata->GetView<std::vector<uint16_t>>(root_utils::versionBranchName);
  auto version = versionView(0);

  m_fileVersion = podio::version::Version{version[0], version[1], version[2]};

  auto edmView = m_metadata->GetView<std::vector<std::tuple<std::string, std::string>>>(root_utils::edmDefBranchName);
  auto edm = edmView(0);
  m_datamodelHolder = DatamodelDefinitionHolder(std::move(edm));

  auto availableCategoriesField = m_metadata->GetView<std::vector<std::string>>(root_utils::availableCategories);
  m_availableCategories = availableCategoriesField(0);
}

unsigned RNTupleReader::getEntries(const std::string& name) {
  if (m_readers.find(name) == m_readers.end()) {
    for (auto& filename : m_filenames) {
      try {
        m_readers[name].emplace_back(ROOT::Experimental::RNTupleReader::Open(name, filename));
      } catch (const ROOT::Experimental::RException& e) {
        std::cout << "Category " << name << " not found in file " << filename << std::endl;
      }
    }
    m_totalEntries[name] = std::accumulate(m_readers[name].begin(), m_readers[name].end(), 0,
                                           [](int total, auto& reader) { return total + reader->GetNEntries(); });
  }
  return m_totalEntries[name];
}

std::vector<std::string_view> RNTupleReader::getAvailableCategories() const {
  std::vector<std::string_view> cats;
  cats.reserve(m_availableCategories.size());
  for (const auto& cat : m_availableCategories) {
    cats.emplace_back(cat);
  }
  return cats;
}

std::unique_ptr<ROOTFrameData> RNTupleReader::readNextEntry(const std::string& name) {
  return readEntry(name, m_entries[name]);
}

std::unique_ptr<ROOTFrameData> RNTupleReader::readEntry(const std::string& category, const unsigned entNum) {
  if (m_totalEntries.find(category) == m_totalEntries.end()) {
    getEntries(category);
  }
  if (entNum >= m_totalEntries[category]) {
    return nullptr;
  }

  if (m_collectionInfo.find(category) == m_collectionInfo.end()) {
    if (!initCategory(category)) {
      return nullptr;
    }
  }

  m_entries[category] = entNum + 1;

  ROOTFrameData::BufferMap buffers;
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 31, 0)
  // We need to create a non-bare entry here, because the entries for the
  // parameters are not explicitly (re)set and we need them default initialized.
  // In principle we would only need a bare entry for the collection data, since
  // we set all the fields there in any case.
  auto dentry = m_readers[category][0]->GetModel().CreateEntry();
#else
  auto dentry = m_readers[category][0]->GetModel()->GetDefaultEntry();
#endif

  for (size_t i = 0; i < m_collectionInfo[category].id.size(); ++i) {
    const auto& collType = m_collectionInfo[category].type[i];
    const auto& bufferFactory = podio::CollectionBufferFactory::instance();
    auto maybeBuffers = bufferFactory.createBuffers(collType, m_collectionInfo[category].schemaVersion[i],
                                                    m_collectionInfo[category].isSubsetCollection[i]);
    auto collBuffers = maybeBuffers.value_or(podio::CollectionReadBuffers{});

    if (!maybeBuffers) {
      std::cout << "WARNING: Buffers couldn't be created for collection " << m_collectionInfo[category].name[i]
                << " of type " << m_collectionInfo[category].type[i] << " and schema version "
                << m_collectionInfo[category].schemaVersion[i] << std::endl;
      return nullptr;
    }

    if (m_collectionInfo[category].isSubsetCollection[i]) {
      auto brName = root_utils::subsetBranch(m_collectionInfo[category].name[i]);
      auto vec = new std::vector<podio::ObjectID>;
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 31, 0)
      dentry->BindRawPtr(brName, vec);
#else
      dentry->CaptureValueUnsafe(brName, vec);
#endif
      collBuffers.references->at(0) = std::unique_ptr<std::vector<podio::ObjectID>>(vec);
    } else {
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 31, 0)
      dentry->BindRawPtr(m_collectionInfo[category].name[i], collBuffers.data);
#else
      dentry->CaptureValueUnsafe(m_collectionInfo[category].name[i], collBuffers.data);
#endif

      const auto relVecNames = podio::DatamodelRegistry::instance().getRelationNames(collType);
      for (size_t j = 0; j < relVecNames.relations.size(); ++j) {
        const auto relName = relVecNames.relations[j];
        auto vec = new std::vector<podio::ObjectID>;
        const auto brName = root_utils::refBranch(m_collectionInfo[category].name[i], relName);
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 31, 0)
        dentry->BindRawPtr(brName, vec);
#else
        dentry->CaptureValueUnsafe(brName, vec);
#endif
        collBuffers.references->at(j) = std::unique_ptr<std::vector<podio::ObjectID>>(vec);
      }

      for (size_t j = 0; j < relVecNames.vectorMembers.size(); ++j) {
        const auto vecName = relVecNames.vectorMembers[j];
        const auto brName = root_utils::vecBranch(m_collectionInfo[category].name[i], vecName);
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 31, 0)
        dentry->BindRawPtr(brName, collBuffers.vectorMembers->at(j).second);
#else
        dentry->CaptureValueUnsafe(brName, collBuffers.vectorMembers->at(j).second);
#endif
      }
    }

    buffers.emplace(m_collectionInfo[category].name[i], std::move(collBuffers));
  }

  m_readers[category][0]->LoadEntry(entNum, *dentry);

  auto parameters = readEventMetaData(category, entNum);

  return std::make_unique<ROOTFrameData>(std::move(buffers), m_idTables[category], std::move(parameters));
}

} // namespace podio
