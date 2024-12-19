#ifndef PODIO_READER_H
#define PODIO_READER_H

#include "podio/Frame.h"
#include "podio/podioVersion.h"

namespace podio {

/// Generic (type erased) reader class that can handle different I/O backends
/// transparently
///
/// Offers some more high level functionality compared to the lower level
/// backend specific readers that this class wraps. In contrast to the lower
/// level readers that usually return arbitrary FrameData, this interface class
/// will return fully constructed Frames. In addition, it provides convenience
/// methods to deal specifically with the "events" frame category.
///
/// @note The recommended way to construct is to use the makeReader() functions
/// since they handle the instantiation of the correct low level readers
class Reader {
private:
  struct ReaderConcept {
    virtual ~ReaderConcept() = default;

    virtual podio::Frame readNextFrame(const std::string& name) = 0;
    virtual podio::Frame readFrame(const std::string& name, size_t index) = 0;
    virtual size_t getEntries(const std::string& name) const = 0;
    virtual podio::version::Version currentFileVersion() const = 0;
    virtual std::optional<podio::version::Version> currentFileVersion(const std::string& name) const = 0;
    virtual std::vector<std::string_view> getAvailableCategories() const = 0;
    virtual const std::string_view getDatamodelDefinition(const std::string& name) const = 0;
    virtual std::vector<std::string> getAvailableDatamodels() const = 0;
  };

private:
  template <typename T>
  struct ReaderModel final : ReaderConcept {
    ReaderModel(std::unique_ptr<T> reader) : m_reader(std::move(reader)) {
    }
    ReaderModel(const ReaderModel&) = delete;
    ReaderModel& operator=(const ReaderModel&) = delete;
    ReaderModel(ReaderModel&&) = default;
    ReaderModel& operator=(ReaderModel&&) = default;

    podio::Frame readNextFrame(const std::string& name) override {
      auto maybeFrame = m_reader->readNextEntry(name);
      if (maybeFrame) {
        return maybeFrame;
      }
      throw std::runtime_error("Failed reading category " + name + " (reading beyond bounds?)");
    }

    podio::Frame readFrame(const std::string& name, size_t index) override {
      auto maybeFrame = m_reader->readEntry(name, index);
      if (maybeFrame) {
        return maybeFrame;
      }
      throw std::runtime_error("Failed reading category " + name + " at frame " + std::to_string(index) +
                               " (reading beyond bounds?)");
    }
    size_t getEntries(const std::string& name) const override {
      return m_reader->getEntries(name);
    }
    podio::version::Version currentFileVersion() const override {
      return m_reader->currentFileVersion();
    }

    std::optional<podio::version::Version> currentFileVersion(const std::string& name) const override {
      return m_reader->currentFileVersion(name);
    }

    std::vector<std::string_view> getAvailableCategories() const override {
      return m_reader->getAvailableCategories();
    }

    const std::string_view getDatamodelDefinition(const std::string& name) const override {
      return m_reader->getDatamodelDefinition(name);
    }

    std::vector<std::string> getAvailableDatamodels() const override {
      return m_reader->getAvailableDatamodels();
    }

    std::unique_ptr<T> m_reader;
  };

  std::unique_ptr<ReaderConcept> m_self{nullptr};

public:
  /// Create a reader from a low level reader
  ///
  /// @tparam T The type of the low level reader (will be deduced)
  /// @param actualReader a low level reader that provides access to FrameDataT
  template <typename T>
  Reader(std::unique_ptr<T> actualReader);

  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
  Reader(Reader&&) = default;
  Reader& operator=(Reader&&) = default;

  /// Read the next frame of a given category
  ///
  /// @param name The category name for which to read the next frame
  ///
  /// @returns A fully constructed Frame with the contents read from file
  ///
  /// @throws std::invalid_argument in case the category is not available or in
  ///         case no more entries are available
  podio::Frame readNextFrame(const std::string& name) {
    return m_self->readNextFrame(name);
  }

  /// Read the next frame of the "events" category
  ///
  /// @returns A fully constructed Frame with the contents read from file
  ///
  /// @throws std::invalid_argument in case no (more) events are available
  podio::Frame readNextEvent() {
    return readNextFrame(podio::Category::Event);
  }

  /// Read a specific frame for a given category
  ///
  /// @param name  The category name for which to read the next entry
  /// @param index The entry number to read
  ///
  /// @returns A fully constructed Frame with the contents read from file
  ///
  /// @throws std::invalid_argument in case the category is not available or in
  ///         case the specified entry is not available
  podio::Frame readFrame(const std::string& name, size_t index) {
    return m_self->readFrame(name, index);
  }

  /// Read a specific frame of the "events" category
  ///
  /// @param index The event number to read
  ///
  /// @returns A fully constructed Frame with the contents read from file
  ///
  /// @throws std::invalid_argument in case the desired event is not available
  podio::Frame readEvent(size_t index) {
    return readFrame(podio::Category::Event, index);
  }

  /// Get the number of entries for the given name
  ///
  /// @param name The name of the category
  ///
  /// @returns The number of entries that are available for the category
  size_t getEntries(const std::string& name) const {
    return m_self->getEntries(name);
  }

  /// Get the number of events
  ///
  /// @returns The number of entries that are available for the category
  size_t getEvents() const {
    return getEntries(podio::Category::Event);
  }

  /// Get the build version of podio that has been used to write the current
  /// file
  ///
  /// @returns The podio build version
  podio::version::Version currentFileVersion() const {
    return m_self->currentFileVersion();
  }

  /// Get the (build) version of a datamodel that has been used to write the
  /// current file
  ///
  /// @param name The name of the datamodel
  ///
  /// @returns The (build) version of the datamodel if available or an empty
  ///          optional
  std::optional<podio::version::Version> currentFileVersion(const std::string& name) const {
    return m_self->currentFileVersion(name);
  }

  /// Get the names of all the available Frame categories in the current file(s).
  ///
  /// @returns The names of the available categories from the file
  std::vector<std::string_view> getAvailableCategories() const {
    return m_self->getAvailableCategories();
  }

  /// Get the datamodel definition for the given name
  ///
  /// @param name The name of the datamodel
  ///
  /// @returns The high level definition of the datamodel in JSON format
  const std::string_view getDatamodelDefinition(const std::string& name) const {
    return m_self->getDatamodelDefinition(name);
  }

  /// Get all names of the datamodels that are available from this reader
  ///
  /// @returns The names of the datamodels
  std::vector<std::string> getAvailableDatamodels() const {
    return m_self->getAvailableDatamodels();
  }
};

/// Create a Reader is able to read the file
///
/// This will inspect the filename as well as peek at the file contents to
/// instantiate the correct low level reader to open and read the file
///
/// @param filename The (path to the) file to read from
///
/// @returns A Reader that has been initialized and that can be used for reading
///          data from the passed file
Reader makeReader(const std::string& filename);

/// Create a Reader that is able to read the files
///
/// This will inspect the filenames as well as peek into the **first file only**
/// to decide based on the contents which low level reader to instantiate for
/// reading. All files are assumed to be of the same I/O format, no switching
/// between formats is possible.
///
/// @note For SIO files this will only work with exactly one file!
///
/// @param filenames The (paths to the) files to read from
///
/// @returns A Reader that has been initialized and that can be used for reading
///          data from the passed files
///
/// @throws std::runtime_error in case the file extensions differ or in case
///         support for the necessary I/O backend has not been built or in case
///         multiple files for the SIO backend are passed
Reader makeReader(const std::vector<std::string>& filenames);

} // namespace podio

#endif // PODIO_READER_H
