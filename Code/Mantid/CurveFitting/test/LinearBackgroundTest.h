#ifndef LINEARBACKGROUNDTEST_H_
#define LINEARBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/LinearBackground.h"

#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::CurveFitting::LinearBackground;
using Mantid::CurveFitting::Fit;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;


class LinearBackgroundTest : public CxxTest::TestSuite
{
public:



  void testAgainstHRPDData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // load hrpd dataset to test against
    std::string inputFile = "../../../../Test/Data/HRP39182.RAW";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string wsName = "HRPD_Dataset";
    loader.setPropertyValue("OutputWorkspace", wsName);
    loader.execute();


    // set up gaussian fitting function
    LinearBackground* quad = new LinearBackground();
    quad->init();

    quad->getParameter("A0") = 3.0;

    alg2.setFunction(quad);


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","66000");
    alg2.setPropertyValue("EndX","67000"); // not this test for now break if interval increased

    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    //std::string str = alg2.getProperty("Output Status");
    //std::cout << std::endl << str << std::endl;
    TS_ASSERT_DELTA( dummy, 2.7,0.1);
    TS_ASSERT_DELTA( quad->getParameter("A0"),2.5183, 0.0001);
    TS_ASSERT_DELTA( quad->getParameter("A1"),-0.0001, 0.0001);


  }

};

#endif /*LINEARBACKGROUNDTEST_H_*/
