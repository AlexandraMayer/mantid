//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadMuonNexus2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidNexus/NexusClasses.h"

#include "Poco/Path.h"
#include <boost/shared_ptr.hpp>

#include <cmath>
#include <numeric>


namespace Mantid
{
  namespace NeXus
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadMuonNexus2)

    using namespace Kernel;
    using namespace API;

    /// Empty default constructor
    LoadMuonNexus2::LoadMuonNexus2() : LoadMuonNexus()
    {}

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadMuonNexus2::exec()
    {
      // Create the root Nexus class
      NXRoot root(getPropertyValue("Filename"));

      int iEntry = getProperty("EntryNumber");
      if (iEntry >= static_cast<int>(root.groups().size()) )
      {
        throw std::invalid_argument("EntryNumber is out of range");
      }

      // Open the data entry
      std::string entryName = root.groups()[iEntry].nxname;
      NXEntry entry = root.openEntry(entryName);

      NXInfo info = entry.getDataSetInfo("definition");
      if (info.stat == NX_ERROR)
      {
        info = entry.getDataSetInfo("analysis");
        if (info.stat == NX_OK && entry.getString("analysis") == "muonTD")
        {
          LoadMuonNexus::exec();
          return;
        }
        else
        {
          throw std::runtime_error("Unknown Muon Nexus file format");
        }
      }
      else
      {
        std::string definition = entry.getString("definition");
        if (info.stat == NX_ERROR || entry.getString("definition") != "pulsedTD")
        {
          throw std::runtime_error("Unknown Muon Nexus file format");
        }
      }

      // Read in the instrument name from the Nexus file
      m_instrument_name = entry.getString("instrument/name");
      
      // Read the number of periods in this file
      m_numberOfPeriods = entry.getInt("run/number_periods");

      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
      boost::shared_ptr<IInstrument> instrument;
      //-      boost::shared_ptr<SpectraDetectorMap> specMap;
      boost::shared_ptr<Sample> sample;

      std::string detectorName;
      // Only the first NXdata found
      for(unsigned int i=0; i< entry.groups().size(); i++)
      {
        std::string className = entry.groups()[i].nxclass;
        if (className == "NXdata")
        {
          detectorName = entry.groups()[i].nxname;
          break;
        }
      }
      NXData dataGroup = entry.openNXData(detectorName);

      NXInt spectrum_index = dataGroup.openNXInt("spectrum_index");
      spectrum_index.load();
      m_numberOfSpectra = spectrum_index.dim0();

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      NXFloat raw_time = dataGroup.openNXFloat("raw_time");
      raw_time.load();
      int nBins = raw_time.dim0();
      std::vector<double> timeBins;
      timeBins.assign(raw_time(),raw_time()+nBins);
      timeBins.push_back(raw_time[nBins-1]+raw_time[1]-raw_time[0]);

      // Calculate the size of a workspace, given its number of periods & spectra to read
      int total_specs;
      if( m_interval || m_list)
      {
        total_specs = m_spec_list.size();
        if (m_interval)
        {
          total_specs += (m_spec_max-m_spec_min+1);
          m_spec_max += 1;
        }
      }
      else
      {
        total_specs = m_numberOfSpectra;
        // for nexus return all spectra
        m_spec_min = 0; // changed to 0 for NeXus, was 1 for Raw
        m_spec_max = m_numberOfSpectra;  // was +1?
      }

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,nBins+1,nBins));
      // Set the unit on the workspace to TOF
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      localWorkspace->setYUnit("Counts");

      //g_log.error()<<" number of perioids= "<<m_numberOfPeriods<<std::endl;
      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
      if(m_numberOfPeriods>1)
      {	
        if(wsGrpSptr)wsGrpSptr->add(localWSName);
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
      }

      NXInt period_index = dataGroup.openNXInt("period_index");
      period_index.load();

      NXInt counts = dataGroup.openIntData();
      counts.load();

      API::Progress progress(this,0.,1.,m_numberOfPeriods * total_specs);
      // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

        if (period == 0)
        {
          // Only run the sub-algorithms once
          runLoadInstrument(localWorkspace );
          localWorkspace->mutableSpectraMap().populate(spectrum_index(),spectrum_index(),m_numberOfSpectra);
          loadLogs(localWorkspace, entry, period);
        }
        else   // We are working on a higher period of a multiperiod raw file
        {
          localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create(localWorkspace));
        }

        std::string outws("");
        if(m_numberOfPeriods>1)
        {
          std::string outputWorkspace = "OutputWorkspace";
          std::stringstream suffix;
          suffix << (period+1);
          outws =outputWorkspace+"_"+suffix.str();
          std::string WSName = localWSName + "_" + suffix.str();
          declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outws,WSName,Direction::Output));
          if(wsGrpSptr)wsGrpSptr->add(WSName);
        }


        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
          loadData(counts,timeBins,counter,period,i,localWorkspace);
          localWorkspace->getAxis(1)->spectraNo(counter) = spectrum_index[i];
          counter++;
          progress.report();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            int k = m_spec_list[i];
            loadData(counts,timeBins,counter,period,k,localWorkspace);
            localWorkspace->getAxis(1)->spectraNo(counter) = spectrum_index[k];
            counter++;
            progress.report();
          }
        }
        // Just a sanity check
        assert(counter == total_specs);

        bool autogroup = getProperty("AutoGroup");

        if (autogroup)
        {
          g_log.warning("Autogrouping is not implemented for muon NeXus version 2 files");
        }

        // Assign the result to the output workspace property
        if(m_numberOfPeriods>1)
          setProperty(outws,localWorkspace);
        else
        {
          setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(localWorkspace));
        }


      } // loop over periods

    //  // Clean up
    //  delete[] timeChannels;
    }

//    /// Validates the optional 'spectra to read' properties, if they have been set
//    void LoadMuonNexus2::checkOptionalProperties()
//    {
//      //read in the settings passed to the algorithm
//      m_spec_list = getProperty("SpectrumList");
//      m_spec_max = getProperty("SpectrumMax");
//      //Are we using a list of spectra or all the spectra in a range?
//      m_list = !m_spec_list.empty();
//      m_interval = (m_spec_max != EMPTY_INT());
//      if ( m_spec_max == EMPTY_INT() ) m_spec_max = 0;
//
//      // Check validity of spectra range, if set
//      if ( m_interval )
//      {
//        m_spec_min = getProperty("SpectrumMin");
//        if ( m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra )
//        {
//          g_log.error("Invalid Spectrum min/max properties");
//          throw std::invalid_argument("Inconsistent properties defined"); 
//        }
//      }
//
//      // Check validity of spectra list property, if set
//      if ( m_list )
//      {
//        const int minlist = *min_element(m_spec_list.begin(),m_spec_list.end());
//        const int maxlist = *max_element(m_spec_list.begin(),m_spec_list.end());
//        if ( maxlist > m_numberOfSpectra || minlist == 0)
//        {
//          g_log.error("Invalid list of spectra");
//          throw std::invalid_argument("Inconsistent properties defined"); 
//        } 
//      }
//
//    }

    /** loadData
     *  Load the counts data from an NXInt into a workspace
     */
    void LoadMuonNexus2::loadData(const NXInt& counts,const std::vector<double>& timeBins,int wsIndex,
      int period,int spec,API::MatrixWorkspace_sptr localWorkspace)
    {
      MantidVec& X = localWorkspace->dataX(wsIndex);
      MantidVec& Y = localWorkspace->dataY(wsIndex);
      MantidVec& E = localWorkspace->dataE(wsIndex);
      int nBins = counts.dim2();
      assert( nBins+1 == static_cast<int>(timeBins.size()) );
      X.assign(timeBins.begin(),timeBins.end());
      int *data = &counts(period,spec,0);
      Y.assign(data,data+nBins);
      typedef double (*uf)(double);
      uf dblSqrt = std::sqrt;
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadMuonNexus2::runLoadInstrument(API::MatrixWorkspace_sptr localWorkspace)
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
      // For Nexus, Instrument name given by MuonNexusReader from Nexus file
      std::string instrumentID = m_instrument_name; //m_filename.substr(stripPath+1,3);  // get the 1st 3 letters of filename part
      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      bool executionSuccessful(true);
      try
      {
        loadInst->setPropertyValue("Filename", fullPathIDF);
        loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
        loadInst->execute();
      }
      catch( std::invalid_argument&)
      {
        g_log.information("Invalid argument to LoadInstrument sub-algorithm");
        executionSuccessful = false;
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
        executionSuccessful = false;
      }

      // If loading instrument definition file fails, run LoadInstrumentFromNexus instead
      // This does not work at present as the example files do not hold the necessary data
      // but is a place holder. Hopefully the new version of Nexus Muon files should be more
      // complete.
      //if ( ! loadInst->isExecuted() )
      //{
      //    runLoadInstrumentFromNexus(localWorkspace);
      //}
    }

    /**  Load logs from Nexus file. Logs are expected to be in
    *   /run/sample group of the file. 
    *   @param ws The workspace to load the logs to.
    *   @param entry The Nexus entry
    *   @param period The period of this workspace
    */
    void LoadMuonNexus2::loadLogs(API::MatrixWorkspace_sptr ws, NXEntry & entry,int period)
    {

      std::string start_time = entry.getString("start_time");

      std::string sampleName = entry.getString("sample/name");
      NXMainClass runlogs = entry.openNXClass<NXMainClass>("sample");
      ws->mutableSample().setName(sampleName);

      for(std::vector<NXClassInfo>::const_iterator it=runlogs.groups().begin();it!=runlogs.groups().end();it++)
      {
        NXLog nxLog = runlogs.openNXLog(it->nxname);
        Kernel::Property* logv = nxLog.createTimeSeries(start_time);
        if (!logv) continue;
        ws->mutableSample().addLogData(logv);
      }

      ws->populateInstrumentParameters();
    }
    
  } // namespace DataHandling
} // namespace Mantid
