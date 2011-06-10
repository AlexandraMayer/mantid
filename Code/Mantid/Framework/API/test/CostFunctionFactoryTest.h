#ifndef COSTFUNCTIONFACTORYTEST_H_
#define COSTFUNCTIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ICostFunction.h"
#include "MantidKernel/System.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::API;

class CostFunctionFactoryTest_A: public ICostFunction
{
  int m_attr;
public:
  CostFunctionFactoryTest_A() {}

  std::string name()const {return "fido";}

  double val(const double* yData, const double* inverseError, double* yCal, const size_t& n)
  {
    UNUSED_ARG(yData);
    UNUSED_ARG(inverseError);
    UNUSED_ARG(yCal);
    UNUSED_ARG(n);
    return 0.0;
  }
  void deriv(const double* yData, const double* inverseError, const double* yCal, 
    const double* jacobian, double* outDerivs, const size_t& p, const size_t& n)
  {
    UNUSED_ARG(yData);
    UNUSED_ARG(inverseError);
    UNUSED_ARG(yCal);
    UNUSED_ARG(jacobian);
    UNUSED_ARG(outDerivs);
    UNUSED_ARG(p);
    UNUSED_ARG(n);
  }

};

DECLARE_COSTFUNCTION(CostFunctionFactoryTest_A, nedtur);


class CostFunctionFactoryTest : public CxxTest::TestSuite
{
public:
  CostFunctionFactoryTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void testCreateFunction()
  {
    ICostFunction* cfA = CostFunctionFactory::Instance().createUnwrapped("nedtur");
    TS_ASSERT(cfA);
    TS_ASSERT(cfA->name().compare("fido") == 0);
    TS_ASSERT(cfA->shortName().compare("Quality") == 0);

    delete cfA;
  }

};

#endif /*COSTFUNCTIONFACTORYTEST_H_*/
