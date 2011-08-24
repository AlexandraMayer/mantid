//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/LoadAlgorithmFactory.h" // For the DECLARE_LOADALGORITHM macro
#include "MantidAPI/SpectraAxis.h"

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/replace.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>

using std::endl;
using std::map;
using std::string;
using std::vector;

using namespace ::NeXus;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadEventNexus)
DECLARE_LOADALGORITHM(LoadEventNexus)

/// Sets documentation strings for this algorithm
void LoadEventNexus::initDocs()
{
  this->setWikiSummary("Loads Event NeXus (produced by the SNS) files and stores it in an [[EventWorkspace]]. Optionally, you can filter out events falling outside a range of times-of-flight and/or a time interval. ");
  this->setOptionalMessage("Loads Event NeXus (produced by the SNS) files and stores it in an EventWorkspace. Optionally, you can filter out events falling outside a range of times-of-flight and/or a time interval.");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument;


//===============================================================================================
//===============================================================================================
/** This task does the disk IO from loading the NXS file,
 * and so will be on a disk IO mutex */
class ProcessBankData : public Task
{
public:
  /**
   *
   * @param alg :: LoadEventNexus
   * @param entry_name :: name of the bank
   * @param pixelID_to_wi_map :: map pixel ID to Workspace Index
   * @param prog :: Progress reporter
   * @param scheduler :: ThreadScheduler running this task
   * @param event_id :: array with event IDs
   * @param event_time_of_flight :: array with event TOFS
   * @param numEvents :: how many events in the arrays
   * @param startAt :: index of the first event from event_index
   * @param event_index_ptr :: ptr to a vector of event index (length of # of pulses)
   * @return
   */
  ProcessBankData(LoadEventNexus * alg, std::string entry_name, detid2index_map * pixelID_to_wi_map,
      Progress * prog, ThreadScheduler * scheduler,
      uint32_t * event_id, float * event_time_of_flight,
      size_t numEvents, size_t startAt, std::vector<uint64_t> * event_index_ptr)
  : Task(),
    alg(alg), entry_name(entry_name), pixelID_to_wi_map(pixelID_to_wi_map), prog(prog), scheduler(scheduler),
    event_id(event_id), event_time_of_flight(event_time_of_flight), numEvents(numEvents), startAt(startAt),
    event_index_ptr(event_index_ptr), event_index(*event_index_ptr)
  {
    // Cost is approximately proportional to the number of events to process.
    m_cost = static_cast<double>(numEvents);
  }

  //----------------------------------------------------
  // Run the data processing
  void run()
  {
    //Local tof limits
    double my_shortest_tof, my_longest_tof;
    my_shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
    my_longest_tof = 0.;

    prog->report(entry_name + ": precount");

    // ---- Pre-counting events per pixel ID ----
    if (alg->precount)
    {
      std::vector<size_t> counts;
      // key = pixel ID, value = count
      counts.resize(alg->eventid_max+1);
      for (size_t i=0; i < numEvents; i++)
      {
        detid_t thisId = detid_t(event_id[i]);
        if (thisId <= alg->eventid_max)
          counts[thisId]++;
      }

      // Now we pre-allocate (reserve) the vectors of events in each pixel counted
      for (detid_t pixID = 0; pixID <= alg->eventid_max; pixID++)
      {
        if (counts[pixID] > 0)
        {
          //Find the the workspace index corresponding to that pixel ID
          size_t wi = static_cast<size_t>((*pixelID_to_wi_map)[ pixID ]);
          // Allocate it
          alg->WS->getEventList(wi).reserve( counts[pixID] );
          if (alg->getCancel()) break; // User cancellation
        }
      }
    }

    // Check for cancelled algorithm
    if (alg->getCancel())
    {  delete [] event_id;  delete [] event_time_of_flight; return;  }

    //Default pulse time (if none are found)
    Mantid::Kernel::DateAndTime pulsetime;

    // Index into the pulse array
    int pulse_i = 0;

    // And there are this many pulses
    int numPulses = static_cast<int>(alg->pulseTimes.size());
    if (numPulses > static_cast<int>(event_index.size()))
    {
      alg->getLogger().warning() << "Entry " << entry_name << "'s event_index vector is smaller than the proton_charge DAS log. This is inconsistent, so we cannot find pulse times for this entry.\n";
      //This'll make the code skip looking for any pulse times.
      pulse_i = numPulses + 1;
    }

    prog->report(entry_name + ": filling events");

    // The workspace
    EventWorkspace_sptr WS = alg->WS;

    // Will we need to compress?
    bool compress = (alg->compressTolerance >= 0);

    // Which detector IDs were touched?
    std::vector<bool> usedDetIds(alg->eventid_max+1, false);

    //Go through all events in the list
    for (std::size_t i = 0; i < numEvents; i++)
    {
      //------ Find the pulse time for this event index ---------
      if (pulse_i < numPulses-1)
      {
        bool breakOut = false;
        //Go through event_index until you find where the index increases to encompass the current index. Your pulse = the one before.
        while ( !((i+startAt >= event_index[pulse_i]) && (i+startAt < event_index[pulse_i+1])))
        {
          pulse_i++;
          // Check once every new pulse if you need to cancel (checking on every event might slow things down more)
          if (alg->getCancel()) breakOut = true;
          if (pulse_i >= (numPulses-1))
            break;
        }
        //Save the pulse time at this index for creating those events
        pulsetime = alg->pulseTimes[pulse_i];

        // Flag to break out of the event loop with using goto ;)
        if (breakOut)
          break;
      }

      //Create the tofevent
      double tof = static_cast<double>( event_time_of_flight[i] );
      if ((tof >= alg->filter_tof_min) && (tof <= alg->filter_tof_max))
      {
        //The event TOF passes the filter.
        TofEvent event(tof, pulsetime);

        // We cached a pointer to the vector<tofEvent> -> so retrieve it and add the event
        detid_t detId = event_id[i];
        if (detId <= alg->eventid_max)
        {
          alg->eventVectors[detId]->push_back( event );

//        //Find the the workspace index corresponding to that pixel ID
//        size_t wi = static_cast<size_t>((*pixelID_to_wi_map)[event_id[i]]);
//        // Add it to the list at that workspace index
//        WS->getEventList(wi).addEventQuickly( event );

          //Local tof limits
          if (tof < my_shortest_tof) { my_shortest_tof = tof;}
          if (tof > my_longest_tof) { my_longest_tof = tof;}

          // Track all the touched wi
          if (compress)
          {
            usedDetIds[detId] = true;
          }
        } // valid detector IDs

      }
    } //(for each event)


    //------------ Compress Events ------------------
    if (compress)
    {
      // Do it on all the detector IDs we touched
      std::set<size_t>::iterator it;
      for (detid_t pixID = 0; pixID <= alg->eventid_max; pixID++)
      {
        if (usedDetIds[pixID])
        {
          //Find the the workspace index corresponding to that pixel ID
          size_t wi = static_cast<size_t>((*pixelID_to_wi_map)[ pixID ]);
          EventList * el = WS->getEventListPtr(wi);
          el->compressEvents(alg->compressTolerance, el);
        }
      }
    }

    //Join back up the tof limits to the global ones
    PARALLEL_CRITICAL(tof_limits)
    {
      //This is not thread safe, so only one thread at a time runs this.
      if (my_shortest_tof < alg->shortest_tof) { alg->shortest_tof = my_shortest_tof;}
      if (my_longest_tof > alg->longest_tof ) { alg->longest_tof  = my_longest_tof;}
    }

    // Free Memory
    delete [] event_id;
    delete [] event_time_of_flight;
    delete event_index_ptr;
    // For Linux with tcmalloc, make sure memory goes back;
    // but don't call if more than 15% of memory is still available, since that slows down the loading.
    MemoryManager::Instance().releaseFreeMemoryIfAbove(0.85);
  }


private:
  /// Algorithm being run
  LoadEventNexus * alg;
  /// NXS path to bank
  std::string entry_name;
  /// Map of pixel ID to Workspace Index
  detid2index_map * pixelID_to_wi_map;
  /// Progress reporting
  Progress * prog;
  /// ThreadScheduler running this task
  ThreadScheduler * scheduler;
  /// event pixel ID array
  uint32_t * event_id;
  /// event TOF array
  float * event_time_of_flight;
  /// # of events in arrays
  size_t numEvents;
  /// index of the first event from event_index
  size_t startAt;
  /// ptr to a vector of event index vs time (length of # of pulses)
  std::vector<uint64_t> * event_index_ptr;
  /// vector of event index (length of # of pulses)
  std::vector<uint64_t> & event_index;
};




//===============================================================================================
//===============================================================================================
/** This task does the disk IO from loading the NXS file,
 * and so will be on a disk IO mutex */
class LoadBankFromDiskTask : public Task
{
public:
  //---------------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param top_entry_name :: The pathname of the top level NXentry to use
   * @param entry_name :: The pathname of the bank to load
   * @param entry_type :: The classtype of the entry to load
   * @param pixelID_to_wi_map :: a map where key = pixelID and value = the workpsace index to use.
   * @param prog :: an optional Progress object
   * @param ioMutex :: a mutex shared for all Disk I-O tasks
   * @param scheduler :: the ThreadScheduler that runs this task.
   */
  LoadBankFromDiskTask(LoadEventNexus * alg, const std::string& top_entry_name, const std::string& entry_name, const std::string & entry_type, detid2index_map * pixelID_to_wi_map,
      Progress * prog, Mutex * ioMutex, ThreadScheduler * scheduler)
  : Task(),
    alg(alg), top_entry_name(top_entry_name), entry_name(entry_name), entry_type(entry_type),
    pixelID_to_wi_map(pixelID_to_wi_map), prog(prog), scheduler(scheduler)
  {
    setMutex(ioMutex);
  }


  //---------------------------------------------------------------------------------------------------
  void run()
  {

    //The vectors we will be filling
    std::vector<uint64_t> * event_index_ptr = new std::vector<uint64_t>();
    std::vector<uint64_t> & event_index = *event_index_ptr;

    // These give the limits in each file as to which events we actually load (when filtering by time).
    std::vector<int> load_start(1); //TODO: Should this be size_t?
    std::vector<int> load_size(1);

    // Data arrays
    uint32_t * event_id = NULL;
    float * event_time_of_flight = NULL;

    bool loadError = false ;

    prog->report(entry_name + ": load from disk");


    // Open the file
    ::NeXus::File file(alg->m_filename);
    try
    {
    file.openGroup(top_entry_name, "NXentry");

    //Open the bankN_event group
    file.openGroup(entry_name, entry_type);

    // Get the event_index (a list of size of # of pulses giving the index in the event list for that pulse)
    file.openData("event_index");
    //Must be uint64
    if (file.getInfo().type == ::NeXus::UINT64)
      file.getData(event_index);
    else
    {
     alg->getLogger().warning() << "Entry " << entry_name << "'s event_index field is not UINT64! It will be skipped.\n";
     loadError = true;
    }
    file.closeData();

    // Look for the sign that the bank is empty
    if (event_index.size()==1)
    {
      if (event_index[0] == 0)
      {
        //One entry, only zero. This means NO events in this bank.
        loadError = true;
        alg->getLogger().debug() << "Bank " << entry_name << " is empty.\n";
      }
    }

    if (event_index.size() != alg->pulseTimes.size())
    {
      alg->getLogger().debug() << "Bank " << entry_name << " has a mismatch between the number of event_index entries and the number of pulse times.\n";
    }

    if (!loadError)
    {
      bool old_nexus_file_names = false;

      // Get the list of pixel ID's
      try
      {
        file.openData("event_id");
      }
      catch (::NeXus::Exception& )
      {
        //Older files (before Nov 5, 2010) used this field.
        file.openData("event_pixel_id");
        old_nexus_file_names = true;
      }

      // By default, use all available indices
      size_t start_event = 0;
      ::NeXus::Info id_info = file.getInfo();
      size_t stop_event = static_cast<size_t>(id_info.dims[0]);

      //TODO: Handle the time filtering by changing the start/end offsets.
      for (size_t i=0; i < alg->pulseTimes.size(); i++)
      {
        if (alg->pulseTimes[i] >= alg->filter_time_start)
        {
          start_event = event_index[i];
          break; // stop looking
        }
      }

      if (start_event > static_cast<size_t>(id_info.dims[0]))
      {
        // For bad file around SEQ_7872, Jul 15, 2011, Janik Zikovsky
        alg->getLogger().information() << this->entry_name << "'s field 'event_index' seem to be invalid (> than the number of events in the bank). Filtering by time ignored.\n";
        start_event = 0;
        stop_event =  static_cast<size_t>(id_info.dims[0]);
      }
      else
      {
        for (size_t i=0; i < alg->pulseTimes.size(); i++)
        {
          if (alg->pulseTimes[i] > alg->filter_time_stop)
          {
            stop_event = event_index[i];
            break;
          }
        }
      }

      // Make sure it is within range
      if (stop_event > static_cast<size_t>(id_info.dims[0]))
        stop_event = id_info.dims[0];

      alg->getLogger().debug() << entry_name << ": start_event " << start_event << " stop_event "<< stop_event << std::endl;

      // These are the arguments to getSlab()
      load_start[0] = static_cast<int>(start_event);
      load_size[0] = static_cast<int>(stop_event - start_event);

      if ((load_size[0] > 0) && (load_start[0]>=0) )
      {
        // Now we allocate the required arrays
        event_id = new uint32_t[load_size[0]];
        event_time_of_flight = new float[load_size[0]];

        // Check that the required space is there in the file.
        if (id_info.dims[0] < load_size[0]+load_start[0])
        {
          alg->getLogger().warning() << "Entry " << entry_name << "'s event_id field is too small (" << id_info.dims[0]
                          << ") to load the desired data size (" << load_size[0]+load_start[0] << ").\n";
          loadError = true;
        }

        if (alg->getCancel()) loadError = true; //To allow cancelling the algorithm

        if (!loadError)
        {
          //Must be uint32
          if (id_info.type == ::NeXus::UINT32)
            file.getSlab(event_id, load_start, load_size);
          else
          {
            alg->getLogger().warning() << "Entry " << entry_name << "'s event_id field is not UINT32! It will be skipped.\n";
            loadError = true;
          }
          file.closeData();
        }

        if (alg->getCancel()) loadError = true; //To allow cancelling the algorithm

        if (!loadError)
        {
          // Get the list of event_time_of_flight's
          if (!old_nexus_file_names)
            file.openData("event_time_offset");
          else
            file.openData("event_time_of_flight");

          // Check that the required space is there in the file.
          ::NeXus::Info tof_info = file.getInfo();
          if (tof_info.dims[0] < load_size[0]+load_start[0])
          {
            alg->getLogger().warning() << "Entry " << entry_name << "'s event_time_offset field is too small to load the desired data.\n";
            loadError = true;
          }

          //Check that the type is what it is supposed to be
          if (tof_info.type == ::NeXus::FLOAT32)
            file.getSlab(event_time_of_flight, load_start, load_size);
          else
          {
            alg->getLogger().warning() << "Entry " << entry_name << "'s event_time_offset field is not FLOAT32! It will be skipped.\n";
            loadError = true;
          }

          if (!loadError)
          {
            std::string units;
            file.getAttr("units", units);
            if (units != "microsecond")
            {
              alg->getLogger().warning() << "Entry " << entry_name << "'s event_time_offset field's units are not microsecond. It will be skipped.\n";
              loadError = true;
            }
            file.closeData();
          } //no error
        } //no error
      } // Size is at least 1
      else
      {
        // Found a size that was 0 or less; stop processign
        loadError=true;
      }

    } //no error

    } // try block
    catch (std::exception & e)
    {
      alg->getLogger().error() << "Error while loading bank " << entry_name << ":" << std::endl;
      alg->getLogger().error() << e.what() << std::endl;
      loadError = true;
    }
    catch (...)
    {
      alg->getLogger().error() << "Unspecified error while loading bank " << entry_name << std::endl;
      loadError = true;
    }

    //Close up the file even if errors occured.
    file.closeGroup();
    file.close();

    //Abort if anything failed
    if (loadError)
    {
      prog->reportIncrement(2, entry_name + ": skipping");
      delete [] event_id;
      delete [] event_time_of_flight;
      delete event_index_ptr;
      return;
    }

    // No error? Launch a new task to process that data.
    size_t numEvents = load_size[0];
    size_t startAt = load_start[0];
    ProcessBankData * newTask = new ProcessBankData(alg, entry_name,pixelID_to_wi_map,prog,scheduler,
        event_id,event_time_of_flight, numEvents, startAt, event_index_ptr);
    scheduler->push(newTask);
  }



private:
  /// Algorithm being run
  LoadEventNexus * alg;
  /// NXS name for top level NXentry
  std::string top_entry_name;
  /// NXS path to bank
  std::string entry_name;
  /// NXS type
  std::string entry_type;
  /// Map of pixel ID to Workspace Index
  detid2index_map * pixelID_to_wi_map;
  /// Progress reporting
  Progress * prog;
  /// ThreadScheduler running this task
  ThreadScheduler * scheduler;
};







//===============================================================================================
//===============================================================================================

/// Empty default constructor
LoadEventNexus::LoadEventNexus() : IDataFileChecker()
{}

/**
 * Do a quick file type check by looking at the first 100 bytes of the file 
 *  @param filePath :: path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
bool LoadEventNexus::quickFileCheck(const std::string& filePath,size_t nread, const file_header& header)
{
  std::string ext = this->extension(filePath);
  // If the extension is nxs then give it a go
  if( ext.compare("nxs") == 0 ) return true;

  // If not then let's see if it is a HDF file by checking for the magic cookie
  if ( nread >= sizeof(int32_t) && (ntohl(header.four_bytes) == g_hdf_cookie) ) return true;
  return false;
}

/**
 * Checks the file by opening it and reading few lines 
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file 
 */
int LoadEventNexus::fileCheck(const std::string& filePath)
{
  int confidence(0);
  typedef std::map<std::string,std::string> string_map_t; 
  try
  {
    string_map_t::const_iterator it;
    ::NeXus::File file = ::NeXus::File(filePath);
    string_map_t entries = file.getEntries();
    for(string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it)
    {
      if ( ((it->first == "entry") || (it->first == "raw_data_1")) && (it->second == "NXentry") ) 
      {
        file.openGroup(it->first, it->second);
        string_map_t entries2 = file.getEntries();
        for(string_map_t::const_iterator it2 = entries2.begin(); it2 != entries2.end(); ++it2)
        {
          if (it2->second == "NXevent_data")
          {
            confidence = 80;
          }
        }
        file.closeGroup();
      }
    }
  }
  catch(::NeXus::Exception&)
  {
  }
  return confidence;
}

/// Initialisation method.
void LoadEventNexus::init()
{
  std::vector<std::string> exts;
  exts.push_back("_event.nxs");
  exts.push_back(".nxs");
  this->declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name (including its full or relative path) of the Nexus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS" );

  this->declareProperty(
    new WorkspaceProperty<IEventWorkspace>("OutputWorkspace", "", Direction::Output),
    "The name of the output EventWorkspace in which to load the EventNexus file." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTof_Min", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events that do not fall within a range of times-of-flight.\n"\
    "This is the minimum accepted value in microseconds." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTof_Max", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events that do not fall within a range of times-of-flight.\n"\
    "This is the maximum accepted value in microseconds." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTime_Start", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events after the provided start time, in seconds (relative to the start of the run).");

  declareProperty(
      new PropertyWithValue<double>("FilterByTime_Stop", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events before the provided stop time, in seconds (relative to the start of the run).");

  declareProperty(
      new PropertyWithValue<string>("BankName", "", Direction::Input),
    "Optional: To only include events from one bank. Any bank whose name does not match the given string will have no events.");

  declareProperty(
      new PropertyWithValue<bool>("SingleBankPixelsOnly", true, Direction::Input),
    "Optional: Only applies if you specified a single bank to load with BankName.\n"
    "Only pixels in the specified bank will be created if true; all of the instrument's pixels will be created otherwise.");

  declareProperty(
      new PropertyWithValue<bool>("LoadMonitors", false, Direction::Input),
      "Load the monitors from the file (optional, default False).");

  declareProperty(
      new PropertyWithValue<bool>("Precount", false, Direction::Input),
      "Pre-count the number of events in each pixel before allocating memory (optional, default False). \n"
      "This can significantly reduce memory use and memory fragmentation; it may also speed up loading.");

  declareProperty(
      new PropertyWithValue<double>("CompressTolerance", -1.0, Direction::Input),
      "Run CompressEvents while loading (optional, leave blank or negative to not do). \n"
      "This specified the tolerance to use (in microseconds) when compressing.");

  declareProperty(new PropertyWithValue<bool>("MonitorsAsEvents", false, Direction::Input),
      "If present, load the monitors as events.\nWARNING: WILL SIGNIFICANTLY INCREASE MEMORY USAGE (optional, default False). \n");
}


/// set the name of the top level NXentry m_top_entry_name
void LoadEventNexus::setTopEntryName()
{
  typedef std::map<std::string,std::string> string_map_t; 
  try
  {
    string_map_t::const_iterator it;
    ::NeXus::File file = ::NeXus::File(m_filename);
    string_map_t entries = file.getEntries();
    for (it = entries.begin(); it != entries.end(); ++it)
    {
      if ( ((it->first == "entry") || (it->first == "raw_data_1")) && (it->second == "NXentry") )
      {
        m_top_entry_name = it->first;
        break;
      }
    }
  }
  catch(const std::exception&)
  {
    g_log.error() << "Unable to determine name of top level NXentry - assuming \"entry\"." << std::endl;
    m_top_entry_name = "entry";
  }
}

//------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 */
void LoadEventNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  precount = getProperty("Precount");
  compressTolerance = getProperty("CompressTolerance");

  loadlogs = true;

  //Get the limits to the filter
  filter_tof_min = getProperty("FilterByTof_Min");
  filter_tof_max = getProperty("FilterByTof_Max");
  if ( (filter_tof_min == EMPTY_DBL()) ||  (filter_tof_max == EMPTY_DBL()))
  {
    //Nothing specified. Include everything
    filter_tof_min = -1e20;
    filter_tof_max = +1e20;
  }
  else if ( (filter_tof_min != EMPTY_DBL()) ||  (filter_tof_max != EMPTY_DBL()))
  {
    //Both specified. Keep these values
  }
  else
    throw std::invalid_argument("You must specify both the min and max of time of flight to filter, or neither!");

  // Check to see if the monitors need to be loaded later
  bool load_monitors = this->getProperty("LoadMonitors");
  setTopEntryName();

  //Initialize progress reporting.
  int reports = 3;
  if (load_monitors)
    reports++;
  Progress prog(this,0.0,0.3,  reports);
  
  // Load the detector events
  WS = createEmptyEventWorkspace(); // Algorithm currently relies on an object-level workspace ptr
  loadEvents(&prog, false); // Do not load monitor blocks
  //Save output
  this->setProperty<IEventWorkspace_sptr>("OutputWorkspace", WS);
  // Load the monitors
  if (load_monitors)
  {
    prog.report("Loading monitors");
    const bool eventMonitors = getProperty("MonitorsAsEvents");
    if( eventMonitors && this->hasEventMonitors() )
    {
      WS = createEmptyEventWorkspace(); // Algorithm currently relies on an object-level workspace ptr
      loadEvents(&prog, true);
      std::string mon_wsname = this->getProperty("OutputWorkspace");
      mon_wsname.append("_monitors");
      this->declareProperty(new WorkspaceProperty<IEventWorkspace>
                            ("MonitorWorkspace", mon_wsname, Direction::Output), "Monitors from the Event NeXus file");
      this->setProperty<IEventWorkspace_sptr>("MonitorWorkspace", WS);      
    }
    else
    {
      this->runLoadMonitors();
    }
  }

  // Clear any large vectors to free up memory.
  this->pulseTimes.clear();

  // Some memory feels like it sticks around (on Linux). Free it.
  MemoryManager::Instance().releaseFreeMemory();

  return;
}



//-----------------------------------------------------------------------------
/** Generate a look-up table where the index = the pixel ID of an event
 * and the value = a pointer to the EventList in the workspace
 */
void LoadEventNexus::makeMapToEventLists()
{
  eventid_max = 0; // seems like a safe lower bound
  if( this->event_id_is_spec )
  {
    // Maximum possible spectrum number
    detid2index_map::const_iterator it = pixelID_to_wi_map->end();
    --it;
    eventid_max = it->first;
  }
  else
  {
    // We want to pad out empty pixels.
    detid2det_map detector_map;
    WS->getInstrument()->getDetectors(detector_map);

    // determine maximum pixel id
    detid2det_map::iterator it;
    for (it = detector_map.begin(); it != detector_map.end(); it++)
    {
      if (it->first > eventid_max) eventid_max = it->first;
    }
  }
  // Make an array where index = pixel ID
  // Set the value to the 0th workspace index by default
  eventVectors.resize(eventid_max+1, &WS->getEventList(0).getEvents() );
  for (detid_t j=0; j<eventid_max+1; j++)
  {
    size_t wi = (*pixelID_to_wi_map)[j];
    // Save a POINTER to the vector<tofEvent>
    eventVectors[j] = &WS->getEventList(wi).getEvents();
  }
}


//-----------------------------------------------------------------------------
/**
 * Load events from the file
 * @param prop :: A pointer to the progress reporting object
 * @param monitors :: If true the events from the monitors are loaded and not the main banks
 */
void LoadEventNexus::loadEvents(API::Progress * const prog, const bool monitors)
{
  // The run_start will be loaded from the pulse times.
  DateAndTime run_start(0,0);

  if (loadlogs)
  {
    prog->doReport("Loading DAS logs");
    runLoadNexusLogs(m_filename, WS, pulseTimes, this);
    run_start = WS->getFirstPulseTime();
  }
  else
  {
    g_log.information() << "Skipping the loading of sample logs!" << endl;
  }

  //Load the instrument
  prog->report("Loading instrument");
  instrument_loaded_correctly = runLoadInstrument(m_filename, WS, m_top_entry_name, this);

  if (!this->instrument_loaded_correctly)
      throw std::runtime_error("Instrument was not initialized correctly! Loading cannot continue.");


  // top level file information
  ::NeXus::File file(m_filename);

  //Start with the base entry
  file.openGroup(m_top_entry_name, "NXentry");

  //Now we want to go through all the bankN_event entries
  vector<string> bankNames;
  map<string, string> entries = file.getEntries();
  map<string,string>::const_iterator it = entries.begin();
  std::string classType = monitors ? "NXmonitor" : "NXevent_data";
  for (; it != entries.end(); it++)
  {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ( entry_class == classType )
    {
      bankNames.push_back( entry_name );
    }
  }

  //Close up the file
  file.closeGroup();
  file.close();

  // --------- Loading only one bank ? ----------------------------------
  std::string onebank = getProperty("BankName");
  bool doOneBank = (onebank != "");
  bool SingleBankPixelsOnly = getProperty("SingleBankPixelsOnly");
  if (doOneBank && !monitors)
  {
    bool foundIt = false;
    for (std::vector<string>::iterator it=bankNames.begin(); it!= bankNames.end(); it++)
    {
      if (*it == ( onebank + "_events") )
      {
        foundIt = true;
        break;
      }
    }
    if (!foundIt)
    {
      throw std::invalid_argument("No entry named '" + onebank + "_events'" + " was found in the .NXS file.\n");
    }
    bankNames.clear();
    bankNames.push_back( onebank + "_events" );
    if( !SingleBankPixelsOnly ) onebank = ""; // Marker to load all pixels 
  }
  else
  {
    onebank = "";
  }

  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().doesExist(outName))
    AnalysisDataService::Instance().remove( outName );

  prog->report("Initializing all pixels");

  //----------------- Pad Empty Pixels -------------------------------
  // Create the required spectra mapping so that the workspace knows what to pad to
  createSpectraMapping(m_filename, WS, monitors, onebank);
  WS->padSpectra();

  //This map will be used to find the workspace index
  if( this->event_id_is_spec ) pixelID_to_wi_map = WS->getSpectrumToWorkspaceIndexMap();
  else pixelID_to_wi_map = WS->getDetectorIDToWorkspaceIndexMap(true);

  // Cache a map for speed.
  this->makeMapToEventLists();

  // --------------------------- Time filtering ------------------------------------
  double filter_time_start_sec, filter_time_stop_sec;
  filter_time_start_sec = getProperty("FilterByTime_Start");
  filter_time_stop_sec = getProperty("FilterByTime_Stop");

  //Default to ALL pulse times
  bool is_time_filtered = false;
  filter_time_start = Kernel::DateAndTime::minimum();
  filter_time_stop = Kernel::DateAndTime::maximum();

  if (pulseTimes.size() > 0)
  {
    //If not specified, use the limits of doubles. Otherwise, convert from seconds to absolute PulseTime
    if (filter_time_start_sec != EMPTY_DBL())
    {
      filter_time_start = run_start + filter_time_start_sec;
      is_time_filtered = true;
    }

    if (filter_time_stop_sec != EMPTY_DBL())
    {
      filter_time_stop = run_start + filter_time_stop_sec;
      is_time_filtered = true;
    }

    //Silly values?
    if (filter_time_stop < filter_time_start)
      throw std::invalid_argument("Your filter for time's Stop value is smaller than the Start value.");
  }

  //Count the limits to time of flight
  shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  longest_tof = 0.;

  Progress * prog2 = new Progress(this,0.3,1.0, bankNames.size()*3);

  // Make the thread pool
  ThreadScheduler * scheduler = new ThreadSchedulerLargestCost();
  ThreadPool pool(scheduler, 8);
  Mutex * diskIOMutex = new Mutex();
  for (size_t i=0; i < bankNames.size(); i++)
  {
    // We make tasks for loading
    pool.schedule( new LoadBankFromDiskTask(this,m_top_entry_name,bankNames[i],classType, pixelID_to_wi_map, prog2, diskIOMutex, scheduler) );
  }
  // Start and end all threads
  pool.joinAll();
  delete diskIOMutex;
  delete prog2;


  //Don't need the map anymore.
  delete pixelID_to_wi_map;

  if (is_time_filtered)
  {
    //Now filter out the run, using the DateAndTime type.
    WS->mutableRun().filterByTime(filter_time_start, filter_time_stop);
  }

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

  // set more properties on the workspace
  loadEntryMetadata(m_filename, WS, m_top_entry_name);
}

//-----------------------------------------------------------------------------
/**
 * Create a blank event workspace
 * @returns A shared pointer to a new empty EventWorkspace object
 */
EventWorkspace_sptr LoadEventNexus::createEmptyEventWorkspace()
{
  // Create the output workspace
  EventWorkspace_sptr eventWS(new EventWorkspace());
  //Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't matter
  eventWS->initialize(1,1,1);

  // Set the units
  eventWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  eventWS->setYUnit("Counts");

  // Create a default "Universal" goniometer in the Run object
  eventWS->mutableRun().getGoniometer().makeUniversalGoniometer();
  return eventWS;
}


//-----------------------------------------------------------------------------
/** Load the run number and other meta data from the given bank */
void LoadEventNexus::loadEntryMetadata(const std::string &nexusfilename, Mantid::API::MatrixWorkspace_sptr WS,
    const std::string &entry_name)
{
  // Open the file
  ::NeXus::File file(nexusfilename);
  file.openGroup(entry_name, "NXentry");

  // get the title
  file.openData("title");
  if (file.getInfo().type == ::NeXus::CHAR) {
    string title = file.getStrData();
    if (!title.empty())
      WS->setTitle(title);
  }
  file.closeData();

  // TODO get the run number
  file.openData("run_number");
  string run("");
  if (file.getInfo().type == ::NeXus::CHAR) {
    run = file.getStrData();
  }
  if (!run.empty()) {
    WS->mutableRun().addProperty("run_number", run);
  }
  file.closeData();

  // get the duration
  file.openData("duration");
  std::vector<double> duration;
  file.getDataCoerce(duration);
  if (duration.size() == 1)
  {
    // get the units
    std::vector<AttrInfo> infos = file.getAttrInfos();
    std::string units("");
    for (std::vector<AttrInfo>::const_iterator it = infos.begin(); it != infos.end(); it++)
    {
      if (it->name.compare("units") == 0)
      {
        units = file.getStrAttr(*it);
        break;
      }
    }

    // set the property
    WS->mutableRun().addProperty("duration", duration[0], units);
  }
  file.closeData();

  // close the file
  file.close();
}



//-----------------------------------------------------------------------------
/** Load the instrument geometry file using info in the NXS file.
 *
 *  @param nexusfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
 *  @param top_entry_name :: entry name at the top of the NXS file
 *  @return true if successful
 */
bool LoadEventNexus::runLoadInstrument(const std::string &nexusfilename, MatrixWorkspace_sptr localWorkspace,
    const std::string & top_entry_name, Algorithm * alg)
{
  string instrument;

  // Get the instrument name
  ::NeXus::File nxfile(nexusfilename);
  //Start with the base entry
  nxfile.openGroup(top_entry_name, "NXentry");
  // Open the instrument
  nxfile.openGroup("instrument", "NXinstrument");
  nxfile.openData("name");
  instrument = nxfile.getStrData();
  alg->getLogger().debug() << "Instrument name read from NeXus file is " << instrument << std::endl;
  if (instrument.compare("POWGEN3") == 0) // hack for powgen b/c of bad long name
          instrument = "POWGEN";
  // Now let's close the file as we don't need it anymore to load the instrument.
  nxfile.close();

  // do the actual work
  IAlgorithm_sptr loadInst= alg->createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", false);
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    alg->getLogger().information() << "Invalid argument to LoadInstrument sub-algorithm : " << e.what() << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    alg->getLogger().information("Unable to successfully run LoadInstrument sub-algorithm");
    alg->getLogger().information(e.what());
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful)
  {
    alg->getLogger().error() << "Error loading Instrument definition file\n";
  }
  return executionSuccessful;
}


//-----------------------------------------------------------------------------
/** Load the sample logs from the NXS file
 *
 *  @param nexusfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the logs
 *  @param[out] pulseTimes :: vector of pulse times to fill
 *  @return true if successful
 */
bool LoadEventNexus::runLoadNexusLogs(const std::string &nexusfilename, API::MatrixWorkspace_sptr localWorkspace,
    std::vector<Kernel::DateAndTime> & pulseTimes, Algorithm * alg)
{
  // --------------------- Load DAS Logs -----------------
  //The pulse times will be empty if not specified in the DAS logs.
  pulseTimes.clear();
  IAlgorithm_sptr loadLogs = alg->createSubAlgorithm("LoadNexusLogs");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    alg->getLogger().information() << "Loading logs from NeXus file..." << endl;
    loadLogs->setPropertyValue("Filename", nexusfilename);
    loadLogs->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadLogs->execute();

    //If successful, we can try to load the pulse times
    Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( localWorkspace->mutableRun().getProperty("proton_charge") );
    std::vector<Kernel::DateAndTime> temp = log->timesAsVector();
    pulseTimes.reserve(temp.size());
    for (size_t i =0; i < temp.size(); i++)
    {
      pulseTimes.push_back( temp[i] );
    }

    // Use the first pulse as the run_start time.
    if (temp.size() > 0)
    {
      Kernel::DateAndTime run_start = localWorkspace->getFirstPulseTime();
      // add the start of the run as a ISO8601 date/time string. The start = first non-zero time.
      // (this is used in LoadInstrumentHelper to find the right instrument file to use).
      localWorkspace->mutableRun().addProperty("run_start", run_start.to_ISO8601_string(), true );
    }
    else
      alg->getLogger().warning() << "Empty proton_charge sample log. You will not be able to filter by time.\n";
  }
  catch (...)
  {
    alg->getLogger().error() << "Error while loading Logs from SNS Nexus. Some sample logs may be missing." << std::endl;
    return false;
  }
  return true;
}



//-----------------------------------------------------------------------------
/**
 * Create the required spectra mapping. If the file contains an isis_vms_compat block then
 * the mapping is read from there, otherwise a 1:1 map with the instrument is created (along
 * with the associated spectra axis)
 * @param nxsfile :: The name of a nexus file to load the mapping from
 * @param workspace :: The workspace to contain the spectra mapping
 * @param bankName :: An optional bank name for loading a single bank
 */
void LoadEventNexus::createSpectraMapping(const std::string &nxsfile, 
    API::MatrixWorkspace_sptr workspace, const bool monitorsOnly,
    const std::string & bankName)
{
  Geometry::ISpectraDetectorMap *spectramap(NULL);
  this->event_id_is_spec = false;
  if( !monitorsOnly && !bankName.empty() )
  {
    // Only build the map for the single bank
    std::vector<IDetector_const_sptr> dets;
    WS->getInstrument()->getDetectorsInBank(dets, bankName);
    if (dets.size() > 0)
    {
      SpectraDetectorMap *singlebank = new API::SpectraDetectorMap;
      // Make an event list for each.
      for(size_t wi=0; wi < dets.size(); wi++)
      {
        const detid_t detID = dets[wi]->getID();
        singlebank->addSpectrumEntries(specid_t(wi+1), std::vector<detid_t>(1, detID));
      }
      spectramap = singlebank;
      g_log.debug() << "Populated spectra map for single bank " << bankName << "\n";
    }
    else
      throw std::runtime_error("Could not find the bank named " + bankName + " as a component assembly in the instrument tree; or it did not contain any detectors.");
  }
  else
  {
    spectramap = loadSpectraMapping(nxsfile, WS->getInstrument(), monitorsOnly, m_top_entry_name, g_log);
    // Did we load one? If so then the event ID is the spectrum number and not det ID
    if( spectramap ) this->event_id_is_spec = true;
  }

  if( !spectramap )
  {
    g_log.debug() << "No custom spectra mapping found, continuing with default 1:1 mapping of spectrum:detectorID\n";
    // The default 1:1 will suffice but exclude the monitors as they are always in a separate workspace
    workspace->rebuildSpectraMapping(false);
    g_log.debug() << "Populated 1:1 spectra map for the whole instrument \n";
  }
  else
  {
    workspace->replaceAxis(1, new API::SpectraAxis(spectramap->nSpectra(), *spectramap));
    workspace->replaceSpectraMap(spectramap);
  }    
}

//-----------------------------------------------------------------------------
/**
 * Returns whether the file contains monitors with events in them
 * @returns True if the file contains monitors with event data, false otherwise
 */
bool LoadEventNexus::hasEventMonitors()
{
  bool result(false);
  // Determine whether to load histograms or events
  try
  {
    ::NeXus::File file(m_filename);
    file.openPath(m_top_entry_name);
    //Start with the base entry
    typedef std::map<std::string,std::string> string_map_t; 
    //Now we want to go through and find the monitors
    string_map_t entries = file.getEntries();
    for( string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it)
    {
      if( it->second == "NXmonitor" )
      {
        file.openGroup(it->first, it->second);
        break;
      }
    }
    file.openData("event_id");
    result = true;
    file.close();
  }
  catch(::NeXus::Exception &)
  {
    result = false;
  }
  return result;
}

//-----------------------------------------------------------------------------
/**
 * Load the Monitors from the NeXus file into a workspace. The original
 * workspace name is used and appended with _monitors.
 */
void LoadEventNexus::runLoadMonitors()
{
  std::string mon_wsname = this->getProperty("OutputWorkspace");
  mon_wsname.append("_monitors");

  IAlgorithm_sptr loadMonitors = this->createSubAlgorithm("LoadNexusMonitors");
  try
  {
    this->g_log.information() << "Loading monitors from NeXus file..."
        << std::endl;
    loadMonitors->setPropertyValue("Filename", m_filename);
    this->g_log.information() << "New workspace name for monitors: "
        << mon_wsname << std::endl;
    loadMonitors->setPropertyValue("OutputWorkspace", mon_wsname);
    loadMonitors->execute();
    MatrixWorkspace_sptr mons = loadMonitors->getProperty("OutputWorkspace");
    this->declareProperty(new WorkspaceProperty<>("MonitorWorkspace",
        mon_wsname, Direction::Output), "Monitors from the Event NeXus file");
    this->setProperty("MonitorWorkspace", mons);
  }
  catch (...)
  {
    this->g_log.error() << "Error while loading the monitors from the file. "
        << "File may contain no monitors." << std::endl;
  }
}

//
/**
 * Load a spectra mapping from the given file. This currently checks for the existence of
 * an isis_vms_compat block in the file, if it exists it pulls out the spectra mapping listed there
 * @param file :: A filename
 * @param monitorsOnly :: If true then only the monitor spectra are loaded
 * @param entry_name :: name of the NXentry to open.
 * @returns A pointer to a new map or NULL if the block does not exist
 */
Geometry::ISpectraDetectorMap * LoadEventNexus::loadSpectraMapping(const std::string & filename, Geometry::Instrument_sptr inst,
                                   const bool monitorsOnly, const std::string entry_name, Mantid::Kernel::Logger & g_log)
{
  ::NeXus::File file(filename);
  try
  {
    g_log.debug() << "Attempting to load custom spectra mapping from '" << entry_name << "/isis_vms_compat'.\n";
    file.openPath(entry_name + "/isis_vms_compat");
  }
  catch(::NeXus::Exception&)
  {
    return NULL; // Doesn't exist
  }
  API::SpectraDetectorMap *spectramap = new API::SpectraDetectorMap;
  // UDET
  file.openData("UDET");
  std::vector<int32_t> udet;
  file.getData(udet);
  file.closeData();
  // SPEC
  file.openData("SPEC");
  std::vector<int32_t> spec;
  file.getData(spec);
  file.closeData();
  // Close 
  file.closeGroup();
  file.close();

  // The spec array will contain a spectrum number for each udet but the spectrum number
  // may be the same for more that one detector
  const size_t ndets(udet.size());
  if( ndets != spec.size() )
  {
    std::ostringstream os;
    os << "UDET/SPEC list size mismatch. UDET=" << udet.size() << ", SPEC=" << spec.size() << "\n";
    throw std::runtime_error(os.str());
  }
  // Monitor filtering/selection
  const std::vector<detid_t> monitors = inst->getMonitors();
  if( monitorsOnly )
  {
    g_log.debug() << "Loading only monitor spectra from " << filename << "\n";
    // Find the det_ids in the udet array. 
    const size_t nmons(monitors.size());
    for( size_t i = 0; i < nmons; ++i )
    {
      // Find the index in the udet array
      const detid_t & id = monitors[i];
      std::vector<int32_t>::const_iterator it = std::find(udet.begin(), udet.end(), id);
      if( it != udet.end() )
      {
        const specid_t & specNo = spec[it - udet.begin()];
        spectramap->addSpectrumEntries(specid_t(specNo), std::vector<detid_t>(1, id));
      }
    }
  }
  else
  {
    g_log.debug() << "Loading only detector spectra from " << filename << "\n";
    // We need to filter the monitors out as they are included in the block also. Here we assume that they
    // occur in a contiguous block
    spectramap->populate(spec.data(), udet.data(), ndets, 
                         std::set<detid_t>(monitors.begin(), monitors.end()));
  }
  g_log.debug() << "Found " << spectramap->nSpectra() << " unique spectra and a total of " << spectramap->nElements() << " elements\n"; 
  return spectramap;
}


} // namespace DataHandling
} // namespace Mantid
