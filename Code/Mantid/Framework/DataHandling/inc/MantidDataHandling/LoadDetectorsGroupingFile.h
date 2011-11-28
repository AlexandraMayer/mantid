#ifndef MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_
#define MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadDetectorsGroupingFile : TODO: DESCRIPTION
    
    @date 2011-11-17

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadDetectorsGroupingFile : public API::Algorithm
  {
  public:
    LoadDetectorsGroupingFile();
    virtual ~LoadDetectorsGroupingFile();
    
    ///
    virtual const std::string name() const { return "LoadDetectorsGroupingFile";};
    /// Algorithm's version for identification
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling";}

   private:
     /// Sets documentation strings for this algorithm
     virtual void initDocs();
     /// Initialise the properties
     void init();
     /// Run the algorithm
     void exec();
     /// Initialize XML parser
     void initializeXMLParser(const std::string & filename);
     /// Parse XML
     void parseXML();
     /// Initialize a Mask Workspace
     void intializeGroupingWorkspace();
     /// Set workspace->group ID map by components
     void setByComponents();
     /// Set workspace->group ID map by detectors (range)
     void setByDetectors();
     /// Convert detector ID combination string to vector of detectors
     void parseDetectorIDs(std::string inputstring, std::vector<detid_t>& detids);
     /// Get attribute value from an XML node
     static std::string getAttributeValueByName(Poco::XML::Node* pNode, std::string attributename, bool& found);
     /// Split and convert string
     void parseRangeText(std::string inputstr, std::vector<int32_t>& singles, std::vector<int32_t>& pairs);

     /// Grouping Workspace
     DataObjects::GroupingWorkspace_sptr mGroupWS;
     /// Instrument name
     std::string mInstrumentName;
     /// XML document loaded
     Poco::XML::Document* pDoc;
     /// Root element of the parsed XML
     Poco::XML::Element* pRootElem;

     /// Data structures to store XML to Group/Detector conversion map
     std::map<int, std::vector<std::string> > mGroupComponentsMap;
     std::map<int, std::vector<detid_t> > mGroupDetectorsMap;



  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_ */
