
// podio specific includes
#include "podio/IReader.h"
#include "podio/CollectionBase.h"
#include "podio/EventStore.h"

namespace podio {

  EventStore::EventStore() :
    m_reader(nullptr),
    m_table(new CollectionIDTable()),
    m_runparameters(new std::map<std::string, std::string>())
  {
    m_cachedCollections.resize(128) ; // allow for a sufficiently large initial number of collections
  }

  EventStore::~EventStore(){
    for (auto& coll : m_collections){
      delete coll.second;
    }
    for (auto& coll : m_failedRetrieves){
      delete coll;
    }
  }


  bool EventStore::get(int id, CollectionBase*& collection) const{
    // see if we have a cached collection
    if( ( collection = getFast(id) )  != nullptr )
      return true ;

    auto val = m_retrievedIDs.insert(id);
    bool success = false;
    if (val.second == true){
      // collection not yet retrieved in recursive-call
      auto name = m_table->name(id);
      success = doGet(name, collection,true);
      if( collection != nullptr ){  // cache the collection for faster retreaval later
	if( m_cachedCollections.size() < id + 1 )
	  m_cachedCollections.resize( id+1 ) ;
	m_cachedCollections[id] = collection ;
      }
    } else {
      // collection already requested in recursive call
      // do not set the references to break collection dependency-cycle
      auto name = m_table->name(id);
      success = doGet(name, collection,false);
    }
    //fg: the set should only be cleared at the end of event (in clear() ) ...
    //    m_retrievedIDs.erase(id);
    return success;
  }

  void EventStore::registerCollection(const std::string& name, podio::CollectionBase* coll) {
    m_collections.push_back({name,coll});
    auto id = m_table->add(name);
    coll->setID(id);
  }

  bool EventStore::isValid() const {
    return m_reader->isValid();
  }

  bool EventStore::doGet(const std::string& name, CollectionBase*& collection, bool setReferences) const {
    auto result = std::find_if(begin(m_collections), end(m_collections),
                               [name](const CollPair& item)->bool { return name==item.first; }
    );
    if (result != end(m_collections)){
      auto tmp = result->second;
      if (tmp != nullptr){
        collection = tmp;
        return true;
      }
    } else if (m_reader != nullptr) {
      auto tmp = m_reader->readCollection(name);
      if (setReferences == true) {
        if (tmp != nullptr){
          tmp->setReferences(this);
          // check again whether collection exists
          // it may have been created on-demand already
          if (collectionRegistered(name) == false) {
            m_collections.emplace_back(std::make_pair(name,tmp));
          }
        }
      }
      collection = tmp;
      if (tmp != nullptr) return true;
    } else {
      return false;
    }
    return false;
  }

  void EventStore::clearCollections(){
    for (auto& coll : m_collections){
      coll.second->clear();
    }
  }

  void EventStore::clear(){
    for (auto& coll : m_collections){
      coll.second->clear();
      delete coll.second;
    }
    for (auto& coll : m_failedRetrieves){
      delete coll;
    }
    clearCaches();
  }

  void EventStore::clearCaches() {
    m_collections.clear();
    m_cachedCollections.clear() ;
    m_cachedCollections.resize(128) ;
    m_retrievedIDs.clear();
    m_failedRetrieves.clear();
  }

  bool EventStore::collectionRegistered(const std::string& name) const {
    auto result = std::find_if(begin(m_collections), end(m_collections),
                               [name](const CollPair& item)->bool { return name==item.first; }
    );
    return (result != end(m_collections));
  }

  void EventStore::setReader(IReader* reader){
    m_reader = reader;
    setCollectionIDTable(reader->getCollectionIDTable());
    m_runparameters = reader->getRunParameters();
  }


} // namespace
