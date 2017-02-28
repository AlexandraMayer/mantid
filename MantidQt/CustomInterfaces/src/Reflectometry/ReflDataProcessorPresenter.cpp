#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"

#include <boost/algorithm/string.hpp>
#include <numeric>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

/**
* Constructor
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A DataProcessorProcessingAlgorithm
* @param postprocessor : A DataProcessorPostprocessingAlgorithm
* workspaces
* @param postprocessMap : A map containing instructions for post-processing.
* This map links column name to properties of the post-processing algorithm
* @param loader : The algorithm responsible for loading data
*/
ReflDataProcessorPresenter::ReflDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::map<std::string, std::string> &postprocessMap,
    const std::string &loader)
    : GenericDataProcessorPresenter(whitelist, preprocessMap, processor,
                                    postprocessor, postprocessMap, loader) {}

/**
* Destructor
*/
ReflDataProcessorPresenter::~ReflDataProcessorPresenter() {}

/**
 Process selected data
*/
void ReflDataProcessorPresenter::process() {

  // If uniform slicing is empty process normally, delegating to
  // GenericDataProcessorPresenter
  std::string timeSlicingValues = m_mainPresenter->getTimeSlicingValues();
  if (timeSlicingValues.empty()) {
    GenericDataProcessorPresenter::process();
    return;
  }

  // Get time slicing type
  std::string timeSlicingType = m_mainPresenter->getTimeSlicingType();

  // Get selected runs
  const auto items = m_manager->selectedData(true);

  // Progress report
  int progress = 0;
  int maxProgress = (int)(items.size());
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  // True if all groups were processed as event workspaces
  bool allGroupsWereEvent = true;
  // True if errors where encountered when reducing table
  bool errors = false;

  // Loop in groups
  for (const auto &item : items) {

    // Group of runs
    GroupData group = item.second;

    try {
      // First load the runs.
      bool allEventWS = loadGroup(group);

      if (allEventWS) {
        // Process the group
        if (processGroupAsEventWS(item.first, group, timeSlicingType,
                                  timeSlicingValues))
          errors = true;

        // Notebook not implemented yet
        if (m_view->getEnableNotebook()) {
          GenericDataProcessorPresenter::giveUserWarning(
              "Notebook not implemented for sliced data yet",
              "Notebook will not be generated");
        }

      } else {
        // Process the group
        if (processGroupAsNonEventWS(item.first, group))
          errors = true;
        // Notebook
      }

      if (!allEventWS)
        allGroupsWereEvent = false;

    } catch (...) {
      errors = true;
    }
    progressReporter.report();
  }

  if (!allGroupsWereEvent)
    m_mainPresenter->giveUserWarning(
        "Some groups could not be processed as event workspaces", "Warning");
  if (errors)
    m_mainPresenter->giveUserWarning("Some errors were encountered when "
                                     "reducing table. Some groups may not have "
                                     "been fully processed.",
                                     "Warning");

  progressReporter.clear();
}

/** Loads a group of runs. Tries loading runs as event workspaces. If any of the
* workspaces in the group is not an event workspace, stops loading and re-loads
* all of them as non-event workspaces. We need the workspaces to be of the same
* type to process them together.
*
* @param group :: the group of runs
* @return :: true if all runs were loaded as event workspaces. False otherwise
*/
bool ReflDataProcessorPresenter::loadGroup(const GroupData &group) {

  // Set of runs loaded successfully
  std::set<std::string> loadedRuns;

  for (const auto &row : group) {

    // The run number
    std::string runNo = row.second.at(0);
    // Try loading as event workspace
    bool eventWS = loadEventRun(runNo);
    if (!eventWS) {
      // This run could not be loaded as event workspace. We need to load and
      // process the whole group as non-event data.
      for (const auto &rowNew : group) {
        // The run number
        std::string runNo = rowNew.second.at(0);
        // Load as non-event workspace
        loadNonEventRun(runNo);
      }
      // Remove monitors which were loaded as separate workspaces
      for (const auto &run : loadedRuns) {
        AnalysisDataService::Instance().remove("TOF_" + run + "_monitors");
      }
      return false;
    }
    loadedRuns.insert(runNo);
  }
  return true;
}

/** Processes a group of runs
*
* @param groupID :: An integer number indicating the id of this group
* @param group :: the group of event workspaces
* @param timeSlicingType :: The type of time slicing being used
* @param timeSlicingValues :: The string of values to perform time slicing with
* @return :: true if errors were encountered
*/
bool ReflDataProcessorPresenter::processGroupAsEventWS(
    int groupID, const GroupData &group, const std::string &timeSlicingType,
    const std::string &timeSlicingValues) {

  bool errors = false;
  bool multiRow = group.size() > 1;
  size_t groupNumSlices = INT_MAX;

  std::vector<double> startTimes, stopTimes;

  // For custom slicing, the start/stop times are the same for all rows
  if (timeSlicingType == "Custom")
    parseCustom(timeSlicingValues, startTimes, stopTimes);

  for (const auto &row : group) {

    auto data = row.second;               // Vector containing data for this row
    std::string runNo = row.second.at(0); // The run number

    if (timeSlicingType == "UniformEven" || timeSlicingType == "Uniform") {
      const std::string runName = "TOF_" + runNo;
      parseUniform(timeSlicingValues, timeSlicingType, runName, startTimes,
                   stopTimes);
    }

    size_t numSlices = startTimes.size();

    for (size_t i = 0; i < numSlices; i++) {
      try {
        auto wsName = takeSlice(runNo, i, startTimes[i], stopTimes[i]);
        std::vector<std::string> slice(data);
        slice[0] = wsName;
        auto newData = reduceRow(slice);
        newData[0] = data[0];
        m_manager->update(groupID, row.first, newData);
      } catch (...) {
        return true;
      }
    }

    // For uniform slicing with multiple rows only the minimum number of slices
    // are common to each row
    if (multiRow && timeSlicingType == "Uniform")
      groupNumSlices = std::min(groupNumSlices, startTimes.size());
  }

  // Post-process (if needed)
  if (multiRow) {

    // All slices are common for uniform even or custom slicing
    if (timeSlicingType == "UniformEven" || timeSlicingType == "Custom")
      groupNumSlices = startTimes.size();

    for (size_t i = 0; i < groupNumSlices; i++) {
      GroupData groupNew;
      std::vector<std::string> data;
      for (const auto &row : group) {
        data = row.second;
        data[0] = row.second[0] + "_slice_" + std::to_string(i);
        groupNew[row.first] = data;
      }
      try {
        postProcessGroup(groupNew);
      } catch (...) {
        errors = true;
      }
    }
  }

  return errors;
}

/** Processes a group of non-event workspaces
*
* @param groupID :: An integer number indicating the id of this group
* @param group :: the group of event workspaces
* @return :: true if errors were encountered
*/
bool ReflDataProcessorPresenter::processGroupAsNonEventWS(
    int groupID, const GroupData &group) {

  bool errors = false;

  for (const auto &row : group) {

    // Reduce this row
    auto newData = reduceRow(row.second);
    // Update the tree
    m_manager->update(groupID, row.first, newData);
  }

  // Post-process (if needed)
  if (group.size() > 1) {
    try {
      postProcessGroup(group);
    } catch (...) {
      errors = true;
    }
  }

  return errors;
}

/** Parses a string to extract uniform time slicing
*
* @param timeSlicing :: The string to parse
* @param slicingType :: The type of uniform slicing being used
* @param wsName :: The name of the workspace to be sliced
* @param startTimes :: [output] A vector containing the start time for each
*slice
* @param stopTimes :: [output] A vector containing the stop time for each
*slice
*/
void ReflDataProcessorPresenter::parseUniform(const std::string &timeSlicing,
                                              const std::string &slicingType,
                                              const std::string &wsName,
                                              std::vector<double> &startTimes,
                                              std::vector<double> &stopTimes) {

  IEventWorkspace_sptr mws;
  if (AnalysisDataService::Instance().doesExist(wsName)) {
    mws = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(wsName);
  } else {
    m_mainPresenter->giveUserCritical("Workspace to slice not found: " + wsName,
                                      "Time slicing error");
    return;
  }

  const auto minTime = mws->getFirstPulseTime();
  const auto maxTime = mws->getLastPulseTime();
  const auto totalDuration = maxTime - minTime;
  double totalDurationSec = totalDuration.seconds();
  double sliceDuration = .0;
  int numSlices = 0;

  if (slicingType == "UniformEven") {
    numSlices = std::stoi(timeSlicing);
    sliceDuration = totalDurationSec / numSlices;
  } else if (slicingType == "Uniform") {
    sliceDuration = std::stod(timeSlicing);
    numSlices = static_cast<int>(ceil(totalDurationSec / sliceDuration));
  }

  // Add the start/stop times
  startTimes = std::vector<double>(numSlices);
  stopTimes = std::vector<double>(numSlices);
  for (int i = 0; i < numSlices; i++) {
    startTimes[i] = sliceDuration * i;
    stopTimes[i] = sliceDuration * (i + 1);
  }
}

/** Parses a string to extract custom time slicing
*
* @param timeSlicing :: The string to parse
* @param startTimes :: [output] A vector containing the start time for each
*slice
* @param stopTimes :: [output] A vector containing the stop time for each
*slice
*/
void ReflDataProcessorPresenter::parseCustom(const std::string &timeSlicing,
                                             std::vector<double> &startTimes,
                                             std::vector<double> &stopTimes) {

  std::vector<std::string> timesStr;
  boost::split(timesStr, timeSlicing, boost::is_any_of(","));

  std::vector<double> times;
  std::transform(timesStr.begin(), timesStr.end(), std::back_inserter(times),
                 [](const std::string &astr) { return std::stod(astr); });

  size_t numTimes = times.size();

  if (numTimes == 1) {
    startTimes.push_back(0);
    stopTimes.push_back(times[0]);
  } else if (numTimes == 2) {
    startTimes.push_back(times[0]);
    stopTimes.push_back(times[1]);
  } else {
    for (size_t i = 0; i < numTimes - 1; i++) {
      startTimes.push_back(times[i]);
      stopTimes.push_back(times[i + 1]);
    }
  }

  if (startTimes.size() != stopTimes.size())
    m_mainPresenter->giveUserCritical("Error parsing time slices",
                                      "Time slicing error");
}

/** Loads an event workspace and puts it into the ADS
*
* @param runNo :: the run number as a string
* @return :: True if algorithm was executed. False otherwise
*/
bool ReflDataProcessorPresenter::loadEventRun(const std::string &runNo) {

  std::string runName = "TOF_" + runNo;

  IAlgorithm_sptr algLoadRun =
      AlgorithmManager::Instance().create("LoadEventNexus");
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", m_view->getProcessInstrument() + runNo);
  algLoadRun->setProperty("OutputWorkspace", runName);
  algLoadRun->setProperty("LoadMonitors", true);
  algLoadRun->execute();
  return algLoadRun->isExecuted();
}

/** Loads a non-event workspace and puts it into the ADS
*
* @param runNo :: the run number as a string
*/
void ReflDataProcessorPresenter::loadNonEventRun(const std::string &runNo) {

  std::string runName = "TOF_" + runNo;

  IAlgorithm_sptr algLoadRun =
      AlgorithmManager::Instance().create("LoadISISNexus");
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", m_view->getProcessInstrument() + runNo);
  algLoadRun->setProperty("OutputWorkspace", runName);
  algLoadRun->execute();
}

/** Takes a slice from a run and puts the 'sliced' workspace into the ADS
*
* @param runNo :: the run number as a string
* @param sliceIndex :: the index of the slice being taken
* @param startTime :: start time
* @param stopTime :: stop time
* @return :: the name of the sliced workspace (without prefix 'TOF_')
*/
std::string ReflDataProcessorPresenter::takeSlice(const std::string &runNo,
                                                  size_t sliceIndex,
                                                  double startTime,
                                                  double stopTime) {

  std::string runName = "TOF_" + runNo;
  std::string sliceName = runName + "_slice_" + std::to_string(sliceIndex);
  std::string monName = runName + "_monitors";

  // Filter by time
  IAlgorithm_sptr filter = AlgorithmManager::Instance().create("FilterByTime");
  filter->initialize();
  filter->setProperty("InputWorkspace", runName);
  filter->setProperty("OutputWorkspace", sliceName);
  filter->setProperty("StartTime", startTime);
  filter->setProperty("StopTime", stopTime);
  filter->execute();

  // Get the normalization constant for this slice
  MatrixWorkspace_sptr mws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(runName);
  double total = mws->run().getProtonCharge();
  mws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sliceName);
  double slice = mws->run().getProtonCharge();
  double fraction = slice / total;

  IAlgorithm_sptr scale = AlgorithmManager::Instance().create("Scale");
  scale->initialize();
  scale->setProperty("InputWorkspace", monName);
  scale->setProperty("Factor", fraction);
  scale->setProperty("OutputWorkspace", "__" + monName + "_temp");
  scale->execute();

  IAlgorithm_sptr rebinDet =
      AlgorithmManager::Instance().create("RebinToWorkspace");
  rebinDet->initialize();
  rebinDet->setProperty("WorkspaceToRebin", sliceName);
  rebinDet->setProperty("WorkspaceToMatch", "__" + monName + "_temp");
  rebinDet->setProperty("OutputWorkspace", sliceName);
  rebinDet->setProperty("PreserveEvents", false);
  rebinDet->execute();

  IAlgorithm_sptr append = AlgorithmManager::Instance().create("AppendSpectra");
  append->initialize();
  append->setProperty("InputWorkspace1", "__" + monName + "_temp");
  append->setProperty("InputWorkspace2", sliceName);
  append->setProperty("OutputWorkspace", sliceName);
  append->setProperty("MergeLogs", true);
  append->execute();

  // Remove temporary monitor ws
  AnalysisDataService::Instance().remove("__" + monName + "_temp");

  return sliceName.substr(4);
}

/** Plots any currently selected rows */
void ReflDataProcessorPresenter::plotRow() {

  const auto items = m_manager->selectedData();
  if (items.size() == 0)
    return;

  // If slicing values are empty plot normally
  std::string timeSlicingValues = m_mainPresenter->getTimeSlicingValues();
  if (timeSlicingValues.empty()) {
    GenericDataProcessorPresenter::plotRow();
    return;
  }

  std::vector<double> startTimes, stopTimes;
  std::string timeSlicingType = m_mainPresenter->getTimeSlicingType();

  // Num of slices can be predetermined with uniform even or custom slicing
  if (timeSlicingType == "UniformEven") {
    const auto &row = items.at(0).at(0);
    const std::string wsName = getReducedWorkspaceName(row, "TOF_");
    parseUniform(timeSlicingValues, timeSlicingType, wsName, startTimes,
                 stopTimes);
  } else if (timeSlicingType == "Custom") {
    parseCustom(timeSlicingValues, startTimes, stopTimes);
  }

  size_t numSlices = startTimes.size();

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  for (const auto &item : items) {
    for (const auto &run : item.second) {

      if (timeSlicingType == "Uniform") {
        // Num slices for each ws in uniform slicing are separately determined
        const std::string wsName = getReducedWorkspaceName(run.second, "TOF_");
        parseUniform(timeSlicingValues, timeSlicingType, wsName, startTimes,
                     stopTimes);
        numSlices = startTimes.size();
      }

      const std::string wsName = getReducedWorkspaceName(run.second, "IvsQ_");

      for (size_t slice = 0; slice < numSlices; slice++) {
        const std::string sliceName =
            wsName + "_slice_" + std::to_string(slice);
        if (AnalysisDataService::Instance().doesExist(sliceName))
          workspaces.insert(sliceName);
        else
          notFound.insert(sliceName);
      }
    }
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
        "The following workspaces were not plotted because they were not "
        "found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the rows you are trying to plot have been "
            "fully processed.",
        "Error plotting rows.");

  plotWorkspaces(workspaces);
}

/** This method returns, for a given set of rows, i.e. a group of runs, the name
* of the output (post-processed) workspace
*
* @param groupData : The data in a given group
* @param prefix : A prefix to be appended to the generated ws name
* @param index : The index of the slice
* @returns : The name of the workspace
*/
std::string ReflDataProcessorPresenter::getPostprocessedWorkspaceName(
    const GroupData &groupData, const std::string &prefix, size_t index) {

  std::vector<std::string> outputNames;

  for (const auto &data : groupData) {
    outputNames.push_back(getReducedWorkspaceName(data.second) + "_slice_" +
                          std::to_string(index));
  }
  return prefix + boost::join(outputNames, "_");
}

/** Plots any currently selected groups */
void ReflDataProcessorPresenter::plotGroup() {

  const auto items = m_manager->selectedData();
  if (items.size() == 0)
    return;

  // If slicing values are empty plot normally
  std::string timeSlicingValues = m_mainPresenter->getTimeSlicingValues();
  if (timeSlicingValues.empty()) {
    GenericDataProcessorPresenter::plotGroup();
    return;
  }

  std::vector<double> startTimes, stopTimes;
  std::string timeSlicingType = m_mainPresenter->getTimeSlicingType();

  // No. of slices can be predetermined with uniform even or custom slicing
  if (timeSlicingType == "UniformEven") {
    const std::string wsName =
        getReducedWorkspaceName(items.at(0).at(0), "TOF_");
    parseUniform(timeSlicingValues, timeSlicingType, wsName, startTimes,
                 stopTimes);
  } else if (timeSlicingType == "Custom") {
    parseCustom(timeSlicingValues, startTimes, stopTimes);
  }

  size_t numSlices = startTimes.size();

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  for (const auto &item : items) {

    if (item.second.size() > 1) {

      // For uniform slicing, we must parse through each workspace to find the
      // minimum number of slices
      if (timeSlicingType == "Uniform") {
        numSlices = INT_MAX;

        for (const auto &run : item.second) {
          const std::string wsName =
              getReducedWorkspaceName(run.second, "TOF_");
          parseUniform(timeSlicingValues, timeSlicingType, wsName, startTimes,
                       stopTimes);
          numSlices = std::min(numSlices, startTimes.size());
        }
      }

      for (size_t slice = 0; slice < numSlices; slice++) {

        const std::string wsName =
            getPostprocessedWorkspaceName(item.second, "IvsQ_", slice);

        if (AnalysisDataService::Instance().doesExist(wsName))
          workspaces.insert(wsName);
        else
          notFound.insert(wsName);
      }
    }
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
        "The following workspaces were not plotted because they were not "
        "found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the groups you are trying to plot have been "
            "fully processed.",
        "Error plotting groups.");

  plotWorkspaces(workspaces);
}
}
}
