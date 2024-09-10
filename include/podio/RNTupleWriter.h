#ifndef PODIO_RNTUPLEWRITER_H
#define PODIO_RNTUPLEWRITER_H

#include "TFile.h"
#include "podio/SchemaEvolution.h"
#include "podio/utilities/DatamodelRegistryIOHelpers.h"
#include "podio/utilities/RootHelpers.h"
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RVersion.hxx>

#include <cstdint>

namespace ROOT {
namespace Experimental {
  class REntry;
} // namespace Experimental
} // namespace ROOT
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 31, 0)
  #include <ROOT/RNTupleWriter.hxx>
#endif

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace podio {
class Frame;
class GenericParameters;

/// The RNTupleWriter writes podio files into ROOT files using the new RNTuple
/// format.
///
/// Each category gets its own RNTuple. Additionally, there is a podio_metadata
/// RNTuple that contains metadata that is necessary for interpreting the files
/// for reading.
///
/// Files written with the RNTupleWriter can be read with the RNTupleReader.
class RNTupleWriter {
public:
  /// Create a RNTupleWriter to write to a file.
  ///
  /// @note Existing files will be overwritten without warning.
  ///
  /// @param filename The path to the file that will be created.
  RNTupleWriter(const std::string& filename);

  /// RNTupleWriter destructor
  ///
  /// This also takes care of writing all the necessary metadata in order to be
  /// able to read files back again.
  ~RNTupleWriter();

  /// The RNTupleWriter is not copy-able
  RNTupleWriter(const RNTupleWriter&) = delete;
  /// The RNTupleWriter is not copy-able
  RNTupleWriter& operator=(const RNTupleWriter&) = delete;

  /// Store the given frame with the given category.
  ///
  /// This stores all available collections from the Frame.
  ///
  /// @note The contents of the first Frame that is written in this way
  /// determines the contents that will be written for all subsequent Frames.
  ///
  /// @param frame    The Frame to store
  /// @param category The category name under which this Frame should be stored
  void writeFrame(const podio::Frame& frame, const std::string& category);

  /// Store the given Frame with the given category.
  ///
  /// This stores only the desired collections and not the complete frame.
  ///
  /// @note The contents of the first Frame that is written in this way
  /// determines the contents that will be written for all subsequent Frames.
  ///
  /// @param frame        The Frame to store
  /// @param category     The category name under which this Frame should be
  ///                     stored
  /// @param collsToWrite The collection names that should be written
  void writeFrame(const podio::Frame& frame, const std::string& category, const std::vector<std::string>& collsToWrite);

  /// Write the current file, including all the necessary metadata to read it
  /// again.
  ///
  /// @note The destructor will also call this, so letting a RNTupleWriter go out
  /// of scope is also a viable way to write a readable file
  void finish();

  /// Check whether the collsToWrite are consistent with the state of the passed
  /// category.
  ///
  /// @note This will only be a meaningful check if the first Frame of the passed
  /// category has already been written. Also, this check is rather expensive as
  /// it has to effectively do two set differences.
  ///
  ///
  /// @param collsToWrite The collection names that should be checked for
  ///                     consistency
  /// @param category     The category name for which consistency should be
  ///                     checked
  ///
  /// @returns two vectors of collection names. The first one contains all the
  /// names that were missing from the collsToWrite but were present in the
  /// category. The second one contains the names that are present in the
  /// collsToWrite only. If both vectors are empty the category and the passed
  /// collsToWrite are consistent.
  std::tuple<std::vector<std::string>, std::vector<std::string>>
  checkConsistency(const std::vector<std::string>& collsToWrite, const std::string& category) const;

private:
  std::unique_ptr<ROOT::Experimental::RNTupleModel>
  createModels(const std::vector<root_utils::StoreCollection>& collections);

  /// Helper struct to group all the necessary information for one category.
  struct CategoryInfo {
    std::unique_ptr<ROOT::Experimental::RNTupleWriter> writer{nullptr}; ///< The RNTupleWriter for this category

    // The following are assumed to run in parallel!
    std::vector<uint32_t> ids{};                  ///< The ids of all collections
    std::vector<std::string> names{};             ///< The names of all collections
    std::vector<std::string> types{};             ///< The types of all collections
    std::vector<short> subsetCollections{};       ///< The flags identifying the subcollections
    std::vector<SchemaVersionT> schemaVersions{}; ///< The schema versions of all collections

    // Storage for the keys & values of all the parameters of this category
    // (resp. at least the current entry)
    root_utils::ParamStorage<int> intParams{};
    root_utils::ParamStorage<float> floatParams{};
    root_utils::ParamStorage<double> doubleParams{};
    root_utils::ParamStorage<std::string> stringParams{};
  };
  CategoryInfo& getCategoryInfo(const std::string& category);

  template <typename T>
  void fillParams(const GenericParameters& params, CategoryInfo& catInfo, ROOT::Experimental::REntry* entry);

  template <typename T>
  root_utils::ParamStorage<T>& getParamStorage(CategoryInfo& catInfo);

  std::unique_ptr<TFile> m_file{};

  DatamodelDefinitionCollector m_datamodelCollector{};

  std::unordered_map<std::string, CategoryInfo> m_categories{};

  bool m_finished{false};
};

} // namespace podio

#endif // PODIO_RNTUPLEWRITER_H
