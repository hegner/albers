#ifndef PODIO_ROOTLEGACYREADER_H
#define PODIO_ROOTLEGACYREADER_H

#include "podio/ROOTFrameData.h"
#include "podio/podioVersion.h"
#include "podio/utilities/RootHelpers.h"

#include "TChain.h"

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// forward declarations
class TClass;
// class TChain;
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

/// A root reader for reading legacy podio root files that have been written
/// using the legacy, non Frame based I/O model. This reader grants Frame based
/// access to those files, by mimicking the Frame I/O functionality and the
/// interfaces of those readers.
///
/// @note Since there was only one category ("events") for those legacy podio
/// files this reader will really only work if you try to read that category, and
/// will simply return no data if you try to read anything else.
class ROOTLegacyReader {

public:
  /// Create a SIOLegacyReader
  ROOTLegacyReader() = default;
  /// Destructor
  ~ROOTLegacyReader() = default;

  /// The SIOLegacyReader is not copy-able
  ROOTLegacyReader(const ROOTLegacyReader&) = delete;
  /// The SIOLegacyReader is not copy-able
  ROOTLegacyReader& operator=(const ROOTLegacyReader&) = delete;

  /// Open a single file for reading.
  ///
  /// @param filename The name of the input file
  void openFile(const std::string& filename);

  /// Open multiple files for reading and then treat them as if they are one file
  ///
  /// @note All of the files are assumed to have the same structure. Specifically
  /// this means:
  /// - The collections that are contained in the individual event are always the
  ///   same
  ///
  /// This usually boils down to "the files have been written with the same
  /// "settings", e.g. they are outputs of a batched process.
  ///
  /// @param filenames The filenames of all input files that should be read
  void openFiles(const std::vector<std::string>& filenames);

  /// Read the next data entry from which a Frame can be constructed.
  ///
  /// @note the category name has to be "events" in this case, as only that
  /// category is available for legacy files. Also the collections to read
  /// argument will be ignored.
  ///
  /// @returns FrameData from which a podio::Frame can be constructed if there
  ///          are still entries left to read. Otherwise a nullptr
  std::unique_ptr<podio::ROOTFrameData> readNextEntry(const std::string&, const std::vector<std::string>& = {});

  /// Read the desired data entry from which a Frame can be constructed.
  ///
  /// @note the category name has to be "events" in this case, as only that
  /// category is available for legacy files. Also the collections to read
  /// argument will be ignored.
  ///
  /// @returns FrameData from which a podio::Frame can be constructed if the
  ///          desired entry exists. Otherwise a nullptr
  std::unique_ptr<podio::ROOTFrameData> readEntry(const std::string&, const unsigned entry,
                                                  const std::vector<std::string>& = {});

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

  /// Get the names of all the available Frame categories in the current file(s).
  ///
  /// @returns The names of the available categories from the file
  std::vector<std::string_view> getAvailableCategories() const;

private:
  std::pair<TTree*, unsigned> getLocalTreeAndEntry(const std::string& treename);

  void createCollectionBranches(const std::vector<root_utils::CollectionWriteInfo>& collInfo);

  podio::GenericParameters readEventMetaData();

  podio::CollectionReadBuffers getCollectionBuffers(const std::pair<std::string, detail::CollectionInfo>& collInfo);

  std::unique_ptr<podio::ROOTFrameData> readEntry();

  // cache the necessary information to more quickly construct and read each
  // collection after it has been read the very first time
  std::vector<std::pair<std::string, detail::CollectionInfo>> m_storedClasses{};

  std::shared_ptr<CollectionIDTable> m_table{nullptr};
  std::unique_ptr<TChain> m_chain{nullptr};
  unsigned m_eventNumber{0};

  // Similar to writing we cache the branches that belong to each collection
  // in order to not having to look them up every event. However, for the
  // reader we cannot guarantee a fixed order of collections as they are read
  // on demand. Hence, we give each collection an index the first time it is
  // read and we start caching the branches.
  std::vector<root_utils::CollectionBranches> m_collectionBranches{};

  podio::version::Version m_fileVersion{0, 0, 0};

  /// The **only** category name that is available from legacy files
  constexpr static auto m_categoryName = "events";
};

} // namespace podio

#endif // PODIO_ROOTLEGACYREADER_H
