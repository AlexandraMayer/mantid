//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"
#include "MantidQtCustomInterfaces/MuonAnalysisOptionTab.h"
#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/cow_ptr.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/StringTokenizer.h>
#include <boost/lexical_cast.hpp>

#include <algorithm>

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>

#include <fstream>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(MuonAnalysis);

using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Muon;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Initialize the logger
Logger& MuonAnalysis::g_log = Logger::get("MuonAnalysis");

//----------------------
// Public member functions
//----------------------
///Constructor
MuonAnalysis::MuonAnalysis(QWidget *parent) :
  UserSubWindow(parent), m_last_dir(), m_workspace_name("MuonAnalysis"), m_currentDataName(""), m_groupTableRowInFocus(0), m_pairTableRowInFocus(0),
  m_tabNumber(0), m_groupNames(), m_groupingTempFilename("tempMuonAnalysisGrouping.xml"), m_settingsGroup("CustomInterfaces/MuonAnalysis/")
{
}

/// Set up the dialog layout
void MuonAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  // Further set initial look
  startUpLook();
  createMicroSecondsLabels(m_uiForm);
  m_uiForm.mwRunFiles->readSettings(m_settingsGroup + "mwRunFilesBrowse");

  connect(m_uiForm.previousRun, SIGNAL(clicked()), this, SLOT(checkAppendingPreviousRun()));
  connect(m_uiForm.nextRun, SIGNAL(clicked()), this, SLOT(checkAppendingNextRun()));

  m_uiForm.appendRun->setDisabled(true);

  m_optionTab = new MuonAnalysisOptionTab(m_uiForm, m_settingsGroup);
  m_optionTab->initLayout();

  // connect guess alpha 
  connect(m_uiForm.guessAlphaButton, SIGNAL(clicked()), this, SLOT(guessAlphaClicked())); 

        // signal/slot connections to respond to changes in instrument selection combo boxes
        connect(m_uiForm.instrSelector, SIGNAL(instrumentSelectionChanged(const QString&)), this, SLOT(userSelectInstrument(const QString&)));

  // Load current
  connect(m_uiForm.loadCurrent, SIGNAL(clicked()), this, SLOT(runLoadCurrent())); 

  // If group table change
  // currentCellChanged ( int currentRow, int currentColumn, int previousRow, int previousColumn )
  connect(m_uiForm.groupTable, SIGNAL(cellChanged(int, int)), this, SLOT(groupTableChanged(int, int))); 
  connect(m_uiForm.groupTable, SIGNAL(cellClicked(int, int)), this, SLOT(groupTableClicked(int, int))); 
  connect(m_uiForm.groupTable->verticalHeader(), SIGNAL(sectionClicked(int)), SLOT(groupTableClicked(int)));


  // group table plot button
  connect(m_uiForm.groupTablePlotButton, SIGNAL(clicked()), this, SLOT(runGroupTablePlotButton())); 

  // If pair table change
  connect(m_uiForm.pairTable, SIGNAL(cellChanged(int, int)), this, SLOT(pairTableChanged(int, int))); 
  connect(m_uiForm.pairTable, SIGNAL(cellClicked(int, int)), this, SLOT(pairTableClicked(int, int)));
  connect(m_uiForm.pairTable->verticalHeader(), SIGNAL(sectionClicked(int)), SLOT(pairTableClicked(int)));
  // Pair table plot button
  connect(m_uiForm.pairTablePlotButton, SIGNAL(clicked()), this, SLOT(runPairTablePlotButton())); 

  // save grouping
  connect(m_uiForm.saveGroupButton, SIGNAL(clicked()), this, SLOT(runSaveGroupButton())); 

  // load grouping
  connect(m_uiForm.loadGroupButton, SIGNAL(clicked()), this, SLOT(runLoadGroupButton())); 

  // clear grouping
  connect(m_uiForm.clearGroupingButton, SIGNAL(clicked()), this, SLOT(runClearGroupingButton())); 

  // front plot button
  connect(m_uiForm.frontPlotButton, SIGNAL(clicked()), this, SLOT(runFrontPlotButton())); 

  // front group/ group pair combobox
  connect(m_uiForm.frontGroupGroupPairComboBox, SIGNAL(currentIndexChanged(int)), this, 
    SLOT(runFrontGroupGroupPairComboBox(int)));

  // connect "?" (Help) Button
  connect(m_uiForm.muonAnalysisHelp, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpClicked()));
  connect(m_uiForm.muonAnalysisHelpGrouping, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpGroupingClicked()));
  connect(m_uiForm.muonAnalysisHelpPlotting, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpPlottingClicked()));
  connect(m_uiForm.muonAnalysisHelpDataAnalysis, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpDataAnalysisClicked()));

  // add combo boxes to pairTable
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }

  // file input 
  connect(m_uiForm.mwRunFiles, SIGNAL(fileEditingFinished()), this, SLOT(inputFileChanged_MWRunFiles()));

  // Input check for First Good Data
  connect(m_uiForm.firstGoodBinFront, SIGNAL(lostFocus()), this, 
    SLOT(runFirstGoodBinFront()));

  // load previous saved values
  loadAutoSavedValues(m_settingsGroup);

  // connect the fit function widget buttons to their respective slots.
  loadFittings();

  // If the data is set to bunch then fit against the bunched data, if make raw then fit against the raw data
  connect(m_uiForm.fitBrowser,SIGNAL(rawData(const std::string&)), this, SLOT(makeRaw(const std::string&)));
  connect(m_uiForm.fitBrowser,SIGNAL(bunchData(const std::string&)), this, SLOT(reBunch(const std::string&)));

  // Detected a workspace change and therefore the peak picker tool needs to be reassigned.
  connect(m_uiForm.fitBrowser,SIGNAL(wsChangePPAssign(const QString &)), this, SLOT(assignPeakPickerTool(const QString &)));

  // Detect when the tab is changed
  connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeTab(int)));

  // Detect when fitting has started, change the plot style to the one specified in plot details tab.
  connect(m_uiForm.fitBrowser,SIGNAL(changeFitPlotStyle(const QString &)), this, SLOT(changeFitPlotType(const QString &)));
}


/**
* Muon Analysis help (slot)
*/
void MuonAnalysis::muonAnalysisHelpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysis"));
}

/**
* Muon Analysis Grouping help (slot)
*/
void MuonAnalysis::muonAnalysisHelpGroupingClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisGrouping"));
}

/**
* Muon Analysis Plotting help (slot)
*/
void MuonAnalysis::muonAnalysisHelpPlottingClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisPlotting"));
}

/**
* Muon Analysis Data Analysis help (slot)
*/
void MuonAnalysis::muonAnalysisHelpDataAnalysisClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisDataAnalysis"));
}

/**
* Front group/ group pair combobox (slot)
*/
void MuonAnalysis::runFrontGroupGroupPairComboBox(int index)
{
  if ( index >= 0 )
    updateFront();
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysis::runFirstGoodBinFront()
{
  try 
  {
    boost::lexical_cast<double>(m_uiForm.firstGoodBinFront->text().toStdString());
    
    // if this value updated then also update 'Start at" Plot option if "Start at First Good Data" set
    if (m_uiForm.timeComboBox->currentIndex() == 0 )
    {
      m_uiForm.timeAxisStartAtInput->setText(m_uiForm.firstGoodBinFront->text());
    }
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in First Good Data (ms)' input box. Reset to 0.3.");
    m_uiForm.firstGoodBinFront->setText("0.3");
  }
}


/**
* Front plot button (slot)
*/
void MuonAnalysis::runFrontPlotButton()
{
  // get current index
  int index = m_uiForm.frontGroupGroupPairComboBox->currentIndex();

  if (index >= numGroups())
  {
    // i.e. index points to a pair
    m_pairTableRowInFocus = m_pairToRow[index-numGroups()];  // this can be improved
    std::string str = m_uiForm.frontPlotFuncs->currentText().toStdString();
    plotPair(str);
  }
  else
  {
    m_groupTableRowInFocus = m_groupToRow[index];
    std::string str = m_uiForm.frontPlotFuncs->currentText().toStdString();
    plotGroup(str);
  }
}


/**
* If the instrument selection has changed (slot)
*
* @param prefix :: instrument name from QComboBox object
*/
void MuonAnalysis::userSelectInstrument(const QString& prefix) 
{
        if ( prefix != m_curInterfaceSetup )
        {
                runClearGroupingButton();
    m_curInterfaceSetup = prefix;

    // save this new choice
    QSettings group;
    group.beginGroup(m_settingsGroup + "instrument");
    group.setValue("name", prefix); 
        }
}


/**
 * Save grouping button (slot)
 */
void MuonAnalysis::runSaveGroupButton()
{
  if ( numGroups() <= 0 )
  {
    QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "No grouping to save.");
    return;
  }

  QSettings prevValues;
  prevValues.beginGroup(m_settingsGroup + "SaveOutput");

  // Get value for "dir". If the setting doesn't exist then use
  // the the path in "defaultsave.directory"
  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultsave.directory"))).toString();

  QString filter;
  filter.append("Files (*.XML *.xml)");
  filter += ";;AllFiles (*.*)";
  QString groupingFile = API::FileDialogHandler::getSaveFileName(this,
                                   "Save Grouping file as", prevPath, filter);

  if( ! groupingFile.isEmpty() )
  {
    saveGroupingTabletoXML(m_uiForm, groupingFile.toStdString());
    
    QString directory = QFileInfo(groupingFile).path();
    prevValues.setValue("dir", directory);
  }
}


/**
 * Load grouping button (slot)
 */
void MuonAnalysis::runLoadGroupButton()
{
  // Get grouping file

  QSettings prevValues;
  prevValues.beginGroup(m_settingsGroup + "LoadGroupFile");

  // Get value for "dir". If the setting doesn't exist then use
  // the the path in "defaultsave.directory"
  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultload.directory"))).toString();

  QString filter;
  filter.append("Files (*.XML *.xml)");
  filter += ";;AllFiles (*.*)";
  QString groupingFile = QFileDialog::getOpenFileName(this, "Load Grouping file", prevPath, filter);    
  if( groupingFile.isEmpty() || QFileInfo(groupingFile).isDir() ) 
    return;
    
  QString directory = QFileInfo(groupingFile).path();
  prevValues.setValue("dir", directory);

  saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
  clearTablesAndCombo();

  try
  {
    loadGroupingXMLtoTable(m_uiForm, groupingFile.toStdString());
  }
  catch (Exception::FileError& e)
  {
    g_log.error(e.what());
    g_log.error("Revert to previous grouping");
    loadGroupingXMLtoTable(m_uiForm, m_groupingTempFilename);
  }


  // add number of detectors column to group table

  int numRows = m_uiForm.groupTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *item = m_uiForm.groupTable->item(i,1);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;

    std::stringstream detNumRead;
    try
    {
      detNumRead << numOfDetectors(item->text().toStdString());
      m_uiForm.groupTable->setItem(i, 2, new QTableWidgetItem(detNumRead.str().c_str()));
    }
    catch (...)
    {
      m_uiForm.groupTable->setItem(i, 2, new QTableWidgetItem("Invalid"));
    }
  }

  updateFront();
}

/**
 * Clear grouping button (slot)
 */
void MuonAnalysis::runClearGroupingButton()
{
  clearTablesAndCombo();

  // also disable plotting buttons and cal alpha button
  noDataAvailable();
}

/**
 * Group table plot button (slot)
 */
void MuonAnalysis::runGroupTablePlotButton()
{
  plotGroup(m_uiForm.groupTablePlotChoice->currentText().toStdString());
}

/**
 * Load current (slot)
 */
void MuonAnalysis::runLoadCurrent()
{
  QString instname = m_uiForm.instrSelector->currentText().toUpper();

  // If Argus data then simple
  if ( instname == "ARGUS" )
  {
    QString argusDAE = "\\\\ndw828\\argusdata\\current cycle\\nexus\\argus0000000.nxs";
    Poco::File l_path( argusDAE.toStdString() );
    if ( !l_path.exists() )
    {
      QMessageBox::warning(this,"Mantid - MuonAnalysis", 
        QString("Can't load ARGUS Current data since\n") +
        argusDAE + QString("\n") +
        QString("does not seem to exist"));
      return;
    }
    m_previousFilename = argusDAE;
    inputFileChanged(argusDAE);
    return;
  }

  if ( instname == "EMU" || instname == "HIFI" || instname == "MUSR")
  {
    // first check if autosave.run exist
    std::string autosavePointsTo = "";
    std::string autosaveFile = "\\\\" + instname.toStdString() + "\\data\\autosave.run";
    Poco::File pathAutosave( autosaveFile );
    if ( pathAutosave.exists() )
    {
      std::ifstream autofileIn(autosaveFile.c_str(), std::ifstream::in); 
      autofileIn >> autosavePointsTo;
    }    

    QString psudoDAE;
    if ( autosavePointsTo.empty() )
      psudoDAE = "\\\\" + instname + "\\data\\" + instname + "auto_A.tmp";
    else
      psudoDAE = "\\\\" + instname + "\\data\\" + autosavePointsTo.c_str();

    Poco::File l_path( psudoDAE.toStdString() );
    if ( !l_path.exists() )
    {
      QMessageBox::warning(this,"Mantid - MuonAnalysis", 
        QString("Can't load ") + "EMU Current data since\n" +
        psudoDAE + QString("\n") +
        QString("does not seem to exist"));
      return;
    }
    m_previousFilename = psudoDAE;
    inputFileChanged(psudoDAE);
    return;
  }

  QString daename = "NDX" + instname;

  // Load dae file
  AnalysisDataService::Instance().remove(m_workspace_name);

  QString pyString = 
      "from mantidsimple import *\n"
      "import sys\n"
      "try:\n"
      "  LoadDAE('" + daename + "','" + m_workspace_name.c_str() + "')\n"
      "except SystemExit, message:\n"
      "  print str(message)";
  QString pyOutput = runPythonCode( pyString ).trimmed();

  // if output is none empty something has gone wrong
  if ( !pyOutput.toStdString().empty() )
  {
    noDataAvailable();
    QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't read from " + daename + ". Plotting disabled");
    return;
  }

  nowDataAvailable();

  // Get hold of a pointer to a matrix workspace and apply grouping if applicatable
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  int numPeriods = 1;   // 1 may mean either a group with one period or simply just 1 normal matrix workspace
  if (wsPeriods)
  {
    numPeriods = wsPeriods->getNumberOfEntries();

    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
  }
  else
  {
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  }

  if ( !isGroupingSet() )
  {
    std::stringstream idstr;
    idstr << "1-" << matrix_workspace->getNumberHistograms();
    m_uiForm.groupTable->setItem(0, 0, new QTableWidgetItem("NoGroupingDetected"));
    m_uiForm.groupTable->setItem(0, 1, new QTableWidgetItem(idstr.str().c_str()));
    updateFrontAndCombo();
  }

  if ( !applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouped") )
    return;

  // Populate instrument fields

  std::stringstream str;
  str << "Description: ";
  int nDet = static_cast<int>(matrix_workspace->getInstrument()->getDetectorIDs().size());
  str << nDet;
  str << " detector spectrometer, main field ";
  str << "unknown"; 
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());


  // Populate run information text field

  std::string infoStr = "Number of spectra in data = ";
  infoStr += boost::lexical_cast<std::string>(matrix_workspace->getNumberHistograms()) + "\n"; 
  infoStr += "Title: "; 
  infoStr += matrix_workspace->getTitle() + "\n" + "Comment: "
    + matrix_workspace->getComment();
  m_uiForm.infoBrowser->setText(infoStr.c_str());


  // Populate period information

  std::stringstream periodLabel;
  periodLabel << "Data collected in " << numPeriods << " Periods. " 
    << "Plot/analyse Period:";
  m_uiForm.homePeriodsLabel->setText(periodLabel.str().c_str());

  while ( m_uiForm.homePeriodBox1->count() != 0 )
    m_uiForm.homePeriodBox1->removeItem(0);
  while ( m_uiForm.homePeriodBox2->count() != 0 )
    m_uiForm.homePeriodBox2->removeItem(0);

  m_uiForm.homePeriodBox2->addItem("None");
  for ( int i = 1; i <= numPeriods; i++ )
  {
    std::stringstream strInt;
    strInt << i;
    m_uiForm.homePeriodBox1->addItem(strInt.str().c_str());
    m_uiForm.homePeriodBox2->addItem(strInt.str().c_str());
  }

  if (wsPeriods)
  {
    m_uiForm.homePeriodBox2->setEnabled(true);
    m_uiForm.homePeriodBoxMath->setEnabled(true);
  }
  else
  {
    m_uiForm.homePeriodBox2->setEnabled(false);
    m_uiForm.homePeriodBoxMath->setEnabled(false);
  }  
}


/**
 * Pair table plot button (slot)
 */
void MuonAnalysis::runPairTablePlotButton()
{
  plotPair(m_uiForm.pairTablePlotChoice->currentText().toStdString());
}

/**
 * Pair table vertical lable clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row)
{
  m_pairTableRowInFocus = row;

  // if something sensible in row then update front
  int pNum = getPairNumberFromRow(row);
  if ( pNum >= 0 )
  {
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(pNum+numGroups());
    updateFront();
  }
}

/**
 * Pair table clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row, int column)
{
  (void) column;

  pairTableClicked(row);
}

/**
 * Group table clicked (slot)
 */
void MuonAnalysis::groupTableClicked(int row, int column)
{
  (void) column;

  groupTableClicked(row);
}

/**
 * Group table clicked (slot)
 */
void MuonAnalysis::groupTableClicked(int row)
{
  m_groupTableRowInFocus = row;

  // if something sensible in row then update front
  int gNum = getGroupNumberFromRow(row);
  if ( gNum >= 0 )
  {
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(gNum);
    updateFront();
  }
}


/**
 * Group table changed, e.g. if:         (slot)
 *
 *    1) user changed detector sequence 
 *    2) user type in a group name
 *
 * @param row :: 
 * @param column
::  */
void MuonAnalysis::groupTableChanged(int row, int column)
{
 // if ( column == 2 )
 //   return;

  // changes to the IDs
  if ( column == 1 )
  {
    QTableWidgetItem* itemNdet = m_uiForm.groupTable->item(row,2);
    QTableWidgetItem *item = m_uiForm.groupTable->item(row,1);

    // if IDs list has been changed to empty string
    if (item->text() == "")
    {
      if (itemNdet)
        itemNdet->setText("");
    }
    else
    {
      int numDet = numOfDetectors(item->text().toStdString());
      std::stringstream detNumRead;
      if (numDet > 0 )
      {
        detNumRead << numDet;
        if (itemNdet == NULL)
          m_uiForm.groupTable->setItem(row,2, new QTableWidgetItem(detNumRead.str().c_str()));
        else
        { 
          itemNdet->setText(detNumRead.str().c_str());
        }
        //checkIf_ID_dublicatesInTable(row);
      }
      else
      {
        if (itemNdet == NULL)
          m_uiForm.groupTable->setItem(row,2, new QTableWidgetItem("Invalid IDs string"));
        else
          m_uiForm.groupTable->item(row, 2)->setText("Invalid IDs string");
      }
    }   
  }

  // Change to group name
  if ( column == 0 )
  {
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(row,0);

    if ( itemName == NULL )  // this should never happen
      m_uiForm.groupTable->setItem(row,0, new QTableWidgetItem(""));
      
    if ( itemName->text() != "" )
    {
      // check that the group name entered does not already exist
      for (int i = 0; i < m_uiForm.groupTable->rowCount(); i++)
      {
        if (i==row)
          continue;

        QTableWidgetItem *item = m_uiForm.groupTable->item(i,0);
        if (item)
        {
          if ( item->text() == itemName->text() )
          {
            QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Group names must be unique. Please re-enter Group name.");
            itemName->setText("");
            break;
          }
        }
      }
    }
  }  

  whichGroupToWhichRow(m_uiForm, m_groupToRow);
  applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouped");
  updatePairTable();
  updateFrontAndCombo();
}


/**
 * Pair table changed, e.g. if:         (slot)
 *
 *    1) user changed alpha value
 *    2) pair name changed
 *
 * @param row :: 
 * @param column
::  */
void MuonAnalysis::pairTableChanged(int row, int column)
{
  // alpha been modified
  if ( column == 3 )
  {
    QTableWidgetItem* itemAlpha = m_uiForm.pairTable->item(row,3);

    if ( !itemAlpha->text().toStdString().empty() )
    {
      try
      {
         boost::lexical_cast<double>(itemAlpha->text().toStdString().c_str());
      }  catch (boost::bad_lexical_cast&)
      {
        QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Alpha must be a number.");
        itemAlpha->setText("");
        return;
      }
    }
    whichPairToWhichRow(m_uiForm, m_pairToRow);
    updateFrontAndCombo();
  }

  // pair name been modified
  if ( column == 0 )
  {
    QTableWidgetItem *itemName = m_uiForm.pairTable->item(row,0);

    if ( itemName == NULL )  // this should never happen
      m_uiForm.pairTable->setItem(row,0, new QTableWidgetItem(""));
      
    if ( itemName->text() != "" )
    {
      // check that the group name entered does not already exist
      for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
      {
        if (i==row)
          continue;

        QTableWidgetItem *item = m_uiForm.pairTable->item(i,0);
        if (item)
        {
          if ( item->text() == itemName->text() )
          {
            QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Pair names must be unique. Please re-enter Pair name.");
            itemName->setText("");
          }
        }
      }
    }

    whichPairToWhichRow(m_uiForm, m_pairToRow);
    updateFrontAndCombo();

    // check to see if alpha is specified (if name!="") and if not
    // assign a default of 1.0
    if ( itemName->text() != "" )
    {
      QTableWidgetItem* itemAlpha = m_uiForm.pairTable->item(row,3);

      if (itemAlpha)
      {
        if ( itemAlpha->text().toStdString().empty() )
        {
          itemAlpha->setText("1.0");
        }
      }
      else
      {
        m_uiForm.pairTable->setItem(row,3, new QTableWidgetItem("1.0"));
      }
    }
    
  }  

}

/**
 * Update pair table
 */
void MuonAnalysis::updatePairTable()
{
  // number of groups has dropped below 2 and pair names specified then
  // clear pair table
  if ( numGroups() < 2 && numPairs() > 0 )
  { 
    m_uiForm.pairTable->clearContents();
    for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
    {
      m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
      m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
    }
    updateFrontAndCombo();
    return;
  }
  else if ( numGroups() < 2 && numPairs() <= 0 )
  {
    return;
  }


  // get previous number of groups as listed in the pair comboboxes
  QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(0,1));
  int previousNumGroups = qwF->count(); // how many groups listed in pair combobox
  int newNumGroups = numGroups();

  // reset context of combo boxes
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
    QComboBox* qwB = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));

    if (previousNumGroups < newNumGroups)
    {
      // then need to increase the number of entrees in combo box
      for (int ii = 1; ii <= newNumGroups-previousNumGroups; ii++)
      {
        qwF->addItem(""); // effectively here just allocate space for extra items
        qwB->addItem("");
      }
    }
    else if (previousNumGroups > newNumGroups)
    {
      // then need to decrease the number of entrees in combo box
      for (int ii = 1; ii <= previousNumGroups-newNumGroups; ii++)
      {
        qwF->removeItem(qwF->count()-1); // remove top items 
        qwB->removeItem(qwB->count()-1);
      }

      // further for this case check that none of the current combo box
      // indexes are larger than the number of groups
      if ( qwF->currentIndex()+1 > newNumGroups || qwB->currentIndex()+1 > newNumGroups )
      {
        qwF->setCurrentIndex(0);
        qwB->setCurrentIndex(1);
      }
    }

    if ( qwF->currentIndex() == 0 && qwB->currentIndex() == 0 )
      qwB->setCurrentIndex(1);

    // re-populate names in combo boxes with group names
    for (int ii = 0; ii < newNumGroups; ii++)
    {
      qwF->setItemText(ii, m_uiForm.groupTable->item(m_groupToRow[ii],0)->text());
      qwB->setItemText(ii, m_uiForm.groupTable->item(m_groupToRow[ii],0)->text());
    }
  }
}


/**
 * Do some check when reading from MWRun, before actually reading new data file, to see if file is valid (slot)
 */
void MuonAnalysis::inputFileChanged_MWRunFiles()
{
  if ( m_uiForm.mwRunFiles->getText().isEmpty() )
    return;

  if ( !m_uiForm.mwRunFiles->isValid() )
  {
    return;
  }

  if ( m_previousFilename.compare(m_uiForm.mwRunFiles->getFirstFilename()) == 0 )
    return;

  m_previousFilename = m_uiForm.mwRunFiles->getFirstFilename();

  int difference(0);
  int appendSeparator(-1);
  appendSeparator = m_previousFilename.find("-");

  if (appendSeparator != -1)
  {
    //if a range has been selected then opent hem all
    //first split into files
    QString currentFile = m_uiForm.mwRunFiles->getText();//m_previousFilename; // m_uiForm.mwRunFiles->getFirstFilename();
    
    int lowSize(-1);
    int lowLimit(-1);
    QString fileExtension("");
    QString lowString("");

    //Get the file extension and then remove it from the current file
    int temp(currentFile.size()-currentFile.find("."));
    fileExtension = currentFile.right(temp);
    currentFile.chop(temp);
    
    //Get the max value and then chop this off
    QString maxString = currentFile.right(currentFile.size() - appendSeparator - 1);
    int maxSize = maxString.size();
    int maxLimit = maxString.toInt();
    //include chopping off the "-" symbol
    currentFile.chop(maxSize + 1);

    separateMuonFile(currentFile, lowSize, lowLimit);
    difference = maxLimit - lowLimit;

    for(int i = 0; i<=difference; ++i)
    {
      lowString = lowString.setNum(lowLimit + i);
      getFullCode(lowSize, lowString);
      m_previousFilename = currentFile + lowString + fileExtension;
      // in case file is selected from browser button check that it actually exist
      Poco::File l_path( m_previousFilename.toStdString() );
      if ( !l_path.exists() )
      {
        QMessageBox::warning(this,"Mantid - MuonAnalysis", m_previousFilename + "Specified data file does not exist.");
        return;
      }
  
      // save selected browse file directory to be reused next time interface is started up
      m_uiForm.mwRunFiles->saveSettings(m_settingsGroup + "mwRunFilesBrowse");

      inputFileChanged(m_previousFilename);
    }
  }
  else
  {
    // in case file is selected from browser button check that it actually exist
    Poco::File l_path( m_previousFilename.toStdString() );
    if ( !l_path.exists() )
    {
      QMessageBox::warning(this,"Mantid - MuonAnalysis", m_previousFilename + "Specified data file does not exist.");
      return;
    }
  
    // save selected browse file directory to be reused next time interface is started up
    m_uiForm.mwRunFiles->saveSettings(m_settingsGroup + "mwRunFilesBrowse");

    inputFileChanged(m_previousFilename);
  }
}

/**
 * Input file changed. Update GUI accordingly.
 * Note this method does no check of input filename assumed
 * done elsewhere depending on e.g. whether filename came from
 * MWRunFiles or 'get current run' button
 * @param filename Filename of new data file
 */
void MuonAnalysis::inputFileChanged(const QString& filename)
{
  Poco::File l_path( filename.toStdString() );

  // and check if file is from a recognised instrument and update instrument combo box
  QString filenamePart = (Poco::Path(l_path.path()).getFileName()).c_str();
  filenamePart = filenamePart.toLower();
  bool foundInst = false;
  for (int i = 0; i < m_uiForm.instrSelector->count(); i++)
  {
    QString instName = m_uiForm.instrSelector->itemText(i).toLower();
    
    std::string sfilename = filenamePart.toStdString();
    std::string sinstName = instName.toStdString();
    size_t found;
    found = sfilename.find(sinstName);
    if ( found != std::string::npos )
    {
      m_uiForm.instrSelector->setCurrentIndex(i);
      foundInst = true;
      break;
    }
  }
  if ( !foundInst )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Muon file not recognised.");
    return;
  }

  QString pyString = "from mantidsimple import *\n"
      "import sys\n"
      "try:\n"
      "  alg = LoadMuonNexus(r'" + filename + "','" + m_workspace_name.c_str() + "', AutoGroup='0')\n"
      "  print alg.getPropertyValue('MainFieldDirection'), alg.getPropertyValue('TimeZero'), alg.getPropertyValue('FirstGoodData')\n"
      "except SystemExit, message:\n"
      "  print ''";
  QString outputParams = runPythonCode( pyString ).trimmed();

  if ( outputParams.isEmpty() )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Problem when executing LoadMuonNexus algorithm.");
    return;
  }

  nowDataAvailable();

  // get hold of output parameters
  std::stringstream strParam(outputParams.toStdString());
  std::string mainFieldDirection;
  double timeZero;
  double firstGoodData;
  strParam >> mainFieldDirection >> timeZero >> firstGoodData;

  // Get hold of a pointer to a matrix workspace and apply grouping if applicatable
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  int numPeriods = 1;   // 1 may mean either a group with one period or simply just 1 normal matrix workspace
  if (wsPeriods)
  {
    numPeriods = wsPeriods->getNumberOfEntries();

    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
  }
  else
  {
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  }

  // if grouping not set, first see if grouping defined in Nexus
  if ( !isGroupingSet() )
    setGroupingFromNexus(filename);
  // if grouping still not set, then take grouping from IDF
  if ( !isGroupingSet() )
    setGroupingFromIDF(mainFieldDirection, matrix_workspace);
  // finally if nothing else works set dummy grouping and display
  // message to user
  if ( !isGroupingSet() )
    setDummyGrouping(static_cast<int>(matrix_workspace->getInstrument()->getDetectorIDs().size()));


  if ( !applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouped") )
    return;

  // Populate instrument fields

  std::stringstream str;
  str << "Description: ";
  int nDet = static_cast<int>(matrix_workspace->getInstrument()->getDetectorIDs().size());
  str << nDet;
  str << " detector spectrometer, main field ";
  str << QString(mainFieldDirection.c_str()).toLower().toStdString(); 
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());

  m_uiForm.timeZeroFront->setText(QString::number(timeZero, 'g',2));
  m_uiForm.firstGoodBinFront->setText(QString::number(firstGoodData-timeZero,'g',2));
  // since content of first-good-bin changed run this slot
  runFirstGoodBinFront();


  // Populate run information text field
  m_title = matrix_workspace->getTitle();
  std::string infoStr = "Title = ";
  infoStr += m_title + "\n" + "Comment: "
    + matrix_workspace->getComment() + "\n";
  const Run& runDetails = matrix_workspace->run();
  infoStr += "Start: ";
  if ( runDetails.hasProperty("run_start") )
  {
    infoStr += runDetails.getProperty("run_start")->value();
  }
  infoStr += "\nEnd: ";
  if ( runDetails.hasProperty("run_end") )
  {
    infoStr += runDetails.getProperty("run_end")->value();
  }
  m_uiForm.infoBrowser->setText(infoStr.c_str());


  // Populate period information

  std::stringstream periodLabel;
  periodLabel << "Data collected in " << numPeriods << " Periods. " 
    << "Plot/analyse Period:";
  m_uiForm.homePeriodsLabel->setText(periodLabel.str().c_str());

  while ( m_uiForm.homePeriodBox1->count() != 0 )
    m_uiForm.homePeriodBox1->removeItem(0);
  while ( m_uiForm.homePeriodBox2->count() != 0 )
    m_uiForm.homePeriodBox2->removeItem(0);

  m_uiForm.homePeriodBox2->addItem("None");
  for ( int i = 1; i <= numPeriods; i++ )
  {
    std::stringstream strInt;
    strInt << i;
    m_uiForm.homePeriodBox1->addItem(strInt.str().c_str());
    m_uiForm.homePeriodBox2->addItem(strInt.str().c_str());
  }

  if (wsPeriods)
  {
    m_uiForm.homePeriodBox2->setEnabled(true);
    m_uiForm.homePeriodBoxMath->setEnabled(true);
  }
  else
  {
    m_uiForm.homePeriodBox2->setEnabled(false);
    m_uiForm.homePeriodBoxMath->setEnabled(false);
  }

  // Populate bin width info in Plot options
  double binWidth = matrix_workspace->dataX(0)[1]-matrix_workspace->dataX(0)[0];
  static const QChar MU_SYM(956);
  m_uiForm.optionLabelBinWidth->setText(QString("Data collected with histogram bins of ") + QString::number(binWidth) + QString(" %1s").arg(MU_SYM));

  // finally the preferred default by users are to by default
  // straight away plot the data
  if (m_uiForm.frontPlotButton->isEnabled() )
    runFrontPlotButton();
}


/**
 * Guess Alpha (slot). For now include all data from first good data(bin)
 */
void MuonAnalysis::guessAlphaClicked()
{
  if ( getPairNumberFromRow(m_pairTableRowInFocus) >= 0 )
  {
    QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
    QComboBox* qwB = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

    if (!qwF || !qwB)
      return;

    // group IDs
    QTableWidgetItem *idsF = m_uiForm.groupTable->item(m_groupToRow[qwF->currentIndex()],1);
    QTableWidgetItem *idsB = m_uiForm.groupTable->item(m_groupToRow[qwB->currentIndex()],1);

    if (!idsF || !idsB)
      return;

    QString inputWS = m_workspace_name.c_str() + QString("Grouped");
    if ( m_uiForm.homePeriodBox2->isEnabled() )
      inputWS += "_" + m_uiForm.homePeriodBox1->currentText();


    QString pyString;

    pyString += "alg=AlphaCalc(\"" + inputWS + "\",\"" 
        + idsF->text() + "\",\""
        + idsB->text() + "\",\"" 
        + firstGoodBin() + "\")\n"
        + "print alg.getPropertyValue('Alpha')";

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();
    pyOutput.truncate(5);

    QComboBox* qwAlpha = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,3));
    if (qwAlpha)
      m_uiForm.pairTable->item(m_pairTableRowInFocus,3)->setText(pyOutput);
    else
      m_uiForm.pairTable->setItem(m_pairTableRowInFocus,3, new QTableWidgetItem(pyOutput));
  }
}

/**
 * Return number of groups defined (not including pairs)
 *
 * @return number of groups
 */
int MuonAnalysis::numGroups()
{
  whichGroupToWhichRow(m_uiForm, m_groupToRow);
  return static_cast<int>(m_groupToRow.size());
}

/**
 * Return number of pairs
 *
 * @return number of pairs
 */
int MuonAnalysis::numPairs()
{
  whichPairToWhichRow(m_uiForm, m_pairToRow);
  return static_cast<int>(m_pairToRow.size());
}

/**
 * Update front "group / group-pair" combo-box based on what the currentIndex now is
 */
void MuonAnalysis::updateFront()
{
  // get current index
  int index = m_uiForm.frontGroupGroupPairComboBox->currentIndex();

  m_uiForm.frontPlotFuncs->clear();
  int numG = numGroups();
  if (numG)
  {
    if (index >= numG && numG >= 2)
    {
      // i.e. index points to a pair
      m_uiForm.frontPlotFuncs->addItems(m_pairPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(true);
      m_uiForm.frontAlphaNumber->setVisible(true);

      m_uiForm.frontAlphaNumber->setText(m_uiForm.pairTable->item(m_pairToRow[index-numG],3)->text());
    }
    else
    {
      // i.e. index points to a group
      m_uiForm.frontPlotFuncs->addItems(m_groupPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(false);
      m_uiForm.frontAlphaNumber->setVisible(false);
    }
  }
}


/**
 * Update front including first re-populate pair list combo box
 */
void MuonAnalysis::updateFrontAndCombo()
{
  // for now brute force clearing and adding new context
  // could go for softer approach and check if is necessary
  // to complete reset this combo box
  int currentI = m_uiForm.frontGroupGroupPairComboBox->currentIndex();
  if (currentI < 0)  // in case this combobox has not been set yet
    currentI = 0;
  m_uiForm.frontGroupGroupPairComboBox->clear();

  int numG = numGroups();
  int numP = numPairs();
  for (int i = 0; i < numG; i++)
    m_uiForm.frontGroupGroupPairComboBox->addItem(
      m_uiForm.groupTable->item(m_groupToRow[i],0)->text());
  for (int i = 0; i < numP; i++)
    m_uiForm.frontGroupGroupPairComboBox->addItem(
      m_uiForm.pairTable->item(m_pairToRow[i],0)->text());
  
  if ( currentI >= m_uiForm.frontGroupGroupPairComboBox->count() )
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(0);
  else 
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(currentI);

  updateFront();
}


/**
 * Return the group-number for the group in a row. Return -1 if 
 * invalid group in row
 *
 * @param row :: A row in the group table
 * @return Group number
 */
int MuonAnalysis::getGroupNumberFromRow(int row)
{
  whichGroupToWhichRow(m_uiForm, m_groupToRow);
  for (unsigned int i = 0; i < m_groupToRow.size(); i++)
  {
    if ( m_groupToRow[i] == row )
      return i;
  }
  return -1;
}

/**
 * Return the pair-number for the pair in a row. Return -1 if 
 * invalid pair in row
 *
 * @param row :: A row in the pair table
 * @return Pair number
 */
int MuonAnalysis::getPairNumberFromRow(int row)
{
  whichPairToWhichRow(m_uiForm, m_pairToRow);
  for (unsigned int i = 0; i < m_pairToRow.size(); i++)
  {
    if ( m_pairToRow[i] == row )
      return i;
  }
  return -1;
}


/**
 * Return the pair which is in focus and -1 if none
 */
int MuonAnalysis::pairInFocus()
{
  // plus some code here which double checks that pair
  // table in focus actually sensible

    return m_pairTableRowInFocus;

}


/**
 * Clear tables and front combo box
 */
void MuonAnalysis::clearTablesAndCombo()
{
  m_uiForm.groupTable->clearContents();
  m_uiForm.frontGroupGroupPairComboBox->clear();
  m_uiForm.frontPlotFuncs->clear();

  m_uiForm.pairTable->clearContents();
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }
}


/**
 * Create WS contained the data for a plot
 * Take the MuonAnalysisGrouped WS and reduce(crop) histograms according to Plot Options.
 * If period data then the resulting cropped WS is on for the period, or sum/difference of, selected 
 * by the user on the front panel
 */
void MuonAnalysis::createPlotWS(const std::string& groupName, const std::string& wsname)
{
  QString inputWS = m_workspace_name.c_str() + QString("Grouped");

  if ( m_uiForm.homePeriodBox2->isEnabled() && m_uiForm.homePeriodBox2->currentText()!="None" )
  {
    QString pyS;
    if ( m_uiForm.homePeriodBoxMath->currentText()=="+" )
    {
      pyS += "Plus(\"" + inputWS + "_" + m_uiForm.homePeriodBox1->currentText()
        + "\",\"" + inputWS + "_" + m_uiForm.homePeriodBox2->currentText() + "\",\""
        + wsname.c_str() + "\")";
    }
    else 
    {
      pyS += "Minus(\"" + inputWS + "_" + m_uiForm.homePeriodBox1->currentText()
        + "\",\"" + inputWS + "_" + m_uiForm.homePeriodBox2->currentText() + "\",\""
        + wsname.c_str() + "\")";
    }
    runPythonCode( pyS ).trimmed();
    inputWS = wsname.c_str();
  }
  else
  {
    if ( m_uiForm.homePeriodBox2->isEnabled() ) 
      inputWS += "_" + m_uiForm.homePeriodBox1->currentText();
  }


  QString cropStr = "CropWorkspace(\"";
  cropStr += inputWS;
  cropStr += "\",\"";
  cropStr += wsname.c_str();
  cropStr += QString("\",") + boost::lexical_cast<std::string>(plotFromTime()).c_str(); 
  if ( !m_uiForm.timeAxisFinishAtInput->text().isEmpty() )  
    cropStr += QString(",") + boost::lexical_cast<std::string>(plotToTime()).c_str();
  cropStr += ");";
  runPythonCode( cropStr ).trimmed();

  // rebin data if option set in Plot Options
  if ( m_uiForm.rebinComboBox->currentText() == "Fixed" )
  {
    // @Rob.Whitley Need to implement.
    // Record the bunch data so that a fit can be done against it
    m_previousBunchWsName = wsname;
    m_previousRebinSteps = m_uiForm.optionStepSizeText->text();

    QString reBunchStr = QString("Rebunch(\"") + wsname.c_str() + "\",\""
        + wsname.c_str() + QString("\",") + m_uiForm.optionStepSizeText->text() + ");";
    runPythonCode( reBunchStr ).trimmed(); 
  } 

  // Make group to display more organised in Mantidplot workspace list
  if ( !AnalysisDataService::Instance().doesExist(groupName) )
  {
    QString rubbish = "boevsMoreBoevs";
    QString groupStr = QString("CloneWorkspace('") + wsname.c_str() + "','"+rubbish+"')\n";
    groupStr += QString("GroupWorkspaces(InputWorkspaces='") + wsname.c_str() + "," + rubbish
      + "',OutputWorkspace='"+groupName.c_str()+"')\n";
    runPythonCode( groupStr ).trimmed();
    AnalysisDataService::Instance().remove(rubbish.toStdString());
  }
  else
  {
    QString groupStr = QString("GroupWorkspaces(InputWorkspaces='") + wsname.c_str() + "," + groupName.c_str()
      + "',OutputWorkspace='" + groupName.c_str() + "')\n";
    runPythonCode( groupStr ).trimmed();
  }
}


// @Rob.Whitley Need to implement
/**
* Check the bunch details then fit using the rebinned data but plot against 
* the data that is currently plotted, this may be the same.
*
* @params wsName :: The name of the workspace the user wants to fit against. 
*/
void MuonAnalysis::reBunch(const std::string & wsName)
{
  // When bunch is selected but no plot option rebin has been stated output message
  if ( m_uiForm.rebinComboBox->currentText() != "Fixed" )
  {
    QMessageBox::warning(this, "Mantid - Moun Analysis", "Warning - Bunch data was selected but the number of steps wasn't specified. "
         "\nThe option for this is located on the Plot Options tab. \n\n Default fit will be against current data."); 
    return;
  }

  // If the fitting has already been plotted with the bunched settings AND against the same workspace then return. (assume last fitting)
  if ((m_previousBunchWsName == wsName) && (m_previousRebinSteps == m_uiForm.optionStepSizeText->text())) 
    return;

  else if (m_previousBunchWsName == wsName)
  {
    //Put back to original, then bunch to specification (make raw currently creates a new plot @Rob.Whitley Need to implement)
    makeRaw(wsName);
    m_previousBunchWsName = wsName;
    m_previousRebinSteps = m_uiForm.optionStepSizeText->text();
    QString reBunchStr = QString("Rebunch(\"") + wsName.c_str() + "\",\"" + wsName.c_str() + QString("\",") + m_uiForm.optionStepSizeText->text() + ");"; 
    runPythonCode( reBunchStr ).trimmed(); 
  }
  else
  {
    m_previousBunchWsName = wsName;
    m_previousRebinSteps = m_uiForm.optionStepSizeText->text();
    QString reBunchStr = QString("Rebunch(\"") + wsName.c_str() + "\",\"" + wsName.c_str() + QString("\",") + m_uiForm.optionStepSizeText->text() + ");"; 
    runPythonCode( reBunchStr ).trimmed(); 
  }
}

/**
* Setup a temporary fit against the current data, then either:
* 1) remove the data from the plot, load up the original raw data, bring the fitting to the front
* 2) copy the fitting onto a new plot where the raw data has been loaded up again and then delete the plot that contains the data you began the function with
*
* @params wsName :: The name of the workspace the user wants to fit against. 
*/
void MuonAnalysis::makeRaw(const std::string & wsName)
{
  UNUSED_ARG(wsName)
  //Load back original without bunching
  //Maybe have a boolean sent if made raw, if this is true then after fit addRawData() will need to be called.
}


/**
 * Plot group
 */
void MuonAnalysis::plotGroup(const std::string& plotType)
{
  int groupNum = getGroupNumberFromRow(m_groupTableRowInFocus);
  if ( groupNum >= 0 )
  {
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(m_groupTableRowInFocus,0);
    QString groupName = itemName->text();

    // Decide on name for workspaceGroup
    Poco::File l_path( m_previousFilename.toStdString() );
    std::string workspaceGroupName = Poco::Path(l_path.path()).getFileName();
    std::size_t extPos = workspaceGroupName.find(".");
    if ( extPos!=std::string::npos)
      workspaceGroupName = workspaceGroupName.substr(0,extPos);

    // decide on name for workspace to be plotted
    QString cropWS;
    QString cropWSfirstPart = QString(workspaceGroupName.c_str()) + "; Group="
      + groupName + "";

    // check if this workspace already exist to avoid replotting an existing workspace
    int plotNum = 1;
    while (1==1)
    {
      cropWS = cropWSfirstPart + "; #" + boost::lexical_cast<std::string>(plotNum).c_str();
      if ( AnalysisDataService::Instance().doesExist(cropWS.toStdString()) ) 
        plotNum++;
      else
        break;
    }

    // create the plot workspace
    createPlotWS(workspaceGroupName,cropWS.toStdString());

    // curve plot label
    QString titleLabel = cropWS;

    // create first part of plotting Python string
    QString gNum = QString::number(groupNum);
    QString pyS;
    if ( m_uiForm.showErrorBars->isChecked() )
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + gNum + ",True)\n";
    else  
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + gNum + ")\n";
    // Add the objectName for the peakPickerTool to find
    pyS += "gs.setObjectName(\"" + titleLabel + "\")\n"
           "l = gs.activeLayer()\n"
           "l.setCurveTitle(0, \"" + titleLabel + "\")\n"
           "l.setTitle(\"" + m_title.c_str() + "\")\n";
      
    Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(cropWS.toStdString());
    MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
    if ( !m_uiForm.yAxisAutoscale->isChecked() )
    {
      const Mantid::MantidVec& dataY = matrix_workspace->readY(groupNum);
      double min = 0.0; double max = 0.0;

      if (m_uiForm.yAxisMinimumInput->text().isEmpty())
      {
        min = *min_element(dataY.begin(), dataY.end());
      }
      else
      {
        min = boost::lexical_cast<double>(m_uiForm.yAxisMinimumInput->text().toStdString());
      }

      if (m_uiForm.yAxisMaximumInput->text().isEmpty())
      {
        max = *max_element(dataY.begin(), dataY.end());
      }
      else
      {
        max = boost::lexical_cast<double>(m_uiForm.yAxisMaximumInput->text().toStdString());
      }

      pyS += "l.setAxisScale(Layer.Left," + QString::number(min) + "," + QString::number(max) + ")\n";
    }

    QString pyString;
    if (plotType.compare("Counts") == 0)
    {
      pyString = pyS;
    }
    else if (plotType.compare("Asymmetry") == 0)
    {
      matrix_workspace->setYUnitLabel("Asymmetry");
      // Normalise before removing exponential decay
      const std::vector<double>& x = matrix_workspace->readX(0);
      const std::vector<double>& y = matrix_workspace->readY(0);
      double targetTime = boost::lexical_cast<double>(firstGoodBin().toStdString());
      int indexTime = 0;
      for (int i = 0; i < static_cast<int>(x.size()); i++)
      {
        if (x[i] > targetTime)
        {
          indexTime = i;
          break;
        }

      }
      double normalizationFactor = 1.0;
      if (y[indexTime] < 0)
        normalizationFactor = -1.0/y[indexTime];
      else
        normalizationFactor = 1.0/y[indexTime];

      QString pyStrNormalise = "Scale('" + cropWS + "','" + cropWS + "','"
        + QString::number(normalizationFactor) + "')";

      runPythonCode( pyStrNormalise ).trimmed();

      pyString = "RemoveExpDecay(\"" + cropWS + "\",\"" 
        + cropWS + "\")\n" + pyS;
        //+ "l.setAxisTitle(Layer.Left, \"Asymmetry\")\n";
    }
    else if (plotType.compare("Logorithm") == 0)
    {
      matrix_workspace->setYUnitLabel("Logorithm");
      pyString += "Logarithm(\"" + cropWS + "\",\"" 
        + cropWS + "\")\n" + pyS;
    }
    else
    {
      g_log.error("Unknown group table plot function");
      return;
    }

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();
    
    m_currentDataName = titleLabel;
    m_uiForm.fitBrowser->manualAddWorkspace(m_currentDataName);
  }  
}

/**
 * Plot pair
 */
void MuonAnalysis::plotPair(const std::string& plotType)
{
  int pairNum = getPairNumberFromRow(m_pairTableRowInFocus);
  if ( pairNum >= 0 )
  {
    QTableWidgetItem *item = m_uiForm.pairTable->item(m_pairTableRowInFocus,3);
    QTableWidgetItem *itemName = m_uiForm.pairTable->item(m_pairTableRowInFocus,0);
    QString pairName = itemName->text();

    // Decide on name for workspaceGroup
    Poco::File l_path( m_previousFilename.toStdString() );
    std::string workspaceGroupName = Poco::Path(l_path.path()).getFileName();
    std::size_t extPos = workspaceGroupName.find(".");
    if ( extPos!=std::string::npos)
      workspaceGroupName = workspaceGroupName.substr(0,extPos);

    // decide on name for workspace to be plotted
    QString cropWS;
    QString cropWSfirstPart = QString(workspaceGroupName.c_str()) + "; Group="
      + pairName + "";

    // check if this workspace already exist to avoid replotting an existing workspace
    int plotNum = 1;
    while (1==1)
    {
      cropWS = cropWSfirstPart + "; #" + boost::lexical_cast<std::string>(plotNum).c_str();
      if ( AnalysisDataService::Instance().doesExist(cropWS.toStdString()) ) 
        plotNum++;
      else
        break;
    }

    // create the plot workspace
    createPlotWS(workspaceGroupName,cropWS.toStdString());

    // curve plot label
    QString titleLabel = cropWS;

    // create first part of plotting Python string
    QString gNum = QString::number(pairNum);
    QString pyS;
    if ( m_uiForm.showErrorBars->isChecked() )
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + "0" + ",True)\n";
    else
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + "0" + ")\n";
    //add the objectName for the peakPickerTool to find
    pyS +=  "gs.setObjectName(\"" + titleLabel + "\")\n"
            "l = gs.activeLayer()\n"
            "l.setCurveTitle(0, \"" + titleLabel + "\")\n"
            "l.setTitle(\"" + m_title.c_str() + "\")\n";   
     
    
    Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(cropWS.toStdString());
    MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
    if ( !m_uiForm.yAxisAutoscale->isChecked() )
    {
      const Mantid::MantidVec& dataY = matrix_workspace->readY(pairNum);
      double min = 0.0; double max = 0.0;

      if (m_uiForm.yAxisMinimumInput->text().isEmpty())
      {
        min = *min_element(dataY.begin(), dataY.end());
      }
      else
      {
        min = boost::lexical_cast<double>(m_uiForm.yAxisMinimumInput->text().toStdString());
      }

      if (m_uiForm.yAxisMaximumInput->text().isEmpty())
      {
        max = *max_element(dataY.begin(), dataY.end());
      }
      else
      {
        max = boost::lexical_cast<double>(m_uiForm.yAxisMaximumInput->text().toStdString());
      }

      pyS += "l.setAxisScale(Layer.Left," + QString::number(min) + "," + QString::number(max) + ")\n";
    }


    QString pyString;
    if (plotType.compare("Asymmetry") == 0)
    {
      matrix_workspace->setYUnitLabel("Asymmetry");
      QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
      QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

      QString pairName;
      QTableWidgetItem *itemName = m_uiForm.pairTable->item(m_pairTableRowInFocus,0);
      if (itemName)
        pairName = itemName->text();

      //QString outputWS_Name = m_workspace_name.c_str() + QString("_") + pairName + periodStr;

      pyString = "AsymmetryCalc(\"" + cropWS + "\",\"" 
        + cropWS + "\","
        + QString::number(qw1->currentIndex()) + "," 
        + QString::number(qw2->currentIndex()) + "," 
        + item->text() + ")\n" + pyS;
    }
    else
    {
      g_log.error("Unknown pair table plot function");
      return;
    }

    // run python script
    std::string bsdfasdf = pyString.toStdString();
    
    QString pyOutput = runPythonCode( pyString ).trimmed();

    // Change the plot style of the graph so that it matches what is selected on 
    // the plot options tab. Default is set to line (0).
    QString plotType("");
    plotType.setNum(m_uiForm.connectPlotType->currentIndex());

    changePlotType(plotType + ".1." + titleLabel);
    
    m_currentDataName = titleLabel;
    m_uiForm.fitBrowser->manualAddWorkspace(m_currentDataName);
  }  
}

/**
 * Is Grouping set.
 *
 * @return true if set
 */
bool MuonAnalysis::isGroupingSet()
{
  std::vector<int> dummy;
  whichGroupToWhichRow(m_uiForm, dummy);

  if (dummy.empty())
    return false;
  else
    return true;
}

/**
 * Apply grouping specified in xml file to workspace
 *
 * @param filename :: Name of grouping file
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS, 
   const std::string& filename)
{
  if ( AnalysisDataService::Instance().doesExist(inputWS) )
  {

    AnalysisDataService::Instance().remove(outputWS);

    QString pyString = 
      "from mantidsimple import *\n"
      "import sys\n"
      "try:\n"
      "  GroupDetectors('" + QString(inputWS.c_str()) + "','" + outputWS.c_str() + "','" + filename.c_str() + "')\n"
      "except SystemExit, message:\n"
      "  print str(message)";

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();

    // if output is none empty something has gone wrong
    if ( !pyOutput.toStdString().empty() )
    {
      noDataAvailable();
      QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't group data file according to group-table. Plotting disabled.");
      return false;
      //m_uiForm.frontWarningMessage->setText("Can't group data file according to group-table. Plotting disabled.");
    }
    else
    {
      nowDataAvailable();
      return true;
    }
  }
  return false;
}

/**
 * Apply whatever grouping is specified in GUI tables to workspace. 
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS)
{
  if ( isGroupingSet() && AnalysisDataService::Instance().doesExist(inputWS) )
  {

    std::string complaint = isGroupingAndDataConsistent();
    if ( complaint.empty() )
    {
      nowDataAvailable();
      m_uiForm.frontWarningMessage->setText("");
    }
    else
    {
      if (m_uiForm.frontPlotButton->isEnabled() )
        QMessageBox::warning(this, "MantidPlot - MuonAnalysis", complaint.c_str());
      noDataAvailable();
      //m_uiForm.frontWarningMessage->setText(complaint.c_str());
      return false;
    }

    saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
    return applyGroupingToWS(inputWS, outputWS, m_groupingTempFilename);
  }
  return false;
}

/**
 * Calculate number of detectors from string of type 1-3, 5, 10-15
 *
 * @param str :: String of type "1-3, 5, 10-15"
 * @return Number of detectors. Return 0 if not recognised
 */
int MuonAnalysis::numOfDetectors(const std::string& str) const
{
  return static_cast<int>(spectrumIDs(str).size());
}


/**
 * Return a vector of IDs for row number from string of type 1-3, 5, 10-15
 *
 * @param str :: String of type "1-3, 5, 10-15"
 * @return Vector of IDs
 */
std::vector<int> MuonAnalysis::spectrumIDs(const std::string& str) const
{
  //int retVal = 0;
  std::vector<int> retVal;


  if (str.empty())
    return retVal;

  typedef Poco::StringTokenizer tokenizer;
  tokenizer values(str, ",", tokenizer::TOK_TRIM);

  for (int i = 0; i < static_cast<int>(values.count()); i++)
  {
    std::size_t found= values[i].find("-");
    if (found!=std::string::npos)
    {
      tokenizer aPart(values[i], "-", tokenizer::TOK_TRIM);

      if ( aPart.count() != 2 )
      {
        retVal.clear();
        return retVal;
      }
      else
      {
        if ( !(isNumber(aPart[0]) && isNumber(aPart[1])) )
        {
          retVal.clear();
          return retVal;
        }
      }

      int leftInt;
      std::stringstream leftRead(aPart[0]);
      leftRead >> leftInt;
      int rightInt;
      std::stringstream rightRead(aPart[1]);
      rightRead >> rightInt;

      if (leftInt > rightInt)
      {
        retVal.clear();
        return retVal;
      }
      for (int step = leftInt; step <= rightInt; step++)
        retVal.push_back(step);
    }
    else
    {

      if (isNumber(values[i]))
        retVal.push_back(boost::lexical_cast<int>(values[i].c_str()));
      else
      {
        retVal.clear();
        return retVal;
      }
    }
  }
  return retVal;
}




/** Is input string a number?
 *
 *  @param s :: The input string
 *  @return True is input string is a number
 */
bool MuonAnalysis::isNumber(const std::string& s) const
{
  if( s.empty() )
  {
    return false;
  }

  const std::string allowed("0123456789");

  for (unsigned int i = 0; i < s.size(); i++)
  {
    if (allowed.find_first_of(s[i]) == std::string::npos)
    {
      return false;
    }
  }

  return true;
}

/**
 * When no data loaded set various buttons etc to inactive
 */
void MuonAnalysis::noDataAvailable()
{
  m_uiForm.frontPlotButton->setEnabled(false);
  m_uiForm.groupTablePlotButton->setEnabled(false);
  m_uiForm.pairTablePlotButton->setEnabled(false);

  m_uiForm.guessAlphaButton->setEnabled(false);
}

/**
 * When data loaded set various buttons etc to active
 */
void MuonAnalysis::nowDataAvailable()
{
  m_uiForm.frontPlotButton->setEnabled(true);
  m_uiForm.groupTablePlotButton->setEnabled(true);
  m_uiForm.pairTablePlotButton->setEnabled(true);

  m_uiForm.guessAlphaButton->setEnabled(true);
}


/**
 * Return true if data are loaded
 */
 bool MuonAnalysis::areDataLoaded()
 {
   return AnalysisDataService::Instance().doesExist(m_workspace_name);
 }

 /**
 * Set start up interface look and populate local attributes 
 * initiated from info set in QT designer
 */
 void MuonAnalysis::startUpLook()
 {
  // populate group plot functions
  for (int i = 0; i < m_uiForm.groupTablePlotChoice->count(); i++)
    m_groupPlotFunc.append(m_uiForm.groupTablePlotChoice->itemText(i));

  // pair plot functions
  for (int i = 0; i < m_uiForm.pairTablePlotChoice->count(); i++)
    m_pairPlotFunc.append(m_uiForm.pairTablePlotChoice->itemText(i));
  
  // Set initial front 
  m_uiForm.frontAlphaLabel->setVisible(false);
  m_uiForm.frontAlphaNumber->setVisible(false);
  m_uiForm.frontAlphaNumber->setEnabled(false);
  m_uiForm.homePeriodBox2->setEditable(false);
  m_uiForm.homePeriodBox2->setEnabled(false);

  // Set initial stuff in Option tab
  m_uiForm.optionBinStep->setVisible(false);
  m_uiForm.optionStepSizeText->setVisible(false);

  // set various properties of the group table
  m_uiForm.groupTable->setColumnWidth(0, 100);
  m_uiForm.groupTable->setColumnWidth(1, 200);
  for (int i = 0; i < m_uiForm.groupTable->rowCount(); i++)
  {
    
    QTableWidgetItem* item = m_uiForm.groupTable->item(i,2);
    if (!item)
    {
      QTableWidgetItem* it = new QTableWidgetItem("");
      it->setFlags(it->flags() & (~Qt::ItemIsEditable));
      m_uiForm.groupTable->setItem(i,2, it);
    }
    else
    {
      item->setFlags(item->flags() & (~Qt::ItemIsEditable));
    }
    item = m_uiForm.groupTable->item(i,0);
    if (!item)
    {
      QTableWidgetItem* it = new QTableWidgetItem("");
      m_uiForm.groupTable->setItem(i,0, it);
    }
  }


 }


 /**
 * set grouping in table from information from nexus raw file
 */
void MuonAnalysis::setGroupingFromNexus(const QString& nexusFile)
{
  // for now do try to set grouping from nexus file if it is already set
  if ( isGroupingSet() )
    return;

  std::string groupedWS = m_workspace_name+"Grouped";

  // Load nexus file with grouping
  QString pyString = "LoadMuonNexus(r'";
  pyString.append(nexusFile);
  pyString.append("','");
  pyString.append( groupedWS.c_str());
  pyString.append("', AutoGroup=\"1\");");
  runPythonCode( pyString ).trimmed();

  // get hold of a matrix-workspace. If period data assume each period has 
  // the same grouping
  Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(groupedWS);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(ws_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  if (wsPeriods)
  {
    Workspace_sptr ws_ptr1 = AnalysisDataService::Instance().retrieve(groupedWS + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr1);
  }
  else
  {
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
  }

  // check if there is any grouping in file
  bool thereIsGrouping = false;
  int numOfHist = static_cast<int>(matrix_workspace->getNumberHistograms()); //Qt has no size_t understanding
  for (int wsIndex = 0; wsIndex < numOfHist; wsIndex++)
  {
    IDetector_const_sptr det;
    try // for some bizarry reason when reading EMUautorun_A.tmp this 
        // underlying nexus file think there are more histogram than there is
        // hence the reason for this try/catch here
    {
      det = matrix_workspace->getDetector(wsIndex);
    }
    catch (...)
    {
      break;
    }

    if( boost::dynamic_pointer_cast<const DetectorGroup>(det) )
    {
      // prepare IDs string

      boost::shared_ptr<const DetectorGroup> detG = boost::dynamic_pointer_cast<const DetectorGroup>(det);
      std::vector<Mantid::detid_t> detIDs = detG->getDetectorIDs();
      if (detIDs.size() > 1)
      {
        thereIsGrouping = true;
        break;
      }
    }
  }

  // if no grouping in nexus then return
  if ( thereIsGrouping == false )
  {
    return;
  }

  // Add info about grouping from Nexus file to group table
  for (int wsIndex = 0; wsIndex < numOfHist; wsIndex++)
  {
    IDetector_const_sptr det = matrix_workspace->getDetector(wsIndex);

    if( boost::dynamic_pointer_cast<const DetectorGroup>(det) )
    {
      // prepare IDs string

      boost::shared_ptr<const DetectorGroup> detG = boost::dynamic_pointer_cast<const DetectorGroup>(det);
      std::vector<Mantid::detid_t> detIDs = detG->getDetectorIDs();
      std::stringstream idstr;
      int leftInt = detIDs[0];  // meaning left as in the left number of the range 8-18 for instance
      int numIDs = static_cast<int>(detIDs.size());
      idstr << detIDs[0];
      for (int i = 1; i < numIDs; i++)
      {
        if (detIDs[i] != detIDs[i-1]+1 )
        {
          if (detIDs[i-1] == leftInt)
          {
              idstr << ", " << detIDs[i];
              leftInt = detIDs[i];
          }
          else
            {
              idstr << "-" << detIDs[i-1] << ", " << detIDs[i];
              leftInt = detIDs[i];
            }
          }
        else if ( i == numIDs-1 )
        {
          idstr << "-" << detIDs[i];
        }
      }

      // prepare group name string

      std::stringstream gName;
      gName << wsIndex;

      // create table row
      QTableWidgetItem* it = m_uiForm.groupTable->item(wsIndex, 0);
      if (it)
        it->setText(gName.str().c_str());
      else
      {
        m_uiForm.groupTable->setItem(wsIndex, 0, new QTableWidgetItem(gName.str().c_str()));
      }

      it = m_uiForm.groupTable->item(wsIndex, 1);
      if (it)
        it->setText(idstr.str().c_str());
      else
        m_uiForm.groupTable->setItem(wsIndex, 1, new QTableWidgetItem(idstr.str().c_str()));
    }
  }  // end loop over wsIndex


  // check if exactly two groups added in which case assume these are forward/backward groups
  // and automatically then create a pair from which, where the first group is assumed to be
  // the forward group

  updatePairTable();
  if ( numGroups() == 2 && numPairs() <= 0 )
  {
      QTableWidgetItem* it = m_uiForm.pairTable->item(0, 0);
      if (it)
        it->setText("pair");
      else
      {
        m_uiForm.pairTable->setItem(0, 0, new QTableWidgetItem("long"));
      }    
      it = m_uiForm.pairTable->item(0, 3);
      if (it)
        it->setText("1.0");
      else
      {
        m_uiForm.pairTable->setItem(0, 3, new QTableWidgetItem("1.0"));
      } 
      updatePairTable();
      updateFrontAndCombo();
      m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(2);
      runFrontGroupGroupPairComboBox(2);
  }
  
  updatePairTable();
  updateFrontAndCombo();
}


/**
 * If nothing else work set dummy grouping and display comment to user
 */
void MuonAnalysis::setDummyGrouping(const int numDetectors)
{
  // if no grouping in nexus then set dummy grouping and display warning to user

    std::stringstream idstr;
    idstr << "1-" << numDetectors;
    m_uiForm.groupTable->setItem(0, 0, new QTableWidgetItem("NoGroupingDetected"));
    m_uiForm.groupTable->setItem(0, 1, new QTableWidgetItem(idstr.str().c_str()));

    updateFrontAndCombo();

    QMessageBox::warning(this, "MantidPlot - MuonAnalysis", QString("No grouping detected in Nexus file.\n")
      + "and no default grouping file specified in IDF\n"
      + "therefore dummy grouping created.");  
}


/**
 * Try to load default grouping file specified in IDF
 */
void MuonAnalysis::setGroupingFromIDF(const std::string& mainFieldDirection, MatrixWorkspace_sptr matrix_workspace)
{
  Instrument_const_sptr inst = matrix_workspace->getInstrument();

  QString instname = m_uiForm.instrSelector->currentText().toUpper();

  QString groupParameter = "Default grouping file";
  // for now hard coded in the special case of MUSR
  if (instname == "MUSR")
  {
    if ( mainFieldDirection == "Transverse" )
      groupParameter += " - Transverse";
    else
      groupParameter += " - Longitudinal";
  }


  std::vector<std::string> groupFile = inst->getStringParameter(groupParameter.toStdString());  

  // get search directory for XML instrument definition files (IDFs)
  std::string directoryName = ConfigService::Instance().getInstrumentDirectory();

  if ( groupFile.size() == 1 )
  {
    try 
    {
      loadGroupingXMLtoTable(m_uiForm, directoryName+groupFile[0]);
    }
    catch (...)
    {
      QMessageBox::warning(this, "MantidPlot - MuonAnalysis", QString("Can't load default grouping file in IDF.\n")
        + "with name: " + groupFile[0].c_str());  
    }
  }
}




 /**
 * Time zero returend in ms
 */
QString MuonAnalysis::timeZero()
{
  return m_uiForm.timeZeroFront->text();
}

 /**
 * first good bin returend in ms
 * returned as the absolute value of first-good-bin minus time zero
 */
QString MuonAnalysis::firstGoodBin()
{
  return m_uiForm.firstGoodBinFront->text();
}

 /**
 * According to Plot Options what time should we plot from in ms
 * @return time to plot from in ms
 */
double MuonAnalysis::plotFromTime()
{
  double retVal;
  try 
  {
    retVal = boost::lexical_cast<double>(m_uiForm.timeAxisStartAtInput->text().toStdString());
  }
  catch (...)
  {
    retVal = 0.0;
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Start at (ms)' input box. Plot from time zero.");
  }

  return retVal;
}


 /**
 * According to Plot Options what time should we plot to in ms
 * @return time to plot to in ms
 */
double MuonAnalysis::plotToTime()
{
  double retVal;
  try 
  {
    retVal = boost::lexical_cast<double>(m_uiForm.timeAxisFinishAtInput->text().toStdString());
  }
  catch (...)
  {
    retVal = 1.0;
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Finish at (ms)' input box. Plot to time=1.0.");
  }

  return retVal;
}


/**
* Check if grouping in table is consistent with data file
*
* @return empty string if OK otherwise a complaint
*/
std::string MuonAnalysis::isGroupingAndDataConsistent()
{
  std::string complaint = "Grouping inconsistent with data file. Plotting disabled.\n";

  // should probably farm the getting of matrix workspace out into separate method or store
  // as attribute assigned in inputFileChanged
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  if (wsPeriods)
  {
    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
  }
  else
  {
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  }

  int nDet = static_cast<int>(matrix_workspace->getNumberHistograms());

  complaint += "Number of spectra in data = " + boost::lexical_cast<std::string>(nDet) + ". ";

  int numG = numGroups();
  bool returnComplaint = false;
  for (int iG = 0; iG < numG; iG++)
  {
    typedef Poco::StringTokenizer tokenizer;
    tokenizer values(m_uiForm.groupTable->item(m_groupToRow[iG],1)->text().toStdString(), ",", tokenizer::TOK_TRIM);


    for (int i = 0; i < static_cast<int>(values.count()); i++)
    {
      std::size_t found= values[i].find("-");
      if (found!=std::string::npos)
      {
        tokenizer aPart(values[i], "-", tokenizer::TOK_TRIM);

        int rightInt;
        std::stringstream rightRead(aPart[1]);
        rightRead >> rightInt;

        if ( rightInt > nDet )
        {
          complaint += " Group-table row " + boost::lexical_cast<std::string>(m_groupToRow[iG]+1) + " refers to spectrum "
            + boost::lexical_cast<std::string>(rightInt) + ".";
          returnComplaint = true;
          break;
        }
      }
      else
      {
        if ( boost::lexical_cast<int>(values[i].c_str()) > nDet )
        {
          complaint += " Group-table row " + boost::lexical_cast<std::string>(m_groupToRow[iG]+1) + " refers to spectrum "
            + values[i] + ".";
          returnComplaint = true;
          break;
        }
      }
    }
  }

  if ( returnComplaint )
    return complaint;
  else
    return std::string("");
}


/**
* Check if dublicate ID between different rows
*/
void MuonAnalysis::checkIf_ID_dublicatesInTable(const int row)
{
  QTableWidgetItem *item = m_uiForm.groupTable->item(row,1);

  // row of IDs to compare against
  std::vector<int> idsNew = spectrumIDs(item->text().toStdString());

  int numG = numGroups();
  int rowInFocus = getGroupNumberFromRow(row);
  for (int iG = 0; iG < numG; iG++)
  {
    if (iG != rowInFocus)
    {
      std::vector<int> ids = spectrumIDs(m_uiForm.groupTable->item(m_groupToRow[iG],1)->text().toStdString());

      for (unsigned int i = 0; i < ids.size(); i++)
        for (unsigned int j = 0; j < idsNew.size(); j++)
        {
          if ( ids[i] == idsNew[j] )
          {
            item->setText(QString("Dublicate ID: " + item->text()));
            return;
          }
        }

    }
  }
}


/**
 * Load auto saved values
 */
void MuonAnalysis::loadAutoSavedValues(const QString& group)
{
  QSettings prevInstrumentValues;
  prevInstrumentValues.beginGroup(group + "instrument");
  QString instrumentName = prevInstrumentValues.value("name", "MUSR").toString();
  m_uiForm.instrSelector->setCurrentIndex(m_uiForm.instrSelector->findText(instrumentName));

  // load Plot Style options
  QSettings prevPlotStyle;
  prevPlotStyle.beginGroup(group + "plotStyleOptions"); 
  int timeComboBoxIndex = prevPlotStyle.value("timeComboBoxIndex", 0).toInt();
  m_uiForm.timeComboBox->setCurrentIndex(timeComboBoxIndex);
  m_optionTab->runTimeComboBox(timeComboBoxIndex);

  double timeAxisStart = prevPlotStyle.value("timeAxisStart", 0.3).toDouble();
  double timeAxisFinish = prevPlotStyle.value("timeAxisFinish", 16.0).toDouble();

  m_uiForm.timeAxisStartAtInput->setText(QString::number(timeAxisStart));
  m_uiForm.timeAxisFinishAtInput->setText(QString::number(timeAxisFinish));

  bool axisAutoScaleOnOff = prevPlotStyle.value("axisAutoScaleOnOff", 1).toBool();
  m_uiForm.yAxisAutoscale->setChecked(axisAutoScaleOnOff);
  m_optionTab->runyAxisAutoscale(axisAutoScaleOnOff);

  bool showErrorBars = prevPlotStyle.value("showErrorBars", 1).toBool();
  m_uiForm.showErrorBars->setChecked(showErrorBars);

  QStringList kusse = prevPlotStyle.childKeys();
  if ( kusse.contains("yAxisStart") )
  {
    double yAxisStart = prevPlotStyle.value("yAxisStart").toDouble();
    m_uiForm.yAxisMinimumInput->setText(QString::number(yAxisStart));
  }
  if ( kusse.contains("yAxisFinish") )
  {
    double yAxisFinish = prevPlotStyle.value("yAxisFinish").toDouble();
    m_uiForm.yAxisMaximumInput->setText(QString::number(yAxisFinish));
  }

  // Load Plot Binning Options
  QSettings prevPlotBinning;
  prevPlotBinning.beginGroup(group + "BinningOptions"); 
  int constStepSize = prevPlotBinning.value("constStepSize", 1).toInt();
  m_uiForm.optionStepSizeText->setText(QString::number(constStepSize));

  int rebinComboBoxIndex = prevPlotBinning.value("rebinComboBoxIndex", 0).toInt();
  m_uiForm.rebinComboBox->setCurrentIndex(rebinComboBoxIndex);
  m_optionTab->runRebinComboBox(rebinComboBoxIndex);
}


/**
*   Loads up the options for the fit browser so that it works in a muon analysis tab
*/
void MuonAnalysis::loadFittings()
{
  // Title of the fitting dock widget that now lies within the fittings tab. Should be made 
  // dynamic so that the Chi-sq can be displayed alongside like original fittings widget
  m_uiForm.fitBrowser->setWindowTitle("Fit Function");
  // Make sure that the window can't be moved or closed within the tab. 
  m_uiForm.fitBrowser->setFeatures(QDockWidget::NoDockWidgetFeatures);
}


/**
*   Check to see if the appending option is true when the previous button has been pressed and acts accordingly 
*/
void MuonAnalysis::checkAppendingPreviousRun()
{  
  if ( m_uiForm.mwRunFiles->getText().isEmpty() )
  {
    return;
  }
  
  if ( !m_uiForm.mwRunFiles->isValid() )
  {
    return;
  }
  
  if (m_uiForm.appendRun->isChecked())
  {
    setAppendingRun(-1);
  }
  else
  {
    //Subtact one from the current run and load
    changeRun(-1);
  }
}


/**
*   Check to see if the appending option is true when the next button has been pressed and acts accordingly 
*/
void MuonAnalysis::checkAppendingNextRun()
{
  if (m_uiForm.mwRunFiles->getText().isEmpty() )
    return;
  //if (m_uiForm.mwRunFiles->isValid() )
  //  return;

  if (m_uiForm.appendRun->isChecked())
  {
    setAppendingRun(1);
  }
  else
  {
    //Add one to current run and laod
    changeRun(1);
  }
}


/**
*   This sets up an appending lot of files so that when the user hits enter
*   all files within the range will open. 
*
*   @params setAppendingRun :: The number to increase the run by, this can be
*   -1 if previous has been selected.
*/
void MuonAnalysis::setAppendingRun(int inc)
{
  //Use this for appending the next run
  QString currentFile = m_uiForm.mwRunFiles->getText(); // m_uiForm.mwRunFiles->getFirstFilename();

  int appendSeparator(-1);
  int lowSize(-1);
  int lowLimit(-1);
  int maxLimit(-1);
  int maxSize(-1);
  QString fileExtension("");
  
  
  //Find where the range starts, if can't find then what is returned?????????
  appendSeparator = currentFile.find("-");

  //Get the file extension and then remove it from the current file
  int temp(currentFile.size()-currentFile.find("."));
  fileExtension = currentFile.right(temp);
  currentFile.chop(temp);

  //if there is a max limit indicated by the "-" symbol then...
  if(appendSeparator != -1)
  {
    QString maxString("");
    //Get the max value and then chop this off
    maxString = currentFile.right(currentFile.size() - appendSeparator - 1);
    maxSize = maxString.size();
    //include chopping off the "-" symbol
    currentFile.chop(maxSize + 1);

    if (inc > 0) //incrementing the range
    {  
    //Increment the max limit and then reconstruct the currentFile 
    maxLimit = maxString.toInt();
    ++maxLimit;
    maxString.setNum(maxLimit);
    getFullCode(maxSize, maxString);
    currentFile = currentFile + "-" + maxString + fileExtension;
    }
    else //if you are decrementing the range
    {
    //Seperate the file out further into lowLimit and the currentFile now becomes the first part which usually contains the instrument. i.e. MUSR000431 becomes (MUSR, 6, 431)
    separateMuonFile(currentFile, lowSize, lowLimit);
    lowLimit += inc;  
    QString lowString("");
    lowString.setNum(lowLimit);
    getFullCode(lowSize, lowString);
    getFullCode(maxSize, maxString);
    currentFile = currentFile + lowString + "-" + maxString + fileExtension;
    }
  }
  else  //need to display new maxLimit
  {
    QString lowString("");
    QString maxString("");
    separateMuonFile(currentFile, lowSize, lowLimit);
    if (inc > 0)  //incrementing the NEW range
    {
      maxSize = lowSize;
      maxLimit = lowLimit + inc;
      maxString.setNum(maxLimit);
      lowString.setNum(lowLimit);
    }
    else //decrementing the NEW range
    {
      maxSize = lowSize;
      maxString = lowString.setNum(lowLimit);
      lowLimit += inc;
      lowString.setNum(lowLimit);
    }
    getFullCode(lowSize, lowString);
    getFullCode(maxSize, maxString);
    currentFile = currentFile + lowString + "-" + maxString + fileExtension;
  } 

  m_previousFilename = currentFile;
  m_uiForm.mwRunFiles->setText(m_previousFilename);

}

/**
*   Opens up the next file if clicked next or previous on the muon analysis
*
*   @params amountToChange :: if clicked next then you need to open the next
*   file so 1 is passed, -1 is passed if previous was clicked by the user.
*/
void MuonAnalysis::changeRun(int amountToChange)
{
  int fileStart(-1);
  int firstFileNumber(-1);
  int lastFileNumber(-1);

  QString currentFile = m_uiForm.mwRunFiles->getFirstFilename();

  //Find where the file begins
  for (int i = 0; i<currentFile.size(); i++)
  {
    if(currentFile[i] == '/')  //.isDigit())
    {
      fileStart = i+1;
    }
  }

  for (int i = fileStart; i<currentFile.size(); i++)
  {
    if(currentFile[i].isDigit())  //.isDigit())
    {
      firstFileNumber = i;
      break;
    }
  }

  //Find where the run number ends
  for (int i = firstFileNumber; i<currentFile.size(); i++)
  {
    if(currentFile[i].isDigit())
    {
      lastFileNumber = i;
    }
  }

  //Get the file extension, -1 because array starts at 0
  int fileExtensionSize(currentFile.size() - lastFileNumber - 1);
  QString fileExtension(currentFile.right(fileExtensionSize));

  currentFile.chop(fileExtensionSize);
  //Get the run number, +1 because we want the firstFileNumberIncluded and -1 again because array starts at 0
  int runNumberSize(currentFile.size() - firstFileNumber + 1 - 1);
  int runNumber = currentFile.right(runNumberSize).toInt();

  currentFile.chop(runNumberSize);
  
  runNumber = runNumber + amountToChange;
  QString previousRunNumber("");
  previousRunNumber.setNum(runNumber);

  //Put preceeding 0's back into the string
  while (runNumberSize > previousRunNumber.size())
  {
    previousRunNumber = "0" + previousRunNumber;
  }

  m_previousFilename = (currentFile + previousRunNumber + fileExtension);
  m_uiForm.mwRunFiles->setUserInput(m_previousFilename);

  // in case file is selected from browser button check that it actually exist
  Poco::File l_path( m_previousFilename.toStdString() );
  if ( !l_path.exists() )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Specified data file does not exist.");
    return;
  }

  // save selected browse file directory to be reused next time interface is started up
  m_uiForm.mwRunFiles->saveSettings(m_settingsGroup + "mwRunFilesBrowse");

  inputFileChanged(m_previousFilename);
}


/**
*   Seperates the a given file into instrument, code and size of the code. 
*   i.e MUSR0002419 becomes MUSR, 7, 2419
*
*   @params currentFile :: This is the file to convert in QString format and once
*   finished will be returned containing the instrument used i.e MUSR or ELF.
*   @params runNumberSize :: Size of the code
*   @params runNumber :: contains the code as an integer, preceeding 0's are lost.
*/
void MuonAnalysis::separateMuonFile(QString &currentFile, int &runNumberSize, int &runNumber)
{
   //QString currentFile = m_uiForm.mwRunFiles->getFirstFilename();
  int firstFileNumber(-1);
  int lastFileNumber(-1);

  //Find where the run number begins
  for (int i = 0; i<currentFile.size(); i++)
  {
    if(currentFile[i].isDigit())
    {
      firstFileNumber = i;
      break;
    }
  }

  //Find where the run number ends
  for (int i = firstFileNumber; i<currentFile.size(); i++)
  {
    if(currentFile[i].isDigit())
    {
      lastFileNumber = i;
    }
  }

  //Get the run number, +1 because we want the firstFileNumberIncluded and -1 again because array starts at 0
  runNumberSize = currentFile.size() - firstFileNumber + 1 - 1;
  runNumber = currentFile.right(runNumberSize).toInt();

  currentFile.chop(runNumberSize);
}  


/**
*   Adds the 0's back onto the file code which were lost when converting 
*   it to an integer.
*   
*   @params size :: The size of the original file code before conversion
*   @params limitedCode :: This is the code after it was incremented or 
*   decremented and then converted back into a QString 
*/
void MuonAnalysis::getFullCode(int size, QString & limitedCode)
{
  while (size > limitedCode.size())
  {
    limitedCode = "0" + limitedCode;
  }
}


/**
* Everytime the tab is changed this is called to decide whether the peakpicker 
* tool needs to be associated with a plot or deleted from a plot
*
* @params tabNumber :: The index value of the current tab (3 = data analysis)
*/
void MuonAnalysis::changeTab(int tabNumber)
{
  m_tabNumber = tabNumber;
  m_uiForm.fitBrowser->setStartX(m_uiForm.timeAxisStartAtInput->text().toDouble());
  m_uiForm.fitBrowser->setEndX(m_uiForm.timeAxisFinishAtInput->text().toDouble());

  // If data analysis tab is chosen by user, assign peak picker tool to the current data if not done so already.
  if (tabNumber == 3)
  {
    // Update the peak picker tool with the current workspace.
    m_uiForm.fitBrowser->updatePPTool(m_currentDataName);
  } 
  else
  {
    // delete the peak picker tool because it is no longer needed.
    emit fittingRequested(m_uiForm.fitBrowser, "");
  }
}


/**
*   Emits a signal containing the fitBrowser and the name of the 
*   workspace we want to attach a peak picker tool to 
*
*   @params workspaceName :: The QString name of the workspace the user wishes 
*   to attach a plot picker tool to.
*/
void MuonAnalysis::assignPeakPickerTool(const QString & workspaceName)
{ 
  if (m_tabNumber == 3)
  {
    emit fittingRequested(m_uiForm.fitBrowser, workspaceName);
  }
}


/**
* Set up the string that will contain all the data needed for making a plot.
* [fitType, curveNum, wsName, color]
*
* @params wsName :: The workspace name of the plot to be created. 
*/
void MuonAnalysis::changeFitPlotType(const QString & wsName)
{
  // First part indicates 
  QString fitType("");
  fitType.setNum(m_uiForm.connectFitType->currentIndex());
  changePlotType(fitType + ".3." + wsName + "." + "Lime");
}

}//namespace MantidQT
}//namespace CustomInterfaces