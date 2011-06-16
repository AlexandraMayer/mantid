#include "MantidMDAlgorithms/VectorMathematics.h"
namespace Mantid
{
namespace MDAlgorithms
{

double dotProduct(Mantid::Kernel::V3D a, Mantid::Kernel::V3D b)
{
  using Mantid::Kernel::V3D;
  return a.scalar_prod(b);
}

double dotProduct(double a1, double a2, double a3, double b1, double b2, double b3)
{
  using Mantid::Kernel::V3D;
  V3D a(a1, a2, a3);
  V3D b(b1, b2, b3);
  return a.scalar_prod(b);
}

Mantid::Kernel::V3D crossProduct(Mantid::Kernel::V3D a, Mantid::Kernel::V3D b)
{
  using Mantid::Kernel::V3D;
  return a.cross_prod(b);
}

Mantid::Kernel::V3D crossProduct(double a1, double a2, double a3, double b1, double b2,
    double b3)
{
  using Mantid::Kernel::V3D;
  V3D a(a1, a2, a3);
  V3D b(b1, b2, b3);
  return a.cross_prod(b);
}

double absolute(double a1, double a2, double a3)
{
  return sqrt((a1 * a1) + (a2 * a2) + (a3 * a3));
}

}
}
