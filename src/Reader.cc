#include "podio/Reader.h"

#include "podio/ROOTReader.h"

#include <TCollection.h>
#include <TList.h>
#include <TObject.h>
#if PODIO_ENABLE_RNTUPLE
  #include "podio/RNTupleReader.h"
#endif
#if PODIO_ENABLE_SIO
  #include "podio/SIOReader.h"
#endif

#include "TFile.h"
#include "TKey.h"

#include <memory>
#include <utility>

namespace podio {

template <typename T>
Reader::Reader(std::unique_ptr<T> reader) : m_self(std::make_unique<ReaderModel<T>>(std::move(reader))) {
}

Reader makeReader(const std::string& filename) {
  return makeReader(std::vector<std::string>{filename});
}

Reader makeReader(const std::vector<std::string>& filenames) {

  if (filenames.empty()) {
    throw std::runtime_error("No files given to create a Podio Reader");
  }

  auto suffix = filenames[0].substr(filenames[0].find_last_of(".") + 1);
  for (size_t i = 1; i < filenames.size(); ++i) {
    if (filenames[i].substr(filenames[i].find_last_of(".") + 1) != suffix) {
      throw std::runtime_error("All files must have the same extension");
    }
  }

  if (suffix == "root") {
    TFile* file = TFile::Open(filenames[0].c_str());
    bool hasRNTuple = false;

    if (!file) {
      throw std::runtime_error("Could not open file: " + filenames[0]);
    }

    for (auto key : *file->GetListOfKeys()) {
      auto tkey = dynamic_cast<TKey*>(key);

      if (tkey && std::string(tkey->GetClassName()) == "ROOT::Experimental::RNTuple") {
        hasRNTuple = true;
        break;
      }
    }
    if (hasRNTuple) {
#if PODIO_ENABLE_RNTUPLE
      auto actualReader = std::make_unique<RNTupleReader>();
      actualReader->openFiles(filenames);
      Reader reader{std::move(actualReader)};
      return reader;
#else
      throw std::runtime_error("ROOT RNTuple reader not available. Please recompile with ROOT RNTuple support.");
#endif
    } else {
      auto actualReader = std::make_unique<ROOTReader>();
      actualReader->openFiles(filenames);
      Reader reader{std::move(actualReader)};
      return reader;
    }
  } else if (suffix == "sio") {
#if PODIO_ENABLE_SIO
    if (filenames.size() > 1) {
      throw std::runtime_error("The SIO reader does currently not support reading multiple files");
    }
    auto actualReader = std::make_unique<SIOReader>();
    actualReader->openFile(filenames[0]);
    Reader reader{std::move(actualReader)};
    return reader;
#else
    throw std::runtime_error("SIO reader not available. Please recompile with SIO support.");
#endif
  }

  throw std::runtime_error("Unknown file extension: " + suffix);
}

} // namespace podio
