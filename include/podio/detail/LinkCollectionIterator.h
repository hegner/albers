#ifndef PODIO_DETAIL_LINKCOLLECTIONITERATOR_H
#define PODIO_DETAIL_LINKCOLLECTIONITERATOR_H

#include "podio/detail/LinkFwd.h"
#include "podio/utilities/MaybeSharedPtr.h"
#include <iterator>

namespace podio {
template <typename FromT, typename ToT, bool Mutable>
class LinkCollectionIteratorT {
  using LinkType = LinkT<FromT, ToT, Mutable>;
  using LinkObjT = LinkObj<FromT, ToT>;

public:
  using value_type = LinkType;
  using difference_type = ptrdiff_t;
  using reference = LinkType;
  using pointer = LinkType*;
  using iterator_category = std::input_iterator_tag;
  // `std::forward_iterator` is supported except that the pointers obtained with `operator->()`
  // remain valid as long as the iterator is valid, not as long as the range is valid.
  using iterator_concept = std::bidirectional_iterator_tag;

  LinkCollectionIteratorT(size_t index, const LinkObjPointerContainer<FromT, ToT>* coll) :
      m_index(index), m_object(podio::utils::MaybeSharedPtr<LinkObjT>{nullptr}), m_collection(coll) {
  }
  LinkCollectionIteratorT() = default;
  LinkCollectionIteratorT(const LinkCollectionIteratorT&) = default;
  LinkCollectionIteratorT& operator=(const LinkCollectionIteratorT&) = default;
  LinkCollectionIteratorT(LinkCollectionIteratorT&&) = default;
  LinkCollectionIteratorT& operator=(LinkCollectionIteratorT&&) = default;
  ~LinkCollectionIteratorT() = default;

  bool operator!=(const LinkCollectionIteratorT& other) const {
    return m_index != other.m_index;
  }

  bool operator==(const LinkCollectionIteratorT& other) const {
    return m_index == other.m_index;
  }

  LinkType operator*() const {
    return LinkType{podio::utils::MaybeSharedPtr<LinkObjT>((*m_collection)[m_index])};
  }

  LinkType* operator->() {
    m_object.m_obj = podio::utils::MaybeSharedPtr<LinkObjT>((*m_collection)[m_index]);
    return &m_object;
  }

  LinkCollectionIteratorT& operator++() {
    ++m_index;
    return *this;
  }

  LinkCollectionIteratorT operator++(int) {
    auto copy = *this;
    ++m_index;
    return copy;
  }

  LinkCollectionIteratorT& operator--() {
    --m_index;
    return *this;
  }

  LinkCollectionIteratorT operator--(int) {
    auto copy = *this;
    --m_index;
    return copy;
  }

private:
  size_t m_index{0};
  LinkType m_object{podio::utils::MaybeSharedPtr<LinkObjT>{nullptr}};
  const LinkObjPointerContainer<FromT, ToT>* m_collection{nullptr};
};
} // namespace podio

#endif // PODIO_DETAIL_LINKCOLLECTIONITERATOR_H
