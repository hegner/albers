# Examples for Supported Interface

The following snippets show the support of PODIO for the different use cases.
 The event `store` used in the examples is just an example implementation, and has to be replaced with the store used in the framework of your choice.

### Object Ownership

Every data created is either explicitly owned by collections or automatically garbage-collected. There is no need for any `new` or `delete` call on user side.

### Object Creation and Storage

Objects and collections can be created via factories, which ensure proper object ownership:

```cpp
    auto& hits = store.create<HitCollection>("hits")
    auto hit1 = hits.create(1.4,2.4,3.7,4.2);  // init with values
    auto hit2 = hits.create(); // default-construct object
    hit2.energy(42.23);
```

In addition, individual objects can be created in the free. If they aren't attached to a collection, they are automatically garbage-collected:

```cpp
    auto hit1 = Hit();
    auto hit2 = Hit();
    ...
    hits.push_back(hit1);
    ...
    <automatic deletion of hit2>
```

In this respect all objects behave like objects in Python.

### Object References

The library supports the creation of one-to-many relations:

```cpp
    auto& hits = store.create<HitCollection>("hits");
    auto hit1 = hits.create();
    auto hit2 = hits.create();
    auto& clusters = store.create<ClusterCollection>("clusters");
    auto cluster = clusters.create();
    cluster.addHit(hit1);
    cluster.addHit(hit2);
```

The references can be accessed via iterators on the referencing objects

```cpp
    for (auto i = cluster.Hits_begin(), \
         end = cluster.Hits_end(); i!=end; ++i){
      std::cout << i->energy() << std::endl;
    }
```

or via direct accessors

```cpp
    auto size = cluster.Hits_size();
    auto hit  = cluster.Hits(<aNumber>);
```

If asking for an entry outside bounds, a std::out_of_range exception is thrown.


### Looping through Collections
Looping through collections is supported in several ways.

Via iterators:

```cpp
    for(auto i = hits.begin(), end = hits.end(); i != end; ++i) {
      std::cout << i->energy() << std::endl;
    }
```

Via direct object access:

```cpp
    for(int i = 0, end = hits.size(), i != end, ++i){
      std::cout << hit[i].energy() << std::endl;
    }
```

Via ranged for loops:

```cpp
    for(auto hit: hits) {
      std::cout << hit.energy() << std::endl;
    }
```

Via `std::for_each`:

```cpp
    std::for_each(hits.begin(), hits.end(), [](const auto& hit) {
      std::cout << hit.energy() << std::endl;
    });
```

Via C++20 ranges and views:

```cpp
    auto is_electron = [](const auto& p){ return p.PDG() == 11; };
    for (auto hit: hits | std::views::filter(is_electron)) {
      std::cout << hit.energy() << std::endl;
    }
```

Note, however, that writing to collections from iterators is not expected to work:
```cpp
    std::fill(hits.begin(), hits.end(), Hit()); // bad! iterator lvalues do not modify collection!
```

### Support for Notebook-Pattern

The `notebook pattern` uses the assumption that it is better to create a small
copy of only the data that are needed for a particular calculation. This
pattern is supported by providing access like

```cpp
    auto x_array = hits.x<10>(); // returning values of
    auto y_array = hits.y<10>(); // the first 10 elements
```

The resulting `std::array` can then be used in (auto-)vectorizable code.
If less objects than requested are contained in the collection, the remaining entries are default initialized.

### EventStore functionality

The event store contained in the package is for *educational* purposes and kept very minimal. It has two main methods:

```cpp
    /// create a new collection
    template<typename T>
    T& create(const std::string& name);

    /// access a collection.
    template<typename T>
    const T& get(const std::string& name);
```

Please note that a `put` method for collections is not foreseen.

### Object Retrieval

Collections can be retrieved explicitly:

```cpp
    auto& hits = store.get<HitCollection>("hits");
    if (hits.isValid()) { ... }
```

Or implicitly when following an object reference. In both cases the access to data that has been retrieved is `const`.


### User defined Meta Data

Sometimes it is necessary or useful to store additional data that is not directly foreseen in the EDM.
This could be configuration parameters of simulation jobs, or parameter descriptions like cell-ID encoding etc. PODIO currently allows to store such meta data in terms of a `GenericParameters` class that
holds an arbitrary number of named parameters of type `int, float, string` or vectors if these.
Meta data can be stored and retrieved from the `EventStore` for runs, collections and events via
the three methods:
```cpp
virtual GenericParameters& EventStore::getRunMetaData(int runID);
virtual GenericParameters& EventStore::getEventMetaData();
virtual GenericParameters& EventStore::getCollectionMetaData(int colID);
```

- example for writing event data:
```cpp
auto& evtMD = store.getEventMetaData() ;
evtMD.setValue( "UserEventWeight" , (float) 100.*i ) ;
```
- example for reading event data:
```cpp
auto& evtMD = store.getEventMetaData() ;
float evtWeight = evtMD.getFloatVal( "UserEventWeight" ) ;

```

- example for writing collection meta data:

```cpp
auto& hits = store.create<ExampleHitCollection>("hits");
// ...
auto& colMD = store.getCollectionMetaData( hits.getID() );
colMD.setValue("CellIDEncodingString","system:8,barrel:3,layer:6,slice:5,x:-16,y:-16");
```

- example for reading collection meta data

```cpp
auto colMD = store.getCollectionMetaData( hits.getID() );
std::string es = colMD.getStringVal("CellIDEncodingString") ;
```


#### Python Interface

The class `EventStore` provides all the necessary (read) access to event files. It can be used as follows:

```python
    from EventStore import EventStore
    store = EventStore(<list of files>)
    for event in store:
      hits = store.get("hits")
      for hit in hits:
        # ...
```
