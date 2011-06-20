#ifndef INDICATOR_H_
#define INDICATOR_H_

#include "AxisInteractor.h"

#include <QGraphicsItem>
#include <QObject>

class QColor;
class QGraphicsSceneMouseEvent;
class QPoint;
class QPolygonF;
class QRect;
/**
 *
  This class represents the manifestation of the slice indicator. This
  indicator is a elongated blue filled triangle by default. The apex point of
  the indicator marks the location for the slice along the associated axis
  widget. Objects are used by the AxisIndicator widget.

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
const int IndicatorItemType = QGraphicsItem::UserType + 1;

class Indicator: public QObject, public QGraphicsPolygonItem
{
  Q_OBJECT

public:
  /// Enumeration for a graphicsitem type
	enum {Type = IndicatorItemType};

	/**
	 * Default constructor.
	 * @param parent the parent widget of the indicator graphicsitem
	 */
	Indicator(QGraphicsItem *parent = 0);
	/// Default destructor.
	virtual ~Indicator() {}
	/// Print the triangle's vertex coordinates.
	void printSelf();
	/**
	 * Create the triangle's vertex coordinates.
	 * @param eloc x,y position of the creation event
   * @param rect location rectangle from the axis widget scale picker
	 */
	void setPoints(const QPoint &eloc, const QRect &rect);
	/**
	 * Set the orientation of the head of the indicator.
	 * @param orient the orientation of the associated axis scale
	 */
	void setOrientation(AxisInteractor::ScalePos orient);
	/**
	 * Get the type value for the indicator graphicsitem.
	 * @return the numeric value for the indicator graphicsitem type
	 */
	int type() const { return Type; }
	/**
	 * Move the indicator to the specified location.
	 * @param pos location to move the indicator to
	 */
	void updatePos(const QPoint &pos);

signals:
  /**
   * Signal to pass along scene coordinate for indicator to an associated
   * axis interactor widget.
   * @param pos the current scene coordinate of the indicator
   * @param coord the coordinate (x or y) that is not relevant for the
   *              axis widget and may be adjusted
   */
  void indicatorMoved(const QPoint &pos, int coord);

protected:
	/**
	 * Update the indicator's position based on the current mouse position.
	 * @param event the associated event
	 */
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	/**
	 * Highlight the indicator as selected when the mouse button is released.
	 * @param event the associated event
	 */
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
	/**
	 * Adjust the indicator's coordinates to get the apex point to point near
	 * the requested axis position.
	 * @param level the requested position
	 * @return the fixed position for the indicator
	 */
	int fixPosition(int level);

	QColor fillColor; ///< The fill color for the triangle
	QColor outlineColor; ///< The outline color for the triangle
	QPolygonF path; ///< The holder for the triangle's shape coordinates
	int half_base; ///< Half the size of the triangle's base
	int tip_edge; ///< The triangle's closest point (apex) to the axis indicator
	AxisInteractor::ScalePos orientation; ///< The orientation of the indicator apex
};

#endif // INDICATOR_H_
