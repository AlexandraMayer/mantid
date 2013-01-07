#ifndef MANTIDPLOT_SHAPE2D_H_
#define MANTIDPLOT_SHAPE2D_H_

#include "RectF.h"

#include <QColor>
#include <QPointF>

class QPainter;
class QPainterPath;
class QMouseEvent;
class QWheelEvent;

/**
  * Base class for an editable 2D shape, which can be drawn on ProjectionSurface.
  */
class Shape2D
{
public:
  Shape2D();
  virtual ~Shape2D(){}

  // --- Public pure virtual methods --- //

  virtual Shape2D* clone()const = 0;
  // modify path so painter.drawPath(path) could be used to draw the shape. needed for filling in complex shapes
  virtual void addToPath(QPainterPath& path) const = 0;
  // make sure the shape is within the bounding box
  virtual void refit() = 0;

  // --- Public virtual methods --- //

  virtual void draw(QPainter& painter) const;
  virtual QPointF origin() const {return m_boundingRect.center();}
  virtual void moveBy(const QPointF& pos);
  virtual size_t getNControlPoints() const;
  virtual QPointF getControlPoint(size_t i) const;
  virtual void setControlPoint(size_t i,const QPointF& pos);
  virtual RectF getBoundingRect() const {return m_boundingRect;}
  // move the left, top, right and bottom sides of the bounding rect
  // by dx1, dy1, dx2, and dy2 correspondingly
  virtual void adjustBoundingRect(double dx1, double dy1, double dx2, double dy2);
  virtual void setBoundingRect(const RectF& rect);
  // will the shape be selected if clicked at a point
  virtual bool selectAt(const QPointF& )const{return false;}
  // is a point inside the shape (closed line)
  virtual bool contains(const QPointF& )const{return false;}
  // is a point "masked" by the shape. Only filled regions of a shape mask a point
  virtual bool isMasked(const QPointF& )const;

  // --- Public methods --- //

  void setColor(const QColor& color)
  {
    m_color.setRed(color.red());
    m_color.setGreen(color.green());
    m_color.setBlue(color.blue());
  }
  QColor getColor()const{return m_color;}
  void setFillColor(const QColor& color){m_fill_color = color;}
  void setScalable(bool on){m_scalable = on;}
  bool isScalable() const {return m_scalable;}
  void edit(bool on){m_editing = on;}
  bool isEditing()const{return m_editing;}

  // --- Properties. for gui interaction --- //

  // double properties
  virtual QStringList getDoubleNames()const{return QStringList();}
  virtual double getDouble(const QString& prop) const{(void)prop; return 0.0;}
  virtual void setDouble(const QString& prop, double value){(void)prop; (void)value;}

  // QPointF properties
  virtual QStringList getPointNames()const{return QStringList();}
  virtual QPointF getPoint(const QString& prop) const{(void)prop; return QPointF();}
  virtual void setPoint(const QString& prop, const QPointF& value){(void)prop; (void)value;}

protected:
  // --- Protected pure virtual methods --- //

  virtual void drawShape(QPainter& painter) const = 0;

  // --- Protected virtual methods --- //

  // return number of control points specific to this shape
  virtual size_t getShapeNControlPoints() const{return 0;}
  // returns position of a shape specific control point, 0 < i < getShapeNControlPoints()
  virtual QPointF getShapeControlPoint(size_t ) const{return QPointF();}
  // sets position of a shape specific control point, 0 < i < getShapeNControlPoints()
  virtual void setShapeControlPoint(size_t ,const QPointF& ){}
  // make sure the bounding box is correct
  virtual void resetBoundingRect() {}

  // --- Protected methods --- //

  // make sure that width and heigth are positive
  void correctBoundingRect();

  // --- Protected data --- //

  static const size_t NCommonCP;
  static const qreal sizeCP;
  RectF m_boundingRect;
  QColor m_color;
  QColor m_fill_color;
  bool m_scalable; ///< shape cann be scaled when zoomed
  bool m_editing;
};

class Shape2DEllipse: public Shape2D
{
public:
  Shape2DEllipse(const QPointF& center,double radius1,double radius2 = 0);
  virtual Shape2D* clone()const{return new Shape2DEllipse(*this);}
  virtual bool selectAt(const QPointF& p)const;
  virtual bool contains(const QPointF& p)const;
  virtual void addToPath(QPainterPath& path) const;
  // double properties
  virtual QStringList getDoubleNames()const;
  virtual double getDouble(const QString& prop) const;
  virtual void setDouble(const QString& prop, double value);
  // QPointF properties
  virtual QStringList getPointNames()const{return QStringList("center");}
  virtual QPointF getPoint(const QString& prop) const;
  virtual void setPoint(const QString& prop, const QPointF& value);

protected:
  virtual void drawShape(QPainter& painter) const;
  virtual void refit(){}
};

class Shape2DRectangle: public Shape2D
{
public:
  Shape2DRectangle();
  Shape2DRectangle(const QPointF& p0,const QPointF& p1);
  Shape2DRectangle(const QPointF& p0,const QSizeF& size);
  virtual Shape2D* clone()const{return new Shape2DRectangle(*this);}
  virtual bool selectAt(const QPointF& p)const;
  virtual bool contains(const QPointF& p)const{return m_boundingRect.contains(p);}
  virtual void addToPath(QPainterPath& path) const;
protected:
  virtual void drawShape(QPainter& painter) const;
  virtual void refit(){}
};

class Shape2DRing: public Shape2D
{
public:
  Shape2DRing(Shape2D* shape, double xWidth = 0.000001, double yWidth = 0.000001);
  Shape2DRing(const Shape2DRing& ring);
  virtual Shape2D* clone()const{return new Shape2DRing(*this);}
  virtual bool selectAt(const QPointF& p)const;
  virtual bool contains(const QPointF& p)const;
  // double properties
  virtual QStringList getDoubleNames()const;
  virtual double getDouble(const QString& prop) const;
  virtual void setDouble(const QString& prop, double value);
  // QPointF properties
  virtual QStringList getPointNames()const{return QStringList("center");}
  virtual QPointF getPoint(const QString& prop) const;
  virtual void setPoint(const QString& prop, const QPointF& value);
protected:
  virtual void drawShape(QPainter& painter) const;
  virtual void addToPath(QPainterPath& ) const {}
  virtual void refit();
  virtual void resetBoundingRect();
  virtual size_t getShapeNControlPoints() const{return 4;}
  virtual QPointF getShapeControlPoint(size_t i) const;
  virtual void setShapeControlPoint(size_t i,const QPointF& pos);
  Shape2D* m_outer_shape;
  Shape2D* m_inner_shape;
  double m_xWidth;
  double m_yWidth;
};


#endif /*MANTIDPLOT_SHAPE2D_H_*/
