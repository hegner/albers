#ifndef PODIO_COLLECTIONBUFFERFACTORY_H
#define PODIO_COLLECTIONBUFFERFACTORY_H

#include "podio/CollectionBuffers.h"
#include "podio/SchemaEvolution.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace podio {

/// The CollectionBufferFactory allows one to create buffers of known datatypes,
/// which can then be populated by e.g. readers. In order to support schema
/// evolution, the buffers have a version and this factory will also require a
/// schema version to create buffers.
///
/// It is implemented as a singleton, which is populated at the time a shared
/// datamodel library is loaded. It is assumed that that happens early on in the
/// startup of an application, such that only a single thread will access the
/// factory instance for registering datatypes. Since the necessary creation
/// functions are part of the core datamodel library, this should be very easy
/// to achieve by simply linking to that library. Once the factory is populated
/// it can be safely accessed from multiple threads concurrently to obtain
/// buffers.
class CollectionBufferFactory {
  /// Internal storage is a map to an array of creation functions, where the
  /// version determines the place in that array. This should be a viable
  /// approach because we know the "latest and greatest" schema version
  using CreationFuncT = std::function<podio::CollectionReadBuffers(bool)>;
  using VersionMapT = std::vector<CreationFuncT>;
  using MapT = std::unordered_map<std::string, VersionMapT>;

public:
  /// The buffer factory is a singleton so we disable all copy and move
  /// constructors explicitly
  CollectionBufferFactory(CollectionBufferFactory const&) = delete;
  CollectionBufferFactory& operator=(CollectionBufferFactory const&) = delete;
  CollectionBufferFactory(CollectionBufferFactory&&) = delete;
  CollectionBufferFactory& operator=(CollectionBufferFactory&&) = delete;
  ~CollectionBufferFactory() = default;

  /// Mutable instance only used for the initial registration of functions
  /// during library loading
  static CollectionBufferFactory& mutInstance();
  /// Get the factory instance
  static CollectionBufferFactory const& instance();

  /// Create buffers for a given collection type of a given schema version.
  ///
  /// @param collType The collection type name (e.g. from collection->getTypeName())
  /// @param version The schema version the created buffers should have
  /// @param subsetColl Should the buffers be for a subset collection or not
  ///
  /// @return CollectionReadBuffers if a creation function for this collection
  /// type has been registered, otherwise an empty optional
  std::optional<podio::CollectionReadBuffers> createBuffers(const std::string& collType, SchemaVersionT version,
                                                            bool subsetColl) const;
  /// Register a creation function for a given collection type and schema version.
  ///
  /// @param collType The collection type name (i.e. what
  ///                 collection->getTypeName() returns)
  /// @param version The schema version for which this creation function is
  ///                valid
  /// @param creationFunc The function that when invoked returns buffers for
  ///                     this collection type and schema version. The
  ///                     signature has to be
  ///                     podio::CollectionReadBuffers(bool) where the boolean
  ///                     parameter steers whether the buffers are for a subset
  ///                     collection or not.
  void registerCreationFunc(const std::string& collType, SchemaVersionT version, const CreationFuncT& creationFunc);

private:
  CollectionBufferFactory() = default;

  MapT m_funcMap{}; ///< Map to the creation functions
};

} // namespace podio

#endif // PODIO_COLLECTIONBUFFERFACTORY_H
