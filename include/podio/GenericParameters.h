// -*- C++ -*-
#ifndef PODIO_GENERICPARAMETERS_H
#define PODIO_GENERICPARAMETERS_H 1

#include "podio/utilities/TypeHelpers.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#if PODIO_ENABLE_SIO
namespace sio {
class read_device;
class write_device;
using version_type = uint32_t; // from sio/definitions
} // namespace sio
#endif

namespace podio {

/// The types which are supported in the GenericParameters
using SupportedGenericDataTypes = std::tuple<int, float, std::string, double>;

/// Static bool for determining if a type T is a supported GenericParameters type
template <typename T>
static constexpr bool isSupportedGenericDataType = detail::isAnyOrVectorOf<T, SupportedGenericDataTypes>;

/// Alias template to be used for enabling / disabling template overloads that
/// should only be present for actually supported data types
template <typename T>
using EnableIfValidGenericDataType = typename std::enable_if_t<isSupportedGenericDataType<T>>;

/// GenericParameters objects allow one to store generic named parameters of type
///  int, float and string or vectors of these types.
///  They can be used  to store (user) meta data that is
///  run, event or collection dependent.
///  (based on lcio::LCParameters)
///
/// @author F. Gaede, DESY
/// @date Apr 2020
class GenericParameters {
public:
  template <typename T>
  using MapType = std::map<std::string, std::vector<T>>;

private:
  // need mutex pointers for having the possibility to copy/move GenericParameters
  using MutexPtr = std::unique_ptr<std::mutex>;

public:
  GenericParameters();

  /// GenericParameters are copyable
  /// @note This is currently mainly done to keep the ROOT I/O happy, because
  /// that needs a copy constructor
  GenericParameters(const GenericParameters&);
  GenericParameters& operator=(const GenericParameters&) = delete;

  /// GenericParameters are default moveable
  GenericParameters(GenericParameters&&) = default;
  GenericParameters& operator=(GenericParameters&&) = default;

  template <typename T>
  std::optional<T> get(const std::string& key) const;

  /// Store (a copy of) the passed value under the given key
  template <typename T>
  void set(const std::string& key, T value);

  /// Overload for catching const char* setting for string values
  void set(const std::string& key, const char* value) {
    set<std::string>(key, std::string(value));
  }

  /// Overload for catching initializer list setting of string vector values
  void set(const std::string& key, std::vector<std::string> values) {
    set<std::vector<std::string>>(key, std::move(values));
  }

  /// Overload for catching initializer list setting for vector values
  template <typename T, typename = std::enable_if_t<detail::isInTuple<T, SupportedGenericDataTypes>>>
  void set(const std::string& key, std::initializer_list<T>&& values) {
    set<std::vector<T>>(key, std::move(values));
  }

  /// Load multiple key value pairs simultaneously
  template <typename T, template <typename...> typename VecLike>
  void loadFrom(VecLike<std::string> keys, VecLike<std::vector<T>> values);

  /// Get the number of elements stored under the given key for a type
  template <typename T, typename = EnableIfValidGenericDataType<T>>
  size_t getN(const std::string& key) const;

  /// Get all available keys for a given type
  template <typename T>
  std::vector<std::string> getKeys() const;

  /// Get all the available values for a given type
  template <typename T>
  std::tuple<std::vector<std::string>, std::vector<std::vector<T>>> getKeysAndValues() const;

  /// erase all elements
  void clear() {
    _intMap.clear();
    _floatMap.clear();
    _doubleMap.clear();
    _stringMap.clear();
  }

  void print(std::ostream& os = std::cout, bool flush = true) const;

  /// Check if no parameter is stored (i.e. if all internal maps are empty)
  bool empty() const {
    return _intMap.empty() && _floatMap.empty() && _doubleMap.empty() && _stringMap.empty();
  }

#if PODIO_ENABLE_SIO
  friend void writeGenericParameters(sio::write_device& device, const GenericParameters& parameters);
  friend void readGenericParameters(sio::read_device& device, GenericParameters& parameters, sio::version_type version);
#endif

  /// Get a reference to the internal map for a given type
  template <typename T>
  const MapType<detail::GetVectorType<T>>& getMap() const {
    if constexpr (std::is_same_v<detail::GetVectorType<T>, int>) {
      return _intMap;
    } else if constexpr (std::is_same_v<detail::GetVectorType<T>, float>) {
      return _floatMap;
    } else if constexpr (std::is_same_v<detail::GetVectorType<T>, double>) {
      return _doubleMap;
    } else {
      return _stringMap;
    }
  }

private:
  /// Get a reference to the internal map for a given type
  template <typename T>
  MapType<detail::GetVectorType<T>>& getMap() {
    if constexpr (std::is_same_v<detail::GetVectorType<T>, int>) {
      return _intMap;
    } else if constexpr (std::is_same_v<detail::GetVectorType<T>, float>) {
      return _floatMap;
    } else if constexpr (std::is_same_v<detail::GetVectorType<T>, double>) {
      return _doubleMap;
    } else {
      return _stringMap;
    }
  }

  /// Get the mutex that guards the map for the given type
  template <typename T>
  std::mutex& getMutex() const {
    if constexpr (std::is_same_v<detail::GetVectorType<T>, int>) {
      return *(m_intMtx.get());
    } else if constexpr (std::is_same_v<detail::GetVectorType<T>, float>) {
      return *(m_floatMtx.get());
    } else if constexpr (std::is_same_v<detail::GetVectorType<T>, double>) {
      return *(m_doubleMtx.get());
    } else {
      return *(m_stringMtx.get());
    }
  }

private:
  MapType<int> _intMap{};                ///< The map storing the integer values
  mutable MutexPtr m_intMtx{nullptr};    ///< The mutex guarding the integer map
  MapType<float> _floatMap{};            ///< The map storing the float values
  mutable MutexPtr m_floatMtx{nullptr};  ///< The mutex guarding the float map
  MapType<std::string> _stringMap{};     ///< The map storing the string values
  mutable MutexPtr m_stringMtx{nullptr}; ///< The mutex guarding the string map
  MapType<double> _doubleMap{};          ///< The map storing the double values
  mutable MutexPtr m_doubleMtx{nullptr}; ///< The mutex guarding the double map
};

template <typename T>
std::optional<T> GenericParameters::get(const std::string& key) const {
  static_assert(podio::isSupportedGenericDataType<T>, "Unsupported parameter type");
  const auto& map = getMap<T>();
  auto& mtx = getMutex<T>();
  std::lock_guard lock{mtx};
  const auto it = map.find(key);
  if (it == map.end()) {
    return std::nullopt;
  }

  // We have to check whether the return type is a vector or a single value
  if constexpr (detail::isVector<T>) {
    return it->second;
  } else {
    const auto& iv = it->second;
    if (iv.empty()) {
      return std::nullopt;
    }
    return iv[0];
  }
}

template <typename T>
void GenericParameters::set(const std::string& key, T value) {
  static_assert(podio::isSupportedGenericDataType<T>, "Unsupported parameter type");
  auto& map = getMap<T>();
  auto& mtx = getMutex<T>();

  if constexpr (detail::isVector<T>) {
    std::lock_guard lock{mtx};
    map.insert_or_assign(key, std::move(value));
  } else {
    // Wrap the value into a vector with exactly one entry and store that
    std::vector<T> v = {std::move(value)};
    std::lock_guard lock{mtx};
    map.insert_or_assign(key, std::move(v));
  }
}

template <typename T, typename>
size_t GenericParameters::getN(const std::string& key) const {
  const auto& map = getMap<T>();
  auto& mtx = getMutex<T>();
  std::lock_guard lock{mtx};
  if (const auto it = map.find(key); it != map.end()) {
    return it->second.size();
  }
  return 0;
}

template <typename T>
std::vector<std::string> GenericParameters::getKeys() const {
  static_assert(podio::isSupportedGenericDataType<T>, "Unsupported parameter type");
  std::vector<std::string> keys;
  const auto& map = getMap<T>();
  keys.reserve(map.size());
  {
    auto& mtx = getMutex<T>();
    std::lock_guard lock{mtx};
    std::transform(map.begin(), map.end(), std::back_inserter(keys), [](const auto& pair) { return pair.first; });
  }

  return keys;
}

template <typename T>
std::tuple<std::vector<std::string>, std::vector<std::vector<T>>> GenericParameters::getKeysAndValues() const {
  static_assert(podio::isSupportedGenericDataType<T>, "Unsupported parameter type");
  std::vector<std::vector<T>> values;
  std::vector<std::string> keys;
  auto& mtx = getMutex<T>();
  const auto& map = getMap<T>();
  {
    // Lock to avoid concurrent changes to the map while we get the stored
    // values
    std::lock_guard lock{mtx};
    values.reserve(map.size());
    keys.reserve(map.size());

    for (const auto& [k, v] : map) {
      keys.emplace_back(k);
      values.emplace_back(v);
    }
  }
  return {keys, values};
}

template <typename T, template <typename...> typename VecLike>
void GenericParameters::loadFrom(VecLike<std::string> keys, VecLike<std::vector<T>> values) {
  auto& map = getMap<T>();
  auto& mtx = getMutex<T>();

  std::lock_guard lock{mtx};
  for (size_t i = 0; i < keys.size(); ++i) {
    map.emplace(std::move(keys[i]), std::move(values[i]));
  }
}

} // namespace podio
#endif
