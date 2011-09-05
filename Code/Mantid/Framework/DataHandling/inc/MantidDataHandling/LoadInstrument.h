#ifndef MANTID_DATAHANDLING_LOADINSTRUMENT_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
  class Element;
}}
/// @endcond

namespace Mantid
{
  namespace Kernel
  {
    class V3D;
  }
  namespace API
  {
    class MatrixWorkspace;
  }
  namespace Geometry
  {
    class CompAssembly;
    class Component;
    class Object;
    class ObjComponent;
    class Instrument;
  }

  namespace DataHandling
  {
    /** @class LoadInstrument LoadInstrument.h DataHandling/LoadInstrument.h

    Loads instrument data from a XML instrument description file and adds it
    to a workspace.

    LoadInstrument is an algorithm and as such inherits
    from the Algorithm class and overrides the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the workspace </LI>
    <LI> Filename - The name of the IDF file </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 19/11/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 7/3/2008

    Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
      virtual ~LoadInstrument() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadInstrument";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}

      /// Add/overwrite any parameters specified in instrument with param values specified in <component-link> XML elements
      void setComponentLinks(boost::shared_ptr<Geometry::Instrument>& instrument, Poco::XML::Element* pElem);

    private:
      void initDocs();
      void init();
      void exec();
      /// Check the validity range and add it to the instrument object
      void setValidityRange(const Poco::XML::Element* pRootElem);
      /// Reads the contents of the \<defaults\> element to set member variables,
      void readDefaults(Poco::XML::Element* defaults);

      /// Structure for holding detector IDs
      struct IdList
      {
        /// Used to count the number of detector encounted so far
        int counted;
        /// list of detector IDs
        std::vector<int> vec;
        /// name of idlist
        std::string idname;

        ///Constructor
        IdList() : counted(0) {};

        /// return true if empty
        bool empty() { return vec.empty();};  

        /// reset idlist
        void reset() { counted=0; vec.clear();};  
      };

      /// Method for populating IdList
      void populateIdList(Poco::XML::Element* pElem, IdList& idList);

      std::vector<std::string> buildExcludeList(const Poco::XML::Element* const location);

      /// Add XML element to parent assuming the element contains other component elements
      void appendAssembly(Geometry::ICompAssembly* parent, Poco::XML::Element* pElem, IdList& idList);
      /// Return true if assembly, false if not assembly and throws exception if string not in assembly
      bool isAssembly(std::string) const;

      /// Add XML element to parent assuming the element contains no other component elements
      void appendLeaf(Geometry::ICompAssembly* parent, Poco::XML::Element* pElem, IdList& idList);

      /// Set parameter/logfile info (if any) associated with component
      void setLogfile(const Geometry::IComponent* comp, Poco::XML::Element* pElem, 
                                std::multimap<std::string, boost::shared_ptr<Geometry::XMLlogfile> >& logfileCache);

      /// get name of location element
      std::string getNameOfLocationElement(Poco::XML::Element* pElem);

      /// Parse position of facing element to V3D
      Kernel::V3D parseFacingElementToV3D(Poco::XML::Element* pElem);
      /// Set facing of comp as specified in XML facing element
      void setFacing(Geometry::IComponent* comp, Poco::XML::Element* pElem);
      /// Make the shape defined in 1st argument face the component in the second argument
      void makeXYplaneFaceComponent(Geometry::IComponent* &in, const Geometry::ObjComponent* facing);
      /// Make the shape defined in 1st argument face the position in the second argument
      void makeXYplaneFaceComponent(Geometry::IComponent* &in, const Kernel::V3D& facingPoint);

      /// Reads in or creates the geometry cache ('vtp') file
      void setupGeometryCache();

      /// If appropriate, creates a second instrument containing neutronic detector positions
      void createNeutronicInstrument(const std::string& idsName);

      /// Run the sub-algorithm LoadParameters
      void runLoadParameterFile();

      /** Holds all the xml elements that have a \<parameter\> child element.
       *  Added purely for the purpose of computing speed and is used in setLogFile() for the purpose
       *  of quickly accessing if a component have a parameter/logfile associated with it or not
       *  - instead of using the comparatively slow poco call getElementsByTagName() (or getChildElement)
       */
      std::vector<Poco::XML::Element*> hasParameterElement;
      /// has hasParameterElement been set - used when public method setComponentLinks is used
      bool hasParameterElement_beenSet;
      /** map which holds names of types and whether or not they are categorized as being
       *  assemblies, which means whether the type element contains component elements
       */
      std::map<std::string,bool> isTypeAssembly;
      /// map which maps the type name to a shared pointer to a geometric shape
      std::map<std::string, boost::shared_ptr<Geometry::Object> > mapTypeNameToShape;
      /// Container to hold all detectors and monitors added to the instrument. Used for 'facing' these to component specified under \<defaults\>. NOTE: Seems unused, ever.
      std::vector< Geometry::ObjComponent* > m_facingComponent;
      /// True if defaults->components-are-facing is set in instrument def. file
      bool m_haveDefaultFacing;
      /// Hold default facing position
      Kernel::V3D m_defaultFacing;
      /// map which holds names of types and pointers to these type for fast retrieval in code
      std::map<std::string,Poco::XML::Element*> getTypeElement;

      /// The name and path of the input file
      std::string m_filename;
      /// For convenience added pointer to instrument here
      boost::shared_ptr<Geometry::Instrument> m_instrument;

      /// Flag to indicate whether offsets given in spherical coordinates are to be added to the current
      /// position (true) or are a vector from the current position (false, default)
      bool m_deltaOffsets;

      /// when this const equals 1 it means that angle=degree (default) is set in IDF
      /// otherwise if this const equals 180/pi it means that angle=radian is set in IDF 
      double m_angleConvertConst; 
      
      /// Everything can reference the workspace if it needs to
      boost::shared_ptr<API::MatrixWorkspace> m_workspace;

      bool m_indirectPositions; ///< Flag to indicate whether IDF contains physical & neutronic positions
      /// A map containing the neutronic position for each detector. Used when m_indirectPositions is true.
      std::map<Geometry::IComponent*,Poco::XML::Element*> m_neutronicPos;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENT_H_*/

