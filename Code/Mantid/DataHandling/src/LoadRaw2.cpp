//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/XMLlogfile.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "LoadRaw/isisraw2.h"

#include <boost/shared_ptr.hpp>
#include "Poco/Path.h"
#include <cmath>
#include <iostream>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadRaw2)

    using namespace Kernel;
    using namespace API;

    /// Constructor
    LoadRaw2::LoadRaw2() :
      Algorithm(), isisRaw(new ISISRAW2), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(unSetInt)
    {}
    
    LoadRaw2::~LoadRaw2()
    {}

    /// Initialisation method.
    void LoadRaw2::init()
    {
      // Extension checking is not case sensitive
      // MG 20/07/09: I've had to change these extensions so that the native Windows file dialog can recognise
      // the file types correctly
      std::vector<std::string> exts;
      exts.push_back("raw");
      exts.push_back("s*");
      //exts.push_back("sav");
      //exts.push_back("s[0-9][0-9]");

      declareProperty("Filename","",new FileValidator(exts), 
        "The name of the RAW file to read, including its full or relative\n"
        "path. (N.B. case sensitive if running on Linux).");
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output),
        "The name of the workspace that will be created, filled with the\n"
        "read-in data and stored in the Analysis Data Service.  If the input\n"
        "RAW file contains multiple periods higher periods will be stored in\n"
        "separate workspaces called OutputWorkspace_PeriodNo.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(1);
      declareProperty("SpectrumMin",1, mustBePositive, "The index number of the first spectrum to read.  Only used if\n"
        "spectrum_max is set.");
      declareProperty("SpectrumMax",unSetInt, mustBePositive->clone(), "The number of the last spectrum to read. Only used if explicitly\n"
        "set.");

      declareProperty(new ArrayProperty<int>("SpectrumList"), "A comma-separated list of individual spectra to read.  Only used if\n"
        "explicitly set.");
      m_cache_options.push_back("If Slow");
      m_cache_options.push_back("Always");
      m_cache_options.push_back("Never");
      declareProperty("Cache","If Slow",new ListValidator(m_cache_options));

    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     *
     *  @throw Exception::FileError If the RAW file cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadRaw2::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");

      //ISISRAW iraw(NULL);
      FILE* file = fopen(m_filename.c_str(),"rb");
      if (file == NULL)
      {
        g_log.error("Unable to open file " + m_filename);
        throw Exception::FileError("Unable to open File:" , m_filename);
      }
      isisRaw->ioRAW(file, true);
      const std::string title(isisRaw->hdr.hd_title);
      g_log.information("**** Run title: "+title+ "***");

      // Read in the number of spectra in the RAW file
      m_numberOfSpectra = isisRaw->t_nsp1;
      // Read the number of periods in this file
      m_numberOfPeriods = isisRaw->t_nper;
      // Read the number of time channels (i.e. bins) from the RAW file
      const int channelsPerSpectrum = isisRaw->t_ntc1;
      // Read in the time bin boundaries
      const int lengthIn = channelsPerSpectrum + 1;

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      // Calculate the size of a workspace, given its number of periods & spectra to read
      int total_specs;
      if( m_interval || m_list)
      {
        if (m_interval)
        {
          total_specs = (m_spec_max-m_spec_min+1);
          m_spec_max += 1;
        }
        else
            total_specs = 0;

        if (m_list)
        {
            if (m_interval)
            {
                for(std::vector<int>::iterator it=m_spec_list.begin();it!=m_spec_list.end();)
                    if (*it >= m_spec_min && *it <m_spec_max)
                    {
                        it = m_spec_list.erase(it);
                    }
                    else
                        it++;

            }
            if (m_spec_list.size() == 0) m_list = false;
            total_specs += m_spec_list.size();
        }
      }
      else
      {
        total_specs = m_numberOfSpectra;
        // In this case want all the spectra, but zeroth spectrum is garbage so go from 1 to NSP1
        m_spec_min = 1;
        m_spec_max = m_numberOfSpectra + 1;
      }

      // If there is not enough memory use ManagedRawFileWorkspace2D.
      if (m_numberOfPeriods == 1 && MemoryManager::Instance().goForManagedWorkspace(total_specs,lengthIn,channelsPerSpectrum) &&
          total_specs == m_numberOfSpectra)
      {
        const std::string cache_option = getPropertyValue("Cache");
        int option = find(m_cache_options.begin(),m_cache_options.end(),cache_option) - m_cache_options.begin();
        DataObjects::Workspace2D_sptr localWorkspace = 
          DataObjects::Workspace2D_sptr(new ManagedRawFileWorkspace2D(m_filename, option));
        progress(0.,"Reading raw file...");
        runLoadInstrument(localWorkspace );
        runLoadMappingTable(localWorkspace );
        runLoadLog(localWorkspace );
        localWorkspace->getSample()->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
        for (int i = 0; i < m_numberOfSpectra; ++i)
          localWorkspace->getAxis(1)->spectraNo(i)= i+1;
        populateInstrumentParameters(localWorkspace);
        setProperty("OutputWorkspace",localWorkspace);
        return;
      }

      float* timeChannels = new float[lengthIn];
      isisRaw->getTimeChannels(timeChannels, lengthIn);
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<std::vector<double> > timeChannelsVec
                          (new std::vector<double>(timeChannels, timeChannels + lengthIn));

      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();

      Progress pr(this,0.,1.,total_specs * m_numberOfPeriods);

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
               (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
      localWorkspace->setTitle(title);
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      //if (dhdr.d_comp == 0) throw std::runtime_error("Oops..");
      // Loop over the number of periods in the raw file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

        if ( period > 0 )
        {
            localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                (WorkspaceFactory::Instance().create(localWorkspace));
            localWorkspace->newSample();
            //localWorkspace->newInstrumentParameters(); ????
        }

        isisRaw->skipData(period*(m_numberOfSpectra+1));
        int counter = 0;
        for (int i = 1; i <= m_numberOfSpectra; ++i)
        {
            int histToRead = i + period*(m_numberOfSpectra+1);
            if ((i >= m_spec_min && i < m_spec_max) ||
                (m_list && find(m_spec_list.begin(),m_spec_list.end(),i) != m_spec_list.end()))
            {
                isisRaw->readData(histToRead);
                // Copy the data into the workspace vector, discarding the 1st entry, which is rubbish
                // But note that the last (overflow) bin is kept
                MantidVec& Y = localWorkspace->dataY(counter);
                Y.assign(isisRaw->dat1 + 1, isisRaw->dat1 + lengthIn);
                // Fill the vector for the errors, containing sqrt(count)
                MantidVec& E = localWorkspace->dataE(counter);
                std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
                // Set the X vector pointer and spectrum number
                localWorkspace->setX(counter, timeChannelsVec);
                localWorkspace->getAxis(1)->spectraNo(counter)= i;
                // NOTE: Raw numbers go straight into the workspace
                //     - no account taken of bin widths/units etc.
                ++counter;
                pr.report();
            }
            else
            {
                isisRaw->skipData(histToRead);
            }
        }

        // Just a sanity check
        assert(counter == total_specs);

        std::string outputWorkspace = "OutputWorkspace";
        if (period == 0)
        {
          // Only run the sub-algorithms once
          runLoadInstrument(localWorkspace );
          runLoadMappingTable(localWorkspace );
          runLoadLog(localWorkspace );
          // Set the total proton charge for this run
          // (not sure how this works for multi_period files)
          localWorkspace->getSample()->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
        }
        else   // We are working on a higher period of a multiperiod raw file
        {
          // Create a WorkspaceProperty for the new workspace of a higher period
          // The workspace name given in the OutputWorkspace property has _periodNumber appended to it
          //                (for all but the first period, which has no suffix)
          std::stringstream suffix;
          suffix << (period+1);
          outputWorkspace += suffix.str();
          std::string WSName = localWSName + "_" + suffix.str();
          declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outputWorkspace,WSName,Direction::Output));
          g_log.information() << "Workspace " << WSName << " created. \n";
          runLoadLog(localWorkspace,period+1);
        }

        // check if values stored in logfiles should be used to define parameters of the instrument
        populateInstrumentParameters(localWorkspace);

        // Assign the result to the output workspace property
        setProperty(outputWorkspace,localWorkspace);

      } // loop over periods

      // Clean up
      delete[] timeChannels;
      //delete[] spectrum;
      fclose(file);
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadRaw2::checkOptionalProperties()
    {
      //read in the settings passed to the algorithm
      m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");

      m_list = !m_spec_list.empty();
      m_interval = m_spec_max != unSetInt;
      if ( m_spec_max == unSetInt ) m_spec_max = 1; 

      // Check validity of spectra list property, if set
      if ( m_list )
      {
        m_list = true;
        if (m_spec_list.size() == 0)
        {
            m_list = false;
        }
        else
        {
            const int minlist = *min_element(m_spec_list.begin(),m_spec_list.end());
            const int maxlist = *max_element(m_spec_list.begin(),m_spec_list.end());
            if ( maxlist > m_numberOfSpectra || minlist <= 0)
            {
                g_log.error("Invalid list of spectra");
                throw std::invalid_argument("Inconsistent properties defined");
            }
        }
      }

      // Check validity of spectra range, if set
      if ( m_interval )
      {
        m_interval = true;
        m_spec_min = getProperty("SpectrumMin");
        if ( m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra )
        {
          g_log.error("Invalid Spectrum min/max properties");
          throw std::invalid_argument("Inconsistent properties defined");
        }
      }
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadRaw2::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
      if ( directoryName.empty() )
      {
	// This is the assumed deployment directory for IDFs, where we need to be relative to the
	// directory of the executable, not the current working directory.
	directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();
      }

      //const int stripPath = m_filename.find_last_of("\\/");
      //std::string instrumentID = m_filename.substr(stripPath+1,3);  // get the 1st 3 letters of filename part
      
      std::string instrumentID = isisRaw->i_inst; // get the instrument name
      size_t i = instrumentID.find_first_of(' '); // cut trailing spaces
      if (i != std::string::npos) instrumentID.erase(i);

      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      loadInst->setPropertyValue("Filename", fullPathIDF);
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
      }

      // If loading instrument definition file fails, run LoadInstrumentFromRaw instead
      if ( ! loadInst->isExecuted() )
      {
        runLoadInstrumentFromRaw(localWorkspace);
      }
    }

    /// Run LoadInstrumentFromRaw as a sub-algorithm (only if loading from instrument definition file fails)
    void LoadRaw2::runLoadInstrumentFromRaw(DataObjects::Workspace2D_sptr localWorkspace)
    {
      g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
        << "the instrument from raw data file.\n";

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromRaw");
      loadInst->setPropertyValue("Filename", m_filename);
      // Set the workspace property to be the same one filled above
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run LoadInstrumentFromRaw sub-algorithm");
      }

      if ( ! loadInst->isExecuted() ) g_log.error("No instrument definition loaded");
    }

    /// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
    void LoadRaw2::runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Now determine the spectra to detector map calling sub-algorithm LoadMappingTable
      // There is a small penalty in re-opening the raw file but nothing major.
      IAlgorithm_sptr loadmap= createSubAlgorithm("LoadMappingTable");
      loadmap->setPropertyValue("Filename", m_filename);
      loadmap->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);
      try
      {
        loadmap->execute();
      }
      catch (std::runtime_error&)
      {
    	  g_log.error("Unable to successfully execute LoadMappingTable sub-algorithm");
      }

      if ( ! loadmap->isExecuted() ) g_log.error("LoadMappingTable sub-algorithm is not executed");
    }

    /// Run the LoadLog sub-algorithm
    void LoadRaw2::runLoadLog(DataObjects::Workspace2D_sptr localWorkspace, int period)
    {
      IAlgorithm_sptr loadLog = createSubAlgorithm("LoadLog");
      // Pass through the same input filename
      loadLog->setPropertyValue("Filename",m_filename);
      // Set the workspace property to be the same one filled above
      loadLog->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);
      loadLog->setProperty("Period",period);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadLog->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run LoadLog sub-algorithm");
      }

      if ( ! loadLog->isExecuted() ) g_log.error("Unable to successfully run LoadLog sub-algorithm");
    }

    double LoadRaw2::dblSqrt(double in)
    {
      return sqrt(in);
    }

    /** Add parameters to the instrument parameter map that are defined in instrument
     *  definition file and for which logfile data are available
     *
     *  @param localWorkspace A pointer to a workspace
     */
    void LoadRaw2::populateInstrumentParameters(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Get instrument and sample

      boost::shared_ptr<Instrument> instrument;
      boost::shared_ptr<Sample> sample;
      instrument = localWorkspace->getBaseInstrument();
      sample = localWorkspace->getSample();


      // Get the data in the logfiles associated with the raw data

      const std::vector<Kernel::Property*>& logfileProp = sample->getLogData();


      // Get pointer to parameter map that we may add parameters to and information about
      // the parameters that my be specified in the instrument definition file (IDF)

      Geometry::ParameterMap& paramMap = localWorkspace->instrumentParameters();
      std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >& paramInfoFromIDF = instrument->getLogfileCache();


      // iterator to browse throw the multimap: paramInfoFromIDF

      std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> > :: const_iterator it;
      std::pair<std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >::iterator,
        std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >::iterator> ret;


      // loop over all logfiles and see if any of these are associated with parameters in the
      // IDF

      unsigned int N = logfileProp.size();
      for (unsigned int i = 0; i < N; i++)
      {
        // Remove the path, the run number and extension from logfile filename

        std::string logFilename = logfileProp[i]->name();
        std::string filenamePart = Poco::Path(logFilename).getFileName(); // get filename part only
        if (filenamePart.size() > 4 && filenamePart.rfind('.') == filenamePart.size() - 4)
        {
            filenamePart = filenamePart.erase(filenamePart.size()-4, filenamePart.size()); // remove extension
            filenamePart = filenamePart.substr(9); // remove front run number part
        }


        // See if filenamePart matches any logfile-IDs in IDF. If this add parameter to parameter map

        ret = paramInfoFromIDF.equal_range(filenamePart);
        for (it=ret.first; it!=ret.second; ++it)
        {
          double value = ((*it).second)->createParamValue(static_cast<Kernel::TimeSeriesProperty<double>*>(logfileProp[i]));

          // special case if parameter name is "x", "y" or "z" and "rot"

          std::string paramN = ((*it).second)->m_paramName;
          if ( paramN.compare("x")==0 || paramN.compare("y")==0 || paramN.compare("z")==0 )
            paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
          else if ( paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )
            paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
          else
            paramMap.addDouble(((*it).second)->m_component, paramN, value);
        }
      }
    }


  } // namespace DataHandling
} // namespace Mantid
