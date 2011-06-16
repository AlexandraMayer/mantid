#ifndef UNWRAPPEDCYLINDER_H
#define UNWRAPPEDCYLINDER_H

#include "UnwrappedSurface.h"

/**
  * Implementation of UnwrappedSurface as a cylinder
  */
class UnwrappedCylinder: public UnwrappedSurface
{
public:
  UnwrappedCylinder(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
protected:
  void calcUV(UnwrappedDetector& udet);
  void calcRot(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const;
  double uPeriod()const;
};

#endif // UNWRAPPEDCYLINDER_H
