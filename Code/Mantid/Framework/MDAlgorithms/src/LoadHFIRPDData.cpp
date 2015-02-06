#include "MantidMDAlgorithms/LoadHFIRPDData.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string/predicate.hpp>
#include <Poco/TemporaryFile.h>

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

DECLARE_ALGORITHM(LoadHFIRPDData)

//------------------------------------------------------------------------------------------------
/** Constructor
 */
LoadHFIRPDData::LoadHFIRPDData() : m_instrumentName(""), m_numSpec(0) {}

//------------------------------------------------------------------------------------------------
/** Destructor
 */
LoadHFIRPDData::~LoadHFIRPDData() {}

//----------------------------------------------------------------------------------------------
/** Init
 */
void LoadHFIRPDData::init() {
  declareProperty(new WorkspaceProperty<TableWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "Input table workspace for data.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("ParentWorkspace", "",
                                                         Direction::Input),
                  "Input matrix workspace serving as parent workspace "
                  "containing sample logs.");

  declareProperty("RunStart", "", "Run start time");

  /// TODO - Add HB2B as it is implemented in future
  std::vector<std::string> allowedinstruments;
  allowedinstruments.push_back("HB2A");
  auto instrumentvalidator =
      boost::make_shared<ListValidator<std::string> >(allowedinstruments);
  declareProperty("Instrument", "HB2A", instrumentvalidator,
                  "Instrument to be loaded. ");

  declareProperty("DetectorPrefix", "anode",
                  "Prefix of the name for detectors. ");

  declareProperty("RunNumberName", "Pt.",
                  "Log name for run number/measurement point.");

  declareProperty(
      "RotationAngleLogName", "2theta",
      "Log name for rotation angle as the 2theta value of detector 0.");

  declareProperty(
      "MonitorCountsLogName", "monitor",
      "Name of the sample log to record monitor counts of each run.");

  declareProperty("DurationLogName", "time",
                  "Name of the sample log to record the duration of each run.");

  declareProperty("InitRunNumber", 1, "Starting value for run number.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name to use for the output workspace.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputMonitorWorkspace", "", Direction::Output),
                  "Name to use for the output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Exec
 */
void LoadHFIRPDData::exec() {

  // Process inputs
  DataObjects::TableWorkspace_sptr dataTableWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr parentWS = getProperty("ParentWorkspace");
  m_instrumentName = getPropertyValue("Instrument");

  // Check whether parent workspace has run start
  DateAndTime runstart(0);
  if (parentWS->run().hasProperty("run_start")) {
    // Use parent workspace's first
    runstart = parentWS->run().getProperty("run_start")->value();
  } else {
    // Use user given
    std::string runstartstr = getProperty("RunStart");
    // raise exception if user does not give a proper run start
    if (runstartstr.size() == 0)
      throw std::runtime_error("Run-start time is not defined either in "
                               "input parent workspace or given by user.");
    runstart = DateAndTime(runstartstr);
  }

  // Convert table workspace to a list of 2D workspaces
  std::map<std::string, std::vector<double> > logvecmap;
  std::vector<Kernel::DateAndTime> vectimes;
  std::vector<MatrixWorkspace_sptr> vec_ws2d =
      convertToWorkspaces(dataTableWS, parentWS, runstart, logvecmap, vectimes);

  // Convert to MD workspaces
  g_log.debug("About to converting to workspaces done!");
  IMDEventWorkspace_sptr m_mdEventWS = convertToMDEventWS(vec_ws2d);
  std::string monitorlogname = getProperty("MonitorCountsLogName");
  IMDEventWorkspace_sptr mdMonitorWS =
      createMonitorMDWorkspace(vec_ws2d, logvecmap[monitorlogname]);

  // Add experiment info for each run and sample log to the first experiment
  // info object
  int initrunnumber = getProperty("InitRunNumber");
  addExperimentInfos(m_mdEventWS, vec_ws2d, initrunnumber);
  addExperimentInfos(mdMonitorWS, vec_ws2d, initrunnumber);
  appendSampleLogs(m_mdEventWS, logvecmap, vectimes);

  // Set property
  setProperty("OutputWorkspace", m_mdEventWS);
  setProperty("OutputMonitorWorkspace", mdMonitorWS);
}

//----------------------------------------------------------------------------------------------
/** Convert runs/pts from table workspace to a list of workspace 2D
 * @brief LoadHFIRPDData::convertToWorkspaces
 * @param tablews
 * @param parentws
 * @param runstart
 * @param logvecmap
 * @param vectimes
 * @return
 */
std::vector<MatrixWorkspace_sptr> LoadHFIRPDData::convertToWorkspaces(
    DataObjects::TableWorkspace_sptr tablews,
    API::MatrixWorkspace_const_sptr parentws, Kernel::DateAndTime runstart,
    std::map<std::string, std::vector<double> > &logvecmap,
    std::vector<Kernel::DateAndTime> &vectimes) {
  // Get table workspace's column information
  size_t irotangle, itime;
  std::vector<std::pair<size_t, size_t> > anodelist;
  std::map<std::string, size_t> sampleindexlist;
  readTableInfo(tablews, irotangle, itime, anodelist, sampleindexlist);
  m_numSpec = anodelist.size();

  // Load data
  size_t numws = tablews->rowCount();
  std::vector<MatrixWorkspace_sptr> vecws(numws);
  double duration = 0;
  vectimes.resize(numws);
  for (size_t irow = 0; irow < numws; ++irow) {
    vecws[irow] = loadRunToMatrixWS(tablews, irow, parentws, runstart,
                                    irotangle, itime, anodelist, duration);
    vectimes[irow] = runstart;
    runstart += static_cast<int64_t>(duration * 1.0E9);
  }

  // Process log data which will not be put to matrix workspace but will got to
  // MDWorkspace
  parseSampleLogs(tablews, sampleindexlist, logvecmap);

  g_log.debug() << "Number of matrix workspaces in vector = " << vecws.size()
                << "\n";
  return vecws;
}

//------------------------------------------------------------------------------------------------
/** Parse sample logs from table workspace and return with a set of vectors
 * @brief LoadHFIRPDData::parseSampleLogs
 * @param tablews
 * @param indexlist
 * @param logvecmap
 */
void LoadHFIRPDData::parseSampleLogs(
    DataObjects::TableWorkspace_sptr tablews,
    const std::map<std::string, size_t> &indexlist,
    std::map<std::string, std::vector<double> > &logvecmap) {
  size_t numrows = tablews->rowCount();

  std::map<std::string, size_t>::const_iterator indexiter;
  for (indexiter = indexlist.begin(); indexiter != indexlist.end();
       ++indexiter) {
    std::string logname = indexiter->first;
    size_t icol = indexiter->second;

    g_log.information() << " Parsing log " << logname << "\n";

    std::vector<double> logvec(numrows);
    for (size_t ir = 0; ir < numrows; ++ir) {
      double dbltemp = tablews->cell_cast<double>(ir, icol);
      logvec[ir] = dbltemp;
    }

    logvecmap.insert(std::make_pair(logname, logvec));
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Load one run of data to a new workspace
 * @brief LoadHFIRPDD::loadRunToMatrixWS
 * @param tablews :: input workspace
 * @param irow :: the row in workspace to load
 * @param parentws :: parent workspace with preset log
 * @param runstart :: run star time
 * @param irotangle :: column index of rotation angle
 * @param itime :: column index of duration
 * @param anodelist :: list of anodes
 * @param duration :: output of duration
 * @return
 */
MatrixWorkspace_sptr LoadHFIRPDData::loadRunToMatrixWS(
    DataObjects::TableWorkspace_sptr tablews, size_t irow,
    MatrixWorkspace_const_sptr parentws, Kernel::DateAndTime runstart,
    size_t irotangle, size_t itime,
    const std::vector<std::pair<size_t, size_t> > anodelist, double &duration) {
  // New workspace from parent workspace
  MatrixWorkspace_sptr tempws =
      WorkspaceFactory::Instance().create(parentws, m_numSpec, 2, 1);

  // Set up angle and time
  double twotheta = tablews->cell<double>(irow, irotangle);
  TimeSeriesProperty<double> *prop2theta =
      new TimeSeriesProperty<double>("rotangle");

  prop2theta->addValue(runstart, twotheta);
  tempws->mutableRun().addProperty(prop2theta);

  TimeSeriesProperty<std::string> *proprunstart =
      new TimeSeriesProperty<std::string>("run_start");
  proprunstart->addValue(runstart, runstart.toISO8601String());

  g_log.debug() << "Run " << irow << ": set run start to "
                << runstart.toISO8601String() << "\n";
  if (tempws->run().hasProperty("run_start")) {
    g_log.information() << "Temporary workspace inherites run_start as "
                        << tempws->run().getProperty("run_start")->value()
                        << ". It will be replaced by the correct value. "
                        << "\n";
    tempws->mutableRun().removeProperty("run_start");
  }
  tempws->mutableRun().addProperty(proprunstart);

  // Load instrument
  IAlgorithm_sptr instloader = this->createChildAlgorithm("LoadInstrument");
  instloader->initialize();
  instloader->setProperty("InstrumentName", m_instrumentName);
  instloader->setProperty("Workspace", tempws);
  instloader->execute();

  tempws = instloader->getProperty("Workspace");

  // Import data
  for (size_t i = 0; i < m_numSpec; ++i) {
    Geometry::IDetector_const_sptr tmpdet = tempws->getDetector(i);
    tempws->dataX(i)[0] = tmpdet->getPos().X();
    tempws->dataX(i)[0] = tmpdet->getPos().X() + 0.01;
    tempws->dataY(i)[0] = tablews->cell<double>(irow, anodelist[i].second);
    tempws->dataE(i)[0] = 1;
  }

  // Return duration
  duration = tablews->cell<double>(irow, itime);

  return tempws;
}

//----------------------------------------------------------------------------------------------
/** Read table workspace's column information
 * @brief LoadHFIRPDData::readTableInfo
 * @param tablews
 * @param ipt
 * @param irotangle
 * @param itime
 * @param anodelist
 * @param sampleindexlist
 */
void LoadHFIRPDData::readTableInfo(
    TableWorkspace_const_sptr tablews, size_t &irotangle, size_t &itime,
    std::vector<std::pair<size_t, size_t> > &anodelist,
    std::map<std::string, size_t> &samplenameindexmap) {

  // Get detectors' names and other sample names
  std::string anodelogprefix = getProperty("DetectorPrefix");
  const std::vector<std::string> &colnames = tablews->getColumnNames();
  for (size_t icol = 0; icol < colnames.size(); ++icol) {
    const std::string &colname = colnames[icol];

    if (boost::starts_with(colname, anodelogprefix)) {
      // anode
      std::vector<std::string> terms;
      boost::split(terms, colname, boost::is_any_of(anodelogprefix));
      size_t anodeid = static_cast<size_t>(atoi(terms.back().c_str()));
      anodelist.push_back(std::make_pair(anodeid, icol));
    } else {
      samplenameindexmap.insert(std::make_pair(colname, icol));
    }
  } // ENDFOR (icol)

  // Check detectors' names
  if (anodelist.size() == 0) {
    std::stringstream errss;
    errss << "There is no log name starting with " << anodelogprefix
          << " for detector. ";
    throw std::runtime_error(errss.str());
  }

  // Find out other essential sample log names
  std::map<std::string, size_t>::iterator mapiter;

  std::string ptname = getProperty("RunNumberName");                 // "Pt."
  std::string monitorlogname = getProperty("MonitorCountsLogName");  //"monitor"
  std::string durationlogname = getProperty("DurationLogName");      //"time"
  std::string rotanglelogname = getProperty("RotationAngleLogName"); // "2theta"

  std::vector<std::string> lognames;
  lognames.push_back(ptname);
  lognames.push_back(monitorlogname);
  lognames.push_back(durationlogname);
  lognames.push_back(rotanglelogname);

  std::vector<size_t> ilognames(lognames.size());

  for (size_t i = 0; i < lognames.size(); ++i) {
    const std::string &logname = lognames[i];
    mapiter = samplenameindexmap.find(logname);
    if (mapiter != samplenameindexmap.end()) {
      ilognames[i] = mapiter->second;
    } else {
      std::stringstream ess;
      ess << "Essential log name " << logname
          << " cannot be found in data table workspace.";
      throw std::runtime_error(ess.str());
    }
  }

  // Retrieve the vector index
  itime = ilognames[2];
  irotangle = ilognames[3];

  // Sort out anode id index list;
  std::sort(anodelist.begin(), anodelist.end());

  return;
}

//----------------------------------------------------------------------------------------------

/** Convert to MD Event workspace
 * @brief LoadHFIRPDData::convertToMDEventWS
 * @param vec_ws2d
 * @return
 */
IMDEventWorkspace_sptr LoadHFIRPDData::convertToMDEventWS(
    const std::vector<MatrixWorkspace_sptr> &vec_ws2d) {
  // Write the lsit of workspacs to a file to be loaded to an MD workspace
  Poco::TemporaryFile tmpFile;
  std::string tempFileName = tmpFile.path();
  g_log.debug() << "Creating temporary MD Event file = " << tempFileName
                << "\n";

  // Construct a file
  std::ofstream myfile;
  myfile.open(tempFileName.c_str());
  myfile << "DIMENSIONS" << std::endl;
  myfile << "x X m 100" << std::endl;
  myfile << "y Y m 100" << std::endl;
  myfile << "z Z m 100" << std::endl;
  myfile << "t T s 100" << std::endl;
  myfile << "# Signal, Error, DetectorId, RunId, coord1, coord2, ... to end of "
            "coords" << std::endl;
  myfile << "MDEVENTS" << std::endl;

  if (vec_ws2d.size() > 0) {
    double relruntime = 0;

    Progress progress(this, 0, 1, vec_ws2d.size());
    size_t detindex = 0;
    for (auto it = vec_ws2d.begin(); it < vec_ws2d.end(); ++it) {
      std::size_t pos = std::distance(vec_ws2d.begin(), it);
      API::MatrixWorkspace_sptr thisWorkspace = *it;

      std::size_t nHist = thisWorkspace->getNumberHistograms();
      for (std::size_t i = 0; i < nHist; ++i) {
        Geometry::IDetector_const_sptr det = thisWorkspace->getDetector(i);
        const MantidVec &signal = thisWorkspace->readY(i);
        const MantidVec &error = thisWorkspace->readE(i);
        myfile << signal[0] << " ";
        myfile << error[0] << " ";
        myfile << det->getID() + detindex << " ";
        myfile << pos << " ";
        Kernel::V3D detPos = det->getPos();
        myfile << detPos.X() << " ";
        myfile << detPos.Y() << " ";
        myfile << detPos.Z() << " ";
        // Add a new dimension as event time
        myfile << relruntime << " ";
        myfile << std::endl;
      }

      // Increment on detector IDs
      if (nHist < 100)
        detindex += 100;
      else
        detindex += nHist;

      // Run time increment by time
      /// Must make 'time' be specified by user.  A validity check is required
      /// too
      relruntime +=
          atof(thisWorkspace->run().getProperty("time")->value().c_str());

      progress.report("Creating MD WS");
    }
    myfile.close();
  }
  else
  {
    throw std::runtime_error("There is no MatrixWorkspace to construct MDWorkspace.");
  }

  // Import to MD Workspace
  IAlgorithm_sptr importMDEWS = createChildAlgorithm("ImportMDEventWorkspace");
  // Now execute the Child Algorithm.
  try {
    importMDEWS->setPropertyValue("Filename", tempFileName);
    importMDEWS->setProperty("OutputWorkspace", "Test");
    importMDEWS->executeAsChildAlg();
  }
  catch (std::exception &exc) {
    throw std::runtime_error(
        std::string("Error running ImportMDEventWorkspace: ") + exc.what());
  }
  IMDEventWorkspace_sptr workspace =
      importMDEWS->getProperty("OutputWorkspace");
  if (!workspace)
    throw(std::runtime_error("Can not retrieve results of child algorithm "
                             "ImportMDEventWorkspace"));

  return workspace;
}

//-----------------------------------------------------------------------------------------------
/** Create an MDWorkspace for monitoring counts.
 * @brief LoadHFIRPDD::createMonitorMDWorkspace
 * @param vec_ws2d
 * @param vecmonitor
 * @return
 */
IMDEventWorkspace_sptr LoadHFIRPDData::createMonitorMDWorkspace(
    const std::vector<MatrixWorkspace_sptr> vec_ws2d,
    const std::vector<double> &vecmonitor) {
  // Write the lsit of workspacs to a file to be loaded to an MD workspace
  Poco::TemporaryFile tmpFile;
  std::string tempFileName = tmpFile.path();
  g_log.debug() << "Creating temporary MD Event file for monitor counts = "
                << tempFileName << "\n";

  // Construct a file
  std::ofstream myfile;
  myfile.open(tempFileName.c_str());
  myfile << "DIMENSIONS" << std::endl;
  myfile << "x X m 100" << std::endl;
  myfile << "y Y m 100" << std::endl;
  myfile << "z Z m 100" << std::endl;
  myfile << "t T s 100" << std::endl;
  myfile << "# Signal, Error, DetectorId, RunId, coord1, coord2, ... to end of "
            "coords" << std::endl;
  myfile << "MDEVENTS" << std::endl;

  if (vec_ws2d.size() > 0) {
    double relruntime = 0;

    Progress progress(this, 0, 1, vec_ws2d.size());
    size_t detindex = 0;
    for (auto it = vec_ws2d.begin(); it < vec_ws2d.end(); ++it) {
      std::size_t pos = std::distance(vec_ws2d.begin(), it);
      API::MatrixWorkspace_sptr thisWorkspace = *it;

      double signal = vecmonitor[static_cast<size_t>(it - vec_ws2d.begin())];

      std::size_t nHist = thisWorkspace->getNumberHistograms();
      for (std::size_t i = 0; i < nHist; ++i) {
        Geometry::IDetector_const_sptr det = thisWorkspace->getDetector(i);

        // const MantidVec &signal = thisWorkspace->readY(i);
        const MantidVec &error = thisWorkspace->readE(i);
        myfile << signal << " ";
        myfile << error[0] << " ";
        myfile << det->getID() + detindex << " ";
        myfile << pos << " ";
        Kernel::V3D detPos = det->getPos();
        myfile << detPos.X() << " ";
        myfile << detPos.Y() << " ";
        myfile << detPos.Z() << " ";
        // Add a new dimension as event time.  Value is not important for
        // monitor workspace
        myfile << relruntime << " ";
        myfile << std::endl;
      }

      // Increment on detector IDs
      if (nHist < 100)
        detindex += 100;
      else
        detindex += nHist;

      // Run time increment by time
      /// Must make 'time' be specified by user.  A validity check is required
      /// too
      relruntime +=
          atof(thisWorkspace->run().getProperty("time")->value().c_str());

      progress.report("Creating MD WS");
    }
    myfile.close();
  }
  else
  {
    throw std::runtime_error("There is no MatrixWorkspace to construct MDWorkspace.");
  }

  // Import to MD Workspace
  IAlgorithm_sptr importMDEWS = createChildAlgorithm("ImportMDEventWorkspace");
  // Now execute the Child Algorithm.
  try {
    importMDEWS->setPropertyValue("Filename", tempFileName);
    importMDEWS->setProperty("OutputWorkspace", "Test");
    importMDEWS->executeAsChildAlg();
  }
  catch (std::exception &exc) {
    throw std::runtime_error(
        std::string("Error running ImportMDEventWorkspace: ") + exc.what());
  }
  IMDEventWorkspace_sptr workspace =
      importMDEWS->getProperty("OutputWorkspace");
  if (!workspace)
    throw(std::runtime_error("Can not retrieve results of child algorithm "
                             "ImportMDEventWorkspace"));

  return workspace;
}

//-----------------------------------------------------------------------------------------------
/** Create sample logs for MD workspace
 * @brief LoadHFIRPDD::appendSampleLogs
 * @param mdws
 * @param logvecmap
 * @param vectimes
 */
void LoadHFIRPDData::appendSampleLogs(
    IMDEventWorkspace_sptr mdws,
    const std::map<std::string, std::vector<double> > &logvecmap,
    const std::vector<Kernel::DateAndTime> &vectimes) {
  // Check!
  size_t numexpinfo = mdws->getNumExperimentInfo();
  if (numexpinfo == 0)
    throw std::runtime_error(
        "There is no ExperimentInfo defined for MDWorkspace. "
        "It is impossible to add any log!");
  else if (numexpinfo != vectimes.size() + 1)
    throw std::runtime_error(
        "The number of ExperimentInfo should be 1 more than "
        "the length of vector of time, i.e., number of matrix workspaces.");

  std::map<std::string, std::vector<double> >::const_iterator miter;

  // get runnumber vector
  std::string runnumlogname = getProperty("RunNumberName");
  miter = logvecmap.find(runnumlogname);
  if (miter == logvecmap.end())
    throw std::runtime_error("Impossible not to find Pt. in log vec map.");
  const std::vector<double> &vecrunno = miter->second;

  // Add run_start to each ExperimentInfo
  for (size_t i = 0; i < vectimes.size(); ++i) {
    Kernel::DateAndTime runstart = vectimes[i];
    mdws->getExperimentInfo(i)->mutableRun().addLogData(
        new PropertyWithValue<std::string>("run_start",
                                           runstart.toFormattedString()));
  }
  mdws->getExperimentInfo(vectimes.size())->mutableRun().addLogData(
      new PropertyWithValue<std::string>("run_start",
                                         vectimes[0].toFormattedString()));

  // Add sample logs
  // get hold of last experiment info
  ExperimentInfo_sptr eilast = mdws->getExperimentInfo(numexpinfo - 1);

  for (miter = logvecmap.begin(); miter != logvecmap.end(); ++miter) {
    std::string logname = miter->first;
    const std::vector<double> &veclogval = miter->second;

    // Check log values and times
    if (veclogval.size() != vectimes.size()) {
      g_log.error() << "Log " << logname
                    << " has different number of log values ("
                    << veclogval.size() << ") than number of log entry time ("
                    << vectimes.size() << ")"
                    << "\n";
      continue;
    }

    // For N single value experiment info
    for (size_t i = 0; i < veclogval.size(); ++i) {
      // get ExperimentInfo
      ExperimentInfo_sptr tmpei = mdws->getExperimentInfo(i);
      // check run number matches
      int runnumber =
          atoi(tmpei->run().getProperty("run_number")->value().c_str());
      if (runnumber != static_cast<int>(vecrunno[i]))
        throw std::runtime_error("Run number does not match to Pt. value.");
      // add property
      tmpei->mutableRun().addLogData(
          new PropertyWithValue<double>(logname, veclogval[i]));
    }

    // Create a new log
    TimeSeriesProperty<double> *templog =
        new TimeSeriesProperty<double>(logname);
    templog->addValues(vectimes, veclogval);

    // Add log to experiment info
    eilast->mutableRun().addLogData(templog);

    // Add log value to each ExperimentInfo for the first N
  }

  return;
}

//---------------------------------------------------------------------------------
/** Add Experiment Info to the MDWorkspace.  Add 1+N ExperimentInfo
 * @brief LoadHFIRPDData::addExperimentInfos
 * @param mdws
 * @param vec_ws2d
 * @param init_runnumber
 */
void LoadHFIRPDData::addExperimentInfos(
    API::IMDEventWorkspace_sptr mdws,
    const std::vector<API::MatrixWorkspace_sptr> vec_ws2d,
    const int &init_runnumber) {
  // Add N experiment info as there are N measurment points
  for (size_t i = 0; i < vec_ws2d.size(); ++i) {
    // Create an ExperimentInfo object
    ExperimentInfo_sptr tmp_expinfo = boost::make_shared<ExperimentInfo>();
    Geometry::Instrument_const_sptr tmp_inst = vec_ws2d[i]->getInstrument();
    tmp_expinfo->setInstrument(tmp_inst);

    tmp_expinfo->mutableRun().addProperty(new PropertyWithValue<int>(
        "run_number", static_cast<int>(i) + init_runnumber));

    // Add ExperimentInfo to workspace
    mdws->addExperimentInfo(tmp_expinfo);
  }

  // Add one additional in order to contain the combined sample logs
  ExperimentInfo_sptr combine_expinfo = boost::make_shared<ExperimentInfo>();
  combine_expinfo->mutableRun().addProperty(
      new PropertyWithValue<int>("run_number", init_runnumber - 1));
  mdws->addExperimentInfo(combine_expinfo);

  return;
}
} // namespace DataHandling
} // namespace Mantid
