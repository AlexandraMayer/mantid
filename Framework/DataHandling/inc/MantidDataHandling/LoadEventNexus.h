#ifndef MANTID_DATAHANDLING_LOADEVENTNEXUS_H_
#define MANTID_DATAHANDLING_LOADEVENTNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/BankPulseTimes.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"

#ifdef _WIN32 // fixing windows issue causing conflict between
// winnt char and nexus char
#undef CHAR
#endif

#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include <memory>
#include <mutex>
#include <boost/lexical_cast.hpp>

namespace Mantid {

namespace DataHandling {

/** @class LoadEventNexus LoadEventNexus.h Nexus/LoadEventNexus.h

  Load Event Nexus files.

  Required Properties:
  <UL>
  <LI> Filename - The name of and path to the input NeXus file </LI>
  <LI> Workspace - The name of the workspace to output</LI>
  </UL>

  @date Sep 27, 2010

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  */
class DLLExport LoadEventNexus
    : public API::IFileLoader<Kernel::NexusDescriptor> {

public:
  LoadEventNexus();
  ~LoadEventNexus() override;

  const std::string name() const override { return "LoadEventNexus"; };

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an Event NeXus file and stores as an "
           "EventWorkspace. Optionally, you can filter out events falling "
           "outside a range of times-of-flight and/or a time interval.";
  }

  /// Version
  int version() const override { return 1; };

  /// Category
  const std::string category() const override { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  /** Sets whether the pixel counts will be pre-counted.
   * @param value :: true if you want to precount. */
  void setPrecount(bool value) { precount = value; }

  template <typename T>
  static boost::shared_ptr<BankPulseTimes> runLoadNexusLogs(
      const std::string &nexusfilename, T localWorkspace, Algorithm &alg,
      bool returnpulsetimes, int &nPeriods,
      std::unique_ptr<const Kernel::TimeSeriesProperty<int>> &periodLog);

  template <typename T>
  static void loadEntryMetadata(const std::string &nexusfilename, T WS,
                                const std::string &entry_name);

  /// Load instrument from Nexus file if possible, else from IDF spacified by
  /// Nexus file
  template <typename T>
  static bool loadInstrument(const std::string &nexusfilename, T localWorkspace,
                             const std::string &top_entry_name, Algorithm *alg);

  /// Load instrument for Nexus file
  template <typename T>
  static bool
  runLoadIDFFromNexus(const std::string &nexusfilename, T localWorkspace,
                      const std::string &top_entry_name, Algorithm *alg);

  /// Load instrument from IDF file specified by Nexus file
  template <typename T>
  static bool
  runLoadInstrument(const std::string &nexusfilename, T localWorkspace,
                    const std::string &top_entry_name, Algorithm *alg);

  static void loadSampleDataISIScompatibility(::NeXus::File &file,
                                              EventWorkspaceCollection &WS);

  /// method used to return instrument name for some old ISIS files where it is
  /// not written properly within the instrument
  static std::string readInstrumentFromISIS_VMSCompat(::NeXus::File &hFile);

public:
  /// The name and path of the input file
  std::string m_filename;

  /// The workspace being filled out
  boost::shared_ptr<EventWorkspaceCollection> m_ws;

  /// Filter by a minimum time-of-flight
  double filter_tof_min;
  /// Filter by a maximum time-of-flight
  double filter_tof_max;

  /// Spectra list to load
  std::vector<int32_t> m_specList;
  /// Minimum spectrum to load
  int32_t m_specMin;
  /// Maximum spectrum to load
  int32_t m_specMax;

  /// Filter by start time
  Kernel::DateAndTime filter_time_start;
  /// Filter by stop time
  Kernel::DateAndTime filter_time_stop;
  /// chunk number
  int chunk;
  /// number of chunks
  int totalChunks;
  /// for multiple chunks per bank
  int firstChunkForBank;
  /// number of chunks per bank
  size_t eventsPerChunk;

  /// Mutex protecting tof limits
  std::mutex m_tofMutex;

  /// Limits found to tof
  double longest_tof;
  /// Limits found to tof
  double shortest_tof;
  /// Count of all the "bad" tofs found. These are events with TOF > 2e8
  /// microsec
  size_t bad_tofs;
  /// A count of events discarded because they came from a pixel that's not in
  /// the IDF
  size_t discarded_events;

  /// Do we pre-count the # of events in each pixel ID?
  bool precount;

  /// Tolerance for CompressEvents; use -1 to mean don't compress.
  double compressTolerance;

  /// Pointer to the vector of events
  typedef std::vector<Mantid::DataObjects::TofEvent> *EventVector_pt;

  /// Vector where index = event_id; value = ptr to std::vector<TofEvent> in the
  /// event list.
  std::vector<std::vector<EventVector_pt>> eventVectors;

  /// Mutex to protect eventVectors from each task
  std::recursive_mutex m_eventVectorMutex;

  /// Maximum (inclusive) event ID possible for this instrument
  int32_t eventid_max;

  /// Vector where (index = pixel ID+pixelID_to_wi_offset), value = workspace
  /// index)
  std::vector<size_t> pixelID_to_wi_vector;

  /// Offset in the pixelID_to_wi_vector to use.
  detid_t pixelID_to_wi_offset;

  /// One entry of pulse times for each preprocessor
  std::vector<boost::shared_ptr<BankPulseTimes>> m_bankPulseTimes;

  /// Pulse times for ALL banks, taken from proton_charge log.
  boost::shared_ptr<BankPulseTimes> m_allBanksPulseTimes;

  /// name of top level NXentry to use
  std::string m_top_entry_name;
  ::NeXus::File *m_file;

  /// whether or not to launch multiple ProcessBankData jobs per bank
  bool splitProcessing;

  /// Flag for dealing with a simulated file
  bool m_haveWeights;

  /// Pointer to the vector of weighted events
  typedef std::vector<Mantid::DataObjects::WeightedEvent> *
      WeightedEventVector_pt;

  /// Vector where index = event_id; value = ptr to std::vector<WeightedEvent>
  /// in the event list.
  std::vector<std::vector<WeightedEventVector_pt>> weightedEventVectors;

private:
  /// Intialisation code
  void init() override;

  /// Execution code
  void exec() override;

  DataObjects::EventWorkspace_sptr createEmptyEventWorkspace();

  /// Map detector IDs to event lists.
  template <class T>
  void makeMapToEventLists(std::vector<std::vector<T>> &vectors);

  void createWorkspaceIndexMaps(const bool monitors,
                                const std::vector<std::string> &bankNames);
  void loadEvents(API::Progress *const prog, const bool monitors);
  void createSpectraMapping(
      const std::string &nxsfile, const bool monitorsOnly,
      const std::vector<std::string> &bankNames = std::vector<std::string>());
  void deleteBanks(EventWorkspaceCollection_sptr workspace,
                   std::vector<std::string> bankNames);
  bool hasEventMonitors();
  void runLoadMonitorsAsEvents(API::Progress *const prog);
  void runLoadMonitors();
  /// Set the filters on TOF.
  void setTimeFilters(const bool monitors);

  /// Load a spectra mapping from the given file
  bool loadSpectraMapping(const std::string &filename, const bool monitorsOnly,
                          const std::string &entry_name);

  /// ISIS specific methods for dealing with wide events
  void loadTimeOfFlight(EventWorkspaceCollection_sptr WS,
                        const std::string &entry_name,
                        const std::string &classType);

  void loadTimeOfFlightData(::NeXus::File &file,
                            EventWorkspaceCollection_sptr WS,
                            const std::string &binsName, size_t start_wi = 0,
                            size_t end_wi = 0);
  template <typename T> void filterDuringPause(T workspace);

  // Validate the optional spectra input properties and initialize m_specList
  void createSpectraList(int32_t min, int32_t max);

  /// Set the top entry field name
  void setTopEntryName();

  /// to open the nexus file with specific exception handling/message
  void safeOpenFile(const std::string fname);

  /// Was the instrument loaded?
  bool m_instrument_loaded_correctly;

  /// Do we load the sample logs?
  bool loadlogs;
  /// have the logs been loaded?
  bool m_logs_loaded_correctly;

  /// True if the event_id is spectrum no not pixel ID
  bool event_id_is_spec;
};

//-----------------------------------------------------------------------------
/** Load the instrument definition file specified by info in the NXS file.
*
*  @param nexusfilename :: Used to pick the instrument.
*  @param localWorkspace :: Templated workspace in which to put the instrument
*geometry
*  @param top_entry_name :: entry name at the top of the NXS file
*  @param alg :: Handle of the algorithm
*  @return true if successful
*/
template <typename T>
bool LoadEventNexus::runLoadInstrument(const std::string &nexusfilename,
                                       T localWorkspace,
                                       const std::string &top_entry_name,
                                       Algorithm *alg) {
  std::string instrument;

  // Get the instrument name
  ::NeXus::File nxfile(nexusfilename);
  // Start with the base entry
  nxfile.openGroup(top_entry_name, "NXentry");
  // Open the instrument
  nxfile.openGroup("instrument", "NXinstrument");
  try {
    nxfile.openData("name");
    instrument = nxfile.getStrData();
    alg->getLogger().debug() << "Instrument name read from NeXus file is "
                             << instrument << '\n';
  } catch (::NeXus::Exception &) {
    // Try to fall back to isis compatibility options
    nxfile.closeGroup();
    instrument = readInstrumentFromISIS_VMSCompat(nxfile);
    if (instrument.empty()) {
      // Get the instrument name from the file instead
      size_t n = nexusfilename.rfind('/');
      if (n != std::string::npos) {
        std::string temp =
            nexusfilename.substr(n + 1, nexusfilename.size() - n - 1);
        n = temp.find('_');
        if (n != std::string::npos && n > 0) {
          instrument = temp.substr(0, n);
        }
      }
    }
  }
  if (instrument.compare("POWGEN3") ==
      0) // hack for powgen b/c of bad long name
    instrument = "POWGEN";
  if (instrument.compare("NOM") == 0) // hack for nomad
    instrument = "NOMAD";

  if (instrument.empty())
    throw std::runtime_error("Could not find the instrument name in the NXS "
                             "file or using the filename. Cannot load "
                             "instrument!");

  // Now let's close the file as we don't need it anymore to load the
  // instrument.
  nxfile.close();

  // do the actual work
  Mantid::API::IAlgorithm_sptr loadInst =
      alg->createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try {
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<Mantid::API::MatrixWorkspace_sptr>("Workspace",
                                                             localWorkspace);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(false));
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around
    // a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument &e) {
    alg->getLogger().information()
        << "Invalid argument to LoadInstrument Child Algorithm : " << e.what()
        << '\n';
    executionSuccessful = false;
  } catch (std::runtime_error &e) {
    alg->getLogger().information(
        "Unable to successfully run LoadInstrument Child Algorithm");
    alg->getLogger().information(e.what());
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful) {
    alg->getLogger().error() << "Error loading Instrument definition file\n";
    return false;
  }

  // Ticket #2049: Cleanup all loadinstrument members to a single instance
  // If requested update the instrument to positions in the data file
  const auto &pmap = localWorkspace->constInstrumentParameters();
  if (!pmap.contains(localWorkspace->getInstrument()->getComponentID(),
                     "det-pos-source"))
    return executionSuccessful;

  boost::shared_ptr<Geometry::Parameter> updateDets = pmap.get(
      localWorkspace->getInstrument()->getComponentID(), "det-pos-source");
  std::string value = updateDets->value<std::string>();
  if (value.substr(0, 8) == "datafile") {
    Mantid::API::IAlgorithm_sptr updateInst =
        alg->createChildAlgorithm("UpdateInstrumentFromFile");
    updateInst->setProperty<Mantid::API::MatrixWorkspace_sptr>("Workspace",
                                                               localWorkspace);
    updateInst->setPropertyValue("Filename", nexusfilename);
    if (value == "datafile-ignore-phi") {
      updateInst->setProperty("IgnorePhi", true);
      alg->getLogger().information("Detector positions in IDF updated with "
                                   "positions in the data file except for the "
                                   "phi values");
    } else {
      alg->getLogger().information(
          "Detector positions in IDF updated with positions in the data file");
    }
    // We want this to throw if it fails to warn the user that the information
    // is not correct.
    updateInst->execute();
  }

  return executionSuccessful;
}

//-----------------------------------------------------------------------------
/** Load the run number and other meta data from the given bank */
template <typename T>
void LoadEventNexus::loadEntryMetadata(const std::string &nexusfilename, T WS,
                                       const std::string &entry_name) {
  // Open the file
  ::NeXus::File file(nexusfilename);
  file.openGroup(entry_name, "NXentry");

  // get the title
  try {
    file.openData("title");
    if (file.getInfo().type == ::NeXus::CHAR) {
      std::string title = file.getStrData();
      if (!title.empty())
        WS->setTitle(title);
    }
    file.closeData();
  } catch (std::exception &) {
    // don't set the title if the field is not loaded
  }

  // get the notes
  try {
    file.openData("notes");
    if (file.getInfo().type == ::NeXus::CHAR) {
      std::string notes = file.getStrData();
      if (!notes.empty())
        WS->mutableRun().addProperty("file_notes", notes);
    }
    file.closeData();
  } catch (::NeXus::Exception &) {
    // let it drop on floor
  }

  // Get the run number
  file.openData("run_number");
  std::string run;
  if (file.getInfo().type == ::NeXus::CHAR) {
    run = file.getStrData();
  } else if (file.isDataInt()) {
    // inside ISIS the run_number type is int32
    std::vector<int> value;
    file.getData(value);
    if (!value.empty())
      run = std::to_string(value[0]);
  }
  if (!run.empty()) {
    WS->mutableRun().addProperty("run_number", run);
  }
  file.closeData();

  // get the experiment identifier
  try {
    file.openData("experiment_identifier");
    std::string expId;
    if (file.getInfo().type == ::NeXus::CHAR) {
      expId = file.getStrData();
    }
    if (!expId.empty()) {
      WS->mutableRun().addProperty("experiment_identifier", expId);
    }
    file.closeData();
  } catch (::NeXus::Exception &) {
    // let it drop on floor
  }

  // get the sample name
  try {
    file.openGroup("sample", "NXsample");
    file.openData("name");
    std::string name;
    if (file.getInfo().type == ::NeXus::CHAR) {
      name = file.getStrData();
    }
    if (!name.empty()) {
      WS->mutableSample().setName(name);
    }
    file.closeData();
    file.closeGroup();
  } catch (::NeXus::Exception &) {
    // let it drop on floor
  }

  // get the duration
  file.openData("duration");
  std::vector<double> duration;
  file.getDataCoerce(duration);
  if (duration.size() == 1) {
    // get the units
    // clang-format off
    std::vector< ::NeXus::AttrInfo> infos = file.getAttrInfos();
    std::string units;
    for (std::vector< ::NeXus::AttrInfo>::const_iterator it = infos.begin();
         it != infos.end(); ++it) {
      if (it->name.compare("units") == 0) {
        units = file.getStrAttr(*it);
        break;
      }
    }
    // clang-format on

    // set the property
    WS->mutableRun().addProperty("duration", duration[0], units);
  }
  file.closeData();

  // close the file
  file.close();
}

//-----------------------------------------------------------------------------
/** Load the instrument from the nexus file or if not found from the IDF file
*  specified by the info in the Nexus file
*
*  @param nexusfilename :: The Nexus file name
*  @param localWorkspace :: templated workspace in which to put the instrument
*geometry
*  @param top_entry_name :: entry name at the top of the Nexus file
*  @param alg :: Handle of the algorithm
*  @return true if successful
*/
template <typename T>
bool LoadEventNexus::loadInstrument(const std::string &nexusfilename,
                                    T localWorkspace,
                                    const std::string &top_entry_name,
                                    Algorithm *alg) {
  bool foundInstrument = runLoadIDFFromNexus<T>(nexusfilename, localWorkspace,
                                                top_entry_name, alg);
  if (!foundInstrument)
    foundInstrument = runLoadInstrument<T>(nexusfilename, localWorkspace,
                                           top_entry_name, alg);
  return foundInstrument;
}

//-----------------------------------------------------------------------------
/** Load the instrument from the nexus file
*
*  @param nexusfilename :: The name of the nexus file being loaded
*  @param localWorkspace :: templated workspace in which to put the instrument
*geometry
*  @param top_entry_name :: entry name at the top of the Nexus file
*  @param alg :: Handle of the algorithm
*  @return true if successful
*/
template <typename T>
bool LoadEventNexus::runLoadIDFFromNexus(const std::string &nexusfilename,
                                         T localWorkspace,
                                         const std::string &top_entry_name,
                                         Algorithm *alg) {
  // Test if IDF exists in file, move on quickly if not
  try {
    ::NeXus::File nxsfile(nexusfilename);
    nxsfile.openPath(top_entry_name + "/instrument/instrument_xml");
  } catch (::NeXus::Exception &) {
    alg->getLogger().information("No instrument definition found in " +
                                 nexusfilename + " at " + top_entry_name +
                                 "/instrument");
    return false;
  }

  Mantid::API::IAlgorithm_sptr loadInst =
      alg->createChildAlgorithm("LoadIDFFromNexus");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("Filename", nexusfilename);
    loadInst->setProperty<Mantid::API::MatrixWorkspace_sptr>("Workspace",
                                                             localWorkspace);
    loadInst->setPropertyValue("InstrumentParentPath", top_entry_name);
    loadInst->execute();
  } catch (std::invalid_argument &) {
    alg->getLogger().error(
        "Invalid argument to LoadIDFFromNexus Child Algorithm ");
  } catch (std::runtime_error &) {
    alg->getLogger().debug("No instrument definition found in " +
                           nexusfilename + " at " + top_entry_name +
                           "/instrument");
  }

  if (!loadInst->isExecuted())
    alg->getLogger().information("No IDF loaded from Nexus file.");
  return loadInst->isExecuted();
}
} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADEVENTNEXUS_H_*/
