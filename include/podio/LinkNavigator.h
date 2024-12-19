#ifndef PODIO_LINKNAVIGATOR_H
#define PODIO_LINKNAVIGATOR_H

#include <map>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace podio {

namespace detail::links {
  /// A small struct that simply bundles an object and its weight for a more
  /// convenient return value for the LinkNavigator
  ///
  /// @note In most uses the names of the members should not really matter as it
  /// is possible to us this via structured bindings
  template <typename T>
  struct WeightedObject {
    WeightedObject(T obj, float w) : o(obj), weight(w) {
    }
    T o;          ///< The object
    float weight; ///< The weight in the link

    bool operator==(const WeightedObject<T>& other) const {
      return other.o == o && other.weight == weight;
    }
  };

  /// Simple struct tag for overload selection in LinkNavigator below
  struct ReturnFromTag {};
  /// Simple struct tag for overload selection in LinkNavigator below
  struct ReturnToTag {};
} // namespace detail::links

/// Tag variable to select the lookup of *From* objects have links with a *To*
/// object in podio::LinkNavigator::getLinked
static constexpr detail::links::ReturnFromTag ReturnFrom;
/// Tag variable to select the lookup of *To* objects that have links with a
/// *From* object in podio::LinkNavigator::getLinked
static constexpr detail::links::ReturnToTag ReturnTo;

/// A helper class to more easily handle one-to-many links.
///
/// Internally simply populates two maps in its constructor and then queries
/// them to retrieve objects that are linked with another.
///
/// @note There are no guarantees on the order of the objects in these maps.
/// Hence, there are also no guarantees on the order of the returned objects,
/// even if there inherintly is an order to them in the underlying links
/// collection.
template <typename LinkCollT>
class LinkNavigator {
  using FromT = typename LinkCollT::from_type;
  using ToT = typename LinkCollT::to_type;

  template <typename T>
  using WeightedObject = detail::links::WeightedObject<T>;

public:
  /// Construct a navigator from an link collection
  LinkNavigator(const LinkCollT& links);

  /// We do only construct from a collection
  LinkNavigator() = delete;
  LinkNavigator(const LinkNavigator&) = default;
  LinkNavigator& operator=(const LinkNavigator&) = default;
  LinkNavigator(LinkNavigator&&) = default;
  LinkNavigator& operator=(LinkNavigator&&) = default;

  /// Get all the *From* objects and weights that have links with the passed
  /// object
  ///
  /// You will get this overload if you pass the podio::ReturnFrom tag as second
  /// argument
  ///
  /// @note This overload works always, even if the LinkCollection that was used
  /// to construct this instance of the LinkNavigator has the same From and To
  /// types.
  ///
  /// @param object The object that is labeled *To* in the link
  /// @param . tag variable for selecting this overload
  ///
  /// @returns A vector of all objects and their weights that have links with
  ///          the passed object
  std::vector<WeightedObject<FromT>> getLinked(const ToT& object, podio::detail::links::ReturnFromTag) const {
    const auto& [begin, end] = m_to2from.equal_range(object);
    std::vector<WeightedObject<FromT>> result;
    result.reserve(std::distance(begin, end));

    for (auto it = begin; it != end; ++it) {
      result.emplace_back(it->second);
    }
    return result;
  }

  /// Get all the *From* objects and weights that have links with the passed
  /// object
  ///
  /// @note This overload will automatically do the right thing (TM) in case the
  /// LinkCollection that has been passed to construct this LinkNavigator has
  /// different From and To types.
  ///
  /// @param object The object that is labeled *To* in the link
  ///
  /// @returns A vector of all objects and their weights that have links with
  ///          the passed object
  template <typename ToU = ToT>
  std::enable_if_t<!std::is_same_v<FromT, ToU>, std::vector<WeightedObject<FromT>>> getLinked(const ToT& object) const {
    return getLinked(object, podio::ReturnFrom);
  }

  /// Get all the *To* objects and weights that have links with the passed
  /// object
  ///
  /// You will get this overload if you pass the podio::ReturnTo tag as second
  /// argument
  ///
  /// @note This overload works always, even if the LinkCollection that was used
  /// to construct this instance of the LinkNavigator has the same From and To
  /// types.
  ///
  /// @param object The object that is labeled *From* in the link
  /// @param . tag variable for selecting this overload
  ///
  /// @returns A vector of all objects and their weights that have links with
  ///          the passed object
  std::vector<WeightedObject<ToT>> getLinked(const FromT& object, podio::detail::links::ReturnToTag) const {
    const auto& [begin, end] = m_from2to.equal_range(object);
    std::vector<WeightedObject<ToT>> result;
    result.reserve(std::distance(begin, end));

    for (auto it = begin; it != end; ++it) {
      result.emplace_back(it->second);
    }
    return result;
  }

  /// Get all the *To* objects and weights that have links with the passed
  /// object
  ///
  /// @note This overload will automatically do the right thing (TM) in case the
  /// LinkCollection that has been passed to construct this LinkNavigator has
  /// different From and To types.
  ///
  /// @param object The object that is labeled *From* in the link
  ///
  /// @returns A vector of all objects and their weights that have links with
  ///          the passed object
  template <typename FromU = FromT>
  std::enable_if_t<!std::is_same_v<FromU, ToT>, std::vector<WeightedObject<ToT>>> getLinked(const FromT& object) const {
    return getLinked(object, podio::ReturnTo);
  }

private:
  std::multimap<FromT, WeightedObject<ToT>> m_from2to{}; ///< Map the from to the to objects
  std::multimap<ToT, WeightedObject<FromT>> m_to2from{}; ///< Map the to to the from objects
};

template <typename LinkCollT>
LinkNavigator<LinkCollT>::LinkNavigator(const LinkCollT& links) {
  for (const auto& [from, to, weight] : links) {
    m_from2to.emplace(std::piecewise_construct, std::forward_as_tuple(from), std::forward_as_tuple(to, weight));
    m_to2from.emplace(std::piecewise_construct, std::forward_as_tuple(to), std::forward_as_tuple(from, weight));
  }
}

} // namespace podio

#endif // PODIO_LINKNAVIGATOR_H
