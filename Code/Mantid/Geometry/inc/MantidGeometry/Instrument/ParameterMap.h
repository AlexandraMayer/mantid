#ifndef PARAMETERMAP_H_ 
#define PARAMETERMAP_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Cache.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/IComponent.h"

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

#include <vector>
#include <typeinfo>

namespace Mantid
{
namespace Geometry
{

//class IComponent;

/** @class ParameterMap ParameterMap.h


    ParameterMap class. Holds the parameters of modified (parametrized) instrument
    components. ParameterMap has a number of 'add' methods for adding parameters of
    different types. 

    @author Roman Tolchenov, Tessella Support Services plc
    @date 2/12/2008

 Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport ParameterMap
{
public:
#ifndef HAS_UNORDERED_MAP_H
  /// Parameter map typedef
  typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> > pmap;
  /// Parameter map iterator typedef
  typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> >::iterator pmap_it;
  /// Parameter map iterator typedef
  typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> >::const_iterator pmap_cit;
#else
  /// Parameter map typedef
  typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> > pmap;
  /// Parameter map iterator typedef
  typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> >::iterator pmap_it;
  /// Parameter map iterator typedef
  typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> >::const_iterator pmap_cit;
#endif
    ///Constructor
    ParameterMap(){}
    ///virtual destructor
    virtual ~ParameterMap(){}
    /// Return the size of the map
    int size() const {return static_cast<int>(m_map.size());}
    ///Copy Contructor
    ParameterMap(const ParameterMap& copy);
    /// Clears the map
    void clear()
    {
      m_map.clear();
      clearCache();
    }

    /// Method for adding a parameter providing its value as a string
    void add(const std::string& type,const IComponent* comp,const std::string& name, const std::string& value)
    {
      boost::shared_ptr<Parameter> param = ParameterFactory::create(type,name);
      param->fromString(value);
      std::pair<pmap_it,pmap_it> range = m_map.equal_range(comp->getComponentID());
      for(pmap_it it=range.first;it!=range.second;++it)
      {
        if (it->second->name() == name)
        {
          it->second = param;
          return;
        }
      }
      m_map.insert(std::make_pair(comp->getComponentID(),param));
    }

    /// Method for adding a parameter providing its value of a particular type
    template<class T>
    void add(const std::string& type,const IComponent* comp,const std::string& name, const T& value)
    {
      boost::shared_ptr<Parameter> param = ParameterFactory::create(type,name);
      ParameterType<T> *paramT = dynamic_cast<ParameterType<T> *>(param.get());
      if (!paramT)
      {
        reportError("Error in adding parameter: incompatible types");
        throw std::runtime_error("Error in adding parameter: incompatible types");
      }
      paramT->setValue(value);
      std::pair<pmap_it,pmap_it> range = m_map.equal_range(comp->getComponentID());
      for(pmap_it it=range.first;it!=range.second;++it)
      {
        if (it->second->name() == name)
        {
          it->second = param;
          return;
        }
      }
      m_map.insert(std::make_pair(comp->getComponentID(),param));
    }

    
    /// Create or adjust "pos" parameter for a component
    void addPositionCoordinate(const IComponent* comp,const std::string& name, const double value);

    /// Create or adjust "rot" parameter for a component
    void addRotationParam(const IComponent* comp,const std::string& name, const double deg);

    // Concrete parameter adding methods.
    /**  Adds a double value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a string
     */
    void addDouble(const IComponent* comp,const std::string& name, const std::string& value){add("double",comp,name,value);}
    /**  Adds a double value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a double
     */
    void addDouble(const IComponent* comp,const std::string& name, double value){add("double",comp,name,value);}

    /**  Adds an int value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a string
     */
    void addInt(const IComponent* comp,const std::string& name, const std::string& value){add("int",comp,name,value);}
    /**  Adds an int value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as an int
     */
    void addInt(const IComponent* comp,const std::string& name, int value){add("int",comp,name,value);}

    /**  Adds a bool value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a string
     */
    void addBool(const IComponent* comp,const std::string& name, const std::string& value){add("bool",comp,name,value);}
    /**  Adds a bool value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a bool
     */
    void addBool(const IComponent* comp,const std::string& name, bool value){add("bool",comp,name,value);}

    /**  Adds a std::string value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value
     */
    void addString(const IComponent* comp,const std::string& name, const std::string& value){add<std::string>("string",comp,name,value);}

    /**  Adds a V3D value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a string
     */
    void addV3D(const IComponent* comp,const std::string& name, const std::string& value)
    {
      add("V3D",comp,name,value);
      clearCache();
    }
    /**  Adds a V3D value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a V3D
     */
    void addV3D(const IComponent* comp,const std::string& name, const V3D& value)
    {
      add("V3D",comp,name,value);
      clearCache();
    }

    /**  Adds a Quat value to the parameter map.
         @param comp Component to which the new parameter is related
         @param name Name for the new parameter
         @param value Parameter value as a Quat
     */
    void addQuat(const IComponent* comp,const std::string& name, const Quat& value)
    {
      add("Quat",comp,name,value);
      clearCache();
    }

    /**  Return the value of a parameter as a string
         @param comp Component to which parameter is related
         @param name Parameter name
     */
    std::string getString(const IComponent* comp,const std::string& name);

    /**  Returns a string parameter as vector's first element if exists and an empty vector if it doesn't
         @param compName Component name
         @param name Parameter name
     */
    std::vector<std::string> getString(const std::string& compName,const std::string& name)const {return getType<std::string>(compName,name);}

    /**  Get the shared pointer to the parameter with name \a name belonging to component \a comp.
         @param comp Component
         @param name Parameter name
     */
    boost::shared_ptr<Parameter> get(const IComponent* comp,const std::string& name)const;

    /**  Same as get() but look up recursively to see if can find param in all parents of comp.
         @param comp Component
         @param name Parameter name
         @param type Parameter type, i.e. is it a fitting parameter or something else
     */
    boost::shared_ptr<Parameter> getRecursive(const IComponent* comp,const std::string& name,const std::string& type)const;

    /** Get the values of a given parameter of all the components that have the name: compName
     *  @tparam The parameter type
     *  @param compName The name of the component
     *  @param name The name of the parameter
     */
    template<class T>
    std::vector<T> getType(const std::string& compName,const std::string& name)const
    {
      std::vector<T> retval;

      pmap_cit it;
      for (it = m_map.begin(); it != m_map.end(); ++it)
      {
        if ( compName.compare(((const IComponent*)(*it).first)->getName()) == 0 )  
        {
          boost::shared_ptr<Parameter> param = get((const IComponent*)(*it).first,name);
          if (param)
            retval.push_back( param->value<T>() );
        }
      }
      return retval;
    }

    /**  Returns a double parameter as vector's first element if exists and an empty vector if it doesn't
         @param compName Component name
         @param name Parameter name
     */
    std::vector<double> getDouble(const std::string& compName,const std::string& name)const{return getType<double>(compName,name);}

    /**  Returns a V3D parameter as vector's first element if exists and an empty vector if it doesn't
         @param compName Component name
         @param name Parameter name
     */
    std::vector<V3D> getV3D(const std::string& compName,const std::string& name)const{return getType<V3D>(compName,name);}

    /// Returns a vector with all parameter names for component comp
    std::vector<std::string> nameList(const IComponent* comp)const;

    /// Returns a string with all component names, parameter names and values
    std::string asString()const;

    ///Clears the location and roatation caches
    void clearCache()
    {
      m_cacheLocMap.clear();
      m_cacheRotMap.clear();
    }
 
    ///Sets a cached location on the location cache
    void setCachedLocation(const IComponent* comp, V3D& location) const;
    
    ///Attempts to retreive a location from the location cache
    bool getCachedLocation(const IComponent* comp, V3D& location) const;

    ///Sets a cached rotation on the rotation cache
    void setCachedRotation(const IComponent* comp, Quat& rotation) const;
    
    ///Attempts to retreive a rotation from the rotation cache
    bool getCachedRotation(const IComponent* comp, Quat& rotation) const;

private:
  ///Assignment operator
  ParameterMap& operator=(const ParameterMap& rhs);
  /// report an error
  void reportError(const std::string& str);
  /// internal parameter map instance
  pmap m_map;

  /// internal cache map instance for cached postition values
  mutable Kernel::Cache<const ComponentID,V3D > m_cacheLocMap;
  /// internal cache map instance for cached rotation values
  mutable Kernel::Cache<const ComponentID,Quat > m_cacheRotMap;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // Namespace Geometry

} // Namespace Mantid

#endif /*PARAMETERMAP_H_*/
