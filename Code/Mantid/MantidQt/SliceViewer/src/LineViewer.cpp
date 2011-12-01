#include "MantidQtSliceViewer/LineViewer.h"
#include <qwt_plot_curve.h>
#include "MantidKernel/VMD.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include <QIntValidator>
#include "MantidAPI/IMDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt
{
namespace SliceViewer
{


LineViewer::LineViewer(QWidget *parent)
 : QWidget(parent),
   m_planeWidth(0),
   m_numBins(100),
   m_allDimsFree(false), m_freeDimX(0), m_freeDimY(1)
{
	ui.setupUi(this);

	// --------- Create the plot -----------------
  m_plotLayout = new QHBoxLayout(ui.frmPlot);
  m_plot = new QwtPlot();
  m_plot->autoRefresh();
  m_plot->setBackgroundColor(QColor(255,255,255)); // White background
  m_plotLayout->addWidget(m_plot, 1);

  // Make the 2 curves
  m_previewCurve = new QwtPlotCurve("Preview");
  m_fullCurve = new QwtPlotCurve("Integrated");
  m_previewCurve->attach(m_plot);
  m_fullCurve->attach(m_plot);
  m_previewCurve->setVisible(false);
  m_fullCurve->setVisible(false);


  // Make the splitter use the minimum size for the controls and not stretch out
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 1);

  //----------- Connect signals -------------
  QObject::connect(ui.btnApply, SIGNAL(clicked()), this, SLOT(apply()));
  QObject::connect(ui.chkAdaptiveBins, SIGNAL(  stateChanged(int)), this, SLOT(adaptiveBinsChanged()));
  QObject::connect(ui.spinNumBins, SIGNAL(valueChanged(int)), this, SLOT(numBinsChanged()));
  QObject::connect(ui.textPlaneWidth, SIGNAL(textEdited(QString)), this, SLOT(widthTextEdited()));

}

LineViewer::~LineViewer()
{

}

//-----------------------------------------------------------------------------------------------
/** With the workspace set, create the dimension text boxes */
void LineViewer::createDimensionWidgets()
{
  // Create all necessary widgets
  if (m_startText.size() < int(m_ws->getNumDims()))
  {
    for (size_t d=m_startText.size(); d<m_ws->getNumDims(); d++)
    {
      QLabel * dimLabel = new QLabel(this);
      dimLabel->setAlignment(Qt::AlignHCenter);
      ui.gridLayout->addWidget(dimLabel, 0, int(d)+1);
      m_dimensionLabel.push_back(dimLabel);

      QLineEdit * startText = new QLineEdit(this);
      QLineEdit * endText = new QLineEdit(this);
      QLineEdit * widthText = new QLineEdit(this);
      startText->setMaximumWidth(100);
      endText->setMaximumWidth(100);
      widthText->setMaximumWidth(100);
      startText->setToolTip("Start point of the line in this dimension");
      endText->setToolTip("End point of the line in this dimension");
      widthText->setToolTip("Width of the line in this dimension");
      startText->setValidator(new QDoubleValidator(startText));
      endText->setValidator(new QDoubleValidator(endText));
      widthText->setValidator(new QDoubleValidator(widthText));
      ui.gridLayout->addWidget(startText, 1, int(d)+1);
      ui.gridLayout->addWidget(endText, 2, int(d)+1);
      ui.gridLayout->addWidget(widthText, 3, int(d)+1);
      m_startText.push_back(startText);
      m_endText.push_back(endText);
      m_widthText.push_back(widthText);
      // Signals that don't change
      QObject::connect(widthText, SIGNAL(textEdited(QString)), this, SLOT(widthTextEdited()));
    }
  }

  // ------ Update the widgets -------------------------
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    m_dimensionLabel[d]->setText( QString::fromStdString(m_ws->getDimension( size_t(d))->getName() ) );
  }
}


//-----------------------------------------------------------------------------------------------
/** Disable any controls relating to dimensions that are not "free"
 * e.g. if you are in the X-Y plane, the Z position cannot be changed.
 */
void LineViewer::updateFreeDimensions()
{
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    // Can always change the start value
    m_startText[d]->setEnabled(true);

    // This dimension is free to move if b == true
    bool b = (m_allDimsFree || d == m_freeDimX || d == m_freeDimY);
    m_endText[d]->setEnabled(b);
    // If all dims are free, width makes little sense. Only allow one (circular) width
    if (m_allDimsFree)
      m_widthText[d]->setVisible(d != 0);
    else
      m_widthText[d]->setVisible(!b);
    m_widthText[d]->setToolTip("Integration width in this dimension.");

    // --- Adjust the signals ---
    m_startText[d]->disconnect();
    m_endText[d]->disconnect();

    if (d == m_freeDimX || d == m_freeDimY)
    {
      // Free dimension - update the preview
      QObject::connect(m_startText[d], SIGNAL(textEdited(QString)), this, SLOT(startEndTextEdited()));
      QObject::connect(m_endText[d], SIGNAL(textEdited(QString)), this, SLOT(startEndTextEdited()));
    }
    else
    {
      // Non-Free dimension - link start to end
      QObject::connect(m_startText[d], SIGNAL(textEdited(QString)), this, SLOT(startLinkedToEndText()));
    }
  }
  if (!m_allDimsFree)
  {
    std::string s = "(in " + m_ws->getDimension(m_freeDimX)->getName() + "-" +  m_ws->getDimension(m_freeDimY)->getName()
        + " plane)";
    ui.lblPlaneWidth->setText(QString::fromStdString(s));
  }

}

//-----------------------------------------------------------------------------------------------
/** Show the start/end/width points in the GUI */
void LineViewer::updateStartEnd()
{
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    m_startText[d]->setText(QString::number(m_start[d]));
    m_endText[d]->setText(QString::number(m_end[d]));
    m_widthText[d]->setText(QString::number(m_width[d]));
  }
  ui.textPlaneWidth->setText(QString::number(m_planeWidth));
}

//-----------------------------------------------------------------------------------------------
/** Read all the text boxes and interpret their values.
 * Does not refresh.
 */
void LineViewer::readTextboxes()
{
  VMD start = m_start;
  VMD end = m_start;
  VMD width = m_width;
  bool allOk = true;
  bool ok;
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    start[d] = m_startText[d]->text().toDouble(&ok);
    allOk = allOk && ok;

    end[d] = m_endText[d]->text().toDouble(&ok);
    allOk = allOk && ok;

    width[d] = m_widthText[d]->text().toDouble(&ok);
    allOk = allOk && ok;
  }
  // Now the planar width
  double tempPlaneWidth = ui.textPlaneWidth->text().toDouble(&ok);
  allOk = allOk && ok;

  // Only continue if all values typed were valid numbers.
  if (!allOk) return;
  m_start = start;
  m_end = end;
  m_width = width;
  m_planeWidth = tempPlaneWidth;
}

//-----------------------------------------------------------------------------------------------
/** Perform the 1D integration using the current parameters */
void LineViewer::apply()
{
  if (m_allDimsFree)
    throw std::runtime_error("Not currently supported with all dimensions free!");

  // BinMD fails on MDHisto.
  IMDHistoWorkspace_sptr mdhws = boost::dynamic_pointer_cast<IMDHistoWorkspace>(m_ws);

  std::string outWsName = m_ws->getName() + "_line" ;
  bool adaptive = ui.chkAdaptiveBins->isChecked();

  // (half-width in the plane)
  double planeWidth = this->getPlanarWidth();
  // Length of the line
  double length = (m_end - m_start).norm();
  double dx = m_end[m_freeDimX] - m_start[m_freeDimX];
  double dy = m_end[m_freeDimY] - m_start[m_freeDimY];
  // Angle of the line
  double angle = atan2(dy, dx);
  double perpAngle = angle + M_PI / 2.0;

  // Build the basis vectors using the angles
  VMD basisX = m_start * 0;
  basisX[m_freeDimX] = cos(angle);
  basisX[m_freeDimY] = sin(angle);
  VMD basisY = m_start * 0;
  basisY[m_freeDimX] = cos(perpAngle);
  basisY[m_freeDimY] = sin(perpAngle);

  // Offset the origin in the plane by the width
  VMD origin = m_start - basisY * planeWidth;
  // And now offset by the width in each direction
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    if ((d != m_freeDimX) && (d != m_freeDimY))
      origin[d] -= m_width[d];
  }

  IAlgorithm * alg = NULL;
  size_t numBins = m_numBins;
  if (adaptive)
  {
    alg = FrameworkManager::Instance().createAlgorithm("SliceMD");
    // "SplitInto" parameter
    numBins = 2;
  }
  else
    alg = FrameworkManager::Instance().createAlgorithm("BinMD");

  alg->setProperty("InputWorkspace", m_ws);
  alg->setPropertyValue("OutputWorkspace", outWsName);
  alg->setProperty("AxisAligned", false);

  // The X basis vector
  alg->setPropertyValue("BasisVectorX", "X,units," + basisX.toString(",")
        + "," + Strings::toString(length) + "," + Strings::toString(numBins) );

  // The Y basis vector, with one bin
  alg->setPropertyValue("BasisVectorY", "Y,units," + basisY.toString(",")
        + "," + Strings::toString(planeWidth*2.0) + ",1" );

  // Now each remaining dimension
  std::string dimChars = "XYZT"; // SlicingAlgorithm::getDimensionChars();
  size_t propNum = 2;
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    if ((d != m_freeDimX) && (d != m_freeDimY))
    {
      // Letter of the dimension
      std::string dim(" "); dim[0] = dimChars[propNum];
      // Simple basis vector going only in this direction
      VMD basis = m_start * 0;
      basis[d] = 1.0;
      // Set the basis vector with the width *2 and 1 bin
      alg->setPropertyValue("BasisVector" + dim, dim +",units," + basis.toString(",")
            + "," + Strings::toString(m_width[d]*2.0) + ",1" );
      propNum++;
      if (propNum >= dimChars.size())
        throw std::runtime_error("LineViewer::apply(): too many dimensions!");
    }
  }

  alg->setPropertyValue("Origin", origin.toString(",") );
  if (!adaptive)
  {
    alg->setProperty("IterateEvents", true);
  }
  alg->execute();

  if (alg->isExecuted())
  {
    //m_sliceWS = alg->getProperty("OutputWorkspace");
    m_sliceWS = boost::dynamic_pointer_cast<IMDWorkspace>(AnalysisDataService::Instance().retrieve(outWsName));
    this->showFull();
  }
  else
  {
    // Unspecified error in algorithm
    this->showPreview();
    m_plot->setTitle("Error integrating workspace - see log.");
  }


}

// ==============================================================================================
// ================================== SLOTS =====================================================
// ==============================================================================================

//-------------------------------------------------------------------------------------------------
/** Slot called when the start text of a non-free dimensions is changed.
 * Changes the end text correspondingly
 */
void LineViewer::startLinkedToEndText()
{
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    if (d != m_freeDimX && d != m_freeDimY)
    {
      // Copy the start text to the end text
      m_endText[d]->setText( m_startText[d]->text() );
    }
  }
  // Call the slot to update the preview
  startEndTextEdited();
}


//-------------------------------------------------------------------------------------------------
/** Slot called when any of the start/end text boxes are edited
 * in GUI. Only changes the values if they are all valid.
 */
void LineViewer::startEndTextEdited()
{
  this->readTextboxes();
  this->showPreview();
  // Send the signal that the positions changed
  emit changedStartOrEnd(m_start, m_end);
}

/** Slot called when the width text box is edited */
void LineViewer::widthTextEdited()
{
  this->readTextboxes();
  //TODO: Don't always auto-apply
  this->apply();
  // Send the signal that the width changed
  emit changedPlanarWidth(this->getPlanarWidth());
}

/** Slot called when the number of bins changes */
void LineViewer::numBinsChanged()
{
  m_numBins = ui.spinNumBins->value();
  //TODO: Don't always auto-apply
  this->apply();
}

/** Slot called when checking the adaptive box */
void LineViewer::adaptiveBinsChanged()
{
  //TODO: Don't always auto-apply
  this->apply();
}


// ==============================================================================================
// ================================== External Getters ==========================================
// ==============================================================================================
/** @return the width in the plane, or the width in dimension 0 if not restricted to a plane */
double LineViewer::getPlanarWidth() const
{
  return m_planeWidth;
}

/// @return the full width vector in each dimensions. The values in the X-Y dimensions should be ignored
Mantid::Kernel::VMD LineViewer::getWidth() const
{
  return m_width;
}


// ==============================================================================================
// ================================== External Setters ==========================================
// ==============================================================================================
//-----------------------------------------------------------------------------------------------
/** Set the workspace being sliced
 *
 * @param ws :: IMDWorkspace */
void LineViewer::setWorkspace(Mantid::API::IMDWorkspace_sptr ws)
{
  m_ws = ws;
  m_width = VMD(ws->getNumDims());
  createDimensionWidgets();
}


/** Set the start point of the line to integrate
 * @param start :: vector for the start point */
void LineViewer::setStart(Mantid::Kernel::VMD start)
{
  if (m_ws && start.getNumDims() != m_ws->getNumDims())
    throw std::runtime_error("LineViewer::setStart(): Invalid number of dimensions in the start vector.");
  m_start = start;
  updateStartEnd();
}

/** Set the end point of the line to integrate
 * @param end :: vector for the end point */
void LineViewer::setEnd(Mantid::Kernel::VMD end)
{
  if (m_ws && end.getNumDims() != m_ws->getNumDims())
    throw std::runtime_error("LineViewer::setEnd(): Invalid number of dimensions in the end vector.");
  m_end = end;
  updateStartEnd();
}


/** Set the width of the line in each dimensions
 * @param width :: vector for the width in each dimension. X dimension stands in for the XY plane width */
void LineViewer::setWidth(Mantid::Kernel::VMD width)
{
  if (m_ws && width.getNumDims() != m_ws->getNumDims())
    throw std::runtime_error("LineViewer::setwidth(): Invalid number of dimensions in the width vector.");
  m_width = width;
  updateStartEnd();
}

/** Set the width of the line in the planar dimension only.
 * Other dimensions' widths will follow unless they were manually changed
 * @param width :: width in the plane. */
void LineViewer::setPlanarWidth(double width)
{
  if (m_allDimsFree)
  {
    for (size_t d=0; d<m_width.getNumDims(); d++)
      m_width[d] = width;
  }
  else
  {
    double oldPlanarWidth = this->getPlanarWidth();
    for (size_t d=0; d<m_width.getNumDims(); d++)
    {
      // Only modify the locked onese
      if (m_width[d] == oldPlanarWidth)
        m_width[d] = width;
    }
    // And always set the planar one
    m_planeWidth = width;
  }
  updateStartEnd();
}

/** Set the number of bins in the line
 * @param nbins :: # of bins */
void LineViewer::setNumBins(size_t numBins)
{
  m_numBins = numBins;
  ui.spinNumBins->blockSignals(true);
  ui.spinNumBins->setValue( int(numBins) );
  ui.spinNumBins->blockSignals(false);
}

/** Set the free dimensions - dimensions that are allowed to change
 *
 * @param all :: Flag that is true when all dimensions are allowed to change
 * @param dimX :: Index of the X dimension in the 2D slice
 * @param dimY :: Index of the Y dimension in the 2D slice
 */
void LineViewer::setFreeDimensions(bool all, int dimX, int dimY)
{
  int nd = int(m_ws->getNumDims());
  if (dimX < 0 || dimX >= nd)
    throw std::runtime_error("LineViewer::setFreeDimensions(): Free X dimension index is out of range.");
  if (dimY < 0 || dimY >= nd)
    throw std::runtime_error("LineViewer::setFreeDimensions(): Free Y dimension index is out of range.");
  m_allDimsFree = all;
  m_freeDimX = dimX;
  m_freeDimY = dimY;
  this->updateFreeDimensions();
}

/** Slot called to set the free dimensions (called from the SliceViewer widget)
 *
 * @param dimX :: index of the X-dimension of the plane
 * @param dimY :: index of the Y-dimension of the plane
 */
void LineViewer::setFreeDimensions(size_t dimX, size_t dimY)
{
  m_allDimsFree = false;
  m_freeDimX = int(dimX);
  m_freeDimY = int(dimY);
  this->updateFreeDimensions();
}


// ==============================================================================================
// ================================== Rendering =================================================
// ==============================================================================================


/** Calculate a curve between two points given a linear start/end point
 *
 * @param ws :: MDWorkspace to plot
 * @param start :: start point in ND
 * @param end :: end point in ND
 * @param minNumPoints :: minimum number of points to plot
 * @param curve :: curve to set
 */
void LineViewer::calculateCurve(IMDWorkspace_sptr ws, VMD start, VMD end,
    size_t minNumPoints, QwtPlotCurve * curve)
{
  if (!ws) return;

  // Use the width of the plot (in pixels) to choose the fineness)
  // That way, there is ~1 point per pixel = as fine as it needs to be
  size_t numPoints = size_t(m_plot->width());
  if (numPoints < minNumPoints) numPoints = minNumPoints;

  VMD step = (end-start) / double(numPoints);
  double stepLength = step.norm();

  // These will be the curve as plotted
  double * x = new double[numPoints];
  double * y = new double[numPoints];

  for (size_t i=0; i<numPoints; i++)
  {
    // Coordinate along the line
    VMD coord = start + step * double(i);
    // Signal in the WS at that coordinate
    signal_t signal = ws->getSignalAtCoord(coord);
    // Make into array
    x[i] = stepLength * double(i);
    y[i] = signal;
  }

  // Make the curve
  curve->setData(x,y, int(numPoints));

  delete [] x;
  delete [] y;

}


/** Calculate and show the preview (non-integrated) line */
void LineViewer::showPreview()
{
  calculateCurve(m_ws, m_start, m_end, 100, m_previewCurve);
  if (m_fullCurve->isVisible())
  {
    m_fullCurve->setVisible(false);
    m_fullCurve->detach();
    m_previewCurve->attach(m_plot);
  }
  m_previewCurve->setVisible(true);
  m_plot->replot();
  m_plot->setTitle("Preview Plot");
}


/** Calculate and show the full (integrated) line */
void LineViewer::showFull()
{
  if (!m_sliceWS) return;
  VMD start(m_sliceWS->getNumDims());
  start *= 0;
  VMD end = start;
  end[0] = m_sliceWS->getDimension(0)->getMaximum();

  calculateCurve(m_sliceWS, start, end, m_numBins, m_fullCurve);
  if (m_previewCurve->isVisible())
  {
    m_previewCurve->setVisible(false);
    m_previewCurve->detach();
    m_fullCurve->attach(m_plot);
  }
  m_fullCurve->setVisible(true);
  m_plot->replot();
  m_plot->setTitle("Integrated Line Plot");
}


} // namespace
}
