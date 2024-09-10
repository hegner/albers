#include "podio/DataSource.h"

#include "podio/Reader.h"

// podio
#include <podio/FrameCategories.h>

// ROOT
#include <TFile.h>

// STL
#include <cstddef>
#include <cstdio>
#include <memory>

namespace podio {
DataSource::DataSource(const std::string& filePath, int nEvents) : m_nSlots{1} {
  m_filePathList.emplace_back(filePath);
  SetupInput(nEvents);
}

DataSource::DataSource(const std::vector<std::string>& filePathList, int nEvents) :
    m_nSlots{1}, m_filePathList{filePathList} {
  SetupInput(nEvents);
}

void DataSource::SetupInput(int nEvents) {
  if (m_filePathList.empty()) {
    throw std::runtime_error("podio::DataSource: No input files provided!");
  }

  // Check if the provided file(s) exists and contain required metadata is done
  // by podio::Reader

  // Create probing frame
  podio::Frame frame;
  unsigned int nEventsInFiles = 0;
  auto podioReader = podio::makeReader(m_filePathList);
  nEventsInFiles = podioReader.getEntries(podio::Category::Event);
  frame = podioReader.readFrame(podio::Category::Event, 0);

  // Determine over how many events to run
  if (nEventsInFiles <= 0) {
    throw std::runtime_error("podio::DataSource: No events found!");
  }

  if (nEvents < 0) {
    m_nEvents = nEventsInFiles;
  } else if (nEvents == 0) {
    throw std::runtime_error("podio::DataSource: Requested to run over zero events!");
  } else {
    m_nEvents = nEvents;
  }
  if (nEventsInFiles < m_nEvents) {
    m_nEvents = nEventsInFiles;
  }

  // Get collections stored in the files
  std::vector<std::string> collNames = frame.getAvailableCollections();
  for (auto&& collName : collNames) {
    const podio::CollectionBase* coll = frame.get(collName);
    if (coll->isValid()) {
      m_columnNames.emplace_back(std::move(collName));
      m_columnTypes.emplace_back(coll->getTypeName());
    }
  }
}

void DataSource::SetNSlots(unsigned int nSlots) {
  m_nSlots = nSlots;

  if (m_nSlots > m_nEvents) {
    throw std::runtime_error("podio::DataSource: Number of events too small!");
  }

  int eventsPerSlot = m_nEvents / m_nSlots;
  for (size_t i = 0; i < (m_nSlots - 1); ++i) {
    m_rangesAll.emplace_back(eventsPerSlot * i, eventsPerSlot * (i + 1));
  }
  m_rangesAll.emplace_back(eventsPerSlot * (m_nSlots - 1), m_nEvents);
  m_rangesAvailable = m_rangesAll;

  // Initialize set of addresses needed
  m_Collections.resize(m_columnNames.size(), std::vector<const podio::CollectionBase*>(m_nSlots, nullptr));

  // Initialize podio readers
  for (size_t i = 0; i < m_nSlots; ++i) {
    m_podioReaders.emplace_back(std::make_unique<podio::Reader>(podio::makeReader(m_filePathList)));
  }

  for (size_t i = 0; i < m_nSlots; ++i) {
    m_frames.emplace_back(std::make_unique<podio::Frame>());
  }
}

void DataSource::Initialize() {
}

std::vector<std::pair<ULong64_t, ULong64_t>> DataSource::GetEntryRanges() {
  std::vector<std::pair<ULong64_t, ULong64_t>> rangesToBeProcessed;
  for (auto& range : m_rangesAvailable) {
    rangesToBeProcessed.emplace_back(range.first, range.second);
    if (rangesToBeProcessed.size() >= m_nSlots) {
      break;
    }
  }

  if (m_rangesAvailable.size() > m_nSlots) {
    m_rangesAvailable.erase(m_rangesAvailable.begin(), m_rangesAvailable.begin() + m_nSlots);
  } else {
    m_rangesAvailable.erase(m_rangesAvailable.begin(), m_rangesAvailable.end());
  }

  return rangesToBeProcessed;
}

void DataSource::InitSlot(unsigned int, ULong64_t) {
}

bool DataSource::SetEntry(unsigned int slot, ULong64_t entry) {
  m_frames[slot] = std::make_unique<podio::Frame>(m_podioReaders[slot]->readFrame(podio::Category::Event, entry));

  for (auto& collectionIndex : m_activeCollections) {
    m_Collections[collectionIndex][slot] = m_frames[slot]->get(m_columnNames.at(collectionIndex));
  }

  return true;
}

void DataSource::FinalizeSlot(unsigned int) {
}

void DataSource::Finalize() {
}

std::vector<void*> DataSource::GetColumnReadersImpl(std::string_view columnName, const std::type_info&) {
  auto itr = std::find(m_columnNames.begin(), m_columnNames.end(), columnName);
  if (itr == m_columnNames.end()) {
    std::string errMsg = "podio::DataSource: Can't find requested column \"";
    errMsg += columnName;
    errMsg += "\"!";
    throw std::runtime_error(errMsg);
  }
  auto columnIndex = std::distance(m_columnNames.begin(), itr);
  m_activeCollections.emplace_back(columnIndex);

  std::vector<void*> columnReaders(m_nSlots);
  for (size_t slotIndex = 0; slotIndex < m_nSlots; ++slotIndex) {
    columnReaders[slotIndex] = static_cast<void*>(&m_Collections[columnIndex][slotIndex]);
  }

  return columnReaders;
}

const std::vector<std::string>& DataSource::GetColumnNames() const {
  return m_columnNames;
}

bool DataSource::HasColumn(std::string_view columnName) const {
  return std::find(m_columnNames.begin(), m_columnNames.end(), columnName) != m_columnNames.end();
}

std::string DataSource::GetTypeName(std::string_view columnName) const {
  auto itr = std::find(m_columnNames.begin(), m_columnNames.end(), columnName);
  if (itr == m_columnNames.end()) {
    std::string errMsg = "podio::DataSource: Type name for \"";
    errMsg += columnName;
    errMsg += "\" not found!";
    throw std::runtime_error(errMsg);
  }

  auto typeIndex = std::distance(m_columnNames.begin(), itr);

  return m_columnTypes.at(typeIndex);
}

ROOT::RDataFrame CreateDataFrame(const std::vector<std::string>& filePathList) {
  ROOT::RDataFrame rdf(std::make_unique<DataSource>(filePathList));

  return rdf;
}

ROOT::RDataFrame CreateDataFrame(const std::string& filePath) {
  ROOT::RDataFrame rdf(std::make_unique<DataSource>(filePath));

  return rdf;
}
} // namespace podio
