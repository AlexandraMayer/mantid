#ifndef BOUNDARYCONSTRAINTTEST_H_
#define BOUNDARYCONSTRAINTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Lorentzian.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Expression.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"

//using namespace Mantid::Kernel;
using namespace Mantid::API;
//using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;


class BoundaryConstraintTest : public CxxTest::TestSuite
{
public:



  void test1()
  {
    // set up fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setParameter("Sigma",1.1);

    BoundaryConstraint* bc = new BoundaryConstraint();
    bc->reset(gaus,2);

    TS_ASSERT(!bc->hasLower());
    TS_ASSERT(!bc->hasUpper());

    bc->setLower(1.0);
    bc->setUpper(2.0);

    TS_ASSERT(bc->hasLower());
    TS_ASSERT(bc->hasUpper());

    BoundaryConstraint* bc2 = new BoundaryConstraint();
    bc2->reset(gaus,2);
    bc2->setBounds(10,20);

    TS_ASSERT_DELTA( bc2->lower(), 10 ,0.0001);
    TS_ASSERT_DELTA( bc2->upper(), 20 ,0.0001);

    TS_ASSERT_DELTA( gaus->getParameter("Sigma"), 1.1 ,0.0001);
    
    bc2->setParamToSatisfyConstraint();
    TS_ASSERT_DELTA( gaus->getParameter("Sigma"), 10.0 ,0.0001);

    delete gaus;

  }

  void testInitialize1()
  {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(10<Sigma<20)");
    bc.initialize(&gaus,expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Sigma" );
    TS_ASSERT_DELTA( bc.lower(), 10 ,0.0001);
    TS_ASSERT_DELTA( bc.upper(), 20 ,0.0001);
  }

  void testInitialize2()
  {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(20>Sigma>10)");
    bc.initialize(&gaus,expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Sigma" );
    TS_ASSERT_DELTA( bc.lower(), 10 ,0.0001);
    TS_ASSERT_DELTA( bc.upper(), 20 ,0.0001);
  }

  void testInitialize3()
  {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(10<Sigma)");
    bc.initialize(&gaus,expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Sigma" );
    TS_ASSERT_DELTA( bc.lower(), 10 ,0.0001);
    TS_ASSERT( !bc.hasUpper() );
  }

  void testInitialize4()
  {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(Sigma<20)");
    bc.initialize(&gaus,expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Sigma" );
    TS_ASSERT_DELTA( bc.upper(), 20 ,0.0001);
    TS_ASSERT( !bc.hasLower() );
  }

  void testInitialize5()
  {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(Sigma==20)");
    TS_ASSERT_THROWS(bc.initialize(&gaus,expr),std::invalid_argument);
  }

  void testInitialize6()
  {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(a<Sigma<b)");
    TS_ASSERT_THROWS(bc.initialize(&gaus,expr),std::invalid_argument);
  }
};

#endif /*BOUNDARYCONSTRAINTTEST_H_*/
