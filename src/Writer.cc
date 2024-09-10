#include "podio/Writer.h"

#include "podio/ROOTWriter.h"
#if PODIO_ENABLE_RNTUPLE
  #include "podio/RNTupleWriter.h"
#endif
#if PODIO_ENABLE_SIO
  #include "podio/SIOWriter.h"
#endif

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <utility>

namespace podio {

Writer makeWriter(const std::string& filename, const std::string& type) {

  auto endsWith = [](const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
  };

  auto lower = [](std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
  };

  if ((type == "default" && endsWith(filename, ".root")) || lower(type) == "root") {
    return Writer{std::make_unique<ROOTWriter>(filename)};
  } else if (lower(type) == "rntuple") {
#if PODIO_ENABLE_RNTUPLE
    return Writer{std::make_unique<RNTupleWriter>(filename)};
#else
    throw std::runtime_error("ROOT RNTuple writer not available. Please recompile with ROOT RNTuple support.");
#endif
  } else if (endsWith(filename, ".sio")) {
#if PODIO_ENABLE_SIO
    return Writer{std::make_unique<SIOWriter>(filename)};
#else
    throw std::runtime_error("SIO writer not available. Please recompile with SIO support.");
#endif
  }
  throw std::runtime_error("Unknown file type for file " + filename + " with type " + type);
}

} // namespace podio
