#ifndef PODIO_FRAME_H
#define PODIO_FRAME_H

#include "podio/CollectionBase.h"
#include "podio/CollectionBufferFactory.h"
#include "podio/CollectionIDTable.h"
#include "podio/FrameCategories.h" // mainly for convenience
#include "podio/GenericParameters.h"
#include "podio/ICollectionProvider.h"
#include "podio/SchemaEvolution.h"
#include "podio/utilities/TypeHelpers.h"

#include <initializer_list>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace podio {

/// Alias template for enabling overloads only for Collections
template <typename T>
using EnableIfCollection = typename std::enable_if_t<isCollection<T>>;

/// Alias template for enabling overloads only for Collection r-values
template <typename T>
using EnableIfCollectionRValue = typename std::enable_if_t<isCollection<T> && !std::is_lvalue_reference_v<T>>;

/// Alias template for enabling overloads for r-values
template <typename T>
using EnableIfRValue = typename std::enable_if_t<!std ::is_lvalue_reference_v<T>>;

namespace detail {
  /// The minimal interface for raw data types
  struct EmptyFrameData {
    podio::CollectionIDTable getIDTable() const {
      return {};
    }

    /// Try to get the buffers for a collection
    std::optional<podio::CollectionReadBuffers> getCollectionBuffers(const std::string&) {
      return std::nullopt;
    }

    /// Get the **still available**, i.e. yet unpacked, collections from the raw data
    std::vector<std::string> getAvailableCollections() const {
      return {};
    }

    /// Get the parameters that are stored in the raw data
    std::unique_ptr<podio::GenericParameters> getParameters() {
      return std::make_unique<podio::GenericParameters>();
    }
  };
} // namespace detail

template <typename FrameDataT>
std::optional<podio::CollectionReadBuffers> unpack(FrameDataT* data, const std::string& name) {
  return data->getCollectionBuffers(name);
}

/// The Frame is a generalized (event) data container that aggregates all
/// relevant data.
///
/// It is possible to store collections as well as parameters / meta data in a
/// Frame and all I/O facilities of podio operate on Frames.
class Frame {
  /// Internal abstract interface for the type-erased implementation of the
  /// Frame class
  struct FrameConcept {
    virtual ~FrameConcept() = default;
    virtual const podio::CollectionBase* get(const std::string& name) const = 0;
    virtual const podio::CollectionBase* put(std::unique_ptr<podio::CollectionBase> coll, const std::string& name) = 0;
    virtual podio::GenericParameters& parameters() = 0;
    virtual const podio::GenericParameters& parameters() const = 0;

    virtual std::vector<std::string> availableCollections() const = 0;

    // Writing interface. Need this to be able to store all necessary information
    // TODO: Figure out whether this can be "hidden" somehow
    virtual podio::CollectionIDTable getIDTable() const = 0;
  };

  /// The interface implementation of the abstract FrameConcept that is
  /// necessary for a type-erased implementation of the Frame class
  template <typename FrameDataT>
  struct FrameModel final : FrameConcept, public ICollectionProvider {

    FrameModel(std::unique_ptr<FrameDataT> data);
    FrameModel(const FrameModel&) = delete;
    FrameModel& operator=(const FrameModel&) = delete;
    FrameModel(FrameModel&&) = default;
    FrameModel& operator=(FrameModel&&) = default;

    /// Try and get the collection from the internal storage and return a
    /// pointer to it if found. Otherwise return a nullptr
    const podio::CollectionBase* get(const std::string& name) const final;

    /// Try and place the collection into the internal storage and return a
    /// pointer to it. If a collection already exists or insertion fails, return
    /// a nullptr
    const podio::CollectionBase* put(std::unique_ptr<CollectionBase> coll, const std::string& name) final;

    /// Get a reference to the internally used GenericParameters
    podio::GenericParameters& parameters() override {
      return *m_parameters;
    }
    /// Get a const reference to the internally used GenericParameters
    const podio::GenericParameters& parameters() const override {
      return *m_parameters;
    }

    bool get(uint32_t collectionID, podio::CollectionBase*& collection) const override;

    podio::CollectionIDTable getIDTable() const override {
      // Make a copy
      return {m_idTable.ids(), m_idTable.names()};
    }

    std::vector<std::string> availableCollections() const override;

  private:
    podio::CollectionBase* doGet(const std::string& name, bool setReferences = true) const;

    using CollectionMapT = std::unordered_map<std::string, std::unique_ptr<podio::CollectionBase>>;

    mutable CollectionMapT m_collections{};                 ///< The internal map for storing unpacked collections
    mutable std::unique_ptr<std::mutex> m_mapMtx{nullptr};  ///< The mutex for guarding the internal collection map
    std::unique_ptr<FrameDataT> m_data{nullptr};            ///< The raw data read from file
    mutable std::unique_ptr<std::mutex> m_dataMtx{nullptr}; ///< The mutex for guarding the raw data
    podio::CollectionIDTable m_idTable{};                   ///< The collection ID table
    std::unique_ptr<podio::GenericParameters> m_parameters{nullptr}; ///< The generic parameter store for this frame
    mutable std::set<uint32_t> m_retrievedIDs{}; ///< The IDs of the collections that we have already read (but not yet
                                                 ///< put into the map)
  };

  std::unique_ptr<FrameConcept> m_self; ///< The internal concept pointer through which all the work is done

public:
  /// Empty Frame constructor
  Frame();

  /// Frame constructor from (almost) arbitrary raw data.
  ///
  /// @tparam FrameDataT Arbitrary data container that provides access to the
  ///                    collection buffers as well as the metadata, when
  ///                    requested by the Frame. The unique_ptr has to be checked
  ///                    for validity before calling this constructor.
  ///
  /// @throws std::invalid_argument if the passed pointer is a nullptr.
  template <typename FrameDataT>
  Frame(std::unique_ptr<FrameDataT>);

  /// Frame constructor from (almost) arbitrary raw data.
  ///
  /// This r-value overload is mainly present for enabling the python bindings,
  /// where cppyy seems to strip the std::unique_ptr somewhere in the process
  ///
  /// @tparam FrameDataT Arbitrary data container that provides access to the
  ///                    collection buffers as well as the metadata, when
  ///                    requested by the Frame.
  template <typename FrameDataT, typename = EnableIfRValue<FrameDataT>>
  Frame(FrameDataT&&);

  /// A Frame is move-only
  Frame(const Frame&) = delete;
  /// A Frame is move-only
  Frame& operator=(const Frame&) = delete;

  /// Frame move constructor
  Frame(Frame&&) = default;

  /// Frame move assignment operator
  Frame& operator=(Frame&&) = default;

  /// Get a collection from the Frame by name.
  ///
  /// @tparam CollT The type of the desired collection
  /// @param  name  The name of the collection
  ///
  /// @returns      A const reference to the collection if it is available or to
  ///               an empty (static) collection
  template <typename CollT, typename = EnableIfCollection<CollT>>
  const CollT& get(const std::string& name) const;

  /// Get a collection pointer from the Frame by name.
  ///
  /// This is a type-erased version that is also used by the python bindings.
  ///
  /// @returns A const pointer to a collection if it is available or a nullptr
  ///          if it is not
  const podio::CollectionBase* get(const std::string& name) const;

  /// (Destructively) move a collection into the Frame and get a reference to
  /// the inserted collection back for further use.
  ///
  /// The collection that is passed into the Frame has to be moved into it
  /// explicitly and the moved-from collection will be in the typical *valid but
  /// undefined state* in c++.
  ///
  /// @tparam CollT The type of the collection
  /// @param  coll  An rvalue reference to the collection to put into the Frame.
  /// @param  name  The name under which this collection should be stored in the
  ///               Frame
  ///
  /// @returns      A const reference to the collection that has just been
  ///               inserted
  template <typename CollT, typename = EnableIfCollectionRValue<CollT>>
  const CollT& put(CollT&& coll, const std::string& name);

  /// (Destructively) move a collection into the Frame.
  ///
  /// @param coll The collection that should be moved into the Frame
  /// @param name The name under which this collection should be stored in the
  ///             Frame
  void put(std::unique_ptr<podio::CollectionBase> coll, const std::string& name);

  /// Add a value to the parameters of the Frame (if the type is supported).
  ///
  /// @tparam T    The type of the parameter. Has to be one of the types that
  ///              is supported by GenericParameters
  /// @param key   The name under which this parameter should be stored
  /// @param value The value of the parameter. A copy will be put into the Frame
  template <typename T, typename = podio::EnableIfValidGenericDataType<T>>
  inline void putParameter(const std::string& key, T value) {
    m_self->parameters().set(key, std::move(value));
  }

  /// Add a string value to the parameters of the Frame.
  ///
  /// This is a dedicated overload for enabling on-the-fly conversion from
  /// string literals.
  ///
  /// @param key   The name under which this parameter should be stored
  /// @param value The value of the parameter. A copy will be put into the Frame
  inline void putParameter(const std::string& key, std::string value) {
    putParameter<std::string>(key, std::move(value));
  }

  /// Add a vector of strings value the parameters of the Frame.
  ///
  /// This is a dedicated overload for enabling on-the-fly conversion from
  /// an initializer_list of string literals
  ///
  /// @param key    The name under which this parameter should be stored
  /// @param values The values of the parameter. A copy will be put into the Frame
  inline void putParameter(const std::string& key, std::vector<std::string> values) {
    putParameter<std::vector<std::string>>(key, std::move(values));
  }

  /// Add a vector of values to the parameters of the Frame (if the type is
  /// supported).
  ///
  /// This is a dedicated overload for enabling on-the-fly conversions of
  /// initializer_list of values
  ///
  /// @tparam T    The type of the parameter. Has to be one of the types that
  ///              is supported by GenericParameters
  /// @param key    The name under which this parameter should be stored
  /// @param values The values of the parameter. A copy will be put into the Frame
  template <typename T, typename = std::enable_if_t<detail::isInTuple<T, SupportedGenericDataTypes>>>
  inline void putParameter(const std::string& key, std::initializer_list<T>&& values) {
    putParameter<std::vector<T>>(key, std::move(values));
  }

  /// Retrieve parameters via key from the internal store.
  ///
  /// @tparam T  The desired type of the parameter (can also be std::vector<T>)
  /// @param key The key under which the value is stored
  ///
  /// @returns   An optional holding the value if it is present
  template <typename T>
  inline auto getParameter(const std::string& key) const {
    static_assert(podio::isSupportedGenericDataType<T>, "Unsupported parameter type");
    return m_self->parameters().get<T>(key);
  }

  /// Retrieve all parameters stored in this Frame.
  ///
  /// This is mainly intended for I/O purposes and we encourage to use the Frame
  /// functionality of getParameter or getParameterKeys in general.
  ///
  /// @returns The internally used GenericParameters
  inline const podio::GenericParameters& getParameters() const {
    return m_self->parameters();
  }

  /// Get the keys of all stored parameters for a given type
  ///
  /// @tparam T The desired parameter type
  ///
  /// @returns  A vector of keys for this parameter type
  template <typename T>
  inline std::vector<std::string> getParameterKeys() const {
    static_assert(podio::isSupportedGenericDataType<T>, "Unsupported parameter type");
    return m_self->parameters().getKeys<T>();
  }

  /// Get all **currently** available collection names.
  ///
  /// @returns The names of all collections, including those that might still
  ///          need unpacking from the internal FrameData
  std::vector<std::string> getAvailableCollections() const {
    return m_self->availableCollections();
  }

  /// Get the name of the passed collection
  ///
  /// @param coll The collection for which the name should be obtained
  ///
  /// @returns The name of the collection or an empty optional if this
  ///          collection is not known to the Frame
  inline std::optional<std::string> getName(const podio::CollectionBase& coll) const {
    return getName(coll.getID());
  }

  /// Get the name for the passed collectionID
  ///
  /// @param collectionID The collection ID of the collection for which the name
  ///                     should be obtained
  /// @returns The name of the collection or an empty optional if this
  ///          collectionID is not known to the Frame
  inline std::optional<std::string> getName(const uint32_t collectionID) const {
    return m_self->getIDTable().name(collectionID);
  }

  // Interfaces for writing below

  /// Get a collection for writing.
  ///
  /// @note This method is intended for I/O purposes only and should not be used
  /// in other code.
  ///
  /// @returns The collection pointer in a prepared and "ready-to-write" state
  const podio::CollectionBase* getCollectionForWrite(const std::string& name) const {
    const auto* coll = m_self->get(name);
    if (coll) {
      coll->prepareForWrite();
    }

    return coll;
  }

  /// Get the internal CollectionIDTable for writing.
  ///
  /// @note This method is intended for I/O purposes only and should not be used
  /// in other code.
  ///
  /// @returns A copy of the internal collection id table
  podio::CollectionIDTable getCollectionIDTableForWrite() const {
    return m_self->getIDTable();
  }
};

// implementations below

inline Frame::Frame() : Frame(std::make_unique<detail::EmptyFrameData>()) {
}

template <typename FrameDataT>
Frame::Frame(std::unique_ptr<FrameDataT> data) : m_self(std::make_unique<FrameModel<FrameDataT>>(std::move(data))) {
}

template <typename FrameDataT, typename>
Frame::Frame(FrameDataT&& data) : Frame(std::make_unique<FrameDataT>(std::move(data))) {
}

template <typename CollT, typename>
const CollT& Frame::get(const std::string& name) const {
  const auto* coll = dynamic_cast<const CollT*>(m_self->get(name));
  if (coll) {
    return *coll;
  }
  // TODO: Handle non-existing collections
  static const auto emptyColl = CollT();
  return emptyColl;
}

inline const podio::CollectionBase* Frame::get(const std::string& name) const {
  return m_self->get(name);
}

inline void Frame::put(std::unique_ptr<podio::CollectionBase> coll, const std::string& name) {
  const auto* retColl = m_self->put(std::move(coll), name);
  if (!retColl) {
    // TODO: Handle collisions
  }
}

template <typename CollT, typename>
const CollT& Frame::put(CollT&& coll, const std::string& name) {
  const auto* retColl = static_cast<const CollT*>(m_self->put(std::make_unique<CollT>(std::move(coll)), name));
  if (retColl) {
    return *retColl;
  }
  // TODO: Handle collision case
  static const auto emptyColl = CollT();
  return emptyColl;
}

template <typename FrameDataT>
Frame::FrameModel<FrameDataT>::FrameModel(std::unique_ptr<FrameDataT> data) :
    m_mapMtx(std::make_unique<std::mutex>()), m_dataMtx(std::make_unique<std::mutex>()) {
  if (!data) {
    throw std::invalid_argument(
        "FrameData is a nullptr. If you are reading from a file it may be corrupted or you may reading beyond the end "
        "of the file, please check the validity of the data before creating a Frame.");
  }
  m_data = std::move(data);
  m_idTable = std::move(m_data->getIDTable());
  m_parameters = std::move(m_data->getParameters());
}

template <typename FrameDataT>
const podio::CollectionBase* Frame::FrameModel<FrameDataT>::get(const std::string& name) const {
  return doGet(name);
}

template <typename FrameDataT>
podio::CollectionBase* Frame::FrameModel<FrameDataT>::doGet(const std::string& name, bool setReferences) const {
  {
    // First check whether the collection is in the map already
    //
    // Collections only land here if they are fully unpacked, i.e.
    // prepareAfterRead has been called or it has been put into the Frame
    std::lock_guard lock{*m_mapMtx};
    if (const auto it = m_collections.find(name); it != m_collections.end()) {
      return it->second.get();
    }
  }

  podio::CollectionBase* retColl = nullptr;

  // Now try to get it from the raw data if we have the possibility
  if (m_data) {
    // Have the buffers in the outer scope here to hold the raw data lock as
    // briefly as possible
    auto buffers = std::optional<podio::CollectionReadBuffers>{std::nullopt};
    {
      std::lock_guard lock{*m_dataMtx};
      buffers = unpack(m_data.get(), name);
    }
    if (buffers) {
      std::unique_ptr<podio::CollectionBase> coll{nullptr};
      // Subset collections do not need schema evolution (by definition)
      if (buffers->data == nullptr) {
        coll = buffers->createCollection(buffers.value(), true);
      } else {
        auto evolvedBuffers = podio::SchemaEvolution::instance().evolveBuffers(buffers.value(), buffers->schemaVersion,
                                                                               std::string(buffers->type));
        coll = evolvedBuffers.createCollection(evolvedBuffers, false);
      }

      coll->prepareAfterRead();
      coll->setID(m_idTable.collectionID(name).value());
      {
        std::lock_guard mapLock{*m_mapMtx};
        auto [it, success] = m_collections.emplace(name, std::move(coll));
        // TODO: Check success? Or simply assume that everything is fine at this point?
        // TODO: Collision handling?
        retColl = it->second.get();
      }

      if (setReferences) {
        retColl->setReferences(this);
      }
    }
  }

  return retColl;
}

template <typename FrameDataT>
bool Frame::FrameModel<FrameDataT>::get(uint32_t collectionID, CollectionBase*& collection) const {
  const auto name = m_idTable.name(collectionID);
  if (!name) {
    return false;
  }
  const auto& [_, inserted] = m_retrievedIDs.insert(collectionID);

  if (inserted) {
    auto coll = doGet(name.value());
    if (coll) {
      collection = coll;
      return true;
    }
  } else {
    auto coll = doGet(name.value(), false);
    if (coll) {
      collection = coll;
      return true;
    }
  }

  return false;
}

template <typename FrameDataT>
const podio::CollectionBase* Frame::FrameModel<FrameDataT>::put(std::unique_ptr<podio::CollectionBase> coll,
                                                                const std::string& name) {
  {
    std::lock_guard lock{*m_mapMtx};
    auto [it, success] = m_collections.try_emplace(name, std::move(coll));
    if (success) {
      // TODO: Check whether this collection is already known to the idTable
      // -> What to do on collision?
      // -> Check before we emplace it into the internal map to prevent possible
      //    collisions from collections that are potentially present from rawdata?
      it->second->setID(m_idTable.add(name));
      return it->second.get();
    } else {
      throw std::invalid_argument("An object with key " + name + " already exists in the frame");
    }
  }

  return nullptr;
}

template <typename FrameDataT>
std::vector<std::string> Frame::FrameModel<FrameDataT>::availableCollections() const {
  // TODO: Check if there is a more efficient way to do this. Currently this is
  // done very conservatively, but in a way that should always work, regardless
  // of assumptions. It might be possible to simply return what is in the
  // idTable here, because that should in principle encompass everything that is
  // in the raw data as well as things that have been put into the frame

  // Lock both the internal map and the rawdata for this
  std::scoped_lock lock{*m_mapMtx, *m_dataMtx};

  auto collections = m_data->getAvailableCollections();
  collections.reserve(collections.size() + m_collections.size());

  for (const auto& [name, _] : m_collections) {
    collections.push_back(name);
  }

  return collections;
}

} // namespace podio

#endif // PODIO_FRAME_H
