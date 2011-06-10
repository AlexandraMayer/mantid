#ifndef UNWRAPPEDSPHERE_H
#define UNWRAPPEDSPHERE_H

#include "UnwrappedSurface.h"

/**
  * Implementation of UnwrappedSurface as a cylinder
  */
class UnwrappedSphere: public UnwrappedSurface
{
public:
  UnwrappedSphere(const InstrumentActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis);
protected:
  void calcUV(UnwrappedDetector& udet);
  void calcRot(const UnwrappedDetector& udet, Mantid::Geometry::Quat& R)const;
  double uPeriod()const;
};

#endif // UNWRAPPEDSPHERE_H
