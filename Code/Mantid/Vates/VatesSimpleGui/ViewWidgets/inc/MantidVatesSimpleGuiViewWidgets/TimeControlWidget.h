#ifndef TIMECONTROLWIDGET_H_
#define	TIMECONTROLWIDGET_H_

#include "ui_TimeControlWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

/**
 *
 This class wraps the ParaView time control toolbars into a widget.

 @author Michael Reuter
 @date 24/05/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS TimeControlWidget : public QWidget
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget for the time control widget
   */
  TimeControlWidget(QWidget *parent = 0);
  /// Default constructor.
  virtual ~TimeControlWidget();

private:
  Ui::TimeControlWidgetClass ui; ///< The time control widget's UI form
};

}
}
}

#endif	// TIMECONTROLWIDGET_H_
