// -*- C++ -*-
#ifndef GenericParameters_H
#define GenericParameters_H 1

#include <map>
#include <vector>
#include <string>

namespace podio {

  typedef std::vector<int> IntVec ;
  typedef std::vector<float> FloatVec ;
  typedef std::vector<std::string> StringVec ;
  typedef std::map< std::string, IntVec >    IntMap ;
  typedef std::map< std::string, FloatVec >  FloatMap ;
  typedef std::map< std::string, StringVec > StringMap ;
  

  /** GenericParameters objects allow to store generic named parameters of type
   *  int, float and string or vectors of these types. 
   *  They can be used  to store (user) meta data that is 
   *  run, event or collection dependent. 
   *  (based on lcio::LCParameters)
   * 
   * @author F. Gaede, DESY 
   * @date Apr 2020
   */
  
  class GenericParameters {

  public: 
    
    GenericParameters() = default; 
    /// Destructor.
    virtual ~GenericParameters() = default;
    
    /** Returns the first integer value for the given key.
     */
    virtual int getIntVal(const std::string & key) const  ;
    
    /** Returns the first float value for the given key.
     */
    virtual float getFloatVal(const std::string & key) const ;
    
    /** Returns the first string value for the given key.
     */
    virtual const std::string & getStringVal(const std::string & key) const ;
    
    /** Adds all integer values for the given key to values.
     *  Returns a reference to values for convenience.
     */
    virtual IntVec & getIntVals(const std::string & key, IntVec & values) const ;
    
    /** Adds all float values for the given key to values.
     *  Returns a reference to values for convenience.
     */
    virtual FloatVec & getFloatVals(const std::string & key, FloatVec & values) const ;
    
    /** Adds all float values for the given key to values.
     *  Returns a reference to values for convenience.
     */
    virtual  StringVec & getStringVals(const std::string & key, StringVec & values) const ;
    
    /** Returns a list of all keys of integer parameters.
     */
    virtual const StringVec & getIntKeys( StringVec & keys) const  ;

    /** Returns a list of all keys of float parameters.
     */
    virtual const StringVec & getFloatKeys(StringVec & keys)  const ;

    /** Returns a list of all keys of string parameters.
     */
    virtual const StringVec & getStringKeys(StringVec & keys)  const ;
    
    /** The number of integer values stored for this key.
     */ 
    virtual int getNInt(const std::string & key) const ;
    
    /** The number of float values stored for this key.
     */ 
    virtual int getNFloat(const std::string & key) const ;
    
    /** The number of string values stored for this key.
     */ 
    virtual int getNString(const std::string & key) const ;
    
    /** Set integer value for the given key.
     */
    virtual void setValue(const std::string & key, int value) ;

    /** Set float value for the given key.
     */
    virtual void setValue(const std::string & key, float value) ;

    /** Set string value for the given key.
     */
    virtual void setValue(const std::string & key, const std::string & value) ;

    /** Set integer values for the given key.
     */
    virtual void setValues(const std::string & key, const IntVec & values);

    /** Set float values for the given key.
     */
    virtual void setValues(const std::string & key, const FloatVec & values);

    /** Set string values for the given key.
     */
    virtual void setValues(const std::string & key, const StringVec & values);

    /// erase all elements
    void clear() {
      _intMap.clear();
      _floatMap.clear();
      _stringMap.clear();
    }

  protected:

    mutable IntMap _intMap{} ;
    mutable FloatMap _floatMap{} ;
    mutable StringMap _stringMap{} ;
    
  }; // class
} // namespace podio
#endif
