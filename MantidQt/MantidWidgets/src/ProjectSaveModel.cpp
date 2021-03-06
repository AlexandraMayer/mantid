#include "MantidQtMantidWidgets/ProjectSaveModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtAPI/WindowIcons.h"
#include "MantidQtAPI/WorkspaceIcons.h"

#include <unordered_set>

using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

/**
 * Construct a new model with a list of window handles
 * @param windows :: vector of handles to windows open in Mantid
 */
ProjectSaveModel::ProjectSaveModel(
    std::vector<IProjectSerialisable *> windows) {
  auto workspaces = getWorkspaces();
  for (auto &ws : workspaces) {
    std::pair<std::string, std::vector<IProjectSerialisable *>> item(
        ws->getName(), std::vector<IProjectSerialisable *>());
    m_workspaceWindows.insert(item);
  }

  for (auto window : windows) {
    auto wsNames = window->getWorkspaceNames();
    for (auto &name : wsNames) {
      m_workspaceWindows[name].push_back(window);
    }
  }
}

/**
 * Get windows which are associated with a given workspace name
 * @param wsName :: the name of the workspace to get window for
 * @return vector of window handles for the workspace
 */
std::vector<IProjectSerialisable *>
ProjectSaveModel::getWindows(const std::string &wsName) const {
  if (hasWindows(wsName)) {
    return m_workspaceWindows.at(wsName);
  }

  return std::vector<IProjectSerialisable *>();
}

/**
 * Get unique windows for a list of workspace names
 * @param wsNames :: vector of workspace names to get associated windows for
 * @return an ordered vector of unique window handles sorted by window name
 */
std::vector<IProjectSerialisable *> ProjectSaveModel::getUniqueWindows(
    const std::vector<std::string> &wsNames) const {
  std::unordered_set<IProjectSerialisable *> uniqueWindows;

  for (auto &name : wsNames) {
    for (auto window : getWindows(name)) {
      uniqueWindows.insert(window);
    }
  }

  std::vector<IProjectSerialisable *> windows(uniqueWindows.cbegin(),
                                              uniqueWindows.cend());
  std::sort(windows.begin(), windows.end(),
            [](IProjectSerialisable *lhs, IProjectSerialisable *rhs) {
              return lhs->getWindowName() < rhs->getWindowName();
            });

  return windows;
}

/**
 * Get all unique window names for a list of workspaces
 *
 * @param wsNames :: vector of workspace names to get associated window names
 * for
 * @return an ordered vector of unique window names sorted alphabetically
 */
std::vector<std::string> ProjectSaveModel::getWindowNames(
    const std::vector<std::string> &wsNames) const {
  std::vector<std::string> names;
  auto windows = getUniqueWindows(wsNames);
  for (auto window : windows) {
    names.push_back(window->getWindowName());
  }
  return names;
}

/**
 * Get all workspace names in the model
 * @return vector of all workspace names in the model
 */
std::vector<std::string> ProjectSaveModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  for (auto &item : m_workspaceWindows) {
    names.push_back(item.first);
  }

  std::sort(names.begin(), names.end());
  return names;
}

/**
 * Get window information for a selection of workspaces
 * @param wsNames :: vector of workspace names to find associated windows for
 * @return vector of window info objects associated with the workpaces
 */
std::vector<WindowInfo> ProjectSaveModel::getWindowInformation(
    const std::vector<std::string> &wsNames) const {
  std::vector<WindowInfo> winInfo;
  WindowIcons icons;

  for (auto window : getUniqueWindows(wsNames)) {
    WindowInfo info;
    info.name = window->getWindowName();
    info.type = window->getWindowType();
    info.icon_id = icons.getIconID(window->getWindowType());
    winInfo.push_back(info);
  }

  return winInfo;
}

/**
 * Get workspace information for all workspaces
 * @return vector of workspace info objects for all workspaces
 */
std::vector<WorkspaceInfo> ProjectSaveModel::getWorkspaceInformation() const {
  std::vector<WorkspaceInfo> wsInfo;

  auto items = AnalysisDataService::Instance().topLevelItems();
  for (auto item : items) {
    auto ws = item.second;
    auto info = makeWorkspaceInfoObject(ws);

    if (ws->id() == "WorkspaceGroup") {
      auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
      for (int i = 0; i < group->getNumberOfEntries(); ++i) {
        auto subInfo = makeWorkspaceInfoObject(group->getItem(i));
        info.subWorkspaces.push_back(subInfo);
      }
    }

    wsInfo.push_back(info);
  }

  return wsInfo;
}

/**
 * Get all workspaces from the ADS
 * @return vector of workspace handles from the ADS
 */
std::vector<Workspace_sptr> ProjectSaveModel::getWorkspaces() const {
  auto &ads = AnalysisDataService::Instance();
  return ads.getObjects();
}

WorkspaceInfo
ProjectSaveModel::makeWorkspaceInfoObject(Workspace_const_sptr ws) const {
  WorkspaceIcons icons;
  WorkspaceInfo info;
  info.name = ws->getName();
  info.numWindows = getWindows(ws->getName()).size();
  info.size = ws->getMemorySizeAsStr();
  info.icon_id = icons.getIconID(ws->id());
  info.type = ws->id();
  return info;
}

/**
 * Check is a workspace has any windows associated with it
 * @param wsName :: the name of workspace
 * @return whether the workspace has > 0 windows associated with it
 */
bool ProjectSaveModel::hasWindows(const std::string &wsName) const {
  auto item = m_workspaceWindows.find(wsName);
  if (item != m_workspaceWindows.end()) {
    return item->second.size() > 0;
  }

  return false;
}
