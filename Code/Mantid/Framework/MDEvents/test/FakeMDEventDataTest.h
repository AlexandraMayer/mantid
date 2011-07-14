#ifndef MANTID_MDEVENTS_FAKEMDEVENTDATATEST_H_
#define MANTID_MDEVENTS_FAKEMDEVENTDATATEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/FakeMDEventData.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class FakeMDEventDataTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    FakeMDEventData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    FakeMDEventData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("FakeMDEventDataTest_ws", in_ws);

    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "FakeMDEventDataTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakParams", "1000, 5.0,5.0,5.0, 1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("UniformParams", "10000"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );

    // Now there are 11000 more points.
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 12000);

    AnalysisDataService::Instance().remove("FakeMDEventDataTest_ws");
  }

  void test_exec_randomizeSignal()
  {
    FakeMDEventData alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    MDEventWorkspace3::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    AnalysisDataService::Instance().addOrReplace("FakeMDEventDataTest_ws", in_ws);

    // No events
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 0);
    TS_ASSERT_DELTA( in_ws->getBox()->getSignal(), 0.0, 1e-5);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "FakeMDEventDataTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakParams", "100, 5.0,5.0,5.0, 1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("UniformParams", "100"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("RandomizeSignal", "1"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );

    // Now there are 200 more points.
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 200);
    // 200 +- 100 signal
    TS_ASSERT_DELTA( in_ws->getBox()->getSignal(), 200.0, 100);
    TS_ASSERT_DELTA( in_ws->getBox()->getErrorSquared(), 200.0, 100);
    // But not exactly 200
    TS_ASSERT_DIFFERS( in_ws->getBox()->getSignal(), 200.0);
    TS_ASSERT_DIFFERS( in_ws->getBox()->getErrorSquared(), 200.0);

    AnalysisDataService::Instance().remove("FakeMDEventDataTest_ws");
  }


};


#endif /* MANTID_MDEVENTS_FAKEMDEVENTDATATEST_H_ */

