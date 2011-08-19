#include "ScalePicker.h"

#include <qwt_scale_draw.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>

#include <QEvent>
#include <QMouseEvent>
#include <QRect>

#include <iostream>
ScalePicker::ScalePicker(QwtScaleWidget *scale) : QObject(scale)
{
  static_cast<QwtScaleWidget *>(this->parent())->installEventFilter(this);
}

bool ScalePicker::eventFilter(QObject *object, QEvent *e)
{
  if (object->inherits("QwtScaleWidget") &&
      e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
    if (Qt::LeftButton == mouseEvent->button())
    {
      this->mouseClicked(static_cast<const QwtScaleWidget *>(object),
                         mouseEvent->pos());
      return true;
    }
  }

  return QObject::eventFilter(object, e);
}

void ScalePicker::mouseClicked(const QwtScaleWidget *scale, const QPoint &pos,
bool createIndicator)
{
  QRect rect = this->scaleRect(scale);

  int margin = 10; // 10 pixels tolerance
  rect.setRect(rect.x() - margin, rect.y() - margin,
  rect.width() + 2 * margin, rect.height() +  2 * margin);

  if (rect.contains(pos)) // No click on the title
  {
    // translate the position in a value on the scale

    double value = 0.0;

    const QwtScaleDraw *sd = scale->scaleDraw();
    switch(scale->alignment())
    {
    case QwtScaleDraw::LeftScale:
    {
      value = sd->map().invTransform(pos.y());
      break;
    }
    case QwtScaleDraw::RightScale:
    {
      value = sd->map().invTransform(pos.y());
      break;
    }
    case QwtScaleDraw::BottomScale:
    {
      value = sd->map().invTransform(pos.x());
      break;
    }
    case QwtScaleDraw::TopScale:
    {
      value = sd->map().invTransform(pos.x());
      break;
    }
    }
    if (createIndicator)
    {
      emit this->makeIndicator(pos);
      emit this->clicked(value);
    }
    else
    {
      emit this->moved(value);
    }
  }
}

QPoint *ScalePicker::getLocation(double axisval)
{
  QwtScaleWidget *scale = static_cast<QwtScaleWidget *>(this->parent());
  const QwtScaleDraw *sd = scale->scaleDraw();

  QRect rect = this->scaleRect(scale);

  int margin = 10; // 10 pixels tolerance
  rect.setRect(rect.x() - margin, rect.y() - margin,
  rect.width() + 2 * margin, rect.height() +  2 * margin);

  int point = sd->map().transform(axisval);
  QPoint *pos = new QPoint();

  switch(scale->alignment())
  {
  case QwtScaleDraw::LeftScale:
  {
    pos->setX(rect.x());
    pos->setY(point);
    break;
  }
  case QwtScaleDraw::RightScale:
  {
    pos->setX(rect.x());
    pos->setY(point);
    break;
  }
  case QwtScaleDraw::BottomScale:
  {
    pos->setX(point);
    pos->setY(rect.y());
    break;
  }
  case QwtScaleDraw::TopScale:
  {
    pos->setX(point);
    pos->setY(rect.y());
    break;
  }
  }
  return pos;
}

// The rect of a scale without the title
QRect ScalePicker::scaleRect(const QwtScaleWidget *scale) const
{
  const int bld = scale->margin();
  const int mjt = scale->scaleDraw()->majTickLength();
  const int sbd = scale->startBorderDist();
  const int ebd = scale->endBorderDist();

  QRect rect;
  switch(scale->alignment())
  {
  case QwtScaleDraw::LeftScale:
  {
    rect.setRect(scale->width() - bld - mjt, sbd,
                 mjt, scale->height() - sbd - ebd);
    break;
  }
  case QwtScaleDraw::RightScale:
  {
    rect.setRect(bld, sbd, mjt, scale->height() - sbd - ebd);
    break;
  }
  case QwtScaleDraw::BottomScale:
  {
    rect.setRect(sbd, bld, scale->width() - sbd - ebd, mjt);
    break;
  }
  case QwtScaleDraw::TopScale:
  {
    rect.setRect(sbd, scale->height() - bld - mjt,
                 scale->width() - sbd - ebd, mjt);
    break;
  }
  }
  return rect;
}

void ScalePicker::determinePosition(const QPoint &pos, int coord)
{
  QwtScaleWidget *scale = static_cast<QwtScaleWidget *>(this->parent());
  QRect rect = this->scaleRect(scale);
  QPoint shiftPt;
  if (coord == 0)
  {
    if (QwtScaleDraw::LeftScale == scale->alignment())
    {
      shiftPt.setX(rect.right());
      shiftPt.setY(pos.y());
    }
    if (QwtScaleDraw::RightScale == scale->alignment())
    {
      shiftPt.setX(rect.left());
      shiftPt.setY(pos.y());
    }
  }
  if (coord == 1)
  {
    if (QwtScaleDraw::TopScale == scale->alignment())
    {
      shiftPt.setX(pos.x());
      shiftPt.setY(rect.bottom());
    }
    if (QwtScaleDraw::BottomScale == scale->alignment())
    {
      shiftPt.setX(pos.x());
      shiftPt.setY(rect.top());
    }
  }
  this->mouseClicked(scale, shiftPt, false);
}
