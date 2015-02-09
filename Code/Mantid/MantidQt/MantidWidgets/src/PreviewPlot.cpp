//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtMantidWidgets/PreviewPlot.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

#include <QAction>
#include <QBrush>
#include <QHBoxLayout>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("PreviewPlot");
}


PreviewPlot::PreviewPlot(QWidget *parent, bool init) : API::MantidWidget(parent),
  m_removeObserver(*this, &PreviewPlot::handleRemoveEvent),
  m_replaceObserver(*this, &PreviewPlot::handleReplaceEvent),
  m_init(init), m_allowPan(false), m_allowZoom(false),
  m_plot(NULL), m_curves(),
  m_magnifyTool(NULL), m_panTool(NULL), m_zoomTool(NULL),
  m_contextMenu(new QMenu(this))
{
  if(init)
  {
    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();
    ads.notificationCenter.addObserver(m_removeObserver);
    ads.notificationCenter.addObserver(m_replaceObserver);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    m_plot = new QwtPlot(this);
    m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(m_plot);

    this->setLayout(mainLayout);
  }

  // Setup plot manipulation tools
  m_zoomTool = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner, QwtPicker::AlwaysOff, m_plot->canvas());
  m_zoomTool->setEnabled(false);

  m_panTool = new QwtPlotPanner(m_plot->canvas());
  m_panTool->setEnabled(false);

  m_magnifyTool = new QwtPlotMagnifier(m_plot->canvas());
  m_magnifyTool->setMouseButton(Qt::NoButton);
  m_magnifyTool->setEnabled(false);

  // Handle showing the context menu
  m_plot->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_plot, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

  // Create the plot tool list for context menu
  QMenu *viewToolMenu = new QMenu(m_contextMenu);
  m_plotToolGroup = new QActionGroup(m_contextMenu);
  m_plotToolGroup->setExclusive(true);

  QStringList plotTools;
  plotTools << "None" << "Pan" << "Zoom";

  for(auto it = plotTools.begin(); it != plotTools.end(); ++it)
  {
    QAction *toolAction = new QAction(*it, viewToolMenu);
    toolAction->setCheckable(true);
    connect(toolAction, SIGNAL(toggled(bool)), this, SLOT(handleViewToolSelect(bool)));

    // Add to the menu and action group
    m_plotToolGroup->addAction(toolAction);
    viewToolMenu->addAction(toolAction);

    // None is selected by default
    toolAction->setChecked(*it == "None");
  }

  QAction *viewToolAction = new QAction("View Tool", this);
  viewToolAction->setMenu(viewToolMenu);
  m_contextMenu->addAction(viewToolAction);

  // Create the reset plot view option
  QAction *resetPlotAction = new QAction("Reset Plot", m_contextMenu);
  connect(resetPlotAction, SIGNAL(triggered()), this, SLOT(resetView()));
  m_contextMenu->addAction(resetPlotAction);
}


/**
 * Destructor
 *
 * Removes observers on the ADS.
 */
PreviewPlot::~PreviewPlot()
{
  if(m_init)
  {
    AnalysisDataService::Instance().notificationCenter.removeObserver(m_removeObserver);
    AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
  }
}


/**
 * Gets the background colour of the plot window.
 *
 * @return Plot canvas colour
 */
QColor PreviewPlot::canvasColour()
{
  if(m_plot)
    return m_plot->canvasBackground();

  return QColor();
}


/**
 * Sets the background colour of the plot window.
 *
 * @param colour Plot canvas colour
 */
void PreviewPlot::setCanvasColour(const QColor & colour)
{
  if(m_plot)
    m_plot->setCanvasBackground(QBrush(colour));
}


/**
 * Checks to see if the option to use the pan tool is enabled.
 *
 * @return True if tool is allowed
 */
bool PreviewPlot::allowPan()
{
  return m_allowPan;
}


/**
 * Enables or disables the option to use the pan tool on the plot.
 *
 * @param allow If tool should be allowed
 */
void PreviewPlot::setAllowPan(bool allow)
{
  m_allowPan = allow;
}


/**
 * Checks to see if the option to use the zoom tool is enabled.
 *
 * @return True if tool is allowed
 */
bool PreviewPlot::allowZoom()
{
  return m_allowZoom;
}


/**
 * Enables or disables the option to use the zoom tool on the plot.
 *
 * @param allow If tool should be allowed
 */
void PreviewPlot::setAllowZoom(bool allow)
{
  m_allowZoom = allow;
}


/**
 * Sets the range of the given axis scale to a given range.
 *
 * @param range Pair of values for range
 * @param axisID ID of axis
 */
void PreviewPlot::setAxisRange(QPair<double, double> range, int axisID)
{
  if(range.first > range.second)
    throw std::runtime_error("Supplied range is invalid.");

  m_plot->setAxisScale(axisID, range.first, range.second);
  replot();
}


/**
 * Gets the X range of a curve given a pointer to the workspace.
 *
 * @param ws Pointer to workspace
 */
QPair<double, double> PreviewPlot::getCurveRange(const Mantid::API::MatrixWorkspace_const_sptr ws)
{
  if(!m_curves.contains(ws))
    throw std::runtime_error("Workspace not on preview plot.");

  size_t numPoints = m_curves[ws]->data().size();

  if(numPoints < 2)
    return qMakePair(0.0, 0.0);

  double low = m_curves[ws]->data().x(0);
  double high = m_curves[ws]->data().x(numPoints - 1);

  return qMakePair(low, high);
}


/**
 * Gets the X range of a curve given its name.
 *
 * @param wsName Name of workspace
 */
QPair<double, double> PreviewPlot::getCurveRange(const QString & wsName)
{
  // Try to get a pointer from the name
  std::string wsNameStr = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsName.toStdString());

  if(!ws)
    throw std::runtime_error(wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  return getCurveRange(ws);
}


/**
 * Adds a workspace to the preview plot given a pointer to it.
 *
 * @param wsName Name of workspace in ADS
 * @param specIndex Spectrrum index to plot
 * @param curveColour Colour of curve to plot
 */
void PreviewPlot::addSpectrum(const MatrixWorkspace_const_sptr ws, const size_t specIndex,
    const QColor & curveColour)
{
  // Check the spectrum index is in range
  if(specIndex >= ws->getNumberHistograms())
    throw std::runtime_error("Workspace index is out of range, cannot plot.");

  // Check the X axis is large enough
  if(ws->readX(0).size() < 2)
    throw std::runtime_error("X axis is too small to generate a histogram plot.");

  // Create the plot data
  const bool logScale(false), distribution(false);
  QwtWorkspaceSpectrumData wsData(*ws, static_cast<int>(specIndex), logScale, distribution);

  // Remove any existing curves
  if(m_curves.contains(ws))
    removeCurve(m_curves[ws]);

  // Create the new curve
  m_curves[ws] = new QwtPlotCurve();
  m_curves[ws]->setData(wsData);
  m_curves[ws]->setPen(curveColour);
  m_curves[ws]->attach(m_plot);

  // Replot
  m_plot->replot();
}


/**
 * Adds a workspace to the preview plot given its name.
 *
 * @param wsName Name of workspace in ADS
 * @param specIndex Spectrrum index to plot
 * @param curveColour Colour of curve to plot
 */
void PreviewPlot::addSpectrum(const QString & wsName, const size_t specIndex,
    const QColor & curveColour)
{
  // Try to get a pointer from the name
  std::string wsNameStr = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsName.toStdString());

  if(!ws)
    throw std::runtime_error(wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  addSpectrum(ws, specIndex, curveColour);
}


/**
 * Removes spectra from a gievn workspace from the plot given a pointer to it.
 *
 * @param ws Pointer to workspace
 */
void PreviewPlot::removeSpectrum(const MatrixWorkspace_const_sptr ws)
{
  // Remove the curve object
  if(m_curves.contains(ws))
    removeCurve(m_curves[ws]);

  // Get the curve from the map
  auto it = m_curves.find(ws);

  // Remove the curve from the map
  if(it != m_curves.end())
    m_curves.erase(it);
}


/**
 * Removes spectra from a gievn workspace from the plot given its name.
 *
 * @param wsName Name of workspace
 */
void PreviewPlot::removeSpectrum(const QString & wsName)
{
  // Try to get a pointer from the name
  std::string wsNameStr = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsNameStr);

  if(!ws)
    throw std::runtime_error(wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  removeSpectrum(ws);
}


/**
 * Toggles the pan plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void PreviewPlot::togglePanTool(bool enabled)
{
  // First disbale the zoom tool
  if(enabled && m_zoomTool->isEnabled())
    m_zoomTool->setEnabled(false);

  m_panTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}


/**
 * Toggles the zoom plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void PreviewPlot::toggleZoomTool(bool enabled)
{
  // First disbale the pan tool
  if(enabled && m_panTool->isEnabled())
    m_panTool->setEnabled(false);

  m_zoomTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}


/**
 * Resets the view to a sensible default.
 */
void PreviewPlot::resetView()
{
  // Auto scale the axis
  m_plot->setAxisAutoScale(QwtPlot::xBottom);
  m_plot->setAxisAutoScale(QwtPlot::yLeft);

  // Set this as the default zoom level
  m_zoomTool->setZoomBase(true);
}


/**
 * Resizes the X axis scale range to exactly fir the curves currently
 * plotted on it.
 */
void PreviewPlot::resizeX()
{
  double low = DBL_MAX;
  double high = DBL_MIN;

  for(auto it = m_curves.begin(); it != m_curves.end(); ++it)
  {
    auto range = getCurveRange(it.key());

    if(range.first < low)
      low = range.first;

    if(range.second > high)
      high = range.second;
  }

  setAxisRange(qMakePair(low, high), QwtPlot::xBottom);
}


/**
 * Removes all curves from the plot.
 */
void PreviewPlot::clear()
{
  for(auto it = m_curves.begin(); it != m_curves.end(); ++it)
    removeCurve(it.value());

  m_curves.clear();

  replot();
}


/**
 * Replots the curves shown on the plot.
 */
void PreviewPlot::replot()
{
  m_plot->replot();
}


void PreviewPlot::handleRemoveEvent(WorkspacePreDeleteNotification_ptr pNf)
{
  //TODO
}


void PreviewPlot::handleReplaceEvent(WorkspaceAfterReplaceNotification_ptr pNf)
{
  //TODO
}


/**
 * Removes a curve from the plot.
 *
 * @param curve Curve to remove
 */
void PreviewPlot::removeCurve(QwtPlotCurve * curve)
{
  if(!curve)
    return;

  // Take it off the plot
  curve->attach(NULL);

  // Delete it
  delete curve;
  curve = NULL;
}


/**
 * Handles displaying the context menu when a user right clicks on the plot.
 *
 * @param position Position at which to show menu
 */
void PreviewPlot::showContextMenu(QPoint position)
{
  // Show the context menu
  m_contextMenu->popup(m_plot->mapToGlobal(position));
}


/**
 * Handles the view tool being selected from the context menu.
 *
 * @param checked If the option was checked
 */
void PreviewPlot::handleViewToolSelect(bool checked)
{
  if(!checked)
    return;

  QAction *selectedPlotType = m_plotToolGroup->checkedAction();
  if(!selectedPlotType)
    return;

  QString selectedTool = selectedPlotType->text();
  if(selectedTool == "None")
  {
    togglePanTool(false);
    toggleZoomTool(false);
  }
  else if(selectedTool == "Pan")
  {
    togglePanTool(true);
  }
  else if(selectedTool == "Zoom")
  {
    toggleZoomTool(true);
  }
}
