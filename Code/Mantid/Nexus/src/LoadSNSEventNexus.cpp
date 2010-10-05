//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadSNSEventNexus.h"
#include "MantidAPI/Instrument.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/UnitFactory.h"

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/replace.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;

using namespace ::NeXus;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadSNSEventNexus)

using namespace Kernel;
using namespace API;

/// Empty default constructor
LoadSNSEventNexus::LoadSNSEventNexus()
{}

/// Initialisation method.
void LoadSNSEventNexus::init()
{
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the Nexus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS" );

  declareProperty(
    new WorkspaceProperty<EventWorkspace>("OutputWorkspace", "", Direction::Output),
    "The name of the output EventWorkspace in which to load the EventNexus file." );

}



//------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 */
void LoadSNSEventNexus::exec()
{

  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Create the output workspace
  WS = EventWorkspace_sptr(new EventWorkspace());

  //Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't matter
  WS->initialize(1,1,1);

  // Set the units
  WS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  WS->setYUnit("Counts");

  //Load the instrument
  runLoadInstrument(m_filename, WS);

  // top level file information
  ::NeXus::File file(m_filename);
  //g_log.information() << "NeXus file found: " << file.inquireFile() << endl;

  //TODO: Here load any other data?

  //Start with the base entry
  file.openGroup("entry", "NXentry");

  //Now we want to go through all the bankN_event entries
  map<string, string> entries = file.getEntries();
  std::vector<string> bankNames;

  map<string,string>::const_iterator it = entries.begin();
  for (; it != entries.end(); it++)
  {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ((entry_class == "NXevent_data"))
    {
      bankNames.push_back( entry_name );
      //TODO: Load the pulse times (once)
    }
  }

  //Close up the file
  file.closeGroup();
  file.close();

  //----------------- Pad Empty Pixels -------------------------------
  if (true)
  {
    //We want to pad out empty pixels.
    if (!this->instrument_loaded_correctly)
    {
      g_log.warning() << "Warning! Cannot pad empty pixels, since the instrument geometry did not load correctly or was not specified. Sorry!\n";
    }
    else
    {
      std::map<int, Geometry::IDetector_sptr> detector_map = WS->getInstrument()->getDetectors();
      std::map<int, Geometry::IDetector_sptr>::iterator it;
      for (it = detector_map.begin(); it != detector_map.end(); it++)
      {
        //Go through each pixel in the map, but forget monitors.
        if (!it->second->isMonitor())
        {
          // and simply get the event list. It will be created if it was not there already.
          WS->getEventListAtPixelID(it->first); //it->first is detector ID #
        }
      }
    }
  }

  //The pulse times will be empty if not specified in the DAS logs.
  pulseTimes.clear();

  // --------------------- Load DAS Logs -----------------
  // do the actual work
  IAlgorithm_sptr loadLogs = createSubAlgorithm("LoadLogsFromSNSNexus");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    g_log.information() << "Loading logs from NeXus file..." << endl;
    loadLogs->setPropertyValue("Filename", m_filename);
    loadLogs->setProperty<MatrixWorkspace_sptr> ("Workspace", WS);
    loadLogs->execute();

    //If successful, we can try to load the pulse times
    Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( WS->mutableRun().getProperty("proton_charge") );
    std::vector<Kernel::dateAndTime> temp = log->timesAsVector();
    for (size_t i =0; i < temp.size(); i++)
      pulseTimes.push_back( Kernel::DateAndTime::get_from_absolute_time( temp[i] ) );
  }
  catch (...)
  {
    g_log.error() << "Error while loading Logs from SNS Nexus. Some sample logs may be missing." << std::endl;
  }

  //Count the limits to time of flight
  shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  longest_tof = 0.;

  //Initialize progress reporting.
  Progress prog(this,0.0,1.0, bankNames.size());

  // Now go through each bank.
  // This'll be parallelized - but you can't run it in parallel if you couldn't pad the pixels.
  //PARALLEL_FOR_NO_WSP_CHECK()
  //PARALLEL_FOR_IF( (this->instrument_loaded_correctly) )
  for (int i=0; i < static_cast<int>(bankNames.size()); i++)
  {
    this->loadBankEventData(bankNames[i]);
    prog.report();
  }

  //Now all the event lists have been made for all pixel ids
  WS->doneLoadingData();

  //Info reporting
  g_log.information() << "Read " << WS->getNumberEvents() << " events"
      << ". Shortest TOF: " << shortest_tof << " microsec; longest TOF: "
      << longest_tof << " microsec." << std::endl;


  //Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec& xRef = axis.access();
  xRef.resize(2);
  xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
  xRef[1] = longest_tof + 1;
  //Set the binning axis using this.
  WS->setAllX(axis);

  //Save output
  this->setProperty<EventWorkspace_sptr>("OutputWorkspace", WS);

  return;
}


//------------------------------------------------------------------------------------------------
/** Load one of the banks' event data from the nexus file
 *
 */
void LoadSNSEventNexus::loadBankEventData(std::string entry_name)
{
  //Local tof limits
  double my_shortest_tof, my_longest_tof;
  my_shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  my_longest_tof = 0.;


  //The vectors we will be filling
  std::vector<uint64_t> event_index;
  std::vector<uint32_t> event_pixel_id;
  std::vector<float> event_time_of_flight;

  bool loadError = false ;

  PARALLEL_CRITICAL(event_nexus_file_access)
  {
    // Open the file
    ::NeXus::File file(m_filename);
    file.openGroup("entry", "NXentry");

    //Open the bankN_event group
    file.openGroup(entry_name, "NXevent_data");

    // Get the event_index (a list of size of # of pulses giving the index in the event list for that pulse)
    file.openData("event_index");
    //Must be uint64
    if (file.getInfo().type == ::NeXus::UINT64)
      file.getData(event_index);
    else
    {
     g_log.warning() << "Entry " << entry_name << "'s event_index field is not uint64! It will be skipped.\n";
     loadError = true;
    }
    file.closeData();

    if (!loadError)
    {
      // Get the list of pixel ID's
      file.openData("event_pixel_id");
      //Must be uint32
      if (file.getInfo().type == ::NeXus::UINT32)
        file.getData(event_pixel_id);
      else
      {
        g_log.warning() << "Entry " << entry_name << "'s event_pixel_id field is not uint32! It will be skipped.\n";
        loadError = true;
      }
      file.closeData();

      if (!loadError)
      {
        // Get the list of event_time_of_flight's
        file.openData("event_time_of_flight");
        //Check that the type is what it is supposed to be
        if (file.getInfo().type == ::NeXus::FLOAT32)
          file.getData(event_time_of_flight);
        else
        {
          g_log.warning() << "Entry " << entry_name << "'s event_time_of_flight field is not FLOAT32! It will be skipped.\n";
          loadError = true;
        }

        if (!loadError)
        {
          std::string units;
          file.getAttr("units", units);
          if (units != "microsecond")
          {
            g_log.warning() << "Entry " << entry_name << "'s event_time_of_flight field's units are not microsecond. It will be skipped.\n";
            loadError = true;
          }
          file.closeData();
        } //no error

      } //no error

    } //no error

    //Close up the file
    file.closeGroup();
    file.close();
  }

  //Abort if anything failed
  if (loadError)
    return;

  // Two arrays must be the same size
  if (event_pixel_id.size() != event_time_of_flight.size())
  {
    g_log.warning() << "Entry " << entry_name << "'s event_time_of_flight and event_pixel_id vectors are not the same size! It will be skipped.\n";
    return;
  }

  //Default pulse time if none are found
  Mantid::Kernel::PulseTimeType pulsetime = 0;

  // Index into the pulse array
  int pulse_i = 0;

  // And there are this many pulses
  int numPulses = static_cast<int>(pulseTimes.size());
  if (numPulses > static_cast<int>(event_index.size()))
  {
    g_log.warning() << "Entry " << entry_name << "'s event_index vector is smaller than the proton_charge DAS log. This is inconsistent, so we cannot find pulse times for this entry.\n";
    //This'll make the code skip looking for any pulse times.
    pulse_i = numPulses + 1;
  }

  //Go through all events in the list
  std::size_t numEvents = event_pixel_id.size();
  for (std::size_t i = 0; i < numEvents; i++)
  {
    //Find the pulse time for this event index
    if (pulse_i < numPulses-1)
    {
      //Go through event_index until you find where the index increases to encompass the current index. Your pulse = the one before.
      while ( !((i >= event_index[pulse_i]) && (i < event_index[pulse_i+1])))
      {
        pulse_i++;
        if (pulse_i >= (numPulses-1))
          break;
      }
      //Save the pulse time at this index for creating those events
      pulsetime = pulseTimes[pulse_i];
    }

    //Create the tofevent
    double tof = static_cast<double>( event_time_of_flight[i] );
    TofEvent event(tof, pulsetime);

    //Add it to the list at that pixel ID
    WS->getEventListAtPixelID( event_pixel_id[i] ).addEventQuickly( event );

    //Local tof limits
    if (tof < my_shortest_tof) { my_shortest_tof = tof;}
    if (tof > my_longest_tof) { my_longest_tof = tof;}

  }

  //Join back up the tof limits to the global ones
  PARALLEL_CRITICAL(tof_limits)
  {
    //This is not thread safe, so only one thread at a time runs this.
    if (my_shortest_tof < shortest_tof) { shortest_tof = my_shortest_tof;}
    if (my_longest_tof > longest_tof ) { longest_tof  = my_longest_tof;}
  }


}



//-----------------------------------------------------------------------------
/** Load the instrument geometry File
 *  @param nexusfilename Used to pick the instrument.
 *  @param localWorkspace MatrixWorkspace in which to put the instrument geometry
 */
void LoadSNSEventNexus::runLoadInstrument(const std::string &nexusfilename, MatrixWorkspace_sptr localWorkspace)
{
  // determine the instrument parameter file
  string instrument = Poco::Path(nexusfilename).getFileName();
  size_t pos = instrument.find("_"); // get rid of everything after the first _
  instrument = instrument.substr(0, pos);

  string filename = Mantid::Kernel::ConfigService::Instance().getInstrumentFilename(instrument);
  if (filename.empty())
    return;
  if (!Poco::File(filename).exists())
    return;

  // do the actual work
  IAlgorithm_sptr loadInst= createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("Filename", filename);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    g_log.information() << "Invalid argument to LoadInstrument sub-algorithm : " << e.what() << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
    g_log.information(e.what());
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



} // namespace NeXus
} // namespace Mantid
