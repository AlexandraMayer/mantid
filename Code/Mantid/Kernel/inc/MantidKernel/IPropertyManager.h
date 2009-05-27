#ifndef MANTID_KERNEL_IPROPERTYMANAGER_H_
#define MANTID_KERNEL_IPROPERTYMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <map>

#include "MantidKernel/PropertyWithValue.h"

namespace Mantid
{
namespace Kernel
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class Logger;

/** @class IPropertyManager IPropertyManager.h Kernel/IPropertyManager.h

 Interface to PropertyManager

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class PropertyMgr (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 20/11/2007
 @author Roman Tolchenov, Tessella plc
 @date 02/03/2009

 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
class DLLExport IPropertyManager
{
public:
    //IPropertyManager(){}
    virtual ~IPropertyManager(){}

    /// Function to declare properties (i.e. store them)
    virtual void declareProperty(Property *p, const std::string &doc="" ) = 0;

    /** Sets all the declared properties from a string.
        @param propertiesArray A list of name = value pairs separated by a semicolon
     */
    virtual void setProperties(const std::string &propertiesArray) = 0;

    /** Sets property value from a string
        @param name Property name
        @param value New property value
     */
    virtual void setPropertyValue(const std::string &name, const std::string &value) = 0;

    /// Set the value of a property by an index
    virtual void setPropertyOrdinal(const int &index, const std::string &value) = 0;

    /// Checks whether the named property is already in the list of managed property.
    virtual bool existsProperty(const std::string &name) const = 0;

    /// Validates all the properties in the collection
    virtual bool validateProperties() const = 0;

    /// Get the value of a property as a string
    virtual std::string getPropertyValue(const std::string &name) const = 0;

    /// Get the list of managed properties.
    virtual const std::vector< Property*>& getProperties() const = 0;

    /** Templated method to set the value of a PropertyWithValue
    *  @param name The name of the property (case insensitive)
    *  @param value The value to assign to the property
    *  @throw Exception::NotFoundError If the named property is unknown
    *  @throw std::invalid_argument If an attempt is made to assign to a property of different type
    */
    template <typename T>
    void setProperty(const std::string &name, const T value)
    {
        PropertyWithValue<T> *prop = dynamic_cast<PropertyWithValue<T>*>(getPointerToProperty(name));
        if (prop)
        {
          *prop = value;
        }
        else
        {
          throw std::invalid_argument("Attempt to assign to property (" + name + ") of incorrect type");
        }
    }

    /// Specialised version of setProperty template method
    void setProperty(const std::string &name, const char* value)
    {
        this->setPropertyValue(name, std::string(value));
    }

protected:

    /** Add a property of the template type to the list of managed properties
    *  @param name The name to assign to the property
    *  @param value The initial value to assign to the property
    *  @param validator Pointer to the (optional) validator. Ownership will be taken over.
    *  @param doc The (optional) documentation string
    *  @param direction The (optional) direction of the property, in, out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    template <typename T>
    void declareProperty(const std::string &name, T value,
      IValidator<T> *validator = new NullValidator<T>, const std::string &doc="", const unsigned int direction = Direction::Input)
    {
        Property *p = new PropertyWithValue<T>(name, value, validator, direction);
        declareProperty(p, doc);
    }

    /** Add a property of the template type to the list of managed properties
    *  @param name The name to assign to the property
    *  @param value The initial value to assign to the property
    *  @param direction The direction of the property, in, out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    template <typename T>
    void declareProperty(const std::string &name, T value, const unsigned int direction)
    {
        Property *p = new PropertyWithValue<T>(name, value, new NullValidator<T>, direction);
        declareProperty(p);
    }

    /** Specialised version of declareProperty template method to prevent the creation of a
    *  PropertyWithValue of type const char* if an argument in quotes is passed (it will be
    *  converted to a string). The validator, if provided, needs to be a string validator.
    *  @param name The name to assign to the property
    *  @param value The initial value to assign to the property
    *  @param validator Pointer to the (optional) validator. Ownership will be taken over.
    *  @param doc The (optional) documentation string
    *  @param direction The (optional) direction of the property, in, out or inout
    *  @throw Exception::ExistsError if a property with the given name already exists
    *  @throw std::invalid_argument  if the name argument is empty
    */
    void declareProperty( const std::string &name, const char* value,
      IValidator<std::string> *validator = new NullValidator<std::string>, const std::string &doc="", const unsigned int direction = Direction::Input )
    {
        // Simply call templated method, converting character array to a string
        declareProperty(name, std::string(value), validator, doc, direction);
    }

   /** Add a property of string type to the list of managed properties
   *  @param name The name to assign to the property
   *  @param value The initial value to assign to the property
   *  @param direction The direction of the property, in, out or inout
   *  @throw Exception::ExistsError if a property with the given name already exists
   *  @throw std::invalid_argument  if the name argument is empty
   */
  void declareProperty(const std::string &name, const char* value, const unsigned int direction)
  {
      declareProperty(name, std::string(value), new NullValidator<std::string>, "", direction);
  }

  /// Get a property by name
  virtual Property* getPointerToProperty(const std::string &name) const = 0;

  /// Get a property by an index
  virtual Property* getPointerToPropertyOrdinal(const int &index) const = 0;

  /** Templated method to get the value of a property
   *  @param name The name of the property (case insensitive)
   *  @return The value of the property
   *  @throw std::runtime_error If an attempt is made to assign a property to a different type
   *  @throw Exception::NotFoundError If the property requested does not exist
   */
  template<typename T>
  T getValue(const std::string &name) const;

  /// Utility class that enables the getProperty() method to effectively be templated on the return type
  struct TypedValue
  {
    /// Reference to the containing PropertyManager
    const IPropertyManager& pm;
    /// The name of the property desired
    const std::string prop;

    /// Constructor
    TypedValue(const IPropertyManager& p, const std::string &name) : pm(p), prop(name) {}

    /// Templated cast operator so that a TypedValue can be cast to what is actually wanted
    template<typename T>
    operator T() { return pm.getValue<T>(prop); }
  };

public:
  /// Get the value of a property
  virtual TypedValue getProperty(const std::string &name) const = 0;

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IPROPERTYMANAGER_H_*/
