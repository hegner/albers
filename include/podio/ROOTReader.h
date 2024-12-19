#ifndef PODIO_ROOTREADER_H
#define PODIO_ROOTREADER_H

#include "podio/ROOTFrameData.h"
#include "podio/podioVersion.h"
#include "podio/utilities/DatamodelRegistryIOHelpers.h"
#include "podio/utilities/RootHelpers.h"

#include "TChain.h"

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

// forward declarations
class TClass;
class TFile;
class TTree;

namespace podio {

namespace detail {
  // Information about the collection class type, whether it is a subset, the
  // schema version on file and the index in the collection branches cache
  // vector
  using CollectionInfo = std::tuple<std::string, bool, SchemaVersionT, size_t>;

} // namespace detail

class CollectionBase;
class CollectionIDTable;
class GenericParameters;
struct CollectionReadBuffers;

/// This class has the function to read available data from disk in ROOTs TTree
/// format.
///
/// The ROOTReader provides the data as ROOTFrameData from which a podio::Frame
/// can be constructed. It can be used to read files written by the ROOTWriter.
class ROOTReader {

public:
  /// Create a ROOTReader
  ROOTReader() = default;
  /// The ROOTReader is not copy-able
  ROOTReader(const ROOTReader&) = delete;
  /// The ROOTReader is not copy-able
  ROOTReader& operator=(const ROOTReader&) = delete;

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

  /// Get the number of entries for the given name
  ///
  /// @param name The name of the category
  ///
  /// @returns The number of entries that are available for the category
  unsigned getEntries(const std::string& name) const;

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

  /// Get the names of all the available Frame categories in the current file(s).
  ///
  /// @returns The names of the available categories from the file
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
  /// Helper struct to group together all the necessary state to read / process
  /// a given category. A "category" in this case describes all frames with the
  /// same name which are constrained by the ROOT file structure that we use to
  /// have the same contents. It encapsulates all state that is necessary for
  /// reading from a TTree / TChain (i.e. collection infos, branches, ...)
  struct CategoryInfo {
    /// constructor from chain for more convenient map insertion
    CategoryInfo(std::unique_ptr<TChain>&& c) : chain(std::move(c)) {
    }
    std::unique_ptr<TChain> chain{nullptr};                                      ///< The TChain with the data
    unsigned entry{0};                                                           ///< The next entry to read
    std::vector<std::pair<std::string, detail::CollectionInfo>> storedClasses{}; ///< The stored collections in this
                                                                                 ///< category
    std::vector<root_utils::CollectionBranches> branches{};                      ///< The branches for this category
    std::shared_ptr<CollectionIDTable> table{nullptr}; ///< The collection ID table for this category
  };

  /// Initialize the passed CategoryInfo by setting up the necessary branches,
  /// collection infos and all necessary meta data to be able to read entries
  /// with this name
  void initCategory(CategoryInfo& catInfo, const std::string& name);

  /// Get the category information for the given name. In case there is no TTree
  /// with contents for the given name this will return a CategoryInfo with an
  /// uninitialized chain (nullptr) member
  CategoryInfo& getCategoryInfo(const std::string& name);

  /// Read the parameters for the entry specified in the passed CategoryInfo
  GenericParameters readEntryParameters(CategoryInfo& catInfo, bool reloadBranches, unsigned int localEntry);

  template <typename T>
  static void readParams(CategoryInfo& catInfo, podio::GenericParameters& params, bool reloadBranches,
                         unsigned int localEntry);

  /// Read the data entry specified in the passed CategoryInfo, and increase the
  /// counter afterwards. In case the requested entry is larger than the
  /// available number of entries, return a nullptr.
  std::unique_ptr<podio::ROOTFrameData> readEntry(ROOTReader::CategoryInfo& catInfo);

  /// Get / read the buffers at index iColl in the passed category information
  podio::CollectionReadBuffers getCollectionBuffers(CategoryInfo& catInfo, size_t iColl, bool reloadBranches,
                                                    unsigned int localEntry);

  std::unique_ptr<TChain> m_metaChain{nullptr};                 ///< The metadata tree
  std::unordered_map<std::string, CategoryInfo> m_categories{}; ///< All categories
  std::vector<std::string> m_availCategories{};                 ///< All available categories from this file

  podio::version::Version m_fileVersion{0, 0, 0};
  DatamodelDefinitionHolder m_datamodelHolder{};
};

} // namespace podio

#endif // PODIO_ROOTREADER_H
