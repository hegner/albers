#ifndef PODIO_RNTUPLEREADER_H
#define PODIO_RNTUPLEREADER_H

#include "podio/ROOTFrameData.h"
#include "podio/SchemaEvolution.h"
#include "podio/podioVersion.h"
#include "podio/utilities/DatamodelRegistryIOHelpers.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <ROOT/RNTuple.hxx>
#include <RVersion.h>
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 31, 0)
  #include <ROOT/RNTupleReader.hxx>
#endif

namespace podio {

/**
This class has the function to read available data from disk
and to prepare collections and buffers.
**/
/// The RNTupleReader can be used to read files that have been written with the
/// RNTuple backend.
///
/// The RNTupleReader provides the data as ROOTFrameData from which a podio::Frame
/// can be constructed. It can be used to read files written by the RNTupleWriter.
class RNTupleReader {

public:
  /// Create a RNTupleReader
  RNTupleReader() = default;
  /// The RNTupleReader is not copy-able
  RNTupleReader(const RNTupleReader&) = delete;
  /// The RNTupleReader is not copy-able
  RNTupleReader& operator=(const RNTupleReader&) = delete;

  /// Open a single file for reading.
  ///
  /// @param filename The name of the input file
  void openFile(const std::string& filename);

  /// Open multiple files for reading and then treat them as if they are one file
  ///
  /// @note All of the files are assumed to have the same structure. Specifically
  /// this means:
  /// - The same categories are available from all files
  /// - The collections that are contained in the individual categories are the
  ///   same across all files
  /// - This usually boils down to "the files have been written with the same
  ///   "settings", e.g. they are outputs of a batched process.
  ///
  /// @param filenames The filenames of all input files that should be read
  void openFiles(const std::vector<std::string>& filenames);

  /// Read the next data entry for a given category.
  ///
  /// @param name The category name for which to read the next entry
  ///
  /// @returns FrameData from which a podio::Frame can be constructed if the
  ///          category exists and if there are still entries left to read.
  ///          Otherwise a nullptr
  std::unique_ptr<podio::ROOTFrameData> readNextEntry(const std::string& name);

  /// Read the desired data entry for a given category.
  ///
  /// @param name  The category name for which to read the next entry
  /// @param entry The entry number to read
  ///
  /// @returns FrameData from which a podio::Frame can be constructed if the
  ///          category and the desired entry exist. Otherwise a nullptr
  std::unique_ptr<podio::ROOTFrameData> readEntry(const std::string& name, const unsigned entry);

  /// Get the names of all the available Frame categories in the current file(s).
  ///
  /// @returns The names of the available categores from the file
  std::vector<std::string_view> getAvailableCategories() const;

  /// Get the number of entries for the given name
  ///
  /// @param name The name of the category
  ///
  /// @returns The number of entries that are available for the category
  unsigned getEntries(const std::string& name);

  /// Get the build version of podio that has been used to write the current
  /// file
  ///
  /// @returns The podio build version
  podio::version::Version currentFileVersion() const {
    return m_fileVersion;
  }

  /// Get the (build) version of a datamodel that has been used to write the
  /// current file
  ///
  /// @param name The name of the datamodel
  ///
  /// @returns The (build) version of the datamodel if available or an empty
  ///          optional
  std::optional<podio::version::Version> currentFileVersion(const std::string& name) const {
    return m_datamodelHolder.getDatamodelVersion(name);
  }

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
  /**
   * Initialize the given category by filling the maps with metadata information
   * that will be used later
   */
  bool initCategory(const std::string& category);

  /**
   * Read and reconstruct the generic parameters of the Frame
   */
  GenericParameters readEventMetaData(const std::string& name, unsigned entNum);

  template <typename T>
  void readParams(const std::string& name, unsigned entNum, GenericParameters& params);

  std::unique_ptr<ROOT::Experimental::RNTupleReader> m_metadata{};

  podio::version::Version m_fileVersion{};
  DatamodelDefinitionHolder m_datamodelHolder{};

  std::unordered_map<std::string, std::vector<std::unique_ptr<ROOT::Experimental::RNTupleReader>>> m_readers{};
  std::unordered_map<std::string, std::unique_ptr<ROOT::Experimental::RNTupleReader>> m_metadata_readers{};
  std::vector<std::string> m_filenames{};

  std::unordered_map<std::string, int> m_entries{};
  std::unordered_map<std::string, unsigned> m_totalEntries{};

  struct CollectionInfo {
    std::vector<unsigned int> id{};
    std::vector<std::string> name{};
    std::vector<std::string> type{};
    std::vector<short> isSubsetCollection{};
    std::vector<SchemaVersionT> schemaVersion{};
  };

  std::unordered_map<std::string, CollectionInfo> m_collectionInfo{};

  std::vector<std::string> m_availableCategories{};

  std::unordered_map<std::string, std::shared_ptr<podio::CollectionIDTable>> m_idTables{};
};

} // namespace podio

#endif
