//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadISISNexus2.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/LogParser.h"
#include "MantidAPI/XMLlogfile.h"

#include "Poco/Path.h"

#include <cmath>
#include <sstream>
#include <cctype>
#include <functional>
#include <algorithm>

namespace Mantid
{
  namespace NeXus
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadISISNexus2)

    using namespace Kernel;
    using namespace API;

    /// Empty default constructor
    LoadISISNexus2::LoadISISNexus2() : 
    Algorithm(), m_filename(), m_instrument_name(), m_samplename(), m_numberOfSpectra(0), 
      m_numberOfPeriods(0), m_numberOfChannels(0), m_have_detector(false), m_spec_min(0), m_spec_max(EMPTY_INT()), m_spec_list(), 
      m_entrynumber(0), m_range_supplied(true), m_tof_data(), m_proton_charge(0.),
      m_spec(), m_monitors(), m_progress()
    {}

    /// Initialisation method.
    void LoadISISNexus2::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".nxs");
      exts.push_back(".n*");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load" );
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("SpectrumMin", 0, mustBePositive);
      declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
      declareProperty(new ArrayProperty<int>("SpectrumList"));
      declareProperty("EntryNumber", 0, mustBePositive->clone(),
        "The particular entry number to read (default: Load all workspaces and creates a workspace group)");
    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadISISNexus2::exec()
    {
      // Create the root Nexus class
      NXRoot root(getPropertyValue("Filename"));

      // Open the raw data group 'raw_data_1'
      NXEntry entry = root.openEntry("raw_data_1");

      // Read in the instrument name from the Nexus file
      m_instrument_name = entry.getString("name");

      //Test if we have a detector block
      int ndets(0);
      try
      {
        NXClass det_class = entry.openNXGroup("detector_1");
        NXInt spectrum_index = det_class.openNXInt("spectrum_index");
        spectrum_index.load();
        ndets = spectrum_index.dim0();
        // We assume that this spectrum list increases monotonically
        m_spec = spectrum_index.sharedBuffer();
        m_spec_end = m_spec.get() + ndets;
        m_have_detector = true;
      }
      catch(std::runtime_error &)
      {
        ndets = 0;
      }

      NXInt nsp1 = entry.openNXInt("isis_vms_compat/NSP1");
      nsp1.load();
      NXInt udet = entry.openNXInt("isis_vms_compat/UDET");
      udet.load();
      NXInt spec = entry.openNXInt("isis_vms_compat/SPEC");
      spec.load();

      //Pull out the monitor blocks, if any exist
      int nmons(0);
      for(std::vector<NXClassInfo>::const_iterator it = entry.groups().begin(); 
        it != entry.groups().end(); ++it) 
      {
        if (it->nxclass == "NXmonitor") // Count monitors
        {
          NXInt index = entry.openNXInt(std::string(it->nxname) + "/spectrum_index");
          index.load();
          m_monitors[*index()] = it->nxname;
          ++nmons;
        }
      }

      if( ndets == 0 && nmons == 0 )
      {
        g_log.error() << "Invalid NeXus structure, cannot find detector or monitor blocks.";
        throw std::runtime_error("Inconsistent NeXus file structure.");
      }

      if( ndets == 0 )
      {

        //Grab the number of channels
        NXInt chans = entry.openNXInt(m_monitors.begin()->second + "/data");
        m_numberOfPeriods = chans.dim0();
        m_numberOfSpectra = nmons;
        m_numberOfChannels = chans.dim2();
      }
      else 
      {
        NXData nxData = entry.openNXData("detector_1");
        NXInt data = nxData.openIntData();
        m_numberOfPeriods  = data.dim0();
        m_numberOfSpectra = nsp1[0];
        m_numberOfChannels = data.dim2();

        if( nmons > 0 && m_numberOfSpectra == data.dim1() )
        {
          m_monitors.clear();
        }
      }
      const int x_length = m_numberOfChannels + 1;

      // Check input is consistent with the file, throwing if not
      checkOptionalProperties();

      // Check which monitors need loading
      const bool empty_spec_list = m_spec_list.empty(); 
      for( std::map<int, std::string>::iterator itr = m_monitors.begin(); itr != m_monitors.end(); )
      {
        int index = itr->first;
        std::vector<int>::iterator spec_it = std::find(m_spec_list.begin(), m_spec_list.end(), index);
        if( (!empty_spec_list && spec_it == m_spec_list.end()) ||
          (m_range_supplied && (index < m_spec_min || index > m_spec_max)) )
        {
          std::map<int, std::string>::iterator itr1 = itr;
          ++itr;
          m_monitors.erase(itr1);
        }
        // In the case that a monitor is in the spectrum list, we need to erase it from there
        else if ( !empty_spec_list && spec_it != m_spec_list.end() )
        {
          m_spec_list.erase(spec_it);
          ++itr;
        }
        else
        {
          ++itr;
        }
      }


      int total_specs(0);
      int list_size = static_cast<int>(m_spec_list.size());
      if( m_range_supplied )
      {
        //Inclusive range + list size
        total_specs = (m_spec_max - m_spec_min + 1) + list_size;
      }
      else
      {
        total_specs = m_spec_list.size() + m_monitors.size();
      }

      m_progress = boost::shared_ptr<API::Progress>(new Progress(this, 0.0, 1.0, total_specs * m_numberOfPeriods));

      DataObjects::Workspace2D_sptr local_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D", total_specs, x_length, m_numberOfChannels));
      // Set the units on the workspace to TOF & Counts
      local_workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      local_workspace->setYUnit("Counts");

      //Load instrument and other data once then copy it later
      m_progress->report("Loading instrument");
      runLoadInstrument(local_workspace);

      local_workspace->mutableSpectraMap().populate(spec(),udet(),udet.dim0());
      loadSampleData(local_workspace, entry);
      m_progress->report("Loading logs");
      loadLogs(local_workspace, entry);

      // Load first period outside loop
      m_progress->report("Loading data");
      if( ndets > 0 )
      {
        //Get the X data
        NXFloat timeBins = entry.openNXFloat("detector_1/time_of_flight");
        timeBins.load();
        m_tof_data.reset(new MantidVec(timeBins(), timeBins() + x_length));
      }
      int firstentry = (m_entrynumber > 0) ? m_entrynumber : 1;
      loadPeriodData(firstentry, entry, local_workspace); 

      if( m_numberOfPeriods > 1 && m_entrynumber == 0 )
      {
        API::WorkspaceGroup_sptr wksp_group(new WorkspaceGroup);      
        //This forms the name of the group
        const std::string base_name = getPropertyValue("OutputWorkspace") + "_";
        const std::string prop_name = "OutputWorkspace_";
        declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(prop_name + "1", base_name + "1", Direction::Output));
        wksp_group->add(getPropertyValue("OutputWorkspace"));
        wksp_group->add(base_name + "1");
        setProperty(prop_name + "1", local_workspace);

        for( int p = 2; p <= m_numberOfPeriods; ++p )
        {
          local_workspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create(local_workspace));
          std::ostringstream os;
          os << p;
          m_progress->report("Loading period " + os.str());
          loadPeriodData(p, entry, local_workspace);

          declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(prop_name + os.str(), base_name + os.str(), Direction::Output));
          wksp_group->add(base_name + os.str());
          setProperty(prop_name + os.str(), local_workspace);
        }
        // The group is the root property value
        setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(wksp_group));
      }
      else
      {
        setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(local_workspace));
      }

      // Clear off the member variable containers
      m_spec_list.clear();
      m_tof_data.reset();
      m_spec.reset();
      m_monitors.clear();
    }

    // Function object for remove_if STL algorithm
    namespace
    {
      //Check the numbers supplied are not in the range and erase the ones that are
      struct range_check
      {	
        range_check(int min, int max) : m_min(min), m_max(max) {}

        bool operator()(int x)
        {
          return (x >= m_min && x <= m_max);
        }

      private:
        int m_min;
        int m_max;
      };

    }

    /**
    * Check the validity of the optional properties of the algorithm
    */
    void LoadISISNexus2::checkOptionalProperties()
    {
      m_spec_min = getProperty("SpectrumMin");
      m_spec_max = getProperty("SpectrumMax");

      if( m_spec_min == 0 && m_spec_max == EMPTY_INT() )
      {
        m_range_supplied = false;
      }

      if( m_spec_min == 0 )
      {
        m_spec_min = 1;
      }

      if( m_spec_max == EMPTY_INT() )
      {
        m_spec_max = m_numberOfSpectra;
      }

      // Sanity check for min/max
      if( m_spec_min > m_spec_max )
      {
        g_log.error() << "Inconsistent range properties. SpectrumMin is larger than SpectrumMax." << std::endl;
        throw std::invalid_argument("Inconsistent range properties defined.");
      }

      if( m_spec_max > m_numberOfSpectra )
      {
        g_log.error() << "Inconsistent range property. SpectrumMax is larger than number of spectra: " 
          << m_numberOfSpectra << std::endl;
        throw std::invalid_argument("Inconsistent range properties defined.");
      }

      // Check the entry number
      m_entrynumber = getProperty("EntryNumber");
      if( m_entrynumber > m_numberOfPeriods || m_entrynumber < 0 )
      {
        g_log.error() << "Invalid entry number entered. File contains " << m_numberOfPeriods << " period. " 
          << std::endl;
        throw std::invalid_argument("Invalid entry number.");
      }
      if( m_numberOfPeriods == 1 )
      {
        m_entrynumber = 1;
      }

      //Check the list property
      m_spec_list = getProperty("SpectrumList");
      if( m_spec_list.empty() ) 
      {
        m_range_supplied = true;
        return;
      }

      // Sort the list so that we can check it's range
      std::sort(m_spec_list.begin(), m_spec_list.end());

      if( m_spec_list.back() > m_numberOfSpectra )
      {
        g_log.error() << "Inconsistent SpectraList property defined for a total of " << m_numberOfSpectra 
          << " spectra." << std::endl;
        throw std::invalid_argument("Inconsistent property defined");
      }

      //Check no negative numbers have been passed
      std::vector<int>::iterator itr = 
        std::find_if(m_spec_list.begin(), m_spec_list.end(), std::bind2nd(std::less<int>(), 0));
      if( itr != m_spec_list.end() )
      {
        g_log.error() << "Negative SpectraList property encountered." << std::endl;
        throw std::invalid_argument("Inconsistent property defined.");
      }

      range_check in_range(m_spec_min, m_spec_max);
      if( m_range_supplied )
      {
        m_spec_list.erase(remove_if(m_spec_list.begin(), m_spec_list.end(), in_range), m_spec_list.end());
      }


    }

    /**
    * Load a given period into the workspace
    * @param period The period number to load (starting from 1) 
    * @param entry The opened root entry node for accessing the monitor and data nodes
    * @param local_workspace The workspace to place the data in
    */
    void LoadISISNexus2::loadPeriodData(int period, NXEntry & entry, DataObjects::Workspace2D_sptr local_workspace)
    {
      int hist_index = m_monitors.size();
      int period_index(period - 1);

      if( m_have_detector )
      {
        NXData nxdata = entry.openNXData("detector_1");
        NXDataSetTyped<int> data = nxdata.openIntData();
        data.open();
        //Start with thelist members that are lower than the required spectrum
        const int * const spec_begin = m_spec.get();
        std::vector<int>::iterator min_end = m_spec_list.end();
        if( !m_spec_list.empty() )
        {
          // If we have a list, by now it is ordered so first pull in the range below the starting block range
          // Note the reverse iteration as we want the last one
          if( m_range_supplied )
          {
            min_end = std::find_if(m_spec_list.begin(), m_spec_list.end(), std::bind2nd(std::greater<int>(), m_spec_min));
          }

          for( std::vector<int>::iterator itr = m_spec_list.begin(); itr < min_end; ++itr )
          {
            // Load each
            int spectra_no = (*itr);
            // For this to work correctly, we assume that the spectrum list increases monotonically
            int filestart = std::lower_bound(spec_begin,m_spec_end,spectra_no) - spec_begin;
            m_progress->report("Loading data");
            loadBlock(data, 1, period_index, filestart, hist_index, spectra_no, local_workspace);
          }
        }    

        if( m_range_supplied )
        {
          // When reading in blocks we need to be careful that the range is exactly divisible by the blocksize
          // and if not have an extra read of the left overs
          const int blocksize = 8;
          const int rangesize = (m_spec_max - m_spec_min + 1) - m_monitors.size();
          const int fullblocks = rangesize / blocksize;
          int read_stop = 0;
          int spectra_no = m_spec_min + m_monitors.size();
          // For this to work correctly, we assume that the spectrum list increases monotonically
          int filestart = std::lower_bound(spec_begin,m_spec_end,spectra_no) - spec_begin;
          if( fullblocks > 0 )
          {
            read_stop = (fullblocks * blocksize) + m_monitors.size();
            for( ; hist_index < read_stop; )
            {
              loadBlock(data, blocksize, period_index, filestart, hist_index, spectra_no, local_workspace);
              filestart += blocksize;
            }
          }
          int finalblock = rangesize - (fullblocks * blocksize);
          if( finalblock > 0 )
          {
            loadBlock(data, finalblock, period_index, filestart, hist_index, spectra_no,  local_workspace);
          }
        }

        //Load in the last of the list indices
        for( std::vector<int>::iterator itr = min_end; itr < m_spec_list.end(); ++itr )
        {
          // Load each
          int spectra_no = (*itr);
          // For this to work correctly, we assume that the spectrum list increases monotonically
          int filestart = std::lower_bound(spec_begin,m_spec_end,spectra_no) - spec_begin;
          loadBlock(data, 1, period_index, filestart, hist_index, spectra_no, local_workspace);
        }
      }

      if( !m_monitors.empty() )
      {
        hist_index = 0;
        for(std::map<int,std::string>::const_iterator it = m_monitors.begin(); 
          it != m_monitors.end(); ++it)
        {
          NXData monitor = entry.openNXData(it->second);
          NXInt mondata = monitor.openIntData();
          m_progress->report("Loading monitor");
          mondata.load(1,period-1);
          MantidVec& Y = local_workspace->dataY(hist_index);
          Y.assign(mondata(),mondata() + m_numberOfChannels);
          MantidVec& E = local_workspace->dataE(hist_index);
          std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
          local_workspace->getAxis(1)->spectraNo(hist_index) = it->first;

          NXFloat timeBins = monitor.openNXFloat("time_of_flight");
          timeBins.load();
          local_workspace->dataX(hist_index).assign(timeBins(),timeBins() + timeBins.dim0());
          hist_index++;
        }
      }


    }


    /**
    * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given blocksize
    * @param data The NXDataSet object
    * @param blocksize The blocksize to use
    * @param period The period number
    * @param start The index within the file to start reading from (zero based)
    * @param hist The workspace index to start reading into
    * @param spec_num The spectrum number that matches the hist variable
    * @param local_workspace The workspace to fill the data with
    */
    void LoadISISNexus2::loadBlock(NXDataSetTyped<int> & data, int blocksize, int period, int start,
      int &hist, int& spec_num, 
      DataObjects::Workspace2D_sptr local_workspace)
    {
      data.load(blocksize, period, start);
      int *data_start = data();
      int *data_end = data_start + m_numberOfChannels;
      int final(hist + blocksize);
      while( hist < final )
      {
        m_progress->report("Loading data");
        MantidVec& Y = local_workspace->dataY(hist);
        Y.assign(data_start, data_end);
        data_start += m_numberOfChannels; data_end += m_numberOfChannels;
        MantidVec& E = local_workspace->dataE(hist);
        std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
        // Populate the workspace. Loop starts from 1, hence i-1
        local_workspace->setX(hist, m_tof_data);
        local_workspace->getAxis(1)->spectraNo(hist)= spec_num;
        ++hist;
        ++spec_num;
      }
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadISISNexus2::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
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

    /**
    * Load data about the sample
    *   @param local_workspace The workspace to load the logs to.
    *   @param entry The Nexus entry
    */
    void LoadISISNexus2::loadSampleData(DataObjects::Workspace2D_sptr local_workspace, NXEntry & entry)
    {
      /// The proton charge
      // boost::shared_ptr<API::Sample> sample_details = local_workspace->getSample();
      local_workspace->mutableSample().setProtonCharge(entry.getFloat("proton_charge"));

      /// Sample geometry
      NXInt spb = entry.openNXInt("isis_vms_compat/SPB");
      // Just load the index we need, not the whole block. The flag is the third value in
      spb.load(1, 2);
      int geom_id = spb[0];
      local_workspace->mutableSample().setGeometryFlag(spb[0]);

      NXFloat rspb = entry.openNXFloat("isis_vms_compat/RSPB");
      // Just load the indices we need, not the whole block. The values start from the 4th onward
      rspb.load(3, 3);
      double thick(rspb[0]), height(rspb[1]), width(rspb[2]);
      local_workspace->mutableSample().setThickness(thick);
      local_workspace->mutableSample().setHeight(height);
      local_workspace->mutableSample().setWidth(width);

      g_log.debug() << "Sample geometry -  ID: " << geom_id << ", thickness: " << thick << ", height: " << height << ", width: " << width << "\n";
    }

    /**  Load logs from Nexus file. Logs are expected to be in
    *   /raw_data_1/runlog group of the file. Call to this method must be done
    *   within /raw_data_1 group.
    *   @param ws The workspace to load the logs to.
    *   @param entry The Nexus entry
    *   @param period The period of this workspace
    */
    void LoadISISNexus2::loadLogs(DataObjects::Workspace2D_sptr ws, NXEntry & entry,int period)
    {

      NXMainClass runlogs = entry.openNXClass<NXMainClass>("runlog");

      for(std::vector<NXClassInfo>::const_iterator it=runlogs.groups().begin();it!=runlogs.groups().end();it++)
      {
        if (it->nxclass == "NXlog")
        {
          
          NXLog nxLog(runlogs,it->nxname);
          nxLog.openLocal();

          Kernel::Property* logv = nxLog.createTimeSeries();
          if (!logv)
          {
            nxLog.close();
            continue;
          }
          ws->mutableSample().addLogData(logv);
          if (it->nxname == "icp_event")
          {
            LogParser parser(logv);
            ws->mutableSample().addLogData(parser.createPeriodLog(period));
            ws->mutableSample().addLogData(parser.createAllPeriodsLog());
            ws->mutableSample().addLogData(parser.createRunningLog());
          }
          nxLog.close();
        }
      }

      NXMainClass selogs = entry.openNXClass<NXMainClass>("selog");
      for(std::vector<NXClassInfo>::const_iterator it=selogs.groups().begin();it!=selogs.groups().end();it++)
      {
        if (it->nxclass == "IXseblock")
        {
          NXMainClass selog(selogs,it->nxname);
          selog.openLocal("IXseblock");
          NXLog nxLog(selog,"value_log");
          bool ok = nxLog.openLocal();

          if (ok)
          {
            Kernel::Property* logv = nxLog.createTimeSeries("","selog_"+it->nxname);
            if (!logv)
            {
              nxLog.close();
              selog.close();
              continue;
            }
            ws->mutableSample().addLogData(logv);
            nxLog.close();
          }
          selog.close();
        }
      }

      ws->populateInstrumentParameters();
    }

    double LoadISISNexus2::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
