#include "OneCurvePlot.h"
#include "PeakMarker2D.h"

#include <qwt_plot_curve.h>
#include <qwt_scale_div.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_canvas.h>
#include <qwt_compat.h>
#include <qwt_plot_zoomer.h>

#include <QFontMetrics>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPainter>

#include <iostream>

OneCurvePlot::OneCurvePlot(QWidget* parent):
QwtPlot(parent),m_curve(NULL)
{
  setAxisFont(QwtPlot::xBottom, parent->font());
  setAxisFont(QwtPlot::yLeft, parent->font());
  canvas()->setCursor(Qt::ArrowCursor);
  //setMouseTracking(true);
  //canvas()->setMouseTracking(true);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  m_zoomer = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner, QwtPicker::AlwaysOff, canvas());
  m_zoomer->setRubberBandPen(QPen(Qt::black));
  QList<QColor> colors;
  m_colors << Qt::red<< Qt::green  << Qt::blue << Qt::cyan << Qt::magenta << Qt::yellow << Qt::gray;
  m_colors << Qt::darkRed<< Qt::darkGreen  << Qt::darkBlue << Qt::darkCyan << Qt::darkMagenta << Qt::darkYellow << Qt::darkGray;
  m_colorIndex = 0;
}

/**
  * Set the scale of the horizontal axis
  * @param from :: Minimum value
  * @param to :: Maximum value
  */
void OneCurvePlot::setXScale(double from, double to)
{
  QFontMetrics fm(this->font());
  int n = from != 0.0 ? abs(static_cast<int>(floor(log10(fabs(from))))) : 0;
  int n1 = to != 0.0 ? abs(static_cast<int>(floor(log10(fabs(to))))) : 0;
  if (n1 > n) n = n1;
  n += 4;

  int labelWidth = n * fm.width("0"); // approxiamte width of a tick label in pixels
  int nMajorTicks = this->width() / labelWidth;
  //std::cerr << "ticks: " << labelWidth << ' ' << nMajorTicks << std::endl;
  const QwtScaleDiv div = axisScaleEngine(QwtPlot::xBottom)->divideScale(from,to,nMajorTicks,nMajorTicks);
  setAxisScaleDiv(xBottom,div);
  m_zoomer->setZoomBase();
}

/**
  * Set the scale of the vertical axis
  * @param from :: Minimum value
  * @param to :: Maximum value
  */
void OneCurvePlot::setYScale(double from, double to)
{
  QwtScaleEngine *engine = axisScaleEngine(yLeft);
  if (dynamic_cast<QwtLog10ScaleEngine*>(engine) && m_curve)
  {
    int n = m_curve->dataSize();
    double yPositiveMin = to;
    for(int i = 0; i < n; ++i)
    {
      double y = m_curve->y(i);
      if (y > 0 && y < yPositiveMin)
      {
        yPositiveMin = y;
      }
    }
    from = yPositiveMin;
  }
  setAxisScale(QwtPlot::yLeft,from,to);
  m_zoomer->setZoomBase();
}

/**
  * Set the data for the curve to display
  * @param x :: A pointer to x values
  * @param y :: A pointer to y values
  * @param dataSize :: The size of the data
  */
void OneCurvePlot::setData(const double* x,const double* y,int dataSize)
{
  if (!m_curve)
  {
    m_curve = new QwtPlotCurve();
    m_curve->attach(this);
  }

  m_curve->setData(x,y,dataSize);
}

/**
 * Set a label which will identify the curve when it is stored.
 */
void OneCurvePlot::setLabel(const QString& label)
{
  m_label = label;
}

/**
  * Hide the curve
  */
void OneCurvePlot::clearCurve()
{
  if (m_curve)
  {
    m_curve->attach(0);
    m_curve = NULL;
  }
}

void OneCurvePlot::resizeEvent(QResizeEvent *e)
{
  QwtPlot::resizeEvent(e);
  recalcAxisDivs();
}

/**
  * Recalculate axis divisions to make sure that tick labels don't overlap
  */
void OneCurvePlot::recalcAxisDivs()
{
  const QwtScaleDiv *div0 = axisScaleDiv(QwtPlot::xBottom);
  double from = div0->lBound();
  double to = div0->hBound();
  setXScale(from,to);
}

void OneCurvePlot::contextMenuEvent (QContextMenuEvent *e)
{
  // context menu will be handled with mouse events
  e->accept();
}

void OneCurvePlot::mousePressEvent(QMouseEvent* e)
{
  if (e->buttons() & Qt::RightButton)
  {
    if (m_zoomer->zoomRectIndex() == 0)
    {
      e->accept();
      // plot owner will display and process context menu
      emit showContextMenu();
    }
    return;
  }
  if (e->buttons() & Qt::LeftButton)
  {
    e->accept();
    m_x0 = e->x();
    m_y0 = e->y();
  }
}

void OneCurvePlot::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton)
  {
    if (m_x0 == e->x() && m_y0 == e->y())
    {// there were no dragging
      emit clickedAt(invTransform(xBottom,e->x()-canvas()->x()),invTransform(yLeft,e->y()-canvas()->y()));
    }
  }
}

void OneCurvePlot::setYAxisLabelRotation(double degrees)
{
  axisScaleDraw(yLeft)->setLabelRotation(degrees);
}

/**
  * Set the log scale on the y axis
  */
void OneCurvePlot::setYLogScale()
{
  QwtLog10ScaleEngine* logEngine = new QwtLog10ScaleEngine();
  setAxisScaleEngine(yLeft,logEngine);
  update();
}

/**
  * Set the linear scale on the y axis
  */
void OneCurvePlot::setYLinearScale()
{
  QwtLinearScaleEngine* engine = new QwtLinearScaleEngine();
  setAxisScaleEngine(yLeft,engine);
  update();
}

/**
 * Add new peak label
 * @param label :: A pointer to a PeakLabel, becomes owned by OneCurvePlot
 */
void OneCurvePlot::addPeakLabel(PeakLabel* label)
{
  label->attach(this);
  m_peakLabels.append(label);
}

/**
 * Removes all peak labels.
 */
void OneCurvePlot::clearPeakLabels()
{
  foreach(PeakLabel* label, m_peakLabels)
  {
    label->detach();
    delete label;
  }
  m_peakLabels.clear();
}

/**
 * Returns true if the current curve isn't NULL
 */
bool OneCurvePlot::hasCurve()const
{
  return m_curve != NULL;
}

/**
 * Store current curve.
 */
void OneCurvePlot::store()
{
  if (m_curve)
  {
    m_stored.insert(m_label,m_curve);
    m_curve->setPen(QPen(m_colors[m_colorIndex]));
    ++m_colorIndex;
    m_colorIndex %= m_colors.size();
    m_curve = NULL;
  }
}

/**
 * Returns true if there are some stored curves.
 */
bool OneCurvePlot::hasStored()const
{
  return ! m_stored.isEmpty();
}

QStringList OneCurvePlot::getLabels()const
{
  QStringList out;
  QMap<QString,QwtPlotCurve*>::const_iterator it = m_stored.begin();
  for(;it!=m_stored.end();++it)
  {
    out << it.key();
  }
  return out;
}

/**
 * Remove a stored curve.
 * @param label :: The label of a curve to remove.
 */
void OneCurvePlot::removeCurve(const QString& label)
{
  QMap<QString,QwtPlotCurve*>::iterator it = m_stored.find(label);
  if (it != m_stored.end())
  {
    it.value()->detach();
    m_stored.erase(it);
  }
}

/**
 * Draw PeakLabel on a plot
 */
void PeakLabel::draw(QPainter *painter, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &canvasRect) const
{
  (void)yMap;
  int x = xMap.transform(m_marker->getTOF());
  int y = static_cast<int>(canvasRect.top() + m_marker->getLabelRect().height());
  painter->drawText(x,y,m_marker->getLabel());
  //std::cerr << x << ' ' << y << ' ' << m_marker->getLabel().toStdString() << std::endl;
}

