//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Power.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM( Power)

///////////////////////////////////


void Power::defineProperties()
{
  declareProperty("Exponent", 1.0);
}

void Power::retrieveProperties()
{
  m_exponent = getProperty("Exponent");
}

void Power::performUnaryOperation(const double& XIn, const double& YIn, const double& EIn, double& YOut,
    double& EOut)
{
  double yIn = YIn; //Create local copy
  double eIn = EIn; //Create local copy
  YOut = calculatePower(YIn, m_exponent);
  EOut = std::fabs(m_exponent * YOut * (eIn / yIn));
}

inline double Power::calculatePower(const double base, const double exponent)
{
  return std::pow(base, exponent);
}

}
}

