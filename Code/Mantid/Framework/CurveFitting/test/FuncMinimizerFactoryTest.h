#ifndef FUNCMINIMIZERFACTORYTEST_H_
#define FUNCMINIMIZERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FuncMinimizerFactory.h"
#include "MantidCurveFitting/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLFunctions.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/System.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class FuncMinimizerFactoryTest_A: public IFuncMinimizer
{
  int m_attr;
public:
  FuncMinimizerFactoryTest_A() {}

  /// Overloading base class methods
  std::string name()const {return "Boevs";}
  bool iterate() {return true;}
  int hasConverged() {return 101;}
  double costFunctionVal() {return 5.0;}
  void calCovarianceMatrix(double epsrel, gsl_matrix * covar)
  {
    UNUSED_ARG(epsrel);
    UNUSED_ARG(covar);
  }
  void initialize(API::ICostFunction_sptr)
  {
  }
};

DECLARE_FUNCMINIMIZER(FuncMinimizerFactoryTest_A, nedtur);


class FuncMinimizerFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FuncMinimizerFactoryTest *createSuite() { return new FuncMinimizerFactoryTest(); }
  static void destroySuite( FuncMinimizerFactoryTest *suite ) { delete suite; }

  FuncMinimizerFactoryTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void testCreateFunction()
  {
    IFuncMinimizer* minimizerA = FuncMinimizerFactory::Instance().createUnwrapped("nedtur");
    TS_ASSERT(minimizerA);
    TS_ASSERT(minimizerA->name().compare("Boevs") == 0);

    delete minimizerA;
  }

};

#endif /*FUNCMINIMIZERFACTORYTEST_H_*/
