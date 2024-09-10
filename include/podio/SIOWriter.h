#ifndef PODIO_SIOWRITER_H
#define PODIO_SIOWRITER_H

#include "podio/SIOBlock.h"
#include "podio/utilities/DatamodelRegistryIOHelpers.h"
#include <sio/definitions.h>

#include <string>
#include <utility>
#include <vector>

namespace podio {

class Frame;

/// The SIOWriter writes podio files into SIO files.
///
/// Each Frame is stored into an SIO record which are written in the order in
/// which Frames are written. As part of the metadata a table of content record
/// is written which is used in the reader to more quickly access the different
/// records.
///
/// Files written with the SIOWriter can be read with the SIOReader
class SIOWriter {
public:
  /// Create a SIOWriter to write to a file.
  ///
  /// @note Existing files will be overwritten without warning.
  ///
  /// @param filename The path to the file that will be created.
  SIOWriter(const std::string& filename);

  /// SIOWriter destructor
  ///
  /// This also takes care of writing all the necessary metadata to read files back again.
  ~SIOWriter();

  /// The SIOWriter is not copy-able
  SIOWriter(const SIOWriter&) = delete;
  /// The SIOWriter is not copy-able
  SIOWriter& operator=(const SIOWriter&) = delete;

  /// Store the given frame with the given category.
  ///
  /// This stores all available collections from the Frame. Frames of the same
  /// category can be independent of each other, i.e. they can have different
  /// collection contents.
  ///
  /// @param frame    The Frame to store
  /// @param category The category name under which this Frame should be stored
  void writeFrame(const podio::Frame& frame, const std::string& category);

  /// Store the given Frame with the given category.
  ///
  /// This stores only the desired collections and not the complete frame.
  /// Frames of the same category can be independent of each other, i.e. they
  /// can have different collection contents.
  ///
  /// @param frame        The Frame to store
  /// @param category     The category name under which this Frame should be
  ///                     stored
  /// @param collsToWrite The collection names that should be written
  void writeFrame(const podio::Frame& frame, const std::string& category, const std::vector<std::string>& collsToWrite);

  /// Write the current file, including all the necessary metadata to read it
  /// again.
  ///
  /// @note The destructor will also call this, so letting a SIOWriter go out
  /// of scope is also a viable way to write a readable file
  void finish();

private:
  sio::ofstream m_stream{};       ///< The output file stream
  SIOFileTOCRecord m_tocRecord{}; ///< The "table of contents" of the written file
  DatamodelDefinitionCollector m_datamodelCollector{};
  bool m_finished{false}; ///< Has finish been called already?
};
} // namespace podio

#endif // PODIO_SIOWRITER_H
