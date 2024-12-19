#ifndef PODIO_DETAIL_LINK_H
#define PODIO_DETAIL_LINK_H

#include "podio/detail/LinkFwd.h"
#include "podio/detail/LinkObj.h"
#include "podio/utilities/MaybeSharedPtr.h"
#include "podio/utilities/TypeHelpers.h"

#ifdef PODIO_JSON_OUTPUT
  #include "nlohmann/json.hpp"
#endif

#include <ostream>
#include <utility> // std::swap

namespace podio {

/// Generalized Link type for both Mutable and immutable (default)
/// versions. User facing classes with the expected naming scheme are defined via
/// template aliases in LinkFwd.h
template <typename FromT, typename ToT, bool Mutable>
class LinkT {
  // The typedefs in LinkFwd.h should make sure that at this point
  // Mutable classes are stripped, i.e. the user should never be able to trigger
  // these!
  static_assert(std::is_same_v<detail::GetDefaultHandleType<FromT>, FromT>,
                "Links need to be instantiated with the default types!");
  static_assert(std::is_same_v<detail::GetDefaultHandleType<ToT>, ToT>,
                "Links need to be instantiated with the default types!");

  using LinkObjT = LinkObj<FromT, ToT>;
  friend LinkCollection<FromT, ToT>;
  friend LinkCollectionIteratorT<FromT, ToT, Mutable>;
  friend LinkT<FromT, ToT, !Mutable>;

  /// Helper member variable to check whether FromU and ToU can be used for this
  /// Link. We need this to make SFINAE trigger in some cases below
  template <typename FromU, typename ToU>
  constexpr static bool sameTypes = std::is_same_v<FromU, FromT> && std::is_same_v<ToU, ToT>;

  /// Variable template to for determining whether T is either FromT or ToT.
  /// Mainly defined for convenience
  template <typename T>
  static constexpr bool isFromOrToT = detail::isInTuple<T, std::tuple<FromT, ToT>>;

  /// Variable template to for determining whether T is either FromT or ToT or
  /// any of their mutable versions.
  template <typename T>
  static constexpr bool isMutableFromOrToT =
      detail::isInTuple<T,
                        std::tuple<FromT, ToT, detail::GetMutableHandleType<FromT>, detail::GetMutableHandleType<ToT>>>;

public:
  using mutable_type = podio::MutableLink<FromT, ToT>;
  using value_type = podio::Link<FromT, ToT>;
  using collection_type = podio::LinkCollection<FromT, ToT>;

  /// Constructor
  LinkT() : m_obj(new LinkObjT{}, podio::utils::MarkOwned) {
  }

  /// Constructor with weight
  LinkT(float weight) : m_obj(new LinkObjT{}, podio::utils::MarkOwned) {
    m_obj->data.weight = weight;
  }

  /// Copy constructor
  LinkT(const LinkT& other) = default;

  /// Assignment operator
  LinkT& operator=(LinkT other) {
    swap(*this, other);
    return *this;
  }

  /// Implicit conversion of mutable to immutable links
  template <typename FromU, typename ToU, typename = std::enable_if_t<Mutable && sameTypes<FromU, ToU>>>
  operator LinkT<FromU, ToU, false>() const {
    return Link<FromU, ToU>(m_obj);
  }

  /// Create a mutable deep-copy
  ///
  /// @param cloneRelations if set to false only the weight will be cloned but
  ///                       not the relations to the objects this link points
  ///                       to. Defaults to true
  ///
  /// @returns A mutable deep-copy of this link which is independent of the
  ///          original one
  template <typename FromU = FromT, typename ToU = ToT, typename = std::enable_if_t<sameTypes<FromU, ToU>>>
  MutableLink<FromU, ToU> clone(bool cloneRelations = true) const {
    auto tmp = new LinkObjT(podio::ObjectID{}, m_obj->data);
    if (cloneRelations) {
      if (m_obj->m_from) {
        tmp->m_from = std::make_unique<FromT>(*m_obj->m_from);
      }
      if (m_obj->m_to) {
        tmp->m_to = std::make_unique<ToT>(*m_obj->m_to);
      }
    }
    return MutableLink<FromU, ToU>(podio::utils::MaybeSharedPtr(tmp, podio::utils::MarkOwned));
  }

  /// Create an empty Link handle
  template <bool Mut = Mutable, typename = std::enable_if_t<!Mut && !Mutable>>
  static Link<FromT, ToT> makeEmpty() {
    return {nullptr};
  }

  /// Get the weight of the link
  float getWeight() const {
    return m_obj->data.weight;
  }

  /// Set the weight of the link
  template <bool Mut = Mutable, typename = std::enable_if_t<Mut && Mutable>>
  void setWeight(float value) {
    m_obj->data.weight = value;
  }

  /// Access the related-from object
  const FromT getFrom() const {
    if (!m_obj->m_from) {
      return FromT::makeEmpty();
    }
    return FromT(*(m_obj->m_from));
  }

  /// Set the related-from object
  ///
  /// @note All setFrom overloads are equivalent and the correct one is selected
  /// at compile time. We need to differentiate between the handles, only to
  /// make the python bindings work
  template <typename FromU,
            std::enable_if_t<Mutable && std::is_same_v<detail::GetDefaultHandleType<FromU>, FromT> &&
                                 detail::isDefaultHandleType<FromU>,
                             int> = 0>
  void setFrom(FromU value) {
    m_obj->m_from = std::make_unique<detail::GetDefaultHandleType<FromU>>(value);
  }

  /// Set the related-from object
  ///
  /// @note All setFrom overloads are equivalent and the correct one is selected
  /// at compile time. We need to differentiate between the handles, only to
  /// make the python bindings work
  template <typename FromU,
            std::enable_if_t<Mutable && std::is_same_v<detail::GetMutableHandleType<FromT>, FromU> &&
                                 detail::isMutableHandleType<FromU>,
                             int> = 0>
  void setFrom(FromU value) {
    setFrom(detail::GetDefaultHandleType<FromU>(value));
  }

  /// Set the related-from object
  ///
  /// @note All setFrom overloads are equivalent and the correct one is selected
  /// at compile time. We need this overload to allow for an implicit conversion
  /// to interface types in case the relation contains an interface type.
  template <typename FromU, std::enable_if_t<Mutable && detail::isInterfaceInitializableFrom<FromT, FromU>, int> = 0>
  void setFrom(FromU value) {
    setFrom(FromT(value));
  }

  /// Access the related-to object
  const ToT getTo() const {
    if (!m_obj->m_to) {
      return ToT::makeEmpty();
    }
    return ToT(*(m_obj->m_to));
  }

  /// Set the related-to object
  ///
  /// @note All setTo overloads are equivalent and the correct one is selected
  /// at compile time. We need to differentiate between the handles, only to
  /// make the python bindings work
  template <typename ToU,
            std::enable_if_t<Mutable && std::is_same_v<detail::GetDefaultHandleType<ToU>, ToT> &&
                                 detail::isDefaultHandleType<ToU>,
                             int> = 0>
  void setTo(ToU value) {
    m_obj->m_to = std::make_unique<detail::GetDefaultHandleType<ToU>>(value);
  }

  /// Set the related-to object
  ///
  /// @note All setTo overloads are equivalent and the correct one is selected
  /// at compile time. We need to differentiate between the handles, only to
  /// make the python bindings work
  template <typename ToU,
            std::enable_if_t<Mutable && std::is_same_v<detail::GetMutableHandleType<ToT>, ToU> &&
                                 detail::isMutableHandleType<ToU>,
                             int> = 0>
  void setTo(ToU value) {
    setTo(detail::GetDefaultHandleType<ToU>(value));
  }

  /// Set the related-to object
  ///
  /// @note All setTo overloads are equivalent and the correct one is selected
  /// at compile time. We need this overload to allow for an implicit conversion
  /// to interface types in case the relation contains an interface type.
  template <typename ToU, std::enable_if_t<Mutable && detail::isInterfaceInitializableFrom<ToT, ToU>, int> = 0>
  void setTo(ToU value) {
    setTo(ToT(value));
  }

  /// Templated version for getting an element of the link by type. Only
  /// available for Links where FromT and ToT are **not the same type**,
  /// and if the requested type is actually part of the Link. It is only
  /// possible to get the immutable types from this. Will result in a
  /// compilation error if any of these conditions is not met.
  ///
  /// @tparam T the desired type
  /// @returns T the element of the Link
  template <typename T, typename = std::enable_if_t<!std::is_same_v<ToT, FromT> && isFromOrToT<T>>>
  T get() const {
    if constexpr (std::is_same_v<T, FromT>) {
      return getFrom();
    } else {
      return getTo();
    }
  }

  /// Tuple like index based access to the elements of the Link. Returns
  /// only immutable types of the links. This method enables structured
  /// bindings for Links.
  ///
  /// @tparam Index an index (smaller than 3) to access an element of the Link
  /// @returns Depending on the value of Index:
  ///   - 0: The From element of the Link
  ///   - 1: The To element of the Link
  ///   - 2: The weight of the Link
  template <size_t Index, typename = std::enable_if_t<(Index < 3)>>
  auto get() const {
    if constexpr (Index == 0) {
      return getFrom();
    } else if constexpr (Index == 1) {
      return getTo();
    } else {
      return getWeight();
    }
  }

  /// Templated version for setting an element of the link by type. Only
  /// available for Links where FromT and ToT are **not the same type**,
  /// and if the requested type is actually part of the Link. Will result
  /// in a compilation error if any of these conditions is not met.
  ///
  /// @tparam T type of value (**inferred!**)
  /// @param value the element to set for this link.
  template <typename T,
            std::enable_if_t<Mutable && !std::is_same_v<ToT, FromT> &&
                                 (isMutableFromOrToT<T> || detail::isInterfaceInitializableFrom<ToT, T> ||
                                  detail::isInterfaceInitializableFrom<FromT, T>),
                             int> = 0>
  void set(T value) {
    if constexpr (std::is_same_v<T, FromT>) {
      setFrom(std::move(value));
    } else {
      setTo(std::move(value));
    }
  }

  /// check whether the object is actually available
  bool isAvailable() const {
    return m_obj;
  }

  /// disconnect from Link instance
  void unlink() {
    m_obj = podio::utils::MaybeSharedPtr<LinkObjT>(nullptr);
  }

  /// Get the ObjectID
  podio::ObjectID getObjectID() const {
    if (m_obj) {
      return m_obj->id;
    }
    return podio::ObjectID{};
  }

  podio::ObjectID id() const {
    return getObjectID();
  }

  bool operator==(const LinkT& other) const {
    return m_obj == other.m_obj;
  }

  bool operator!=(const LinkT& other) const {
    return !(*this == other);
  }

  template <typename FromU, typename ToU, typename = std::enable_if_t<sameTypes<FromU, ToU>>>
  bool operator==(const LinkT<FromU, ToU, !Mutable>& other) const {
    return m_obj == other.m_obj;
  }

  template <typename FromU, typename ToU, typename = std::enable_if_t<sameTypes<FromU, ToU>>>
  bool operator!=(const LinkT<FromU, ToU, !Mutable>& other) const {
    return !(*this == other);
  }

  bool operator<(const LinkT& other) const {
    return m_obj < other.m_obj;
  }

  friend void swap(LinkT& a, LinkT& b) {
    using std::swap;
    swap(a.m_obj, b.m_obj); // swap out the internal pointers
  }

private:
  /// Constructor from existing LinkObj
  explicit LinkT(podio::utils::MaybeSharedPtr<LinkObjT> obj) : m_obj(std::move(obj)) {
  }

  template <bool Mut = Mutable, typename = std::enable_if_t<!Mut && !Mutable>>
  LinkT(LinkObjT* obj) : m_obj(podio::utils::MaybeSharedPtr<LinkObjT>(obj)) {
  }

  podio::utils::MaybeSharedPtr<LinkObjT> m_obj{nullptr};
};

template <typename FromT, typename ToT>
std::ostream& operator<<(std::ostream& os, const Link<FromT, ToT>& link) {
  if (!link.isAvailable()) {
    return os << "[not available]";
  }

  return os << " id: " << link.id() << '\n'
            << " weight: " << link.getWeight() << '\n'
            << " from: " << link.getFrom().id() << '\n'
            << " to: " << link.getTo().id() << '\n';
}

#if defined(PODIO_JSON_OUTPUT) && !defined(__CLING__)
template <typename FromT, typename ToT>
void to_json(nlohmann::json& j, const podio::LinkT<FromT, ToT, false>& link) {
  j = nlohmann::json{{"weight", link.getWeight()}};

  j["from"] = nlohmann::json{{"collectionID", link.getFrom().getObjectID().collectionID},
                             {"index", link.getFrom().getObjectID().index}};

  j["to"] = nlohmann::json{{"collectionID", link.getTo().getObjectID().collectionID},
                           {"index", link.getTo().getObjectID().index}};
}
#endif

} // namespace podio

#endif // PODIO_DETAIL_LINK_H
