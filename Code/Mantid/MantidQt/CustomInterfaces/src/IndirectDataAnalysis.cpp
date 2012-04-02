//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/IndirectDataAnalysis.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"

#include <QValidator>
#include <QIntValidator>
#include <QDoubleValidator>

#include <QLineEdit>
#include <QFileInfo>
#include <QMenu>
#include <QTreeWidget>

#include <QDesktopServices>
#include <QUrl>

#include <QPalette>
#include <QColor>
#include <QApplication>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#include <QtCheckBoxFactory>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{
//Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(IndirectDataAnalysis);

IndirectDataAnalysis::IndirectDataAnalysis(QWidget *parent) :
  UserSubWindow(parent), m_nDec(6), m_valInt(NULL), m_valDbl(NULL), 
  m_elwPlot(NULL), m_elwR1(NULL), m_elwR2(NULL), m_elwDataCurve(NULL),
  m_msdPlot(NULL), m_msdRange(NULL), m_msdDataCurve(NULL), m_msdTree(NULL), m_msdDblMng(NULL),
  m_furPlot(NULL), m_furRange(NULL), m_furCurve(NULL), m_furTree(NULL), m_furDblMng(NULL),
  m_furyResFileType(true),
  m_ffDataCurve(NULL), m_ffFitCurve(NULL),
  m_cfDataCurve(NULL), m_cfCalcCurve(NULL),
  m_changeObserver(*this, &IndirectDataAnalysis::handleDirectoryChange)
{
}

void IndirectDataAnalysis::closeEvent(QCloseEvent*)
{
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

void IndirectDataAnalysis::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();
  // std::string preValue = pNf->preValue();  // Unused
  // std::string curValue = pNf->curValue();  // Unused

  if ( key == "defaultsave.directory" )
  {
    loadSettings();
  }
}

void IndirectDataAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // create validators
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);
  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory();
  m_blnEdFac = new QtCheckBoxFactory();

  m_stringManager = new QtStringPropertyManager();

  setupElwin();
  setupMsd();
  setupFury();
  setupFuryFit();
  setupConFit();
  setupAbsorptionF2Py();
  setupAbsCor();

  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));
}

void IndirectDataAnalysis::initLocalPython()
{
  QString pyInput = "from mantidsimple import *";
  QString pyOutput = runPythonCode(pyInput).trimmed();
  loadSettings();
}

void IndirectDataAnalysis::loadSettings()
{
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);
  m_uiForm.elwin_inputFile->readSettings(settings.group());
  m_uiForm.msd_inputFile->readSettings(settings.group());
  m_uiForm.fury_iconFile->readSettings(settings.group());
  m_uiForm.fury_resFile->readSettings(settings.group());
  m_uiForm.furyfit_inputFile->readSettings(settings.group());
  m_uiForm.confit_inputFile->readSettings(settings.group());
  m_uiForm.confit_resInput->readSettings(settings.group());
  m_uiForm.absp_inputFile->readSettings(settings.group());
  m_uiForm.abscor_sample->readSettings(settings.group());
  m_uiForm.abscor_can->readSettings(settings.group());
  settings.endGroup();
}

void IndirectDataAnalysis::setupElwin()
{
  // Create QtTreePropertyBrowser object
  m_elwTree = new QtTreePropertyBrowser();
  m_uiForm.elwin_properties->addWidget(m_elwTree);

  // Create Manager Objects
  m_elwDblMng = new QtDoublePropertyManager();
  m_elwBlnMng = new QtBoolPropertyManager();
  m_elwGrpMng = new QtGroupPropertyManager();

  // Editor Factories
  m_elwTree->setFactoryForManager(m_elwDblMng, m_dblEdFac);
  m_elwTree->setFactoryForManager(m_elwBlnMng, m_blnEdFac);

  // Create Properties
  m_elwProp["R1S"] = m_elwDblMng->addProperty("Start");
  m_elwDblMng->setDecimals(m_elwProp["R1S"], m_nDec);
  m_elwProp["R1E"] = m_elwDblMng->addProperty("End");
  m_elwDblMng->setDecimals(m_elwProp["R1E"], m_nDec);  
  m_elwProp["R2S"] = m_elwDblMng->addProperty("Start");
  m_elwDblMng->setDecimals(m_elwProp["R2S"], m_nDec);
  m_elwProp["R2E"] = m_elwDblMng->addProperty("End");
  m_elwDblMng->setDecimals(m_elwProp["R2E"], m_nDec);

  m_elwProp["UseTwoRanges"] = m_elwBlnMng->addProperty("Use Two Ranges");

  m_elwProp["Range1"] = m_elwGrpMng->addProperty("Range One");
  m_elwProp["Range1"]->addSubProperty(m_elwProp["R1S"]);
  m_elwProp["Range1"]->addSubProperty(m_elwProp["R1E"]);
  m_elwProp["Range2"] = m_elwGrpMng->addProperty("Range Two");
  m_elwProp["Range2"]->addSubProperty(m_elwProp["R2S"]);
  m_elwProp["Range2"]->addSubProperty(m_elwProp["R2E"]);

  m_elwTree->addProperty(m_elwProp["Range1"]);
  m_elwTree->addProperty(m_elwProp["UseTwoRanges"]);
  m_elwTree->addProperty(m_elwProp["Range2"]);

  // Create Slice Plot Widget for Range Selection
  m_elwPlot = new QwtPlot(this);
  m_elwPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_elwPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.elwin_plot->addWidget(m_elwPlot);
  m_elwPlot->setCanvasBackground(Qt::white);
  // We always want one range selector... the second one can be controlled from
  // within the elwinTwoRanges(bool state) function
  m_elwR1 = new MantidWidgets::RangeSelector(m_elwPlot);
  connect(m_elwR1, SIGNAL(minValueChanged(double)), this, SLOT(elwinMinChanged(double)));
  connect(m_elwR1, SIGNAL(maxValueChanged(double)), this, SLOT(elwinMaxChanged(double)));
  // create the second range
  m_elwR2 = new MantidWidgets::RangeSelector(m_elwPlot);
  m_elwR2->setColour(Qt::darkGreen); // dark green for background
  connect(m_elwR1, SIGNAL(rangeChanged(double, double)), m_elwR2, SLOT(setRange(double, double)));
  connect(m_elwR2, SIGNAL(minValueChanged(double)), this, SLOT(elwinMinChanged(double)));
  connect(m_elwR2, SIGNAL(maxValueChanged(double)), this, SLOT(elwinMaxChanged(double)));
  m_elwR2->setRange(m_elwR1->getRange());
  // Refresh the plot window
  m_elwPlot->replot();
  
  connect(m_elwDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(elwinUpdateRS(QtProperty*, double)));
  connect(m_elwBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(elwinTwoRanges(QtProperty*, bool)));
  elwinTwoRanges(0, false);

  // m_uiForm element signals and slots
  connect(m_uiForm.elwin_pbPlotInput, SIGNAL(clicked()), this, SLOT(elwinPlotInput()));

  // Set any default values
  m_elwDblMng->setValue(m_elwProp["R1S"], -0.02);
  m_elwDblMng->setValue(m_elwProp["R1E"], 0.02);
}

void IndirectDataAnalysis::setupMsd()
{
  // Tree Browser
  m_msdTree = new QtTreePropertyBrowser();
  m_uiForm.msd_properties->addWidget(m_msdTree);

  m_msdDblMng = new QtDoublePropertyManager();

  m_msdTree->setFactoryForManager(m_msdDblMng, m_dblEdFac);

  m_msdProp["Start"] = m_msdDblMng->addProperty("StartX");
  m_msdDblMng->setDecimals(m_msdProp["Start"], m_nDec);
  m_msdProp["End"] = m_msdDblMng->addProperty("EndX");
  m_msdDblMng->setDecimals(m_msdProp["End"], m_nDec);

  m_msdTree->addProperty(m_msdProp["Start"]);
  m_msdTree->addProperty(m_msdProp["End"]);

  m_msdPlot = new QwtPlot(this);
  m_uiForm.msd_plot->addWidget(m_msdPlot);

  // Cosmetics
  m_msdPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_msdPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_msdPlot->setCanvasBackground(Qt::white);

  m_msdRange = new MantidWidgets::RangeSelector(m_msdPlot);

  connect(m_msdRange, SIGNAL(minValueChanged(double)), this, SLOT(msdMinChanged(double)));
  connect(m_msdRange, SIGNAL(maxValueChanged(double)), this, SLOT(msdMaxChanged(double)));
  connect(m_msdDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(msdUpdateRS(QtProperty*, double)));

  connect(m_uiForm.msd_pbPlotInput, SIGNAL(clicked()), this, SLOT(msdPlotInput()));
}

void IndirectDataAnalysis::setupFury()
{
  m_furTree = new QtTreePropertyBrowser();
  m_uiForm.fury_TreeSpace->addWidget(m_furTree);

  m_furDblMng = new QtDoublePropertyManager();

  m_furPlot = new QwtPlot(this);
  m_uiForm.fury_PlotSpace->addWidget(m_furPlot);
  m_furPlot->setCanvasBackground(Qt::white);
  m_furPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_furPlot->setAxisFont(QwtPlot::yLeft, this->font());

  m_furProp["ELow"] = m_furDblMng->addProperty("ELow");
  m_furDblMng->setDecimals(m_furProp["ELow"], m_nDec);
  m_furProp["EWidth"] = m_furDblMng->addProperty("EWidth");
  m_furDblMng->setDecimals(m_furProp["EWidth"], m_nDec);
  m_furProp["EHigh"] = m_furDblMng->addProperty("EHigh");
  m_furDblMng->setDecimals(m_furProp["EHigh"], m_nDec);

  m_furTree->addProperty(m_furProp["ELow"]);
  m_furTree->addProperty(m_furProp["EWidth"]);
  m_furTree->addProperty(m_furProp["EHigh"]);

  m_furTree->setFactoryForManager(m_furDblMng, m_dblEdFac);

  m_furRange = new MantidQt::MantidWidgets::RangeSelector(m_furPlot);

  // signals / slots & validators
  connect(m_furRange, SIGNAL(minValueChanged(double)), this, SLOT(furyMinChanged(double)));
  connect(m_furRange, SIGNAL(maxValueChanged(double)), this, SLOT(furyMaxChanged(double)));
  connect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(furyUpdateRS(QtProperty*, double)));
  
  connect(m_uiForm.fury_cbInputType, SIGNAL(currentIndexChanged(int)), m_uiForm.fury_swInput, SLOT(setCurrentIndex(int)));  
  connect(m_uiForm.fury_cbResType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(furyResType(const QString&)));
  connect(m_uiForm.fury_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyPlotInput()));
}

void IndirectDataAnalysis::setupFuryFit()
{
  m_ffTree = new QtTreePropertyBrowser();
  m_uiForm.furyfit_properties->addWidget(m_ffTree);
  
  // Setup FuryFit Plot Window
  m_ffPlot = new QwtPlot(this);
  m_ffPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_ffPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.furyfit_vlPlot->addWidget(m_ffPlot);
  m_ffPlot->setCanvasBackground(QColor(255,255,255));
  
  m_ffRangeS = new MantidQt::MantidWidgets::RangeSelector(m_ffPlot);
  connect(m_ffRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitXMinSelected(double)));
  connect(m_ffRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(furyfitXMaxSelected(double)));

  m_ffBackRangeS = new MantidQt::MantidWidgets::RangeSelector(m_ffPlot,
    MantidQt::MantidWidgets::RangeSelector::YSINGLE);
  m_ffBackRangeS->setRange(0.0,1.0);
  m_ffBackRangeS->setColour(Qt::darkGreen);
  connect(m_ffBackRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitBackgroundSelected(double)));

  // setupTreePropertyBrowser
  m_groupManager = new QtGroupPropertyManager();
  m_ffDblMng = new QtDoublePropertyManager();
  m_ffRangeManager = new QtDoublePropertyManager();
  
  m_ffTree->setFactoryForManager(m_ffDblMng, m_dblEdFac);
  m_ffTree->setFactoryForManager(m_ffRangeManager, m_dblEdFac);

  m_ffProp["StartX"] = m_ffRangeManager->addProperty("StartX");
  m_ffRangeManager->setDecimals(m_ffProp["StartX"], m_nDec);
  m_ffProp["EndX"] = m_ffRangeManager->addProperty("EndX");
  m_ffRangeManager->setDecimals(m_ffProp["EndX"], m_nDec);

  connect(m_ffRangeManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(furyfitRangePropChanged(QtProperty*, double)));

  m_ffProp["LinearBackground"] = m_groupManager->addProperty("LinearBackground");
  m_ffProp["BackgroundA0"] = m_ffRangeManager->addProperty("A0");
  m_ffRangeManager->setDecimals(m_ffProp["BackgroundA0"], m_nDec);
  m_ffProp["LinearBackground"]->addSubProperty(m_ffProp["BackgroundA0"]);

  m_ffProp["Exponential1"] = createExponential("Exponential 1");
  m_ffProp["Exponential2"] = createExponential("Exponential 2");
  
  m_ffProp["StretchedExp"] = createStretchedExp("Stretched Exponential");

  furyfitTypeSelection(m_uiForm.furyfit_cbFitType->currentIndex());

  // Connect to PlotGuess checkbox
  connect(m_ffDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(furyfitPlotGuess(QtProperty*)));

  // Signal/slot ui connections
  connect(m_uiForm.furyfit_inputFile, SIGNAL(fileEditingFinished()), this, SLOT(furyfitPlotInput()));
  connect(m_uiForm.furyfit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(furyfitTypeSelection(int)));
  connect(m_uiForm.furyfit_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyfitPlotInput()));
  connect(m_uiForm.furyfit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(furyfitPlotInput()));
  connect(m_uiForm.furyfit_cbInputType, SIGNAL(currentIndexChanged(int)), m_uiForm.furyfit_swInput, SLOT(setCurrentIndex(int)));  
  connect(m_uiForm.furyfit_pbSeqFit, SIGNAL(clicked()), this, SLOT(furyfitSequential()));
  // apply validators - furyfit
  m_uiForm.furyfit_leSpecNo->setValidator(m_valInt);

  // Set a custom handler for the QTreePropertyBrowser's ContextMenu event
  m_ffTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ffTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));
}

void IndirectDataAnalysis::setupConFit()
{
  // Create Property Managers
  m_cfGrpMng = new QtGroupPropertyManager();
  m_cfBlnMng = new QtBoolPropertyManager();
  m_cfDblMng = new QtDoublePropertyManager();

  // Create TreeProperty Widget
  m_cfTree = new QtTreePropertyBrowser();
  m_uiForm.confit_properties->addWidget(m_cfTree);

  // add factories to managers
  m_cfTree->setFactoryForManager(m_cfBlnMng, m_blnEdFac);
  m_cfTree->setFactoryForManager(m_cfDblMng, m_dblEdFac);

  // Create Plot Widget
  m_cfPlot = new QwtPlot(this);
  m_cfPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_cfPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_cfPlot->setCanvasBackground(Qt::white);
  m_uiForm.confit_plot->addWidget(m_cfPlot);

  // Create Range Selectors
  m_cfRangeS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot);
  m_cfBackgS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot, 
    MantidQt::MantidWidgets::RangeSelector::YSINGLE);
  m_cfBackgS->setColour(Qt::darkGreen);
  m_cfBackgS->setRange(0.0, 1.0);
  m_cfHwhmRange = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot);
  m_cfHwhmRange->setColour(Qt::red);

  // Populate Property Widget
  m_cfProp["FitRange"] = m_cfGrpMng->addProperty("Fitting Range");
  m_cfProp["StartX"] = m_cfDblMng->addProperty("StartX");
  m_cfDblMng->setDecimals(m_cfProp["StartX"], m_nDec);
  m_cfProp["EndX"] = m_cfDblMng->addProperty("EndX");
  m_cfDblMng->setDecimals(m_cfProp["EndX"], m_nDec);
  m_cfProp["FitRange"]->addSubProperty(m_cfProp["StartX"]);
  m_cfProp["FitRange"]->addSubProperty(m_cfProp["EndX"]);
  m_cfTree->addProperty(m_cfProp["FitRange"]);

  m_cfProp["LinearBackground"] = m_cfGrpMng->addProperty("Background");
  m_cfProp["BGA0"] = m_cfDblMng->addProperty("A0");
  m_cfDblMng->setDecimals(m_cfProp["BGA0"], m_nDec);
  m_cfProp["BGA1"] = m_cfDblMng->addProperty("A1");
  m_cfDblMng->setDecimals(m_cfProp["BGA1"], m_nDec);
  m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA0"]);
  m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]);
  m_cfTree->addProperty(m_cfProp["LinearBackground"]);

  // Delta Function
  m_cfProp["DeltaFunction"] = m_cfGrpMng->addProperty("Delta Function");
  m_cfProp["UseDeltaFunc"] = m_cfBlnMng->addProperty("Use");
  m_cfProp["DeltaHeight"] = m_cfDblMng->addProperty("Height");
  m_cfDblMng->setDecimals(m_cfProp["DeltaHeight"], m_nDec);
  m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["UseDeltaFunc"]);
  m_cfTree->addProperty(m_cfProp["DeltaFunction"]);

  m_cfProp["Lorentzian1"] = createLorentzian("Lorentzian 1");
  m_cfProp["Lorentzian2"] = createLorentzian("Lorentzian 2");

  // Connections
  connect(m_cfRangeS, SIGNAL(minValueChanged(double)), this, SLOT(confitMinChanged(double)));
  connect(m_cfRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(confitMaxChanged(double)));
  connect(m_cfBackgS, SIGNAL(minValueChanged(double)), this, SLOT(confitBackgLevel(double)));
  connect(m_cfHwhmRange, SIGNAL(minValueChanged(double)), this, SLOT(confitHwhmChanged(double)));
  connect(m_cfHwhmRange, SIGNAL(maxValueChanged(double)), this, SLOT(confitHwhmChanged(double)));
  connect(m_cfDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(confitUpdateRS(QtProperty*, double)));
  connect(m_cfBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(confitCheckBoxUpdate(QtProperty*, bool)));

  connect(m_cfDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(confitPlotGuess(QtProperty*)));

  // Have HWHM Range linked to Fit Start/End Range
  connect(m_cfRangeS, SIGNAL(rangeChanged(double, double)), m_cfHwhmRange, SLOT(setRange(double, double)));
  m_cfHwhmRange->setRange(-1.0,1.0);
  confitHwhmUpdateRS(0.02);

  confitTypeSelection(m_uiForm.confit_cbFitType->currentIndex());
  confitBgTypeSelection(m_uiForm.confit_cbBackground->currentIndex());

  // Replot input automatically when file / spec no changes
  connect(m_uiForm.confit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(confitPlotInput()));
  connect(m_uiForm.confit_inputFile, SIGNAL(fileEditingFinished()), this, SLOT(confitPlotInput()));
  
  connect(m_uiForm.confit_cbInputType, SIGNAL(currentIndexChanged(int)), m_uiForm.confit_swInput, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.confit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(confitTypeSelection(int)));
  connect(m_uiForm.confit_cbBackground, SIGNAL(currentIndexChanged(int)), this, SLOT(confitBgTypeSelection(int)));
  connect(m_uiForm.confit_pbPlotInput, SIGNAL(clicked()), this, SLOT(confitPlotInput()));
  connect(m_uiForm.confit_pbSequential, SIGNAL(clicked()), this, SLOT(confitSequential()));

  m_uiForm.confit_leSpecNo->setValidator(m_valInt);
  m_uiForm.confit_leSpecMax->setValidator(m_valInt);

  // Context menu
  m_cfTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_cfTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));
}

void IndirectDataAnalysis::setupAbsorptionF2Py()
{
  // set signals and slot connections for F2Py Absorption routine
  connect(m_uiForm.absp_cbInputType, SIGNAL(currentIndexChanged(int)), m_uiForm.absp_swInput, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.absp_cbShape, SIGNAL(currentIndexChanged(int)), this, SLOT(absf2pShape(int)));
  connect(m_uiForm.absp_ckUseCan, SIGNAL(toggled(bool)), this, SLOT(absf2pUseCanChecked(bool)));
  connect(m_uiForm.absp_letc1, SIGNAL(editingFinished()), this, SLOT(absf2pTCSync()));
  // apply QValidators to items.
  m_uiForm.absp_lewidth->setValidator(m_valDbl);
  m_uiForm.absp_leavar->setValidator(m_valDbl);
  // sample
  m_uiForm.absp_lesamden->setValidator(m_valDbl);
  m_uiForm.absp_lesamsigs->setValidator(m_valDbl);
  m_uiForm.absp_lesamsiga->setValidator(m_valDbl);
  // can
  m_uiForm.absp_lecanden->setValidator(m_valDbl);
  m_uiForm.absp_lecansigs->setValidator(m_valDbl);
  m_uiForm.absp_lecansiga->setValidator(m_valDbl);
  // flat shape
  m_uiForm.absp_lets->setValidator(m_valDbl);
  m_uiForm.absp_letc1->setValidator(m_valDbl);
  m_uiForm.absp_letc2->setValidator(m_valDbl);
  // cylinder shape
  m_uiForm.absp_ler1->setValidator(m_valDbl);
  m_uiForm.absp_ler2->setValidator(m_valDbl);
  m_uiForm.absp_ler3->setValidator(m_valDbl);

  // "Nudge" color of title of QGroupBox to change.
  absf2pUseCanChecked(m_uiForm.absp_ckUseCan->isChecked());
}

void IndirectDataAnalysis::setupAbsCor()
{
  // Disable Container inputs is "Use Container" is not checked
  connect(m_uiForm.abscor_ckUseCan, SIGNAL(toggled(bool)), m_uiForm.abscor_lbContainerInputType, SLOT(setEnabled(bool)));
  connect(m_uiForm.abscor_ckUseCan, SIGNAL(toggled(bool)), m_uiForm.abscor_cbContainerInputType, SLOT(setEnabled(bool)));
  connect(m_uiForm.abscor_ckUseCan, SIGNAL(toggled(bool)), m_uiForm.abscor_swContainerInput, SLOT(setEnabled(bool)));

  connect(m_uiForm.abscor_cbSampleInputType, SIGNAL(currentIndexChanged(int)), m_uiForm.abscor_swSampleInput, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.abscor_cbContainerInputType, SIGNAL(currentIndexChanged(int)), m_uiForm.abscor_swContainerInput, SLOT(setCurrentIndex(int)));
}

bool IndirectDataAnalysis::validateElwin()
{
  bool valid = true;

  if ( ! m_uiForm.elwin_inputFile->isValid() )
  {
    valid = false;
  }

  return valid;
}

bool IndirectDataAnalysis::validateMsd()
{
  bool valid = true;

  if ( ! m_uiForm.msd_inputFile->isValid() )
  {
    valid = false;
  }

  return valid;
}

bool IndirectDataAnalysis::validateFury()
{
  bool valid = true;

  switch ( m_uiForm.fury_cbInputType->currentIndex() )
  {
  case 0:
    {
      if ( ! m_uiForm.fury_iconFile->isValid() )
      {
        valid = false;
      }
    }
    break;
  case 1:
    {
      if ( m_uiForm.fury_wsSample->currentText() == "" )
      {
        valid = false;
      }
    }
    break;
  }

  if ( ! m_uiForm.fury_resFile->isValid()  )
  {
    valid = false;
  }

  return valid;
}

bool IndirectDataAnalysis::validateConfit()
{
  bool valid = true;

  if ( m_uiForm.confit_cbInputType->currentIndex() == 0 ) // File
  {
    if ( ! m_uiForm.confit_inputFile->isValid() )
    {
      valid = false;
    }
  }
  else // Workspace
  {
    if ( m_uiForm.confit_wsSample->currentText() == "" )
    { 
      valid = false;
    }
  }

  if ( m_uiForm.confit_cbFitType->currentIndex() == 0 && ! m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
  {
    valid = false;
  }

  return valid;
}

bool IndirectDataAnalysis::validateAbsorptionF2Py()
{
  bool valid = true;

  // Input (file or workspace)
  if ( m_uiForm.absp_cbInputType->currentText() == "File" )
  {
    if ( ! m_uiForm.absp_inputFile->isValid() ) { valid = false; }
  }
  else
  {
    if ( m_uiForm.absp_wsInput->currentText() == "" ) { valid = false; }
  }

  if ( m_uiForm.absp_cbShape->currentText() == "Flat" )
  {
    // Flat Geometry
    if ( m_uiForm.absp_lets->text() != "" )
    {
      m_uiForm.absp_valts->setText(" ");
    }
    else
    {
      m_uiForm.absp_valts->setText("*");
      valid = false;
    }

    if ( m_uiForm.absp_ckUseCan->isChecked() )
    {
      if ( m_uiForm.absp_letc1->text() != "" )
      {
        m_uiForm.absp_valtc1->setText(" ");
      }
      else
      {
        m_uiForm.absp_valtc1->setText("*");
        valid = false;
      }

      if ( m_uiForm.absp_letc2->text() != "" )
      {
        m_uiForm.absp_valtc2->setText(" ");
      }
      else
      {
        m_uiForm.absp_valtc2->setText("*");
        valid = false;
      }
    }
  }

  if ( m_uiForm.absp_cbShape->currentText() == "Cylinder" )
  {
    // Cylinder geometry
    if ( m_uiForm.absp_ler1->text() != "" )
    {
      m_uiForm.absp_valR1->setText(" ");
    }
    else
    {
      m_uiForm.absp_valR1->setText("*");
      valid = false;
    }

    if ( m_uiForm.absp_ler2->text() != "" )
    {
      m_uiForm.absp_valR2->setText(" ");
    }
    else
    {
      m_uiForm.absp_valR2->setText("*");
      valid = false;
    }
    
    // R3  only relevant when using can
    if ( m_uiForm.absp_ckUseCan->isChecked() )
    {
      if ( m_uiForm.absp_ler3->text() != "" )
      {
        m_uiForm.absp_valR3->setText(" ");
      }
      else
      {
        m_uiForm.absp_valR3->setText("*");
        valid = false;
      }
    }
  }

  // Can angle to beam || Step size
  if ( m_uiForm.absp_leavar->text() != "" )
  {
    m_uiForm.absp_valAvar->setText(" ");
  }
  else
  {
    m_uiForm.absp_valAvar->setText("*");
    valid = false;
  }

  // Beam Width
  if ( m_uiForm.absp_lewidth->text() != "" )
  {
    m_uiForm.absp_valWidth->setText(" ");
  }
  else
  {
    m_uiForm.absp_valWidth->setText("*");
    valid = false;
  }

  // Sample details
  if ( m_uiForm.absp_lesamden->text() != "" )
  {
    m_uiForm.absp_valSamden->setText(" ");
  }
  else
  {
    m_uiForm.absp_valSamden->setText("*");
    valid = false;
  }

  if ( m_uiForm.absp_lesamsigs->text() != "" )
  {
    m_uiForm.absp_valSamsigs->setText(" ");
  }
  else
  {
    m_uiForm.absp_valSamsigs->setText("*");
    valid = false;
  }

  if ( m_uiForm.absp_lesamsiga->text() != "" )
  {
    m_uiForm.absp_valSamsiga->setText(" ");
  }
  else
  {
    m_uiForm.absp_valSamsiga->setText("*");
    valid = false;
  }

  // Can details (only test if "Use Can" is checked)
  if ( m_uiForm.absp_ckUseCan->isChecked() )
  {
    if ( m_uiForm.absp_lecanden->text() != "" )
    {
      m_uiForm.absp_valCanden->setText(" ");
    }
    else
    {
      m_uiForm.absp_valCanden->setText("*");
      valid = false;
    }

    if ( m_uiForm.absp_lecansigs->text() != "" )
    {
      m_uiForm.absp_valCansigs->setText(" ");
    }
    else
    {
      m_uiForm.absp_valCansigs->setText("*");
      valid = false;
    }

    if ( m_uiForm.absp_lecansiga->text() != "" )
    {
      m_uiForm.absp_valCansiga->setText(" ");
    }
    else
    {
      m_uiForm.absp_valCansiga->setText("*");
      valid = false;
    }
  }

  return valid;
}

Mantid::API::CompositeFunction_sptr IndirectDataAnalysis::furyfitCreateFunction(bool tie)
{
  Mantid::API::CompositeFunction_sptr result( new Mantid::API::CompositeFunction );
  QString fname;
  const int fitType = m_uiForm.furyfit_cbFitType->currentIndex();

  Mantid::API::IFunction_sptr func = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
  func->setParameter("A0", m_ffDblMng->value(m_ffProp["BackgroundA0"]));
  result->addFunction(func);
  result->tie("f0.A1", "0");
  if ( tie ) { result->tie("f0.A0", m_ffProp["BackgroundA0"]->valueText().toStdString()); }
  
  if ( fitType == 2 ) { fname = "Stretched Exponential"; }
  else { fname = "Exponential 1"; }

  result->addFunction(furyfitCreateUserFunction(fname, tie));

  if ( fitType == 1 || fitType == 3 )
  {
    if ( fitType == 1 ) { fname = "Exponential 2"; }
    else { fname = "Stretched Exponential"; }
    result->addFunction(furyfitCreateUserFunction(fname, tie));
  }

  // Return CompositeFunction object to caller.
  result->applyTies();
  return result;
}

Mantid::API::IFunction_sptr IndirectDataAnalysis::furyfitCreateUserFunction(const QString & name, bool tie)
{
  Mantid::API::IFunction_sptr result = Mantid::API::FunctionFactory::Instance().createFunction("UserFunction");  
  std::string formula;

  if ( name.startsWith("Exp") ) { formula = "Intensity*exp(-(x/Tau))"; }
  else { formula = "Intensity*exp(-(x/Tau)^Beta)"; }

  Mantid::API::IFunction::Attribute att(formula);  
  result->setAttribute("Formula", att);

  result->setParameter("Intensity", m_ffDblMng->value(m_ffProp[name+".Intensity"]));

  if ( tie || ! m_ffProp[name+".Intensity"]->subProperties().isEmpty() )
  {
    result->tie("Intensity", m_ffProp[name+".Intensity"]->valueText().toStdString());
  }
  result->setParameter("Tau", m_ffDblMng->value(m_ffProp[name+".Tau"]));
  if ( tie || ! m_ffProp[name+".Tau"]->subProperties().isEmpty() )
  {
    result->tie("Tau", m_ffProp[name+".Tau"]->valueText().toStdString());
  }
  if ( name.startsWith("Str") )
  {
    result->setParameter("Beta", m_ffDblMng->value(m_ffProp[name+".Beta"]));
    if ( tie || ! m_ffProp[name+".Beta"]->subProperties().isEmpty() )
    {
      result->tie("Beta", m_ffProp[name+".Beta"]->valueText().toStdString());
    }
  }

  return result;
}

Mantid::API::CompositeFunction_sptr IndirectDataAnalysis::confitCreateFunction(bool tie)
{
  auto conv = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(Mantid::API::FunctionFactory::Instance().createFunction("Convolution"));
  Mantid::API::CompositeFunction_sptr result( new Mantid::API::CompositeFunction );
  Mantid::API::CompositeFunction_sptr comp;

  bool singleFunction = false;

  if ( m_uiForm.confit_cbFitType->currentText() == "Two Lorentzians" )
  {
    comp.reset( new Mantid::API::CompositeFunction );
  }
  else
  {
    comp = conv;
    singleFunction = true;
  }

  size_t index = 0;

  // 0 = Fixed Flat, 1 = Fit Flat, 2 = Fit all
  const int bgType = m_uiForm.confit_cbBackground->currentIndex();

  // 1 - CompositeFunction A
  // - - 1 LinearBackground
  // - - 2 Convolution Function
  // - - - - 1 Resolution
  // - - - - 2 CompositeFunction B
  // - - - - - - DeltaFunction (yes/no)
  // - - - - - - Lorentzian 1 (yes/no)
  // - - - - - - Lorentzian 2 (yes/no)

  Mantid::API::IFunction_sptr func;

  // Background
  func = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
  index = result->addFunction(func);
  if ( tie  || bgType == 0 || ! m_cfProp["BGA0"]->subProperties().isEmpty() )
  {
    result->tie("f0.A0", m_cfProp["BGA0"]->valueText().toStdString() );
  }
  else
  {
    func->setParameter("A0", m_cfProp["BGA0"]->valueText().toDouble());
  }

  if ( bgType != 2 )
  {
    result->tie("f0.A1", "0.0");
  }
  else
  {
    if ( tie || ! m_cfProp["BGA1"]->subProperties().isEmpty() )
    {
      result->tie("f0.A1", m_cfProp["BGA1"]->valueText().toStdString() );
    }
    else { func->setParameter("A1", m_cfProp["BGA1"]->valueText().toDouble()); }
  }

  // Resolution
  func = Mantid::API::FunctionFactory::Instance().createFunction("Resolution");
  index = conv->addFunction(func);
  std::string resfilename = m_uiForm.confit_resInput->getFirstFilename().toStdString();
  Mantid::API::IFunction::Attribute attr(resfilename);
  func->setAttribute("FileName", attr);

  // Delta Function
  if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
  {
    func = Mantid::API::FunctionFactory::Instance().createFunction("DeltaFunction");
    index = comp->addFunction(func);
    if ( tie || ! m_cfProp["DeltaHeight"]->subProperties().isEmpty() )
    {
      comp->tie("f0.Height", m_cfProp["DeltaHeight"]->valueText().toStdString() );
    }
    else { func->setParameter("Height", m_cfProp["DeltaHeight"]->valueText().toDouble()); }
  }

  // Lorentzians
  switch ( m_uiForm.confit_cbFitType->currentIndex() )
  {
  case 0: // No Lorentzians
    break;
  case 1: // 1 Lorentzian
    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = comp->addFunction(func);
    populateFunction(func, comp, m_cfProp["Lorentzian1"], index, tie);
    break;
  case 2: // 2 Lorentzian
    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = comp->addFunction(func);
    populateFunction(func, comp, m_cfProp["Lorentzian1"], index, tie);
    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = comp->addFunction(func);
    populateFunction(func, comp, m_cfProp["Lorentzian2"], index, tie);
    // Tie PeakCentres together
    if ( ! tie )
    {
      QString tieL = "f" + QString::number(index-1) + ".PeakCentre";
      QString tieR = "f" + QString::number(index) + ".PeakCentre";
      comp->tie(tieL.toStdString(), tieR.toStdString());
    }
    break;
  }

  if ( ! singleFunction )
    conv->addFunction(comp);
  result->addFunction(conv);

  result->applyTies();

  return result;
}

QtProperty* IndirectDataAnalysis::createLorentzian(const QString & name)
{
  QtProperty* lorentzGroup = m_cfGrpMng->addProperty(name);
  m_cfProp[name+".Height"] = m_cfDblMng->addProperty("Height");
  // m_cfDblMng->setRange(m_cfProp[name+".Height"], 0.0, 1.0); // 0 < Height < 1
  m_cfProp[name+".PeakCentre"] = m_cfDblMng->addProperty("PeakCentre");
  m_cfProp[name+".HWHM"] = m_cfDblMng->addProperty("HWHM");
  m_cfDblMng->setDecimals(m_cfProp[name+".Height"], m_nDec);
  m_cfDblMng->setDecimals(m_cfProp[name+".PeakCentre"], m_nDec);
  m_cfDblMng->setDecimals(m_cfProp[name+".HWHM"], m_nDec);
  m_cfDblMng->setValue(m_cfProp[name+".HWHM"], 0.02);
  lorentzGroup->addSubProperty(m_cfProp[name+".Height"]);
  lorentzGroup->addSubProperty(m_cfProp[name+".PeakCentre"]);
  lorentzGroup->addSubProperty(m_cfProp[name+".HWHM"]);
  return lorentzGroup;
}

QtProperty* IndirectDataAnalysis::createExponential(const QString & name)
{
  QtProperty* expGroup = m_groupManager->addProperty(name);
  m_ffProp[name+".Intensity"] = m_ffDblMng->addProperty("Intensity");
  m_ffDblMng->setDecimals(m_ffProp[name+".Intensity"], m_nDec);
  m_ffProp[name+".Tau"] = m_ffDblMng->addProperty("Tau");
  m_ffDblMng->setDecimals(m_ffProp[name+".Tau"], m_nDec);
  expGroup->addSubProperty(m_ffProp[name+".Intensity"]);
  expGroup->addSubProperty(m_ffProp[name+".Tau"]);
  return expGroup;
}

QtProperty* IndirectDataAnalysis::createStretchedExp(const QString & name)
{
  QtProperty* prop = m_groupManager->addProperty(name);
  m_ffProp[name+".Intensity"] = m_ffDblMng->addProperty("Intensity");
  m_ffProp[name+".Tau"] = m_ffDblMng->addProperty("Tau");
  m_ffProp[name+".Beta"] = m_ffDblMng->addProperty("Beta");
  m_ffDblMng->setDecimals(m_ffProp[name+".Intensity"], m_nDec);
  m_ffDblMng->setDecimals(m_ffProp[name+".Tau"], m_nDec);
  m_ffDblMng->setDecimals(m_ffProp[name+".Beta"], m_nDec);
  prop->addSubProperty(m_ffProp[name+".Intensity"]);
  prop->addSubProperty(m_ffProp[name+".Tau"]);
  prop->addSubProperty(m_ffProp[name+".Beta"]);
  return prop;
}

void IndirectDataAnalysis::populateFunction(Mantid::API::IFunction_sptr func, Mantid::API::IFunction_sptr comp, QtProperty* group, size_t index, bool tie)
{
  // Get subproperties of group and apply them as parameters on the function object
  QList<QtProperty*> props = group->subProperties();
  QString pref = "f" + QString::number(index) + ".";

  for ( int i = 0; i < props.size(); i++ )
  {
    if ( tie || ! props[i]->subProperties().isEmpty() )
    {
      QString propName = pref + props[i]->propertyName();
      comp->tie(propName.toStdString(), props[i]->valueText().toStdString() );
    }
    else
    {
      std::string propName = props[i]->propertyName().toStdString();
      double propValue = props[i]->valueText().toDouble();
      func->setParameter(propName, propValue);
    }
  }
}

QwtPlotCurve* IndirectDataAnalysis::plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, std::string workspace, size_t index)
{
  if ( curve != NULL )
  {
    curve->attach(0);
    delete curve;
    curve = 0;
  }

  Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(workspace));

  size_t nhist = ws->getNumberHistograms();
  if ( index >= nhist )
  {
    showInformationBox("Error: Workspace index out of range.");
    return NULL;
  }

  using Mantid::MantidVec;
  const MantidVec & dataX = ws->readX(index);
  const MantidVec & dataY = ws->readY(index);

  curve = new QwtPlotCurve();
  curve->setData(&dataX[0], &dataY[0], static_cast<int>(ws->blocksize()));
  curve->attach(plot);

  plot->replot();

  return curve;
}

/**
 * Returns the range of the given curve data.
 * @param curve :: A Qwt plot curve
 * @returns A pair of doubles indicating the range
 * @throws std::invalid_argument If the curve has too few points (<2) or is NULL
 */
std::pair<double,double> IndirectDataAnalysis::getCurveRange(QwtPlotCurve* curve)
{
  if( !curve )
  {
    throw std::invalid_argument("Invalid curve as argument to getCurveRange");
  }  
  size_t npts = curve->data().size();
  if( npts < 2 )
  {
    throw std::invalid_argument("Too few points on data curve to determine range.");
  }
  return std::make_pair(curve->data().x(0), curve->data().x(npts-1));
}


void IndirectDataAnalysis::run()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());

  if ( tabName == "Elwin" )  { elwinRun(); }
  else if ( tabName == "MSD Fit" ) { msdRun(); }
  else if ( tabName == "Fury" ) { furyRun(); }
  else if ( tabName == "FuryFit" ) { furyfitRun(); }
  else if ( tabName == "ConvFit" ) { confitRun(); }
  else if ( tabName == "Calculate Corrections" ) { absf2pRun(); }
  else if ( tabName == "Apply Corrections" ) { abscorRun(); }
  else { showInformationBox("This tab does not have a 'Run' action."); }
}

void IndirectDataAnalysis::fitContextMenu(const QPoint &)
{
  QtBrowserItem* item(NULL);
  QtDoublePropertyManager* dblMng(NULL);

  int pageNo = m_uiForm.tabWidget->currentIndex();
  if ( pageNo == 3 )
  { // FuryFit
    item = m_ffTree->currentItem();
    dblMng = m_ffDblMng;  
  }
  else if ( pageNo == 4 )
  { // Convolution Fit
    item = m_cfTree->currentItem();
    dblMng = m_cfDblMng;
  }

  if ( ! item )
  {
    return;
  }

  // is it a fit property ?
  QtProperty* prop = item->property();

  if ( pageNo == 4 && ( prop == m_cfProp["StartX"] || prop == m_cfProp["EndX"] ) )
  {
    return;
  }

  // is it already fixed?
  bool fixed = prop->propertyManager() != dblMng;

  if ( fixed && prop->propertyManager() != m_stringManager ) { return; }

  // Create the menu
  QMenu* menu = new QMenu("FuryFit", m_ffTree);
  QAction* action;

  if ( ! fixed )
  {
    action = new QAction("Fix", this);
    connect(action, SIGNAL(triggered()), this, SLOT(fixItem()));
  }
  else
  {
    action = new QAction("Remove Fix", this);
    connect(action, SIGNAL(triggered()), this, SLOT(unFixItem()));
  }

  menu->addAction(action);

  // Show the menu
  menu->popup(QCursor::pos());
}

void IndirectDataAnalysis::fixItem()
{
  int pageNo = m_uiForm.tabWidget->currentIndex();

  QtBrowserItem* item(NULL);
  if ( pageNo == 3 )
  { // FuryFit
    item = m_ffTree->currentItem();
  }
  else if ( pageNo == 4 )
  { // Convolution Fit
    item = m_cfTree->currentItem();
  }

  // Determine what the property is.
  QtProperty* prop = item->property();

  QtProperty* fixedProp = m_stringManager->addProperty( prop->propertyName() );
  QtProperty* fprlbl = m_stringManager->addProperty("Fixed");
  fixedProp->addSubProperty(fprlbl);
  m_stringManager->setValue(fixedProp, prop->valueText());

  item->parent()->property()->addSubProperty(fixedProp);

  m_fixedProps[fixedProp] = prop;

  item->parent()->property()->removeSubProperty(prop);    
}

void IndirectDataAnalysis::unFixItem()
{
  QtBrowserItem* item(NULL);
  
  int pageNo = m_uiForm.tabWidget->currentIndex();
  if ( pageNo == 3 )
  { // FuryFit
    item = m_ffTree->currentItem();
  }
  else if ( pageNo == 4 )
  { // Convolution Fit
    item = m_cfTree->currentItem();
  }

  QtProperty* prop = item->property();
  if ( prop->subProperties().empty() )
  { 
    item = item->parent();
    prop = item->property();
  }

  item->parent()->property()->addSubProperty(m_fixedProps[prop]);
  item->parent()->property()->removeSubProperty(prop);
  m_fixedProps.remove(prop);
  QtProperty* proplbl = prop->subProperties()[0];
  delete proplbl;
  delete prop;
}

void IndirectDataAnalysis::elwinRun()
{
  if ( ! validateElwin() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import elwin\n"
    "input = [r'" + m_uiForm.elwin_inputFile->getFilenames().join("', r'") + "']\n"
    "eRange = [ " + QString::number(m_elwDblMng->value(m_elwProp["R1S"])) +","+ QString::number(m_elwDblMng->value(m_elwProp["R1E"]));

  if ( m_elwBlnMng->value(m_elwProp["UseTwoRanges"]) )
  {
    pyInput += ", " + QString::number(m_elwDblMng->value(m_elwProp["R2S"])) + ", " + QString::number(m_elwDblMng->value(m_elwProp["R2E"]));
  }

  pyInput+= "]\n";

  if ( m_uiForm.elwin_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.elwin_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.elwin_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "eq1_ws, eq2_ws = elwin(input, eRange, Save=save, Verbose=verbose, Plot=plot)\n";

  if ( m_uiForm.elwin_ckConcat->isChecked() )
  {
    pyInput += "from IndirectDataAnalysis import concatWSs\n"
      "concatWSs(eq1_ws, 'MomentumTransfer', 'ElwinQResults')\n"
      "concatWSs(eq2_ws, 'QSquared', 'ElwinQSqResults')\n";
  }

  QString pyOutput = runPythonCode(pyInput).trimmed();

}

void IndirectDataAnalysis::elwinPlotInput()
{
  if ( m_uiForm.elwin_inputFile->isValid() )
  {
    QString filename = m_uiForm.elwin_inputFile->getFirstFilename();
    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
    QString pyOutput = runPythonCode(pyInput);

    std::string workspace = wsname.toStdString();

    m_elwDataCurve = plotMiniplot(m_elwPlot, m_elwDataCurve, workspace, 0);
    try
    {
      const std::pair<double, double> range = getCurveRange(m_elwDataCurve);    
      m_elwR1->setRange(range.first, range.second);
      // Replot
      m_elwPlot->replot();
    }
    catch(std::invalid_argument & exc)
    {
      showInformationBox(exc.what());
    }

  }
  else
  {
    showInformationBox("Selected input files are invalid.");
  }
}

void IndirectDataAnalysis::elwinTwoRanges(QtProperty*, bool val)
{
  m_elwR2->setVisible(val);
}

void IndirectDataAnalysis::elwinMinChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_elwR1 )
  {
    m_elwDblMng->setValue(m_elwProp["R1S"], val);
  }
  else if ( from == m_elwR2 )
  {
    m_elwDblMng->setValue(m_elwProp["R2S"], val);
  }
}

void IndirectDataAnalysis::elwinMaxChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_elwR1 )
  {
    m_elwDblMng->setValue(m_elwProp["R1E"], val);
  }
  else if ( from == m_elwR2 )
  {
    m_elwDblMng->setValue(m_elwProp["R2E"], val);
  }
}

void IndirectDataAnalysis::elwinUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_elwProp["R1S"] ) m_elwR1->setMinimum(val);
  else if ( prop == m_elwProp["R1E"] ) m_elwR1->setMaximum(val);
  else if ( prop == m_elwProp["R2S"] ) m_elwR2->setMinimum(val);
  else if ( prop == m_elwProp["R2E"] ) m_elwR2->setMaximum(val);
}

void IndirectDataAnalysis::msdRun()
{
  if ( ! validateMsd() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import msdfit\n"
    "startX = " + QString::number(m_msdDblMng->value(m_msdProp["Start"])) +"\n"
    "endX = " + QString::number(m_msdDblMng->value(m_msdProp["End"])) +"\n"
    "inputs = [r'" + m_uiForm.msd_inputFile->getFilenames().join("', r'") + "']\n";

  if ( m_uiForm.msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.msd_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.msd_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "msdfit(inputs, startX, endX, Save=save, Verbose=verbose, Plot=plot)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void IndirectDataAnalysis::msdPlotInput()
{
  if ( m_uiForm.msd_inputFile->isValid() )
  {
    QString filename = m_uiForm.msd_inputFile->getFirstFilename();
    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
    QString pyOutput = runPythonCode(pyInput);

    std::string workspace = wsname.toStdString();

    m_msdDataCurve = plotMiniplot(m_msdPlot, m_msdDataCurve, workspace, 0);
    try
    {
      const std::pair<double, double> range = getCurveRange(m_msdDataCurve);    
      m_msdRange->setRange(range.first, range.second);
      // Replot
      m_msdPlot->replot();
    }
    catch(std::invalid_argument & exc)
    {
      showInformationBox(exc.what());
    }
  }
  else
  {
    showInformationBox("Selected input files are invalid.");
  }
}

void IndirectDataAnalysis::msdMinChanged(double val)
{
  m_msdDblMng->setValue(m_msdProp["Start"], val);
}

void IndirectDataAnalysis::msdMaxChanged(double val)
{
  m_msdDblMng->setValue(m_msdProp["End"], val);
}

void IndirectDataAnalysis::msdUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_msdProp["Start"] ) m_msdRange->setMinimum(val);
  else if ( prop == m_msdProp["End"] ) m_msdRange->setMaximum(val);
}

void IndirectDataAnalysis::furyRun()
{
  if ( !validateFury() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString filenames;
  switch ( m_uiForm.fury_cbInputType->currentIndex() )
  {
  case 0:
    filenames = m_uiForm.fury_iconFile->getFilenames().join("', r'");
    break;
  case 1:
    filenames = m_uiForm.fury_wsSample->currentText();
    break;
  }

  QString pyInput =
    "from IndirectDataAnalysis import fury\n"
    "samples = [r'" + filenames + "']\n"
    "resolution = r'" + m_uiForm.fury_resFile->getFirstFilename() + "'\n"
    "rebin = '" + m_furProp["ELow"]->valueText() +","+ m_furProp["EWidth"]->valueText() +","+m_furProp["EHigh"]->valueText()+"'\n";

  if ( m_uiForm.fury_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.fury_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.fury_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "fury_ws = fury(samples, resolution, rebin, Save=save, Verbose=verbose, Plot=plot)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void IndirectDataAnalysis::furyResType(const QString& type)
{
  QStringList exts;
  if ( type == "RES File" )
  {
    exts.append("_res.nxs");
    m_furyResFileType = true;
  }
  else
  {
    exts.append("_red.nxs");
    m_furyResFileType = false;
  }
  m_uiForm.fury_resFile->setFileExtensions(exts);
}

void IndirectDataAnalysis::furyPlotInput()
{
  std::string workspace;
  if ( m_uiForm.fury_cbInputType->currentIndex() == 0 )
  {
    if ( m_uiForm.fury_iconFile->isValid() )
    {
      QString filename = m_uiForm.fury_iconFile->getFirstFilename();
      QFileInfo fi(filename);
      QString wsname = fi.baseName();

      QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
      QString pyOutput = runPythonCode(pyInput);

      workspace = wsname.toStdString();
    }
    else
    {
      showInformationBox("Selected input files are invalid.");
      return;
    }
  }
  else if ( m_uiForm.fury_cbInputType->currentIndex() == 1 )
  {
    workspace = m_uiForm.fury_wsSample->currentText().toStdString();
    if ( workspace.empty() )
    {
      showInformationBox("No workspace selected.");
      return;
    }
  }

  m_furCurve = plotMiniplot(m_furPlot, m_furCurve, workspace, 0);
  try
  {
    const std::pair<double, double> range = getCurveRange(m_furCurve);    
    m_furRange->setRange(range.first, range.second);
    m_furPlot->replot();
  }
  catch(std::invalid_argument & exc)
  {
    showInformationBox(exc.what());
  }
}

void IndirectDataAnalysis::furyMaxChanged(double val)
{
  m_furDblMng->setValue(m_furProp["EHigh"], val);
}

void IndirectDataAnalysis::furyMinChanged(double val)
{
  m_furDblMng->setValue(m_furProp["ELow"], val);
}

void IndirectDataAnalysis::furyUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_furProp["ELow"] )
    m_furRange->setMinimum(val);
  else if ( prop == m_furProp["EHigh"] )
    m_furRange->setMaximum(val);
}

void IndirectDataAnalysis::furyfitRun()
{
  // First create the function
  auto function = furyfitCreateFunction();

  m_uiForm.furyfit_ckPlotGuess->setChecked(false);
  
  const int fitType = m_uiForm.furyfit_cbFitType->currentIndex();

  if ( m_uiForm.furyfit_ckConstrainIntensities->isChecked() )
  {
    switch ( fitType )
    {
    case 0: // 1 Exp
    case 2: // 1 Str
      m_furyfitTies = "f1.Intensity = 1-f0.A0";
      break;
    case 1: // 2 Exp
    case 3: // 1 Exp & 1 Str
      m_furyfitTies = "f1.Intensity=1-f2.Intensity-f0.A0";
      break;
    default:
      break;
    }
  }
  QString ftype;
  switch ( fitType )
  {
  case 0:
    ftype = "1E_s"; break;
  case 1:
    ftype = "2E_s"; break;
  case 2:
    ftype = "1S_s"; break;
  case 3:
    ftype = "1E1S_s"; break;
  default:
    ftype = "s"; break;
  }

  furyfitPlotInput();
  if ( m_ffInputWS == NULL )
  {
    return;
  }

  QString pyInput = "from IndirectCommon import getWSprefix\nprint getWSprefix('%1')\n";
  pyInput = pyInput.arg(QString::fromStdString(m_ffInputWSName));
  QString outputNm = runPythonCode(pyInput).trimmed();
  outputNm += QString("fury_") + ftype + m_uiForm.furyfit_leSpecNo->text();
  std::string output = outputNm.toStdString();

  // Create the Fit Algorithm
  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setPropertyValue("InputWorkspace", m_ffInputWSName);
  alg->setProperty("WorkspaceIndex", m_uiForm.furyfit_leSpecNo->text().toInt());
  alg->setProperty("StartX", m_ffRangeManager->value(m_ffProp["StartX"]));
  alg->setProperty("EndX", m_ffRangeManager->value(m_ffProp["EndX"]));
  alg->setProperty("Ties", m_furyfitTies.toStdString());
  alg->setPropertyValue("Function", function->asString());
  alg->setPropertyValue("Output", output);
  alg->execute();

  if ( ! alg->isExecuted() )
  {
    QString msg = "There was an error executing the fitting algorithm. Please see the "
      "Results Log pane for more details.";
    showInformationBox(msg);
    return;
  }

  // Now show the fitted curve of the mini plot
  m_ffFitCurve = plotMiniplot(m_ffPlot, m_ffFitCurve, output+"_Workspace", 1);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_ffFitCurve->setPen(fitPen);
  m_ffPlot->replot();

  // Do it as we do in Convolution Fit tab
  QMap<QString,double> parameters;
  QStringList parNames = QString::fromStdString(alg->getPropertyValue("ParameterNames")).split(",", QString::SkipEmptyParts);
  QStringList parVals = QString::fromStdString(alg->getPropertyValue("Parameters")).split(",", QString::SkipEmptyParts);
  for ( int i = 0; i < parNames.size(); i++ )
  {
    parameters[parNames[i]] = parVals[i].toDouble();
  }

  m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], parameters["f0.A0"]);
  
  if ( fitType != 2 )
  {
    // Exp 1
    m_ffDblMng->setValue(m_ffProp["Exponential 1.Intensity"], parameters["f1.Intensity"]);
    m_ffDblMng->setValue(m_ffProp["Exponential 1.Tau"], parameters["f1.Tau"]);
    
    if ( fitType == 1 )
    {
      // Exp 2
      m_ffDblMng->setValue(m_ffProp["Exponential 2.Intensity"], parameters["f2.Intensity"]);
      m_ffDblMng->setValue(m_ffProp["Exponential 2.Tau"], parameters["f2.Tau"]);
    }
  }
  
  if ( fitType > 1 )
  {
    // Str
    QString fval;
    if ( fitType == 2 ) { fval = "f1."; }
    else { fval = "f2."; }
    
    m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Intensity"], parameters[fval+"Intensity"]);
    m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Tau"], parameters[fval+"Tau"]);
    m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Beta"], parameters[fval+"Beta"]);
  }

  if ( m_uiForm.furyfit_ckPlotOutput->isChecked() )
  {
    QString pyInput = "from mantidplot import *\n"
      "plotSpectrum('" + QString::fromStdString(output) + "_Workspace', [0,1,2])\n";
    QString pyOutput = runPythonCode(pyInput);
  }
}

void IndirectDataAnalysis::furyfitTypeSelection(int index)
{
  m_ffTree->clear();

  m_ffTree->addProperty(m_ffProp["StartX"]);
  m_ffTree->addProperty(m_ffProp["EndX"]);

  m_ffTree->addProperty(m_ffProp["LinearBackground"]);

  switch ( index )
  {
  case 0:
    m_ffTree->addProperty(m_ffProp["Exponential1"]);
    break;
  case 1:
    m_ffTree->addProperty(m_ffProp["Exponential1"]);
    m_ffTree->addProperty(m_ffProp["Exponential2"]);
    break;
  case 2:
    m_ffTree->addProperty(m_ffProp["StretchedExp"]);
    break;
  case 3:
    m_ffTree->addProperty(m_ffProp["Exponential1"]);
    m_ffTree->addProperty(m_ffProp["StretchedExp"]);
    break;
  }
}

void IndirectDataAnalysis::furyfitPlotInput()
{
  std::string wsname;

  switch ( m_uiForm.furyfit_cbInputType->currentIndex() )
  {
  case 0: // "File"
    {
      if ( ! m_uiForm.furyfit_inputFile->isValid() )
      {
        return;
      }
      else
      {
      QFileInfo fi(m_uiForm.furyfit_inputFile->getFirstFilename());
      wsname = fi.baseName().toStdString();
      if ( (m_ffInputWS == NULL) || ( wsname != m_ffInputWSName ) )
      {
        std::string filename = m_uiForm.furyfit_inputFile->getFirstFilename().toStdString();
        // LoadNexus
        Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadNexus");
        alg->initialize();
        alg->setPropertyValue("Filename", filename);
        alg->setPropertyValue("OutputWorkspace",wsname);
        alg->execute();
        // get the output workspace
        m_ffInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
      }
      }
    }
    break;
  case 1: // Workspace
    {
      wsname = m_uiForm.furyfit_wsIqt->currentText().toStdString();
      try
      {
        m_ffInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
      }
      catch ( Mantid::Kernel::Exception::NotFoundError & )
      {
        QString msg = "Workspace: '" + QString::fromStdString(wsname) + "' could not be "
          "found in the Analysis Data Service.";
        showInformationBox(msg);
        return;
      }
    }
    break;
  }
  m_ffInputWSName = wsname;

  int specNo = m_uiForm.furyfit_leSpecNo->text().toInt();

  m_ffDataCurve = plotMiniplot(m_ffPlot, m_ffDataCurve, m_ffInputWSName, specNo);
  try
  {
    const std::pair<double, double> range = getCurveRange(m_ffDataCurve);
    m_ffRangeS->setRange(range.first, range.second);
    m_ffRangeManager->setRange(m_ffProp["StartX"], range.first, range.second);
    m_ffRangeManager->setRange(m_ffProp["EndX"], range.first, range.second);
    
    m_ffPlot->setAxisScale(QwtPlot::xBottom, range.first, range.second);
    m_ffPlot->setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
    m_ffPlot->replot();
  }
  catch(std::invalid_argument & exc)
  {
    showInformationBox(exc.what());
  }
}

void IndirectDataAnalysis::furyfitXMinSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["StartX"], val);
}

void IndirectDataAnalysis::furyfitXMaxSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["EndX"], val);
}

void IndirectDataAnalysis::furyfitBackgroundSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], val);
}

void IndirectDataAnalysis::furyfitRangePropChanged(QtProperty* prop, double val)
{
  if ( prop == m_ffProp["StartX"] )
  {
    m_ffRangeS->setMinimum(val);
  }
  else if ( prop == m_ffProp["EndX"] )
  {
    m_ffRangeS->setMaximum(val);
  }
  else if ( prop == m_ffProp["BackgroundA0"] )
  {
    m_ffBackRangeS->setMinimum(val);
  }
}

void IndirectDataAnalysis::furyfitSequential()
{
  furyfitPlotInput();
  if ( m_ffInputWS == NULL )
  {
    return;
  }

  Mantid::API::CompositeFunction_sptr func = furyfitCreateFunction();

  // Function Ties
  func->tie("f0.A1", "0");
  if ( m_uiForm.furyfit_ckConstrainIntensities->isChecked() )
  {
    switch ( m_uiForm.furyfit_cbFitType->currentIndex() )
    {
    case 0: // 1 Exp
    case 2: // 1 Str
      func->tie("f1.Intensity","1-f0.A0");
      break;
    case 1: // 2 Exp
    case 3: // 1 Exp & 1 Str
      func->tie("f1.Intensity","1-f2.Intensity-f0.A0");
      break;
    }
  }

  std::string function = std::string(func->asString());
  
  QString pyInput = "from IndirectDataAnalysis import furyfitSeq\n"
    "input = '" + QString::fromStdString(m_ffInputWSName) + "'\n"
    "func = r'" + QString::fromStdString(function) + "'\n"
    "startx = " + m_ffProp["StartX"]->valueText() + "\n"
    "endx = " + m_ffProp["EndX"]->valueText() + "\n"
    "plot = '" + m_uiForm.furyfit_cbPlotOutput->currentText() + "'\n"
    "save = ";
  pyInput += m_uiForm.furyfit_ckSaveSeq->isChecked() ? "True\n" : "False\n";
  pyInput += "furyfitSeq(input, func, startx, endx, save, plot)\n";
  
  QString pyOutput = runPythonCode(pyInput);
}

void IndirectDataAnalysis::furyfitPlotGuess(QtProperty*)
{
  if ( ! m_uiForm.furyfit_ckPlotGuess->isChecked() || m_ffDataCurve == NULL )
  {
    return;
  }

  Mantid::API::CompositeFunction_sptr function = furyfitCreateFunction(true);

  // Create the double* array from the input workspace
  const size_t binIndxLow = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["StartX"]));
  const size_t binIndxHigh = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["EndX"]));
  const size_t nData = binIndxHigh - binIndxLow;

  std::vector<double> inputXData(nData);

  const Mantid::MantidVec& XValues = m_ffInputWS->readX(0);

  const bool isHistogram = m_ffInputWS->isHistogramData();

  for ( size_t i = 0; i < nData ; i++ )
  {
    if ( isHistogram )
      inputXData[i] = 0.5*(XValues[binIndxLow+i]+XValues[binIndxLow+i+1]);
    else
      inputXData[i] = XValues[binIndxLow+i];
  }

  Mantid::API::FunctionDomain1D domain(inputXData);
  Mantid::API::FunctionValues outputData(domain);
  function->function(domain, outputData);

  QVector<double> dataX;
  QVector<double> dataY;

  for ( size_t i = 0; i < nData; i++ )
  {
    dataX.append(inputXData[i]);
    dataY.append(outputData.getCalculated(i));
  }

  // Create the curve
  if ( m_ffFitCurve != NULL )
  {
    m_ffFitCurve->attach(0);
    delete m_ffFitCurve;
    m_ffFitCurve = 0;
  }

  m_ffFitCurve = new QwtPlotCurve();
  m_ffFitCurve->setData(dataX, dataY);
  m_ffFitCurve->attach(m_ffPlot);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_ffFitCurve->setPen(fitPen);
  m_ffPlot->replot();
}

void IndirectDataAnalysis::confitRun()
{
  if ( ! validateConfit() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  confitPlotInput();

  if ( m_cfDataCurve == NULL )
  {
    showInformationBox("There was an error reading the data file.");
    return;
  }

  m_uiForm.confit_ckPlotGuess->setChecked(false);

  Mantid::API::CompositeFunction_sptr function = confitCreateFunction();

  // get output name
  QString ftype = "";
  switch ( m_uiForm.confit_cbFitType->currentIndex() )
  {
  case 0:
    ftype += "Delta"; break;
  case 1:
    ftype += "1L"; break;
  case 2:
    ftype += "2L"; break;
  default:
    break;
  }
  switch ( m_uiForm.confit_cbBackground->currentIndex() )
  {
  case 0:
    ftype += "FixF_s"; break;
  case 1:
    ftype += "FitF_s"; break;
  case 2:
    ftype += "FitL_s"; break;
  }

  QString outputNm = runPythonCode(QString("from IndirectCommon import getWSprefix\nprint getWSprefix('") + QString::fromStdString(m_cfInputWSName) + QString("')\n")).trimmed();
  outputNm += QString("conv_") + ftype + m_uiForm.confit_leSpecNo->text();  
  std::string output = outputNm.toStdString();

  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setPropertyValue("InputWorkspace", m_cfInputWSName);
  alg->setProperty<int>("WorkspaceIndex", m_uiForm.confit_leSpecNo->text().toInt());
  alg->setProperty<double>("StartX", m_cfDblMng->value(m_cfProp["StartX"]));
  alg->setProperty<double>("EndX", m_cfDblMng->value(m_cfProp["EndX"]));
  alg->setPropertyValue("Function", function->asString());
  alg->setPropertyValue("Output", output);
  alg->execute();

  if ( ! alg->isExecuted() )
  {
    showInformationBox("Fit algorithm failed.");
    return;
  }

  // Plot the line on the mini plot
  m_cfCalcCurve = plotMiniplot(m_cfPlot, m_cfCalcCurve, output+"_Workspace", 1);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_cfCalcCurve->setPen(fitPen);
  m_cfPlot->replot();

  // Update parameter values (possibly easier from algorithm properties)
  QMap<QString,double> parameters;
  QStringList parNames = QString::fromStdString(alg->getPropertyValue("ParameterNames")).split(",", QString::SkipEmptyParts);
  QStringList parVals = QString::fromStdString(alg->getPropertyValue("Parameters")).split(",", QString::SkipEmptyParts);
  for ( int i = 0; i < parNames.size(); i++ )
  {
    parameters[parNames[i]] = parVals[i].toDouble();
  }

  // Populate Tree widget with values
  // Background should always be f0
  m_cfDblMng->setValue(m_cfProp["BGA0"], parameters["f0.A0"]);
  m_cfDblMng->setValue(m_cfProp["BGA1"], parameters["f0.A1"]);

  int noLorentz = m_uiForm.confit_cbFitType->currentIndex();

  int funcIndex = 1;
  QString prefBase = "f1.f";
  if ( noLorentz > 1 || ( noLorentz > 0 && m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) ) )
  {
    prefBase += "1.f";
    funcIndex--;
  }

  if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
  {
    QString key = prefBase+QString::number(funcIndex)+".Height";
    m_cfDblMng->setValue(m_cfProp["DeltaHeight"], parameters[key]);
    funcIndex++;
  }

  if ( noLorentz > 0 )
  {
    // One Lorentz
    QString pref = prefBase + QString::number(funcIndex) + ".";
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.Height"], parameters[pref+"Height"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.PeakCentre"], parameters[pref+"PeakCentre"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], parameters[pref+"HWHM"]);
    funcIndex++;
  }

  if ( noLorentz > 1 )
  {
    // Two Lorentz
    QString pref = prefBase + QString::number(funcIndex) + ".";
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.Height"], parameters[pref+"Height"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.PeakCentre"], parameters[pref+"PeakCentre"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.HWHM"], parameters[pref+"HWHM"]);
  }

  // Plot Output
  if ( m_uiForm.confit_ckPlotOutput->isChecked() )
  {
    QString pyInput =
      "plotSpectrum('" + QString::fromStdString(output) + "_Workspace', [0,1,2])\n";
    QString pyOutput = runPythonCode(pyInput);
  }

}

void IndirectDataAnalysis::confitTypeSelection(int index)
{
  m_cfTree->removeProperty(m_cfProp["Lorentzian1"]);
  m_cfTree->removeProperty(m_cfProp["Lorentzian2"]);
  
  switch ( index )
  {
  case 0:
    m_cfHwhmRange->setVisible(false);
    break;
  case 1:
    m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
    m_cfHwhmRange->setVisible(true);
    break;
  case 2:
    m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
    m_cfTree->addProperty(m_cfProp["Lorentzian2"]);
    m_cfHwhmRange->setVisible(true);
    break;
  }    
}

void IndirectDataAnalysis::confitBgTypeSelection(int index)
{
  if ( index == 2 )
  {
    m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]);
  }
  else
  {
    m_cfProp["LinearBackground"]->removeSubProperty(m_cfProp["BGA1"]);
  }
}

void IndirectDataAnalysis::confitPlotInput()
{
  std::string wsname;
  const bool plotGuess = m_uiForm.confit_ckPlotGuess->isChecked();
  m_uiForm.confit_ckPlotGuess->setChecked(false);

  switch ( m_uiForm.confit_cbInputType->currentIndex() )
  {
  case 0: // "File"
    {
      if ( m_uiForm.confit_inputFile->isValid() )
      {
        QFileInfo fi(m_uiForm.confit_inputFile->getFirstFilename());
        wsname = fi.baseName().toStdString();
        if ( (m_ffInputWS == NULL) || ( wsname != m_ffInputWSName ) )
        {
          std::string filename = m_uiForm.confit_inputFile->getFirstFilename().toStdString();
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadNexus");
          alg->initialize();
          alg->setPropertyValue("Filename", filename);
          alg->setPropertyValue("OutputWorkspace",wsname);
          alg->execute();
          m_cfInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
        }
      }
      else
      {
        return;
      }
    }
    break;
  case 1: // Workspace
    {
      wsname = m_uiForm.confit_wsSample->currentText().toStdString();
      try
      {
        m_cfInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
      }
      catch ( Mantid::Kernel::Exception::NotFoundError & )
      {
        QString msg = "Workspace: '" + QString::fromStdString(wsname) + "' could not be "
          "found in the Analysis Data Service.";
        showInformationBox(msg);
        return;
      }
    }
    break;
  }
  m_cfInputWSName = wsname;

  int specNo = m_uiForm.confit_leSpecNo->text().toInt();
  // Set spectra max value
  size_t specMax = m_cfInputWS->getNumberHistograms();
  if( specMax > 0 ) specMax -= 1;
  if ( specNo < 0 || static_cast<size_t>(specNo) > specMax ) //cast is okay as the first check is for less-than-zero
  {
    m_uiForm.confit_leSpecNo->setText("0");
    specNo = 0;
  }
  int smCurrent = m_uiForm.confit_leSpecMax->text().toInt();
  if ( smCurrent < 0 || static_cast<size_t>(smCurrent) > specMax )
  {
    m_uiForm.confit_leSpecMax->setText(QString::number(specMax));
  }

  m_cfDataCurve = plotMiniplot(m_cfPlot, m_cfDataCurve, wsname, specNo);
  try
  {
    const std::pair<double, double> range = getCurveRange(m_cfDataCurve);    
    m_cfRangeS->setRange(range.first, range.second);
    m_uiForm.confit_ckPlotGuess->setChecked(plotGuess);
  }
  catch(std::invalid_argument & exc)
  {
    showInformationBox(exc.what());
  }
}

void IndirectDataAnalysis::confitPlotGuess(QtProperty*)
{

  if ( ! m_uiForm.confit_ckPlotGuess->isChecked() || m_cfDataCurve == NULL )
  {
    return;
  }

  Mantid::API::CompositeFunction_sptr function = confitCreateFunction(true);

  if ( m_cfInputWS == NULL )
  {
    confitPlotInput();
  }

  // std::string inputName = m_cfInputWS->getName();  // Unused

  const size_t binIndexLow = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["StartX"]));
  const size_t binIndexHigh = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["EndX"]));
  const size_t nData = binIndexHigh - binIndexLow;

  std::vector<double> inputXData(nData);
  //double* outputData = new double[nData];

  const Mantid::MantidVec& XValues = m_cfInputWS->readX(0);
  const bool isHistogram = m_cfInputWS->isHistogramData();

  for ( size_t i = 0; i < nData; i++ )
  {
    if ( isHistogram )
    {
      inputXData[i] = 0.5 * ( XValues[binIndexLow+i] + XValues[binIndexLow+i+1] );
    }
    else
    {
      inputXData[i] = XValues[binIndexLow+i];
    }
  }

  Mantid::API::FunctionDomain1D domain(inputXData);
  Mantid::API::FunctionValues outputData(domain);
  function->function(domain, outputData);

  QVector<double> dataX, dataY;

  for ( size_t i = 0; i < nData; i++ )
  {
    dataX.append(inputXData[i]);
    dataY.append(outputData.getCalculated(i));
  }

  if ( m_cfCalcCurve != NULL )
  {
    m_cfCalcCurve->attach(0);
    delete m_cfCalcCurve;
    m_cfCalcCurve = 0;
  }

  m_cfCalcCurve = new QwtPlotCurve();
  m_cfCalcCurve->setData(dataX, dataY);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_cfCalcCurve->setPen(fitPen);
  m_cfCalcCurve->attach(m_cfPlot);
  m_cfPlot->replot();
}

void IndirectDataAnalysis::confitSequential()
{
  if ( m_cfInputWS == NULL )
  {
    return;
  }

  QString bg = m_uiForm.confit_cbBackground->currentText();
  if ( bg == "Fixed Flat" )
  {
    bg = "FixF";
  }
  else if ( bg == "Fit Flat" )
  {
    bg = "FitF";
  }
  else if ( bg == "Fit Linear" )
  {
    bg = "FitL";
  }

  Mantid::API::CompositeFunction_sptr func = confitCreateFunction();
  std::string function = std::string(func->asString());
  QString stX = m_cfProp["StartX"]->valueText();
  QString enX = m_cfProp["EndX"]->valueText();

  QString pyInput =
    "from IndirectDataAnalysis import confitSeq\n"
    "input = '" + QString::fromStdString(m_cfInputWSName) + "'\n"
    "func = r'" + QString::fromStdString(function) + "'\n"
    "startx = " + stX + "\n"
    "endx = " + enX + "\n"
    "specMin = " + m_uiForm.confit_leSpecNo->text() + "\n"
    "specMax = " + m_uiForm.confit_leSpecMax->text() + "\n"
    "plot = '" + m_uiForm.confit_cbPlotOutput->currentText() + "'\n"
    "save = ";
  
  pyInput += m_uiForm.confit_ckSaveSeq->isChecked() ? "True\n" : "False\n";
  
  pyInput +=    
    "bg = '" + bg + "'\n"
    "confitSeq(input, func, startx, endx, save, plot, bg, specMin, specMax)\n";

  QString pyOutput = runPythonCode(pyInput);
}

void IndirectDataAnalysis::confitMinChanged(double val)
{
  m_cfDblMng->setValue(m_cfProp["StartX"], val);
}

void IndirectDataAnalysis::confitMaxChanged(double val)
{
  m_cfDblMng->setValue(m_cfProp["EndX"], val);
}

void IndirectDataAnalysis::confitHwhmChanged(double val)
{
  const double peakCentre = m_cfDblMng->value(m_cfProp["Lorentzian 1.PeakCentre"]);
  // Always want HWHM to display as positive.
  if ( val > peakCentre )
  {
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], val-peakCentre);
  }
  else
  {
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], peakCentre-val);
  }
}

void IndirectDataAnalysis::confitBackgLevel(double val)
{
  m_cfDblMng->setValue(m_cfProp["BGA0"], val);
}

void IndirectDataAnalysis::confitUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_cfProp["StartX"] ) { m_cfRangeS->setMinimum(val); }
  else if ( prop == m_cfProp["EndX"] ) { m_cfRangeS->setMaximum(val); }
  else if ( prop == m_cfProp["BGA0"] ) { m_cfBackgS->setMinimum(val); }
  else if ( prop == m_cfProp["Lorentzian 1.HWHM"] ) { confitHwhmUpdateRS(val); }
}

void IndirectDataAnalysis::confitHwhmUpdateRS(double val)
{
  const double peakCentre = m_cfDblMng->value(m_cfProp["Lorentzian 1.PeakCentre"]);
  m_cfHwhmRange->setMinimum(peakCentre-val);
  m_cfHwhmRange->setMaximum(peakCentre+val);
}

void IndirectDataAnalysis::confitCheckBoxUpdate(QtProperty* prop, bool checked)
{
  // Add/remove some properties to display only relevant options
  if ( prop == m_cfProp["UseDeltaFunc"] )
  {
    if ( checked ) { m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["DeltaHeight"]); }
    else { m_cfProp["DeltaFunction"]->removeSubProperty(m_cfProp["DeltaHeight"]); }
  }
}

void IndirectDataAnalysis::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

void IndirectDataAnalysis::help()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
  QString url = "http://www.mantidproject.org/IDA";
  if ( tabName == "Initial Settings" )
    url += "";
  else if ( tabName == "Elwin" )
    url += ":Elwin";
  else if ( tabName == "MSD Fit" )
    url += ":MSDFit";
  else if ( tabName == "Fury" )
    url += ":Fury";
  else if ( tabName == "FuryFit" )
    url += ":FuryFit";
  else if ( tabName == "ConvFit" )
    url += ":ConvFit";
  else if ( tabName == "Calculate Corrections" )
    url += ":CalcCor";
  else if ( tabName == "Apply Corrections" )
    url += ":AbsCor";
  QDesktopServices::openUrl(QUrl(url));
}

void IndirectDataAnalysis::absf2pRun()
{
  if ( ! validateAbsorptionF2Py() )
  {
    showInformationBox("Invalid input.");
    return;
  }

  QString pyInput = "import IndirectAbsCor\n";
  
  QString geom;
  QString size;

  if ( m_uiForm.absp_cbShape->currentText() == "Flat" )
  {
    geom = "flt";
    if ( m_uiForm.absp_ckUseCan->isChecked() ) 
    {
      size = "[" + m_uiForm.absp_lets->text() + ", " +
      m_uiForm.absp_letc1->text() + ", " +
      m_uiForm.absp_letc2->text() + "]";
    }
    else
    {
      size = "[" + m_uiForm.absp_lets->text() + ", 0.0, 0.0]";
    }
  }
  else if ( m_uiForm.absp_cbShape->currentText() == "Cylinder" )
  {
    geom = "cyl";

    // R3 only populated when using can. R4 is fixed to 0.0
    if ( m_uiForm.absp_ckUseCan->isChecked() ) 
    {
      size = "[" + m_uiForm.absp_ler1->text() + ", " +
        m_uiForm.absp_ler2->text() + ", " +
        m_uiForm.absp_ler3->text() + ", 0.0 ]";
    }
    else
    {
      size = "[" + m_uiForm.absp_ler1->text() + ", " +
        m_uiForm.absp_ler2->text() + ", 0.0, 0.0 ]";
    }
    
  }

  QString width = m_uiForm.absp_lewidth->text();

  if ( m_uiForm.absp_cbInputType->currentText() == "File" )
  {
    QString input = m_uiForm.absp_inputFile->getFirstFilename();
    if ( input == "" ) { return; }
    pyInput +=
    "import os.path as op\n"
    "file = r'" + input + "'\n"
    "( dir, filename ) = op.split(file)\n"
    "( name, ext ) = op.splitext(filename)\n"
    "LoadNexusProcessed(file, name)\n"
    "inputws = name\n";
  }
  else
  {
    pyInput += "inputws = '" + m_uiForm.absp_wsInput->currentText() + "'\n";
  }
  
  if ( m_uiForm.absp_ckUseCan->isChecked() )
  {
    pyInput +=
      "ncan = 2\n"
      "density = [" + m_uiForm.absp_lesamden->text() + ", " + m_uiForm.absp_lecanden->text() + ", " + m_uiForm.absp_lecanden->text() + "]\n"
      "sigs = [" + m_uiForm.absp_lesamsigs->text() + "," + m_uiForm.absp_lecansigs->text() + "," + m_uiForm.absp_lecansigs->text() + "]\n"
      "siga = [" + m_uiForm.absp_lesamsiga->text() + "," + m_uiForm.absp_lecansiga->text() + "," + m_uiForm.absp_lecansiga->text() + "]\n";
  }
  else
  {
    pyInput +=
      "ncan = 1\n"
      "density = [" + m_uiForm.absp_lesamden->text() + ", 0.0, 0.0 ]\n"
      "sigs = [" + m_uiForm.absp_lesamsigs->text() + ", 0.0, 0.0]\n"
      "siga = [" + m_uiForm.absp_lesamsiga->text() + ", 0.0, 0.0]\n";
  }

  pyInput +=
    "geom = '" + geom + "'\n"
    "beam = [3.0, 0.5*" + width + ", -0.5*" + width + ", 2.0, -2.0, 0.0, 3.0, 0.0, 3.0]\n"
    "size = " + size + "\n"
    "avar = " + m_uiForm.absp_leavar->text() + "\n"
    "plotOpt = '" + m_uiForm.absp_cbPlotOutput->currentText() + "'\n"
    "IndirectAbsCor.AbsRunFeeder(inputws, geom, beam, ncan, size, density, sigs, siga, avar, plotOpt=plotOpt)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void IndirectDataAnalysis::absf2pShape(int index)
{
  m_uiForm.absp_swShapeDetails->setCurrentIndex(index);
  // Meaning of the "avar" variable changes depending on shape selection
  if ( index == 0 ) { m_uiForm.absp_lbAvar->setText("Can Angle to Beam"); }
  else if ( index == 1 ) { m_uiForm.absp_lbAvar->setText("Step Size"); }
}

void IndirectDataAnalysis::absf2pUseCanChecked(bool checked)
{
  // Disable thickness fields/labels/asterisks.
  m_uiForm.absp_lbtc1->setEnabled(checked);
  m_uiForm.absp_lbtc2->setEnabled(checked);
  m_uiForm.absp_letc1->setEnabled(checked);
  m_uiForm.absp_letc2->setEnabled(checked);
  m_uiForm.absp_valtc1->setVisible(checked);
  m_uiForm.absp_valtc2->setVisible(checked);

  // Disable R3 field/label/asterisk.
  m_uiForm.absp_lbR3->setEnabled(checked);
  m_uiForm.absp_ler3->setEnabled(checked);
  m_uiForm.absp_valR3->setVisible(checked);

  // Disable "Can Details" group and asterisks.
  m_uiForm.absp_gbCan->setEnabled(checked);
  m_uiForm.absp_valCanden->setVisible(checked);
  m_uiForm.absp_valCansigs->setVisible(checked);
  m_uiForm.absp_valCansiga->setVisible(checked);
  
  // Workaround for "disabling" title of the QGroupBox.
  QPalette palette;
  if(checked)
    palette.setColor(
      QPalette::Disabled, 
      QPalette::WindowText,
      QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
  else
    palette.setColor(
      QPalette::Active, 
      QPalette::WindowText,
      QApplication::palette().color(QPalette::Active, QPalette::WindowText));

  m_uiForm.absp_gbCan->setPalette(palette);
}

void IndirectDataAnalysis::absf2pTCSync()
{
  if ( m_uiForm.absp_letc2->text() == "" )
  {
    QString val = m_uiForm.absp_letc1->text();
    m_uiForm.absp_letc2->setText(val);
  }
}

void IndirectDataAnalysis::abscorRun()
{
  QString geom = m_uiForm.abscor_cbGeometry->currentText();
  if ( geom == "Flat" )
  {
    geom = "flt";
  }
  else if ( geom == "Cylinder" )
  {
    geom = "cyl";
  }

  QString pyInput = "from IndirectDataAnalysis import abscorFeeder, loadNexus\n";

  if ( m_uiForm.abscor_cbSampleInputType->currentText() == "File" )
  {
    pyInput +=
      "sample = loadNexus(r'" + m_uiForm.abscor_sample->getFirstFilename() + "')\n";
  }
  else
  {
    pyInput +=
      "sample = '" + m_uiForm.abscor_wsSample->currentText() + "'\n";
  }

  if ( m_uiForm.abscor_ckUseCan->isChecked() )
  {
    if ( m_uiForm.abscor_cbContainerInputType->currentText() == "File" )
    {
      pyInput +=
        "container = loadNexus(r'" + m_uiForm.abscor_can->getFirstFilename() + "')\n";
    }
    else
    {
      pyInput +=
        "container = '" + m_uiForm.abscor_wsContainer->currentText() + "'\n";
    }
  }
  else
  {
    pyInput += "container = ''\n";
  }

  pyInput += "geom = '" + geom + "'\n";


  if ( m_uiForm.abscor_ckUseCorrections->isChecked() )
  {
    pyInput += "useCor = True\n";
  }
  else
  {
    pyInput += "useCor = False\n";
  }

  pyInput += "abscorFeeder(sample, container, geom, useCor)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}

} //namespace CustomInterfaces
} //namespace MantidQt
