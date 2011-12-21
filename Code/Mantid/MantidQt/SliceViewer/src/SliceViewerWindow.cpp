#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/VMD.h"
#include "MantidQtSliceViewer/LineViewer.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include <qboxlayout.h>
#include <qmainwindow.h>
#include <qlayout.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::SliceViewer;

namespace MantidQt
{
namespace SliceViewer
{

/** SliceViewerWindow constructor.
 * Creates it with NULL parent so that it does not stay on top of the main window on Windows.
 *
 * @param wsName :: name of the workspace being viewer
 * @param label
 * @param f
 * @return
 */
SliceViewerWindow::SliceViewerWindow(const QString& wsName, const QString& label, Qt::WFlags f)
 : QMainWindow(NULL, f),
   WorkspaceObserver()
{
  // Set the window icon
  QIcon icon;
  icon.addFile(QString::fromUtf8(":/SliceViewer/icons/SliceViewerWindow_icon.png"), QSize(), QIcon::Normal, QIcon::Off);
  this->setWindowIcon(icon);

  // Get the workspace
  m_wsName = wsName.toStdString();
  m_ws = boost::dynamic_pointer_cast<IMDWorkspace>( AnalysisDataService::Instance().retrieve(m_wsName) );

  // Watch for the deletion of the associated workspace
  observeAfterReplace();
  observeDelete();
  observeADSClear();

  // Set up the window
  m_label = label;
  QString caption = QString("Slice Viewer (") + wsName + QString(")");
  if (!m_label.isEmpty()) caption += QString(" ") + m_label;
  this->setCaption(caption);
  this->resize(500, 500);

  // Create the m_slicer and add it to the MDI window
  QLayout * layout = this->layout();
  if (!layout)
  {
    layout = new QVBoxLayout(this);
    this->setLayout(layout);
  }

  // Make a horizontal splitter
  m_splitter = new QSplitter(this);
  m_splitter->setObjectName(QString::fromUtf8("splitter"));
  m_splitter->setOrientation(Qt::Horizontal);
  m_splitter->setOpaqueResize(false);

  m_slicer = new SliceViewer(m_splitter);
  m_liner = new LineViewer(m_splitter);
  m_liner->setVisible(false);

  this->setCentralWidget(m_splitter);

  m_splitter->addWidget(m_slicer);
  m_splitter->addWidget(m_liner);

  // Connect WorkspaceObserver signals
  connect(this,SIGNAL(needToClose()),this,SLOT(closeWindow()));
  connect(this,SIGNAL(needToUpdate()),this,SLOT(updateWorkspace()));

  // When the Slicer changes workspace, carry over to THIS and LineViewer
  QObject::connect( m_slicer, SIGNAL(workspaceChanged()),
            this, SLOT(slicerWorkspaceChanged()) );

  // Connect the SliceViewer and the LineViewer together
  QObject::connect( m_slicer, SIGNAL(showLineViewer(bool)),
            this, SLOT(showLineViewer(bool)) );
  QObject::connect( m_slicer, SIGNAL(changedShownDim(size_t, size_t)),
            m_liner, SLOT(setFreeDimensions(size_t, size_t)) );
  QObject::connect( m_slicer, SIGNAL(changedSlicePoint(Mantid::Kernel::VMD)),
            this, SLOT(changedSlicePoint(Mantid::Kernel::VMD)) );

  // Drag-dropping the line around
  QObject::connect( m_slicer->getLineOverlay(), SIGNAL(lineChanging(QPointF, QPointF, double)),
            this, SLOT(lineChanging(QPointF, QPointF, double)) );
  QObject::connect( m_slicer->getLineOverlay(), SIGNAL(lineChanged(QPointF, QPointF, double)),
            this, SLOT(lineChanged(QPointF, QPointF, double)) );

  // Link back the LineViewer to the SliceViewer's line overlay.
  QObject::connect( m_liner, SIGNAL(changedStartOrEnd(Mantid::Kernel::VMD, Mantid::Kernel::VMD)),
            this, SLOT(changeStartOrEnd(Mantid::Kernel::VMD, Mantid::Kernel::VMD)) );
  QObject::connect( m_liner, SIGNAL(changedPlanarWidth(double)),
            this, SLOT(changePlanarWidth(double)) );

  // Set the current workspace
  this->updateWorkspace();
}


SliceViewerWindow::~SliceViewerWindow()
{
}

//------------------------------------------------------------------------------------------------
/** Get the SliceViewer widget inside the SliceViewerWindow.
 * This is the main widget for controlling the 2D views
 * and slice points.
 *
 * @return a pointer to the SliceViewer widget.
 */
MantidQt::SliceViewer::SliceViewer* SliceViewerWindow::getSlicer()
{ return m_slicer; }

//------------------------------------------------------------------------------------------------
/** Get the LineViewer widget inside the SliceViewerWindow.
 * This is the widget for controlling the 1D line integration
 * settings.
 *
 * @return a pointer to the LineViewer widget.
 */
MantidQt::SliceViewer::LineViewer* SliceViewerWindow::getLiner()
{ return m_liner; }


//------------------------------------------------------------------------------------------------
/** @return the label that was attached to this SliceViewerWindow's title */
const QString& SliceViewerWindow::getLabel() const
{
  return m_label;
}


//------------------------------------------------------------------------------------------------
void SliceViewerWindow::resizeEvent(QResizeEvent * /*event*/)
{
//  if (m_liner->isVisible())
//    m_lastLinerWidth = m_liner->width();
}


//------------------------------------------------------------------------------------------------
/** Slot to close the window */
void SliceViewerWindow::closeWindow()
{
  //askOnCloseEvent(false); //(MdiSubWindow)
  close();
}

//------------------------------------------------------------------------------------------------
/** Slot to replace the workspace being looked at. */
void SliceViewerWindow::updateWorkspace()
{
  m_liner->setWorkspace(m_ws);
  m_slicer->setWorkspace(m_ws);
}

//------------------------------------------------------------------------------------------------
/** Slot called when the SliceViewer changes which workspace
 * is being viewed. */
void SliceViewerWindow::slicerWorkspaceChanged()
{
  m_ws = m_slicer->getWorkspace();
  // Propagate the change to Liner
  m_liner->setWorkspace(m_ws);
}

//------------------------------------------------------------------------------------------------
/** Slot called when the line viewer should be shown/hidden */
void SliceViewerWindow::showLineViewer(bool visible)
{
  int linerWidth = m_liner->width();
  if (linerWidth <= 0) linerWidth = m_lastLinerWidth;
  if (linerWidth <= 0) linerWidth = m_liner->sizeHint().width();
  // Account for the splitter handle
  linerWidth += m_splitter->handleWidth() - 3;

  this->setUpdatesEnabled(false);
  if (visible && !m_liner->isVisible())
  {
    // Expand the window to include the liner
    int w = this->width() + linerWidth + 2;
    m_liner->setVisible(true);
    // If the right splitter was hidden, show it
    QList<int> sizes = m_splitter->sizes();
    if (m_lastLinerWidth > 0)
    {
      sizes[1] = m_lastLinerWidth;
      m_splitter->setSizes(sizes);
    }
    this->resize(w, this->height());
  }
  else if (!visible && m_liner->isVisible())
  {
    // Shrink the window to exclude the liner
    int w = this->width() - (m_liner->width() + m_splitter->handleWidth());
    if (m_liner->width() > 0)
      m_lastLinerWidth = m_liner->width();
    m_liner->setVisible(false);
    QApplication::processEvents();
    this->resize(w, this->height());
    // This call is necessary to allow resizing smaller than would be allowed if both left/right widgets were visible.
    QApplication::processEvents();
    this->resize(w, this->height());
  }
  else
  {
    // Toggle the visibility of the liner
    m_liner->setVisible(visible);
  }
  this->setUpdatesEnabled(true);

}


//------------------------------------------------------------------------------------------------
/** Using the positions from the LineOverlay, set the values in the LineViewer,
 * but don't update view. */
void SliceViewerWindow::setLineViewerValues(QPointF start2D, QPointF end2D, double width)
{
  VMD start = m_slicer->getSlicePoint();
  VMD end = start;
  start[m_slicer->getDimX()] = start2D.x();
  start[m_slicer->getDimY()] = start2D.y();
  end[m_slicer->getDimX()] = end2D.x();
  end[m_slicer->getDimY()] = end2D.y();
  m_liner->setStart(start);
  m_liner->setEnd(end);
  m_liner->setPlanarWidth(width);
}

//------------------------------------------------------------------------------------------------
/** Slot called when the line overlay position is changing (being dragged) */
void SliceViewerWindow::lineChanging(QPointF start2D, QPointF end2D, double width)
{
  setLineViewerValues(start2D, end2D, width);
  m_liner->showPreview();
}

/** Slot called when the line overlay drag is released */
void SliceViewerWindow::lineChanged(QPointF start2D, QPointF end2D, double width)
{
  setLineViewerValues(start2D, end2D, width);
  m_liner->apply();
}

/** Slot called when changing the slice point of the 2D view
 * (keeping the line in the same 2D point) */
void SliceViewerWindow::changedSlicePoint(Mantid::Kernel::VMD slice)
{
  UNUSED_ARG(slice);
  setLineViewerValues( m_slicer->getLineOverlay()->getPointA() ,  m_slicer->getLineOverlay()->getPointB(),  m_slicer->getLineOverlay()->getWidth() );
  m_liner->showPreview();
}

/** Slot called when the user manually changes start/end points in the text box,
 * so that the graph updates
 * @param start :: start coordinates
 * @param end :: end coordinates
 */
void SliceViewerWindow::changeStartOrEnd(Mantid::Kernel::VMD start, Mantid::Kernel::VMD end)
{
  QPointF start2D(start[m_slicer->getDimX()], start[m_slicer->getDimY()]);
  QPointF end2D(end[m_slicer->getDimX()], end[m_slicer->getDimY()]);
  m_slicer->getLineOverlay()->blockSignals(true);
  m_slicer->getLineOverlay()->setPointA(start2D);
  m_slicer->getLineOverlay()->setPointB(end2D);
  m_slicer->getLineOverlay()->blockSignals(false);
  m_slicer->getLineOverlay()->update();
}

/** Slot called when the user manually changes the width in the text box,
 * to update the gui.
 * @param width :: new planar width.
 */
void SliceViewerWindow::changePlanarWidth(double width)
{
  m_slicer->getLineOverlay()->blockSignals(true);
  m_slicer->getLineOverlay()->setWidth(width);
  m_slicer->getLineOverlay()->blockSignals(false);
  m_slicer->getLineOverlay()->update();
}


//------------------------------------------------------------------------------------------------
/** Signal to close this window if the workspace has just been deleted */
void SliceViewerWindow::deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::IMDWorkspace * ws_ptr = dynamic_cast<Mantid::API::IMDWorkspace*>(ws.get());
  if (!ws_ptr) return;
  if (ws_ptr == m_ws.get() || wsName == m_wsName)
  {
    emit needToClose();
  }
}

//------------------------------------------------------------------------------------------------
/** Signal that the workspace being looked at was just replaced with a different one */
void SliceViewerWindow::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::IMDWorkspace_sptr new_ws = boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(ws);
  if (!new_ws) return;
  if (new_ws.get() == m_ws.get() || wsName == m_wsName)
  {
    m_ws = new_ws;
    emit needToUpdate();
  }
}

}//namespace SliceViewer
}//namespace MantidQt
