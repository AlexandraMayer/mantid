#ifndef GETMASKEDDETECTORS_H_
#define GETMASKEDDETECTORS_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/GetMaskedDetectors.h"
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <vector>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class GetMaskedDetectorsTest : public CxxTest::TestSuite
{
public:
  GetMaskedDetectorsTest()
  {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    MantidVecPtr x,vec;
    x.access().resize(6,10.0);
    vec.access().resize(5,1.0);
    int forSpecDetMap[5];
    for (int j = 0; j < 5; ++j)
    {
      space2D->setX(j,x);
      space2D->setData(j,vec,vec);
      space2D->getAxis(1)->spectraNo(j) = j;
      forSpecDetMap[j] = j;
    }

    Instrument_sptr instr = boost::dynamic_pointer_cast<Instrument>(space->getBaseInstrument);

    Detector *d = new Detector("det",0);
    d->setID(0);
    instr->markAsDetector(d);
    Detector *d1 = new Detector("det",0);
    d1->setID(1);
    instr->markAsDetector(d1);
    Detector *d2 = new Detector("det",0);
    d2->setID(2);
    instr->markAsDetector(d2);
    Detector *d3 = new Detector("det",0);
    d3->setID(3);
    instr->markAsDetector(d3);
    Detector *d4 = new Detector("det",0);
    d4->setID(4);
    instr->markAsDetector(d4);

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, 5 );

    // Register the workspace in the data service
    inputWS = "test_masked_ws";
    AnalysisDataService::Instance().add(inputWS, space);

    // Mask detectors in the test workspace
    MaskDetectors marker_mask;
    marker_mask.initialize();
    marker_mask.setPropertyValue("Workspace", inputWS);
    marker_mask.setPropertyValue("DetectorList","1,3");
    marker_mask.execute();
    boost::shared_ptr<IInstrument> instrument = space->getInstrument();
    TS_ASSERT( instrument->getDetector(1)->isMasked() )
    TS_ASSERT( instrument->getDetector(3)->isMasked() )
  }

  void testName()
  {
    TS_ASSERT_EQUALS( marker.name(), "GetMaskedDetectors" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( marker.version(), 1 )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( marker.initialize() )
    TS_ASSERT( marker.isInitialized() );
  }

  void testExec()
  {
    if ( !marker.isInitialized() ) marker.initialize();
    marker.setPropertyValue("InputWorkspace", inputWS);
    TS_ASSERT_THROWS_NOTHING( marker.execute());
    TS_ASSERT( marker.isExecuted() );

    std::vector<int> list = marker.getProperty("DetectorList");

    TS_ASSERT_EQUALS(list.size(), 2);
    TS_ASSERT_EQUALS(list[0], 1);
    TS_ASSERT_EQUALS(list[1], 3);

    AnalysisDataService::Instance().remove(inputWS);
  }

private:
  GetMaskedDetectors marker;
  std::string inputWS;
};

#endif /*MARKDEADDETECTORSTEST_H_*/
