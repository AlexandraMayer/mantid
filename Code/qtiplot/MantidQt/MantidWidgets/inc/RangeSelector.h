#ifndef MANTIDQT_MANTIDWIDGET_POSHPLOTTING_H
#define MANTIDQT_MANTIDWIDGET_POSHPLOTTING_H

#include "WidgetDllOption.h"

#include <qwt_plot_picker.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>

#include <QPen>

namespace MantidQt
{
namespace MantidWidgets
{
  /**
  * Allows for simpler (in a way) selection of a range on a QwtPlot in MantidQt.
  * @author Michael Whitty, RAL ISIS
  * @date 11/10/2010
  */
  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS RangeSelector : public QwtPlotPicker
  {
    Q_OBJECT
  public:
    enum Range { XMINMAX, XSINGLE, YMINMAX, YSINGLE };

    RangeSelector(QwtPlot* plot, Range range=XMINMAX);
    ~RangeSelector() {};

    bool eventFilter(QObject*, QEvent*);
    
    void setRange(double, double);
    bool changingMin(double, double);
    bool changingMax(double, double);

  signals:
    void minValueChanged(double);
    void maxValueChanged(double);

  public slots:
    void minChanged(double);
    void maxChanged(double);
    void setMinimum(double); ///< outside setting of value
    void setMaximum(double); ///< outside setting of value
    void reapply(); ///< re-apply the range selector lines

  private:
    void setMin(double val);
    void setMax(double val);
    void verify();
    bool inRange(double);


    // MEMBER ATTRIBUTES
    Range m_range; ///< type of selection widget is for

    double m_min;
    double m_max;
    double m_lower; ///< lowest allowed value for range
    double m_higher; ///< highest allowed value for range
    
    QwtPlotCanvas* m_canvas;
    QwtPlot* m_plot;

    QwtPlotMarker* m_mrkMin;
    QwtPlotMarker* m_mrkMax;

    bool m_minChanging;
    bool m_maxChanging;

    /** Strictly UI options and settings below this point **/

    QPen* m_pen; ///< pen object used to define line style, colour, etc
    QCursor m_movCursor; ///< the cursor object to display when an item is being moved

  };

} // MantidWidgets
} // MantidQt

#endif