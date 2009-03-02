#ifndef MANTID_DATAHANDLING_LOADINSTRUMENT_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
  class Element;
}}
/// @endcond

namespace Mantid
{

namespace Geometry
{
  class CompAssembly;
  class Component;
  class Object;
  class ObjComponent;
  class V3D;
}
namespace API
{
  class Instrument;
}

  namespace DataHandling
  {
    /** @class LoadInstrument LoadInstrument.h DataHandling/LoadInstrument.h

    Loads instrument data from a XML instrument description file and adds it
    to a workspace.

    LoadInstrument is intended to be used as a child algorithm of
    other Loadxxx algorithms, rather than being used directly.
    LoadInstrument is an algorithm and as such inherits
    from the Algorithm class and overrides the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the workspace in which to use as a basis for any data to be added.</LI>
    <LI> Filename - The name of and path to the input RAW file </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 19/11/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 7/3/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    */
    class DLLExport LoadInstrument : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadInstrument();

      /// Destructor
      ~LoadInstrument() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadInstrument";};
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}

    private:
      void init();
      void exec();

      /// Structure for holding detector IDs
      struct IdList
      {
        ///Counted
        int counted;
        ///Vector of the values of the ID list
        std::vector<int> vec;

        ///Constructor
        IdList() : counted(0) {};
      };

      /// Method for populating IdList
      void populateIdList(Poco::XML::Element* pElem, IdList& idList);

      /// Add XML element to parent assuming the element contains other component elements
      void appendAssembly(Geometry::CompAssembly* parent, Poco::XML::Element* pElem, IdList& idList);
      /// Add XML element to parent assuming the element contains other component elements
      void appendAssembly(boost::shared_ptr<Geometry::CompAssembly> parent, Poco::XML::Element* pElem, IdList& idList);

      /// Add XML element to parent assuming the element contains no other component elements
      void appendLeaf(Geometry::CompAssembly* parent, Poco::XML::Element* pElem, IdList& idList);
      /// Add XML element to parent assuming the element contains no other component elements
      void appendLeaf(boost::shared_ptr<Geometry::CompAssembly> parent, Poco::XML::Element* pElem, IdList& idList);

      /// Set location (position) of comp as specified in XML location element
      void setLocation(Geometry::Component* comp, Poco::XML::Element* pElem);

      /// Set parameter/logfile info (if any) associated with component
      void setLogfile(Geometry::Component* comp, Poco::XML::Element* pElem);

      /// Holds all the xml elements that have a <parameter> child element.
      /// Added purely for the purpose of computing speed and is used in setLogFile()
      /// for the purpose of quickly accessing if a component have a parameter/logfile
      /// associated with it or not - instead of using the comparatively slow poco
      /// call getElementsByTagName() (or getChildElement)
      std::vector<Poco::XML::Element*> hasParameterElement;

      /// Get parent component element of location element
      Poco::XML::Element* getParentComponent(Poco::XML::Element* pLocElem);

      /// map which holds names of types and whether or not they are catagorised as being
      /// assembles, which means whether the type element contains component elements
      std::map<std::string,bool> isTypeAssembly;

      /// map which maps the type name to a shared pointer to a geometric shape
      std::map<std::string, boost::shared_ptr<Geometry::Object> > mapTypeNameToShape;

      /// Container to hold all detectors and monitors added to the instrument. Used for
      /// 'facing' these to component specified under <defaults>
      std::vector< Geometry::ObjComponent* > m_facingComponent;

      /// Parse position of facing element to V3D
      Geometry::V3D parseFacingElementToV3D(Poco::XML::Element* pElem);

      /// Set facing of comp as specified in XML facing element
      void setFacing(Geometry::Component* comp, Poco::XML::Element* pElem);

      /// True if defaults->components-are-facing is set in instrument def. file
      bool m_haveDefaultFacing;

      /// Hold default facing position
      Geometry::V3D m_defaultFacing;

      /// Make the shape defined in 1st argument face the component in the second argument
      void makeXYplaneFaceComponent(Geometry::Component* &in, const Geometry::ObjComponent* facing);

      /// Make the shape defined in 1st argument face the position in the second argument
      void makeXYplaneFaceComponent(Geometry::Component* &in, const Geometry::V3D& facingPoint);

      /// Return true if assembly, false if not assembly and throws exception if string not in assembly
      bool isAssembly(std::string);

      /// map which holds names of types and pointers to these type for fast retrievel in code
      std::map<std::string,Poco::XML::Element*> getTypeElement;

      /// The name and path of the input file
      std::string m_filename;

      /// For convenience added pointer to instrument here
      boost::shared_ptr<API::Instrument> m_instrument;

      /// Flag to indicate whether offsets given in spherical coordinates are to be added to the current
      /// position (true) or are a vector from the current position (false, default)
      bool m_deltaOffsets;

      /** Stripped down vector that holds position in terms of spherical coordinates,
       *  Needed when processing instrument definition files that use the 'Ariel format'
       */
      struct SphVec
      {
        ///@cond Exclude from doxygen documentation
        double r,theta,phi;
        SphVec() : r(0.0), theta(0.0), phi(0.0) {}
        SphVec(const double& r, const double& theta, const double& phi) : r(r), theta(theta), phi(phi) {}
        ///@endcond
      };

      /// Map to store positions of parent components in spherical coordinates
      std::map<const Geometry::ComponentID,SphVec> m_tempPosHolder;

      ///static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENT_H_*/

