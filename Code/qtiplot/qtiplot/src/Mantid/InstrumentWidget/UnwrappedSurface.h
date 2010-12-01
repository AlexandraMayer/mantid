#ifndef UNWRAPPEDSURFACE_H
#define UNWRAPPEDSURFACE_H

#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "GLActor.h"
#include <boost/shared_ptr.hpp>

#include <QImage>
#include <QList>
#include <QStack>
#include <map>

namespace Mantid{
  namespace Geometry{
    class IDetector;
  }
}

class GLColor;
class GL3DWidget;

/*!
\class UnwrappedDetector
\brief Class helper for drawing detectors on unwraped surfaces
\date 15 Nov 2010
\author Roman Tolchenov, Tessella plc

This class keeps information used to draw a detector on an unwrapped cylindrical surface.

*/
class UnwrappedDetector
{
public:
  UnwrappedDetector(const unsigned char* c,
                       boost::shared_ptr<const Mantid::Geometry::IDetector> det
                       );
  unsigned char color[3]; ///< red, green, blue colour components (0 - 255)
  double u;      ///< horizontal "unwrapped" coordinate
  double v;      ///< vertical "unwrapped" coordinate
  double width;  ///< detector width in units of u
  double height; ///< detector height in units of v
  double uscale; ///< scaling factor in u direction
  double vscale; ///< scaling factor in v direction
  boost::shared_ptr<const Mantid::Geometry::IDetector> detector;
  Mantid::Geometry::V3D minPoint,maxPoint;
};


/**
  * @class UnwrappedSurface
  * @brief Performs projection of an instrument onto a 2D surface and unwrapping it into a plane. Draws the resulting image
  *        on the screen.
  * @author Roman Tolchenov, Tessella plc
  * @date 18 Nov 2010
  */

class UnwrappedSurface: public GLActor::DetectorCallback
{
public:
  UnwrappedSurface(const GLActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis);
  ~UnwrappedSurface();
  void startUnwrappedSelection(int x,int y);
  void moveUnwrappedSelection(int x,int y);
  void endUnwrappedSelection(int x,int y);
  void zoomUnwrapped();
  void unzoomUnwrapped();
  void updateView();
  void updateDetectors();

  virtual void draw(GL3DWidget* widget);

protected:
  virtual void calcUV(UnwrappedDetector& udet) = 0;
  virtual void calcRot(UnwrappedDetector& udet, Mantid::Geometry::Quat& R) = 0;

  void init();
  void calcSize(UnwrappedDetector& udet,const Mantid::Geometry::V3D& X,
                const Mantid::Geometry::V3D& Y);
  void callback(boost::shared_ptr<const Mantid::Geometry::IDetector> det,const DetectorCallbackData& data);
  void clear();

  const GLActor* m_rootActor;
  const Mantid::Geometry::V3D m_pos;   ///< Origin (sample position)
  const Mantid::Geometry::V3D m_zaxis; ///< The z axis, symmetry axis of the cylinder
  Mantid::Geometry::V3D m_xaxis;       ///< The x axis, defines the zero of the polar phi angle
  Mantid::Geometry::V3D m_yaxis;       ///< The y axis, rotation from x to y defines positive phi
  double m_u_min;                    ///< Minimum phi
  double m_u_max;                    ///< Maximum phi
  double m_v_min;                      ///< Minimum z
  double m_v_max;                      ///< Maximum z
  QImage* m_unwrappedImage;      ///< storage for unwrapped image
  bool m_unwrappedViewChanged;   ///< set when the unwrapped image must be redrawn
  QList<UnwrappedDetector> m_unwrappedDetectors;  ///< info needed to draw detectors onto unwrapped image
  QRectF m_unwrappedView;
  QRect m_selectRect;
  QStack<QRectF> m_zoomStack;

  static double m_tolerance;     ///< tolerance for comparing 3D vectors

  void BasisRotation(const Mantid::Geometry::V3D& Xfrom,
                  const Mantid::Geometry::V3D& Yfrom,
                  const Mantid::Geometry::V3D& Zfrom,
                  const Mantid::Geometry::V3D& Xto,
                  const Mantid::Geometry::V3D& Yto,
                  const Mantid::Geometry::V3D& Zto,
                  Mantid::Geometry::Quat& R,
                  bool out = false
                  );
};

#endif // UNWRAPPEDSURFACE_H
