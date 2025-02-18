#ifndef PODIO_OBJECTID_H
#define PODIO_OBJECTID_H

#include <cstdint>
#include <functional>
#include <iomanip>
#include <ostream>

#if defined(PODIO_JSON_OUTPUT) && !defined(__CLING__)
  #include "nlohmann/json.hpp"
#endif

namespace podio {

class ObjectID {

public:
  /// not part of a collection
  static const int untracked = -1;
  /// invalid or non-available object
  static const int invalid = -2;

  /// index of object in collection
  int index{untracked};
  /// ID of the collection
  uint32_t collectionID{static_cast<uint32_t>(untracked)};

  /// index and collectionID uniquely defines the object.
  /// this operator is necessary for meaningful comparisons in python
  bool operator==(const ObjectID& other) const {
    return index == other.index && collectionID == other.collectionID;
  }
  bool operator!=(const ObjectID& other) const {
    return !(*this == other);
  }
};

inline std::ostream& operator<<(std::ostream& os, const podio::ObjectID& id) {
  const auto oldFlags = os.flags();
  os << std::hex << std::setw(8) << id.collectionID;
  os.flags(oldFlags);
  return os << "|" << id.index;
}

#if defined(PODIO_JSON_OUTPUT) && !defined(__CLING__)
inline void to_json(nlohmann::json& j, const podio::ObjectID& id) {
  j = nlohmann::json{{"collectionID", id.collectionID}, {"index", id.index}};
}
#endif

} // namespace podio

template<>
struct std::hash<podio::ObjectID>
{
    std::size_t operator()(const podio::ObjectID& id) const noexcept
    {
        auto hash_collectionID = std::hash<uint32_t>{}(id.collectionID);
        auto hash_index = std::hash<int>{}(id.index);

        return hash_collectionID ^ hash_index;
    }
};

#endif
