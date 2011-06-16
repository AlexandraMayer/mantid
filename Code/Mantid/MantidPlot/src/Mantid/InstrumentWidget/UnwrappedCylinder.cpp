#include "UnwrappedCylinder.h"

#include "MantidGeometry/IDetector.h"

UnwrappedCylinder::UnwrappedCylinder(const InstrumentActor* rootActor, const Mantid::Kernel::V3D &origin, const Mantid::Kernel::V3D &axis):
    UnwrappedSurface(rootActor,origin,axis)
{
  init();
}

void UnwrappedCylinder::calcUV(UnwrappedDetector& udet)
{
  //static const double pi2 = 2*acos(-1.);
  Mantid::Kernel::V3D pos = udet.detector->getPos() - m_pos;

  // projection to cylinder axis
  udet.v = pos.scalar_prod(m_zaxis);
  double x = pos.scalar_prod(m_xaxis);
  double y = pos.scalar_prod(m_yaxis);
  udet.u = -atan2(y,x);

  udet.uscale = 1./sqrt(x*x+y*y);
  udet.vscale = 1.;

  calcSize(udet,Mantid::Kernel::V3D(-1,0,0),Mantid::Kernel::V3D(0,1,0));

}

void UnwrappedCylinder::calcRot(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const
{
  // Basis vectors for a detector image on the screen
  const Mantid::Kernel::V3D X(-1,0,0);
  const Mantid::Kernel::V3D Y(0,1,0);
  const Mantid::Kernel::V3D Z(0,0,-1);

  // Find basis with x axis pointing to the detector from the sample,
  // z axis is coplanar with x and m_zaxis, and y making the basis right handed
  Mantid::Kernel::V3D x,y,z;
  z = udet.detector->getPos() - m_pos;
  y = m_zaxis;
  y.normalize();
  x = y.cross_prod(z);
  x.normalize();
  z = z - y * z.scalar_prod(y);
  z.normalize();
  Mantid::Kernel::Quat R1;
  InstrumentActor::BasisRotation(x,y,z,X,Y,Z,R1);

  R =  R1 * udet.detector->getRotation();

}


double UnwrappedCylinder::uPeriod()const
{
  return 2 * M_PI;
}
