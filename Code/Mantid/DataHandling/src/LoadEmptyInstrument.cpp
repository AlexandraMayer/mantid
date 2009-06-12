//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"

#include "Poco/Path.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadEmptyInstrument)

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& LoadEmptyInstrument::g_log = Logger::get("LoadEmptyInstrument");

    /// Empty default constructor
    LoadEmptyInstrument::LoadEmptyInstrument() : Algorithm()
    {}

    /// Initialisation method.
    void LoadEmptyInstrument::init()
    {
      std::vector<std::string> exts;
      exts.push_back("XML");
      exts.push_back("xml");			
      declareProperty("Filename","",new FileValidator(exts),
        "The filename (including its full or relative path) of an ISIS\n"
        "instrument defintion file");
      declareProperty(
        new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output),
        "The name of the workspace in which to store the imported instrument" );
      
      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0.0);
      declareProperty("detector_value",1.0, mustBePositive,
        "This value affects the colour of the detectors in the instrument\n"
        "display window (default 1)" );
      declareProperty("monitor_value",2.0, mustBePositive->clone(),
        "This value affects the colour of the monitors in the instrument\n"
        "display window (default 2)");
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
      const double detector_value = getProperty("detector_value");
      const double monitor_value = getProperty("monitor_value");

      // load the instrument into this workspace
      IInstrument_sptr instrument = this->runLoadInstrument();

      // Get detectors stored in instrument and create dummy c-arrays for the purpose
      // of calling method of SpectraDetectorMap 
      const std::map<int, Geometry::IDetector_sptr> detCache = instrument->getDetectors();
      const int number_spectra = static_cast<int>(detCache.size());
      
      // Now create the outputworkspace and copy over the instrument object
      DataObjects::Workspace2D_sptr localWorkspace = 
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D",number_spectra,2,1));
      localWorkspace->setInstrument(instrument);
      
      int *spec = new int[number_spectra];
      int *udet = new int[number_spectra];

      std::map<int, Geometry::IDetector_sptr>::const_iterator it;
      int counter = 0;
      for ( it = detCache.begin(); it != detCache.end(); ++it )
      {
        counter++;
        spec[counter-1] = counter;    // have no feeling of how best to number these spectra
                                      // and sure whether the way it is done here is the best way...
        udet[counter-1] = it->first;
      }

      localWorkspace->mutableSpectraMap().populate(spec,udet,number_spectra);

      counter = 0;
      DataObjects::Histogram1D::RCtype x,v,v_monitor;
      x.access().resize(2); x.access()[0]=1.0; x.access()[1]=2.0;
      v.access().resize(1); v.access()[0]=detector_value;
      v_monitor.access().resize(1); v_monitor.access()[0]=monitor_value;

      for ( it = detCache.begin(); it != detCache.end(); ++it )
      {
        if ( (it->second)->isMonitor() )
          localWorkspace->setData(counter, v_monitor, v_monitor);
        else
          localWorkspace->setData(counter, v, v);
        localWorkspace->setX(counter, x);
        localWorkspace->getAxis(1)->spectraNo(counter)= counter+1;  // Not entirely sure if this 100% ok
        ++counter;
      }

      setProperty("OutputWorkspace",localWorkspace);
        
      
      // Clean up
      delete[] spec;
      delete[] udet;
    }


    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    API::IInstrument_sptr LoadEmptyInstrument::runLoadInstrument()
    {
      const std::string filename = getPropertyValue("Filename");
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");      
      if ( directoryName.empty() )
      {
        // This is the assumed deployment directory for IDFs, where we need to be relative to the
        // directory of the executable, not the current working directory.
        directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();  
      }
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
      MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("WorkspaceSingleValue",1,1,1);
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
      
      return ws->getInstrument();
    }



  } // namespace DataHandling
} // namespace Mantid
