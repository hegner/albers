#ifndef PODIO_USERDATACOLLECTION_H
#define PODIO_USERDATACOLLECTION_H

#include "podio/CollectionBase.h"
#include "podio/CollectionBuffers.h"
#include "podio/DatamodelRegistry.h"
#include "podio/SchemaEvolution.h"
#include "podio/utilities/TypeHelpers.h"

#define PODIO_ADD_USER_TYPE(type)                                                                                      \
  template <>                                                                                                          \
  consteval const char* userDataTypeName<type>() {                                                                     \
    return #type;                                                                                                      \
  }                                                                                                                    \
  template <>                                                                                                          \
  consteval const char* userDataCollTypeName<type>() {                                                                 \
    return "podio::UserDataCollection<" #type ">";                                                                     \
  }

namespace podio {

/// tuple of basic types supported in user vector
using SupportedUserDataTypes =
    std::tuple<float, double, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

/// Alias template to be used to enable template specializations only for the types listed in the
/// SupportedUserDataTypes list
/// EnableIfSupportedUserType is kept because cppyy does not seem to like
/// when UsedDataCollection is used with the concept
template <typename T>
concept SupportedUserDataType = detail::isInTuple<T, SupportedUserDataTypes>;
template <typename T>
using EnableIfSupportedUserType = std::enable_if_t<detail::isInTuple<T, SupportedUserDataTypes>>;

/// helper template to provide readable type names for basic types with macro
/// PODIO_ADD_USER_TYPE(type)
template <SupportedUserDataType BasicType>
consteval const char* userDataTypeName();

/// Helper template to provide the fully qualified name of a UserDataCollection.
/// Implementations are populated by the PODIO_ADD_USER_TYPE macro.
template <SupportedUserDataType BasicType>
consteval const char* userDataCollTypeName();

PODIO_ADD_USER_TYPE(float)
PODIO_ADD_USER_TYPE(double)

PODIO_ADD_USER_TYPE(int8_t)
PODIO_ADD_USER_TYPE(int16_t)
PODIO_ADD_USER_TYPE(int32_t)
PODIO_ADD_USER_TYPE(int64_t)
PODIO_ADD_USER_TYPE(uint8_t)
PODIO_ADD_USER_TYPE(uint16_t)
PODIO_ADD_USER_TYPE(uint32_t)
PODIO_ADD_USER_TYPE(uint64_t)

/// Collection of basic types for additional user data not defined in the EDM.
/// The data is stored in an std::vector<basic_type>. Supported are all basic
/// types supported in PODIO, i.e. float, double and 8-64 bit fixed size signed
/// and unsigned integers - @see SupportedUserDataTypes.
///
/// @author F.Gaede, DESY
/// @date Sep 2021
template <typename BasicType, typename = EnableIfSupportedUserType<BasicType>>
class UserDataCollection : public CollectionBase {

private:
  std::vector<BasicType> _vec{};
  // Pointer to the actual storage, necessary for I/O. In order to have
  // simpler move-semantics this will be set and properly initialized on
  // demand during the call to getBuffers
  std::vector<BasicType>* _vecPtr{nullptr};
  uint32_t m_collectionID{0};
  CollRefCollection m_refCollections{};
  VectorMembersInfo m_vecmem_info{};

public:
  using value_type = typename std::vector<BasicType>::value_type;
  using mutable_type = value_type;
  using const_iterator = typename std::vector<BasicType>::const_iterator;
  using iterator = typename std::vector<BasicType>::iterator;
  using difference_type = typename std::vector<BasicType>::difference_type;
  using size_type = typename std::vector<BasicType>::size_type;
  using const_reverse_iterator = typename std::vector<BasicType>::const_reverse_iterator;
  using reverse_iterator = typename std::vector<BasicType>::reverse_iterator;

  UserDataCollection() = default;
  /// Constructor from an existing vector (which will be moved from!)
  UserDataCollection(std::vector<BasicType>&& vec) : _vec(std::move(vec)) {
  }
  UserDataCollection(const UserDataCollection&) = delete;
  UserDataCollection& operator=(const UserDataCollection&) = delete;
  UserDataCollection(UserDataCollection&&) = default;
  UserDataCollection& operator=(UserDataCollection&&) = default;
  ~UserDataCollection() = default;

  /// The schema version of UserDataCollections
  static constexpr SchemaVersionT schemaVersion = 1;

  constexpr static auto typeName = userDataCollTypeName<BasicType>();
  constexpr static auto valueTypeName = userDataTypeName<BasicType>();
  constexpr static auto dataTypeName = userDataTypeName<BasicType>();

  /// prepare buffers for serialization
  void prepareForWrite() const override {
  }

  /// re-create collection from buffers after read
  void prepareAfterRead() override {
  }

  /// initialize references after read
  bool setReferences(const ICollectionProvider*) override {
    return true;
  }

  /// set collection ID
  void setID(uint32_t id) override {
    m_collectionID = id;
  }

  /// get collection ID
  uint32_t getID() const override {
    return m_collectionID;
  }

  /// Get the collection buffers for this collection
  podio::CollectionWriteBuffers getBuffers() override {
    _vecPtr = &_vec; // Set the pointer to the correct internal vector
    return {&_vecPtr, _vecPtr, &m_refCollections, &m_vecmem_info};
  }

  /// check for validity of the container after read
  bool isValid() const override {
    return true;
  }

  /// number of elements in the collection
  size_t size() const override {
    return _vec.size();
  }

  /// maximal number of elements in the collection
  size_t max_size() const override {
    return _vec.max_size();
  }

  /// Is the collection empty
  bool empty() const override {
    return _vec.empty();
  }

  /// fully qualified type name
  const std::string_view getTypeName() const override {
    return typeName;
  }

  /// fully qualified type name of elements - with namespace
  const std::string_view getValueTypeName() const override {
    return valueTypeName;
  }

  /// fully qualified type name of stored POD elements - with namespace
  const std::string_view getDataTypeName() const override {
    return dataTypeName;
  }

  /// clear the collection and all internal states
  void clear() override {
    _vec.clear();
  }

  /// check if this collection is a subset collection - no subset possible
  bool isSubsetCollection() const override {
    return false;
  }

  /// declare this collection to be a subset collection - no effect
  void setSubsetCollection(bool) override {
  }

  /// The schema version is fixed manually
  SchemaVersionT getSchemaVersion() const final {
    return schemaVersion;
  }

  /// Print this collection to the passed stream
  void print(std::ostream& os = std::cout, bool flush = true) const override {
    os << "[";
    if (!_vec.empty()) {
      os << _vec[0];
      for (size_t i = 1; i < _vec.size(); ++i) {
        os << ", " << _vec[i];
      }
    }
    os << "]";

    if (flush) {
      os.flush(); // Necessary for python
    }
  }

  size_t getDatamodelRegistryIndex() const override {
    return DatamodelRegistry::NoDefinitionNecessary;
  }

  // ----- some wrappers for std::vector and access to the complete std::vector (if really needed)

  iterator begin() {
    return _vec.begin();
  }
  iterator end() {
    return _vec.end();
  }
  const_iterator begin() const {
    return _vec.begin();
  }
  const_iterator end() const {
    return _vec.end();
  }
  const_iterator cbegin() const {
    return _vec.cbegin();
  }
  const_iterator cend() const {
    return _vec.cend();
  }
  // reverse iterators
  reverse_iterator rbegin() {
    return _vec.rbegin();
  }
  const_reverse_iterator rbegin() const {
    return _vec.rbegin();
  }
  const_reverse_iterator crbegin() const {
    return _vec.crbegin();
  }
  reverse_iterator rend() {
    return _vec.rend();
  }
  const_reverse_iterator rend() const {
    return _vec.rend();
  }
  const_reverse_iterator crend() const {
    return _vec.crend();
  }

  typename std::vector<BasicType>::reference operator[](size_t idx) {
    return _vec[idx];
  }
  typename std::vector<BasicType>::const_reference operator[](size_t idx) const {
    return _vec[idx];
  }

  void resize(size_t count) {
    _vec.resize(count);
  }
  void push_back(const BasicType& value) {
    _vec.push_back(value);
  }

  /// access to the actual data vector
  typename std::vector<BasicType>& vec() {
    return _vec;
  }

  /// const access to the actual data vector
  const typename std::vector<BasicType>& vec() const {
    return _vec;
  }
};

// don't make this macro public as it should only be used internally here...
#undef PODIO_ADD_USER_TYPE

template <SupportedUserDataType BasicType>
std::ostream& operator<<(std::ostream& o, const podio::UserDataCollection<BasicType>& coll) {
  coll.print(o);
  return o;
}

} // namespace podio

#endif
