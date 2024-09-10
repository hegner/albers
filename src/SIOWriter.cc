#include "podio/SIOWriter.h"

#include "podio/Frame.h"
#include "podio/SIOBlock.h"
#include "sioUtils.h"

#include <memory>
#include <string>

namespace podio {

SIOWriter::SIOWriter(const std::string& filename) {
  m_stream.open(filename, std::ios::binary);
  if (!m_stream.is_open()) {
    SIO_THROW(sio::error_code::not_open, "Couldn't open output stream '" + filename + "'");
  }

  auto& libLoader [[maybe_unused]] = SIOBlockLibraryLoader::instance();

  sio::block_list blocks;
  blocks.emplace_back(std::make_shared<SIOVersionBlock>(podio::version::build_version));
  // write the version uncompressed
  sio_utils::writeRecord(blocks, "podio_header_info", m_stream, sizeof(podio::version::Version), false);
}

SIOWriter::~SIOWriter() {
  if (!m_finished) {
    finish();
  }
}

void SIOWriter::writeFrame(const podio::Frame& frame, const std::string& category) {
  writeFrame(frame, category, frame.getAvailableCollections());
}

void SIOWriter::writeFrame(const podio::Frame& frame, const std::string& category,
                           const std::vector<std::string>& collsToWrite) {
  std::vector<sio_utils::StoreCollection> collections;
  collections.reserve(collsToWrite.size());
  for (const auto& name : collsToWrite) {
    collections.emplace_back(name, frame.getCollectionForWrite(name));
    m_datamodelCollector.registerDatamodelDefinition(collections.back().second, name);
  }

  // Write necessary metadata and the actual data into two different records.
  // Otherwise we cannot easily unpack the data record, because necessary
  // information is contained within the record.
  sio::block_list tableBlocks;
  tableBlocks.emplace_back(sio_utils::createCollIDBlock(collections, frame.getCollectionIDTableForWrite()));
  m_tocRecord.addRecord(category, sio_utils::writeRecord(tableBlocks, category + "_HEADER", m_stream));

  const auto blocks = sio_utils::createBlocks(collections, frame.getParameters());
  sio_utils::writeRecord(blocks, category, m_stream);
}

void SIOWriter::finish() {
  auto edmDefMap = std::make_shared<podio::SIOMapBlock<std::string, std::string>>(
      m_datamodelCollector.getDatamodelDefinitionsToWrite());

  sio::block_list blocks;
  blocks.push_back(edmDefMap);
  m_tocRecord.addRecord(sio_helpers::SIOEDMDefinitionName, sio_utils::writeRecord(blocks, "EDMDefinitions", m_stream));

  blocks.clear();
  blocks.emplace_back(std::make_shared<SIOFileTOCRecordBlock>(&m_tocRecord));

  auto tocStartPos = sio_utils::writeRecord(blocks, sio_helpers::SIOTocRecordName, m_stream);

  // Now that we know the position of the TOC Record, put this information
  // into a final marker that can be identified and interpreted when reading
  // again
  uint64_t finalWords = (((uint64_t)sio_helpers::SIOTocMarker) << 32) | ((uint64_t)tocStartPos & 0xffffffff);
  m_stream.write(reinterpret_cast<char*>(&finalWords), sizeof(finalWords));

  m_stream.close();

  m_finished = true;
}

} // namespace podio
