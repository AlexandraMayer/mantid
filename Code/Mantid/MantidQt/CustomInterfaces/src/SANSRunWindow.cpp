//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/SANSRunWindow.h"
#include "MantidQtCustomInterfaces/SANSUtilityDialogs.h"
#include "MantidQtCustomInterfaces/SANSAddFiles.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/PythonRunner.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"

#include <QLineEdit>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>

#include "Poco/StringTokenizer.h"
#include "boost/lexical_cast.hpp"

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(SANSRunWindow);


using namespace MantidQt::MantidWidgets;
using namespace MantidQt::API;
using namespace MantidQt::CustomInterfaces;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::IInstrument_sptr;
using Mantid::Geometry::IInstrument;

// Initialize the logger
Logger& SANSRunWindow::g_log = Logger::get("SANSRunWindow");

//----------------------------------------------
// Public member functions
//----------------------------------------------
///Constructor
SANSRunWindow::SANSRunWindow(QWidget *parent) :
  UserSubWindow(parent), m_addFilesTab(NULL), m_saveWorkspaces(NULL),
  m_ins_defdir(""), m_last_dir(""),
  m_cfg_loaded(true), m_userFname(false), m_sample_no(), m_run_no_boxes(),
  m_period_lbls(), m_warnings_issued(false), m_force_reload(false),
  m_log_warnings(false), m_newInDir(*this, &SANSRunWindow::handleInputDirChange),
  m_delete_observer(*this, &SANSRunWindow::handleMantidDeleteWorkspace),
  m_s2d_detlabels(), m_loq_detlabels(), m_allowed_batchtags(), m_lastreducetype(-1),
  m_have_reducemodule(false), m_dirty_batch_grid(false), m_tmp_batchfile(""),m_diagnosticsTab(NULL)
{
  ConfigService::Instance().addObserver(m_newInDir);
}

///Destructor
SANSRunWindow::~SANSRunWindow()
{
  try
  {
    ConfigService::Instance().removeObserver(m_newInDir);
    if( isInitialized() )
    {
      // Seems to crash on destruction of if I don't do this 
      AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
      saveSettings();
      delete m_addFilesTab;
    }
    delete m_diagnosticsTab;
  }
  catch(...)
  {
    //we've cleaned up the best we can, move on
  }
}

//--------------------------------------------
// Private member functions
//--------------------------------------------
/**
 * Set up the dialog layout
 */
void SANSRunWindow::initLayout()
{
  g_log.debug("Initializing interface layout");
  m_uiForm.setupUi(this);

  m_reducemapper = new QSignalMapper(this);
  m_mode_mapper = new QSignalMapper(this);

  //Set column stretch on the mask table
  m_uiForm.mask_table->horizontalHeader()->setStretchLastSection(true);

  setupSaveBox();
  
  connectButtonSignals();

  // Disable most things so that load is the only thing that can be done
  m_uiForm.oneDBtn->setEnabled(false);
  m_uiForm.twoDBtn->setEnabled(false);
  m_uiForm.saveDefault_btn->setEnabled(false);
  for( int i = 1; i < 4; ++i)
  {
    m_uiForm.tabWidget->setTabEnabled(i, false);
  }

  //Mode switches
  connect(m_uiForm.single_mode_btn, SIGNAL(clicked()), m_mode_mapper, SLOT(map()));
  m_mode_mapper->setMapping(m_uiForm.single_mode_btn, SANSRunWindow::SingleMode);
  connect(m_uiForm.batch_mode_btn, SIGNAL(clicked()), m_mode_mapper, SLOT(map()));
  m_mode_mapper->setMapping(m_uiForm.batch_mode_btn, SANSRunWindow::BatchMode);
  connect(m_mode_mapper, SIGNAL(mapped(int)), this, SLOT(switchMode(int)));

  //Set a custom context menu for the batch table
  m_uiForm.batch_table->setContextMenuPolicy(Qt::ActionsContextMenu);
  m_batch_paste = new QAction(tr("&Paste"),m_uiForm.batch_table);
  m_batch_paste->setShortcut(tr("Ctrl+P"));
  connect(m_batch_paste, SIGNAL(activated()), this, SLOT(pasteToBatchTable()));
  m_uiForm.batch_table->addAction(m_batch_paste);

  m_batch_clear = new QAction(tr("&Clear"),m_uiForm.batch_table);    
  m_uiForm.batch_table->addAction(m_batch_clear);
  connect(m_batch_clear, SIGNAL(activated()), this, SLOT(clearBatchTable()));

  //Logging
  connect(this, SIGNAL(logMessageReceived(const QString&)), this, SLOT(updateLogWindow(const QString&)));
  connect(m_uiForm.logger_clear, SIGNAL(clicked()), this, SLOT(clearLogger()));
  m_uiForm.logging_field->ensureCursorVisible();

  //Create the widget hash maps
  initWidgetMaps();

  connectChangeSignals();

  initAnalysDetTab();

  if( ! m_addFilesTab )
  {//sets up the AddFiles tab which must be deleted in the destructor
    m_addFilesTab = new SANSAddFiles(this, &m_uiForm);
  } 
  //diagnostics tab
  if(!m_diagnosticsTab)
  {
    m_diagnosticsTab = new SANSDiagnostics(this,&m_uiForm);
  }
  connect(this,SIGNAL(userfileLoaded()),m_diagnosticsTab,SLOT(enableMaskFileControls()));
  //Listen for Workspace delete signals
  AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);

  readSettings();
}

void SANSRunWindow::initAnalysDetTab()
{
  //Add shortened forms of step types to step boxes
  m_uiForm.q_dq_opt->setItemData(0, "LIN");
  m_uiForm.q_dq_opt->setItemData(1, "LOG");
  m_uiForm.qy_dqy_opt->setItemData(0, "LIN");
//remove the following two lines once the beamfinder is in the new framework
  m_uiForm.wav_dw_opt->setItemData(0, "LIN");
  m_uiForm.wav_dw_opt->setItemData(1, "LOG");

  //the file widget always has a *.* filter, passing an empty list means we get only that
  m_uiForm.floodFile->setAlgorithmProperty("CorrectToFile|Filename");
  m_uiForm.floodFile->isOptional(true);

  //the unicode code for the angstrom symbol is 197, doing the below keeps this file ASCII compatible
  static const QChar ANGSROM_SYM(197);
  m_uiForm.wavlength_lb->setText(QString("Wavelength (%1)").arg(ANGSROM_SYM));
  m_uiForm.qx_lb->setText(QString("Qx (%1^-1)").arg(ANGSROM_SYM));
  m_uiForm.qxy_lb->setText(QString("Qxy (%1^-1)").arg(ANGSROM_SYM));
  m_uiForm.transFit_lb->setText(QString("Trans Fit (%1)").arg(ANGSROM_SYM));

  
 
  //Listen for Workspace delete signals
  AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);

  makeValidator(m_uiForm.wavRanVal_lb, m_uiForm.wavRanges, m_uiForm.tab_2,
             "A comma separated list of numbers is required here");
  connect(m_uiForm.wavRanges, SIGNAL(editingFinished()),
                                    this, SLOT(checkList()));
}
/** Formats a Qlabel to be a validator and adds it to the list
*  @param newValid :: a QLabel to use as a validator
*  @param control :: the control whose entry the validator is validates
*  @param tab :: the tab that contains this widgets
*  @param errorMsg :: the tooltip message that the validator should have
*/
void SANSRunWindow::makeValidator(QLabel * const newValid, QWidget * control, QWidget * tab, const QString & errorMsg)
{
  QPalette pal = newValid->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  newValid->setPalette(pal);
  newValid->setToolTip(errorMsg);

  // regester the validator       and say      where it's control is
  m_validators[newValid] = std::pair<QWidget *, QWidget *>(control, tab);
}

/**
 * Run local Python initialization code
 */
void SANSRunWindow::initLocalPython()
{
  // Import the SANS module and set the correct instrument
  QString result = runPythonCode("try:\n\timport isis_reducer\nexcept (ImportError,SyntaxError), details:\tprint 'Error importing isis_reducer: ' + str(details)\n");
  if( result.trimmed().isEmpty() )
  {
    m_have_reducemodule = true;
  }
  else
  {
    showInformationBox(result);
    m_have_reducemodule = false;
    setProcessingState(true, -1);    
  }
  runPythonCode("import ISISCommandInterface as i\nimport copy");
  runPythonCode("import isis_instrument\nimport isis_reduction_steps");
  handleInstrumentChange();
}
/** Initialise some of the data and signal connections in the save box
*/
void SANSRunWindow::setupSaveBox()
{
  connect(m_uiForm.saveDefault_btn, SIGNAL(clicked()), this, SLOT(handleDefSaveClick()));
  connect(m_uiForm.saveSel_btn, SIGNAL(clicked()),
    this, SLOT(saveWorkspacesDialog()));
  connect(m_uiForm.saveFilename_btn, SIGNAL(clicked()),
    this, SLOT(saveFileBrowse()));
  connect(m_uiForm.outfile_edit, SIGNAL(textEdited(const QString &)),
    this, SLOT(setUserFname()));

  //link the save option tick boxes to their save algorithm
  m_savFormats.insert(m_uiForm.saveNex_check, "SaveNexus");
  m_savFormats.insert(m_uiForm.saveCan_check, "SaveCanSAS1D");
  m_savFormats.insert(m_uiForm.saveRKH_check, "SaveRKH");
  m_savFormats.insert(m_uiForm.saveCSV_check, "SaveCSV");

  for(SavFormatsConstIt i=m_savFormats.begin(); i != m_savFormats.end(); ++i)
  {
    connect(i.key(), SIGNAL(stateChanged(int)),
      this, SLOT(enableOrDisableDefaultSave()));
  }
}
/** Raises a saveWorkspaces dialog which allows people to save any workspace or
*  workspaces the user chooses
*/
void SANSRunWindow::saveWorkspacesDialog()
{
  //this dialog must have delete on close selected to aviod a memory leak
  m_saveWorkspaces =
    new SaveWorkspaces(this, m_uiForm.outfile_edit->text(), m_savFormats);
  //this dialog sometimes needs to run Python, pass this to Mantidplot via our runAsPythonScript() signal
  connect(m_saveWorkspaces, SIGNAL(runAsPythonScript(const QString&)),
    this, SIGNAL(runAsPythonScript(const QString&)));
  //we need know if we have a pointer to a valid window or not
  connect(m_saveWorkspaces, SIGNAL(closing()),
    this, SLOT(saveWorkspacesClosed()));
  m_uiForm.saveSel_btn->setEnabled(false);
  m_saveWorkspaces->show();
}
/**When the save workspaces dialog box is closes its pointer, m_saveWorkspaces,
* is set to NULL and the raise dialog button is re-enabled
*/
void SANSRunWindow::saveWorkspacesClosed()
{
  m_uiForm.saveSel_btn->setEnabled(true);
  m_saveWorkspaces = NULL;
}
/** Connection the buttons to their signals
*/
void SANSRunWindow::connectButtonSignals()
{
  connect(m_uiForm.data_dirBtn, SIGNAL(clicked()), this, SLOT(selectDataDir()));
  connect(m_uiForm.userfileBtn, SIGNAL(clicked()), this, SLOT(selectUserFile()));
  connect(m_uiForm.csv_browse_btn,SIGNAL(clicked()), this, SLOT(selectCSVFile()));

  connect(m_uiForm.load_dataBtn, SIGNAL(clicked()), this, SLOT(handleLoadButtonClick()));
  connect(m_uiForm.runcentreBtn, SIGNAL(clicked()), this, SLOT(handleRunFindCentre()));

  // Reduction buttons
  connect(m_uiForm.oneDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
  m_reducemapper->setMapping(m_uiForm.oneDBtn, "1D");
  connect(m_uiForm.twoDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
  m_reducemapper->setMapping(m_uiForm.twoDBtn, "2D");
  connect(m_reducemapper, SIGNAL(mapped(const QString &)), this, SLOT(handleReduceButtonClick(const QString &)));
    
  connect(m_uiForm.showMaskBtn, SIGNAL(clicked()), this, SLOT(handleShowMaskButtonClick()));
  connect(m_uiForm.clear_log, SIGNAL(clicked()), m_uiForm.centre_logging, SLOT(clear()));
}
/** Connect signals from the textChanged() signal from text boxes, index changed
*  on ComboBoxes etc.
*/
void SANSRunWindow::connectChangeSignals()
{
  //Connect each box's edited signal to flag if the box's text has changed
  for( int idx = 0; idx < 9; ++idx )
  {
    connect(m_run_no_boxes.value(idx), SIGNAL(textEdited(const QString&)), this, SLOT(runChanged()));
  }

  connect(m_uiForm.smpl_offset, SIGNAL(textEdited(const QString&)), this, SLOT(runChanged()));
  connect(m_uiForm.outfile_edit, SIGNAL(textEdited(const QString&)),
    this, SLOT(enableOrDisableDefaultSave()));

  // Combo boxes
  connect(m_uiForm.wav_dw_opt, SIGNAL(currentIndexChanged(int)), this, 
    SLOT(handleWavComboChange(int)));
  connect(m_uiForm.q_dq_opt, SIGNAL(currentIndexChanged(int)), this, 
    SLOT(handleStepComboChange(int)));
  connect(m_uiForm.qy_dqy_opt, SIGNAL(currentIndexChanged(int)), this, 
    SLOT(handleStepComboChange(int)));

  connect(m_uiForm.inst_opt, SIGNAL(currentIndexChanged(int)), this, 
    SLOT(handleInstrumentChange()));

  // Default transmission switch
  connect(m_uiForm.def_trans, SIGNAL(stateChanged(int)), this, SLOT(updateTransInfo(int)));

  connect(m_uiForm.enableFlood_ck, SIGNAL(stateChanged(int)), this, SLOT(prepareFlood(int)));
}
/**
 * Initialize the widget maps
 */
void SANSRunWindow::initWidgetMaps()
{
  //          single run mode settings
    //Text edit map
    m_run_no_boxes.insert(0, m_uiForm.sct_sample_edit);
    m_run_no_boxes.insert(1, m_uiForm.sct_can_edit);
    m_run_no_boxes.insert(2, m_uiForm.sct_bkgd_edit);
    m_run_no_boxes.insert(3, m_uiForm.tra_sample_edit);
    m_run_no_boxes.insert(4, m_uiForm.tra_can_edit);
    m_run_no_boxes.insert(5, m_uiForm.tra_bkgd_edit);
    m_run_no_boxes.insert(6, m_uiForm.direct_sample_edit);
    m_run_no_boxes.insert(7, m_uiForm.direct_can_edit);
    m_run_no_boxes.insert(8, m_uiForm.direct_bkgd_edit);

    //Period label hash. Each label has a buddy set to its corresponding text edit field
    m_period_lbls.insert(0, m_uiForm.sct_prd_tot1);
    m_period_lbls.insert(1, m_uiForm.sct_prd_tot2);
    m_period_lbls.insert(2, m_uiForm.sct_prd_tot3);
    m_period_lbls.insert(3, m_uiForm.tra_prd_tot1);
    m_period_lbls.insert(4, m_uiForm.tra_prd_tot2);
    m_period_lbls.insert(5, m_uiForm.tra_prd_tot3);
    m_period_lbls.insert(6, m_uiForm.direct_prd_tot1);
    m_period_lbls.insert(7, m_uiForm.direct_prd_tot2);   
    m_period_lbls.insert(8, m_uiForm.direct_prd_tot3);

  //       batch mode settings
  m_allowed_batchtags.insert("sample_sans",0);
  m_allowed_batchtags.insert("sample_trans",1);
  m_allowed_batchtags.insert("sample_direct_beam",2);
  m_allowed_batchtags.insert("can_sans",3);
  m_allowed_batchtags.insert("can_trans",4);
  m_allowed_batchtags.insert("can_direct_beam",5);
  m_allowed_batchtags.insert("background_sans",-1);
  m_allowed_batchtags.insert("background_trans",-1);
  m_allowed_batchtags.insert("background_direct_beam",-1);
  m_allowed_batchtags.insert("output_as",6);

  //            detector info  
  // SANS2D det names/label map
    QHash<QString, QLabel*> labelsmap;
    labelsmap.insert("Front_Det_Z", m_uiForm.dist_smp_frontZ);
    labelsmap.insert("Front_Det_X", m_uiForm.dist_smp_frontX);
    labelsmap.insert("Front_Det_Rot", m_uiForm.smp_rot);
    labelsmap.insert("Rear_Det_X", m_uiForm.dist_smp_rearX);
    labelsmap.insert("Rear_Det_Z", m_uiForm.dist_smp_rearZ);
    m_s2d_detlabels.append(labelsmap);
  
    labelsmap.clear();
    labelsmap.insert("Front_Det_Z", m_uiForm.dist_can_frontZ);
    labelsmap.insert("Front_Det_X", m_uiForm.dist_can_frontX);
    labelsmap.insert("Front_Det_Rot", m_uiForm.can_rot);
    labelsmap.insert("Rear_Det_X", m_uiForm.dist_can_rearX);
    labelsmap.insert("Rear_Det_Z", m_uiForm.dist_can_rearZ);
    m_s2d_detlabels.append(labelsmap);

    labelsmap.clear();
    labelsmap.insert("Front_Det_Z", m_uiForm.dist_bkgd_frontZ);
    labelsmap.insert("Front_Det_X", m_uiForm.dist_bkgd_frontX);
    labelsmap.insert("Front_Det_Rot", m_uiForm.bkgd_rot);
    labelsmap.insert("Rear_Det_X", m_uiForm.dist_bkgd_rearX);
    labelsmap.insert("Rear_Det_Z", m_uiForm.dist_bkgd_rearZ);
    m_s2d_detlabels.append(labelsmap);

    //LOQ labels
    labelsmap.clear();
    labelsmap.insert("moderator-sample", m_uiForm.dist_sample_ms);
    labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_smp_mdb);
    labelsmap.insert("sample-HAB",m_uiForm.dist_smp_hab);
    m_loq_detlabels.append(labelsmap);
  
    labelsmap.clear();
    labelsmap.insert("moderator-sample", m_uiForm.dist_can_ms);
    labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_can_mdb);
    labelsmap.insert("sample-HAB",m_uiForm.dist_can_hab);
    m_loq_detlabels.append(labelsmap);

    labelsmap.clear();
    labelsmap.insert("moderator-sample", m_uiForm.dist_bkgd_ms);
    labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_bkgd_mdb);
    labelsmap.insert("sample-HAB", m_uiForm.dist_bkgd_hab);
    m_loq_detlabels.append(labelsmap);

    // Full workspace names as they appear in the service
    m_workspace_names.clear();

}

/**
 * Restore previous input
 */
void SANSRunWindow::readSettings()
{
  g_log.debug("Reading settings.");
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  m_uiForm.userfile_edit->setText(value_store.value("user_file").toString());
  m_last_dir = value_store.value("last_dir", "").toString();

  int index = m_uiForm.inst_opt->findText(
                              value_store.value("instrum", "LOQ").toString());
  // if the saved instrument no longer exists set index to zero
  index = index < 0 ? 0 : index;
  m_uiForm.inst_opt->setCurrentIndex(index);
  
  int mode_flag = value_store.value("runmode", 0).toInt();
  if( mode_flag == SANSRunWindow::SingleMode )
  {
    m_uiForm.single_mode_btn->click();
  }
  else
  {
    m_uiForm.batch_mode_btn->click();
  }

  //The instrument definition directory
  m_ins_defdir = QString::fromStdString(
    ConfigService::Instance().getString("instrumentDefinition.directory"));
  upDateDataDir();

  // Set allowed extensions
  m_uiForm.file_opt->clear();
  m_uiForm.file_opt->addItem("nexus", QVariant(".nxs"));
  m_uiForm.file_opt->addItem("raw", QVariant(".raw"));
  //Set old file extension
  m_uiForm.file_opt->setCurrentIndex(value_store.value("fileextension", 0).toInt());

  m_uiForm.enableFlood_ck->setChecked(
                   value_store.value("enable_flood_correct",true).toBool());
  m_uiForm.floodFile->setEnabled(m_uiForm.enableFlood_ck->isChecked());
  m_uiForm.floodFile->readSettings("flood_correct");

  int i = m_uiForm.wav_dw_opt->findText(
    value_store.value("wave_binning", "Linear").toString());
  i = i > -1 ? i : 0;
  m_uiForm.wav_dw_opt->setCurrentIndex(i);
  //ensure this is called once even if the index hadn't changed
  handleWavComboChange(i);

  value_store.endGroup();
  readSaveSettings(value_store);

  g_log.debug() << "Found previous data directory " << "\nFound previous user mask file "
    << m_uiForm.userfile_edit->text().toStdString()
    << "\nFound instrument definition directory " << m_ins_defdir.toStdString() << std::endl;

}
/** Sets the states of the checkboxes in the save box using those
* in the passed QSettings object
*  @param valueStore :: where the settings will be stored
*/
void SANSRunWindow::readSaveSettings(QSettings & valueStore)
{
  valueStore.beginGroup("CustomInterfaces/SANSRunWindow/SaveOutput");
  m_uiForm.saveNex_check->setChecked(valueStore.value("nexus",false).toBool());
  m_uiForm.saveCan_check->setChecked(valueStore.value("canSAS",false).toBool());
  m_uiForm.saveRKH_check->setChecked(valueStore.value("RKH", false).toBool());
  m_uiForm.saveCSV_check->setChecked(valueStore.value("CSV", false).toBool());
}

/**
 * Save input for future use
 */
void SANSRunWindow::saveSettings()
{
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  if( !m_uiForm.userfile_edit->text().isEmpty() ) 
  {
    value_store.setValue("user_file", m_uiForm.userfile_edit->text());
  }

  value_store.setValue("last_dir", m_last_dir);

  value_store.setValue("instrum", m_uiForm.inst_opt->currentText());
  value_store.setValue("fileextension", m_uiForm.file_opt->currentIndex());

  value_store.setValue("enable_flood_correct", m_uiForm.enableFlood_ck->isChecked());
  m_uiForm.floodFile->saveSettings("flood_correct");

  value_store.setValue("wave_binning", m_uiForm.wav_dw_opt->currentText());

  unsigned int mode_id(0);
  if( m_uiForm.single_mode_btn->isChecked() )
  {
    mode_id = SANSRunWindow::SingleMode;
  }
  else
  {
    mode_id = SANSRunWindow::BatchMode;
  }
  value_store.setValue("runmode",mode_id);
  value_store.endGroup();
  saveSaveSettings(value_store);
}
/** Stores the state of the checkboxes in the save box with the
* passed QSettings object
*  @param valueStore :: where the settings will be stored
*/
void SANSRunWindow::saveSaveSettings(QSettings & valueStore)
{
  valueStore.beginGroup("CustomInterfaces/SANSRunWindow/SaveOutput");
  valueStore.setValue("nexus", m_uiForm.saveNex_check->isChecked());
  valueStore.setValue("canSAS", m_uiForm.saveCan_check->isChecked());
  valueStore.setValue("RKH", m_uiForm.saveRKH_check->isChecked());
  valueStore.setValue("CSV", m_uiForm.saveCSV_check->isChecked());
}
/**
 * Run a function from the SANS reduction script, ensuring that the first call imports the module
 * @param pycode :: The code to execute
 * @returns A trimmed string containing the output of the code execution
 */
QString SANSRunWindow::runReduceScriptFunction(const QString & pycode)
{
  if( !m_have_reducemodule )
  {
    return QString();
  }
  g_log.debug() << "Executing Python: " << pycode.toStdString() << std::endl;

  const static QString PYTHON_SEP("C++runReduceScriptFunctionC++");
  QString code_torun =  pycode + ";print '"+PYTHON_SEP+"p'";
  QString pythonOut = runPythonCode(code_torun).trimmed();
  
  QStringList allOutput = pythonOut.split(PYTHON_SEP);

  if ( allOutput.count() < 2 )
  {
    QMessageBox::critical(this, "Fatal error found during reduction", "Error reported by Python script, more information maybe found in the scripting console");
    return "Error";
  }

  return allOutput[0];
}

/**
 * Trim off Python markers surrounding things like strings or lists that have been 
 * printed by Python
 */
void SANSRunWindow::trimPyMarkers(QString & txt)
{
 txt.remove(0,1);
 txt.chop(1);
}

/**
 * Load the user file specified in the text field
 * @returns Boolean indicating whether we were successful or not
 */
bool SANSRunWindow::oldLoadUserFile()
{
  QString filetext = m_uiForm.userfile_edit->text();
  if( filetext.isEmpty() ) return false;
  if( QFileInfo(filetext).isRelative() )
  {
    QString data_path(m_uiForm.saveDir_lb->text().trimmed());

    filetext = QDir(data_path).absoluteFilePath(filetext);
  }

  if( !QFileInfo(filetext).exists() ) return false;

  QFile user_file(filetext);
  if( !user_file.open(QIODevice::ReadOnly) ) return false;

  user_file.close();

  
  QString instFunc = m_uiForm.inst_opt->currentText().trimmed();
  instFunc += "()";
  runReduceScriptFunction("SANSReduction."+instFunc);

  // Use python function to read the file and then extract the fields
  if ( runReduceScriptFunction("print SANSReduction.MaskFile(r'"+filetext+"')") != "True\n" )
  {
    return false;
  }
  
  return true;
}
/** Issues a Python command to load the user file and returns any output if
*  there are warnings or errors
*  @param[out] errors the output produced by the string
*  @return the output printed by the Python commands
*/
bool SANSRunWindow::loadUserFile(QString & errors)
{
  QString filetext = m_uiForm.userfile_edit->text().trimmed();
  if( filetext.isEmpty() )
  {
    errors = "No user file has been specified";
    return false;
  }

  if( QFileInfo(filetext).isRelative() )
  {
    QString dataDir(m_uiForm.saveDir_lb->text().trimmed());
    filetext = QDir(dataDir).absoluteFilePath(filetext);
  }
  if( !QFileInfo(filetext).exists() )
  {
    errors = "File \""+filetext+"\" not found aborting";
    return false;
  }
  
  QFile user_file(filetext);
  if( !user_file.open(QIODevice::ReadOnly) )
  {
    errors = "Could not open user file \""+filetext+"\"";
    return false;
  }

  user_file.close();
  
  //Clear the def masking info table.
  int mask_table_count = m_uiForm.mask_table->rowCount();
  for( int i = mask_table_count - 1; i >= 0; --i )
  {
    m_uiForm.mask_table->removeRow(i);
  }

  // Use python function to read the file and then extract the fields
  runReduceScriptFunction(
    "i.ReductionSingleton().user_settings = isis_reduction_steps.UserFile(r'"+filetext+"')");

  errors = runReduceScriptFunction(
    "print i.ReductionSingleton().user_settings.execute(i.ReductionSingleton())").trimmed();
  // create a string list with a string for each line
  const QStringList allOutput = errors.split("\n");
  errors.clear();
  bool canContinue = false;
  for (int i = 0; i < allOutput.count(); ++i)
  {
    if ( i < allOutput.count()-1 )
    {
      errors += allOutput[i]+"\n";
    }
    else
    {
      canContinue = allOutput[i].trimmed() == "True";
    }
  }

  if ( ! canContinue )
  {
    return false;
  }

  const double unit_conv(1000.);
  // Radius
  double dbl_param = runReduceScriptFunction(
      "print i.ReductionSingleton().mask.min_radius").toDouble();
  m_uiForm.rad_min->setText(QString::number(dbl_param*unit_conv));
  dbl_param = runReduceScriptFunction(
      "print i.ReductionSingleton().mask.max_radius").toDouble();
  m_uiForm.rad_max->setText(QString::number(dbl_param*unit_conv));
  //Wavelength
  m_uiForm.wav_min->setText(runReduceScriptFunction(
      "print i.ReductionSingleton().to_wavelen.wav_low"));
  m_uiForm.wav_max->setText(runReduceScriptFunction(
      "print i.ReductionSingleton().to_wavelen.wav_high").trimmed());
  const QString wav_step = runReduceScriptFunction(
      "print i.ReductionSingleton().to_wavelen.wav_step").trimmed();
  setLimitStepParameter("wavelength", wav_step, m_uiForm.wav_dw,
                        m_uiForm.wav_dw_opt);
  //Q
  QString text = runReduceScriptFunction(
      "print i.ReductionSingleton().Q_REBIN");
  QStringList values = text.split(",");
  if( values.count() == 3 )
  {
    m_uiForm.q_min->setText(values[0].trimmed());
    m_uiForm.q_max->setText(values[2].trimmed());
    setLimitStepParameter("Q", values[1].trimmed(), m_uiForm.q_dq,
        m_uiForm.q_dq_opt);
  }
  else
  {
    m_uiForm.q_rebin->setText(text.trimmed());
    m_uiForm.q_dq_opt->setCurrentIndex(2);
  }
  //Qxy
  m_uiForm.qy_max->setText(runReduceScriptFunction(
      "print i.ReductionSingleton().QXY2"));
  setLimitStepParameter("Qxy", runReduceScriptFunction(
      "print i.ReductionSingleton().DQXY"), m_uiForm.qy_dqy,
      m_uiForm.qy_dqy_opt);

  // Tranmission options
  m_uiForm.trans_min->setText(runReduceScriptFunction(
      "print i.ReductionSingleton().get_trans_lambdamin()"));
  m_uiForm.trans_max->setText(runReduceScriptFunction(
      "print i.ReductionSingleton().get_trans_lambdamax()"));
  text = runReduceScriptFunction(
      "print i.ReductionSingleton().transmission_calculator.fit_method").trimmed();
  int index = m_uiForm.trans_opt->findText(text, Qt::MatchCaseSensitive);
  if( index >= 0 )
  {
    m_uiForm.trans_opt->setCurrentIndex(index);
  }

  //Monitor spectra
  m_uiForm.monitor_spec->setText(runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.get_incident_mon()"));
  m_uiForm.trans_monitor->setText(runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.incid_mon_4_trans_calc"));
  m_uiForm.monitor_interp->setChecked(runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.is_interpolating_norm").trimmed() == "True");
  m_uiForm.trans_interp->setChecked(runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.use_interpol_trans_calc"
    ).trimmed() == "True");

  //Direct efficiency correction
  m_uiForm.direct_file->setText(runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.detector_file('rear')"));
  m_uiForm.front_direct_file->setText(runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.detector_file('front')"));

  QString file = runReduceScriptFunction(
      "print i.ReductionSingleton().flood_file.get_filename()");
  file = file.trimmed();
  //Check if the file name is set to Python's None object
  file = file == "None" ? "" : file;
  m_uiForm.floodFile->setFileText(file);

  //Scale factor
  dbl_param = runReduceScriptFunction(
    "print i.ReductionSingleton()._corr_and_scale.rescale").toDouble();
  m_uiForm.scale_factor->setText(QString::number(dbl_param/100.));

  //Sample offset if one has been specified
  dbl_param = runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.SAMPLE_Z_CORR").toDouble();
  m_uiForm.smpl_offset->setText(QString::number(dbl_param*unit_conv));

  //Centre coordinates
  dbl_param = runReduceScriptFunction(
    "print i.ReductionSingleton()._beam_finder.get_beam_center()[0]").toDouble();
  m_uiForm.beam_x->setText(QString::number(dbl_param*1000.0));
  dbl_param = runReduceScriptFunction(
    "print i.ReductionSingleton()._beam_finder.get_beam_center()[1]").toDouble();
  m_uiForm.beam_y->setText(QString::number(dbl_param*1000.0));

  //Gravity switch
  QString param = runReduceScriptFunction(
    "print i.ReductionSingleton().to_Q.get_gravity()").trimmed();
  if( param == "True" )
  {
    m_uiForm.gravity_check->setChecked(true);
  }
  else
  {
    m_uiForm.gravity_check->setChecked(false);
  }
  
  ////Detector bank
  QString detName = runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.cur_detector().name()").trimmed();
  index = m_uiForm.detbank_sel->findText(detName);  
  if( index >= 0 && index < 2 )
  {
    m_uiForm.detbank_sel->setCurrentIndex(index);
  }

  //Masking table
  updateMaskTable();
 
  // Phi values 
  m_uiForm.phi_min->setText(runReduceScriptFunction(
    "print i.ReductionSingleton().mask.phi_min"));
  m_uiForm.phi_max->setText(runReduceScriptFunction(
    "print i.ReductionSingleton().mask.phi_max"));

  if ( runReduceScriptFunction(
    "print i.ReductionSingleton().mask.phi_mirror").trimmed() == "True" )
  {
    m_uiForm.mirror_phi->setChecked(true);
  }
  else
  {
    m_uiForm.mirror_phi->setChecked(false);
  }
  
  m_cfg_loaded = true;
  m_uiForm.userfileBtn->setText("Reload");
  m_uiForm.tabWidget->setTabEnabled(m_uiForm.tabWidget->count() - 1, true);
  return true;
}

/**
 * Load a CSV file specifying information run numbers and populate the batch mode grid
 */
bool SANSRunWindow::loadCSVFile()
{
  QString filename = m_uiForm.csv_filename->text(); 
  QFile csv_file(filename);
  if( !csv_file.open(QIODevice::ReadOnly | QIODevice::Text) )
  {
    showInformationBox("Error: Cannot open CSV file \"" + filename + "\"");
    return false;
  }
  
  //Clear the current table
  clearBatchTable();
  QTextStream file_in(&csv_file);
  int errors(0);
  while( !file_in.atEnd() )
  {
    QString line = file_in.readLine().simplified();
    if( !line.isEmpty() )
    {
      errors += addBatchLine(line, ",");
    }
  }
  if( errors > 0 )
  {
    showInformationBox("Warning: " + QString::number(errors) + " malformed lines detected in \"" + filename + "\". Lines skipped.");
  }
  return true;
}

/**
 * Set a pair of an QLineEdit field and type QComboBox using the parameter given
 * @param pname :: The name of the parameter
 * @param param :: A string representing a value that maybe prefixed with a minus to indicate a different step type
 * @param step_value :: The field to store the actual value
 * @param step_type :: The combo box with the type options
 */
void SANSRunWindow::setLimitStepParameter(const QString& pname, QString param, QLineEdit* step_value, QComboBox* step_type)
{
  if( param.startsWith("-") )
  {
    int index = step_type->findText("Logarithmic");
    if( index < 0 )
    {
     raiseOneTimeMessage("Warning: Unable to find logarithmic scale option for " + pname + ", setting as linear.", 1);
     index = step_type->findText("Linear");
    }
    step_type->setCurrentIndex(index);
    step_value->setText(param.remove(0,1));
  }
  else
  {
    step_type->setCurrentIndex(step_type->findText("Linear"));
    step_value->setText(param);
  }
}

/**
 * Construct the mask table on the Mask tab 
 */
void SANSRunWindow::updateMaskTable()
{
  //Clear the current contents
  for( int i = m_uiForm.mask_table->rowCount() - 1; i >= 0; --i )
  {
	  m_uiForm.mask_table->removeRow(i);
	}

  QString reardet_name("rear-detector"), frontdet_name("front-detector");
  if( m_uiForm.inst_opt->currentText() == "LOQ" )
  {
    reardet_name = "main-detector-bank";
    frontdet_name = "HAB";
  }
  
  // First create 2 default mask cylinders at min and max radius for the beam stop and 
  // corners
  m_uiForm.mask_table->insertRow(0);
  m_uiForm.mask_table->setItem(0, 0, new QTableWidgetItem("beam stop"));
  m_uiForm.mask_table->setItem(0, 1, new QTableWidgetItem(reardet_name));
  m_uiForm.mask_table->setItem(0, 2, new QTableWidgetItem("infinite-cylinder, r = rmin"));
  if( m_uiForm.rad_max->text() != "-1" )
  {  
    m_uiForm.mask_table->insertRow(1);
    m_uiForm.mask_table->setItem(1, 0, new QTableWidgetItem("corners"));
    m_uiForm.mask_table->setItem(1, 1, new QTableWidgetItem(reardet_name));
    m_uiForm.mask_table->setItem(1, 2, new QTableWidgetItem("infinite-cylinder, r = rmax"));
  }

  //Now add information from the mask file
  //Spectrum mask, "Rear" det
  QString mask_string = runReduceScriptFunction(
      "print i.ReductionSingleton().mask.spec_mask_r");
  addSpectrumMasksToTable(mask_string, reardet_name);
  //"Front" det
  mask_string = runReduceScriptFunction(
      "print i.ReductionSingleton().mask.spec_mask_f");
  addSpectrumMasksToTable(mask_string, frontdet_name);

  //Time masks
  mask_string = runReduceScriptFunction(
      "print i.ReductionSingleton().mask.time_mask");
  addTimeMasksToTable(mask_string, "-");
  //Rear detector
  mask_string = runReduceScriptFunction(
      "print i.ReductionSingleton().mask.time_mask_r");
  addTimeMasksToTable(mask_string, reardet_name);
  //Front detectors
  mask_string = runReduceScriptFunction(
      "print i.ReductionSingleton().mask.time_mask_f");
  addTimeMasksToTable(mask_string, frontdet_name);
}

/**
 * Add a spectrum mask string to the mask table
 * @param mask_string :: The string of mask information
 * @param det_name :: The detector it relates to 
 */
void SANSRunWindow::addSpectrumMasksToTable(const QString & mask_string, const QString & det_name)
{
  QStringList elements = mask_string.split(",", QString::SkipEmptyParts);
  QStringListIterator sitr(elements);
  while(sitr.hasNext())
  {
    QString item = sitr.next().trimmed();
    QString col1_txt;
    if( item.startsWith('s', Qt::CaseInsensitive) )
    {
      col1_txt = "Spectrum";
    }
    else if( item.startsWith('h', Qt::CaseInsensitive) || item.startsWith('v', Qt::CaseInsensitive) )
    {
      if( item.contains('+') )
      {
        col1_txt = "Box";
      }
      else
      {
        col1_txt = "Strip";
      }
    }
    else continue;

    int row = m_uiForm.mask_table->rowCount();
    //Insert line after last row
    m_uiForm.mask_table->insertRow(row);
    m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem(col1_txt));
    m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(det_name));
    m_uiForm.mask_table->setItem(row, 2, new QTableWidgetItem(item));
  }
}

/**
 * Add a time mask string to the mask table
 * @param mask_string :: The string of mask information
 * @param det_name :: The detector it relates to 
 */
void SANSRunWindow::addTimeMasksToTable(const QString & mask_string, const QString & det_name)
{
  QStringList elements = mask_string.split(";",QString::SkipEmptyParts);
  QStringListIterator sitr(elements);
  while(sitr.hasNext())
  {
    int row = m_uiForm.mask_table->rowCount();
    m_uiForm.mask_table->insertRow(row);
    m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem("time"));
    m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(det_name));
    const QString shape(sitr.next().trimmed());
    m_uiForm.mask_table->setItem(row, 2, new QTableWidgetItem(shape));
  }
}

/**
 * Retrieve and set the component distances
 * @param workspace :: The workspace pointer
 * @param lms :: The result of the moderator-sample distance
 * @param lsda :: The result of the sample-detector bank 1 distance
 * @param lsdb :: The result of the sample-detector bank 2 distance
 */
void SANSRunWindow::componentLOQDistances(Mantid::API::MatrixWorkspace_sptr workspace, double & lms, double & lsda, double & lsdb)
{
  IInstrument_sptr instr = workspace->getInstrument();
  if( instr == boost::shared_ptr<IInstrument>() ) return;

  Mantid::Geometry::IObjComponent_sptr source = instr->getSource();
  if( source == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;
  Mantid::Geometry::IObjComponent_sptr sample = instr->getSample();
  if( sample == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;

  lms = source->getPos().distance(sample->getPos()) * 1000.;
   
  //Find the main detector bank
  boost::shared_ptr<Mantid::Geometry::IComponent> comp = instr->getComponentByName("main-detector-bank");
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsda = sample->getPos().distance(comp->getPos()) * 1000.;
  }

  comp = instr->getComponentByName("HAB");
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsdb = sample->getPos().distance(comp->getPos()) * 1000.;
  }

}

/**
 * Set the state of processing.
 * @param running :: If we are processing then some interaction is disabled
 * @param type :: The reduction type, 0 = 1D and 1 = 2D
 */
void SANSRunWindow::setProcessingState(bool running, int type)
{
  if( m_uiForm.single_mode_btn->isChecked() )
  {
    m_uiForm.load_dataBtn->setEnabled(!running);
  }
  else
  {
    m_uiForm.load_dataBtn->setEnabled(false);
  }

  //buttons that are available as long as Python is available
  m_uiForm.oneDBtn->setEnabled(!running);
  m_uiForm.twoDBtn->setEnabled(!running);
  m_uiForm.saveSel_btn->setEnabled(!running);
  m_uiForm.runcentreBtn->setEnabled(!running);
  m_uiForm.userfileBtn->setEnabled(!running);
  m_uiForm.data_dirBtn->setEnabled(!running);
  
  if( running )
  {
    m_uiForm.saveDefault_btn->setEnabled(false);

    if( type == 0 )
    {   
      m_uiForm.oneDBtn->setText("Running ...");
    }
    else if( type == 1 )
    {
      m_uiForm.twoDBtn->setText("Running ...");
    }
    else {}
  }
  else
  {
    enableOrDisableDefaultSave();

    m_uiForm.oneDBtn->setText("1D Reduce");
    m_uiForm.twoDBtn->setText("2D Reduce");
  }

  for( int i = 0; i < 4; ++i)
  {
    if( i == m_uiForm.tabWidget->currentIndex() ) continue;
    m_uiForm.tabWidget->setTabEnabled(i, !running);
  }

  QCoreApplication::processEvents();
}

/**
 * Does the workspace exist in the AnalysisDataService
 * @param ws_name :: The name of the workspace
 * @returns A boolean indicatingif the given workspace exists in the AnalysisDataService
 */
bool SANSRunWindow::workspaceExists(const QString & ws_name) const
{
  return AnalysisDataService::Instance().doesExist(ws_name.toStdString());
}

/**
 * @returns A list of the currently available workspaces
 */
QStringList SANSRunWindow::currentWorkspaceList() const
{
  std::set<std::string> ws_list = AnalysisDataService::Instance().getObjectNames();
  std::set<std::string>::const_iterator iend = ws_list.end();
  QStringList current_list;
  for( std::set<std::string>::const_iterator itr = ws_list.begin(); itr != iend; ++itr )
  {
    current_list.append(QString::fromStdString(*itr));
  }
  return current_list;
}

/**
 * Is the user file loaded
 * @returns A boolean indicating whether the user file has been parsed in to the details tab
 */
bool SANSRunWindow::isUserFileLoaded() const
{
  return m_cfg_loaded;
}


/**
 * Create the mask strings for spectra and times
 */
void SANSRunWindow::addUserMaskStrings(QString& exec_script,const QString& importCommand, enum MaskType mType)
{  
  //Clear current
 
  QString temp = importCommand+"('MASK/CLEAR')\n"; 
  exec_script += temp;
  temp = importCommand + "('MASK/CLEAR/TIME')\n";
  exec_script += temp;
  
  //Pull in the table details first, skipping the first two rows
  int nrows = m_uiForm.mask_table->rowCount();
  for(int row = 0; row <  nrows; ++row)
  {
    if( m_uiForm.mask_table->item(row, 2)->text().startsWith("inf") )
    {
      continue;
    }
    if(mType == PixelMask)
    {
      if( m_uiForm.mask_table->item(row, 0)->text() == "time")
      {
        continue;
      }
    }
    else if (mType == TimeMask)
    {
       if( m_uiForm.mask_table->item(row, 0)->text() != "time")
      {
        continue;
      }

    }
    //Details are in the third column
    temp = importCommand + "('MASK";
    exec_script += temp;
    if( m_uiForm.mask_table->item(row, 0)->text() == "time")
    {
      exec_script += "/TIME";
    }
    QString details = m_uiForm.mask_table->item(row, 2)->text();
    QString detname = m_uiForm.mask_table->item(row, 1)->text().trimmed();
    if( detname == "-" )
    {
      exec_script += " " + details;
    }
    else if( detname == "rear-detector" || detname == "main-detector-bank" )
    {
      exec_script += "/REAR " + details;
    }
    else
    {
      exec_script += "/FRONT " + details;
    }
    exec_script += "')\n";
  }

  
  //Spectra mask first
  QStringList mask_params = m_uiForm.user_spec_mask->text().split(",", QString::SkipEmptyParts);
  QStringListIterator sitr(mask_params);
  QString bad_masks;
  while(sitr.hasNext())
  {
    QString item = sitr.next().trimmed();
    if( item.startsWith("REAR", Qt::CaseInsensitive) || item.startsWith("FRONT", Qt::CaseInsensitive) )
    {
      temp = importCommand+"('MASK/" + item + "')\n";
      exec_script += temp;
    }
    else if( item.startsWith('S', Qt::CaseInsensitive) || item.startsWith('H', Qt::CaseInsensitive) ||
        item.startsWith('V', Qt::CaseInsensitive) )
    {
      temp = importCommand +" ('MASK " + item + "')\n";
      
    }
    else
    {
      bad_masks += item + ",";
    }
  }
  if( !bad_masks.isEmpty() )
  {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(QString("Warning: Could not parse the following spectrum masks: ") + bad_masks + ". Values skipped.");
  }

  //Time masks
  mask_params = m_uiForm.user_time_mask->text().split(",", QString::SkipEmptyParts);
  sitr = QStringListIterator(mask_params);
  bad_masks = "";
  while(sitr.hasNext())
  {
    QString item = sitr.next().trimmed();
    if( item.startsWith("REAR", Qt::CaseInsensitive) || item.startsWith("FRONT", Qt::CaseInsensitive) )
    {
      int ndetails = item.split(" ").count();
      if( ndetails == 3 || ndetails == 2 )
      {
        temp = importCommand + "('/TIME" + item + "')\n";
        exec_script += temp;

      }
      else
      {
        bad_masks += item + ",";
      }
    }
  }

  if( !bad_masks.isEmpty() )
  {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(QString("Warning: Could not parse the following time masks: ") + bad_masks + ". Values skipped.");
  }
 
}

/** This method applys mask to a given workspace
  * @param wsName name of the workspace
  * @param time_pixel  true if time mask needs to be applied
*/
void SANSRunWindow::applyMask(const QString& wsName,bool time_pixel)
{
  QString script = "mask= isis_reduction_steps.Mask_ISIS()\n";
  QString str;
  if(time_pixel)
  {
    addUserMaskStrings(str,"mask.parse_instruction",TimeMask);
  }
  else
  {
    addUserMaskStrings(str,"mask.parse_instruction",PixelMask);
  }
  
  script += str;
  script += "mask.execute(i.ReductionSingleton(),\"";
  script += wsName;
  script += "\"";
  script += ",xcentre=0,ycentre=0)";
  runPythonCode(script.trimmed());
 
}
void SANSRunWindow::oldUserMaskStrings(QString & exec_script)
{
  //Clear current
  exec_script += "SANSReduction.Mask('MASK/CLEAR')\n";
  exec_script += "SANSReduction.Mask('MASK/CLEAR/TIME')\n";

  //Pull in the table details first, skipping the first two rows
  int nrows = m_uiForm.mask_table->rowCount();
  for(int row = 0; row <  nrows; ++row)
  {
    if( m_uiForm.mask_table->item(row, 2)->text().startsWith("inf") )
    {
      continue;
    }
    //Details are in the third column
    exec_script += "SANSReduction.Mask('MASK";
    if( m_uiForm.mask_table->item(row, 0)->text() == "time")
    {
      exec_script += "/TIME";
    }
    QString details = m_uiForm.mask_table->item(row, 2)->text();
    QString detname = m_uiForm.mask_table->item(row, 1)->text().trimmed();
    if( detname == "-" )
    {
      exec_script += " " + details;
    }
    else if( detname == "rear-detector" || detname == "main-detector-bank" )
    {
      exec_script += "/REAR " + details;
    }
    else
    {
      exec_script += "/FRONT " + details;
    }
    exec_script += "')\n";
  }

  //Spectra mask first
  QStringList mask_params = m_uiForm.user_spec_mask->text().split(",", QString::SkipEmptyParts);
  QStringListIterator sitr(mask_params);
  QString bad_masks;
  while(sitr.hasNext())
  {
    QString item = sitr.next().trimmed();
    if( item.startsWith("REAR", Qt::CaseInsensitive) || item.startsWith("FRONT", Qt::CaseInsensitive) )
    {
      exec_script += "SANSReduction.Mask('MASK/" + item + "')\n";
    }
    else if( item.startsWith('S', Qt::CaseInsensitive) || item.startsWith('H', Qt::CaseInsensitive) ||
        item.startsWith('V', Qt::CaseInsensitive) )
    {
      exec_script += "SANSReduction.Mask('MASK " + item + "')\n";
    }
    else
    {
      bad_masks += item + ",";
    }
  }
  if( !bad_masks.isEmpty() )
  {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(QString("Warning: Could not parse the following spectrum masks: ") + bad_masks + ". Values skipped.");
  }

  //Time masks
  mask_params = m_uiForm.user_time_mask->text().split(",", QString::SkipEmptyParts);
  sitr = QStringListIterator(mask_params);
  bad_masks = "";
  while(sitr.hasNext())
  {
    QString item = sitr.next().trimmed();
    if( item.startsWith("REAR", Qt::CaseInsensitive) || item.startsWith("FRONT", Qt::CaseInsensitive) )
    {
      int ndetails = item.split(" ").count();
      if( ndetails == 3 || ndetails == 2 )
      {
        exec_script += "SANSReduction.Mask('/TIME" + item + "')\n";
      }
      else
      {
        bad_masks += item + ",";
      }
    }
  }
  if( !bad_masks.isEmpty() )
  {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(QString("Warning: Could not parse the following time masks: ") + bad_masks + ". Values skipped.");
  }
}

/**
 * Set the information about component distances on the geometry tab
 */
void SANSRunWindow::setGeometryDetails(const QString & sample_logs, const QString & can_logs)
{
  resetGeometryDetailsBox();

  double unit_conv(1000.);
  
  QString workspace_name = getWorkspaceName(0);
  if( workspace_name.isEmpty() ) return;
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(workspace_name.toStdString());
  MatrixWorkspace_sptr sample_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  if ( !sample_workspace )
  {//assume all geometry information is in the first member of the group and it is constant for all group members
    //function throws if a fisrt member can't be retrieved
    sample_workspace = getGroupMember(workspace_ptr, 1);
  }

  IInstrument_sptr instr = sample_workspace->getInstrument();
  boost::shared_ptr<Mantid::Geometry::IComponent> source = instr->getSource();

  // Moderator-monitor distance is common to LOQ and S2D
  int monitor_spectrum = m_uiForm.monitor_spec->text().toInt();
  std::vector<int> dets = sample_workspace->spectraMap().getDetectors(monitor_spectrum);
  if( dets.empty() ) return;
  double dist_mm(0.0);
  QString colour("black");
  try
  {
    Mantid::Geometry::IDetector_sptr detector = instr->getDetector(dets[0]);  
    dist_mm = detector->getDistance(*source) * unit_conv;
  }
  catch(std::runtime_error&)
  {
    colour = "red";
  }

  if( m_uiForm.inst_opt->currentText() == "LOQ" )
  {
    if( colour == "red" )
    {
      m_uiForm.dist_mod_mon->setText("<font color='red'>error<font>");
    }
    else
    {
      m_uiForm.dist_mod_mon->setText(formatDouble(dist_mm, colour));
    }
    setLOQGeometry(sample_workspace, 0);
    QString can = getWorkspaceName(1);
    if( !can.isEmpty() )
    {
      Workspace_sptr workspace_ptr = Mantid::API::AnalysisDataService::Instance().retrieve(can.toStdString());
      MatrixWorkspace_sptr can_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
      
      if ( ! can_workspace )
      {//assume all geometry information is in the first member of the group and it is constant for all group members
        //function throws if a fisrt member can't be retrieved
        can_workspace = getGroupMember(workspace_ptr, 1);
      }
      setLOQGeometry(can_workspace, 1);
    }
  }
  else if( m_uiForm.inst_opt->currentText() == "SANS2D" )
  {
    if( colour == "red" )
    {
      m_uiForm.dist_mon_s2d->setText("<font color='red'>error<font>");
    }
    else
    {
      m_uiForm.dist_mon_s2d->setText(formatDouble(dist_mm, colour));
    }

    //SANS2D - Sample
    setSANS2DGeometry(sample_workspace, sample_logs, 0);
    //Get the can workspace if there is one
    QString can = getWorkspaceName(1);
    if( can.isEmpty() ) 
    {
      return;
    }
    Workspace_sptr workspace_ptr;
    try 
    { 
      workspace_ptr = AnalysisDataService::Instance().retrieve(can.toStdString());
    }
    catch(std::runtime_error&)
    {
      return;
    }

    Mantid::API::MatrixWorkspace_sptr can_workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace_ptr);
    if ( !can_workspace )
    {//assume all geometry information is in the first member of the group and it is constant for all group members
      //function throws if a fisrt member can't be retrieved
      can_workspace = getGroupMember(workspace_ptr, 1);
    }

    setSANS2DGeometry(can_workspace, can_logs, 1);

    //Check for discrepancies
    bool warn_user(false);
    double lms_sample(m_uiForm.dist_sample_ms_s2d->text().toDouble()), lms_can(m_uiForm.dist_can_ms_s2d->text().toDouble());
    if( std::fabs(lms_sample - lms_can) > 5e-03 )
    {
      warn_user = true;
      markError(m_uiForm.dist_sample_ms_s2d);
      markError(m_uiForm.dist_can_ms_s2d);
    }

    QString marked_dets = runReduceScriptFunction("print i.GetMismatchedDetList(),").trimmed();
    trimPyMarkers(marked_dets);
    if( !marked_dets.isEmpty() )
    {
      QStringList detnames = marked_dets.split(",");
      QStringListIterator itr(detnames);
      while( itr.hasNext() )
      {
        QString name = itr.next().trimmed();
        trimPyMarkers(name);
        for( int i = 0; i < 2; ++i )
        {
          markError(m_s2d_detlabels[i].value(name));
          warn_user = true;
        }
      }
    }
    if( warn_user )
    {
      raiseOneTimeMessage("Warning: Some detector distances do not match for the assigned Sample/Can runs, see Geometry tab for details.");
    }
  }
}

/**
 * Set SANS2D geometry info
 * @param workspace :: The workspace
 * @param logs :: The log information
*/
void SANSRunWindow::setSANS2DGeometry(Mantid::API::MatrixWorkspace_sptr workspace, const QString & logs, int wscode)
{  
  double unitconv = 1000.;

  IInstrument_sptr instr = workspace->getInstrument();
  boost::shared_ptr<Mantid::Geometry::IComponent> sample = instr->getSample();
  boost::shared_ptr<Mantid::Geometry::IComponent> source = instr->getSource();
  double distance = source->getDistance(*sample) * unitconv;
  //Moderator-sample
  QLabel *dist_label(NULL); 
  if( wscode == 0 )
  {
    dist_label = m_uiForm.dist_sample_ms_s2d;
  }
  else if( wscode == 1 )
  {
    dist_label = m_uiForm.dist_can_ms_s2d;
  }
  else
  {
    dist_label = m_uiForm.dist_bkgd_ms_s2d;
  }
  dist_label->setText(formatDouble(distance, "black"));

  //Detectors
  QStringList det_info = logs.split(",");
  QStringListIterator itr(det_info);
  while( itr.hasNext() )
  {
    QString line = itr.next();
    QStringList values = line.split(":");
    QString detname = values[0].trimmed();
    QString distance = values[1].trimmed();
    trimPyMarkers(detname);
    trimPyMarkers(distance);
  
    QLabel *lbl = m_s2d_detlabels[wscode].value(detname);
    if( lbl ) lbl->setText(distance);
  }
}

/**
 * Set LOQ geometry information
 * @param workspace :: The workspace to operate on
 */
void SANSRunWindow::setLOQGeometry(Mantid::API::MatrixWorkspace_sptr workspace, int wscode)
{
  double dist_ms(0.0), dist_mdb(0.0), dist_hab(0.0);
  //Sample
  componentLOQDistances(workspace, dist_ms, dist_mdb, dist_hab);
  
  QHash<QString, QLabel*> & labels = m_loq_detlabels[wscode];
  QLabel *detlabel = labels.value("moderator-sample");
  if( detlabel )
  {
    detlabel->setText(QString::number(dist_ms));
  }

  detlabel = labels.value("sample-main-detector-bank");
  if( detlabel )
  {
    detlabel->setText(QString::number(dist_mdb));
  }

  detlabel = labels.value("sample-HAB");
  if( detlabel )
  {
    detlabel->setText(QString::number(dist_hab));
  }

}

/**
 * Mark an error on a label
 * @param label :: A pointer to a QLabel instance
 */
void SANSRunWindow::markError(QLabel* label)
{
  if( label )
  {
    label->setText("<font color=\"red\">" + label->text() + "</font>");
  }
}

//-------------------------------------
// Private SLOTS
//------------------------------------
/**
 * Select the base directory for the data
 */
void SANSRunWindow::selectDataDir()
{
  MantidQt::API::ManageUserDirectories::openUserDirsDialog(this);
}

/**
 * Select and load the user file
 */
void SANSRunWindow::selectUserFile()
{
  if( !browseForFile("Select a user file", m_uiForm.userfile_edit) )
  {
    return;
  }
  
  runReduceScriptFunction("i.ReductionSingleton().user_file_path='"+
    QFileInfo(m_uiForm.userfile_edit->text()).path() + "'");

  QString loadErrors;
  if( loadUserFile(loadErrors) )
  {// the load was successful
    if ( ! loadErrors.isEmpty() )
    {//but there are some warnings to display
      showInformationBox("User file opened with some warnings:\n"+loadErrors);
    }
  }
  else
  {// there was a fatal problem with the load we can not continue, the error should already have been raised at this point
    m_cfg_loaded = false;
    return;
  }

  //Check for warnings
  checkLogFlags();

  m_cfg_loaded = true;
  emit userfileLoaded();
  m_uiForm.tabWidget->setTabEnabled(1, true);
  m_uiForm.tabWidget->setTabEnabled(2, true);
  m_uiForm.tabWidget->setTabEnabled(3, true);
  

  //path() returns the directory
  m_last_dir = QFileInfo(m_uiForm.userfile_edit->text()).path();
}

/**
 * Select and load a CSV file
 */
void SANSRunWindow::selectCSVFile()
{
  if( !m_cfg_loaded )
  {
    showInformationBox("Please load the relevant user file.");
    return;
  }

  if( !browseForFile("Select CSV file",m_uiForm.csv_filename, "CSV files (*.csv)") )
  {
    return;
  }

  if( !loadCSVFile() )
  {
    return;
  }
  //path() returns the directory
  m_last_dir = QFileInfo(m_uiForm.csv_filename->text()).path();
  if( m_cfg_loaded ) setProcessingState(false, -1);
}
/** Raises a browse dialog and inserts the selected file into the
*  save text edit box, outfile_edit
*/
void SANSRunWindow::saveFileBrowse()
{
  QString title = "Save output workspace as";

  QSettings prevValues;
  prevValues.beginGroup("CustomInterfaces/SANSRunWindow/SaveOutput");
  //use their previous directory first and go to their default if that fails
  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultsave.directory"))).toString();

  const QString filter = ";;AllFiles (*.*)";
  
  QString oFile = FileDialogHandler::getSaveFileName(this, title, 
    prevPath+"/"+m_uiForm.outfile_edit->text());

  if( ! oFile.isEmpty() )
  {
    m_uiForm.outfile_edit->setText(oFile);
    
    QString directory = QFileInfo(oFile).path();
    prevValues.setValue("dir", directory);
  }
}
/**
 * Mark that a run number has changed
*/
void SANSRunWindow::runChanged()
{
  m_warnings_issued = false;
  forceDataReload(true);
}
/**
 * Flip the flag to confirm whether data is reloaded
 * @param force :: If true, the data is reloaded when reduce is clicked
 */
void SANSRunWindow::forceDataReload(bool force)
{
  m_force_reload = force;
}

/**
 * Browse for a file and set the text of the given edit box
 * @param box_title :: The title field for the display box
 * @param A :: QLineEdit box to use for the file path
 * @param file_filter :: An optional file filter
 */
bool SANSRunWindow::browseForFile(const QString & box_title, QLineEdit* file_field, QString file_filter)
{
  QString box_text = file_field->text();
  QString start_path = box_text;
  if( box_text.isEmpty() )
  {
    start_path = m_last_dir;
  }
  file_filter += ";;AllFiles (*.*)";
  QString file_path = QFileDialog::getOpenFileName(this, box_title, start_path, file_filter);    
  if( file_path.isEmpty() || QFileInfo(file_path).isDir() ) return false;
  file_field->setText(file_path);
  return true;
}

bool SANSRunWindow::oldLoadButtonClick()
{
  QString origin_dir = QDir::currentPath();

  QString data_dir(m_uiForm.saveDir_lb->text());
  if( !data_dir.endsWith('/') ) data_dir += "/";
  runReduceScriptFunction("import SANSReduction\nSANSReduction.INSTRUMENT = i.ReductionSingleton().instrument\nSANSReduction.DataPath('" + data_dir + "')");

  runReduceScriptFunction("SANSReduction.UserPath('" + QFileInfo(m_uiForm.userfile_edit->text()).path() + "')");
  oldLoadUserFile();

  setProcessingState(true, -1);
  m_uiForm.load_dataBtn->setText("Loading ...");

  if( m_force_reload ) cleanup();

  QString run_number = m_run_no_boxes.value(0)->text();
  if( run_number.isEmpty() )
  {
    showInformationBox("Error: No sample run given, cannot continue.");
    setProcessingState(false, -1);
    m_uiForm.load_dataBtn->setText("Loading Data");
    return false;
  }

  if(!m_run_no_boxes.value(3)->text().isEmpty() && m_run_no_boxes.value(6)->text().isEmpty() )
  {
    showInformationBox("Error: Can run supplied without direct run, cannot continue.");
    setProcessingState(false, -1);
      m_uiForm.load_dataBtn->setText("Load Data");
    return false;
  }

  QString sample_logs, can_logs;
  bool is_loaded(true);
  QString error;
  //Quick check that there is a can direct run if a trans can is defined. If not use the sample one
  if( !m_run_no_boxes.value(4)->text().isEmpty() && m_run_no_boxes.value(7)->text().isEmpty() )
  {
    m_run_no_boxes.value(7)->setText(m_run_no_boxes.value(6)->text());
  }

  QHashIterator<int, QLineEdit*> itr(m_run_no_boxes);
  while( itr.hasNext() )
  {
    itr.next();
    int key = itr.key();
    // Skip background as we are not using those at the moment.
    if( key == 2 ) continue;
    if( key == 5 ) break;
    QString run_no = itr.value()->text();
    QString logs;
    if( run_no.isEmpty() )
    {
      m_workspace_names.insert(key, "");
      try
      {
        //Clear any that are assigned
        oldAssign(key, logs);
      }
      catch(std::runtime_error)
      {//the user should already have seen an error message box pop up
        g_log.error() << "Problem loading file\n";
        is_loaded = false;
        break;
      }
      continue;
    }
    try
    {
      is_loaded &= oldAssign(key, logs);
    }
    catch(std::runtime_error)
    {//the user should already have seen an error message box pop up
      g_log.error() << "Problem loading file\n";
      is_loaded = false;
      break;
     }
    // Check if the last LoadRaw algorithm was run successfully. If so then any problem with
    // loading is with the log files for the first 2 keys
    Mantid::API::IAlgorithm_sptr last_run = Mantid::API::AlgorithmManager::Instance().algorithms().back();
    bool raw_data_ok = last_run->isExecuted();
    if( !raw_data_ok )
    {
      QString period("");
      if ( getPeriod(key) > 1 )
      {
        period = QString("period ") + QString::number(getPeriod(key));
      }
      showInformationBox("Error: Cannot load run \""+run_no+"\" " + period + ", see results log for details.");
      break;
    }
    if( key == 0 )
    {
      sample_logs = logs;
      if(m_uiForm.inst_opt->currentText() == "SANS2D" && sample_logs.isEmpty())
      {
        is_loaded = false;
        showInformationBox("Error: Cannot find log file for sample run, cannot continue.");
        break;
      }
    }
    else if( key == 1 )
    {
      can_logs = logs;
      if( m_uiForm.inst_opt->currentText() == "SANS2D" && can_logs.isEmpty() )
      {
        can_logs = sample_logs;
        showInformationBox("Warning: Cannot find log file for can run, using sample values.");
      }
    }
    else{}
  }
  if (!is_loaded)
  {
    setProcessingState(false, -1);
    m_uiForm.load_dataBtn->setText("Load Data");
    return false;
  }



  for( int index = 1; index < m_uiForm.tabWidget->count(); ++index )
  {
    m_uiForm.tabWidget->setTabEnabled(index, true);
  }
  setProcessingState(false, -1);
  m_uiForm.load_dataBtn->setText("Load Data");
  return true;
}
/**
 * Receive a load button click signal
 */
bool SANSRunWindow::handleLoadButtonClick()
{
  // Check if we have loaded the data_file
  if( !isUserFileLoaded() )
  {
    showInformationBox("Please load the relevant user file.");
    return false;
  }

  setProcessingState(true, -1);
  m_uiForm.load_dataBtn->setText("Loading ...");

  if( m_force_reload ) cleanup();

  QString run_number = m_run_no_boxes.value(0)->text();
  if( run_number.isEmpty() )
  {
    showInformationBox("Error: No sample run given, cannot continue.");
    setProcessingState(false, -1);
    m_uiForm.load_dataBtn->setText("Loading Data");
    return false;
  }

  if(!m_run_no_boxes.value(3)->text().isEmpty() && m_run_no_boxes.value(6)->text().isEmpty() )
  {
    showInformationBox("Error: Can run supplied without direct run, cannot continue.");
    setProcessingState(false, -1);
      m_uiForm.load_dataBtn->setText("Load Data");
    return false;
  }

  QString sample_logs, can_logs;
  bool is_loaded(true); 
  QString error;
  //Quick check that there is a can direct run if a trans can is defined. If not use the sample one
  if( !m_run_no_boxes.value(4)->text().isEmpty() && m_run_no_boxes.value(7)->text().isEmpty() )
  {
    m_run_no_boxes.value(7)->setText(m_run_no_boxes.value(6)->text());
  }

  QHashIterator<int, QLineEdit*> itr(m_run_no_boxes);
  while( itr.hasNext() )
  {
    itr.next();
    int key = itr.key();
    // Skip background as we are not using those at the moment.
    if( key == 2 ) continue;
    if( key == 5 ) break;
    QString run_no = itr.value()->text();
    QString logs;
    if( run_no.isEmpty() ) 
    {
      m_workspace_names.insert(key, "");
      try
      {
        //Clear any that are assigned
        runAssign(key, logs);
      }
      catch(std::runtime_error)
      {//the user should already have seen an error message box pop up
        g_log.error() << "Problem loading file\n";
        is_loaded = false;
        break;
      }
      continue;
    }
    try
    {
      is_loaded &= runAssign(key, logs);
    }
    catch(std::runtime_error)
    {//the user should already have seen an error message box pop up
      g_log.error() << "Problem loading file\n";
      is_loaded = false;
      break;
     }
    // Check if the last LoadRaw algorithm was run successfully. If so then any problem with
    // loading is with the log files for the first 2 keys
    Mantid::API::IAlgorithm_sptr last_run = Mantid::API::AlgorithmManager::Instance().algorithms().back();
    bool raw_data_ok = last_run->isExecuted();
    if( !raw_data_ok )
    {
      QString period("");
      if ( getPeriod(key) > 1 )
      {
        period = QString("period ") + QString::number(getPeriod(key));
      }
      showInformationBox("Error: Cannot load run \""+run_no+"\" " + period + ", see results log for details.");
      break;
    }
    if( key == 0 ) 
    { 
      sample_logs = logs;
      if(m_uiForm.inst_opt->currentText() == "SANS2D" && sample_logs.isEmpty())
      {
        is_loaded = false;
        showInformationBox("Error: Cannot find log file for sample run, cannot continue.");
        break;
      }
    }
    else if( key == 1 ) 
    { 
      can_logs = logs;
      if(m_uiForm.inst_opt->currentText() == "SANS2D" && can_logs.isEmpty())
      {
        can_logs = sample_logs;
        showInformationBox("Warning: Cannot find log file for can run, using sample values.");
      }
    }
    else{}
  }
  if (!is_loaded) 
  {
    setProcessingState(false, -1);
    m_uiForm.load_dataBtn->setText("Load Data");
    return false;
  }

  // Sort out the log information
  setGeometryDetails(sample_logs, can_logs);
  
  Mantid::API::Workspace_sptr baseWS =
    Mantid::API::AnalysisDataService::Instance().retrieve(getWorkspaceName(0).toStdString());
  // Enter information from sample workspace on to analysis and geometry tab
  Mantid::API::MatrixWorkspace_sptr sample_workspace =
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(baseWS);
    
  if ( ! sample_workspace )
  {
    try
    {
      sample_workspace = getGroupMember(baseWS, m_uiForm.sct_smp_prd->text().toInt());
    }
    catch(std::exception &)
    {
      setProcessingState(false, -1);
      m_uiForm.load_dataBtn->setText("Load Data");
      showInformationBox("Error: Could not retrieve sample workspace from Mantid");
      return false;
    }
  }
  
  if( sample_workspace != boost::shared_ptr<Mantid::API::MatrixWorkspace>() && !sample_workspace->readX(0).empty() )
  {
    m_uiForm.tof_min->setText(QString::number(sample_workspace->readX(0).front())); 
    m_uiForm.tof_max->setText(QString::number(sample_workspace->readX(0).back()));
  }

  // Set the geometry if the sample has been changed
  if ( m_sample_no != run_number )
  {
    int geomid  = sample_workspace->sample().getGeometryFlag();
    if( geomid > 0 && geomid < 4 )
    {
      m_uiForm.sample_geomid->setCurrentIndex(geomid - 1);
      m_uiForm.sample_thick->setText(QString::number(sample_workspace->sample().getThickness()));
      m_uiForm.sample_width->setText(QString::number(sample_workspace->sample().getWidth()));
      m_uiForm.sample_height->setText(QString::number(sample_workspace->sample().getHeight()));
    }
    else
    {
      m_uiForm.sample_geomid->setCurrentIndex(2);
      m_uiForm.sample_thick->setText("1");
      m_uiForm.sample_width->setText("8");
      m_uiForm.sample_height->setText("8");
      //Warn user
      showInformationBox("Warning: Incorrect geometry flag encountered: " + QString::number(geomid) +". Using default values.");
    }
  }

  forceDataReload(false);

  for( int index = 1; index < m_uiForm.tabWidget->count(); ++index )
  {
    m_uiForm.tabWidget->setTabEnabled(index, true);
  }
 
  m_sample_no = run_number;
  setProcessingState(false, -1);
  m_uiForm.load_dataBtn->setText("Load Data");
  return true;
}

/** 
 * Construct the python code to perform the analysis based on the 
 * current settings
 * @param type :: The reduction type: 1D or 2D
 */
QString SANSRunWindow::createAnalysisDetailsScript(const QString & type)
{
  //Construct a run script based upon the current values within the various widgets
  QString exec_reduce = "i.ReductionSingleton().instrument.setDetector('" +
                            m_uiForm.detbank_sel->currentText() + "')\n";

  exec_reduce += "i.ReductionSingleton().to_Q.output_type='"+type+"'\n";
  //Analysis details
  exec_reduce +="i.ReductionSingleton().user_settings.readLimitValues('L/R '+'"+
    //get rid of the 1 in the line below, a character is need at the moment to give the correct number of characters
    m_uiForm.rad_min->text()+" '+'"+m_uiForm.rad_max->text()+" '+'1', i.ReductionSingleton())\n";

  QString logLin = m_uiForm.wav_dw_opt->currentText().toUpper();
  if (logLin.contains("LOG"))
  {
    logLin = "LOG";
  }
  if (logLin.contains("LIN"))
  {
    logLin = "LIN";
  }
  exec_reduce += "i.LimitsWav(" + m_uiForm.wav_min->text().trimmed() + "," + m_uiForm.wav_max->text() + "," +
    m_uiForm.wav_dw->text()+",'"+logLin+"')\n";

  if( m_uiForm.q_dq_opt->currentIndex() == 2 )
  {
    exec_reduce += "i.ReductionSingleton().user_settings.readLimitValues('L/Q "+m_uiForm.q_rebin->text() +
      "', i.ReductionSingleton())\n";
  }
  else
  {
    exec_reduce += "i.ReductionSingleton().user_settings.readLimitValues('L/Q "+
      m_uiForm.q_min->text()+" "+m_uiForm.q_max->text()+" "+m_uiForm.q_dq->text()+"/"+
      m_uiForm.q_dq_opt->itemData(m_uiForm.q_dq_opt->currentIndex()).toString() +
      "', i.ReductionSingleton())\n";
  }
  exec_reduce += "i.LimitsQXY(0.0," + m_uiForm.qy_max->text().trimmed() + "," +
    m_uiForm.qy_dqy->text().trimmed() + ",'"
    + m_uiForm.qy_dqy_opt->itemData(m_uiForm.qy_dqy_opt->currentIndex()).toString()+"')\n" +
    "i.LimitsPhi(" + m_uiForm.phi_min->text().trimmed() + "," + m_uiForm.phi_max->text().trimmed();
  if ( m_uiForm.mirror_phi->isChecked() )
  {
    exec_reduce += ", True";
  }
  else
  {
    exec_reduce += ", False";
  }
  exec_reduce += ")\n";
  QString floodFile =
    m_uiForm.enableFlood_ck->isChecked() ? m_uiForm.floodFile->getFirstFilename().trimmed() : "";
  exec_reduce += "i.SetDetectorFloodFile('"+floodFile+"')\n";

  //Transmission behaviour
  exec_reduce += "i.TransFit('" + m_uiForm.trans_opt->currentText() + "','" +
    m_uiForm.trans_min->text().trimmed()+"','"+m_uiForm.trans_max->text().trimmed()+"')\n";

  //Centre values
  exec_reduce += "i.SetCentre('" + m_uiForm.beam_x->text()+
                 "','"+m_uiForm.beam_y->text()+"')\n";
  //Gravity correction
  exec_reduce += "i.Gravity(";
  if( m_uiForm.gravity_check->isChecked() )
  {
    exec_reduce += "True";
  }
  else
  {
    exec_reduce += "False";
  }
  exec_reduce += ")\n";
  //Sample offset
  exec_reduce += "i.SetSampleOffset('"+
                 m_uiForm.smpl_offset->text()+"')\n";

  //Monitor spectrum
  exec_reduce += "i.SetMonitorSpectrum('" + m_uiForm.monitor_spec->text().trimmed() + "',";
  exec_reduce += m_uiForm.monitor_interp->isChecked() ? "True" : "False";
  exec_reduce += ")\n";
  //the monitor to normalise the tranmission spectrum against
  exec_reduce += "i.SetTransSpectrum('" + m_uiForm.trans_monitor->text().trimmed() + "',";
  exec_reduce += m_uiForm.trans_interp->isChecked() ? "True" : "False";
  exec_reduce += ")\n";
  //mask strings that the user has entered manually on to the GUI
  addUserMaskStrings(exec_reduce,"i.Mask",DefaultMask);

  //Set geometry info
  exec_reduce += 
    "i.ReductionSingleton().geometry.height = " + m_uiForm.sample_height->text()+"\n"+
    "i.ReductionSingleton().geometry.width = " + m_uiForm.sample_width->text()+"\n" +
    "i.ReductionSingleton().geometry.thickness = " + m_uiForm.sample_thick->text() +"\n"+
    "i.ReductionSingleton().geometry.shape = " + m_uiForm.sample_geomid->currentText().at(0)+"\n";
 
  return exec_reduce;
}

QString SANSRunWindow::createOldAnalysisDetailsScript(const QString & type)
{
  //Construct a run script based upon the current values within the various widgets
  QString exec_reduce = "SANSReduction.Detector('" + m_uiForm.detbank_sel->currentText() + "')\n";

  //Add the path in the single mode data box if it is not empty
  QString data_path(m_uiForm.saveDir_lb->text().trimmed());

  if( !data_path.isEmpty() )
  {
    exec_reduce += "SANSReduction.DataPath('" + data_path + "')\n";
  }

  if( type.startsWith("1D") )
  {
    exec_reduce += "SANSReduction.Set1D()\n";
  }
  else
  {
    exec_reduce += "SANSReduction.Set2D()\n";
  }
  //Analysis details
  exec_reduce +=
    "SANSReduction.LimitsR(" + m_uiForm.rad_min->text() + "," + m_uiForm.rad_max->text() + ")\n" +
    "SANSReduction.LimitsWav(" + m_uiForm.wav_min->text() + "," + m_uiForm.wav_max->text() + "," +
    m_uiForm.wav_dw->text() + ",'" + m_uiForm.wav_dw_opt->itemData(m_uiForm.wav_dw_opt->currentIndex()).toString() + "')\n";
  if( m_uiForm.q_dq_opt->currentIndex() == 2 )
  {
    exec_reduce += "SANSReduction.LimitsQ('" + m_uiForm.q_rebin->text() + "')\n";
  }
  else
  {
    exec_reduce += "SANSReduction.LimitsQ(" + m_uiForm.q_min->text() + "," + m_uiForm.q_max->text() + "," +
      m_uiForm.q_dq->text() + ",'" + m_uiForm.q_dq_opt->itemData(m_uiForm.q_dq_opt->currentIndex()).toString() + "')\n";
  }
  exec_reduce += "SANSReduction.LimitsQXY(0.0," + m_uiForm.qy_max->text() + "," +
    m_uiForm.qy_dqy->text() + ",'" + m_uiForm.qy_dqy_opt->itemData(m_uiForm.qy_dqy_opt->currentIndex()).toString() + "')\n" +
    "SANSReduction.LimitsPhi(" + m_uiForm.phi_min->text() + "," + m_uiForm.phi_max->text();
  if ( m_uiForm.mirror_phi->isChecked() )
  {
    exec_reduce += ", True)\n";
  }
  else
  {
    exec_reduce += ", False)\n";
  }
  QString floodFile =
    m_uiForm.enableFlood_ck->isChecked() ? m_uiForm.floodFile->getFirstFilename().trimmed() : "";
  exec_reduce += "SANSReduction.SetDetectorFloodFile('"+floodFile+"')\n";

  //Transmission behaviour
  QString fitType = m_uiForm.trans_opt->currentText().trimmed().toUpper();
  if (fitType == "LOGARITHMIC")
  {
    fitType = "LOG";
  }
  exec_reduce += "SANSReduction.TransFit('" + fitType + "'," +
    m_uiForm.trans_min->text() + "," + m_uiForm.trans_max->text() + ")\n";

  //Centre values
  exec_reduce += "SANSReduction.SetCentre(" + m_uiForm.beam_x->text() + "," + m_uiForm.beam_y->text() + ")\n";
  //Gravity correction
  exec_reduce += "SANSReduction.Gravity(";
  if( m_uiForm.gravity_check->isChecked() )
  {
    exec_reduce += "True)\n";
  }
  else
  {
    exec_reduce += "False)\n";
  }
  //Sample offset
  exec_reduce += "SANSReduction.SetSampleOffset(" + m_uiForm.smpl_offset->text() + ")\n";

  //Monitor spectrum
  exec_reduce += "SANSReduction.SetMonitorSpectrum(" + m_uiForm.monitor_spec->text() + ",";
  exec_reduce += m_uiForm.monitor_interp->isChecked() ? "True" : "False";
  exec_reduce += ")\n";
  //the monitor to normalise the tranmission spectrum against
  exec_reduce += "SANSReduction.SetTransSpectrum(" + m_uiForm.trans_monitor->text() + ",";
  exec_reduce += m_uiForm.trans_interp->isChecked() ? "True" : "False";
  exec_reduce += ")\n";
  //Extra mask information
  oldUserMaskStrings(exec_reduce);

  //Set geometry info
  exec_reduce +=
    "SANSReduction.SampleHeight(" + m_uiForm.sample_height->text() + ")\n" +
    "SANSReduction.SampleWidth(" + m_uiForm.sample_width->text() + ")\n" +
    "SANSReduction.SampleThickness(" + m_uiForm.sample_thick->text() + ")\n"
    "SANSReduction.SampleGeometry(" + m_uiForm.sample_geomid->currentText().at(0) + ")\n";

  return exec_reduce;
}
/**
 * Run the analysis script
 * @param type :: The data reduction type, 1D or 2D
 */
void SANSRunWindow::handleReduceButtonClick(const QString & type)
{
  if ( ! entriesAreValid() )
  {
    QMessageBox::warning(this, "Validation Error", "There is a problem with one or more entries on the form. These are marked\nwith an *");
    return;
  }
  //new reduction is going to take place, remove the results from the last reduction
  resetDefaultOutput();

  //The possiblities are batch mode or single run mode
  const RunMode runMode =
    m_uiForm.single_mode_btn->isChecked() ? SingleMode : BatchMode;
  if ( runMode == SingleMode )
  {
    // Currently the components are moved with each reduce click. Check if a load is necessary
    // This must be done before the script is written as we need to get correct values from the
    // loaded raw data
    if ( ! handleLoadButtonClick() )
    {
      return;
    }
  }

  QString py_code = createAnalysisDetailsScript(type);
  if( py_code.isEmpty() )
  {
    showInformationBox("Error: An error occurred while constructing the reduction code, please check installation.");
    return;
  }

  const static QString PYTHON_SEP("C++handleReduceButtonClickC++");

  //copy the user setting to use as a base for future reductions after the one that is about to start
  py_code += "\n_user_settings_copy = copy.deepcopy(i.ReductionSingleton().user_settings)";
  const QString verb = m_uiForm.verbose_check ? "True" : "False";
  py_code += "\ni.SetVerboseMode(" + verb + ")";
  //Need to check which mode we're in
  if ( runMode == SingleMode )
  {
    py_code += reduceSingleRun();
    //output the name of the output workspace, this is returned up by the runPythonCode() call below
    py_code += "\nprint '"+PYTHON_SEP+"'+reduced+'"+PYTHON_SEP+"'";
  }
  else
  {
    //Have we got anything to reduce?
    if( m_uiForm.batch_table->rowCount() == 0 )
    {
      showInformationBox("Error: No run information specified.");
      return;
    }

    QString csv_file(m_uiForm.csv_filename->text());
    if( m_dirty_batch_grid )
    {
      QString selected_file = QFileDialog::getSaveFileName(this, "Save as CSV", m_last_dir);
      csv_file = saveBatchGrid(selected_file);
    }
    py_code = "import SANSBatchMode as batch\n" + py_code;
    py_code += "\nbatch.BatchReduce('" + csv_file + "','" + m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString() + "',";
    py_code += m_uiForm.def_trans->isChecked() ? "True" : "False";
    if( m_uiForm.plot_check->isChecked() )
    {
      py_code += ", plotresults=True";
    }

    py_code += ", saveAlgs={";
    QStringList algs(getSaveAlgs());
    for ( QStringList::const_iterator it = algs.begin(); it != algs.end(); ++it)
    {// write a Python dict object in the form { algorithm_name : file extension , ... ,}
      py_code += "'"+*it+"':'"+SaveWorkspaces::getSaveAlgExt(*it)+"',";
    }
    py_code += "}";

    if( m_uiForm.log_colette->isChecked() )
    {
      py_code += ", verbose=True";
    }
    py_code += ", reducer=i.ReductionSingleton().reference())";
  }

  int idtype(0);
  if( type.startsWith("2") ) idtype = 1;
  //Disable buttons so that interaction is limited while processing data
  setProcessingState(true, idtype);
  m_lastreducetype = idtype;

  QString pythonStdOut = runReduceScriptFunction(py_code);

  //Reset the objects by initialising a new reducer object
  py_code = "i.ReductionSingleton().set_instrument(isis_instrument."+getInstrumentClass()+")";
  //restore the settings from the user file
  py_code += "\ni.ReductionSingleton().user_file_path='"+
    QFileInfo(m_uiForm.userfile_edit->text()).path() + "'";
  py_code += "\ni.ReductionSingleton().user_settings = _user_settings_copy";
  py_code += "\ni.ReductionSingleton().user_settings.execute(i.ReductionSingleton())";
  runReduceScriptFunction(py_code);

  if ( runMode == SingleMode )
  {
    QStringList pythonDiag = pythonStdOut.split(PYTHON_SEP);
    if ( pythonDiag.count() > 1 )
    {
      QString reducedWS = pythonDiag[1];
      reducedWS = reducedWS.split("\n")[0];
      resetDefaultOutput(reducedWS);
    }
  }

  // Mark that a reload is necessary to rerun the same reduction
  forceDataReload();
  //Reenable stuff
  setProcessingState(false, idtype);

  //If we used a temporary file in batch mode, remove it
  if( m_uiForm.batch_mode_btn->isChecked() && !m_tmp_batchfile.isEmpty() )
  {
    QFile tmp_file(m_tmp_batchfile);
    tmp_file.remove();
  }
  checkLogFlags();
}
/** Iterates through the validators and stops if it finds one that is shown and enabled
*  @return true if there are no validator problems if false if it finds one
*/
bool SANSRunWindow::entriesAreValid()
{
  typedef std::map<QLabel * const, std::pair<QWidget *, QWidget *> >::const_iterator it_type;
  for( it_type it(m_validators.begin()); it != m_validators.end(); ++it )
  {// is the validator active denoting a problem? don't do anything if it's been disabled
    if ( ( ! it->first->isHidden() ) && ( it->first->isEnabled() ) )
    {// the first in the pair is the widget whose value we're having a problem with
      it->second.first->setFocus();
      //the second part of the pair is the tab it's in
      m_uiForm.tabWidget->setCurrentWidget(it->second.second);
      return false;
    }
  }
  // no problems have been found
  return true;
}
/** Generates the code that can run a reduction chain (and then reset it)
*  @return Python code that can be passed to a Python interpreter
*/
QString SANSRunWindow::reduceSingleRun() const
{
  QString reducer_code;
  if ( m_uiForm.wav_dw_opt->currentText().toUpper().startsWith("RANGE") )
  {
    reducer_code += "\nreduced = i.CompWavRanges( ";
    reducer_code += "("+m_uiForm.wavRanges->text()+") ";
    reducer_code += ", plot=";
    reducer_code += m_uiForm.plot_check->isChecked() ? "True" : "False";
    reducer_code += ")";
  }
  else
  {
    reducer_code += "\nreduced = i.WavRangeReduction(full_trans_wav=";
    QString fullTransRang = m_uiForm.def_trans->isChecked() ? "True" : "False";
    reducer_code += fullTransRang + ")";
    if( m_uiForm.plot_check->isChecked() )
    {
      reducer_code += "\ni.PlotResult(reduced)";
    }
  }
  return reducer_code;
}

/** Returns the Python instrument class name to create for the current instrument
  @returns the Python class name corrosponding to the user selected instrument
*/
QString SANSRunWindow::getInstrumentClass() const
{
  QString instrum = m_uiForm.inst_opt->currentText();
  instrum = instrum.isEmpty() ? "LOQ" : instrum;
  return instrum + "()";
}
void SANSRunWindow::handleRunFindCentre()
{
  if( m_uiForm.beamstart_box->currentIndex() == 1 && (m_uiForm.beam_x->text().isEmpty() || m_uiForm.beam_y->text().isEmpty()) )
  {
    showInformationBox("Current centre postion is invalid, please check input.");
    return;
  }

  // Start iteration
  updateCentreFindingStatus("::SANS::Loading data");
  oldLoadButtonClick();

  // Disable interaction
  setProcessingState(true, 0);

  // This checks whether we have a sample run and that it has been loaded
  QString py_code = createOldAnalysisDetailsScript("1D");
  if( py_code.isEmpty() )
  {
    setProcessingState(false, 0);
    return;
  }

  if( m_uiForm.beam_rmin->text().isEmpty() )
  {
    m_uiForm.beam_rmin->setText("60");
  }

  if( m_uiForm.beam_rmax->text().isEmpty() )
  {
    if( m_uiForm.inst_opt->currentText() == "LOQ" )
    {
      m_uiForm.beam_rmax->setText("200");
    }
    else if( m_uiForm.inst_opt->currentText() == "SANS2D" )
    {
      m_uiForm.beam_rmax->setText("280");
    }
  }
  if( m_uiForm.beam_iter->text().isEmpty() )
  {
    m_uiForm.beam_iter->setText("15");
  }

  //Find centre function
  py_code += "SANSReduction.FindBeamCentre(rlow=" + m_uiForm.beam_rmin->text() + ",rupp=" + m_uiForm.beam_rmax->text() +
      ",MaxIter=" + m_uiForm.beam_iter->text() + ",";


  if( m_uiForm.beamstart_box->currentIndex() == 0 )
  {
    py_code += "xstart = None, ystart = None)";
  }
  else
  {
    py_code += "xstart=float(" + m_uiForm.beam_x->text() + ")/1000.,ystart=float(" + m_uiForm.beam_y->text() + ")/1000.)";
  }

  updateCentreFindingStatus("::SANS::Iteration 1");
  m_uiForm.beamstart_box->setFocus();

  //Execute the code
  //Connect up the logger to handle updating the centre finding status box
  connect(this, SIGNAL(logMessageReceived(const QString&)), this, 
	  SLOT(updateCentreFindingStatus(const QString&)));
  disconnect(this, SIGNAL(logMessageReceived(const QString&)), this, SLOT(updateLogWindow(const QString&)));
  
  runReduceScriptFunction(py_code);
  
  disconnect(this, SIGNAL(logMessageReceived(const QString&)), this, 
	     SLOT(updateCentreFindingStatus(const QString&)));
  connect(this, SIGNAL(logMessageReceived(const QString&)), this, SLOT(updateLogWindow(const QString&)));

  QString coordstr = runReduceScriptFunction("SANSReduction.printParameter('XBEAM_CENTRE');SANSReduction.printParameter('YBEAM_CENTRE')");
  
  QString result("");
  if( coordstr.isEmpty() )
  {
    result = "::SANS::No coordinates returned!";
  }
  else
  {
    //Remove all internal whitespace characters and replace with single space
    coordstr = coordstr.simplified();
    QStringList xycoords = coordstr.split(" ");
    if( xycoords.count() == 2 )
    {
      double coord = xycoords[0].toDouble();
      m_uiForm.beam_x->setText(QString::number(coord*1000.));
      coord = xycoords[1].toDouble();
      m_uiForm.beam_y->setText(QString::number(coord*1000.));
      result = "::SANS::Coordinates updated";
    }
    else
    {
      result = "::SANS::Incorrect number of parameters returned from function, check script.";

    }
  }
  updateCentreFindingStatus(result);
  
  //Reenable stuff
  setProcessingState(false, 0);
}
/** Save the output workspace from a single run reduction (i.e. the
*  workspace m_outputWS) in all the user selected formats
*/
void SANSRunWindow::handleDefSaveClick()
{
  const QString fileBase = m_uiForm.outfile_edit->text();
  if (fileBase.isEmpty())
  {
    QMessageBox::warning(this, "Filename required", "A filename must be entred into the text box above to save this file");
  }

  const QStringList algs(getSaveAlgs());
  QString saveCommand;
  for(QStringList::const_iterator alg = algs.begin(); alg != algs.end(); ++alg)
  {
    QString ext = SaveWorkspaces::getSaveAlgExt(*alg);
    QString fname = fileBase.endsWith(ext) ? fileBase : fileBase+ext;
    saveCommand += (*alg)+"('"+m_outputWS+"','"+fname+"')\n";
  }

  saveCommand += "print 'success'\n";
  QString result = runPythonCode(saveCommand).trimmed();
  if ( result != "success" )
  {
    QMessageBox::critical(this, "Error saving workspace", "Problem encountered saving workspace, does it still exist. There may be more information in the results console?");
  }

  runPythonCode(saveCommand);
}
/**
 * Set up controls based on the users selection in the combination box
 * @param new_index :: The new index that has been set
 */
void SANSRunWindow::handleWavComboChange(int new_index)
{
  QString userSel = m_uiForm.wav_dw_opt->itemText(new_index);

  if ( userSel.toUpper().contains("LOG") )
  {
    m_uiForm.wav_step_lbl->setText("dW / W");
  }
  else
  {
    m_uiForm.wav_step_lbl->setText("step");
  }

  if ( userSel.toUpper().startsWith("RANGE") )
  {
    m_uiForm.wav_stack->setCurrentIndex(1);
    m_uiForm.wavRanVal_lb->setEnabled(true);
  }
  else
  {
    m_uiForm.wav_stack->setCurrentIndex(0);
    m_uiForm.wavRanVal_lb->setEnabled(false);
  }
}
/**
 * A ComboBox option change
 * @param new_index :: The new index that has been set
 */
void SANSRunWindow::handleStepComboChange(int new_index)
{
  if( !sender() ) return;

  QString origin = sender()->objectName();
  if( origin.startsWith("q_dq") )
  {
    if( new_index == 0 ) 
    {
      m_uiForm.q_stack->setCurrentIndex(0);
      m_uiForm.q_step_lbl->setText("step");
    }
    else if( new_index == 1 ) 
    {
      m_uiForm.q_stack->setCurrentIndex(0);
      m_uiForm.q_step_lbl->setText("dQ / Q");
    }
    else 
    {
      m_uiForm.q_stack->setCurrentIndex(1);
    }
  } 
  else
  {
    if( new_index == 0 ) m_uiForm.qy_step_lbl->setText("XY step");
    else m_uiForm.qy_step_lbl->setText("dQ / Q");
  }

}

/**
 * Called when the show mask button has been clicked
 */
void SANSRunWindow::handleShowMaskButtonClick()
{
  QString analysis_script;
  addUserMaskStrings(analysis_script,"i.Mask",DefaultMask);
  analysis_script += "\ni.DisplayMask()";//analysis_script += "\ni.ReductionSingleton().ViewCurrentMask()";

  m_uiForm.showMaskBtn->setEnabled(false);
  m_uiForm.showMaskBtn->setText("Working...");

  runReduceScriptFunction(analysis_script);

  m_uiForm.showMaskBtn->setEnabled(true);
  m_uiForm.showMaskBtn->setText("Display mask");
}
/** Update the GUI and the Python objects with the instrument selection
 * @throw runtime_error if the instrument doesn't have exactly two detectors 
 */
void SANSRunWindow::handleInstrumentChange()
{
  //set up the required Python objects and delete what's out of date (perhaps everything is cleaned here)
  const QString instClass(getInstrumentClass());
  QString pyCode("if i.ReductionSingleton().get_instrument() != '");
  pyCode += m_uiForm.inst_opt->currentText()+"':";
  pyCode += "\n\ti.ReductionSingleton.clean(isis_reducer.ISISReducer)";
  pyCode += "\ni.ReductionSingleton().set_instrument(isis_instrument.";
  pyCode += instClass+")";
  runReduceScriptFunction(pyCode);

  //now update the GUI
  fillDetectNames(m_uiForm.detbank_sel);
  QString detect = runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.cur_detector().name()");
  int ind = m_uiForm.detbank_sel->findText(detect);  
  if( ind >= 0 && ind < 2 )
  {
    m_uiForm.detbank_sel->setCurrentIndex(ind);
  }

  m_uiForm.beam_rmin->setText("60");
  if( instClass == "LOQ()" )
  {
    m_uiForm.beam_rmax->setText("200");
    
    m_uiForm.geom_stack->setCurrentIndex(0);

  }
  else if ( instClass == "SANS2D()" )
  { 
    m_uiForm.beam_rmax->setText("280");

    m_uiForm.geom_stack->setCurrentIndex(1);

  }
  // flag that the user settings file needs to be loaded for this instrument
  m_cfg_loaded = false;
}
/** Record if the user has changed the default filename, because then we don't
*  change it
*/
void SANSRunWindow::setUserFname()
{
  m_userFname = true;
}
/**
 * Update the centre finding status label
 * @param msg :: The message string
 */
void SANSRunWindow::updateCentreFindingStatus(const QString & msg)
{
  static QString prefix = "::SANS";
  if( msg.startsWith(prefix) )
  {
    QStringList sections = msg.split("::");
    QString txt = sections.at(2);
    m_uiForm.centre_logging->append(txt);
    if( sections.at(1) == "SANSIter" )
    {
      m_uiForm.centre_stat->setText(txt);
    }
  }  
}
/** Enables or disables the floodFile run widget
*  @param state :: Qt::CheckState enum value, Checked means enable otherwise disabled
*/
void SANSRunWindow::prepareFlood(int state)
{
  m_uiForm.floodFile->setEnabled(state == Qt::Checked);
}
/**Enables  the default save button, saveDefault_Btn, if there is an output workspace
* stored in m_outputWS and text in outfile_edit
*/
void SANSRunWindow::enableOrDisableDefaultSave()
{
  if ( m_outputWS.isEmpty() )
  {//setEnabled(false) gets run below 
  }
  else if ( m_uiForm.outfile_edit->text().isEmpty() )
  {//setEnabled(false) gets run below 
  }
  else
  {//ensure that one format box is checked
    for(SavFormatsConstIt i=m_savFormats.begin(); i != m_savFormats.end(); ++i)
    {
      if (i.key()->isChecked())
      {
        m_uiForm.saveDefault_btn->setEnabled(true);
        return;
      }
    }
  }
  m_uiForm.saveDefault_btn->setEnabled(false);
}
/**
 * Update the logging window with status messages
 * @param msg :: The message received
 */
void SANSRunWindow::updateLogWindow(const QString & msg)
{
  static QString prefix = "::SANS";
  if( msg.startsWith(prefix) )
  {
    QString txt = msg.section("::",2);
    bool logwarnings = txt.contains("warning", Qt::CaseInsensitive);
    if( m_uiForm.verbose_check->isChecked() || logwarnings || m_uiForm.log_colette->isChecked() )
    {
      if( logwarnings )
      {
	m_log_warnings = true;
	m_uiForm.logging_field->setTextColor(Qt::red);
      }
      else
      {
	m_uiForm.logging_field->setTextColor(Qt::black);
      }
      m_uiForm.logging_field->append(txt);
    }
  }
}

/**
* Switch between run modes
* @param mode_id :: Indicates which toggle has been pressed
*/
void SANSRunWindow::switchMode(int mode_id)
{
  if( mode_id == SANSRunWindow::SingleMode )
  {
    m_uiForm.mode_stack->setCurrentIndex(0);
    m_uiForm.load_dataBtn->setEnabled(true);
  }
  else if( mode_id == SANSRunWindow::BatchMode )
  {
    m_uiForm.mode_stack->setCurrentIndex(1);
    m_uiForm.load_dataBtn->setEnabled(false);
  }
  else {}
}

/**
 * Paste to the batch table
 */
void SANSRunWindow::pasteToBatchTable()
{
  if( !m_cfg_loaded )
  {
    showInformationBox("Please load the relevant user file before continuing.");
    return;
  }

  QClipboard *clipboard = QApplication::clipboard();
  QString copied_text = clipboard->text();
  if( copied_text.isEmpty() ) return;
  
  QStringList runlines = copied_text.split("\n");
  QStringListIterator sitr(runlines);
  int errors(0);
  while( sitr.hasNext() )
  {
    QString line = sitr.next().simplified();
    if( !line.isEmpty() )
    {
      errors += addBatchLine(line);
    }
  }
  if( errors > 0 )
  {
    showInformationBox("Warning: " + QString::number(errors) + " malformed lines detected in pasted text. Lines skipped.");
  }
  if( m_uiForm.batch_table->rowCount() > 0 )
  {
    m_dirty_batch_grid = true;
    setProcessingState(false, -1);
  }
}

/**
 * Clear the batch table
 */
void SANSRunWindow::clearBatchTable()
{
  int row_count = m_uiForm.batch_table->rowCount();
  for( int i = row_count - 1; i >= 0; --i )
  {
    m_uiForm.batch_table->removeRow(i);
  }
  m_dirty_batch_grid = false;
  m_tmp_batchfile = "";
}

/**
 * Clear the logger field
 */
void SANSRunWindow::clearLogger()
{
  m_uiForm.logging_field->clear();
  m_uiForm.tabWidget->setTabText(4, "Logging");
}
/**Respond to the "Use default transmission" check box being clicked. If
 * the box is checked the transmission fit wavelength maximum and minimum
 * boxs with be set to the defaults for the instrument and disabled.
 * Otherwise they are enabled
 * @param state :: equal to Qt::Checked or not
 */
void SANSRunWindow::updateTransInfo(int state)
{
  if( state == Qt::Checked )
  {//"Use default transmission" means use the full range for the instrument
    m_uiForm.trans_min->setText(runReduceScriptFunction(
        "print i.ReductionSingleton().instrument.WAV_RANGE_MIN").trimmed());
    m_uiForm.trans_max->setText(runReduceScriptFunction(
        "print i.ReductionSingleton().instrument.WAV_RANGE_MAX").trimmed());
    m_uiForm.trans_min->setEnabled(false);
    m_uiForm.trans_max->setEnabled(false);
  }
  else
  {//use the user selected wavelengh range for the transmission calculation
    m_uiForm.trans_min->setEnabled(true);
    m_uiForm.trans_max->setEnabled(true);
  }
}
/** A slot to validate entries for Python lists and tupples
*/
void SANSRunWindow::checkList()
{
  // may be a need to generalise this
  QLineEdit *toValdate = m_uiForm.wavRanges;
  QLabel *validator = m_uiForm.wavRanVal_lb;
  const std::string input(toValdate->text().trimmed().toStdString());

  bool valid(false);
  Poco::StringTokenizer in(input, ",");
  try
  {
    for(Poco::StringTokenizer::Iterator i=in.begin(), end=in.end(); i!=end; ++i)
    {// try a lexical cast, we don't need its result only if there was an error
      boost::lexical_cast<double>(*i);
    }
    // there were no errors
    if ( ! input.empty() )
    {
      valid = true;
    }
  }
  catch (boost::bad_lexical_cast)
  {// there is a problem with the input somewhere
    valid = false;
  }
  
  if (valid)
  {
    validator->hide();
  }
  else
  {
    validator->show();
  }
}
/** Record the output workspace name, if there is no output
*  workspace pass an empty string or an empty argument list
*  @param wsName :: the name of the output workspace or empty for no output
*/
void SANSRunWindow::resetDefaultOutput(const QString & wsName)
{
  m_outputWS = wsName;
  enableOrDisableDefaultSave();

  if ( ! m_userFname )
  {
    m_uiForm.outfile_edit->setText(wsName);
  }
}
/** 
 * Run a SANS assign command
 * @param key :: The key of the edit box to assign from
 * @param logs :: An output parameter specifying the log data
 */
bool SANSRunWindow::runAssign(int key, QString & logs)
{
  //Work out if sans/trans and sample/can
  bool is_trans(false);
  if( key > 2 && key < 6 )
  {
    is_trans = true;
  }
  bool is_can(false);
  if( key == 1 || key == 4 )
  {
    is_can = true;
  }
  
  // Default extension if the box run number does not contain one
  QString extension = m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString();
  QString run_number = m_run_no_boxes.value(key)->text();
  if( QFileInfo(run_number).completeSuffix().isEmpty() )
  {
    if( run_number.endsWith(".") ) 
    {
      run_number.chop(1);
    }
    run_number += extension;
  }
  bool status(true);
  //need something to place between names printed by Python that won't be intepreted as the names or removed as white space
  const static QString PYTHON_SEP("C++runAssignC++");
  if( is_trans )
  {
    QString direct_run = m_run_no_boxes.value(key + 3)->text();
    QString direct_per = QString::number(getPeriod(key + 3));
    if( QFileInfo(direct_run).completeSuffix().isEmpty() )
    {
      if( direct_run.endsWith(".") ) 
      {
        direct_run.chop(1);
      }
      direct_run += extension;
    }
    QString assign_fn;
    if( is_can )
    {
      assign_fn = "i.TransmissionCan";
    }
    else
    {
      assign_fn = "i.TransmissionSample";
    }
    assign_fn += "('"+run_number+"','"+direct_run+"', reload = True";
    assign_fn += ", period_t = " + QString::number(getPeriod(key));
    assign_fn += ", period_d = " + direct_per+")";
    //assign the workspace name to a Python variable and read back some details
    QString pythonC="t1, t2 = " + assign_fn + ";print '"+PYTHON_SEP+"',t1,'"+PYTHON_SEP+"',t2";
    QString ws_names = runReduceScriptFunction(pythonC);
    if (ws_names.startsWith("error", Qt::CaseInsensitive))
    {
      throw std::runtime_error("Couldn't load a transmission file");
    }
    //read the informtion returned from Python
    QString trans_ws = ws_names.section(PYTHON_SEP, 1,1).trimmed();
    QString direct_ws = ws_names.section(PYTHON_SEP, 2).trimmed();

    status = ( ! trans_ws.isEmpty() ) && ( ! direct_ws.isEmpty() );

    //if the workspaces have loaded
    if (status)
    {//save the workspace names
      m_workspace_names.insert(key, trans_ws);
      m_workspace_names.insert(key + 3, direct_ws);

      //and display to the user how many periods are in the run
      QString pythonVar = is_can ? "i.ReductionSingleton().can_trans_load.TRANS_SAMPLE_N_PERIODS" : "i.ReductionSingleton().samp_trans_load.TRANS_SAMPLE_N_PERIODS";
      int nPeriods =
        runReduceScriptFunction("print " + pythonVar).toInt();
      setNumberPeriods(key, nPeriods);
      
      pythonVar = is_can ? "i.ReductionSingleton().can_trans_load.DIRECT_SAMPLE_N_PERIODS" : "i.ReductionSingleton().samp_trans_load.DIRECT_SAMPLE_N_PERIODS";
      nPeriods =
        runReduceScriptFunction("print " + pythonVar).toInt();
      setNumberPeriods(key + 3, nPeriods);
    }
    else
    {//workspaces didn't load so remove the (out of date) period information
      unSetPeriods(key);
      unSetPeriods(key+3);
    }
  }
  else
  {
    QString assign_fn;
    if( is_can )
    {
      assign_fn = "i.AssignCan";
    }
    else
    {
      assign_fn = "i.AssignSample";
    }
    assign_fn += "('" + run_number + "', reload = True";
    assign_fn += ", period = " + QString::number(getPeriod(key))+")";
    //assign the workspace name to a Python variable and read back some details
    QString run_info = "SCATTER_SAMPLE, logvalues = " + assign_fn + ";print '"+PYTHON_SEP+"',SCATTER_SAMPLE,'"+PYTHON_SEP+"',logvalues";
    run_info = runReduceScriptFunction(run_info);
    if (run_info.startsWith("error", Qt::CaseInsensitive))
    {
      throw std::runtime_error("Couldn't sample or can");
    }
    //read the informtion returned from Python
    QString base_workspace = run_info.section(PYTHON_SEP, 1, 1).trimmed();

    logs = run_info.section(PYTHON_SEP, 2);
    if( !logs.isEmpty() )
    {
      trimPyMarkers(logs);
    }
    status = ! base_workspace.isEmpty();
    
    //if the workspace was loaded
    if (status)
    {//save the workspace name
      m_workspace_names.insert(key, base_workspace);
      //and display to the user how many periods are in the run
      QString pythonVar = is_can ? "i.ReductionSingleton().background_subtracter._CAN_N_PERIODS" : "i.ReductionSingleton().data_loader._SAMPLE_N_PERIODS";
      int nPeriods =
        runReduceScriptFunction("print "+pythonVar).toInt();
      setNumberPeriods(key, nPeriods);
    }
    else
    {
      unSetPeriods(key);
    }
  }
  return status;
}
/**
 * Run a SANS assign command
 * @param key :: The key of the edit box to assign from
 * @param logs :: An output parameter specifying the log data
 */
bool SANSRunWindow::oldAssign(int key, QString & logs)
{
  //Work out if sans/trans and sample/can
  bool is_trans(false);
  if( key > 2 && key < 6 )
  {
    is_trans = true;
  }
  bool is_can(false);
  if( key == 1 || key == 4 )
  {
    is_can = true;
  }

  // Default extension if the box run number does not contain one
  QString extension = m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString();
  QString run_number = m_run_no_boxes.value(key)->text();
  if( QFileInfo(run_number).completeSuffix().isEmpty() )
  {
    if( run_number.endsWith(".") )
    {
      run_number.chop(1);
    }
    run_number += extension;
  }
  bool status(true);
  //need something to place between names printed by Python that won't be intepreted as the names or removed as white space
  const static QString PYTHON_SEP("C++runAssignC++");
  if( is_trans )
  {
    QString direct_run = m_run_no_boxes.value(key + 3)->text();
    if( QFileInfo(direct_run).completeSuffix().isEmpty() )
    {
      if( direct_run.endsWith(".") )
      {
        direct_run.chop(1);
      }
      direct_run += extension;
    }
    QString assign_fn;
    if( is_can )
    {
      assign_fn = "SANSReduction.TransmissionCan";
    }
    else
    {
      assign_fn = "SANSReduction.TransmissionSample";
    }
    assign_fn += "('"+run_number+"','"+direct_run+"', reload = True";
    assign_fn += ", period = " + QString::number(getPeriod(key))+")";
    //assign the workspace name to a Python variable and read back some details
    QString pythonC="t1, t2 = " + assign_fn + ";print t1,'"+PYTHON_SEP+"',t2";
    QString ws_names = runReduceScriptFunction(pythonC);
    if (ws_names.startsWith("error", Qt::CaseInsensitive))
    {
      throw std::runtime_error("Couldn't load a transmission file");
    }
    //read the informtion returned from Python
    QString trans_ws = ws_names.section(PYTHON_SEP, 0,0).trimmed();
    QString direct_ws = ws_names.section(PYTHON_SEP, 1).trimmed();

    status = ( ! trans_ws.isEmpty() ) && ( ! direct_ws.isEmpty() );

    //if the workspaces have loaded
    if (status)
    {//save the workspace names
      m_workspace_names.insert(key, trans_ws);
      m_workspace_names.insert(key + 3, direct_ws);

      //and display to the user how many periods are in the run
      QString pythonVar = is_can ? "TRANS_CAN_N_PERIODS" : "_TRANS_SAMPLE_N_PERIODS";
      int nPeriods =
        runReduceScriptFunction("SANSReduction.printParameter('"+pythonVar+"'),").toInt();
      setNumberPeriods(key, nPeriods);

      pythonVar = is_can ? "DIRECT_CAN_N_PERIODS" : "DIRECT_SAMPLE_N_PERIODS";
      nPeriods =
        runReduceScriptFunction("SANSReduction.printParameter('"+pythonVar+"'),").toInt();
      setNumberPeriods(key + 3, nPeriods);
    }
    else
    {//workspaces didn't load so remove the (out of date) period information
      unSetPeriods(key);
      unSetPeriods(key+3);
    }
  }
  else
  {
    QString assign_fn;
    if( is_can )
    {
      assign_fn = "SANSReduction.AssignCan";
    }
    else
    {
      assign_fn = "SANSReduction.AssignSample";
    }
    assign_fn += "('" + run_number + "', reload = True";
    assign_fn += ", period = " + QString::number(getPeriod(key)) + ")";
    //assign the workspace name to a Python variable and read back some details
    QString run_info = "SCATTER_SAMPLE, logvalues = " + assign_fn + ";print SCATTER_SAMPLE,'"+PYTHON_SEP+"',logvalues";
    run_info = runReduceScriptFunction(run_info);
    if (run_info.startsWith("error", Qt::CaseInsensitive))
    {
      throw std::runtime_error("Couldn't sample or can");
    }
    //read the informtion returned from Python
    QString base_workspace = run_info.section(PYTHON_SEP, 0, 0).trimmed();

    logs = run_info.section(PYTHON_SEP, 1);
    if( !logs.isEmpty() )
    {
      trimPyMarkers(logs);
    }
    status = ! base_workspace.isEmpty();

    //if the workspace was loaded
    if (status)
    {//save the workspace name
      m_workspace_names.insert(key, base_workspace);
      //and display to the user how many periods are in the run
      QString pythonVar = is_can ? "_CAN_N_PERIODS" : "_SAMPLE_N_PERIODS";
      int nPeriods =
        runReduceScriptFunction("SANSReduction.printParameter('"+pythonVar+"'),").toInt();
      setNumberPeriods(key, nPeriods);
    }
    else
    {
      unSetPeriods(key);
    }
  }
  return status;
}
/** Gets the detectors that the instrument has and fills the
*  combination box with these, there must exactly two detectors
*  @parma output[out] this combination box will be cleared and filled with the new names
*  @throw runtime_error if there aren't exactly two detectors 
*/
void SANSRunWindow::fillDetectNames(QComboBox *output)
{
  QString detsTuple = runReduceScriptFunction(
    "print i.ReductionSingleton().instrument.listDetectors()");

  if (detsTuple.isEmpty())
  {//this happens if the run Python signal hasn't yet been connected
    return;
  }

  QStringList dets = detsTuple.split("'", QString::SkipEmptyParts);
  // the tuple will be of the form ('det1', 'det2'), hence the split should return 5 parts
  if ( dets.count() != 5 )
  {
    QMessageBox::critical(this, "Can't Load Instrument", "The instrument must have only 2 detectors. Can't proceed with this instrument");
    throw std::runtime_error("Invalid instrument setting, you should be able to continue by selecting a valid instrument");
  }
  
  output->setItemText(0, dets[1]);
  output->setItemText(1, dets[3]);
}
/** gets the number entered into the periods box
* @param key :: The box this applies to
* @return the entry number the user entered into the box, or -1 if the box is empty
*/
int SANSRunWindow::getPeriod(const int key)
{
  QLabel *label = qobject_cast<QLabel*>(m_period_lbls.value(key));
  QLineEdit *userentry = qobject_cast<QLineEdit*>(label->buddy());

  if ( ! userentry->text().isEmpty() )
  {
    return userentry->text().toInt();
  }
  else
  {
    return -1;
  }
}
/** Set number of periods for the given workspace
* @param key :: The box this applies to
* @param num :: the number of periods there are known to be
*/
void SANSRunWindow::setNumberPeriods(const int key, const int num)
{
  QLabel *label = qobject_cast<QLabel*>(m_period_lbls.value(key));
  QLineEdit *userentry = qobject_cast<QLineEdit*>(label->buddy());

  if (num > 0)
  {
    label->setText("/" + QString::number(num));
    if (userentry->text().isEmpty())
    {//default period to analysis is the first one
      userentry->setText("1");
    }
  }
  else
  {
    userentry->clear();
    label->setText("/??");
  }
}
/** Blank the periods information in a box
* @param key :: The box this applies to
*/
void SANSRunWindow::unSetPeriods(const int key)
{
  setNumberPeriods(key, -1);
}
/** Checks if the workspace is a group and returns the first member of group, throws
*  if nothing can be retrived
*  @param[in] workspace the group to examine
*  @param[in] member entry or period number of the requested workspace, these start at 1
*  @return the first member of the passed group
*  @throw NotFoundError if a workspace can't be returned
*/
Mantid::API::MatrixWorkspace_sptr SANSRunWindow::getGroupMember(Mantid::API::Workspace_const_sptr in, const int member) const
{
  Mantid::API::WorkspaceGroup_const_sptr group =
    boost::dynamic_pointer_cast<const Mantid::API::WorkspaceGroup>(in);
  if ( ! group )
  {
    throw Mantid::Kernel::Exception::NotFoundError("Problem retrieving workspace ", in->getName());
  }
  
  const std::vector<std::string> gNames = group->getNames();
  //currently the names array starts with the name of the group
  if ( static_cast<int>(gNames.size()) < member + 1 )
  {
    throw Mantid::Kernel::Exception::NotFoundError("Workspace group" + in->getName() + " doesn't have " + boost::lexical_cast<std::string>(member) + " entries", member);
  }
  //throws NotFoundError if the workspace couldn't be found
  Mantid::API::Workspace_sptr base = Mantid::API::AnalysisDataService::Instance().retrieve(gNames[member]);
  Mantid::API::MatrixWorkspace_sptr memberWS =
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(base);
  if ( ! memberWS )
  {
    throw Mantid::Kernel::Exception::NotFoundError("Problem getting period number " + boost::lexical_cast<std::string>(member) + " from group workspace " + base->getName(), member);
  }
  
  return memberWS;
}
/**
 * Get a properly qualified workspace name for the given key
 */
QString SANSRunWindow::getWorkspaceName(int key)
{
  return m_workspace_names.value(key);
}
/** Find which save formats have been selected by the user 
*  @return save algorithm names
*/
QStringList SANSRunWindow::getSaveAlgs()
{
  QStringList checked;
  for(SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end(); ++i)
  {//the key is the check box
    if (i.key()->isChecked())
    {// and value() is the name of the algorithm associated with that checkbox
      checked.append(i.value());
    }
  }
  return checked;
}
/**
 * Handle a delete notification from Mantid
 * @param p_dnf :: A Mantid delete notification
 */
void SANSRunWindow::handleMantidDeleteWorkspace(Mantid::API::WorkspaceDeleteNotification_ptr p_dnf)
{
  QString wksp_name = QString::fromStdString(p_dnf->object_name());
  int names_count = m_workspace_names.count();
  for( int key = 0; key < names_count; ++key )
  {
    if( wksp_name == m_workspace_names.value(key) )
    {
      forceDataReload();
      return;
    }
  }
}

/**
 * Format a double as a string
 * @param value :: The double to convert to a string
 * @param colour :: The colour
 * @param format :: The format char
 * @param precision :: The precision
 */
QString SANSRunWindow::formatDouble(double value, const QString & colour, char format, int precision)
{
  return QString("<font color='") + colour + QString("'>") + QString::number(value, format, precision)  + QString("</font>");
}

/**
 * Raise a message if current status allows
 * @param msg :: The message to include in the box
 * @param index :: The tab index to set as current
*/
void SANSRunWindow::raiseOneTimeMessage(const QString & msg, int index)
{
  if( m_warnings_issued ) return;
  if( index >= 0 )
  {
    m_uiForm.tabWidget->setCurrentIndex(index);
  }
  showInformationBox(msg);
  m_warnings_issued = true;
}


/**
 * Rest the geometry details box 
 */
void SANSRunWindow::resetGeometryDetailsBox()
{
  QString blank("-");
  //LOQ
  m_uiForm.dist_mod_mon->setText(blank);

  //SANS2D
  m_uiForm.dist_mon_s2d->setText(blank);
  m_uiForm.dist_sample_ms_s2d->setText(blank);
  m_uiForm.dist_can_ms_s2d->setText(blank);
  m_uiForm.dist_bkgd_ms_s2d->setText(blank);

  for(int i = 0; i < 3; ++i )
  {
    //LOQ
    QMutableHashIterator<QString,QLabel*> litr(m_loq_detlabels[i]);
    while(litr.hasNext())
    {
      litr.next();
      litr.value()->setText(blank);
    }
    //SANS2D
    QMutableHashIterator<QString,QLabel*> sitr(m_s2d_detlabels[i]);
    while(sitr.hasNext())
    {
      sitr.next();
      sitr.value()->setText(blank);
    }
  }
  
}

void SANSRunWindow::cleanup()
{
  Mantid::API::AnalysisDataServiceImpl & ads = Mantid::API::AnalysisDataService::Instance();
  std::set<std::string> workspaces = ads.getObjectNames();
  std::set<std::string>::const_iterator iend = workspaces.end();
  for( std::set<std::string>::const_iterator itr = workspaces.begin(); itr != iend; ++itr )
  {
    QString name = QString::fromStdString(*itr);
    if( name.endsWith("_raw") || name.endsWith("_nxs"))
    {
      ads.remove(*itr);
    }
  }
}

/**
 * Add a csv line to the batch grid
 * @param csv_line :: Add a line of csv text to the grid 
 * @param separator :: An optional separator, default = ","
*/
int SANSRunWindow::addBatchLine(QString csv_line, QString separator)
{
  //Try to detect separator if one is not specified
  if( separator.isEmpty() )
  {
    if( csv_line.contains(",") )
    {
      separator = ",";
    }
    else
    {
      separator = " ";
    }
  }
  QStringList elements = csv_line.split(separator);
  //Insert new row
  int row = m_uiForm.batch_table->rowCount();
  m_uiForm.batch_table->insertRow(row);

  int nelements = elements.count() - 1;
  bool error(false);
  for( int i = 0; i < nelements; )
  {
    QString cola = elements.value(i);
    QString colb = elements.value(i+1);
    if( m_allowed_batchtags.contains(cola) )
    {
      if( !m_allowed_batchtags.contains(colb) )
      {
        if( !colb.isEmpty() && !cola.contains("background") )
        {
          m_uiForm.batch_table->setItem(row, m_allowed_batchtags.value(cola), new QTableWidgetItem(colb));
        }
        i += 2;        
      }
      else
      {
        ++i;
      }
    }
    else
    {
      error = true;
      break;
    }
  }
  if( error ) 
  {
    m_uiForm.batch_table->removeRow(row);
    return 1;
  }
  return 0;
}

/**
 * Save the batch file to a CSV file.
 * @param filename :: An optional filename. If none is given then a temporary file is used and its name returned 
*/
QString SANSRunWindow::saveBatchGrid(const QString & filename)
{
  QString csv_filename = filename;
  if( csv_filename.isEmpty() )
  {
    //Generate a temporary filename
    QTemporaryFile tmp;
    tmp.open();
    csv_filename = tmp.fileName();
    tmp.close();
    m_tmp_batchfile = csv_filename;
  }

  QFile csv_file(csv_filename);
  if( !csv_file.open(QIODevice::WriteOnly|QIODevice::Text) )
  {
    showInformationBox("Error: Cannot write to CSV file \"" + csv_filename + "\".");
    return "";
  }
  
  QTextStream out_strm(&csv_file);
  int nrows = m_uiForm.batch_table->rowCount();
  const QString separator(",");
  for( int r = 0; r < nrows; ++r )
  {
    for( int c = 0; c < 7; ++c )
    {
      out_strm << m_allowed_batchtags.key(c) << separator;
      if( QTableWidgetItem* item = m_uiForm.batch_table->item(r, c) )
      {
        out_strm << item->text();
      }
      if( c < 6 ) out_strm << separator; 
    }
    out_strm << "\n";
  }
  csv_file.close();
  if( !filename.isEmpty() )
  {
    m_tmp_batchfile = "";
    m_dirty_batch_grid = false;
    m_uiForm.csv_filename->setText(csv_filename);
  }
  else
  {
     m_uiForm.csv_filename->clear();
  }
  return csv_filename;
}

void SANSRunWindow::checkLogFlags()
{
  if( m_log_warnings )
  {
    m_uiForm.tabWidget->setTabText(4, "Logging - WARNINGS");
    //    m_uiForm.tabWidget->tabBar()->setTabTextColor(4, QColor("red"));
  }
  m_log_warnings = false;
}
/** Display the first data search and the number of data directorys to users and
*  update our input directory in m_data_dir
*/
void SANSRunWindow::upDateDataDir()
{
  const std::vector<std::string> &dirs
    = ConfigService::Instance().getDataSearchDirs();
  if ( ! dirs.empty() )
  {// use the first directory in the list
    QString dataDir = QString::fromStdString(dirs.front());
    //check for windows and its annoying path separator thing, windows' paths can't contain /
    if ( dataDir.contains('\\') && ! dataDir.contains('/') )
    {
      dataDir.replace('\\', '/');
    }
    m_uiForm.saveDir_lb->setText(dataDir);

    m_uiForm.plusDirs_lb->setText(
      QString("+ ") + QString::number(dirs.size()-1) + QString(" others"));
  }
  else
  {
    m_uiForm.saveDir_lb->setText("No input search directories defined");
    m_uiForm.plusDirs_lb->setText("");
  }

}
/** Update the input directory labels if the Mantid system input
*  directories have changed
*  @param pDirInfo :: a pointer to an object with the output directory name in it
*/
void SANSRunWindow::handleInputDirChange(Mantid::Kernel::ConfigValChangeNotification_ptr pDirInfo)
{
  if ( pDirInfo->key() == "datasearch.directories" )
  {
    upDateDataDir();
  }
}

} //namespace CustomInterfaces

} //namespace MantidQt


