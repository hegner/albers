#include "podio/ROOTNTupleReader.h"
#include "podio/CollectionBase.h"
#include "podio/CollectionBuffers.h"
#include "podio/CollectionIDTable.h"
#include "podio/GenericParameters.h"
#include "rootUtils.h"

// ROOT specific includes
#include "TClass.h"
#include "TFile.h"
#include <memory>

#include "datamodel/ExampleMCData.h"

namespace podio {

GenericParameters ROOTNTupleReader::readEventMetaData(const std::string& name) {
  // Parameter branch is always the last one
  // auto& paramBranches = catInfo.branches.back();
  // auto* branch = paramBranches.data;

  GenericParameters params;
  // auto* emd = &params;
  // branch->SetAddress(&emd);
  // branch->GetEntry(catInfo.entry);
  return params;
}

void ROOTNTupleReader::initCategory(const std::string& category) {
  std::cout << "initCategory(" << category << ")" << std::endl;
  std::cout << "Getting id" << std::endl;
  // Assume that the metadata is the same in all files
  auto filename = m_filenames[0];
  auto id = m_metadata_readers[filename]->GetView<std::vector<int>>(root_utils::idTableName(category));
  m_collectionId[category] = id(0);

  std::cout << "Getting collectionName" << std::endl;
  auto collectionName = m_metadata_readers[filename]->GetView<std::vector<std::string>>(category + "_name");
  m_collectionName[category] = collectionName(0);

  std::cout << "Getting collectionType" << std::endl;
  auto collectionType = m_metadata_readers[filename]->GetView<std::vector<std::string>>(root_utils::collInfoName(category));
  m_collectionType[category] = collectionType(0);
   
  std::cout << "Getting subsetCollection" << std::endl;
  auto subsetCollection = m_metadata_readers[filename]->GetView<std::vector<bool>>(category + "_test");
  m_isSubsetCollection[category] = subsetCollection(0);
}

void ROOTNTupleReader::openFile(const std::string& filename) {
  openFiles({filename});
}

void ROOTNTupleReader::openFiles(const std::vector<std::string>& filenames) {

  m_filenames.insert(m_filenames.end(), filenames.begin(), filenames.end());
  for (auto& filename : filenames) {
    if (m_metadata_readers.find(filename) == m_metadata_readers.end()) {
      m_metadata_readers[filename] = ROOT::Experimental::RNTupleReader::Open(root_utils::metaTreeName, filename);
    }
  }

  m_metadata = ROOT::Experimental::RNTupleReader::Open(root_utils::metaTreeName, "example_rntuple.root");

  auto version_view = m_metadata->GetView<std::vector<int>>(root_utils::versionBranchName);
  auto version = version_view(0);

  m_fileVersion = podio::version::Version{version[0], version[1], version[2]};
  std::cout << "Version is " << m_fileVersion.major << " " << m_fileVersion.minor << " " << m_fileVersion.patch << std::endl;

  auto edm_view = m_metadata->GetView<std::vector<std::tuple<std::string, std::string>>>(root_utils::edmDefBranchName);
  auto edm = edm_view(0);

  // m_datamodelHolder = DatamodelDefinitionHolder(std::move(*datamodelDefs));

  // Do some work up front for setting up categories and setup all the chains
  // and record the available categories. The rest of the setup follows on
  // demand when the category is first read
  // m_availCategories = ::podio::getAvailableCategories2(m_metaChain.get());
  // for (const auto& cat : m_availCategories) {
  //   auto [it, _] = m_categories.try_emplace(cat, std::make_unique<TChain>(cat.c_str()));
  //   for (const auto& fn : filenames) {
  //     it->second.chain->Add(fn.c_str());
  //   }
  // }
}

unsigned ROOTNTupleReader::getEntries(const std::string& name) {
  if (m_readers.find(name) == m_readers.end()) {
    for (auto& filename : m_filenames) {
      m_readers[name].emplace_back(ROOT::Experimental::RNTupleReader::Open(name, filename));
    }
  }
  return std::accumulate(m_readers[name].begin(), m_readers[name].end(), 0, [](int total, auto& reader) {return total + reader->GetNEntries();});
}

std::unique_ptr<ROOTFrameData> ROOTNTupleReader::readNextEntry(const std::string& name) {
  // auto& catInfo = getCategoryInfo(name);
  int current_entry = m_entries[name];

  return readEntry(name, current_entry);
}

std::unique_ptr<ROOTFrameData> ROOTNTupleReader::readEntry(const std::string& category, const unsigned entNum) {
  std::cout << "Calling readEntry " << std::endl;

  if (m_collectionId.find(category) == m_collectionId.end()) {
    initCategory(category);
  }

  ROOTFrameData::BufferMap buffers;
  auto dentry = m_readers[category][0]->GetModel()->GetDefaultEntry();

  for (int i = 0; i < m_collectionId[category].size(); ++i) {
    std::cout << "i = " << i << " " << m_collectionId[category][i] << " " << m_collectionType[category][i] << " " << m_collectionName[category][i] << std::endl;

    const auto collectionClass = TClass::GetClass(m_collectionType[category][i].c_str());

    auto collection =
        std::unique_ptr<podio::CollectionBase>(static_cast<podio::CollectionBase*>(collectionClass->New()));

    const std::string bufferClassName = "std::vector<" + collection->getDataTypeName() + ">";
    const auto bufferClass = m_isSubsetCollection[category][i] ? nullptr : TClass::GetClass(bufferClassName.c_str());

    auto collBuffers = podio::CollectionReadBuffers();
    // const bool isSubsetColl = bufferClass == nullptr;
    const bool isSubsetColl = bufferClass == nullptr;
    if (!isSubsetColl) {
      collBuffers.data = bufferClass->New();
    }
    collection->setSubsetCollection(isSubsetColl);

    auto tmpBuffers = collection->createBuffers();
    collBuffers.createCollection = std::move(tmpBuffers.createCollection);
    collBuffers.recast = std::move(tmpBuffers.recast);

    if (auto* refs = tmpBuffers.references) {
      collBuffers.references = new podio::CollRefCollection(refs->size());
    }
    if (auto* vminfo = tmpBuffers.vectorMembers) {
      collBuffers.vectorMembers = new podio::VectorMembersInfo();
      collBuffers.vectorMembers->reserve(vminfo->size());

      for (const auto& [type, _] : (*vminfo)) {
        const auto* vecClass = TClass::GetClass(("vector<" + type + ">").c_str());
        collBuffers.vectorMembers->emplace_back(type, vecClass->New());
      }
    }

    if (!isSubsetColl) {
      dentry->CaptureValueUnsafe(m_collectionName[category][i], collBuffers.data);
    }
    if (auto* refCollections = collBuffers.references) {
      std::cout << "The number of references is " << refCollections->size() << std::endl;
      for (size_t j = 0; j < refCollections->size(); ++j) {
        // The unique_ptrs are nullptrs at the beginning, we first initialize
        // them and then fill the values with the read data since
        refCollections->at(j) = std::make_unique<std::vector<podio::ObjectID>>();
        const auto brName = root_utils::refBranch(m_collectionName[category][i], j);
        std::cout << "brName = " << brName << " " << (refCollections->at(j) == nullptr) << std::endl;
        dentry->CaptureValueUnsafe(brName, (*refCollections)[j].get());
      }
    }
    std::cout << "CaptureValueUnsafe done" << std::endl;
    buffers.emplace(m_collectionName[category][i], std::move(collBuffers));
  }
  m_readers[category][0]->LoadEntry(entNum);

  auto buf = buffers["mcparticles"];
  auto ptr = (std::vector<ExampleMCData>*)(buf.data);
  std::cout << "Size of MCData is " << ptr->size();


  auto parameters = readEventMetaData(category);
  auto table = std::make_shared<CollectionIDTable>();

  auto names = m_collectionName[category];
  auto ids = m_collectionId[category];

  std::vector<std::pair<int, std::string>> v;
  for (int i = 0; i < names.size(); ++i) {
    v.emplace_back(std::make_pair<int, std::string>(int(ids[i]),std::string(names[i])));
  }
  std::sort(v.begin(), v.end());

  for (auto& [name, id] : v) {
    table->add(std::to_string(name));
  }

  return std::make_unique<ROOTFrameData>(std::move(buffers), table, std::move(parameters));
}

} // namespace podio
