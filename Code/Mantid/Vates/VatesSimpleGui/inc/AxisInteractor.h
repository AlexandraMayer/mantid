#ifndef AXISINTERACTOR_H_
#define AXISINTERACTOR_H_

#include <QtGui/QWidget>

class QGraphicsScene;
class QGraphicsView;
class QGridLayout;
class QMenu;
class QMouseEvent;
class QString;
class QwtScaleEngine;
class QwtScaleTransformation;
class QwtScaleWidget;
class ScalePicker;
/**
 *
  This class provides a mechanism for setting slices onto a dataset that are
  associated with an individual dataset axis. The slice indicators are
  represented by triangles pointing at their current location via the
  associated axis widget. A new slice and indicator is achieved by
  right-clicking on the empty space near, but not on, the axis widget.

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
class AxisInteractor : public QWidget
{
	Q_OBJECT

	Q_ENUMS(ScalePos)

	Q_PROPERTY( ScalePos scalePosition READ scalePosition
	        WRITE setScalePosition )

public:
	/// Enumeration for scale orientation
	enum ScalePos {
	  LeftScale,
    RightScale,
    TopScale,
    BottomScale };
	/**
	 * Default constructor.
	 * @param parent the parent UI object for the axis interactor widget
	 */
	AxisInteractor(QWidget *parent = 0);
	/// Default destructor.
	virtual ~AxisInteractor() {}
	/**
	 * Remove highlights from all selected indicators.
	 */
	void clearSelections();
  /**
   * Get the associated ScalePicker for the indicator.
   * @return the associated ScalePicker
   */
  ScalePicker *getScalePicker() { return this->scalePicker; }
	/**
	 * Is there at least one indicator?
	 * @return true if yes
	 */
	bool hasIndicator();
	/**
	 * Return the orientation of the axis scale ticket marks
	 * @return the orientation code
	 */
  ScalePos scalePosition() const;
  /**
   * Highlight the requested indicator.
   * @param name the name of the slice being highlighted
   */
  void selectIndicator(const QString &name);
	/**
	 * Set the axis information for the associated dataset axis.
	 * @param title the dataset axis title
	 * @param min the dataset axis minimum extent
	 * @param max the dataset axis maximum extent
	 */
	void setInformation(QString title, double min, double max);
	/**
	 * Set the orientation of the axis scale and graphicsview.
	 * @param orient the orientation of the graphicsview
	 * @param scalePos the orientation of the axis scale
	 */
  void setOrientation(Qt::Orientation orient, ScalePos scalePos);
  /**
   * Set the orientation of the axis scale tick marks.
   * @param scalePos the orientation code
   */
  void setScalePosition(ScalePos scalePos);
	/**
	 * Update the current indicator to a new location.
	 * @param value the new location for the indicator
	 */
	void updateIndicator(double value);

signals:
  /**
   * Signal to pass along the name of the indicator to delete.
   * @param name the name of the indicator to be deleted
   */
  void deleteIndicator(const QString &name);
  /**
   * Signal to pass along the name of the indicator being selected.
   * @param name the name of the selected indicator
   */
  void indicatorSelected(const QString &name);
  /**
   * Signal to show or hide the given indicator.
   * @param isVisible flag the determines showing or hiding the indicator
   * @param name the name of the indicator to show or hide
   */
  void showOrHideIndicator(bool isVisible, const QString &name);

protected slots:
  /**
   * Create an indicator at the requested location that is associated with
   * a new slice.
   * @param point the (x,y) location for the indicator
   */
	void createIndicator(const QPoint &point);
  /// Determine the indicator being selected and pass along that information.
  void getIndicator();
	/**
	 * Associate a ParaView slice object name with the new indicator.
	 * @param name the ParaView name of the slice
	 */
  void setIndicatorName(const QString &name);
  /**
   * Show a context menu for the indicator that will allow it to be
   * deleted or hidden/shown.
   * @param pos location to show the context menu
   */
  void showContextMenu(const QPoint &pos);

protected:
	/**
	 * Intercept mouse clicks to avoid inadvertent creation of indicators. This
	 * forces the mode of right clicking near the axis to get an indicator.
	 * @param obj the QObject that spawned the event
	 * @param event the associated QEvent
	 */
	bool eventFilter(QObject *obj, QEvent *event);

private:
  /// Create the context menu of the indicators
  void createContextMenu();
	/// Handle the setup of the widget based on orientation requests.
	void widgetLayout();

  QMenu *indicatorContextMenu; ///< The indicator context menu
	QwtScaleEngine *engine; ///< The scale type for the axis widget
	QGraphicsView *graphicsView; ///< The holder for the slice indicators
	QGridLayout *gridLayout; ///< Layout manager for widgets
	bool isSceneGeomInit; ///< Flag to ensure the scene is initialized once
  Qt::Orientation orientation; ///< The overall orientation of the widget
	ScalePicker *scalePicker; ///< The picker that retrieves the axis location
  ScalePos scalePos; ///< The orientation of the axis scale tick marks
  QwtScaleWidget *scaleWidget; ///< The axis scale widget
	QGraphicsScene *scene; ///< The contained for the slice indicators
	QwtScaleTransformation *transform; ///< The scale type for the engine
};

#endif // AXISINTERACTOR_H_
