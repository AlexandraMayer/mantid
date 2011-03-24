#ifndef MANTID_ALGORITHMS_REMOVELOWRESTOFTEST_H_
#define MANTID_ALGORITHMS_REMOVELOWRESTOFTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <iostream>
#include <iomanip>
#include <set>
#include <string>


#include "MantidAlgorithms/RemoveLowResTOF.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class RemoveLowResTOFTest : public CxxTest::TestSuite
{
private:
  double BIN_DELTA;
  int NUMPIXELS;
  int NUMBINS;

  void makeFakeEventWorkspace(std::string wsName)
  {
    //Make an event workspace with 2 events in each bin.
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateEventWorkspace(NUMPIXELS, NUMBINS, NUMBINS, 0.0, BIN_DELTA, 2);
    //Fake a TOF unit in the data.
    test_in->getAxis(0)->unit() =UnitFactory::Instance().create("TOF");
    test_in->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(NUMPIXELS/9, false) );
    // fake a SpectraDetectorMap
    SpectraDetectorMap& myMap = test_in->mutableSpectraMap();
    myMap.clear();
    for (int i = 0; i < NUMPIXELS; i++)
    {
      std::set<int> detids;
      detids.insert(static_cast<int>(i+1));
      myMap.addSpectrumEntries(i, detids);
    }
    //Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

public:
  RemoveLowResTOFTest()
  {
    BIN_DELTA = 2.;
    NUMPIXELS = 36;
    NUMBINS = 50;
  }

  void test_RemoveLowResEventsInplace()
  {
    // setup
    std::string name("RemoveLowResTOF");
    this->makeFakeEventWorkspace(name);
    EventWorkspace_sptr ws
         = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(name));
    size_t num_events = ws->getNumberEvents();
    double min_event0 = ws->getEventList(0).getTofMin();
    double max_event0 = ws->getEventList(0).getTofMax();
    double min_eventN = ws->getEventList(NUMPIXELS - 1).getTofMin();
    double max_eventN = ws->getEventList(NUMPIXELS - 1).getTofMax();

    // run the algorithm
    RemoveLowResTOF algo;
    if (!algo.isInitialized()) algo.initialize();
    algo.setPropertyValue("InputWorkspace", name);
    algo.setPropertyValue("OutputWorkspace", name);
    algo.setProperty("ReferenceDIFC", 5.);
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    ws = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(name));
    TS_ASSERT_EQUALS(NUMPIXELS, ws->getNumberHistograms()); // shouldn't drop histograms
    TS_ASSERT(num_events > ws->getNumberEvents()); // should drop events

    // pixel 0 shouldn't be adjusted
    TS_ASSERT_EQUALS(min_event0, ws->getEventList(0).getTofMin());
    TS_ASSERT_EQUALS(max_event0, ws->getEventList(0).getTofMax());

    // pixel NUMPIXELS - 1 should be moved
    TS_ASSERT(min_eventN < ws->getEventList(NUMPIXELS - 1).getTofMin());
    TS_ASSERT_EQUALS(max_eventN, ws->getEventList(NUMPIXELS - 1).getTofMax());
  }


};


#endif /* MANTID_ALGORITHMS_REMOVELOWRESTOFTEST_H_ */

