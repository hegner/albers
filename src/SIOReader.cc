#include "podio/SIOReader.h"

#include "sioUtils.h"
#include <sio/api.h>
#include <sio/definitions.h>

#include <algorithm>
#include <utility>

namespace podio {

SIOReader::SIOReader() {
  auto& libLoader [[maybe_unused]] = SIOBlockLibraryLoader::instance();
}

void SIOReader::openFile(const std::string& filename) {
  m_stream.open(filename, std::ios::binary);
  if (!m_stream.is_open()) {
    throw std::runtime_error("File " + filename + " couldn't be opened");
  }

  // NOTE: reading TOC record first because that jumps back to the start of the file!
  readFileTOCRecord();
  readPodioHeader();
  readEDMDefinitions(); // Potentially could do this lazily
}

std::unique_ptr<SIOFrameData> SIOReader::readNextEntry(const std::string& name) {
  // Skip to where the next record of this name starts in the file, based on
  // how many times we have already read this name
  //
  // NOTE: exploiting the fact that the operator[] of a map will create a
  // default initialized entry for us if not present yet
  const auto recordPos = m_tocRecord.getPosition(name, m_nameCtr[name]);
  if (recordPos == 0) {
    return nullptr;
  }
  m_stream.seekg(recordPos);

  auto [tableBuffer, tableInfo] = sio_utils::readRecord(m_stream, false);
  auto [dataBuffer, dataInfo] = sio_utils::readRecord(m_stream, false);

  m_nameCtr[name]++;

  return std::make_unique<SIOFrameData>(std::move(dataBuffer), dataInfo._uncompressed_length, std::move(tableBuffer),
                                        tableInfo._uncompressed_length);
}

std::unique_ptr<SIOFrameData> SIOReader::readEntry(const std::string& name, const unsigned entry) {
  // NOTE: Will create or overwrite the entry counter
  //       All checks are done in the following function
  m_nameCtr[name] = entry;
  return readNextEntry(name);
}

std::vector<std::string_view> SIOReader::getAvailableCategories() const {
  // Filter the available records from the TOC to remove records that are
  // stored, but use reserved record names for podio meta data
  auto recordNames = m_tocRecord.getRecordNames();
  recordNames.erase(std::remove_if(recordNames.begin(), recordNames.end(),
                                   [](const auto& elem) { return elem == sio_helpers::SIOEDMDefinitionName; }),
                    recordNames.end());
  return recordNames;
}

unsigned SIOReader::getEntries(const std::string& name) const {
  return m_tocRecord.getNRecords(name);
}

bool SIOReader::readFileTOCRecord() {
  // Check if there is a dedicated marker at the end of the file that tells us
  // where the TOC actually starts
  m_stream.seekg(-sio_helpers::SIOTocInfoSize, std::ios_base::end);
  uint64_t firstWords{0};
  m_stream.read(reinterpret_cast<char*>(&firstWords), sizeof(firstWords));

  const uint32_t marker = (firstWords >> 32) & 0xffffffff;
  if (marker == sio_helpers::SIOTocMarker) {
    const uint32_t position = firstWords & 0xffffffff;
    m_stream.seekg(position);

    const auto& [uncBuffer, _] = sio_utils::readRecord(m_stream);

    sio::block_list blocks;
    auto tocBlock = std::make_shared<SIOFileTOCRecordBlock>();
    tocBlock->record = &m_tocRecord;
    blocks.push_back(tocBlock);

    sio::api::read_blocks(uncBuffer.span(), blocks);

    m_stream.seekg(0);
    return true;
  }

  m_stream.clear();
  m_stream.seekg(0);
  return false;
}

void SIOReader::readPodioHeader() {
  const auto& [buffer, _] = sio_utils::readRecord(m_stream, false, sizeof(podio::version::Version));

  sio::block_list blocks;
  blocks.emplace_back(std::make_shared<SIOVersionBlock>());
  sio::api::read_blocks(buffer.span(), blocks);

  m_fileVersion = static_cast<SIOVersionBlock*>(blocks[0].get())->version;
}

void SIOReader::readEDMDefinitions() {
  const auto recordPos = m_tocRecord.getPosition(sio_helpers::SIOEDMDefinitionName);
  if (recordPos == 0) {
    // No EDM definitions found
    return;
  }
  m_stream.seekg(recordPos);

  const auto& [buffer, _] = sio_utils::readRecord(m_stream);

  sio::block_list blocks;
  blocks.emplace_back(std::make_shared<podio::SIOMapBlock<std::string, std::string>>());
  sio::api::read_blocks(buffer.span(), blocks);

  auto datamodelDefs = static_cast<SIOMapBlock<std::string, std::string>*>(blocks[0].get());
  m_datamodelHolder = DatamodelDefinitionHolder(std::move(datamodelDefs->mapData));
}

} // namespace podio
