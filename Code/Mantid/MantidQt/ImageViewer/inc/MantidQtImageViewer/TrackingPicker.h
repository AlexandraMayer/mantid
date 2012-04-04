#ifndef TRACKING_PICKER_H
#define TRACKING_PICKER_H

#include <qwt_plot_picker.h>
#include <qwt_plot_canvas.h>
#include "MantidQtImageViewer/DllOptionIV.h"

/** 
   @class TrackingPicker

      This class is a QwtPlotPicker that will emit a signal whenever the 
    mouse is moved.  It was adapted from the SliceViewer's CustomPicker 
  
    @author Dennis Mikkelson 
    @date   2012-04-03 
     
    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories
  
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
    
    Code Documentation is available at 
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace ImageView
{

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER TrackingPicker : public QwtPlotPicker
{
  Q_OBJECT

public:
  TrackingPicker(QwtPlotCanvas* canvas);

  /// Disable (or enable) position readout at cursor position, even if
  /// tracking is ON.  Tracking MUST be on for the mouseMoved signal to be
  /// emitted.
  void HideReadout( bool hide );

signals:
  void mouseMoved() const;

protected:

  // Unhide base class method (to avoid Intel compiler warning)
//  using QwtPlotPicker::trackerText;

  QwtText trackerText( const QPoint & point ) const;
  QwtText trackerText( const QwtDoublePoint & pos) const;

private:
  bool hide_readout;

};

} // namespace MantidQt 
} // namespace ImageView

#endif  // TRACKING_PICKER_H
