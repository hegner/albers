#include "datamodel/ExampleHit.h"
#include "datamodel/ExampleHitCollection.h"
#include "datamodel/MutableExampleHit.h"

#include "podio/LinkCollection.h"
#include "podio/RelationRange.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>

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
constexpr bool has_value_type_v = has_value_type<T>::value;

// typename T::reference
template <typename, typename = void>
struct has_reference : std::false_type {};
template <typename T>
struct has_reference<T, std::void_t<typename T::reference>> : std::true_type {};
template <typename T>
constexpr bool has_reference_v = has_reference<T>::value;

// typename T::const_reference
template <typename, typename = void>
struct has_const_reference : std::false_type {};
template <typename T>
struct has_const_reference<T, std::void_t<typename T::const_reference>> : std::true_type {};
template <typename T>
constexpr bool has_const_reference_v = has_const_reference<T>::value;

// typename T::iterator
template <typename, typename = void>
struct has_iterator : std::false_type {};
template <typename T>
struct has_iterator<T, std::void_t<typename T::iterator>> : std::true_type {};
template <typename T>
constexpr bool has_iterator_v = has_iterator<T>::value;

// typename T::const_iterator
template <typename, typename = void>
struct has_const_iterator : std::false_type {};
template <typename T>
struct has_const_iterator<T, std::void_t<typename T::const_iterator>> : std::true_type {};
template <typename T>
constexpr bool has_const_iterator_v = has_const_iterator<T>::value;

// typename T::difference_type
template <typename, typename = void>
struct has_difference_type : std::false_type {};
template <typename T>
struct has_difference_type<T, std::void_t<typename T::difference_type>> : std::true_type {};
template <typename T>
constexpr bool has_difference_type_v = has_difference_type<T>::value;

// typename T::size_type
template <typename, typename = void>
struct has_size_type : std::false_type {};
template <typename T>
struct has_size_type<T, std::void_t<typename T::size_type>> : std::true_type {};
template <typename T>
constexpr bool has_size_type_v = has_size_type<T>::value;

// typename T::pointer
template <typename, typename = void>
struct has_pointer : std::false_type {};
template <typename T>
struct has_pointer<T, std::void_t<typename T::pointer>> : std::true_type {};
template <typename T>
constexpr bool has_pointer_v = has_pointer<T>::value;

// typename T::allocator_type
template <typename, typename = void>
struct has_allocator_type : std::false_type {};
template <typename T>
struct has_allocator_type<T, std::void_t<typename T::allocator_type>> : std::true_type {};
template <typename T>
constexpr bool has_allocator_type_v = has_allocator_type<T>::value;

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
constexpr bool is_erasable_allocator_unaware_v = is_erasable_allocator_unaware<T>::value;

// typename T::iterator_category
template <typename, typename = void>
struct has_iterator_category : std::false_type {};
template <typename T>
struct has_iterator_category<T, std::void_t<typename T::iterator_category>> : std::true_type {};
template <typename T>
constexpr bool has_iterator_category_v = has_iterator_category<T>::value;

// T::begin()
template <typename, typename = void>
struct has_begin : std::false_type {};
template <typename T>
struct has_begin<T, std::void_t<decltype(std::declval<T>().begin())>> : std::true_type {};
template <typename T>
constexpr bool has_begin_v = has_begin<T>::value;

// T::end()
template <typename, typename = void>
struct has_end : std::false_type {};
template <typename T>
struct has_end<T, std::void_t<decltype(std::declval<T>().end())>> : std::true_type {};
template <typename T>
constexpr bool has_end_v = has_end<T>::value;

// T::cbegin()
template <typename, typename = void>
struct has_cbegin : std::false_type {};
template <typename T>
struct has_cbegin<T, std::void_t<decltype(std::declval<T>().cbegin())>> : std::true_type {};
template <typename T>
constexpr bool has_cbegin_v = has_cbegin<T>::value;

// T::cend()
template <typename, typename = void>
struct has_cend : std::false_type {};
template <typename T>
struct has_cend<T, std::void_t<decltype(std::declval<T>().cend())>> : std::true_type {};
template <typename T>
constexpr bool has_cend_v = has_cend<T>::value;

// T::operator==(T)
template <typename, typename = void>
struct has_equality_comparator : std::false_type {};
template <typename T>
struct has_equality_comparator<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>> : std::true_type {};
template <typename T>
constexpr bool has_equality_comparator_v = has_equality_comparator<T>::value;

// T::operator!=(T)
template <typename, typename = void>
struct has_inequality_comparator : std::false_type {};
template <typename T>
struct has_inequality_comparator<T, std::void_t<decltype(std::declval<T>() != std::declval<T>())>> : std::true_type {};
template <typename T>
constexpr bool has_inequality_comparator_v = has_inequality_comparator<T>::value;

// T::swap(T)
template <typename, typename = void>
struct has_swap : std::false_type {};
template <typename T>
struct has_swap<T, std::void_t<decltype(std::declval<T>().swap(std::declval<T>()))>> : std::true_type {};
template <typename T>
constexpr bool has_swap_v = has_swap<T>::value;

// T::size()
template <typename, typename = void>
struct has_size : std::false_type {};
template <typename T>
struct has_size<T, std::void_t<decltype(std::declval<T>().size())>> : std::true_type {};
template <typename T>
constexpr bool has_size_v = has_size<T>::value;

// T::max_size()
template <typename, typename = void>
struct has_max_size : std::false_type {};
template <typename T>
struct has_max_size<T, std::void_t<decltype(std::declval<T>().max_size())>> : std::true_type {};
template <typename T>
constexpr bool has_max_size_v = has_max_size<T>::value;

// T::empty()
template <typename, typename = void>
struct has_empty : std::false_type {};
template <typename T>
struct has_empty<T, std::void_t<decltype(std::declval<T>().empty())>> : std::true_type {};
template <typename T>
constexpr bool has_empty_v = has_empty<T>::value;

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
constexpr bool has_preincrement_v = has_preincrement<T>::value;

// T::operator++(int) (postincrement)
template <typename, typename = void>
struct has_postincrement : std::false_type {};
template <typename T>
struct has_postincrement<T, std::void_t<decltype(std::declval<T&>()++)>> : std::true_type {};
template <typename T>
constexpr bool has_postincrement_v = has_postincrement<T>::value;

// *It = val
template <typename, typename, typename = void>
struct has_dereference_assignment : std::false_type {};
template <typename T, typename Value>
struct has_dereference_assignment<T, Value, std::void_t<decltype(*std::declval<T&>() = std::declval<Value>())>>
    : std::true_type {};
template <typename T, typename Value>
constexpr bool has_dereference_assignment_v = has_dereference_assignment<T, Value>::value;

// *It++ = val
template <typename, typename, typename = void>
struct has_dereference_assignment_increment : std::false_type {};
template <typename T, typename Value>
struct has_dereference_assignment_increment<T, Value,
                                            std::void_t<decltype(*std::declval<T&>()++ = std::declval<Value>())>>
    : std::true_type {};
template <typename T, typename Value>
constexpr bool has_dereference_assignment_increment_v = has_dereference_assignment_increment<T, Value>::value;

// T::push_back(Value)
template <typename, typename, typename = void>
struct has_push_back : std::false_type {};
template <typename T, typename Value>
struct has_push_back<T, Value, std::void_t<decltype(std::declval<T>().push_back(std::declval<Value>()))>>
    : std::true_type {};
template <typename T, typename Value>
constexpr bool has_push_back_v = has_push_back<T, Value>::value;

// T::push_front(Value)
template <typename, typename, typename = void>
struct has_push_front : std::false_type {};
template <typename T, typename Value>
struct has_push_front<T, Value, std::void_t<decltype(std::declval<T>().push_front(std::declval<Value>()))>>
    : std::true_type {};
template <typename T, typename Value>
constexpr bool has_push_front_v = has_push_front<T, Value>::value;

// T::insert(Value)
template <typename, typename, typename, typename = void>
struct has_insert : std::false_type {};
template <typename T, typename Iter, typename Value>
struct has_insert<T, Iter, Value,
                  std::void_t<decltype(std::declval<T>().insert(std::declval<Iter>(), std::declval<Value>()))>>
    : std::true_type {};
template <typename T, typename Iter, typename Value>
constexpr bool has_insert_v = has_insert<T, Iter, Value>::value;
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
  STATIC_REQUIRE(traits::has_value_type_v<CollectionType::iterator>);
  STATIC_REQUIRE(
      std::is_same_v<CollectionType::value_type, std::iterator_traits<CollectionType::iterator>::value_type>);

  // const_iterator
  STATIC_REQUIRE(traits::has_const_iterator_v<CollectionType>);
  STATIC_REQUIRE(traits::has_value_type_v<CollectionType::const_iterator>);
  STATIC_REQUIRE(
      std::is_same_v<CollectionType::value_type, std::iterator_traits<CollectionType::const_iterator>::value_type>);

  // difference_type
  STATIC_REQUIRE(traits::has_difference_type_v<CollectionType>);
  STATIC_REQUIRE(std::is_signed_v<CollectionType::difference_type>);
  STATIC_REQUIRE(std::is_integral_v<CollectionType::difference_type>);
  STATIC_REQUIRE(traits::has_difference_type_v<std::iterator_traits<CollectionType::iterator>>);
  STATIC_REQUIRE(
      std::is_same_v<CollectionType::difference_type, std::iterator_traits<CollectionType::iterator>::difference_type>);
  STATIC_REQUIRE(traits::has_difference_type_v<std::iterator_traits<CollectionType::const_iterator>>);
  STATIC_REQUIRE(std::is_same_v<CollectionType::difference_type,
                                std::iterator_traits<CollectionType::const_iterator>::difference_type>);

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

TEST_CASE("Collection and iterator concepts", "[collection][container][iterator][std]") {

  SECTION("input_or_output_iterator") {
    // weakly incrementable
    // iterator
    STATIC_REQUIRE(std::weakly_incrementable<iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      auto it1 = coll.begin();
      auto it2 = coll.begin();
      REQUIRE(it1 == it2);
      ++it1;
      it2++;
      REQUIRE(it1 == it2);
      it1 = coll.begin();
      REQUIRE(std::addressof(++it1) == std::addressof(it1));
    }
    // const_iterator
    STATIC_REQUIRE(std::weakly_incrementable<const_iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      auto it1 = coll.cbegin();
      auto it2 = coll.cbegin();
      REQUIRE(it1 == it2);
      ++it1;
      it2++;
      REQUIRE(it1 == it2);
      it1 = coll.cbegin();
      REQUIRE(std::addressof(++it1) == std::addressof(it1));
    }

    // incrementable
    // iterator
    STATIC_REQUIRE(std::incrementable<iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      auto a = coll.begin();
      auto b = coll.begin();
      REQUIRE(bool(a == b));
      REQUIRE(bool(a++ == b));
      a = coll.begin();
      REQUIRE(bool(a == b));
      REQUIRE(bool(((void)a++, a) == ++b));
    }
    // const_iterator
    STATIC_REQUIRE(std::incrementable<const_iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      auto a = coll.cbegin();
      auto b = coll.cbegin();
      REQUIRE(bool(a == b));
      REQUIRE(bool(a++ == b));
      a = coll.cbegin();
      REQUIRE(bool(a == b));
      REQUIRE(bool(((void)a++, a) == ++b));
    }

    // input_or_output_iterator
    // iterator
    STATIC_REQUIRE(std::input_or_output_iterator<iterator>);
    // const_iterator
    STATIC_REQUIRE(std::input_or_output_iterator<iterator>);
  }

  SECTION("input_iterator") {
    // indirectly_readable
    // iterator
    STATIC_REQUIRE(std::indirectly_readable<iterator>);
    // const_iterator
    STATIC_REQUIRE(std::indirectly_readable<const_iterator>);

    // input_iterator
    // iterator
    STATIC_REQUIRE(std::input_iterator<iterator>);
    // const_iterator
    STATIC_REQUIRE(std::input_iterator<const_iterator>);
  }

  SECTION("forward_iterator") {
    // iterator
    STATIC_REQUIRE(std::forward_iterator<iterator>);
    {
      REQUIRE(iterator{} == iterator{});
      auto coll = CollectionType();
      coll.create();
      auto i = coll.begin();
      auto j = coll.begin();
      REQUIRE(i == j);
      REQUIRE(++i == ++j);
      i = coll.begin();
      REQUIRE(((void)[](auto x) { ++x; }(i), *i) == *i);
    }
    // const_iterator
    STATIC_REQUIRE(std::forward_iterator<const_iterator>);
    {
      REQUIRE(const_iterator{} == const_iterator{});
      auto coll = CollectionType();
      coll.create();
      auto i = coll.cbegin();
      auto j = coll.cbegin();
      REQUIRE(i == j);
      REQUIRE(++i == ++j);
      i = coll.cbegin();
      REQUIRE(((void)[](auto x) { ++x; }(i), *i) == *i);
    }
  }

  SECTION("bidirectional_iterator") {
    // iterator
    STATIC_REQUIRE(std::bidirectional_iterator<iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      auto a = ++coll.begin();
      REQUIRE(std::addressof(--a) == std::addressof(a));
      a = ++coll.begin();
      auto b = ++coll.begin();
      REQUIRE(a == b);
      REQUIRE(a-- == b);
      a = ++coll.begin();
      REQUIRE(a == b);
      a--;
      --b;
      REQUIRE(a == b);
      a = ++coll.begin();
      b = ++coll.begin();
      REQUIRE(a == b);
      REQUIRE(--(++a) == b);
      REQUIRE(++(--a) == b);
    }
    // const_iterator
    STATIC_REQUIRE(std::bidirectional_iterator<const_iterator>);
    auto coll = CollectionType();
    coll.create();
    auto a = ++coll.cbegin();
    REQUIRE(std::addressof(--a) == std::addressof(a));
    a = ++coll.cbegin();
    auto b = ++coll.cbegin();
    REQUIRE(a == b);
    REQUIRE(a-- == b);
    a = ++coll.cbegin();
    REQUIRE(a == b);
    a--;
    --b;
    REQUIRE(a == b);
    a = ++coll.cbegin();
    b = ++coll.cbegin();
    REQUIRE(a == b);
    REQUIRE(--(++a) == b);
    REQUIRE(++(--a) == b);
  }

  SECTION("random_access_iterator") {
    // iterator
    STATIC_REQUIRE(std::totally_ordered<iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      coll.create();
      coll.create();
      auto a = coll.begin();
      auto b = coll.begin();
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE((a == b));
      b = ++coll.begin();
      REQUIRE((a < b));
      REQUIRE(!(a > b));
      REQUIRE(!(a == b));
      a = ++coll.begin();
      b = coll.begin();
      REQUIRE(!(a < b));
      REQUIRE(a > b);
      REQUIRE(!(a == b));
      auto c = coll.begin();
      a = c++;
      b = c++;
      REQUIRE(a < b);
      REQUIRE(b < c);
      REQUIRE(a < c);
      REQUIRE((a > b) == (b < a));
      REQUIRE((a >= b) == !(a < b));
      REQUIRE((a <= b) == !(a > b));
    }
    STATIC_REQUIRE(std::sized_sentinel_for<iterator, iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      auto i = coll.begin();
      auto s = coll.end();
      auto n = 1;
      REQUIRE(s - i == n);
      REQUIRE(i - s == -n);
    }
    STATIC_REQUIRE(std::random_access_iterator<iterator>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      coll.create().cellID(43);
      coll.create().cellID(44);
      coll.create().cellID(45);
      auto a = coll.begin();
      auto n = 2;
      auto b = a + n;
      REQUIRE((a += n) == b);
      a = coll.begin();
      REQUIRE(std::addressof(a += n) == std::addressof(a));
      a = coll.begin();
      auto k = a + n;
      REQUIRE(k == (a += n));
      a = coll.begin();
      REQUIRE((a + n) == (n + a));
      auto x = 1;
      auto y = 2;
      REQUIRE((a + (x + y)) == ((a + x) + y));
      REQUIRE((a + 0) == a);
      b = a + n;
      REQUIRE((--b) == (a + n - 1));
      b = a + n;
      REQUIRE((b += -n) == a);
      b = a + n;
      REQUIRE((b -= +n) == a);
      b = a + n;
      REQUIRE(std::addressof(b -= n) == std::addressof(b));
      b = a + n;
      k = b - n;
      REQUIRE(k == (b -= n));
      b = a + n;
      REQUIRE(a[n] == *b);
      REQUIRE(a <= b);
    }
    // const_iterator
    STATIC_REQUIRE(std::totally_ordered<const_iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      coll.create();
      coll.create();
      auto a = coll.cbegin();
      auto b = coll.cbegin();
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE((a == b));
      b = ++coll.cbegin();
      REQUIRE((a < b));
      REQUIRE(!(a > b));
      REQUIRE(!(a == b));
      a = ++coll.cbegin();
      b = coll.cbegin();
      REQUIRE(!(a < b));
      REQUIRE(a > b);
      REQUIRE(!(a == b));
      auto c = coll.cbegin();
      a = c++;
      b = c++;
      REQUIRE(a < b);
      REQUIRE(b < c);
      REQUIRE(a < c);
      REQUIRE((a > b) == (b < a));
      REQUIRE((a >= b) == !(a < b));
      REQUIRE((a <= b) == !(a > b));
    }
    STATIC_REQUIRE(std::sized_sentinel_for<const_iterator, const_iterator>);
    {
      auto coll = CollectionType();
      coll.create();
      auto i = coll.cbegin();
      auto s = coll.cend();
      auto n = 1;
      REQUIRE(s - i == n);
      REQUIRE(i - s == -n);
    }
    STATIC_REQUIRE(std::random_access_iterator<const_iterator>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      coll.create().cellID(43);
      coll.create().cellID(44);
      coll.create().cellID(45);
      auto a = coll.cbegin();
      auto n = 2;
      auto b = a + n;
      REQUIRE((a += n) == b);
      a = coll.cbegin();
      REQUIRE(std::addressof(a += n) == std::addressof(a));
      a = coll.cbegin();
      auto k = a + n;
      REQUIRE(k == (a += n));
      a = coll.cbegin();
      REQUIRE((a + n) == (n + a));
      auto x = 1;
      auto y = 2;
      REQUIRE((a + (x + y)) == ((a + x) + y));
      REQUIRE((a + 0) == a);
      b = a + n;
      REQUIRE((--b) == (a + n - 1));
      b = a + n;
      REQUIRE((b += -n) == a);
      b = a + n;
      REQUIRE((b -= +n) == a);
      b = a + n;
      REQUIRE(std::addressof(b -= n) == std::addressof(b));
      b = a + n;
      k = b - n;
      REQUIRE(k == (b -= n));
      b = a + n;
      REQUIRE(a[n] == *b);
      REQUIRE(a <= b);
    }
  }
}

TEST_CASE("Collection and unsupported iterator concepts", "[collection][container][iterator][std]") {
  // std::indirectly_writable
  DOCUMENTED_STATIC_FAILURE(std::indirectly_writable<iterator, CollectionType::value_type>);
  DOCUMENTED_STATIC_FAILURE(std::indirectly_writable<iterator, CollectionType::mutable_type>);
  DOCUMENTED_STATIC_FAILURE(std::indirectly_writable<const_iterator, CollectionType::value_type>);
  DOCUMENTED_STATIC_FAILURE(std::indirectly_writable<const_iterator, CollectionType::mutable_type>);
  // std::output_iterator
  DOCUMENTED_STATIC_FAILURE(std::output_iterator<iterator, CollectionType::value_type>);
  DOCUMENTED_STATIC_FAILURE(std::output_iterator<iterator, CollectionType::mutable_type>);
  DOCUMENTED_STATIC_FAILURE(std::output_iterator<const_iterator, CollectionType::value_type>);
  DOCUMENTED_STATIC_FAILURE(std::output_iterator<const_iterator, CollectionType::mutable_type>);
  // std::contiguous_iterator
  DOCUMENTED_STATIC_FAILURE(std::is_lvalue_reference_v<iterator::reference>);
  DOCUMENTED_STATIC_FAILURE(std::same_as<iterator::value_type, std::remove_cvref_t<iterator::reference>>);
  DOCUMENTED_STATIC_FAILURE(std::contiguous_iterator<iterator>);
  DOCUMENTED_STATIC_FAILURE(std::is_lvalue_reference_v<const_iterator::reference>);
  STATIC_REQUIRE(std::same_as<const_iterator::value_type, std::remove_cvref_t<const_iterator::reference>>);
  DOCUMENTED_STATIC_FAILURE(std::contiguous_iterator<const_iterator>);
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
        STATIC_REQUIRE(std::is_move_constructible_v<iterator>);
        STATIC_REQUIRE(std::is_copy_constructible_v<iterator>);
        // const_iterator
        STATIC_REQUIRE(std::is_move_constructible_v<const_iterator>);
        STATIC_REQUIRE(std::is_copy_constructible_v<const_iterator>);

        // CopyAssignable
        // iterator
        STATIC_REQUIRE(std::is_move_assignable_v<iterator>);
        STATIC_REQUIRE(std::is_copy_assignable_v<iterator>);
        // const_iterator
        STATIC_REQUIRE(std::is_move_assignable_v<const_iterator>);
        STATIC_REQUIRE(std::is_copy_assignable_v<const_iterator>);

        // Destructible
        // iterator
        STATIC_REQUIRE(std::is_nothrow_destructible_v<iterator>);
        // const_iterator
        STATIC_REQUIRE(std::is_nothrow_destructible_v<const_iterator>);

        // Swappable
        // iterator
        STATIC_REQUIRE(std::is_swappable_v<iterator&>);
        // const_iterator
        STATIC_REQUIRE(std::is_swappable_v<const_iterator&>);

        // std::iterator_traits<It>::difference_type
        // iterator
        STATIC_REQUIRE(traits::has_difference_type_v<std::iterator_traits<iterator>>);
        // const_iterator
        STATIC_REQUIRE(traits::has_difference_type_v<std::iterator_traits<const_iterator>>);

        // std::iterator_traits<It>::reference
        // iterator
        STATIC_REQUIRE(traits::has_reference_v<std::iterator_traits<iterator>>);
        // const_iterator
        STATIC_REQUIRE(traits::has_reference_v<std::iterator_traits<const_iterator>>);

        // std::iterator_traits<It>::pointer
        // iterator
        STATIC_REQUIRE(traits::has_pointer_v<std::iterator_traits<iterator>>);
        // const_iterator
        STATIC_REQUIRE(traits::has_pointer_v<std::iterator_traits<const_iterator>>);

        // std::iterator_traits<It>::iterator_category
        // iterator
        STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
        // const_iterator
        STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);

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
      STATIC_REQUIRE(traits::has_reference_v<iterator>);
      STATIC_REQUIRE(std::is_same_v<std::iterator_traits<iterator>::reference, decltype(*std::declval<iterator>())>);
      STATIC_REQUIRE(traits::has_value_type_v<iterator>);
      STATIC_REQUIRE(
          std::is_convertible_v<decltype(*std::declval<iterator>()), std::iterator_traits<iterator>::value_type>);
      // const_iterator
      STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
      STATIC_REQUIRE(traits::has_reference_v<const_iterator>);
      STATIC_REQUIRE(
          std::is_same_v<std::iterator_traits<const_iterator>::reference, decltype(*std::declval<const_iterator>())>);
      STATIC_REQUIRE(traits::has_value_type_v<const_iterator>);
      STATIC_REQUIRE(std::is_convertible_v<decltype(*std::declval<const_iterator>()),
                                           std::iterator_traits<const_iterator>::value_type>);

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
      STATIC_REQUIRE(traits::has_postincrement_v<iterator>);
      STATIC_REQUIRE(
          std::is_same_v<decltype((void)++std::declval<iterator&>()), decltype((void)std::declval<iterator&>()++)>);
      // const_iterator
      STATIC_REQUIRE(traits::has_preincrement_v<const_iterator>);
      STATIC_REQUIRE(traits::has_postincrement_v<const_iterator>);
      STATIC_REQUIRE(std::is_same_v<decltype((void)++std::declval<const_iterator&>()),
                                    decltype((void)std::declval<const_iterator&>()++)>);

      // *r++
      // iterator
      STATIC_REQUIRE(traits::has_indirection_v<iterator>);
      STATIC_REQUIRE(traits::has_value_type_v<std::iterator_traits<iterator>>);
      STATIC_REQUIRE(traits::has_postincrement_v<iterator>);
      STATIC_REQUIRE(
          std::is_convertible_v<decltype(*std::declval<iterator&>()++), std::iterator_traits<iterator>::value_type>);
      // const_iterator
      STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
      STATIC_REQUIRE(traits::has_value_type_v<std::iterator_traits<const_iterator>>);
      STATIC_REQUIRE(traits::has_postincrement_v<const_iterator>);
      STATIC_REQUIRE(std::is_convertible_v<decltype(*std::declval<const_iterator&>()++),
                                           std::iterator_traits<const_iterator>::value_type>);

      // iterator_category - not strictly necessary but advised
      // iterator
      STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
      STATIC_REQUIRE(std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
      // const_iterator
      STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
      STATIC_REQUIRE(
          std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<const_iterator>::iterator_category>);

    } // end of LegacyInputIterator

    // Mutable LegacyForwardIterator (LegacyForwardIterator that is also LegacyOutputIterator):
    // - reference same as value_type& or value_type&&
    // iterator
    STATIC_REQUIRE(traits::has_reference_v<iterator>);
    STATIC_REQUIRE(traits::has_value_type_v<iterator>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_same_v<std::iterator_traits<iterator>::reference, std::iterator_traits<iterator>::value_type&> ||
        std::is_same_v<std::iterator_traits<iterator>::reference, std::iterator_traits<iterator>::value_type&&>);
    // const_iterator
    STATIC_REQUIRE(traits::has_reference_v<const_iterator>);
    STATIC_REQUIRE(traits::has_value_type_v<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::is_same_v<std::iterator_traits<const_iterator>::reference,
                                             std::iterator_traits<const_iterator>::value_type&> ||
                              std::is_same_v<std::iterator_traits<const_iterator>::reference,
                                             std::iterator_traits<const_iterator>::value_type&&>);

    // (Immutable) iterator:
    // - reference same as const value_type& or const value_type&&
    // iterator
    STATIC_REQUIRE(traits::has_reference_v<iterator>);
    STATIC_REQUIRE(traits::has_value_type_v<iterator>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_same_v<std::iterator_traits<iterator>::reference, const std::iterator_traits<iterator>::value_type&> ||
        std::is_same_v<std::iterator_traits<iterator>::reference, const std::iterator_traits<iterator>::value_type&&>);
    // const_iterator
    STATIC_REQUIRE(traits::has_reference_v<const_iterator>);
    STATIC_REQUIRE(traits::has_value_type_v<const_iterator>);
    DOCUMENTED_STATIC_FAILURE(std::is_same_v<std::iterator_traits<const_iterator>::reference,
                                             const std::iterator_traits<const_iterator>::value_type&> ||
                              std::is_same_v<std::iterator_traits<const_iterator>::reference,
                                             const std::iterator_traits<const_iterator>::value_type&&>);

    // DefaultConstructible
    // iterator
    STATIC_REQUIRE(std::is_default_constructible_v<iterator>);
    // const_iterator
    STATIC_REQUIRE(std::is_default_constructible_v<const_iterator>);

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
      STATIC_REQUIRE(std::is_copy_constructible_v<iterator>);
      auto a_copy = a;
      ++a_copy;
      REQUIRE(a == b);
      REQUIRE(*a == *b);
      // bound to the same object
      const auto& ref_a = *a;
      const auto& ref_b = *b;
      DOCUMENTED_FAILURE(std::addressof(ref_a) == std::addressof(ref_b));

      // const_iterator
      auto ca = coll.cbegin();
      auto cb = coll.cbegin();
      REQUIRE(ca == cb);
      REQUIRE(*ca == *cb);
      REQUIRE(++ca == ++cb);
      REQUIRE(*ca == *cb);
      STATIC_REQUIRE(std::is_copy_constructible_v<const_iterator>);
      auto ca_copy = ca;
      ++ca_copy;
      REQUIRE(ca == cb);
      REQUIRE(*ca == *cb);
      // bound to the same object
      const auto& ref_ca = *ca;
      const auto& ref_cb = *cb;
      DOCUMENTED_FAILURE(std::addressof(ref_ca) == std::addressof(ref_cb));
    }

    // Singular iterators
    // iterator
    STATIC_REQUIRE(traits::has_equality_comparator_v<iterator>);
    STATIC_REQUIRE(std::is_default_constructible_v<iterator>);
    { REQUIRE(iterator{} == iterator{}); }
    // const_iterator
    STATIC_REQUIRE(traits::has_equality_comparator_v<const_iterator>);
    STATIC_REQUIRE(std::is_default_constructible_v<const_iterator>);
    { REQUIRE(const_iterator{} == const_iterator{}); }

    // i++
    // iterator
    STATIC_REQUIRE(traits::has_postincrement_v<iterator>);
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<iterator&>()++), iterator>);
    // const_iterator
    STATIC_REQUIRE(traits::has_postincrement_v<const_iterator>);
    STATIC_REQUIRE(std::is_same_v<decltype(std::declval<const_iterator&>()++), const_iterator>);

    // *i++
    // iterator
    STATIC_REQUIRE(traits::has_indirection_v<iterator>);
    STATIC_REQUIRE(traits::has_reference_v<std::iterator_traits<iterator>>);
    STATIC_REQUIRE(traits::has_postincrement_v<iterator>);
    STATIC_REQUIRE(std::is_same_v<decltype(*std::declval<iterator&>()++), std::iterator_traits<iterator>::reference>);
    // const_iterator
    STATIC_REQUIRE(traits::has_indirection_v<const_iterator>);
    STATIC_REQUIRE(traits::has_reference_v<std::iterator_traits<const_iterator>>);
    STATIC_REQUIRE(traits::has_postincrement_v<const_iterator>);
    STATIC_REQUIRE(
        std::is_same_v<decltype(*std::declval<const_iterator&>()++), std::iterator_traits<const_iterator>::reference>);

    // iterator_category - not strictly necessary but advised
    // iterator
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_base_of_v<std::forward_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
    // const_iterator
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_base_of_v<std::forward_iterator_tag, std::iterator_traits<const_iterator>::iterator_category>);

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
    STATIC_REQUIRE(traits::has_dereference_assignment_v<iterator, CollectionType::mutable_type>);
    {
      auto coll = CollectionType{};
      auto item = coll.create(13ull, 0., 0., 0., 0.);
      REQUIRE(coll.begin()->cellID() == 13ull);
      auto new_item = CollectionType::mutable_type{42ull, 0., 0., 0., 0.};
      *coll.begin() = new_item;
      DOCUMENTED_FAILURE(coll.begin()->cellID() == 42ull);
    }
    // const_iterator
    STATIC_REQUIRE(traits::has_dereference_assignment_v<const_iterator, CollectionType::value_type>);
    STATIC_REQUIRE(traits::has_dereference_assignment_v<const_iterator, CollectionType::mutable_type>);
    {
      auto coll = CollectionType{};
      auto item = coll.create(13ull, 0., 0., 0., 0.);
      REQUIRE(coll.cbegin()->cellID() == 13ull);
      auto new_item = CollectionType::mutable_type{42ull, 0., 0., 0., 0.};
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
    STATIC_REQUIRE(traits::has_postincrement_v<iterator>);
    STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<iterator&>()++), const iterator&>);
    // const_iterator
    STATIC_REQUIRE(traits::has_postincrement_v<const_iterator>);
    STATIC_REQUIRE(std::is_convertible_v<decltype(std::declval<const_iterator&>()++), const const_iterator&>);

    // *r++ = o
    // iterator
    DOCUMENTED_STATIC_FAILURE(traits::has_dereference_assignment_increment_v<iterator, CollectionType::value_type>);
    STATIC_REQUIRE(traits::has_dereference_assignment_increment_v<iterator, CollectionType::mutable_type>);
    {
      auto coll = CollectionType{};
      auto item = coll.create(13ull, 0., 0., 0., 0.);
      REQUIRE(coll.begin()->cellID() == 13ull);
      auto new_item = CollectionType::mutable_type{42ull, 0., 0., 0., 0.};
      *coll.begin()++ = new_item;
      DOCUMENTED_FAILURE(coll.begin()->cellID() == 42ull);
    }
    // const_iterator
    STATIC_REQUIRE(traits::has_dereference_assignment_increment_v<const_iterator, CollectionType::value_type>);
    STATIC_REQUIRE(traits::has_dereference_assignment_increment_v<const_iterator, CollectionType::mutable_type>);
    {
      auto coll = CollectionType{};
      auto item = coll.create(13ull, 0., 0., 0., 0.);
      REQUIRE(coll.cbegin()->cellID() == 13ull);
      auto new_item = CollectionType::mutable_type{42ull, 0., 0., 0., 0.};
      *coll.cbegin()++ = new_item;
      DOCUMENTED_FAILURE(coll.cbegin()->cellID() == 42ull);
      new_item.cellID(44ull);
      *coll.cbegin()++ = static_cast<CollectionType::value_type>(new_item);
      DOCUMENTED_FAILURE(coll.cbegin()->cellID() == 44ull);
    }

    // iterator_category - not strictly necessary but advised
    // Derived either from:
    // - std::output_iterator_tag
    // - std::forward_iterator_tag (for mutable LegacyForwardIterators)
    // iterator
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_base_of_v<std::output_iterator_tag, std::iterator_traits<iterator>::iterator_category> ||
        std::is_base_of_v<std::forward_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
    // const_iterator
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_base_of_v<std::output_iterator_tag, std::iterator_traits<iterator>::iterator_category> ||
        std::is_base_of_v<std::forward_iterator_tag, std::iterator_traits<iterator>::iterator_category>);

  } // end of LegacyOutputIterator
}

TEST_CASE("Collection and std iterator adaptors", "[collection][container][adapter][std]") {
  SECTION("Reverse iterator") {
    // iterator
    STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_base_of_v<std::bidirectional_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
    STATIC_REQUIRE(std::bidirectional_iterator<iterator>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      coll.create().cellID(43);
      auto rit = std::reverse_iterator(std::end(coll));
      DOCUMENTED_STATIC_FAILURE(
          traits::has_member_of_pointer_v<const iterator>); // can't -> because
                                                            // std::reverse_iterator<I>::operator->()
                                                            // is const
      // REQUIRE(rit->cellID() == 43);
      REQUIRE((*rit).cellID() == 43);
      ++rit;
      // REQUIRE(rit->cellID() == 42);
      REQUIRE((*rit).cellID() == 42);
      REQUIRE(rit.base()->cellID() == 43);
      rit = std::reverse_iterator(std::begin(coll));
      REQUIRE(rit.base()->cellID() == 42);
      --rit;
      // REQUIRE(rit->cellID() == 42);
      REQUIRE((*rit).cellID() == 42);
      REQUIRE(rit.base()->cellID() == 43);
      --rit;
      // REQUIRE(rit->cellID() == 43);
      REQUIRE((*rit).cellID() == 43);
    }
    // const_iterator
    STATIC_REQUIRE(traits::has_const_iterator_v<CollectionType>);
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
    DOCUMENTED_STATIC_FAILURE(
        std::is_base_of_v<std::bidirectional_iterator_tag, std::iterator_traits<const_iterator>::iterator_category>);
    STATIC_REQUIRE(std::bidirectional_iterator<const_iterator>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      coll.create().cellID(43);
      auto rit = std::reverse_iterator(std::cend(coll));
      DOCUMENTED_STATIC_FAILURE(
          traits::has_member_of_pointer_v<const const_iterator>); // can't -> because
                                                                  // std::reverse_iterator<I>::operator->()
                                                                  // is const
      // REQUIRE(rit->cellID() == 43);
      REQUIRE((*rit).cellID() == 43);
      ++rit;
      // REQUIRE(rit->cellID() == 42);
      REQUIRE((*rit).cellID() == 42);
      REQUIRE(rit.base()->cellID() == 43);
      rit = std::reverse_iterator(std::cbegin(coll));
      REQUIRE(rit.base()->cellID() == 42);
      --rit;
      // REQUIRE(rit->cellID() == 42);
      REQUIRE((*rit).cellID() == 42);
      REQUIRE(rit.base()->cellID() == 43);
      --rit;
      // REQUIRE(rit->cellID() == 43);
      REQUIRE((*rit).cellID() == 43);
    }
  }
  SECTION("Back inserter") {
    DOCUMENTED_STATIC_FAILURE(traits::has_const_reference_v<CollectionType>);
    // STATIC_REQUIRE(traits::has_push_back_v<CollectionType, CollectionType::const_reference>);

    STATIC_REQUIRE(traits::has_value_type_v<CollectionType>);
    STATIC_REQUIRE(traits::has_push_back_v<CollectionType, const CollectionType::value_type&>);
    STATIC_REQUIRE(traits::has_push_back_v<CollectionType, CollectionType::value_type&&>);

    auto coll = CollectionType();
    auto it = std::back_inserter(coll);
    // insert immutable to not-SubsetCollection
    REQUIRE_THROWS_AS(it = CollectionType::value_type{}, std::invalid_argument);
    // insert mutable (implicit cast to immutable) to not-SubsetCollection
    REQUIRE_THROWS_AS(it = CollectionType::mutable_type{}, std::invalid_argument);
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

#if (__cplusplus >= 202302L)
  SECTION("Const iterator") {
    // iterator
    STATIC_REQUIRE(std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
    STATIC_REQUIRE(std::input_iterator<iterator>);
    // make_const_iterator(iterator) gives basic_const_iterator not our iterator or our const_iterator
    STATIC_REQUIRE(std::is_same_v<std::const_iterator<iterator>, std::basic_const_iterator<iterator>>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      auto cit = std::make_const_iterator(std::begin(coll));
      REQUIRE((*cit).cellID() == 42);
      // can't -> because iterators' -> is non-const
      DOCUMENTED_STATIC_FAILURE(traits::has_member_of_pointer_v<std::const_iterator<iterator>>);
      // REQUIRE(cit->cellID() == 42)
      // REQUIRE(counted.base()->cellID() == 42);
    }
    // const_iterator
    STATIC_REQUIRE(std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<const_iterator>::iterator_category>);
    STATIC_REQUIRE(std::input_iterator<const_iterator>);
    // make_const_iterator(iterator) gives basic_const_iterator not our iterator or our const_iterator
    STATIC_REQUIRE(std::is_same_v<std::const_iterator<const_iterator>, std::basic_const_iterator<const_iterator>>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      auto cit = std::make_const_iterator(std::cbegin(coll));
      REQUIRE((*cit).cellID() == 42);
      // can't -> because const_iterators' -> is non-const
      DOCUMENTED_STATIC_FAILURE(traits::has_member_of_pointer_v<std::const_iterator<const_iterator>>);
      // REQUIRE(cit->cellID() == 42)
      // REQUIRE(counted.base()->cellID() == 42);
    }
  }
#endif

  SECTION("Move iterator") {
    // iterator
    STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<iterator>>);
    STATIC_REQUIRE(std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<iterator>::iterator_category>);
    STATIC_REQUIRE(std::input_iterator<iterator>);
    STATIC_REQUIRE(std::is_same_v<iterator::reference, std::move_iterator<iterator>::reference>);
    // const_iterator
    STATIC_REQUIRE(traits::has_iterator_v<CollectionType>);
    STATIC_REQUIRE(traits::has_iterator_category_v<std::iterator_traits<const_iterator>>);
    STATIC_REQUIRE(std::is_base_of_v<std::input_iterator_tag, std::iterator_traits<const_iterator>::iterator_category>);
    STATIC_REQUIRE(std::input_iterator<const_iterator>);
    STATIC_REQUIRE(std::is_same_v<const_iterator::reference, std::move_iterator<const_iterator>::reference>);
  }

  SECTION("Counted iterator") {
    // iterator
    STATIC_REQUIRE(std::input_or_output_iterator<iterator>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      auto counted = std::counted_iterator(coll.begin(), coll.size());
      REQUIRE(counted != std::default_sentinel);
      REQUIRE(counted.count() == 1);
      REQUIRE((*counted).cellID() == 42);
      DOCUMENTED_STATIC_FAILURE(std::contiguous_iterator<iterator>); // counted-> requires std::contiguous_iterator
      // REQUIRE(counted->cellID() == 42);
      DOCUMENTED_STATIC_FAILURE(traits::has_member_of_pointer_v<const iterator>); // can't base()->() because
                                                                                  // base() returns const object
      // REQUIRE(counted.base()->cellID() == 42);
      REQUIRE(++counted == std::default_sentinel);
    }
    // const_iterator
    STATIC_REQUIRE(std::input_or_output_iterator<const_iterator>);
    {
      auto coll = CollectionType();
      coll.create().cellID(42);
      auto counted = std::counted_iterator(coll.cbegin(), coll.size());
      REQUIRE(counted != std::default_sentinel);
      REQUIRE(counted.count() == 1);
      REQUIRE((*counted).cellID() == 42);
      DOCUMENTED_STATIC_FAILURE(std::contiguous_iterator<iterator>); // counted-> requires std::contiguous_iterator
      // REQUIRE(counted->cellID() == 42)
      DOCUMENTED_STATIC_FAILURE(traits::has_member_of_pointer_v<const const_iterator>); // can't base()->() because
                                                                                        // base() returns const object
      // REQUIRE(counted.base()->cellID() == 42);
      REQUIRE(++counted == std::default_sentinel);
    }
  }
}

TEST_CASE("Collection as range", "[collection][ranges][std]") {
  CollectionType coll;
  coll.create();

  // std::ranges::range
  STATIC_REQUIRE(std::ranges::range<CollectionType>);
  // std::range::borrowed_range
  DOCUMENTED_STATIC_FAILURE(std::ranges::borrowed_range<CollectionType>);
  // std::range::sized_range
  STATIC_REQUIRE(std::ranges::sized_range<CollectionType>);
  REQUIRE(std::ranges::size(coll) == 1);
  REQUIRE(std::ranges::size(coll) ==
          static_cast<decltype(std::ranges::size(coll))>(std::ranges::distance(std::begin(coll), std::end(coll))));
  // std::ranges::input_range
  STATIC_REQUIRE(std::ranges::input_range<CollectionType>);
  // std::range::output_range
  DOCUMENTED_STATIC_FAILURE(std::ranges::output_range<CollectionType, CollectionType::value_type>);
  DOCUMENTED_STATIC_FAILURE(std::ranges::output_range<CollectionType, CollectionType::mutable_type>);
  // std::range::forward_range
  STATIC_REQUIRE(std::ranges::forward_range<CollectionType>);
  // std::range::bidirectional_range
  STATIC_REQUIRE(std::ranges::bidirectional_range<CollectionType>);
  // std::range::random_access_range
  STATIC_REQUIRE(std::ranges::random_access_range<CollectionType>);
  // std::range::contiguous_range
  DOCUMENTED_STATIC_FAILURE(std::ranges::contiguous_range<CollectionType>);
  // std::range::common_range
  STATIC_REQUIRE(std::ranges::common_range<CollectionType>);
  // std::range::viewable_range
  STATIC_REQUIRE(std::ranges::viewable_range<CollectionType>);
}

TEST_CASE("Collection and std algorithms", "[collection][iterator][std]") {
  auto coll = CollectionType();
  coll.create().cellID(1);
  coll.create().cellID(5);
  coll.create().cellID(2);
  coll.create().cellID(2);
  coll.create().cellID(3);

  // std::find_if
  auto it = std::find_if(std::cbegin(coll), std::cend(coll), [](const auto& x) { return x.cellID() == 5; });
  REQUIRE(it != std::cend(coll));
  REQUIRE(it == ++std::cbegin(coll));
  it = std::find_if(std::cbegin(coll), std::cend(coll), [](const auto& x) { return x.cellID() == 0; });
  REQUIRE(it == std::cend(coll));

  // std::count_if
  REQUIRE(2 == std::count_if(std::cbegin(coll), std::cend(coll), [](const auto& x) { return x.cellID() > 2; }));

  // std::copy_if
  auto subcoll = CollectionType{};
  subcoll.setSubsetCollection();
  std::copy_if(std::begin(coll), std::end(coll), std::back_inserter(subcoll),
               [](const auto& x) { return x.cellID() > 2; });
  REQUIRE(subcoll.size() == 2);
  REQUIRE(subcoll[0].cellID() == 5);
  REQUIRE(subcoll[1].cellID() == 3);

  // Algorithms requiring iterator category not supported by collection iterators
  // are not checked here as their compilation and results are unspecified
}

TEST_CASE("Collection and std ranges algorithms", "[collection][ranges][std]") {
  auto coll = CollectionType();
  coll.create().cellID(1);
  coll.create().cellID(5);
  coll.create().cellID(2);
  coll.create().cellID(2);
  coll.create().cellID(3);

  // std::ranges_find_if
  auto it = std::ranges::find_if(coll, [](const auto& x) { return x.cellID() == 5; });
  REQUIRE(it != std::end(coll));
  REQUIRE(it == ++std::begin(coll));
  it = std::ranges::find_if(coll, [](const auto& x) { return x.cellID() == 0; });
  REQUIRE(it == std::end(coll));

  // std::ranges::count_if
  REQUIRE(2 == std::ranges::count_if(coll, [](const auto& x) { return x.cellID() > 2; }));

  // std::ranges_copy_if
  auto subcoll = CollectionType{};
  subcoll.setSubsetCollection();
  std::ranges::copy_if(coll, std::back_inserter(subcoll), [](const auto& x) { return x.cellID() > 2; });
  REQUIRE(subcoll.size() == 2);
  REQUIRE(subcoll[0].cellID() == 5);
  REQUIRE(subcoll[1].cellID() == 3);

  auto adjacent_it =
      std::ranges::adjacent_find(coll, [](const auto& a, const auto& b) { return a.cellID() == b.cellID(); });
  REQUIRE(adjacent_it != std::end(coll));
  auto target = std::begin(coll);
  std::ranges::advance(target, 2);
  REQUIRE(adjacent_it == target);

  auto rev_view = std::ranges::views::reverse(coll);
  REQUIRE(rev_view.front().cellID() == 3);
}

// helper concept for unsupported algorithm compilation test
template <typename T>
concept is_range_sortable =
    requires(T coll) { std::ranges::sort(coll, [](const auto& a, const auto& b) { return a.cellID() < b.cellID(); }); };

// helper concept for unsupported algorithm compilation test
template <typename T>
concept is_range_fillable = requires(T coll) { std::ranges::fill(coll, typename T::value_type{}); };

TEST_CASE("Collection and unsupported std ranges algorithms", "[collection][ranges][std]") {
  // check that algorithms requiring unsupported iterator concepts won't compile
  DOCUMENTED_STATIC_FAILURE(is_range_sortable<CollectionType>);
  DOCUMENTED_STATIC_FAILURE(is_range_fillable<CollectionType>);
}

TEST_CASE("LinkCollectionIterator and iterator concepts", "[links][ranges][std]") {
  using link_iterator = podio::LinkCollectionIteratorT<ExampleHit, ExampleHit, true>;
  using link_const_iterator = podio::LinkCollectionIteratorT<ExampleHit, ExampleHit, false>;
  using link_collection = podio::LinkCollection<ExampleHit, ExampleHit>;

  STATIC_REQUIRE(std::input_iterator<link_iterator>);
  STATIC_REQUIRE(std::input_iterator<link_const_iterator>);
  STATIC_REQUIRE(std::forward_iterator<link_iterator>);
  STATIC_REQUIRE(std::forward_iterator<link_const_iterator>);
  SECTION("bidirectional_iterator") {
    STATIC_REQUIRE(std::bidirectional_iterator<link_iterator>);
    {
      auto coll = link_collection();
      coll.create();
      auto a = ++coll.begin();
      REQUIRE(std::addressof(--a) == std::addressof(a));
      a = ++coll.begin();
      auto b = ++coll.begin();
      REQUIRE(a == b);
      REQUIRE(a-- == b);
      a = ++coll.begin();
      REQUIRE(a == b);
      a--;
      --b;
      REQUIRE(a == b);
      a = ++coll.begin();
      b = ++coll.begin();
      REQUIRE(a == b);
      REQUIRE(--(++a) == b);
      REQUIRE(++(--a) == b);
    }
    STATIC_REQUIRE(std::bidirectional_iterator<link_const_iterator>);
    {
      auto coll = link_collection();
      coll.create();
      auto a = ++coll.cbegin();
      REQUIRE(std::addressof(--a) == std::addressof(a));
      a = ++coll.cbegin();
      auto b = ++coll.cbegin();
      REQUIRE(a == b);
      REQUIRE(a-- == b);
      a = ++coll.cbegin();
      REQUIRE(a == b);
      a--;
      --b;
      REQUIRE(a == b);
      a = ++coll.cbegin();
      b = ++coll.cbegin();
      REQUIRE(a == b);
      REQUIRE(--(++a) == b);
      REQUIRE(++(--a) == b);
    }
  }
}

TEST_CASE("LinkCollection and range concepts", "[links][iterator][std]") {
  using link_collection = podio::LinkCollection<ExampleHit, ExampleHit>;

  STATIC_REQUIRE(std::ranges::input_range<link_collection>);
  STATIC_REQUIRE(std::ranges::forward_range<link_collection>);
  STATIC_REQUIRE(std::ranges::bidirectional_range<link_collection>);
  STATIC_REQUIRE(std::ranges::sized_range<link_collection>);
  STATIC_REQUIRE(std::ranges::common_range<link_collection>);
  STATIC_REQUIRE(std::ranges::viewable_range<link_collection>);
}

TEST_CASE("RelationRange as range", "[relations][ranges][std]") {
  using relation_range = podio::RelationRange<ExampleHit>;

  STATIC_REQUIRE(std::ranges::contiguous_range<relation_range>);
  STATIC_REQUIRE(std::ranges::sized_range<relation_range>);
  STATIC_REQUIRE(std::ranges::common_range<relation_range>);
  STATIC_REQUIRE(std::ranges::viewable_range<relation_range>);
  STATIC_REQUIRE(std::ranges::view<relation_range>);
  STATIC_REQUIRE(std::ranges::borrowed_range<relation_range>);
}

#undef DOCUMENTED_STATIC_FAILURE
#undef DOCUMENTED_FAILURE
