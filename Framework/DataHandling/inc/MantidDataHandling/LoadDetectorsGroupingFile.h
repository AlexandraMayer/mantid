#ifndef MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_
#define MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"

#include <fstream>

namespace Poco {
namespace XML {
class Document;
class Element;
class Node;
}
}

namespace Mantid {
namespace DataHandling {

/**
  LoadDetectorsGroupingFile

  Algorithm used to generate a GroupingWorkspace from an .xml or .map file
  containing the
  detectors' grouping information.

  @date 2011-11-17

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LoadDetectorsGroupingFile : public API::Algorithm {
public:
  LoadDetectorsGroupingFile();
  virtual ~LoadDetectorsGroupingFile();

  ///
  virtual const std::string name() const {
    return "LoadDetectorsGroupingFile";
  };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load an XML or Map file, which contains definition of detectors "
           "grouping, to a GroupingWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "DataHandling\\Grouping;Transforms\\Grouping";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// Initialize XML parser
  void initializeXMLParser(const std::string &filename);
  /// Parse XML
  void parseXML();
  /// Initialize a Mask Workspace
  void intializeGroupingWorkspace();
  /// Set workspace->group ID map by components
  void setByComponents();
  /// Set workspace->group ID map by detectors (range)
  void setByDetectors();
  /// Set workspace index/group ID by spectraum ID
  void setBySpectrumIDs();
  /// Convert detector ID combination string to vector of detectors
  void parseDetectorIDs(std::string inputstring, std::vector<detid_t> &detids);
  /// Convert spectrum IDs combintation string to vector of spectrum ids
  void parseSpectrumIDs(std::string inputstring, std::vector<int> &specids);
  /// Get attribute value from an XML node
  static std::string getAttributeValueByName(Poco::XML::Node *pNode,
                                             std::string attributename,
                                             bool &found);
  /// Split and convert string
  void parseRangeText(std::string inputstr, std::vector<int32_t> &singles,
                      std::vector<int32_t> &pairs);
  /// Generate a GroupingWorkspace w/o instrument
  void generateNoInstrumentGroupWorkspace();

  /// Grouping Workspace
  DataObjects::GroupingWorkspace_sptr m_groupWS;

  /// Instrument to use if given by user
  Geometry::Instrument_const_sptr m_instrument;

  /// XML document loaded
  Poco::XML::Document *m_pDoc;
  /// Root element of the parsed XML
  Poco::XML::Element *m_pRootElem;

  /// Data structures to store XML to Group/Detector conversion map
  std::map<int, std::vector<std::string>> m_groupComponentsMap;
  std::map<int, std::vector<detid_t>> m_groupDetectorsMap;
  std::map<int, std::vector<int>> m_groupSpectraMap;
};

class DLLExport LoadGroupXMLFile {
public:
  LoadGroupXMLFile();
  ~LoadGroupXMLFile();

  void loadXMLFile(std::string xmlfilename);
  void setDefaultStartingGroupID(int startgroupid) {
    m_startGroupID = startgroupid;
  }

  std::string getInstrumentName() { return m_instrumentName; }
  bool isGivenInstrumentName() { return m_userGiveInstrument; }

  std::string getDate() { return m_date; }
  bool isGivenDate() { return m_userGiveDate; }

  std::string getDescription() { return m_description; }
  bool isGivenDescription() { return m_userGiveDescription; }

  /// Data structures to store XML to Group/Detector conversion map
  std::map<int, std::vector<std::string>> getGroupComponentsMap() {
    return m_groupComponentsMap;
  }
  std::map<int, std::vector<detid_t>> getGroupDetectorsMap() {
    return m_groupDetectorsMap;
  }
  std::map<int, std::vector<int>> getGroupSpectraMap() {
    return m_groupSpectraMap;
  }

  std::map<int, std::string> getGroupNamesMap() { return m_groupNamesMap; }

private:
  /// Instrument name
  std::string m_instrumentName;
  /// User-define instrument name
  bool m_userGiveInstrument;

  /// Date in ISO 8601 for which this grouping is relevant
  std::string m_date;
  /// Whether date is given by user
  bool m_userGiveDate;

  /// Grouping description. Empty if not specified.
  std::string m_description;
  /// Whether description is given by user
  bool m_userGiveDescription;

  /// XML document loaded
  Poco::XML::Document *m_pDoc;
  /// Root element of the parsed XML
  Poco::XML::Element *m_pRootElem;
  /// Data structures to store XML to Group/Detector conversion map
  std::map<int, std::vector<std::string>> m_groupComponentsMap;
  std::map<int, std::vector<detid_t>> m_groupDetectorsMap;
  std::map<int, std::vector<int>> m_groupSpectraMap;
  int m_startGroupID;

  /// Map of group names
  std::map<int, std::string> m_groupNamesMap;

  /// Initialize XML parser
  void initializeXMLParser(const std::string &filename);
  /// Parse XML
  void parseXML();
  /// Get attribute value from an XML node
  static std::string getAttributeValueByName(Poco::XML::Node *pNode,
                                             std::string attributename,
                                             bool &found);
};

/**
 * Class used to load a grouping information from .map file.
 *
 * @author Arturs Bekasovs
 * @date 21/08/2013
 */
class DLLExport LoadGroupMapFile {
public:
  /// Constructor. Opens a file.
  LoadGroupMapFile(const std::string &fileName, Kernel::Logger &log);

  /// Desctructor. Closes a file.
  ~LoadGroupMapFile();

  /// Parses grouping information from .map file
  void parseFile();

  /// Return the map parsed from file. Should only be called after the file is
  /// parsed,
  /// otherwise a map will always be empty.
  std::map<int, std::vector<int>> getGroupSpectraMap() {
    return m_groupSpectraMap;
  }

private:
  /// Skips all the empty lines and comment lines, and returns next line with
  /// real data
  bool nextDataLine(std::string &line);

  /// The name of the file being parsed
  const std::string m_fileName;

  /// Logger used
  Kernel::Logger &m_log;

  /// group_id -> [list of spectra]
  std::map<int, std::vector<int>> m_groupSpectraMap;

  /// The file being parsed
  std::ifstream m_file;

  /// Number of the last line parsed
  int m_lastLineRead;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_ */