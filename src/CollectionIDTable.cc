// podio specific includes
#include "podio/CollectionIDTable.h"

#include "MurmurHash3.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <utility>

namespace podio {

CollectionIDTable::CollectionIDTable() : m_mutex(std::make_unique<std::mutex>()) {
}

CollectionIDTable::CollectionIDTable(std::vector<uint32_t>&& ids, std::vector<std::string>&& names) :
    m_collectionIDs(std::move(ids)), m_names(std::move(names)), m_mutex(std::make_unique<std::mutex>()) {
}

CollectionIDTable::CollectionIDTable(const std::vector<uint32_t>& ids, const std::vector<std::string>& names) :
    m_collectionIDs(ids), m_names(names), m_mutex(std::make_unique<std::mutex>()) {
}

std::optional<const std::string> CollectionIDTable::name(uint32_t ID) const {
  std::lock_guard<std::mutex> lock{*m_mutex};
  const auto result = std::find(begin(m_collectionIDs), end(m_collectionIDs), ID);
  const auto index = std::distance(m_collectionIDs.begin(), result);
  if (index >= static_cast<ptrdiff_t>(m_names.size())) {
    return std::nullopt;
  }
  return m_names[index];
}

std::optional<uint32_t> CollectionIDTable::collectionID(const std::string& name) const {
  std::lock_guard<std::mutex> lock{*m_mutex};
  const auto result = std::find(begin(m_names), end(m_names), name);
  const auto index = std::distance(m_names.begin(), result);
  if (index >= static_cast<ptrdiff_t>(m_collectionIDs.size())) {
    return std::nullopt;
  }
  return m_collectionIDs[index];
}

void CollectionIDTable::print() const {
  std::lock_guard<std::mutex> lock{*m_mutex};
  std::cout << "CollectionIDTable" << std::endl;
  for (unsigned i = 0; i < m_names.size(); ++i) {
    std::cout << "\t" << m_names[i] << " : " << m_collectionIDs[i] << std::endl;
  }
}

bool CollectionIDTable::present(const std::string& name) const {
  return collectionID(name).has_value();
}

bool CollectionIDTable::present(uint32_t collectionID) const {
  return name(collectionID).has_value();
}

uint32_t CollectionIDTable::add(const std::string& name) {
  std::lock_guard<std::mutex> lock{*m_mutex};
  const auto result = std::find(begin(m_names), end(m_names), name);
  uint32_t ID = 0;
  if (result == m_names.end()) {
    m_names.emplace_back(name);
    MurmurHash3_x86_32(name.c_str(), name.size(), 0, &ID);
    m_collectionIDs.emplace_back(ID);
  } else {
    const auto index = std::distance(m_names.begin(), result);
    ID = m_collectionIDs[index];
  }
  return ID;
}

} // namespace podio
