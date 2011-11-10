#include "MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h"

#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorSelectionDialog.h"
#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"
#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"
#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h"

#include "MantidQtAPI/InterfaceManager.h"
#include "MantidKernel/DynamicFactory.h"

#include <pqActiveObjects.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqApplicationCore.h>
#include <pqLoadDataReaction.h>
#include <pqObjectBuilder.h>
#include <pqObjectInspectorWidget.h>
#include <pqParaViewBehaviors.h>
#include <pqPipelineSource.h>
#include <pqPVApplicationCore.h>
#include <pqRenderView.h>
#include <pqStatusBar.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMReaderFactory.h>
#include <vtksys/SystemTools.hxx>

#include <pqPipelineRepresentation.h>

// Used for plugin mode
#include <pqAlwaysConnectedBehavior.h>
#include <pqAutoLoadPluginXMLBehavior.h>
#include <pqCommandLineOptionsBehavior.h>
#include <pqCrashRecoveryBehavior.h>
#include <pqDataTimeStepBehavior.h>
#include <pqDefaultViewBehavior.h>
#include <pqDeleteBehavior.h>
#include <pqFixPathsInStateFilesBehavior.h>
#include <pqObjectPickingBehavior.h>
//#include <pqPersistentMainWindowStateBehavior.h>
#include <pqPipelineContextMenuBehavior.h>
//#include <pqPluginActionGroupBehavior.h>
//#include <pqPluginDockWidgetsBehavior.h>
#include <pqPluginManager.h>
#include <pqPVNewSourceBehavior.h>
#include <pqQtMessageHandlerBehavior.h>
#include <pqSpreadSheetVisibilityBehavior.h>
#include <pqStandardViewModules.h>
#include <pqUndoRedoBehavior.h>
#include <pqViewFrameActionsBehavior.h>
#include <pqVerifyRequiredPluginBehavior.h>

#include <QAction>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QModelIndex>
#include <QWidget>

#include <iostream>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
using namespace MantidQt::API;

REGISTER_VATESGUI(MdViewerWidget)

MdViewerWidget::MdViewerWidget() : VatesViewerInterface()
{
  this->isPluginInitialized = false;
  this->pluginMode = true;
  this->colorDialog = NULL;
}

MdViewerWidget::MdViewerWidget(QWidget *parent) : VatesViewerInterface(parent)
{
  this->checkEnvSetup();
  // We're in the standalone application mode
  this->isPluginInitialized = false;
  this->pluginMode = false;
  this->colorDialog = NULL;
  this->setupUiAndConnections();
  // FIXME: This doesn't allow a clean split of the classes. I will need
  //        to investigate creating the individual behaviors to see if that
  //        eliminates the dependence on the QMainWindow.
  if (parent->inherits("QMainWindow"))
  {
    QMainWindow *mw = qobject_cast<QMainWindow *>(parent);
    new pqParaViewBehaviors(mw, mw);
  }
  this->setupMainView();
}

MdViewerWidget::~MdViewerWidget()
{
}

void MdViewerWidget::checkEnvSetup()
{
  QString pv_plugin_path = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
  if (pv_plugin_path.isEmpty())
  {
    throw std::runtime_error("PV_PLUGIN_PATH not setup.\nVates plugins will not be available.\n"
                             "Further use will cause the program to crash.\nPlease exit and "
                             "set this variable.");
  }
}

void MdViewerWidget::setupUiAndConnections()
{
  this->ui.setupUi(this);
  this->ui.splitter_2->setStretchFactor(1, 1);
  this->ui.statusBar->setSizeGripEnabled(false);

  // Unset the connections since the views aren't up yet.
  this->removeProxyTabWidgetConnections();

  QObject::connect(this->ui.modeControlWidget,
                   SIGNAL(executeSwitchViews(ModeControlWidget::Views)),
                   this, SLOT(switchViews(ModeControlWidget::Views)));
}

void MdViewerWidget::setupMainView()
{
  // Commented this out to only use Mantid supplied readers
  // Initialize all readers available to ParaView. Now our application can load
  // all types of datasets supported by ParaView.
  //vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->RegisterPrototypes("sources");

  // Set the standard view as the default
  this->currentView = this->setMainViewWidget(this->ui.viewWidget,
                                              ModeControlWidget::STANDARD);
  this->currentView->installEventFilter(this);

  // Create a layout to manage the view properly
  this->viewLayout = new QHBoxLayout(this->ui.viewWidget);
  this->viewLayout->setMargin(0);
  this->viewLayout->setStretch(0, 1);
  this->viewLayout->addWidget(this->currentView);

  this->setParaViewComponentsForView();
}

void MdViewerWidget::setupPluginMode()
{
  this->createAppCoreForPlugin();
  this->checkEnvSetup();
  this->setupUiAndConnections();
  if (!this->isPluginInitialized)
  {
    this->setupParaViewBehaviors();
    this->createMenus();
  }
  this->setupMainView();
}

void MdViewerWidget::createAppCoreForPlugin()
{
  if (!pqApplicationCore::instance())
  {
    int argc = 1;
    char *argv[] = {"/tmp/MantidPlot"};
    new pqPVApplicationCore(argc, argv);
  }
  else
  {
    this->isPluginInitialized = true;
  }
}

void MdViewerWidget::setupParaViewBehaviors()
{
  // Register ParaView interfaces.
  pqPluginManager* pgm = pqApplicationCore::instance()->getPluginManager();

  // * adds support for standard paraview views.
  pgm->addInterface(new pqStandardViewModules(pgm));

  // Load plugins distributed with application.
  pqApplicationCore::instance()->loadDistributedPlugins();

  // Define application behaviors.
  new pqQtMessageHandlerBehavior(this);
  new pqDataTimeStepBehavior(this);
  new pqViewFrameActionsBehavior(this);
  new pqSpreadSheetVisibilityBehavior(this);
  new pqPipelineContextMenuBehavior(this);
  new pqDefaultViewBehavior(this);
  new pqAlwaysConnectedBehavior(this);
  new pqPVNewSourceBehavior(this);
  new pqDeleteBehavior(this);
  new pqUndoRedoBehavior(this);
  new pqCrashRecoveryBehavior(this);
  new pqAutoLoadPluginXMLBehavior(this);
  //new pqPluginDockWidgetsBehavior(mainWindow);
  new pqVerifyRequiredPluginBehavior(this);
  //new pqPluginActionGroupBehavior(mainWindow);
  new pqFixPathsInStateFilesBehavior(this);
  new pqCommandLineOptionsBehavior(this);
  //new pqPersistentMainWindowStateBehavior(mainWindow);
  new pqObjectPickingBehavior(this);
}

void MdViewerWidget::connectLoadDataReaction(QAction *action)
{
  // We want the actionLoad to result in the showing up the ParaView's OpenData
  // dialog letting the user pick from one of the supported file formats.
  this->dataLoader = new pqLoadDataReaction(action);
  QObject::connect(this->dataLoader, SIGNAL(loadedData(pqPipelineSource*)),
                   this, SLOT(onDataLoaded(pqPipelineSource*)));
}

void MdViewerWidget::removeProxyTabWidgetConnections()
{
  QObject::disconnect(&pqActiveObjects::instance(), 0,
                      this->ui.proxyTabWidget, 0);
}

ViewBase* MdViewerWidget::setMainViewWidget(QWidget *container,
                                            ModeControlWidget::Views v)
{
  ViewBase *view;
  switch(v)
  {
  case ModeControlWidget::STANDARD:
  {
    view = new StandardView(container);
  }
  break;
  case ModeControlWidget::THREESLICE:
  {
    view = new ThreeSliceView(container);
  }
  break;
  case ModeControlWidget::MULTISLICE:
  {
    view = new MultiSliceView(container);
  }
  break;
  case ModeControlWidget::SPLATTERPLOT:
  {
    view = new SplatterPlotView(container);
  }
  break;
  default:
    view = NULL;
    break;
  }
  return view;
}

void MdViewerWidget::setParaViewComponentsForView()
{
  // Extra setup stuff to hook up view to other items
  this->ui.proxyTabWidget->setupDefaultConnections();
  this->ui.proxyTabWidget->setView(this->currentView->getView());
  this->ui.proxyTabWidget->setShowOnAccept(true);
  this->ui.pipelineBrowser->setActiveView(this->currentView->getView());
  QObject::connect(this->ui.proxyTabWidget->getObjectInspector(),
                   SIGNAL(postaccept()),
                   this, SLOT(checkForUpdates()));
  QObject::connect(this->currentView, SIGNAL(triggerAccept()),
                   this->ui.proxyTabWidget->getObjectInspector(),
                   SLOT(accept()));
  if (this->currentView->inherits("MultiSliceView"))
  {
    QObject::connect(this->ui.pipelineBrowser,
                     SIGNAL(clicked(const QModelIndex &)),
                     static_cast<MultiSliceView *>(this->currentView),
                     SLOT(selectIndicator()));
    QObject::connect(this->ui.proxyTabWidget->getObjectInspector(),
                     SIGNAL(accepted()),
                     static_cast<MultiSliceView *>(this->currentView),
                     SLOT(updateSelectedIndicator()));
  }

  QObject::connect(this->currentView, SIGNAL(setViewsStatus(bool)),
                   this->ui.modeControlWidget, SLOT(enableViewButtons(bool)));

  // Set color selection widget <-> view signals/slots
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorMapChanged(const pqColorMapModel *)),
                   this->currentView,
                   SLOT(onColorMapChange(const pqColorMapModel *)));
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorScaleChanged(double, double)),
                   this->currentView,
                   SLOT(onColorScaleChange(double, double)));
  QObject::connect(this->currentView, SIGNAL(dataRange(double, double)),
                   this->ui.colorSelectionWidget,
                   SLOT(setColorScaleRange(double, double)));
  QObject::connect(this->ui.colorSelectionWidget, SIGNAL(autoScale()),
                   this->currentView, SLOT(onAutoScale()));
  QObject::connect(this->ui.colorSelectionWidget, SIGNAL(logScale(int)),
                   this->currentView, SLOT(onLogScale(int)));

  // Set animation (time) control widget <-> view signals/slots.
  QObject::connect(this->currentView,
                   SIGNAL(setAnimationControlState(bool)),
                   this->ui.timeControlWidget,
                   SLOT(enableAnimationControls(bool)));
  QObject::connect(this->currentView,
                   SIGNAL(setAnimationControlInfo(double, double, int)),
                   this->ui.timeControlWidget,
                   SLOT(updateAnimationControls(double, double, int)));
}

void MdViewerWidget::onDataLoaded(pqPipelineSource* source)
{
  UNUSED_ARG(source);
  this->renderAndFinalSetup();
}

void MdViewerWidget::renderWorkspace(QString wsname, int wstype)
{
  QString sourcePlugin = "";
  if (VatesViewerInterface::PEAKS == wstype)
  {
    sourcePlugin = "Peaks Source";
  }
  else
  {
    sourcePlugin = "MDEW Source";
  }

  this->currentView->setPluginSource(sourcePlugin, wsname);
  this->renderAndFinalSetup();
}

void MdViewerWidget::renderAndFinalSetup()
{
  this->currentView->render();
  this->currentView->checkView();
  this->currentView->setTimeSteps();
}

void MdViewerWidget::checkForUpdates()
{
  vtkSMProxy *proxy = pqActiveObjects::instance().activeSource()->getProxy();
  if (strcmp(proxy->GetXMLName(), "MDEWRebinningCutter") == 0)
  {
    this->currentView->resetDisplay();
    //this->currentView->getView()->resetCamera();
    this->currentView->onAutoScale();
    this->currentView->setTimeSteps(true);
  }
  if (QString(proxy->GetXMLName()).contains("Threshold"))
  {
    vtkSMDoubleVectorProperty *range = \
        vtkSMDoubleVectorProperty::SafeDownCast(\
          proxy->GetProperty("ThresholdBetween"));
    this->ui.colorSelectionWidget->setColorScaleRange(range->GetElement(0),
                                                      range->GetElement(1));
  }
}

void MdViewerWidget::switchViews(ModeControlWidget::Views v)
{
  this->removeProxyTabWidgetConnections();
  this->hiddenView = this->setMainViewWidget(this->ui.viewWidget, v);
  this->hiddenView->hide();
  this->viewLayout->removeWidget(this->currentView);
  this->swapViews();
  this->viewLayout->addWidget(this->currentView);
  this->currentView->installEventFilter(this);
  this->currentView->show();
  this->hiddenView->hide();
  this->setParaViewComponentsForView();
  this->hiddenView->close();
  this->hiddenView->destroyView();
  delete this->hiddenView;
  this->currentView->render();
  this->currentView->correctVisibility(this->ui.pipelineBrowser);
}

void MdViewerWidget::swapViews()
{
  ViewBase *temp;
  temp = this->currentView;
  this->currentView = this->hiddenView;
  this->hiddenView = temp;
}

/**
 * This function allows one to filter the Qt events and look for a hide
 * event. It then executes source cleanup and view mode switch if the viewer
 * is in plugin mode.
 * @param obj the subject of the event
 * @param ev the actual event
 * @return true if the event was handled
 */
bool MdViewerWidget::eventFilter(QObject *obj, QEvent *ev)
{
  if (this->currentView == obj)
  {
    if (this->pluginMode && QEvent::Hide == ev->type())
    {
      pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
      builder->destroySources();
      this->ui.modeControlWidget->setToStandardView();
    }
    return true;
  }
  return VatesViewerInterface::eventFilter(obj, ev);
}

/**
 * This function creates the main view widget specific menu items.
 */
void MdViewerWidget::createMenus()
{
  QMenuBar *menubar;
  if (this->pluginMode)
  {
    menubar = new QMenuBar(this);
  }
  else
  {
    menubar = qobject_cast<QMainWindow *>(this->parentWidget())->menuBar();
  }

  QMenu *viewMenu = menubar->addMenu(QApplication::tr("&View"));

  QAction *colorAction = new QAction(QApplication::tr("&Color Options"), this);
  colorAction->setShortcut(QKeySequence::fromString("Ctrl+Shift+C"));
  colorAction->setStatusTip(QApplication::tr("Open the color options dialog."));
  QObject::connect(colorAction, SIGNAL(triggered()),
                   this, SLOT(onColorOptions()));
  viewMenu->addAction(colorAction);

  if (this->pluginMode)
  {
    this->ui.verticalLayout->insertWidget(0, menubar);
  }
}

/**
 * This function adds the menus defined here to a QMainWindow menu bar.
 * This must be done after the setup of the standalone application so that
 * the MdViewerWidget menus aren't added before the standalone ones.
 */
void MdViewerWidget::addMenus()
{
  this->createMenus();
}

/**
 * This function handles creating the color options dialog box and setting
 * the signal and slot comminucation between it and the current view.
 */
void MdViewerWidget::onColorOptions()
{
  if (NULL == this->colorDialog)
  {
    this->colorDialog = new ColorSelectionDialog(this);

    // Set color selection widget <-> view signals/slots
    QObject::connect(this->colorDialog,
                     SIGNAL(colorMapChanged(const pqColorMapModel *)),
                     this->currentView,
                     SLOT(onColorMapChange(const pqColorMapModel *)));
    QObject::connect(this->colorDialog,
                     SIGNAL(colorScaleChanged(double, double)),
                     this->currentView,
                     SLOT(onColorScaleChange(double, double)));
    QObject::connect(this->currentView, SIGNAL(dataRange(double, double)),
                     this->colorDialog,
                     SLOT(setColorScaleRange(double, double)));
    QObject::connect(this->colorDialog, SIGNAL(autoScale()),
                     this->currentView, SLOT(onAutoScale()));
    QObject::connect(this->colorDialog, SIGNAL(logScale(int)),
                     this->currentView, SLOT(onLogScale(int)));
    this->currentView->onAutoScale();
  }
  this->colorDialog->show();
  this->colorDialog->raise();
  this->colorDialog->activateWindow();
}

}
}
}
