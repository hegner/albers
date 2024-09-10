#ifndef PODIO_SIO_UTILS_H // NOLINT(llvm-header-guard): internal headers confuse clang-tidy
#define PODIO_SIO_UTILS_H // NOLINT(llvm-header-guard): internal headers confuse clang-tidy

#include "podio/CollectionBase.h"
#include "podio/GenericParameters.h"
#include "podio/SIOBlock.h"
#include <sio/api.h>
#include <sio/compression/zlib.h>
#include <sio/definitions.h>

#include <string_view>
#include <utility>

namespace podio {
namespace sio_utils {
  /// Read the record into a buffer and potentially uncompress it
  inline std::pair<sio::buffer, sio::record_info> readRecord(sio::ifstream& stream, bool decompress = true,
                                                             std::size_t initBufferSize = sio::mbyte) {
    sio::record_info recInfo;
    sio::buffer infoBuffer{sio::max_record_info_len};
    sio::buffer recBuffer{initBufferSize};
    sio::api::read_record_info(stream, recInfo, infoBuffer);
    sio::api::read_record_data(stream, recInfo, recBuffer);

    if (decompress) {
      sio::buffer uncBuffer{recInfo._uncompressed_length};
      sio::zlib_compression compressor;
      compressor.uncompress(recBuffer.span(), uncBuffer);
      return std::make_pair(std::move(uncBuffer), recInfo);
    }

    return std::make_pair(std::move(recBuffer), recInfo);
  }

  using StoreCollection = std::pair<const std::string&, const podio::CollectionBase*>;

  /// Create the collection ID block from the passed collections
  inline std::shared_ptr<SIOCollectionIDTableBlock> createCollIDBlock(const std::vector<StoreCollection>& collections,
                                                                      const podio::CollectionIDTable& collIdTable) {
    // Need to make sure that the type names and subset collection bits are in
    // the same order here!
    std::vector<std::string> types;
    types.reserve(collections.size());
    std::vector<short> subsetColl;
    subsetColl.reserve(collections.size());
    std::vector<std::string> names;
    names.reserve(collections.size());
    std::vector<uint32_t> ids;
    ids.reserve(collections.size());

    for (const auto& [name, coll] : collections) {
      names.emplace_back(name);
      ids.emplace_back(collIdTable.collectionID(name).value());
      types.emplace_back(coll->getValueTypeName());
      subsetColl.emplace_back(coll->isSubsetCollection());
    }

    return std::make_shared<SIOCollectionIDTableBlock>(std::move(names), std::move(ids), std::move(types),
                                                       std::move(subsetColl));
  }

  /// Create all blocks to store the passed collections and parameters into a record
  inline sio::block_list createBlocks(const std::vector<StoreCollection>& collections,
                                      const podio::GenericParameters& parameters) {
    sio::block_list blocks;
    blocks.reserve(collections.size() + 1); // parameters + collections

    auto paramBlock = std::make_shared<SIOEventMetaDataBlock>();
    // TODO: get rid of const_cast
    paramBlock->metadata = const_cast<podio::GenericParameters*>(&parameters);
    blocks.emplace_back(std::move(paramBlock));

    for (const auto& [name, col] : collections) {
      blocks.emplace_back(podio::SIOBlockFactory::instance().createBlock(col, name));
    }

    return blocks;
  }

  /// Write the passed record and return where it starts in the file
  inline sio::ifstream::pos_type writeRecord(const sio::block_list& blocks, const std::string& recordName,
                                             sio::ofstream& stream, std::size_t initBufferSize = sio::mbyte,
                                             bool compress = true) {
    auto buffer = sio::buffer{initBufferSize};
    auto recInfo = sio::api::write_record(recordName, buffer, blocks, 0);

    if (compress) {
      // use zlib to compress the record into another buffer
      sio::zlib_compression compressor;
      compressor.set_level(6); // Z_DEFAULT_COMPRESSION==6
      auto comBuffer = sio::buffer{initBufferSize};
      sio::api::compress_record(recInfo, buffer, comBuffer, compressor);

      sio::api::write_record(stream, buffer.span(0, recInfo._header_length), comBuffer.span(), recInfo);
    } else {
      sio::api::write_record(stream, buffer.span(), recInfo);
    }

    return recInfo._file_start;
  }

} // namespace sio_utils
} // namespace podio

#endif
