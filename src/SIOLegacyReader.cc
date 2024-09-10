#include "podio/SIOLegacyReader.h"

#include "sioUtils.h"
#include <sio/api.h>
#include <sio/compression/zlib.h>
#include <sio/definitions.h>

namespace podio {

SIOLegacyReader::SIOLegacyReader() {
  auto& libLoader [[maybe_unused]] = SIOBlockLibraryLoader::instance();
}

void SIOLegacyReader::openFile(const std::string& filename) {
  m_stream.open(filename, std::ios::binary);
  if (!m_stream.is_open()) {
    throw std::runtime_error("File " + filename + " couldn't be opened");
  }

  // NOTE: reading TOC record first because that jumps back to the start of the file!
  readFileTOCRecord();
  readCollectionIDTable();
}

std::unique_ptr<SIOFrameData> SIOLegacyReader::readNextEntry(const std::string& name) {
  if (name != m_categoryName) {
    return nullptr;
  }
  // skip possible intermediate records that are not event data
  try {
    sio::api::go_to_record(m_stream, "event_record");
  } catch (sio::exception&) {
    // If anything goes wrong, return a nullptr
    return nullptr;
  }

  auto [dataBuffer, dataInfo] = sio_utils::readRecord(m_stream, false);
  // Need to work around the fact that sio::buffers are not copyable by copying
  // the underlying buffer (vector<byte>) and then using that to move construct
  // a new buffer
  sio::buffer::container bufferBytes{m_tableBuffer.data(), m_tableBuffer.data() + m_tableBuffer.size()};
  auto tableBuffer = sio::buffer(std::move(bufferBytes));

  m_eventNumber++;
  return std::make_unique<SIOFrameData>(std::move(dataBuffer), dataInfo._uncompressed_length, std::move(tableBuffer),
                                        m_tableUncLength);
}

std::unique_ptr<podio::SIOFrameData> SIOLegacyReader::readEntry(const std::string& name, const unsigned entry) {
  if (name != m_categoryName) {
    return nullptr;
  }

  // Setting the event number to the desired one here and putting the stream to
  // the right position is the necessary setup before simply handing off to readNextEntry
  m_eventNumber = entry;
  // NOTE: In legacy files the "events" are stored in "event_record" records
  const auto recordPos = m_tocRecord.getPosition("event_record", entry);
  if (recordPos == 0) {
    return nullptr;
  }
  m_stream.seekg(recordPos);

  return readNextEntry(name);
}

unsigned SIOLegacyReader::getEntries(const std::string& name) const {
  if (name != "events") {
    return 0;
  }
  return m_tocRecord.getNRecords("event_record");
}

void SIOLegacyReader::readCollectionIDTable() {
  // Need to decompress the buffers here, because in this record not only the
  // collectionID table is stored, but also the version information...
  auto [infoBuffer, _] = sio_utils::readRecord(m_stream, true);

  sio::block_list blocks;
  blocks.emplace_back(std::make_shared<SIOCollectionIDTableBlock>());
  blocks.emplace_back(std::make_shared<SIOVersionBlock>());
  sio::api::read_blocks(infoBuffer.span(), blocks);

  m_fileVersion = static_cast<SIOVersionBlock*>(blocks[1].get())->version;

  // recompress the collection ID table block...
  blocks.resize(1); // remove the SIOVersionBlock
  auto tmpUncBuffer = sio::buffer{sio::mbyte};
  auto tmpRecInfo = sio::api::write_record("dummy", tmpUncBuffer, blocks, 0);
  sio::zlib_compression compressor;
  compressor.set_level(6);
  sio::api::compress_record(tmpRecInfo, tmpUncBuffer, m_tableBuffer, compressor);
  m_tableUncLength = tmpRecInfo._uncompressed_length;
}

bool SIOLegacyReader::readFileTOCRecord() {
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

std::vector<std::string_view> SIOLegacyReader::getAvailableCategories() const {
  return {m_categoryName};
}

} // namespace podio
