#ifndef MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/CreateMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class CreateMDEventWorkspaceTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CreateMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  /** Validate bad inputs. */
  void test_validation()
  {
    TS_ASSERT(  !AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 4,
        "OutputWorkspace","failed_output",
        "Dimensions", "0")->isExecuted() );
    TS_ASSERT(  !AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 6,
        "OutputWorkspace","failed_output",
        "Dimensions", "3",
        "Extents", "-1,1,-2,2")->isExecuted() );
    TS_ASSERT(  !AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 6,
        "OutputWorkspace","failed_output",
        "Dimensions", "3",
        "Extents", "-1,1,-2,2,3,3,4,4")->isExecuted() );
    TS_ASSERT(  !AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 8,
        "OutputWorkspace","failed_output",
        "Dimensions", "3", "Extents", "-1,1,-2,2,3,3",
        "Names", "One,Two")->isExecuted() );
    TS_ASSERT(  !AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 12,
        "OutputWorkspace","failed_output",
        "Dimensions", "3", "Extents", "-1,1,-2,2,3,3",
        "Names", "One,Two,Three",
        "MinRecursionDepth", "5",
        "MaxRecursionDepth", "4")->isExecuted() );
    // Uses too much memory
    TS_ASSERT(  !AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 14,
        "OutputWorkspace","failed_output",
        "Dimensions", "3", "Extents", "-1,1,-2,2,3,3",
        "Names", "One,Two,Three",
        "Units", "One,Two,Three",
        "SplitInto", "10",
        "MinRecursionDepth", "5",
        "MaxRecursionDepth", "5")->isExecuted() );
  }

  void do_test_exec(std::string Filename, bool lean, int MinRecursionDepth=0, int expectedNumMDBoxes=216)
  {
    std::string wsName = "CreateMDEventWorkspaceTest_out";
    CreateMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("Dimensions", "3");
    alg.setPropertyValue("EventType", lean ? "MDLeanEvent" : "MDEvent");
    alg.setPropertyValue("Extents", "-1,1,-2,2,-3,3");
    alg.setPropertyValue("Names", "x,y,z");
    alg.setPropertyValue("Units", "m,mm,um");
    alg.setPropertyValue("SplitInto", "6");
    alg.setPropertyValue("SplitThreshold", "500");
    alg.setProperty("MinRecursionDepth", MinRecursionDepth);
    alg.setProperty("MaxRecursionDepth", 7);
    alg.setPropertyValue("OutputWorkspace",wsName);
    alg.setPropertyValue("Filename", Filename);
    alg.setPropertyValue("Memory", "1");

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Get it from data service
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = boost::dynamic_pointer_cast<IMDEventWorkspace>( AnalysisDataService::Instance().retrieve(wsName) ));
    TS_ASSERT( ws );

    // Correct info?
    TS_ASSERT_EQUALS( ws->getNumDims(), 3);
    TS_ASSERT_EQUALS( ws->getNPoints(), 0);

    IMDDimension_sptr dim;
    dim = ws->getDimension(0);
    TS_ASSERT_DELTA( dim->getMaximum(), 1.0, 1e-6);
    TS_ASSERT_EQUALS( dim->getName(), "x");
    TS_ASSERT_EQUALS( dim->getUnits(), "m");
    dim = ws->getDimension(1);
    TS_ASSERT_DELTA( dim->getMaximum(), 2.0, 1e-6);
    TS_ASSERT_EQUALS( dim->getName(), "y");
    TS_ASSERT_EQUALS( dim->getUnits(), "mm");
    dim = ws->getDimension(2);
    TS_ASSERT_DELTA( dim->getMaximum(), 3.0, 1e-6);
    TS_ASSERT_EQUALS( dim->getName(), "z");
    TS_ASSERT_EQUALS( dim->getUnits(), "um");

    // What about the box controller
    BoxController_sptr bc;

    if (lean)
    {
      MDEventWorkspace3Lean::sptr ews = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(ws);
      TS_ASSERT( ews ); if (!ews) return;
      bc = ews->getBoxController();
    }
    else
    {
      MDEventWorkspace3::sptr ews = boost::dynamic_pointer_cast<MDEventWorkspace3>(ws);
      TS_ASSERT( ews ); if (!ews) return;
      bc = ews->getBoxController();
    }

    TS_ASSERT( bc );
    if (!bc) return;
    TS_ASSERT_EQUALS(bc->getSplitInto(0), 6 );
    TS_ASSERT_EQUALS(bc->getSplitThreshold(), 500 );
    TS_ASSERT_EQUALS(bc->getMaxDepth(), 7 );

    TS_ASSERT_EQUALS(bc->getTotalNumMDBoxes(), expectedNumMDBoxes);

    if (Filename != "")
    {
      std::string s = alg.getPropertyValue("Filename");
      TSM_ASSERT( "File for the back-end was created.", Poco::File(s).exists() );
      std::cout << "Closing the file." << std::endl;
      bc->closeFile();
      if (Poco::File(s).exists()) Poco::File(s).remove();
    }
  }

  void test_exec_MDEvent()
  {
    do_test_exec("", false);
  }

  void test_exec_MDEvent_fileBacked()
  {
    do_test_exec("CreateMDEventWorkspaceTest.nxs", false);
  }

  void test_exec_MDLeanEvent()
  {
    do_test_exec("", true);
  }

  void test_exec_MDLeanEvent_fileBacked()
  {
    do_test_exec("CreateMDEventWorkspaceTest.nxs", true);
  }


  void test_exec_MinRecursionDepth()
  {
    do_test_exec("", true, 2, 216*216);
  }
};


#endif /* MANTID_MDEVENTS_CREATEMDEVENTWORKSPACETEST_H_ */

