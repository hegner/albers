#ifndef PODIO_ROOTFRAMEDATA_H
#define PODIO_ROOTFRAMEDATA_H

#include "podio/CollectionBuffers.h"
#include "podio/CollectionIDTable.h"
#include "podio/GenericParameters.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace podio {

class ROOTFrameData {
  using CollIDPtr = std::shared_ptr<const podio::CollectionIDTable>;

public:
  using BufferMap = std::unordered_map<std::string, podio::CollectionReadBuffers>;

  ROOTFrameData() = delete;
  ~ROOTFrameData();
  ROOTFrameData(ROOTFrameData&&) = default;
  ROOTFrameData& operator=(ROOTFrameData&&) = default;
  ROOTFrameData(const ROOTFrameData&) = delete;
  ROOTFrameData& operator=(const ROOTFrameData&) = delete;

  ROOTFrameData(BufferMap&& buffers, CollIDPtr&& idTable, podio::GenericParameters&& params);

  std::optional<podio::CollectionReadBuffers> getCollectionBuffers(const std::string& name);

  podio::CollectionIDTable getIDTable() const;

  std::unique_ptr<podio::GenericParameters> getParameters();

  std::vector<std::string> getAvailableCollections() const;

private:
  // TODO: switch to something more elegant once the basic functionality and
  // interface is better defined
  BufferMap m_buffers{};
  // This is co-owned by each FrameData and the original reader. (for now at least)
  CollIDPtr m_idTable{nullptr};
  podio::GenericParameters m_parameters{};
};

} // namespace podio

#endif // PODIO_ROOTFRAMEDATA_H
