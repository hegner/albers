#ifndef PODIO_SIOBLOCKUSERDATA_H
#define PODIO_SIOBLOCKUSERDATA_H

#include "podio/CollectionBufferFactory.h"
#include "podio/CollectionBuffers.h"
#include "podio/SIOBlock.h"
#include "podio/UserDataCollection.h"
#include <sio/api.h>
#include <sio/io_device.h>
#include <sio/version.h>

#include <algorithm>
#include <string>
#include <typeindex>

namespace podio {
namespace detail {

  /// helper function to get valid sio block names
  template <typename BasicType, typename = podio::EnableIfSupportedUserType<BasicType>>
  inline std::string sio_name() {
    std::string s = podio::userDataTypeName<BasicType>();
    std::replace(s.begin(), s.end(), ' ', '_');
    return s;
  }
} // namespace detail

template <typename BasicType, typename = EnableIfSupportedUserType<BasicType>>
class SIOBlockUserData : public podio::SIOBlock {
public:
  SIOBlockUserData() :
      SIOBlock(detail::sio_name<BasicType>(),
               sio::version::encode_version(UserDataCollection<BasicType>::schemaVersion, 0)) {

    podio::SIOBlockFactory::instance().registerBlockForCollection(podio::userDataTypeName<BasicType>(), this);
  }

  SIOBlockUserData(const std::string& name) :
      SIOBlock(name, sio::version::encode_version(UserDataCollection<BasicType>::schemaVersion, 0)) {
  }

  void read(sio::read_device& device, sio::version_type version) override {
    const auto& bufferFactory = podio::CollectionBufferFactory::instance();
    m_buffers =
        bufferFactory
            .createBuffers(podio::userDataCollTypeName<BasicType>(), sio::version::major_version(version), false)
            .value();

    auto* dataVec = new std::vector<BasicType>();
    unsigned size(0);
    device.data(size);
    dataVec->resize(size);
    podio::handlePODDataSIO(device, &(*dataVec)[0], size);
    m_buffers.data = dataVec;
  }

  void write(sio::write_device& device) override {
    auto* dataVec = podio::CollectionWriteBuffers::asVector<BasicType>(m_buffers.data);
    unsigned size = dataVec->size();
    device.data(size);
    podio::handlePODDataSIO(device, &(*dataVec)[0], size);
  }

  SIOBlock* create(const std::string& name) const override {
    return new SIOBlockUserData(name);
  }

private:
};

} // namespace podio
#endif
