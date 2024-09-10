#include "podio/SIOFrameData.h"

#include "podio/SIOBlock.h"
#include <sio/compression/zlib.h>

#include <algorithm>
#include <iterator>

namespace podio {
std::optional<podio::CollectionReadBuffers> SIOFrameData::getCollectionBuffers(const std::string& name) {
  unpackBuffers();

  if (m_idTable.present(name)) {
    // The collections that we read are not necessarily in the same order as
    // they are in the collection id table. Hence, we cannot simply use the
    // collection ID to index into the blocks
    const auto& names = m_idTable.names();
    const auto nameIt = std::find(std::begin(names), std::end(names), name);
    // collection indices start at 1!
    const auto index = std::distance(std::begin(names), nameIt) + 1;

    // Mark this block as consumed
    m_availableBlocks[index] = 0;
    return {dynamic_cast<podio::SIOBlock*>(m_blocks[index].get())->getBuffers()};
  }

  return std::nullopt;
}

std::unique_ptr<podio::GenericParameters> SIOFrameData::getParameters() {
  unpackBuffers();
  m_availableBlocks[0] = 0;
  return std::make_unique<podio::GenericParameters>(std::move(m_parameters));
}

std::vector<std::string> SIOFrameData::getAvailableCollections() {
  unpackBuffers();
  std::vector<std::string> collections;
  for (size_t i = 1; i < m_blocks.size(); ++i) {
    if (m_availableBlocks[i]) {
      // We have to get the collID of this collection in the idTable as there is
      // no guarantee that it coincides with the index in the blocks.
      // Additionally, collection indices start at 1
      const auto collID = m_idTable.ids()[i - 1];
      collections.push_back(m_idTable.name(collID).value());
    }
  }

  return collections;
}

void SIOFrameData::unpackBuffers() {
  // Only do the unpacking once. Use the block as proxy for deciding whether
  // we have already unpacked things, since that is the main thing we do in
  // here: create blocks and read the data into them
  if (!m_blocks.empty()) {
    return;
  }

  if (m_idTable.empty()) {
    readIdTable();
  }

  createBlocks();

  sio::zlib_compression compressor;
  sio::buffer uncBuffer{m_dataSize};
  compressor.uncompress(m_recBuffer.span(), uncBuffer);
  sio::api::read_blocks(uncBuffer.span(), m_blocks);
}

void SIOFrameData::createBlocks() {
  m_blocks.reserve(m_typeNames.size() + 1);
  // First block during writing is parameters / metadata, then collections
  auto parameters = std::make_shared<podio::SIOEventMetaDataBlock>();
  parameters->metadata = &m_parameters;
  m_blocks.push_back(parameters);

  for (size_t i = 0; i < m_typeNames.size(); ++i) {
    const bool subsetColl = !m_subsetCollectionBits.empty() && m_subsetCollectionBits[i];
    auto blk = podio::SIOBlockFactory::instance().createBlock(m_typeNames[i], m_idTable.names()[i], subsetColl);
    m_blocks.push_back(blk);
  }

  m_availableBlocks.resize(m_blocks.size(), 1);
}

void SIOFrameData::readIdTable() {
  sio::buffer uncBuffer{m_tableSize};
  sio::zlib_compression compressor;
  compressor.uncompress(m_tableBuffer.span(), uncBuffer);

  sio::block_list blocks;
  blocks.emplace_back(std::make_shared<SIOCollectionIDTableBlock>());
  sio::api::read_blocks(uncBuffer.span(), blocks);

  auto* idTableBlock = static_cast<SIOCollectionIDTableBlock*>(blocks[0].get());
  m_idTable = idTableBlock->getTable();
  m_typeNames = idTableBlock->getTypeNames();
  m_subsetCollectionBits = idTableBlock->getSubsetCollectionBits();
}

} // namespace podio
