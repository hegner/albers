# PODIO Collection as a *Container*

Comparison of the PODIO `Collection`s with a C++ named requirement [*Container*](https://en.cppreference.com/w/cpp/named_req/Container).

The PODIO `Collection`s interface was designed to mimic the standard *Container* interface, in particular `std::vector`. Perfect compliance with the *Container* is not achieved as the `Collection`s are concerned with additional semantics such as mutable/immutable element access, associations and relations, and IO which that are not part of *Container*.

On the implementation level most of the differences with respect to the *Container* comes from the fact that in order to satisfy the additional semantics a `Collection` doesn't directly store [user layer objects](design.md#the-user-layer). Instead, [data layer objects](design.md#the-internal-data-layer) are stored and user layer objects are constructed and returned when needed. Similarly, the `Collection` iterators operate on the user layer objects but don't expose `Collection`'s storage directly to the users. Instead, they construct and return user layer objects when needed.
In other words, a `Collection` utilizes the user layer type as a reference type instead of using plain references (`&` or `&&`) to stored data layer types.

As a consequence some of the **standard algorithms may not work** with PODIO `Collection` iterators. See [standard algorithm documentation](#collection-and-standard-algorithms) below.

The following tables list the compliance of a PODIO generated collection with the *Container* named requirement, stating which member types, interfaces, or concepts are fulfilled and which are not. Additionally, there are some comments explaining missing parts or pointing out differences in behaviour.

### Container Types

| Name | Type | Requirements | Fulfilled by Collection? | Comment |
|------|------|--------------|--------------------------|---------|
| `value_type` | `T` | *[Erasable](https://en.cppreference.com/w/cpp/named_req/Erasable)* | ✔️ yes | Defined as an immutable user layer object type |
| `reference` | `T&` | | ❌ no | Not defined |
| `const_reference` | `const T&` | | ❌ no | Not defined |
| `iterator` | Iterator whose `value_type` is `T` | [*LegacyForwardIterator*](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) convertible to `const_iterator` | ❌ no | Defined as podio `MutableCollectionIterator`. Not [*LegacyForwardIterator*](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) ([see below](#legacyforwarditerator)), not convertible to `const_iterator`|
| `const_iterator` | Constant iterator whose `value_type` is `T` | [*LegacyForwardIterator*](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) | ❌ no | Defined as podio `CollectionIterator`. Not [*LegacyForwardIterator*](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) ([see below](#legacyforwarditerator))
| `difference_type`| Signed integer | Must be the same as `std::iterator_traits::difference_type` for `iterator` and `const_iterator` | ✔️ yes | |
| `size_type` | Unsigned integer | Large enough to represent all positive values of `difference_type` | ✔️ yes | |

### Container member functions and operators

| Expression | Return type | Semantics | Fulfilled by Collection? | Comment |
|------------|-------------|-----------|--------------------------|---------|
| `C()` | `C` | Creates an empty container | ✔️ yes | |
| `C(a)` | `C` | Creates a copy of `a` | ❌ no | Not defined, non-copyable by design |
| `C(rv)` | `C` | Moves `rv` | ✔️ yes | |
| `a = b` | `C&` | Destroys or copy-assigns all elements of `a` from elements of `b` | ❌ no | Not defined, non-copyable by design |
| `a = rv` | `C&` | Destroys or move-assigns all elements of `a` from elements of `rv` | ✔️ yes | |
| `a.~C()` | `void` | Destroys all elements of `a` and frees all memory| ✔️ yes | Invalidates all handles retrieved from this collection |
| `a.begin()` | `(const_)iterator` | Iterator to the first element of `a` | ✔️ yes | |
| `a.end()` | `(const_)iterator` | Iterator to one past the last element of `a` | ✔️ yes | |
| `a.cbegin()` | `const_iterator` | Same as `const_cast<const C&>(a).begin()` | ✔️ yes | |
| `a.cend()` | `const_iterator` | Same as `const_cast<const C&>(a).end()`| ✔️ yes | |
| `a == b` | Convertible to `bool` | Same as `std::equal(a.begin(), a.end(), b.begin(), b.end())`| ❌ no | Not defined |
| `a != b` | Convertible to `bool` | Same as `!(a == b)` | ❌ no | Not defined |
| `a.swap(b)` | `void` | Exchanges the values of `a` and `b` | ❌ no | Not defined |
| `swap(a,b)` | `void` | Same as `a.swap(b)` | ❌ no | `a.swap(b)` not defined |
| `a.size()` | `size_type` | Same as `std::distance(a.begin(), a.end())` | ✔️ yes | |
| `a.max_size()` | `size_type` | `b.size()` where `b` is the largest possible container | ✔️ yes | |
| `a.empty()` | Convertible to `bool` | Same as `a.begin() == a.end()` | ✔️ yes | |

## Collection as an *AllocatorAwareContainer*

The C++ standard specifies [AllocatorAwareContainer](https://en.cppreference.com/w/cpp/named_req/AllocatorAwareContainer) for containers that can use other allocators beside the default allocator.

PODIO collections don't provide a customization point for allocators and use only the default allocator. Therefore they are not *AllocatorAwareContainers*.

### AllocatorAwareContainer types

| Name | Requirements | Fulfilled by Collection? | Comment |
|------|--------------|--------------------------|---------|
| `allocator_type` | `allocator_type::value_type` same as `value_type` | ❌ no | `allocator_type` not defined |

### *AllocatorAwareContainer* expression and statements

The PODIO Collections currently are not checked against expression and statements requirements for *AllocatorAwareContainer*.

## Collection iterators as an *Iterator*

The C++ specifies a set of named requirements for iterators. Starting with C++20 the standard specifies also iterator concepts. The requirements imposed by the concepts and named requirements are similar but not identical.

In the following tables a convention from `Collection` is used: `iterator` stands for PODIO `MutableCollectionIterator` and `const_iterator` stands for PODIO `CollectionIterator`.

### Iterator summary

| Named requirement | `iterator` | `const_iterator` |
|-------------------|-----------------------|-----------------------------|
| [LegacyIterator](https://en.cppreference.com/w/cpp/named_req/Iterator) | ✔️ yes ([see below](#legacyiterator)) | ✔️ yes ([see below](#legacyiterator)) |
| [LegacyInputIterator](https://en.cppreference.com/w/cpp/named_req/InputIterator) | ✔️ yes ([see below](#legacyinputiterator)) | ✔️ yes ([see below](#legacyinputiterator)) |
| [LegacyForwardIterator](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) | ❌ no ([see below](#legacyforwarditerator)) | ❌ no ([see below](#legacyforwarditerator)) |
| [LegacyOutputIterator](https://en.cppreference.com/w/cpp/named_req/OutputIterator) | ❌ no ([see below](#legacyoutputiterator)) | ❌ no ([see below](#legacyoutputiterator)) |
| [LegacyBidirectionalIterator](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator) | ❌ no | ❌ no |
| [LegacyRandomAccessIterator](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator) | ❌ no | ❌ no |
| [LegacyContiguousIterator](https://en.cppreference.com/w/cpp/named_req/ContiguousIterator) | ❌ no | ❌ no |

| Concept | `iterator` | `const_iterator` |
|---------|------------------------|------------------------------|
| `std::indirectly_readable` | ✔️ yes | ✔️ yes |
| `std::indirectly_writable` | ❌ no | ❌ no |
| `std::weakly_incrementable` | ✔️ yes | ✔️ yes |
| `std::incrementable` | ✔️ yes | ✔️ yes |
| `std::input_or_output_iterator` | ✔️ yes | ✔️ yes |
| `std::input_iterator` | ✔️ yes | ✔️ yes |
| `std::output_iterator` | ❌ no | ❌ no |
| `std::forward_iterator` | ✔️ yes (see note below) | ✔️ yes (see note below) |
| `std::bidirectional_iterator` | ✔️ yes (see note below) | ✔️ yes (see note below) |
| `std::random_access_iterator` | ✔️ yes (see note below) | ✔️ yes (see note below) |
| `std::contiguous_iterator` | ❌ no | ❌ no |

:::{note}
The collections' iterators fulfil the `std::forward_iterator` except that the pointers obtained with `->` remain valid only as long as the iterator itself is valid instead of as long as the range remains valid. In practice, this means a `ptr` obtained with `auto* ptr = it.operator->();` is valid only as long as `it` is valid.
The values obtained immediately through `->` (for instance, `auto& e = it->energy();`) behave as expected for `std::forward_iterator`, as their validity is tied to the collection's validity.
Despite the exception, the collection iterators are made to still report themselves as fulfilling the forward iterator requirements since the problematic usage (`auto* ptr = it.operator->();`) is rare in normal scenarios.
:::

### LegacyIterator

| Requirement | Fulfilled by `iterator`/`const_iterator`? | Comment |
|-------------|-------------------------------------------|---------|
| [*CopyConstructible*](https://en.cppreference.com/w/cpp/named_req/CopyConstructible) | ✔️ yes / ✔️ yes | |
| [*CopyAssignable*](https://en.cppreference.com/w/cpp/named_req/CopyAssignable) | ✔️ yes / ✔️ yes | |
| [*Destructible*](https://en.cppreference.com/w/cpp/named_req/Destructible) | ✔️ yes / ✔️ yes | |
| [*Swappable*](https://en.cppreference.com/w/cpp/named_req/Swappable) | ✔️ yes / ✔️ yes | |
| `std::iterator_traits::difference_type` | ✔️ yes / ✔️ yes | |
| `std::iterator_traits::reference` | ✔️ yes / ✔️ yes | |
| `std::iterator_traits::pointer` | ✔️ yes / ✔️ yes | |
| `std::iterator_traits::iterator_category` | ✔️ yes / ✔️ yes | |

| Expression | Return type | Semantics | Fulfilled by `iterator`/`const_iterator`? | Comment |
|------------|-------------|-----------|-------------------------------------------|---------|
| `*r` | Unspecified | | ✔️ yes / ✔️ yes | |
| `++r` | `It&` | | ✔️ yes / ✔️ yes | |

### LegacyInputIterator

| Requirement | Fulfilled by `iterator`/`const_iterator`? | Comment |
|-------------|-------------------------------------------|---------|
| [*LegacyIterator*](https://en.cppreference.com/w/cpp/named_req/Iterator) | ✔️ yes / ✔️ yes | [See above](#legacyiterator) |
| [*EqualityComparable*](https://en.cppreference.com/w/cpp/named_req/EqualityComparable) | ✔️ yes / ✔️ yes | |

| Expression | Return type | Semantics | Fulfilled by `iterator`/`const_iterator`? | Comment |
|------------|-------------|-----------|-------------------------------------------|---------|
| `i != j` | Contextually convertible to `bool` | Same as `!(i==j)` | ✔️ yes / ✔️ yes | |
| `*i` | `reference`, convertible to `value_type` | | ✔️ yes / ✔️ yes | |
| `i->m` | | Same as `(*i).m` | ✔️ yes / ✔️ yes | |
| `++r` | `It&` | | ✔️ yes / ✔️ yes | |
| `(void)r++` | | Same as `(void)++r` | ✔️ yes / ✔️ yes | |
| `*r++` | Convertible to `value_type` | Same as `value_type x = *r; ++r; return x;` | ✔️ yes / ✔️ yes | |

### LegacyForwardIterator

In addition to the *LegacyForwardIterator* the C++ standard specifies also the *mutable LegacyForwardIterator*, which is both *LegacyForwardIterator* and *LegacyOutputIterator*. The term **mutable** used in this context doesn't imply mutability in the sense used in the PODIO.

| Requirement | Fulfilled by `iterator`/`const_iterator`? | Comment |
|-------------|-------------------------------------------|---------|
| [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator) | ✔️ yes / ✔️ yes | [See above](#legacyinputiterator)|
| [*DefaultConstructible*](https://en.cppreference.com/w/cpp/named_req/DefaultConstructible) | ✔️ yes / ✔️ yes | |
| If *mutable* iterator then `reference` same as `value_type&` or `value_type&&`, otherwise same as `const value_type&` or `const value_type&&` | ❌ no / ❌ no | `reference` type is not a reference (`&` or `&&`) |
| [Multipass guarantee](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) | ❌ no / ❌ no | References from dereferencing equal iterators aren't bound to the same object |
| [Singular iterators](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) | ✔️ yes / ✔️ yes | |

| Expression | Return type | Semantics | Fulfilled by `iterator`/`const_iterator`? | Comment |
|------------|-------------|-----------|-------------------------------------------|---------|
| `i++` | `It` | Same as `It ip = i; ++i; return ip;` | ✔️ yes / ✔️ yes | |
| `*i++` | `reference` | | ✔️ yes / ✔️ yes | |

### LegacyOutputIterator

| Requirement | Fulfilled by `iterator`/`const_iterator`? | Comment |
|-------------|-------------------------------------------|---------|
| [*LegacyIterator*](https://en.cppreference.com/w/cpp/named_req/Iterator) | ✔️ yes / ✔️ yes | [See above](#legacyiterator) |
| Is pointer type or class type | ✔️ yes / ✔️ yes | |

| Expression | Return type | Semantics | Fulfilled by `iterator`/`const_iterator`? | Comment |
|------------|-------------|-----------|-------------------------------------------|---------|
| `*r = o` | | | ❗ attention / ❗ attention | Defined but an assignment doesn't modify objects inside collection |
| `++r` | `It&` | | ✔️ yes / ✔️ yes | |
| `r++` | Convertible to `const It&` | Same as `It temp = r; ++r; return temp;` | ✔️ yes / ✔️ yes | |
| `*r++ = o` | | Same as `*r = o; ++r;`| ❗ attention / ❗ attention | Defined but an assignment doesn't modify objects inside collection |

## Collection iterators and standard iterator adaptors

| Adaptor | Compatible with Collection? | Comment |
|---------|-----------------------------|---------|
| `std::reverse_iterator` | ❗ attention | `operator->` not defined as `iterator`'s and `const_iterator`'s `operator->` are non-`const` |
| `std::back_insert_iterator` | ❗ attention | Compatible only with SubsetCollections, otherwise throws `std::invalid_argument` |
| `std::front_insert_iterator` | ❌ no | `push_front` not defined |
| `std::insert_iterator` | ❌ no | `insert` not defined |
| `std::const_iterator` (C++23) | ❗ attention | `operator->` not defined as `iterator`'s and `const_iterator`'s `operator->` are non-`const`. |
| `std::move_iterator` | ✔️ yes | Limited usefulness since dereference returns `reference` type not rvalue reference (`&&`) |
| `std::counted_iterator` | ❗ attention | `operator->` not defined as it requires `std::contiguous_iterator` |


## Collection as a *range*

| Concept | Fulfilled by Collection? |
|---------|--------------------------|
| `std::ranges::range` | ✔️ yes |
| `std::ranges::borrowed_range` | ❌ no |
| `std::ranges::sized_range` | ✔️ yes |
| `std::ranges::input_range` | ✔️ yes |
| `std::ranges::output_range` | ❌ no |
| `std::ranges::forward_range` | ✔️ yes |
| `std::ranges::bidirectional_range` | ✔️ yes |
| `std::ranges::random_access_range` | ✔️ yes |
| `std::ranges::contiguous_range` | ❌ no |
| `std::ranges::common_range` | ✔️ yes |
| `std::ranges::viewable_range` | ✔️ yes |

## Collection and standard algorithms

Most of the standard algorithms require iterators to meet specific named requirements, such as [*LegacyInputIterator*](https://en.cppreference.com/w/cpp/named_req/InputIterator), [*LegacyForwardIterator*](https://en.cppreference.com/w/cpp/named_req/ForwardIterator), or [*LegacyRandomAccessIterator*](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator). These requirements are not always strictly enforced at compile time, making it essential to understand the allowed iterator category when using standard algorithms.

The iterators of PODIO collections conform to the [**LegacyInputIterator**](https://en.cppreference.com/w/cpp/named_req/InputIterator) named requirement, therefore are guaranteed to work with any algorithm requiring [**LegacyIterator**](https://en.cppreference.com/w/cpp/named_req/Iterator) or [**LegacyInputIterator**](https://en.cppreference.com/w/cpp/named_req/InputIterator).

Algorithms requiring a different iterator category may compile but do not guarantee correct results. An important case are mutating algorithms requiring the iterators to be [*writable*](https://en.cppreference.com/w/cpp/iterator), or [*LegacyOutputIterator*](https://en.cppreference.com/w/cpp/named_req/OutputIterator) or *mutable*, which are not compatible and will produce incorrect results.

For example:
```c++
std::find_if(std::begin(collection), std::end(collection), predicate ); // requires InputIterator -> OK
std::adjacent_find(std::begin(collection), std::end(collection), predicate ); // requires ForwardIterator -> might compile, unspecified result
std::fill(std::begin(collection), std::end(collection), value ); // requires ForwardIterator and writable -> might compile, wrong result
std::sort(std::begin(collection), std::end(collection)); // requires RandomAccessIterator and Swappable -> might compile, wrong result
```

## Collection and standard range algorithms

The arguments of standard range algorithms are checked at compile time and must fulfil certain iterator concepts, such as `std::input_iterator` or `std::ranges::input_range`.

The iterators of PODIO collections model the `std::random_access_iterator` concept, so range algorithms that require this iterator type will work correctly with PODIO iterators. If an algorithm compiles, it is guaranteed to work as expected.

In particular, the PODIO collections' iterators do not fulfil the `std::output_iterator` concept, and as a result, mutating algorithms relying on this iterator type will not compile.

Similarly the collections themselves model the `std::random_access_range` concept and can be used in the range algorithms that require that concept. The algorithms requiring unsupported range concept, such as `std::output_range`, won't compile.

For example:
```c++
std::ranges::find_if(collection, predicate ); // requires input_range -> OK
std::ranges::adjacent_find(collection, predicate ); // requires forward_range -> OK
std::ranges::views::reverse(collection); //requires bidirectional_range -> OK
std::ranges::fill(collection, value ); // requires output_range -> won't compile
std::ranges::sort(collection); // requires random_access_range and sortable -> won't compile
```
