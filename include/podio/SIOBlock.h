#ifndef PODIO_SIOBLOCK_H
#define PODIO_SIOBLOCK_H

#include <podio/CollectionBase.h>
#include <podio/CollectionIDTable.h>
#include <podio/GenericParameters.h>
#include <podio/podioVersion.h>
#include <podio/utilities/TypeHelpers.h>
#include <sio/block.h>
#include <sio/io_device.h>
#include <sio/version.h>

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace podio {

template <typename devT, typename PODData>
void handlePODDataSIO(devT& device, PODData* data, size_t size) {
  unsigned count = size * sizeof(PODData);
  char* dataPtr = reinterpret_cast<char*>(data);
  device.data(dataPtr, count);
}

/// Write anything that iterates like an std::map
template <typename MapLikeT>
void writeMapLike(sio::write_device& device, const MapLikeT& map) {
  device.data((int)map.size());
  for (const auto& [key, value] : map) {
    device.data(key);
    device.data(value);
  }
}

/// Read anything that iterates like an std::map
template <typename MapLikeT>
void readMapLike(sio::read_device& device, MapLikeT& map) {
  int size;
  device.data(size);
  while (size--) {
    detail::GetKeyType<MapLikeT> key;
    device.data(key);
    detail::GetMappedType<MapLikeT> value;
    device.data(value);
    if constexpr (podio::detail::isVector<MapLikeT>) {
      map.emplace_back(std::move(key), std::move(value));
    } else {
      map.emplace(std::move(key), std::move(value));
    }
  }
}

/// Base class for sio::block handlers used with PODIO
class SIOBlock : public sio::block {

public:
  SIOBlock(const std::string& nam, sio::version_type vers) : sio::block(nam, vers) {
  }
  SIOBlock() = delete;
  SIOBlock(const SIOBlock&) = delete;
  SIOBlock& operator=(const SIOBlock&) = delete;

  podio::CollectionBase* getCollection() {
    return m_buffers.createCollection(m_buffers, m_subsetColl).release();
  }

  podio::CollectionReadBuffers getBuffers() const {
    return m_buffers;
  }

  std::string name() {
    return sio::block::name();
  }

  void setSubsetCollection(bool subsetColl) {
    m_subsetColl = subsetColl;
  }

  void setCollection(podio::CollectionBase* col) {
    m_subsetColl = col->isSubsetCollection();
    m_buffers = col->getBuffers();
  }

  virtual SIOBlock* create(const std::string& name) const = 0;

protected:
  bool m_subsetColl{false};
  podio::CollectionReadBuffers m_buffers{};
};

/// A dedicated block for handling the I/O of the CollectionIDTable
class SIOCollectionIDTableBlock : public sio::block {
public:
  SIOCollectionIDTableBlock() : sio::block("CollectionIDs", sio::version::encode_version(0, 4)) {
  }

  SIOCollectionIDTableBlock(std::vector<std::string>&& names, std::vector<uint32_t>&& ids,
                            std::vector<std::string>&& types, std::vector<short>&& isSubsetColl) :
      sio::block("CollectionIDs", sio::version::encode_version(0, 3)),
      _names(std::move(names)),
      _ids(std::move(ids)),
      _types(std::move(types)),
      _isSubsetColl(std::move(isSubsetColl)) {
  }

  SIOCollectionIDTableBlock(const SIOCollectionIDTableBlock&) = delete;
  SIOCollectionIDTableBlock& operator=(const SIOCollectionIDTableBlock&) = delete;

  void read(sio::read_device& device, sio::version_type version) override;
  void write(sio::write_device& device) override;

  podio::CollectionIDTable getTable() {
    return podio::CollectionIDTable(std::move(_ids), std::move(_names));
  }
  const std::vector<std::string>& getTypeNames() const {
    return _types;
  }
  const std::vector<short>& getSubsetCollectionBits() const {
    return _isSubsetColl;
  }

private:
  std::vector<std::string> _names{};
  std::vector<uint32_t> _ids{};
  std::vector<std::string> _types{};
  std::vector<short> _isSubsetColl{};
};

struct SIOVersionBlock : public sio::block {
  SIOVersionBlock() : sio::block("podio_version", sio::version::encode_version(1, 0)) {
  }

  SIOVersionBlock(podio::version::Version v) :
      sio::block("podio_version", sio::version::encode_version(1, 0)), version(v) {
  }

  void write(sio::write_device& device) override {
    device.data(version);
  }

  void read(sio::read_device& device, sio::version_type) override {
    device.data(version);
  }

  podio::version::Version version{};
};

/// A block for handling the EventMeta data
class SIOEventMetaDataBlock : public sio::block {
public:
  SIOEventMetaDataBlock() : sio::block("EventMetaData", sio::version::encode_version(0, 2)) {
  }

  SIOEventMetaDataBlock(const SIOEventMetaDataBlock&) = delete;
  SIOEventMetaDataBlock& operator=(const SIOEventMetaDataBlock&) = delete;

  void read(sio::read_device& device, sio::version_type version) override;
  void write(sio::write_device& device) override;

  podio::GenericParameters* metadata{nullptr};
};

/// A block to serialize anything that behaves similar in iterating as a
/// map<KeyT, ValueT>, e.g. vector<tuple<KeyT, ValueT>>, which is what is used
/// internally to represent the data to be written.
template <typename KeyT, typename ValueT>
struct SIOMapBlock : public sio::block {
  SIOMapBlock() : sio::block("SIOMapBlock", sio::version::encode_version(0, 1)) {
  }
  SIOMapBlock(std::vector<std::tuple<KeyT, ValueT>>&& data) :
      sio::block("SIOMapBlock", sio::version::encode_version(0, 1)), mapData(std::move(data)) {
  }

  SIOMapBlock(const SIOMapBlock&) = delete;
  SIOMapBlock& operator=(const SIOMapBlock&) = delete;

  void read(sio::read_device& device, sio::version_type) override {
    readMapLike(device, mapData);
  }
  void write(sio::write_device& device) override {
    writeMapLike(device, mapData);
  }

  std::vector<std::tuple<KeyT, ValueT>> mapData{};
};

/// A block for handling the run and collection meta data
class SIONumberedMetaDataBlock : public sio::block {
public:
  SIONumberedMetaDataBlock(const std::string& name) : sio::block(name, sio::version::encode_version(0, 2)) {
  }

  SIONumberedMetaDataBlock(const SIONumberedMetaDataBlock&) = delete;
  SIONumberedMetaDataBlock& operator=(const SIONumberedMetaDataBlock&) = delete;

  void read(sio::read_device& device, sio::version_type version) override;
  void write(sio::write_device& device) override;

  std::map<int, GenericParameters>* data{nullptr};
};

/// factory for creating sio::blocks for a given type of EDM-collection
class SIOBlockFactory {
private:
  SIOBlockFactory() = default;

  typedef std::unordered_map<std::string, SIOBlock*> BlockMap;
  BlockMap _map{};

public:
  void registerBlockForCollection(const std::string& type, SIOBlock* b) {
    _map[type] = b;
  }

  std::shared_ptr<SIOBlock> createBlock(const podio::CollectionBase* col, const std::string& name) const;

  // return a block with a new collection (used for reading )
  std::shared_ptr<SIOBlock> createBlock(const std::string& typeStr, const std::string& name,
                                        const bool isRefColl = false) const;

  static SIOBlockFactory& instance() {
    static SIOBlockFactory me;
    return me;
  }
};

class SIOBlockLibraryLoader {
private:
  SIOBlockLibraryLoader();

  /// Status code for loading shared SIOBlocks libraries
  enum class LoadStatus : short { Success = 0, AlreadyLoaded = 1, Error = 2 };

  /// Load a library with the given name via dlopen
  LoadStatus loadLib(const std::string& libname);

  /// Get all files that are found on LD_LIBRARY_PATH and that have "SioBlocks"
  /// in their name together with the directory they are in
  static std::vector<std::tuple<std::string, std::string>> getLibNames();

  std::map<std::string, void*> _loadedLibs{};

public:
  static SIOBlockLibraryLoader& instance() {
    static SIOBlockLibraryLoader instance;
    return instance;
  }
};

namespace sio_helpers {
  /// marker for showing that a TOC has been stored in the file
  static constexpr uint32_t SIOTocMarker = 0xc001fea7;
  /// the number of bits necessary to store the SIOTocMarker and the actual
  /// position of the start of the SIOFileTOCRecord
  static constexpr int SIOTocInfoSize = sizeof(uint64_t); // i.e. usually 8
  /// The name of the TOCRecord
  static constexpr const char* SIOTocRecordName = "podio_SIO_TOC_Record";

  /// The name of the record containing the EDM definitions in json format
  static constexpr const char* SIOEDMDefinitionName = "podio_SIO_EDMDefinitions";

  // should hopefully be enough for all practical purposes
  using position_type = uint32_t;
} // namespace sio_helpers

class SIOFileTOCRecord {
public:
  using PositionType = sio_helpers::position_type;
  void addRecord(const std::string& name, PositionType startPos);

  size_t getNRecords(const std::string& name) const;

  /// Get the position of the iEntry-th record with the given name. If no entry
  /// with the given name is recorded, return 0. Note there is no internal check
  /// on whether the given name actually has iEntry records. Use getNRecords to
  /// check for that if necessary.
  PositionType getPosition(const std::string& name, unsigned iEntry = 0) const;

  /// Get all the record names that are stored in this TOC record
  std::vector<std::string_view> getRecordNames() const;

private:
  friend struct SIOFileTOCRecordBlock;

  using RecordListType = std::pair<std::string, std::vector<PositionType>>;
  using MapType = std::vector<RecordListType>;

  MapType m_recordMap{};
};

struct SIOFileTOCRecordBlock : public sio::block {
  SIOFileTOCRecordBlock() : sio::block(sio_helpers::SIOTocRecordName, sio::version::encode_version(0, 1)) {
  }

  SIOFileTOCRecordBlock(SIOFileTOCRecord* r) :
      sio::block(sio_helpers::SIOTocRecordName, sio::version::encode_version(0, 1)), record(r) {
  }

  SIOFileTOCRecordBlock(const SIOFileTOCRecordBlock&) = delete;
  SIOFileTOCRecordBlock& operator=(const SIOFileTOCRecordBlock&) = delete;

  void read(sio::read_device& device, sio::version_type version) override;
  void write(sio::write_device& device) override;

  SIOFileTOCRecord* record{nullptr};
};

} // namespace podio
#endif
