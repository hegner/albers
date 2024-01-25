#include "podio/Writer.h"

#include "podio/ROOTFrameWriter.h"
#if PODIO_ENABLE_RNTUPLE
  #include "podio/RNTupleWriter.h"
#endif
#if PODIO_ENABLE_SIO
  #include "podio/SIOFrameWriter.h"
#endif

#include <memory>

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
    return Writer{std::make_unique<ROOTFrameWriter>(filename)};
  }
  if (lower(type) == "rntuple") {
#if PODIO_ENABLE_RNTUPLE
    return Writer{std::make_unique<RNTupleWriter>(filename)};
#else
    throw std::runtime_error("ROOT RNTuple writer not available. Please recompile with ROOT RNTuple support.");
#endif
  }
  if ((type == "default" && endsWith(filename, ".sio")) || lower(type) == "sio") {
#if PODIO_ENABLE_SIO
    return Writer{std::make_unique<SIOFrameWriter>(filename)};
#else
    throw std::runtime_error("SIO writer not available. Please recompile with SIO support.");
#endif
  }
  throw std::runtime_error("Unknown file type");
}

} // namespace podio
