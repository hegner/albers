#ifndef UserDataSIOBlock_H
#define UserDataSIOBlock_H

#include "podio/SIOBlock.h"
#include "podio/UserDataCollection.h"

#include <sio/api.h>
#include <sio/io_device.h>
#include <sio/version.h>

#include <typeindex>
#include <string>


template <typename BasicType>
class UserDataSIOBlock: public podio::SIOBlock {
public:
  UserDataSIOBlock() :
    SIOBlock( podio::UserDataTypes::instance().sio_name( typeid(BasicType) ),
	      sio::version::encode_version(0, 1)) {

    podio::SIOBlockFactory::instance().registerBlockForCollection(
      podio::UserDataTypes::instance().name( typeid(BasicType) ) , this);
  }

  UserDataSIOBlock(const std::string& name) :
    SIOBlock(name, sio::version::encode_version(0, 1)) {}


  virtual void read(sio::read_device& device, sio::version_type /*version*/) override {
    auto collBuffers = _col->getBuffers();
    if (not _col->isSubsetCollection()) {
      auto* dataVec = collBuffers.dataAsVector<BasicType>();
      unsigned size(0);
      device.data( size );
      dataVec->resize(size);
      podio::handlePODDataSIO(device, &(*dataVec)[0], size);
    }
  }

  virtual void write(sio::write_device& device) override {
    _col->prepareForWrite() ;
    auto collBuffers = _col->getBuffers();
    if (not _col->isSubsetCollection()) {
      auto* dataVec = collBuffers.dataAsVector<BasicType>();
      unsigned size = dataVec->size() ;
      device.data( size ) ;
      podio::handlePODDataSIO( device ,  &(*dataVec)[0], size ) ;
    }
  }

  virtual void createCollection(const bool subsetCollection=false) override{
    setCollection(new podio::UserDataCollection<BasicType>);
    _col->setSubsetCollection(subsetCollection);
  }

  SIOBlock* create(const std::string& name) const override {
    return new UserDataSIOBlock(name);
  }
};


#endif
