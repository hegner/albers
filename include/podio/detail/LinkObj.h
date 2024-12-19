#ifndef PODIO_DETAIL_LINKOBJ_H
#define PODIO_DETAIL_LINKOBJ_H

#include "podio/detail/LinkFwd.h"

#include "podio/ObjectID.h"

#include <memory>

namespace podio {

template <typename FromT, typename ToT>
class LinkObj {

  friend Link<FromT, ToT>;
  friend MutableLink<FromT, ToT>;

public:
  /// Constructor
  LinkObj() : id(), data(LinkData{1.0f}), m_from(nullptr), m_to(nullptr) {
  }

  /// Constructor from ObjectID and data (does not initialize relations yet!)
  LinkObj(const podio::ObjectID id_, LinkData data_) : id(id_), data(data_) {
  }

  /// Copy constructor (deep-copy of relations)
  LinkObj(const LinkObj& other) : id(), data(other.data), m_from(nullptr), m_to(nullptr) {
    if (other.m_from) {
      m_from = new FromT(*other.m_from);
    }
    if (other.m_to) {
      m_to = new ToT(*other.m_to);
    }
  }

  /// No assignment operator
  LinkObj& operator=(const LinkObj&) = delete;

public:
  podio::ObjectID id{};
  LinkData data{1.0f};

  std::unique_ptr<FromT> m_from{nullptr};
  std::unique_ptr<ToT> m_to{nullptr};
};

} // namespace podio

#endif // PODIO_DETAIL_LINKOBJ_H
