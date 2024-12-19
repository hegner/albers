#ifndef PODIO_COLLECTIONIDTABLE_H
#define PODIO_COLLECTIONIDTABLE_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace podio {

class CollectionIDTable {

public:
  CollectionIDTable();

  CollectionIDTable(const CollectionIDTable&) = delete;
  CollectionIDTable& operator=(const CollectionIDTable&) = delete;
  CollectionIDTable(CollectionIDTable&&) = default;
  CollectionIDTable& operator=(CollectionIDTable&&) = default;

  /// constructor from existing ID:name mapping
  CollectionIDTable(std::vector<uint32_t>&& ids, std::vector<std::string>&& names);

  CollectionIDTable(const std::vector<uint32_t>& ids, const std::vector<std::string>& names);

  /// return collection ID for given name
  std::optional<uint32_t> collectionID(const std::string& name) const;

  /// return name for given collection ID
  std::optional<const std::string> name(uint32_t collectionID) const;

  /// Check if collection name is known
  bool present(const std::string& name) const;

  /// Check if collection ID is known
  bool present(uint32_t collectionID) const;

  /// return registered names
  const std::vector<std::string>& names() const {
    return m_names;
  }

  /// return the ids
  const std::vector<uint32_t>& ids() const {
    return m_collectionIDs;
  }

  /// register new name to the table
  /// returns assigned collection ID
  uint32_t add(const std::string& name);

  /// Prints collection information
  void print() const;

  /// Does this table hold any information?
  bool empty() const {
    return m_names.empty();
  }

private:
  std::vector<uint32_t> m_collectionIDs{};
  std::vector<std::string> m_names{};
  mutable std::unique_ptr<std::mutex> m_mutex{nullptr};
};

} // namespace podio
#endif
