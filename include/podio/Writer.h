#ifndef PODIO_WRITER_H
#define PODIO_WRITER_H

#include "podio/Frame.h"
#include "podio/FrameCategories.h"

#include <memory>
#include <string>
#include <vector>

namespace podio {

/// Generic (type erased) writer class that can handle different I/O backends
/// (almost) transparently
///
/// Offers some more high level functionality compared to the lower level
/// backend specific writers that this class wraps. In addition, it provides
/// convenience methods to deal specifically with the "events" frame category.
///
/// @note Since this simply wraps lower level writers, some of the limitations
/// of the wrapped writers will still apply, e.g. if used for writing ROOT files
/// frames of a given category will have to have the same contents.
///
/// @note The recommended way to construct is to use the makeWriter() function
/// since that handles the instantiation of the correct low level writers
class Writer {
private:
  struct WriterConcept {
    virtual ~WriterConcept() = default;

    virtual void writeFrame(const podio::Frame& frame, const std::string& category,
                            const std::vector<std::string>& collections) = 0;
    virtual void finish() = 0;
  };

private:
  template <typename T>
  struct WriterModel final : WriterConcept {
    WriterModel(std::unique_ptr<T> writer) : m_writer(std::move(writer)) {
    }
    WriterModel(const WriterModel&) = delete;
    WriterModel& operator=(const WriterModel&) = delete;
    WriterModel(WriterModel&&) = default;
    WriterModel& operator=(WriterModel&&) = default;

    ~WriterModel() = default;

    void writeFrame(const podio::Frame& frame, const std::string& category,
                    const std::vector<std::string>& collections) override {
      return m_writer->writeFrame(frame, category, collections);
    }
    void finish() override {
      return m_writer->finish();
    }
    std::unique_ptr<T> m_writer{nullptr};
  };

  std::unique_ptr<WriterConcept> m_self{nullptr};

public:
  /// Create a Writer from a lower level writer
  ///
  /// @tparam T the type of the low level writer (will be deduced)
  /// @param writer A low level writer that does the actual work
  template <typename T>
  Writer(std::unique_ptr<T> writer) : m_self(std::make_unique<WriterModel<T>>(std::move(writer))) {
  }

  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;
  Writer(Writer&&) = default;
  Writer& operator=(Writer&&) = default;

  /// Destructor
  ///
  /// This also takes care of writing all the necessary metadata to read files
  /// back again.
  ~Writer() = default;

  /// Store the given frame with the given category
  ///
  /// This stores all available categories from the passed frame
  ///
  /// @param frame    The frame to write
  /// @param category The category name under which this frame should be stored
  void writeFrame(const podio::Frame& frame, const std::string& category) {
    return m_self->writeFrame(frame, category, frame.getAvailableCollections());
  }

  /// Store the given Frame with the given category.
  ///
  /// This stores only the desired collections and not the complete frame.
  ///
  /// @param frame        The Frame to store
  /// @param category     The category name under which this Frame should be
  ///                     stored
  /// @param collsToWrite The collection names that should be written
  void writeFrame(const podio::Frame& frame, const std::string& category, const std::vector<std::string>& collections) {
    return m_self->writeFrame(frame, category, collections);
  }

  /// Store the given frame under the "events" category
  ///
  /// This stores all available categories from the passed frame
  ///
  /// @param frame    The frame to write
  void writeEvent(const podio::Frame& frame) {
    writeFrame(frame, podio::Category::Event, frame.getAvailableCollections());
  }

  /// Store the given Frame under the "events" category
  ///
  /// This stores only the desired collections and not the complete frame.
  ///
  /// @param frame        The Frame to store
  /// @param collsToWrite The collection names that should be written
  void writeEvent(const podio::Frame& frame, const std::vector<std::string>& collections) {
    writeFrame(frame, podio::Category::Event, collections);
  }

  /// Write the current file, including all the necessary metadata to read it
  /// again.
  ///
  /// @note The destructor will also call this, so letting a Writer go out of
  /// scope is also a viable way to write a readable file
  void finish() {
    return m_self->finish();
  }
};

/// Create a Writer that is able to write files for the desired backend
///
/// Will look at the desired filename as well as the type argument to decide on
/// the backend.
///
/// @param filename The filename of the output file that will be created.
/// @param type     The (optional) type argument to switch between RNTuple and TTree
///                 based backend in case the suffix is ".root". Will be ignored
///                 in case the suffix is ".sio"
///
/// @returns A fully initialized Writer for the I/O backend that has been
///         determined
///
/// @throws std::runtime_error In case the suffix can not be associated to an
///         I/O backend or if support for the desired I/O backend has not been built
Writer makeWriter(const std::string& filename, const std::string& type = "default");

} // namespace podio

#endif // PODIO_WRITER_H
