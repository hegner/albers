#ifndef PODIO_ROOTWRITER_H
#define PODIO_ROOTWRITER_H

#include "podio/CollectionIDTable.h"
#include "podio/utilities/DatamodelRegistryIOHelpers.h"
#include "podio/utilities/RootHelpers.h"

#include "TFile.h"

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

// forward declarations
class TTree;

namespace podio {
class Frame;
class CollectionBase;
class GenericParameters;

/// The ROOTWriter writes podio files into ROOT files using TTrees.
///
/// Each category gets its own TTree. Additionally, there is a podio_metadata
/// TTree that contains metadata that is necessary for interpreting the files
/// for reading.
///
/// Files written with the ROOTWriter can be read with the ROOTReader.
class ROOTWriter {
public:
  /// Create a ROOTWriter to write to a file.
  ///
  /// @note Existing files will be overwritten without warning.
  ///
  /// @param filename The path to the file that will be created.
  ROOTWriter(const std::string& filename);

  /// ROOTWriter destructor
  ///
  /// This also takes care of writing all the necessary metadata to read files back again.
  ~ROOTWriter();

  /// The ROOTWriter is not copy-able
  ROOTWriter(const ROOTWriter&) = delete;
  /// The ROOTWriter is not copy-able
  ROOTWriter& operator=(const ROOTWriter&) = delete;

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
  /// @note The destructor will also call this, so letting a ROOTWriter go out
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
  /// Helper struct to group together all necessary state to write / process a
  /// given category. Created during the first writing of a category
  struct CategoryInfo {
    TTree* tree{nullptr};                                     ///< The TTree to which this category is written
    std::vector<root_utils::CollectionBranches> branches{};   ///< The branches for this category
    std::vector<root_utils::CollectionWriteInfo> collInfo{}; ///< Collection info for this category
    podio::CollectionIDTable idTable{};                       ///< The collection id table for this category
    std::vector<std::string> collsToWrite{};                  ///< The collections to write for this category

    // Storage for the keys & values of all the parameters of this category
    // (resp. at least the current entry)
    root_utils::ParamStorage<int> intParams{};
    root_utils::ParamStorage<float> floatParams{};
    root_utils::ParamStorage<double> doubleParams{};
    root_utils::ParamStorage<std::string> stringParams{};
  };

  /// Initialize the branches for this category
  void initBranches(CategoryInfo& catInfo, const std::vector<root_utils::StoreCollection>& collections,
                    /*const*/ podio::GenericParameters& parameters);

  /// Get the (potentially uninitialized category information for this category)
  CategoryInfo& getCategoryInfo(const std::string& category);

  static void resetBranches(CategoryInfo& categoryInfo, const std::vector<root_utils::StoreCollection>& collections);

  /// Fill the parameter keys and values into the CategoryInfo storage
  static void fillParams(CategoryInfo& catInfo, const GenericParameters& params);

  std::unique_ptr<TFile> m_file{nullptr};                       ///< The storage file
  std::unordered_map<std::string, CategoryInfo> m_categories{}; ///< All categories

  DatamodelDefinitionCollector m_datamodelCollector{};

  bool m_finished{false}; ///< Whether writing has been actually done
};

} // namespace podio

#endif // PODIO_ROOTWRITER_H
