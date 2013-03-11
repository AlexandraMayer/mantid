/*WIKI*

The StartLiveData algorithm launches a background job that monitors and processes live data.

The background algorithm started is [[MonitorLiveData]], which simply calls [[LoadLiveData]] at a fixed interval.

For details on the way to specify the data processing steps, see: [[LoadLiveData#Description|LoadLiveData]].

=== Live Plots ===

Once live data monitoring has started, you can open a plot in MantidPlot. For example, you can right-click a workspace and choose "Plot Spectra".

As the data is acquired, this plot updates automatically.

Another way to start plots is to use [[MantidPlot:_Help#Python_Scripting_in_MantidPlot|python MantidPlot commands]].
The StartLiveData algorithm returns after the first chunk of data has been loaded and processed.
This makes it simple to write a script that will open a live plot. For example:

<source lang="python">
StartLiveData(UpdateEvery='1.0',Instrument='FakeEventDataListener',
  ProcessingAlgorithm='Rebin',ProcessingProperties='Params=10e3,1000,60e3;PreserveEvents=1',
  OutputWorkspace='live')
plotSpectrum('live', [0,1])
</source>

=== End-Of-Run Behavior ===

* When the experimenter stops a run, the Live Data Listener receives this as a signal.
* The ''EndRunBehavior'' property specifies what to do when the run ends.
** Restart: the accumulated data from the previous run is discarded as soon as the next chunk of data arrives.
** Stop: live data monitoring ends. It will have to be restarted manually.
** Rename: the previous workspaces are renamed, and monitoring continues with cleared ones. The run number, if found, is used to rename the old workspaces.
*** There is a check for available memory before renaming; if there is not enough memory, the old data is discarded.
* Note that LiveData continues monitoring even if outside of a run (i.e. before a run begins you will still receive live data).

=== Multiple Live Data Sessions ===

It is possible to have multiple live data sessions running at the same time.
Simply call StartLiveData more than once, but make sure to specify unique
names for the ''OutputWorkspace''.

Please note that you may be limited in how much simultaneous processing you
can do by your available memory and CPUs.

*WIKI*/

#include "MantidDataHandling/StartLiveData.h"
#include "MantidKernel/System.h"
#include "MantidDataHandling/LoadLiveData.h"
#include "MantidDataHandling/MonitorLiveData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProxy.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(StartLiveData)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  StartLiveData::StartLiveData()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  StartLiveData::~StartLiveData()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string StartLiveData::name() const { return "StartLiveData";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int StartLiveData::version() const { return 1;};

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void StartLiveData::initDocs()
  {
    this->setWikiSummary("Begin live data monitoring.");
    this->setOptionalMessage("Begin live data monitoring.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void StartLiveData::init()
  {
    declareProperty(new PropertyWithValue<bool>("FromNow", true, Direction::Input),
        "Process live data starting from the current time only.");

    declareProperty(new PropertyWithValue<bool>("FromStartOfRun", false, Direction::Input),
        "Record live data, but go back to the the start of the run and process all data since then.");

    declareProperty(new PropertyWithValue<bool>("FromTime", false, Direction::Input),
        "Record live data, but go back to a specific time and process all data since then.\n"
        "You must specify the StartTime property if this is checked.");

    declareProperty(new PropertyWithValue<double>("UpdateEvery", 60.0, Direction::Input),
        "Frequency of updates, in seconds. Default 60.\n"
        "If you specify 0, MonitorLiveData will not launch and you will get only one chunk.");

    // Initialize the properties common to LiveDataAlgorithm.
    initProps();
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void StartLiveData::exec()
  {
    // Validate the inputs
    bool FromNow = getProperty("FromNow");
    bool FromStartOfRun = getProperty("FromStartOfRun");
    bool FromTime = getProperty("FromTime");
    int numChecked = 0;
    if (FromNow) numChecked++;
    if (FromStartOfRun) numChecked++;
    if (FromTime) numChecked++;

    if (numChecked != 1)
      throw std::runtime_error("Please check exactly one of FromNow, FromStartOfRun, FromTime.");


    // Adjust the StartTime if you are starting from run/now.
    if (FromNow)
      this->setPropertyValue("StartTime", "1990-01-01T00:00:00");
      // Use the epoch value for the start time.  It will get converted to 0 when passed
      // to SMSD in the ClientHello packet.  See the description of the ClientHello packet
      // in the ADARA network protocol docs.
    else if (FromStartOfRun)
      // At this point, we don't know when the start of the run was.  Set the requested time
      // to 1 second past the epoch (which will get turned into 1 when passed to SMSD in
      // the ClientHello packet) which will cause the SMS to replay all the historical data
      // it has.  We'll filter out unnecessary packets down in the live listener
      this->setPropertyValue("StartTime", "1990-01-01T00:00:01");
    else
    {
      // Validate the StartTime property.  Don't allow times from the future
      DateAndTime reqStartTime( this->getPropertyValue( "StartTime"));
      // DateAndTime will throw an exception if it can't interpret the string, so
      // we don't need to test for that condition.

      // check for a requested time in the future
      if (reqStartTime > DateAndTime::getCurrentTime())
      {
        g_log.error() << "Requested start time in the future.  Resetting to current time." << std::endl;
        this->setPropertyValue("StartTime", DateAndTime::getCurrentTime().toISO8601String());
      }
    }


    // Get the listener (and start listening) as early as possible
    ILiveListener_sptr listener = this->getLiveListener();

    // TODO: Wait a bit to make sure something gets accumulated?

    LoadLiveData loadAlg;
    loadAlg.initialize();
    loadAlg.setChild(true);
    // Copy settings from THIS to LoadAlg
    loadAlg.copyPropertyValuesFrom(*this);
    // Force replacing the output workspace on the first run, to clear out old junk.
    loadAlg.setPropertyValue("AccumulationMethod", "Replace");
    // Give the listener directly to LoadLiveData (don't re-create it)
    loadAlg.setLiveListener(listener);

    // Run the LoadLiveData for the first time.
    loadAlg.executeAsChildAlg();

    // Copy the output workspace properties from LoadLiveData
    Workspace_sptr outWS = loadAlg.getProperty("OutputWorkspace");
    this->setProperty("OutputWorkspace", outWS);
    Workspace_sptr accumWS = loadAlg.getProperty("AccumulationWorkspace");
    this->setProperty("AccumulationWorkspace", accumWS);


    double UpdateEvery = this->getProperty("UpdateEvery");
    if (UpdateEvery > 0)
    {
      // Create the MonitorLiveData but DO NOT make a AlgorithmProxy to it
      IAlgorithm_sptr algBase = AlgorithmManager::Instance().create("MonitorLiveData", -1, false);
      MonitorLiveData * monitorAlg = dynamic_cast<MonitorLiveData*>(algBase.get());

      if (!monitorAlg)
        throw std::runtime_error("Error creating the MonitorLiveData algorithm");

      // Copy settings from THIS to monitorAlg
      monitorAlg->initialize();
      monitorAlg->copyPropertyValuesFrom(*this);
      monitorAlg->setProperty("UpdateEvery", UpdateEvery);

      // Give the listener directly to LoadLiveData (don't re-create it)
      monitorAlg->setLiveListener(listener);

      // Launch asyncronously
      monitorAlg->executeAsync();
    }

  }



} // namespace Mantid
} // namespace DataHandling
