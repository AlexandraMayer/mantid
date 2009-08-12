// SaveNeXus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveNeXus.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NeXusUtils.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include "Poco/File.h"

namespace Mantid
{
namespace NeXus
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Empty default constructor
SaveNexus::SaveNexus() : Algorithm() {}

/** Initialisation method.
 *
 */
void SaveNexus::init()
{
  // Declare required parameters, filename with ext {.nx,.nx5,xml} and input workspace
  std::vector<std::string> exts;
  exts.push_back("NXS");
  exts.push_back("nxs");
  exts.push_back("nx5");
  exts.push_back("NX5");
  exts.push_back("xml");
  exts.push_back("XML");
  declareProperty("FileName", "", new FileValidator(exts, false),
      "The name of the Nexus file to write, as a full or relative path");
  declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace", "", Direction::Input),
      "Name of the workspace to be saved");
  //
  // Declare optional input parameters
  // These are:
  // Title       - string to describe data
  // EntryNumber - integer >0 to be used in entry name "mantid_workspace_<n>"
  //                          Within a file the entries will be sequential from 1.
  //                          This option should allow overwrite of existing entry,
  //                          *not* addition of out-of-sequence entry numbers.
  // spectrum_min, spectrum_max - range of "spectra" numbers to write
  // spectrum_list            list of spectra values to write
  //
  declareProperty("Title", "", new NullValidator<std::string> ,
      "A title to describe the saved workspace");
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
  mustBePositive->setLower(0);
  // declareProperty("EntryNumber", unSetInt, mustBePositive,
  //  "(Not implemented yet) The index number of the workspace within the Nexus file\n"
  // "(default leave unchanged)" );
  declareProperty("WorkspaceIndexMin", 0, mustBePositive->clone(),
      "Number of first WorkspaceIndex to read, only for single period data.\n"
        "Not yet implemented");
  declareProperty("WorkspaceIndexMax", unSetInt, mustBePositive->clone(),
      "Number of last WorkspaceIndex to read, only for single period data.\n"
        "Not yet implemented.");
  declareProperty(new ArrayProperty<int> ("WorkspaceIndexList"),
      "List of WorkspaceIndex numbers to read, only for single period data.\n"
        "Not yet implemented");
  declareProperty("Append", false, "Determines whether .nxs file needs to be\n"
    "over written or appended");
  // option which might be required in future - should be a choice e.g. MantidProcessed/Muon1
  // declareProperty("Filetype","",new NullValidator<std::string>);
}

/** Execute the algorithm. Currently just calls SaveNexusProcessed but could
 *  call write other formats if support added
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SaveNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("FileName");
  m_inputWorkspace = getPropertyValue("InputWorkspace");
  //retrieve the append property
  m_bAppend = getProperty("Append");
  std::string period("1");
  //getting the workspace number which gives the period 
  //looking for the number after "_"
  std::string::size_type index = m_inputWorkspace.find_last_of("_");
  if (index != std::string::npos)
  {
	std::string::size_type len= m_inputWorkspace.length();

   // std::string::reference ref = m_inputWorkspace.at(index + 1);
	std::string ref=m_inputWorkspace.substr(index + 1,len-index);
    period = ref;
	//g_log.error()<<"period = "<<period<<std::endl;
  }
  if (!period.compare("1"))
  { // if m_bAppend is default (false) overwrite (delete )the .nxs file for period 1
    if (!m_bAppend)
    {
      Poco::File file(m_filename);
      if (file.exists())
      {
        file.remove();
      }
    }
  }
  m_filetype = "NexusProcessed";

  if (m_filetype == "NexusProcessed")
  {
    runSaveNexusProcessed();
  }
  else
  {
    throw Exception::NotImplementedError("SaveNexus passed invalid filetype.");
  }

  return;
}

void SaveNexus::runSaveNexusProcessed()
{
  IAlgorithm_sptr saveNexusPro = createSubAlgorithm("SaveNexusProcessed");
  // Pass through the same output filename
  saveNexusPro->setPropertyValue("Filename", m_filename);
  // Set the workspace property
  std::string inputWorkspace = "inputWorkspace";
  saveNexusPro->setPropertyValue(inputWorkspace, m_inputWorkspace);
  //
  std::vector<int> specList = getProperty("WorkspaceIndexList");
  if (!specList.empty())
    saveNexusPro->setPropertyValue("WorkspaceIndexList", getPropertyValue("WorkspaceIndexList"));
  //
  int specMax = getProperty("WorkspaceIndexMax");
  if (specMax != unSetInt)
  {
    saveNexusPro->setPropertyValue("WorkspaceIndexMax", getPropertyValue("WorkspaceIndexMax"));
    saveNexusPro->setPropertyValue("WorkspaceIndexMin", getPropertyValue("WorkspaceIndexMin"));
  }
  std::string title = getProperty("Title");
  if (!title.empty())
    saveNexusPro->setPropertyValue("Title", getPropertyValue("Title"));
  // int entryNum = getProperty("EntryNumber");
  // if( entryNum != unSetInt )
  //     saveNexusPro->setPropertyValue("EntryNumber",getPropertyValue("EntryNumber"));

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    saveNexusPro->execute();
  } catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run SaveNexusprocessed sub-algorithm");
  }
  if (!saveNexusPro->isExecuted())
    g_log.error("Unable to successfully run SaveNexusProcessed sub-algorithm");
  //
  progress(1);
}
} // namespace NeXus
} // namespace Mantid
