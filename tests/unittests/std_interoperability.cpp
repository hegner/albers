#include "datamodel/ExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/MutableExampleHit.h"
#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

// DOCUMENTED_STATIC_FAILURE and DOCUMENTED_FAILURE macro are used to indicate checks that corresponds to requirements
// imposed but the standard but currently not met by podio. These check use inverted logic (they pass when the
// requirement is not met) in order to detect when the support for the requirement is added.
// On their failure:
// - replace DOCUMENTED_STATIC_FAILURE with STATIC_REQUIRE_FALSE or DOCUMENTED_FAILURE with REQUIRE_FALSE
// - uncomment checks below it
// - update documentation 'doc/collection_as_container.md'
#define DOCUMENTED_STATIC_FAILURE(...) STATIC_REQUIRE_FALSE(__VA_ARGS__)
#define DOCUMENTED_FAILURE(...) REQUIRE_FALSE(__VA_ARGS__)

using CollectionType = ExampleHitCollection;
using iterator = CollectionType::iterator;
using const_iterator = CollectionType::const_iterator;

namespace traits {

// typename T::value_type
template <typename, typename = void>
struct has_value_type : std::false_type {};
template <typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_value_type_v = has_value_type<T>::value;

// typename T::reference
template <typename, typename = void>
struct has_reference : std::false_type {};
template <typename T>
struct has_reference<T, std::void_t<typename T::reference>> : std::true_type {};
template <typename T>
inline constexpr bool has_reference_v = has_reference<T>::value;

// typename T::const_reference
template <typename, typename = void>
struct has_const_reference : std::false_type {};
template <typename T>
struct has_const_reference<T, std::void_t<typename T::const_reference>> : std::true_type {};
template <typename T>
inline constexpr bool has_const_reference_v = has_const_reference<T>::value;

// typename T::iterator
template <typename, typename = void>
struct has_iterator : std::false_type {};
template <typename T>
struct has_iterator<T, std::void_t<typename T::iterator>> : std::true_type {};
template <typename T>
inline constexpr bool has_iterator_v = has_iterator<T>::value;

// typename T::const_iterator
template <typename, typename = void>
struct has_const_iterator : std::false_type {};
template <typename T>
struct has_const_iterator<T, std::void_t<typename T::const_iterator>> : std::true_type {};
template <typename T>
inline constexpr bool has_const_iterator_v = has_const_iterator<T>::value;

// typename T::difference_type
template <typename, typename = void>
struct has_difference_type : std::false_type {};
template <typename T>
struct has_difference_type<T, std::void_t<typename T::difference_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_difference_type_v = has_difference_type<T>::value;

// typename T::size_type
template <typename, typename = void>
struct has_size_type : std::false_type {};
template <typename T>
struct has_size_type<T, std::void_t<typename T::size_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_size_type_v = has_size_type<T>::value;

// typename T::pointer
template <typename, typename = void>
struct has_pointer : std::false_type {};
template <typename T>
struct has_pointer<T, std::void_t<typename T::pointer>> : std::true_type {};
template <typename T>
inline constexpr bool has_pointer_v = has_pointer<T>::value;

// typename T::allocator_type
template <typename, typename = void>
struct has_allocator_type : std::false_type {};
template <typename T>
struct has_allocator_type<T, std::void_t<typename T::allocator_type>> : std::true_type {};
template <typename T>
inline constexpr bool has_allocator_type_v = has_allocator_type<T>::value;

// is_erasable_allocator_unaware
template <typename, typename = void>
struct is_erasable_allocator_unaware : std::false_type {};
template <typename T>
struct is_erasable_allocator_unaware<
    T,
    std::void_t<decltype(std::allocator_traits<std::allocator<typename T::value_type>>::destroy(
        std::declval<std::allocator<typename T::value_type>&>(),
        std::declval<std::add_pointer_t<typename T::value_type>>()))>> : std::true_type {};
template <typename T>
inline constexpr bool is_erasable_allocator_unaware_v = is_erasable_allocator_unaware<T>::value;

// typename T::iterator_category
template <typename, typename = void>
struct has_iterator_category : std::false_type {};
template <typename T>
struct has_iterator_category<T, std::void_t<typename T::iterator_category>> : std::true_type {};
template <typename T>
inline constexpr bool has_iterator_category_v = has_iterator_category<T>::value;

// T::begin()
template <typename, typename = void>
struct has_begin : std::false_type {};
template <typename T>
struct has_begin<T, std::void_t<decltype(std::declval<T>().begin())>> : std::true_type {};
template <typename T>
inline constexpr bool has_begin_v = has_begin<T>::value;

// T::end()
template <typename, typename = void>
struct has_end : std::false_type {};
template <typename T>
struct has_end<T, std::void_t<decltype(std::declval<T>().end())>> : std::true_type {};
template <typename T>
inline constexpr bool has_end_v = has_end<T>::value;

// T::cbegin()
template <typename, typename = void>
struct has_cbegin : std::false_type {};
template <typename T>
struct has_cbegin<T, std::void_t<decltype(std::declval<T>().cbegin())>> : std::true_type {};
template <typename T>
inline constexpr bool has_cbegin_v = has_cbegin<T>::value;

// T::cend()
template <typename, typename = void>
struct has_cend : std::false_type {};
template <typename T>
struct has_cend<T, std::void_t<decltype(std::declval<T>().cend())>> : std::true_type {};
template <typename T>
inline constexpr bool has_cend_v = has_cend<T>::value;

// T::operator==(T)
template <typename, typename = void>
struct has_equality_comparator : std::false_type {};
template <typename T>
struct has_equality_comparator<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>> : std::true_type {};
template <typename T>
inline constexpr bool has_equality_comparator_v = has_equality_comparator<T>::value;

// T::operator!=(T)
template <typename, typename = void>
struct has_inequality_comparator : std::false_type {};
template <typename T>
struct has_inequality_comparator<T, std::void_t<decltype(std::declval<T>() != std::declval<T>())>> : std::true_type {};
template <typename T>
inline constexpr bool has_inequality_comparator_v = has_inequality_comparator<T>::value;

// T::swap(T)
template <typename, typename = void>
struct has_swap : std::false_type {};
template <typename T>
struct has_swap<T, std::void_t<decltype(std::declval<T>().swap(std::declval<T>()))>> : std::true_type {};
template <typename T>
inline constexpr bool has_swap_v = has_swap<T>::value;

// T::size()
template <typename, typename = void>
struct has_size : std::false_type {};
template <typename T>
struct has_size<T, std::void_t<decltype(std::declval<T>().size())>> : std::true_type {};
template <typename T>
inline constexpr bool has_size_v = has_size<T>::value;

// T::max_size()
template <typename, typename = void>
struct has_max_size : std::false_type {};
template <typename T>
struct has_max_size<T, std::void_t<decltype(std::declval<T>().max_size())>> : std::true_type {};
template <typename T>
inline constexpr bool has_max_size_v = has_max_size<T>::value;

// T::empty()
template <typename, typename = void>
struct has_empty : std::false_type {};
template <typename T>
struct has_empty<T, std::void_t<decltype(std::declval<T>().empty())>> : std::true_type {};
template <typename T>
inline constexpr bool has_empty_v = has_empty<T>::value;

// T::operator*()
template <typename, typename = void>
struct has_indirection : std::false_type {};
template <typename T>
struct has_indirection<T, std::void_t<decltype(*std::declval<T&>())>> : std::true_type {};
template <typename T>
constexpr bool has_indirection_v = has_indirection<T>::value;

// T::operator->()
template <typename, typename = void>
struct has_member_of_pointer : std::false_type {};
template <typename T>
struct has_member_of_pointer<T, std::void_t<decltype(std::declval<T&>().operator->())>> : std::true_type {};
template <typename T>
constexpr bool has_member_of_pointer_v = has_member_of_pointer<T>::value;

// T::operator++() (preincrement)
template <typename, typename = void>
struct has_preincrement : std::false_type {};
template <typename T>
struct has_preincrement<T, std::void_t<decltype(++std::declval<T&>())>> : std::true_type {};
template <typename T>
inline constexpr bool has_preincrement_v = has_preincrement<T>::value;

// T::operator++(int) (postincrement)
template <typename, typename = void>
struct has_postincrement : std::false_type {};
template <typename T>
struct has_postincrement<T, std::void_t<decltype(std::declval<T&>()++)>> : std::true_type {};
template <typename T>
inline constexpr bool has_postincrement_v = has_postincrement<T>::value;

// *It = val
template <typename, typename, typename = void>
struct has_dereference_assignment : std::false_type {};
template <typename T, typename Value>
struct has_dereference_assignment<T, Value, std::void_t<decltype(*std::declval<T&>() = std::declval<Value>())>>
    : std::true_type {};
template <typename T, typename Value>
inline constexpr bool has_dereference_assignment_v = has_dereference_assignment<T, Value>::value;

// *It++ = val
template <typename, typename, typename = void>
struct has_dereference_assignment_increment : std::false_type {};
template <typename T, typename Value>
struct has_dereference_assignment_increment<T, Value,
                                            std::void_t<decltype(*std::declval<T&>()++ = std::declval<Value>())>>
    : std::true_type {};
template <typename T, typename Value>
inline constexpr bool has_dereference_assignment_increment_v = has_dereference_assignment_increment<T, Value>::value;

// T::push_back(Value)
template <typename, typename, typename = void>
struct has_push_back : std::false_type {};
template <typename T, typename Value>
struct has_push_back<T, Value, std::void_t<decltype(std::declval<T>().push_back(std::declval<Value>()))>>
    : std::true_type {};
template <typename T, typename Value>
inline constexpr bool has_push_back_v = has_push_back<T, Value>::value;

// T::push_front(Value)
template <typename, typename, typename = void>
struct has_push_front : std::false_type {};
template <typename T, typename Value>
struct has_push_front<T, Value, std::void_t<decltype(std::declval<T>().push_front(std::declval<Value>()))>>
    : std::true_type {};
template <typename T, typename Value>
inline constexpr bool has_push_front_v = has_push_front<T, Value>::value;

// T::insert(Value)
template <typename, typename, typename, typename = void>
struct has_insert : std::false_type {};
template <typename T, typename Iter, typename Value>
struct has_insert<T, Iter, Value,
                  std::void_t<decltype(std::declval<T>().insert(std::declval<Iter>(), std::declval<Value>()))>>
    : std::true_type {};
template <typename T, typename Iter, typename Value>
inline constexpr bool has_insert_v = has_insert<T, Iter, Value>::value;
} // namespace traits

TEST_CASE("Collection container types", "[collection][container][types][std]") {

  // value_type
  STATIC_REQUIRE(traits::has_value_type_v<CollectionType>);

  // Erasable -allocator aware - mutually exclusive with  Erasable -allocator not aware
  DOCUMENTED_STATIC_FAILURE(traits::has_allocator_type_v<CollectionType>);
  // add check for `std::allocator_traits<A>::destroy(m, p);` expression here
  // STATIC_REQUIRE(...)
  // Erasable -allocator not aware - mutually exclusive // Erasable -allocator aware
  STATIC_REQUIRE(traits::is_erasable_allocator_unaware_v<CollectionType>);

  // reference
  DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<CollectionType>);
  // STATIC_REQUIRE(std::is_same_v<CollectionType::reference, CollectionType::value_type&>);

  // const_reference
  DOCUMENTED_STATIC_FAILURE(traits::has_const_reference_v<CollectionType>);
  // STATIC_REQUIRE(std::is_same_v<CollectionType::const_reference, const CollectionType::value_type&>);

  // iterator
  STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
  DOCUMENTED_STATIC_FAILURE(std::is_convertible_v<CollectionType::iterator, CollectionType::const_iterator>);

  // const_iterator
  STATIC_REQUIRE(traits::has_const_iterator_v<CollectionType>);

  // difference_type
  STATIC_REQUIRE(traits::has_difference_type_v<CollectionType>);
  STATIC_REQUIRE(std::is_signed_v<CollectionType::difference_type>);
  STATIC_REQUIRE(std::is_integral_v<CollectionType::difference_type>);
  DOCUMENTED_STATIC_FAILURE(traits::has_difference_type_v<std::iterator_traits<CollectionType::iterator>>);
  // STATIC_REQUIRE(
  //     std::is_same_v<CollectionType::difference_type,
  //     std::iterator_traits<CollectionType::iterator>::difference_type>);
  DOCUMENTED_STATIC_FAILURE(traits::has_difference_type_v<std::iterator_traits<CollectionType::const_iterator>>);
  // STATIC_REQUIRE(std::is_same_v<CollectionType::difference_type,
  //                               std::iterator_traits<CollectionType::const_iterator>::difference_type>);

  // size_type
  STATIC_REQUIRE(traits::has_size_type_v<CollectionType>);
  STATIC_REQUIRE(std::is_unsigned_v<CollectionType::size_type>);
  STATIC_REQUIRE(std::is_integral_v<CollectionType::size_type>);
  STATIC_REQUIRE(std::numeric_limits<CollectionType::size_type>::max() >=
                 std::numeric_limits<CollectionType::difference_type>::max());
}

TEST_CASE("Collection container members", "[collection][container][members][std]") {
  // C()
  STATIC_REQUIRE(std::is_default_constructible_v<CollectionType>);
  REQUIRE(CollectionType().empty() == true);

  // C(a)
  DOCUMENTED_STATIC_FAILURE(std::is_copy_constructible_v<CollectionType>);

  // C(rv)
  STATIC_REQUIRE(std::is_move_constructible_v<CollectionType>);

  // a = b
  DOCUMENTED_STATIC_FAILURE(std::is_copy_assignable_v<CollectionType>);

  // a = rv
  STATIC_REQUIRE(std::is_move_assignable_v<CollectionType>);

  // a.~C()
  STATIC_REQUIRE(std::is_destructible_v<CollectionType>);

  // a.begin()
  STATIC_REQUIRE(traits::has_begin_v<CollectionType>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<CollectionType>().begin()), CollectionType::iterator>);
  STATIC_REQUIRE(
      std::is_same_v<decltype(std::declval<const CollectionType>().begin()), CollectionType::const_iterator>);

  // a.end()
  STATIC_REQUIRE(traits::has_end_v<CollectionType>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<CollectionType>().end()), CollectionType::iterator>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const CollectionType>().end()), CollectionType::const_iterator>);

  // a.cbegin()
  STATIC_REQUIRE(traits::has_cbegin_v<CollectionType>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<CollectionType>().cbegin()), CollectionType::const_iterator>);

  // a.cend()
  STATIC_REQUIRE(traits::has_cend_v<CollectionType>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<CollectionType>().cend()), CollectionType::const_iterator>);

  // value_type is EqualityComparable
  STATIC_REQUIRE(traits::has_equality_comparator_v<CollectionType::value_type>);
  STATIC_REQUIRE(
      std::is_convertible_v<
          decltype(std::declval<CollectionType::value_type>() != std::declval<CollectionType::value_type>()), bool>);
  // a == b
  DOCUMENTED_STATIC_FAILURE(traits::has_equality_comparator_v<CollectionType>);
  // STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<CollectionType>()==std::declval<CollectionType>()),
  // bool>);

  // a != b
  DOCUMENTED_STATIC_FAILURE(traits::has_inequality_comparator_v<CollectionType>);
  // STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<CollectionType>()!=std::declval<CollectionType>()),
  // bool>);

  // a.swap(b)
  DOCUMENTED_STATIC_FAILURE(traits::has_swap_v<CollectionType>);
  // STATIC_REQUIRE(
  //     std::is_same_v<decltype(std::declval<CollectionType>().swap(std::declval<CollectionType>())), void>);

  // swap(a,b)
  STATIC_REQUIRE(std::is_swappable_v<CollectionType>);

  // a.size()
  STATIC_REQUIRE(traits::has_size_v<CollectionType>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<CollectionType>().size()), CollectionType::size_type>);

  // a.max_size())
  STATIC_REQUIRE(traits::has_max_size_v<CollectionType>);
  STATIC_REQUIRE(std::is_same_v<decltype(std::declval<CollectionType>().max_size()), CollectionType::size_type>);

  // a.empty()
  STATIC_REQUIRE(traits::has_empty_v<CollectionType>);
  STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<CollectionType>().empty()), bool>);
}

TEST_CASE("Collection AllocatorAwareContainer types", "[collection][container][types][std]") {
  DOCUMENTED_STATIC_FAILURE(traits::has_allocator_type_v<CollectionType>);
}
// TODO add tests for AllocatorAwareContainer statements and expressions

TEST_CASE("Collection and iterator concepts") {
#if (__cplusplus >= 202002L)
  SECTION("Iterator") {
    DOCUMENTED_STATIC_FAILURE(std::indirectly_readable<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::indirectly_writable<iterator, CollectionType::value_type>);
    DOCUMENTED_STATIC_FAILURE(std::weakly_incrementable<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::incrementable<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::input_or_output_iterator<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::input_iterator<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::output_iterator<iterator, CollectionType::value_type>);
    DOCUMENTED_STATIC_FAILURE(std::forward_iterator<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::bidirectional_iterator<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::random_access_iterator<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::contiguous_iterator<iterator>);
  }
  SECTION("Const_iterator") {
    DOCUMENTED_STATIC_FAILURE(std::indirectly_readable<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::indirectly_writable<const_iterator, CollectionType::value_type>);
    DOCUMENTED_STATIC_FAILURE(std::weakly_incrementable<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::incrementable<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::input_or_output_iterator<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::input_iterator<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::output_iterator<const_iterator, CollectionType::value_type>);
    DOCUMENTED_STATIC_FAILURE(std::forward_iterator<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::bidirectional_iterator<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::random_access_iterator<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::contiguous_iterator<const_iterator>);
  }
#endif
}

TEST_CASE("Collection iterators", "[collection][container][iterator][std]") {
  // the checks are duplicated for iterator and const_iterator as expectations on them are slightly different

  // nested sections as the requirements make a hierarchy
  SECTION("LegacyForwardIterator") {

    // LegacyForwardIterator requires LegacyInputIterator
    SECTION("LegacyInputIterator") {

      // LegacyInputIterator requires LegacyIterator
      SECTION("LegacyIterator") {

        // CopyConstructible
        // iterator
        DOCUMENTED_STATIC_FAILURE(std::is_move_constructible_v<iterator>);
        DOCUMENTED_STATIC_FAILURE(std::is_copy_constructible_v<iterator>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(std::is_move_constructible_v<const_iterator>);
        DOCUMENTED_STATIC_FAILURE(std::is_copy_constructible_v<const_iterator>);

        // CopyAssignable
        // iterator
        DOCUMENTED_STATIC_FAILURE(std::is_move_assignable_v<iterator>);
        DOCUMENTED_STATIC_FAILURE(std::is_copy_assignable_v<iterator>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(std::is_move_assignable_v<const_iterator>);
        DOCUMENTED_STATIC_FAILURE(std::is_copy_assignable_v<const_iterator>);

        // Destructible
        // iterator
        STATIC_REQUIRE(std::is_nothrow_destructible_v<iterator>);
        // const_iterator
        STATIC_REQUIRE(std::is_nothrow_destructible_v<const_iterator>);

        // Swappable
        // iterator
        DOCUMENTED_STATIC_FAILURE(std::is_swappable_v<iterator&>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(std::is_swappable_v<const_iterator&>);

#if (__cplusplus < 202002L)
        // std::iterator_traits<It>::value_type (required prior to C++20)
        // iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<std::iterator_traits<iterator>>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<std::iterator_traits<const_iterator>>);
#endif
        // std::iterator_traits<It>::difference_type
        // iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_difference_type_v<std::iterator_traits<iterator>>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_difference_type_v<std::iterator_traits<const_iterator>>);

        // std::iterator_traits<It>::reference
        // iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<std::iterator_traits<iterator>>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<std::iterator_traits<const_iterator>>);

        // std::iterator_traits<It>::pointer
        // iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_pointer_v<std::iterator_traits<iterator>>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_pointer_v<std::iterator_traits<const_iterator>>);

        // std::iterator_traits<It>::iterator_category
        // iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
        // const_iterator
        DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);

        // *r
        // iterator
        STATIC_REQUIRE(traits::has_indirection_v<iterator>);
        STATIC_REQUIRE_FALSE(std::is_same_v<void, decltype(*std::declval<iterator&>())>);
        // const_iterator
        STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
        STATIC_REQUIRE_FALSE(std::is_same_v<void, decltype(*std::declval<const_iterator&>())>);

        // ++r
        // iterator
        STATIC_REQUIRE(traits::has_preincrement_v<iterator&>);
        STATIC_REQUIRE(std::is_same_v<decltype(++std::declval<iterator&>()), iterator&>);
        // const_iterator
        STATIC_REQUIRE(traits::has_preincrement_v<const_iterator&>);
        STATIC_REQUIRE(std::is_same_v<decltype(++std::declval<const_iterator&>()), const_iterator&>);

      } // end of LegacyIterator

      // EqualityComparable
      // iterator
      STATIC_REQUIRE(traits::has_equality_comparator_v<iterator>);
      STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<iterator>() != std::declval<iterator>()), bool>);
      // const_iterator
      STATIC_REQUIRE(traits::has_equality_comparator_v<const_iterator>);
      STATIC_REQUIRE(
          std::is_convertible_v<decltype(std::declval<const_iterator>() != std::declval<const_iterator>()), bool>);

      // i != j (contextually convertible)
      // iterator
      STATIC_REQUIRE(traits::has_inequality_comparator_v<iterator>);
      STATIC_REQUIRE(std::is_constructible_v<bool, decltype(std::declval<iterator>() != std::declval<iterator>())>);
      // const_ iterator
      STATIC_REQUIRE(traits::has_inequality_comparator_v<const_iterator>);
      STATIC_REQUIRE(
          std::is_constructible_v<bool, decltype(std::declval<const_iterator>() != std::declval<const_iterator>())>);

      // *i
      // iterator
      STATIC_REQUIRE(traits::has_indirection_v<iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<iterator>);
      // STATIC_REQUIRE(std::is_same_v<std::iterator_traits<iterator>::reference,
      // decltype(*std::declval<iterator>())>);
      DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<iterator>);
      // STATIC_REQUIRE(std::is_convertible_v<decltype(*std::declval<iterator>()),
      // std::iterator_traits<iterator>::value_type>);
      // const_iterator
      STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<const_iterator>);
      // STATIC_REQUIRE(std::is_same_v<std::iterator_traits<const_iterator>::reference,
      // decltype(*std::declval<const_iterator>())>);
      DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<const_iterator>);
      // STATIC_REQUIRE(std::is_convertible_v<decltype(*std::declval<const_iterator>()),
      // std::iterator_traits<const_iterator>::value_type>);

      // i->m
      // iterator
      STATIC_REQUIRE(traits::has_member_of_pointer_v<iterator>);
      STATIC_REQUIRE(traits::has_indirection_v<iterator>);
      STATIC_REQUIRE(
          std::is_same_v<decltype(std::declval<iterator>()->energy()), decltype((*std::declval<iterator>()).energy())>);
      // const_iterator
      STATIC_REQUIRE(traits::has_member_of_pointer_v<const_iterator>);
      STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
      STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const_iterator>()->energy()),
                                    decltype((*std::declval<const_iterator>()).energy())>);

      // ++r
      // iterator
      STATIC_REQUIRE(traits::has_preincrement_v<iterator>);
      STATIC_REQUIRE(std::is_same_v<decltype(++std::declval<iterator&>()), iterator&>);
      // const_iterator
      STATIC_REQUIRE(traits::has_preincrement_v<const_iterator>);
      STATIC_REQUIRE(std::is_same_v<decltype(++std::declval<const_iterator&>()), const_iterator&>);

      // (void)r++
      // iterator
      STATIC_REQUIRE(traits::has_preincrement_v<iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<iterator>);
      // STATIC_REQUIRE(
      //     std::is_same_v<decltype((void)++std::declval<iterator&>()), decltype((void)std::declval<iterator&>()++)>);
      // const_iterator
      STATIC_REQUIRE(traits::has_preincrement_v<const_iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<const_iterator>);
      // STATIC_REQUIRE(std::is_same_v<decltype((void)++std::declval<const_iterator&>()),
      //                               decltype((void)std::declval<const_iterator&>()++)>);

      // *r++
      // iterator
      STATIC_REQUIRE(traits::has_indirection_v<iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<std::iterator_traits<iterator>>);
      // STATIC_REQUIRE(
      //     std::is_convertible_v<decltype(*std::declval<iterator&>()++), std::iterator_traits<iterator>::value_type>);
      // const_iterator
      STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<const_iterator>);
      DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<std::iterator_traits<const_iterator>>);
      // STATIC_REQUIRE(std::is_convertible_v<decltype(*std::declval<const_iterator&>()++),
      //                                      std::iterator_traits<const_iterator>::value_type>);

      // iterator_category - not strictly necessary but advised
      // iterator
      DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
      // STATIC_REQUIRE(std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
      // const_iterator
      DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
      // STATIC_REQUIRE(std::is_base_of_v<std::input_iterator_tag,
      // std::iterator_traits<const_iterator>::iterator_category>);

    } // end of LegacyInputIterator

    // Mutable LegacyForwardIterator (LegacyForwardIterator that is also LegacyOutputIterator):
    // - reference same as value_type& or value_type&&
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<iterator>);
    // STATIC_REQUIRE(
    //     std::is_same_v<std::iterator_traits<iterator>::reference, std::iterator_traits<iterator>::value_type&> ||
    //     std::is_same_v<std::iterator_traits<iterator>::reference, std::iterator_traits<iterator>::value_type&&>);
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<const_iterator>);
    // STATIC_REQUIRE(
    //     std::is_same_v<std::iterator_traits<const_iterator>::reference,
    //     std::iterator_traits<const_iterator>::value_type&> ||
    //     std::is_same_v<std::iterator_traits<const_iterator>::reference,
    //     std::iterator_traits<const_iterator>::value_type&&>);

    // (Immutable) iterator:
    // - reference same as const value_type& or const value_type&&
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<iterator>);
    // STATIC_REQUIRE(std::is_same_v<std::iterator_traits<iterator>::reference,
    //                               const std::iterator_traits<iterator>::value_type&> ||
    //                std::is_same_v<std::iterator_traits<iterator>::reference,
    //                               const std::iterator_traits<iterator>::value_type&&>);
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_value_type_v<const_iterator>);
    // STATIC_REQUIRE(std::is_same_v<std::iterator_traits<const_iterator>::reference,
    //                               const std::iterator_traits<const_iterator>::value_type&> ||
    //                std::is_same_v<std::iterator_traits<const_iterator>::reference,
    //                               const std::iterator_traits<const_iterator>::value_type&&>);

    // DefaultConstructible
    // iterator
    DOCUMENTED_STATIC_FAILURE(std::is_default_constructible_v<iterator>);
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(std::is_default_constructible_v<const_iterator>);

    // Multipass guarantee
    // iterator
    {
      CollectionType coll;
      for (int i = 0; i < 3; ++i) {
        coll.create();
      }
      // iterator
      auto a = coll.begin();
      auto b = coll.begin();
      REQUIRE(a == b);
      REQUIRE(*a == *b);
      REQUIRE(++a == ++b);
      REQUIRE(*a == *b);
      DOCUMENTED_STATIC_FAILURE(std::is_copy_constructible_v<iterator>);
      // auto a_copy = a;
      // ++a_copy;
      // REQUIRE(a == b);
      // REQUIRE(*a == *b);

      // const_iterator
      auto ca = coll.cbegin();
      auto cb = coll.cbegin();
      REQUIRE(ca == cb);
      REQUIRE(*ca == *cb);
      REQUIRE(++ca == ++cb);
      REQUIRE(*ca == *cb);
      DOCUMENTED_STATIC_FAILURE(std::is_copy_constructible_v<const_iterator>);
      // auto ca_copy = ca;
      // ++ca_copy;
      // REQUIRE(ca == cb);
      // REQUIRE(*ca == *cb);
    }

    // Singular iterators
    // iterator
    STATIC_REQUIRE(traits::has_equality_comparator_v<iterator>);
    DOCUMENTED_STATIC_FAILURE(std::is_default_constructible_v<iterator>);
    //{
    //  REQUIRE(iterator{} == iterator{});
    //}
    // const_iterator
    STATIC_REQUIRE(traits::has_equality_comparator_v<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::is_default_constructible_v<const_iterator>);
    //{
    //  REQUIRE(const_iterator{} == const_iterator{});
    //}

    // i++
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<iterator>);
    // STATIC_REQUIRE(std::is_same_v<decltype(std::declval<iterator&>()++), iterator>);
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<const_iterator>);
    // STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const_iterator&>()++), const_iterator>);

    // *i++
    // iterator
    STATIC_REQUIRE(traits::has_indirection_v<iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<std::iterator_traits<iterator>>);
    // STATIC_REQUIRE(std::is_same_v<decltype(*std::declval<iterator&>()++),
    // std::iterator_traits<iterator>::reference>);
    // const_iterator
    STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(traits::has_reference_v<std::iterator_traits<const_iterator>>);
    // STATIC_REQUIRE(
    //     std::is_same_v<decltype(*std::declval<const_iterator&>()++),
    //     std::iterator_traits<const_iterator>::reference>);

    // iterator_category - not strictly necessary but advised
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    // STATIC_REQUIRE(std::is_base_of_v<std::forward_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
    // STATIC_REQUIRE(std::is_base_of_v<std::forward_iterator_tag,
    // std::iterator_traits<const_iterator>::iterator_category>);

  } // end of LegacyForwardIterator

  SECTION("LegacyOutputIterator") {

    // is class type or pointer type
    // iterator
    STATIC_REQUIRE(std::is_pointer_v<iterator> || std::is_class_v<iterator>);
    // const_iterator
    STATIC_REQUIRE(std::is_pointer_v<const_iterator> || std::is_class_v<const_iterator>);

    // *r = o
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_dereference_assignment_v<iterator, CollectionType::value_type>);
    STATIC_REQUIRE(traits::has_dereference_assignment_v<iterator, CollectionType::value_type::mutable_type>);
    {
      auto coll = CollectionType{};
      auto item = coll.create(13ull, 0., 0., 0., 0.);
      REQUIRE(coll.begin()->cellID() == 13ull);
      auto new_item = CollectionType::value_type::mutable_type{42ull, 0., 0., 0., 0.};
      *coll.begin() = new_item;
      DOCUMENTED_FAILURE(coll.begin()->cellID() == 42ull);
    }
    // const_iterator
    STATIC_REQUIRE(traits::has_dereference_assignment_v<const_iterator, CollectionType::value_type>);
    STATIC_REQUIRE(traits::has_dereference_assignment_v<const_iterator, CollectionType::value_type::mutable_type>);
    {
      auto coll = CollectionType{};
      auto item = coll.create(13ull, 0., 0., 0., 0.);
      REQUIRE(coll.cbegin()->cellID() == 13ull);
      auto new_item = CollectionType::value_type::mutable_type{42ull, 0., 0., 0., 0.};
      *coll.cbegin() = new_item;
      DOCUMENTED_FAILURE(coll.cbegin()->cellID() == 42ull);
      new_item.cellID(44ull);
      *coll.cbegin() = static_cast<CollectionType::value_type>(new_item);
      DOCUMENTED_FAILURE(coll.cbegin()->cellID() == 44ull);
    }

    // ++r
    // iterator
    STATIC_REQUIRE(traits::has_preincrement_v<iterator>);
    STATIC_REQUIRE(std::is_same_v<decltype(++std::declval<iterator&>()), iterator&>);
    // const_iterator
    STATIC_REQUIRE(traits::has_preincrement_v<const_iterator>);
    STATIC_REQUIRE(std::is_same_v<decltype(++std::declval<const_iterator&>()), const_iterator&>);

    // r++
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<iterator>);
    // STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<iterator&>()++), const iterator&>);
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_postincrement_v<const_iterator>);
    // STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<const_iterator&>()++), const const_iterator&>);

    // *r++ = o
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_dereference_assignment_increment_v<iterator, CollectionType::value_type>);
    DOCUMENTED_STATIC_FAILURE(
        traits::has_dereference_assignment_increment_v<iterator, CollectionType::value_type::mutable_type>);
    // TODO add runtime check for assignment validity like in '*r = o' case
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(
        traits::has_dereference_assignment_increment_v<const_iterator, CollectionType::value_type>);
    DOCUMENTED_STATIC_FAILURE(
        traits::has_dereference_assignment_increment_v<const_iterator, CollectionType::value_type::mutable_type>);
    // TODO add runtime check for assignment validity like in '*r = o' case

    // iterator_category - not strictly necessary but advised
    // Derived either from:
    // - std::output_iterator_tag
    // - std::forward_iterator_tag (for mutable LegacyForwardIterators)
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    // STATIC_REQUIRE(std::is_base_of_v<std::output_iterator_tag, std::iterator_traits<iterator>::iterator_category> ||
    //                std::is_base_of_v<std::forward_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
    // STATIC_REQUIRE(std::is_base_of_v<std::output_iterator_tag, std::iterator_traits<iterator>::iterator_category> ||
    //                    std::is_base_of_v<std::forward_iterator_tag,
    //                    std::iterator_traits<iterator>::iterator_category>);

  } // end of LegacyOutputIterator
}

TEST_CASE("Collection and std iterator adaptors", "[collection][container][adapter][std]") {
  auto coll = CollectionType();
  SECTION("Reverse iterator") {
    // iterator
    STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    // STATIC_REQUIRE(
    //    std::is_base_of_v<std::bidirectional_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
#if (__cplusplus >= 202002L)
    DOCUMENTED_STATIC_FAILURE(std::bidirectional_iterator<iterator>);
#endif
    // const_iterator
    STATIC_REQUIRE(traits::has_const_iterator_v<CollectionType>);
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
    // STATIC_REQUIRE(
    //    std::is_base_of_v<std::bidirectional_iterator_tag, std::iterator_traits<const_iterator>::iterator_category>);
#if (__cplusplus >= 202002L)
    DOCUMENTED_STATIC_FAILURE(std::bidirectional_iterator<iterator>);
#endif
    // TODO add runtime checks here
  }
  SECTION("Back inserter") {
    DOCUMENTED_STATIC_FAILURE(traits::has_const_reference_v<CollectionType>);
    // STATIC_REQUIRE(traits::has_push_back_v<CollectionType, CollectionType::const_reference>);

    STATIC_REQUIRE(traits::has_value_type_v<CollectionType>);
    STATIC_REQUIRE(traits::has_push_back_v<CollectionType, const CollectionType::value_type&>);
    STATIC_REQUIRE(traits::has_push_back_v<CollectionType, CollectionType::value_type&&>);

    auto it = std::back_inserter(coll);
    // insert immutable to not-SubsetCollection
    REQUIRE_THROWS_AS(it = CollectionType::value_type{}, std::invalid_argument);
    // insert mutable (implicit cast to immutable) to not-SubsetCollection
    REQUIRE_THROWS_AS(it = CollectionType::value_type::mutable_type{}, std::invalid_argument);
    auto subColl = CollectionType{};
    subColl.setSubsetCollection(true);
    auto subIt = std::back_inserter(subColl);
    auto val = coll.create();
    val.cellID(42);
    // insert immutable to SubsetCollection
    REQUIRE_NOTHROW(subIt = val);
    REQUIRE(subColl.begin()->cellID() == 42);
  }

  SECTION("Front inserter") {
    DOCUMENTED_STATIC_FAILURE(traits::has_const_reference_v<CollectionType>);
    // STATIC_REQUIRE(traits::has_push_front_v<CollectionType, CollectionType::const_reference>);

    STATIC_REQUIRE(traits::has_value_type_v<CollectionType>);
    DOCUMENTED_STATIC_FAILURE(traits::has_push_front_v<CollectionType, const CollectionType::value_type&>);
    DOCUMENTED_STATIC_FAILURE(traits::has_push_front_v<CollectionType, CollectionType::value_type&&>);
    // TODO add runtime checks here
  }

  SECTION("Inserter") {
    STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
    DOCUMENTED_STATIC_FAILURE(traits::has_const_reference_v<CollectionType>);
    // STATIC_REQUIRE(
    //     traits::has_insert_v<CollectionType, CollectionType::iterator, CollectionType::const_reference>);

    STATIC_REQUIRE(traits::has_value_type_v<CollectionType>);
    DOCUMENTED_STATIC_FAILURE(
        traits::has_insert_v<CollectionType, CollectionType::iterator, const CollectionType::value_type&>);
    DOCUMENTED_STATIC_FAILURE(
        traits::has_insert_v<CollectionType, CollectionType::iterator, CollectionType::value_type&&>);
    // TODO add runtime checks here
  }

  SECTION("Const iterator") {
    // C++23 required
    DOCUMENTED_STATIC_FAILURE((__cplusplus >= 202302L));
  }

  SECTION("Move iterator") {
    // iterator
    STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    // STATIC_REQUIRE(
    //    std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
#if (__cplusplus >= 202002L)
    DOCUMENTED_STATIC_FAILURE(std::input_iterator<iterator>);
#endif
    // TODO add more checks here
    // const_iterator
    STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
    DOCUMENTED_STATIC_FAILURE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    // STATIC_REQUIRE(
    //    std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
#if (__cplusplus >= 202002L)
    DOCUMENTED_STATIC_FAILURE(std::input_iterator<iterator>);
#endif
    // TODO add more checks here
  }

#if (__cplusplus >= 202002L)
  SECTION("Counted iterator") {
    // iterator
    DOCUMENTED_STATIC_FAILURE(std::input_or_output_iterator<iterator>);
    // TODO add runtime checks
    // const_iterator
    DOCUMENTED_STATIC_FAILURE(std::input_or_output_iterator<const_iterator>);
    // TODO add runtime checks
  }
#endif
}

#undef DOCUMENTED_STATIC_FAILURE
#undef DOCUMENTED_FAILURE