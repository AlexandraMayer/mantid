#ifndef MANTID_MDEVENTS_MERGEMDEWTEST_H_
#define MANTID_MDEVENTS_MERGEMDEWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MergeMDEW.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class MergeMDEWTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    MergeMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    do_test_exec("");
  }

  void test_exec_fileBacked()
  {
    do_test_exec("MergeMDEWTest_OutputWS.nxs");
  }
  
  void do_test_exec(std::string OutputFilename)
  {
    // Create a bunch of input files
    std::vector<std::string> filenames;
    std::vector<MDEventWorkspace3Lean::sptr> inWorkspaces;
    for (size_t i=0; i<3; i++)
    {
      std::ostringstream mess;
      mess << "MergeMDEWTestInput" << i;
      MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeFileBackedMDEW(mess.str(), true);
      inWorkspaces.push_back(ws);
      filenames.push_back(ws->getBoxController()->getFilename());
    }

    // Name of the output workspace.
    std::string outWSName("MergeMDEWTest_OutputWS");
  
    MergeMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Filenames", filenames) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputFilename", OutputFilename) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    std::string actualOutputFilename = alg.getPropertyValue("OutputFilename");

    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    TS_ASSERT_EQUALS( ws->getNPoints(), 30000);
    IMDBox3Lean * box = ws->getBox();
    TS_ASSERT_EQUALS( box->getNumChildren(), 1000);

    // Every sub-box has on average 30 events (there are 1000 boxes)
    // Check that each box has at least SOMETHING
    for (size_t i=0; i<box->getNumChildren(); i++)
      TS_ASSERT_LESS_THAN( 1, box->getChild(i)->getNPoints());

    if (!OutputFilename.empty())
    {
      TS_ASSERT( ws->isFileBacked() );
      TS_ASSERT( Poco::File(actualOutputFilename).exists());
      // Remove the file
      Poco::File(actualOutputFilename).remove();
    }

    // Cleanup generated input files
    for (size_t i=0; i<filenames.size(); i++)
    {
        Poco::File(filenames[i]).remove();
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }



};


#endif /* MANTID_MDEVENTS_MERGEMDEWTEST_H_ */

