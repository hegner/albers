#ifndef PODIO_SIOREADER_H
#define PODIO_SIOREADER_H

#include "podio/SIOBlock.h"
#include "podio/SIOFrameData.h"
#include "podio/podioVersion.h"
#include "podio/utilities/DatamodelRegistryIOHelpers.h"
#include <sio/definitions.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace podio {

class CollectionIDTable;

/// The SIOReader can be used to read files that have been written with the SIO
/// backend.
///
/// The SIOReader provides the data as SIOFrameData from which a podio::Frame
/// can be constructed. It can be used to read files written by the SIOWriter.
class SIOReader {

public:
  /// Create an SIOReader
  SIOReader();
  /// SIOReader destructor
  ~SIOReader() = default;

  /// The SIOReader is not copy-able
  SIOReader(const SIOReader&) = delete;
  /// The SIOReader is not copy-able
  SIOReader& operator=(const SIOReader&) = delete;

  /// Read the next data entry for a given category.
  ///
  /// @param name The category name for which to read the next entry
  ///
  /// @returns FrameData from which a podio::Frame can be constructed if the
  ///          category exists and if there are still entries left to read.
  ///          Otherwise a nullptr
  std::unique_ptr<podio::SIOFrameData> readNextEntry(const std::string& name);

  /// Read the desired data entry for a given category.
  ///
  /// @param name  The category name for which to read the next entry
  /// @param entry The entry number to read
  ///
  /// @returns FrameData from which a podio::Frame can be constructed if the
  ///          category and the desired entry exist. Otherwise a nullptr
  std::unique_ptr<podio::SIOFrameData> readEntry(const std::string& name, const unsigned entry);

  /// Get the number of entries for the given name
  ///
  /// @param name The name of the category
  ///
  /// @returns The number of entries that are available for the category
  unsigned getEntries(const std::string& name) const;

  /// Open the passed file for reading.
  ///
  /// @param filename The path to the file to read from
  void openFile(const std::string& filename);

  /// Get the build version of podio that has been used to write the current
  /// file
  ///
  /// @returns The podio build version
  podio::version::Version currentFileVersion() const {
    return m_fileVersion;
  }

  /// Get the names of all the available Frame categories in the current file.
  ///
  /// @returns The names of the available categores from the file
  std::vector<std::string_view> getAvailableCategories() const;

  /// Get the datamodel definition for the given name
  ///
  /// @param name The name of the datamodel
  ///
  /// @returns The high level definition of the datamodel in JSON format
  const std::string_view getDatamodelDefinition(const std::string& name) const {
    return m_datamodelHolder.getDatamodelDefinition(name);
  }

  /// Get all names of the datamodels that are available from this reader
  ///
  /// @returns The names of the datamodels
  std::vector<std::string> getAvailableDatamodels() const {
    return m_datamodelHolder.getAvailableDatamodels();
  }

private:
  void readPodioHeader();

  /// read the TOC record
  bool readFileTOCRecord();

  void readEDMDefinitions();

  sio::ifstream m_stream{}; ///< The stream from which we read

  /// Count how many times each an entry of this name has been read already
  std::unordered_map<std::string, unsigned> m_nameCtr{};

  /// Table of content record where starting points of named entries can be read from
  SIOFileTOCRecord m_tocRecord{};
  /// The podio version that has been used to write the file
  podio::version::Version m_fileVersion{0};

  DatamodelDefinitionHolder m_datamodelHolder{};
};

} // namespace podio

#endif // PODIO_SIOREADER_H
