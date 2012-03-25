/*WIKI* 


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/BoundedValidator.h"
#include <cmath>
#include <Poco/Path.h>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadEmptyInstrument)
    
    /// Sets documentation strings for this algorithm
    void LoadEmptyInstrument::initDocs()
    {
      this->setWikiSummary(" Loads an Instrument Definition File ([[InstrumentDefinitionFile|IDF]]) into a [[workspace]], with the purpose of being able to visualise an instrument without requiring to read in a raw datafile first. The name of the algorithm refers to the fact that an instrument is loaded into a workspace but without any real data - hence the reason for referring to it as an 'empty' instrument.  For more information on IDFs see [[InstrumentDefinitionFile]]. ");
      this->setOptionalMessage("Loads an Instrument Definition File (IDF) into a workspace, with the purpose of being able to visualise an instrument without requiring to read in a raw datafile first. The name of the algorithm refers to the fact that an instrument is loaded into a workspace but without any real data - hence the reason for referring to it as an 'empty' instrument.  For more information on IDFs see InstrumentDefinitionFile.");
    }
    

    using namespace Kernel;
    using namespace API;
    using namespace Geometry;
    using namespace DataObjects;

    /// Empty default constructor
    LoadEmptyInstrument::LoadEmptyInstrument() : Algorithm()
    {}

    /// Initialisation method.
    void LoadEmptyInstrument::init()
    {
      declareProperty(new FileProperty("Filename","", FileProperty::Load, ".xml"),
		      "The filename (including its full or relative path) of an instrument\n"
		      "definition file");
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "The name of the workspace in which to store the imported instrument" );
      
      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      mustBePositive->setLower(0.0);
      declareProperty("DetectorValue",1.0, mustBePositive,
        "This value affects the colour of the detectors in the instrument\n"
        "display window (default 1)" );
      declareProperty("MonitorValue",2.0, mustBePositive,
        "This value affects the colour of the monitors in the instrument\n"
        "display window (default 2)");

      declareProperty(new PropertyWithValue<bool>("MakeEventWorkspace", false),
          "Set to True to create an EventWorkspace (with no events) instead of a Workspace2D.");
    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     * 
     *  @throw Exception::FileError If the RAW file cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadEmptyInstrument::exec()
    {
      // Get other properties
      const double detector_value = getProperty("DetectorValue");
      const double monitor_value = getProperty("MonitorValue");

      // load the instrument into this workspace
      MatrixWorkspace_sptr ws = this->runLoadInstrument();
      Instrument_const_sptr instrument = ws->getInstrument();

      // Get detectors stored in instrument and create dummy c-arrays for the purpose
      // of calling method of SpectraDetectorMap 
      std::map<detid_t, IDetector_const_sptr> detCache;

      //FIXME: Use GetDetectorID's here since it'll be way faster.
      instrument->getDetectors(detCache);
      const int64_t number_spectra = static_cast<int64_t>(detCache.size());
      
      bool MakeEventWorkspace = getProperty("MakeEventWorkspace");
      
      MatrixWorkspace_sptr outWS;

      if (MakeEventWorkspace)
      {
        //Make a brand new EventWorkspace
        EventWorkspace_sptr localWorkspace = boost::dynamic_pointer_cast<EventWorkspace>(
            API::WorkspaceFactory::Instance().create("EventWorkspace", number_spectra, 2, 1));
        //Copy geometry over.
        API::WorkspaceFactory::Instance().initializeFromParent(ws, localWorkspace, true);

        // Cast to matrix WS
        outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(localWorkspace);
      }
      else
      {
        // Now create the outputworkspace and copy over the instrument object
        DataObjects::Workspace2D_sptr localWorkspace =
          boost::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create(ws,number_spectra,2,1));

        outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(localWorkspace);
      }

      outWS->rebuildSpectraMapping( true /* include monitors */);


      // ---- Set the values ----------
      if (!MakeEventWorkspace)
      {
        MantidVecPtr x,v,v_monitor;
        x.access().resize(2); x.access()[0]=1.0; x.access()[1]=2.0;
        v.access().resize(1); v.access()[0]=detector_value;
        v_monitor.access().resize(1); v_monitor.access()[0]=monitor_value;

        for (size_t i=0; i < outWS->getNumberHistograms(); i++)
        {
          IDetector_const_sptr det = outWS->getDetector(i);
          if ( det->isMonitor() )
            outWS->setData(i, v_monitor, v_monitor);
          else
            outWS->setData(i, v, v);
        }
      }
      // Save in output
      this->setProperty("OutputWorkspace", outWS);

    }
    
    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    API::MatrixWorkspace_sptr LoadEmptyInstrument::runLoadInstrument()
    {
      const std::string filename = getPropertyValue("Filename");
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();
      const std::string::size_type stripPath = filename.find_last_of("\\/");

      std::string fullPathIDF;
      if (stripPath != std::string::npos)
      {
        fullPathIDF = filename;   // since if path already provided don't modify m_filename
      }
      else
      {
        //std::string instrumentID = m_filename.substr(stripPath+1);
        fullPathIDF = directoryName + "/" + filename;
      }

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument",0,1);
      loadInst->setPropertyValue("Filename", fullPathIDF);
      MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,2,1);
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",ws);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run LoadInstrument sub-algorithm");
      }
      
      return ws;
    }

  } // namespace DataHandling
} // namespace Mantid
