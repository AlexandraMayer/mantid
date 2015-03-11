#ifndef MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_

#include "ui_TomoReconstruction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtAPI/UserSubWindow.h"

#include <QDialog>
#include <jsoncpp/json/json.h>

class QTreeWidgetItem;
class QLineEdit;

namespace MantidQt {
namespace CustomInterfaces {
/**
Tomographu reconstruction GUI. Interface for editing parameters and
running and monitoring reconstruction jobs.

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport TomoReconstruction : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public: // public constructor, destructor and functions
  /// Default Constructor
  TomoReconstruction(QWidget *parent = 0);
  /// Destructor
  virtual ~TomoReconstruction();
  /// Interface name
  static std::string name() { return "Tomography Reconstruction"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Diffraction"; }
  /// Setup tab UI
  virtual void initLayout();

protected slots:
  /// for buttons, run tab
  void reconstructClicked();
  void toolSetupClicked();
  void runVisualizeClicked();
  void jobCancelClicked();
  void jobTableRefreshClicked();

protected:
  bool doPing();
  void doLogin(const std::string &pw);
  void doLogout();
  void doQueryJobStatus(std::vector<std::string> &ids,
                        std::vector<std::string> &names,
                        std::vector<std::string> &status,
                        std::vector<std::string> &cmds);
  void doSubmitReconstructionJob();
  void doCancelJob(const std::string &id);

  void makeRunnableWithOptions(std::string &run, std::string &opt);
  std::string getComputeResource();
  std::string getUsername();
  std::string getPassword();

  // current paths set by the user
  std::string currentPathSCARF();
  std::string currentPathFITS();
  std::string currentPathFlat();
  std::string currentPathDark();
  std::string currentPathSavuConfig();

private slots:
  void compResourceIndexChanged(int);
  void SCARFLoginClicked();
  void SCARFLogoutClicked();

  void browseImageClicked();

  void fitsPathBrowseClicked();
  void flatPathBrowseClicked();
  void darkPathBrowseClicked();
  void savuConfigFileBrowseClicked();

  void menuSaveClicked();
  void menuSaveAsClicked();
  void availablePluginSelected();
  void currentPluginSelected();
  void transferClicked();
  void moveUpClicked();
  void moveDownClicked();
  void removeClicked();
  void menuOpenClicked();
  void paramValModified(QTreeWidgetItem *, int);
  void expandedItem(QTreeWidgetItem *);

private:
  void doSetupSectionSetup();
  void doSetupSectionParameters();
  void doSetupSectionRun();

  void setupComputeResource();
  void setupRunTool();

  void enableLoggedActions(bool enable);
  void updateCompResourceStatus(bool online);

  void processPathBrowseClick(QLineEdit *le, std::string &data);

  /// Load default interface settings for each tab
  void loadSettings();

  std::string validateCompResource(const std::string &res);

  void drawImage(const Mantid::API::MatrixWorkspace_sptr &ws);

  void userWarning(std::string err, std::string description);

  void userError(std::string err, std::string description);

  std::string paramValStringFromArray(const Json::Value &jsonVal,
                                      const std::string &name);
  std::string pluginParamValString(const Json::Value &jsonVal,
                                   const std::string &name);

  /// to load plugins (savu classification / API)
  void loadAvailablePlugins();

  /// refresh the list/tree of savu plugins
  void refreshAvailablePluginListUI();

  void refreshCurrentPluginListUI();

  /// make a tree entry from a row of a table of savu plugins
  void createPluginTreeEntry(Mantid::API::TableRow &row);
  void createPluginTreeEntries(Mantid::API::ITableWorkspace_sptr table);

  std::string createUniqueNameHidden();

  QString tableWSRowToString(Mantid::API::ITableWorkspace_sptr table,
                             size_t i);

  void loadSavuTomoConfig(std::string &filePath,
                          Mantid::API::ITableWorkspace_sptr &currentPlugins);

  /// Main interface window
  Ui::TomoReconstruction m_ui;

  /// login status (from local perspective)
  bool m_loggedIn;

  /// facility for the remote compute resource
  const std::string m_facility;
  /// compute resources suppoted by this GUI (remote ones, clusters, etc.)
  std::vector<std::string> m_computeRes;
  /// display name of the "local" compute resource
  const std::string m_localCompName;

  /// reduction tools
  std::vector<std::string> m_SCARFtools;

  /// file paths, base dir on scarf
  std::string m_pathSCARFbase;
  /// path to fits file (sample data)
  std::string m_pathFITS;
  /// path to flat/bright image
  std::string m_pathFlat;
  /// path to dark image
  std::string m_pathDark;
  /// path to an tomography config file (savu NX format)
  std::string m_pathSavuConfigFile;

  static const std::string m_SCARFName;

  // Names of image reconstruction tools
  static const std::string m_TomoPyTool;
  static const std::string m_AstraTool;
  static const std::string m_CCPiTool;
  static const std::string m_SavuTool;
  static const std::string m_CustomCmdTool;

  // plugins for savu config files
  //std::vector<Mantid::API::ITableWorkspace_sptr> m_availPlugins;
  Mantid::API::ITableWorkspace_sptr m_availPlugins;
  //std::vector<Mantid::API::ITableWorkspace_sptr> m_currPlugins;
  Mantid::API::ITableWorkspace_sptr m_currPlugins;
  std::string m_currentParamPath;
  static size_t m_nameSeqNo;
};

class TomoToolSetupDialog: public QDialog {
  Q_OBJECT

public:
  TomoToolSetupDialog(QWidget *parent = 0);

private slots:
  void okClicked();
  void cancelClicked();

private:

  QLabel *labelRun, *labelOpt;
  QLineEdit *editRun, *editOpt;
  QHBoxLayout *hRun, *hOpt;
  QGridLayout *layout;
  QPushButton *okButton, *cancelButton;
};

}
}

#endif // MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_
