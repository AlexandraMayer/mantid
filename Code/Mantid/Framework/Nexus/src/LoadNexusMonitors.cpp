#include "MantidNexus/LoadNexusMonitors.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadNexusMonitors)

/// Sets documentation strings for this algorithm
void LoadNexusMonitors::initDocs()
{
  this->setWikiSummary(" Load all monitors from a NeXus file into a workspace. ");
  this->setOptionalMessage("Load all monitors from a NeXus file into a workspace.");
}


LoadNexusMonitors::LoadNexusMonitors() : Algorithm(),
nMonitors(0)
{
}

LoadNexusMonitors::~LoadNexusMonitors()
{
}

/// Initialisation method.
void LoadNexusMonitors::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load,
      ".nxs"),
      "The name (including its full or relative path) of the Nexus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS" );

  declareProperty(
    new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "",
        Kernel::Direction::Output),
    "The name of the output workspace in which to load the Nexus monitors." );
}

/**
 * Executes the algorithm. Reading in the file and creating and populating
 * the output workspace
 */
void LoadNexusMonitors::exec()
{
  // Retrieve the filename from the properties
  this->filename = this->getPropertyValue("Filename");

  API::Progress prog1(this, 0.0, 0.2, 2);

  // top level file information
  ::NeXus::File file(this->filename);

  // FIXME: This is a SNS specific NXentry. In order to make this reader
  // more generic, this must be changed.
  //Start with the base entry
  file.openGroup("entry", "NXentry");
  prog1.report();

  //Now we want to go through and find the monitors
  std::map<std::string, std::string> entries = file.getEntries();
  std::vector<std::string> monitorNames;
  prog1.report();

  API::Progress prog2(this, 0.2, 0.6, entries.size());

  std::map<std::string,std::string>::const_iterator it = entries.begin();
  for (; it != entries.end(); it++)
  {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ((entry_class == "NXmonitor"))
    {
      monitorNames.push_back( entry_name );
    }
    prog2.report();
  }
  this->nMonitors = monitorNames.size();

  // TODO: Sort the monitor names so we can read them in order

  // Create the output workspace
  this->WS = API::WorkspaceFactory::Instance().create("Workspace2D",
      this->nMonitors, 1, 1);

  // a temporary place to put the spectra/detector numbers
  boost::shared_array<specid_t> spectra_numbers(new specid_t[this->nMonitors]);
  boost::shared_array<detid_t> detector_numbers(new detid_t[this->nMonitors]);

  API::Progress prog3(this, 0.6, 1.0, this->nMonitors);

  for (std::size_t i = 0; i < this->nMonitors; ++i)
  {
    g_log.information() << "Loading " << monitorNames[i] << std::endl;
    // Do not rely on the order in path list
    Poco::Path monPath(monitorNames[i]);
    std::string monitorName = monPath.getBaseName();
    std::string::size_type loc = monitorName.rfind('r');

    detid_t monIndex = boost::lexical_cast<int64_t>(monitorName.substr(loc+1));
    specid_t spectraIndex = i;

    g_log.debug() << "monIndex = " << monIndex << std::endl;
    g_log.debug() << "spectraIndex = " << spectraIndex << std::endl;

    spectra_numbers[spectraIndex] = monIndex;
    detector_numbers[spectraIndex] = -monIndex;
    this->WS->getAxis(1)->spectraNo(spectraIndex) = monIndex;

    // Now, actually retrieve the necessary data
    file.openGroup(monitorNames[i], "NXmonitor");
    file.openData("data");
    MantidVec data;
    MantidVec error;
    file.getDataCoerce(data);
    file.getDataCoerce(error);
    file.closeData();

    // Transform errors via square root
    std::transform(error.begin(), error.end(), error.begin(),
        (double(*)(double)) sqrt);

    // Get the TOF axis
    file.openData("time_of_flight");
    MantidVec tof;
    file.getDataCoerce(tof);
    file.closeData();
    file.closeGroup();

    this->WS->dataX(spectraIndex) = tof;
    this->WS->dataY(spectraIndex) = data;
    this->WS->dataE(spectraIndex) = error;
    prog3.report();
  }

  // Need to get the instrument name from the file
  // FIXME: System uses short name for now
  std::string instrumentName;
  file.openGroup("instrument", "NXinstrument");
  file.openPath("name");
  std::vector< ::NeXus::AttrInfo > attrs = file.getAttrInfos();
  std::vector< ::NeXus::AttrInfo >::const_iterator ait = attrs.begin();

  while(ait != attrs.end()) {
    if(ait->name == std::string("short_name")) {
      instrumentName = file.getStrAttr(*ait);
      break;
    }
    ait++;
  }

  file.closeGroup(); // Close the NXinstrument

  file.closeGroup(); // Close the NXentry
  file.close();

  this->WS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  this->WS->setYUnit("Counts");

  // Load the instrument
  this->runLoadInstrument(instrumentName, this->WS);

  // Populate the Spectra Map
  this->WS->mutableSpectraMap().populate(spectra_numbers.get(),
      detector_numbers.get(), static_cast<int64_t> (nMonitors));

  this->setProperty("OutputWorkspace", this->WS);
}
/**
 * Load the instrument geometry File
 *  @param instrument :: instrument name.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
 */
void LoadNexusMonitors::runLoadInstrument(const std::string &instrument,
    API::MatrixWorkspace_sptr localWorkspace)
{

  // do the actual work
  API::IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<API::MatrixWorkspace_sptr> ("Workspace",
        localWorkspace);
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    g_log.information() << "Invalid argument to LoadInstrument sub-algorithm : " << e.what()
        << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    g_log.information() << "Unable to successfully run LoadInstrument sub-algorithm : " << e.what()
        << std::endl;
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful)
  {
    g_log.error() << "Error loading Instrument definition file\n";
  }
  else
  {
    this->instrument_loaded_correctly = true;
  }
}

} // end NeXus
} // end Mantid
