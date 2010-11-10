#ifndef MANTID_TESTDETECTOR__
#define MANTID_TESTDETECTOR__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"

using namespace Mantid::Geometry;

class testDetector : public CxxTest::TestSuite
{
public:
  void testNameConstructor()
  {
    Detector det("det1",0);
    TS_ASSERT_EQUALS(det.getName(),"det1");
    TS_ASSERT(!det.getParent());
    TS_ASSERT_EQUALS(det.getID(),0);
    TS_ASSERT(!det.isMasked());
    TS_ASSERT(!det.isMonitor());
  }

  void testNameParentConstructor()
  {
    Component parent("Parent");
    Detector det("det1", &parent);
    TS_ASSERT_EQUALS(det.getName(),"det1");
    TS_ASSERT(det.getParent());
    TS_ASSERT_EQUALS(det.getID(),0);
    TS_ASSERT(!det.isMasked());
    TS_ASSERT(!det.isMonitor());
  }

  void testId()
  {
    int id1=41;
    int id2=-43;
    Detector det("det1",0);
    TS_ASSERT_EQUALS(det.getID(),0);
    det.setID(id1);
    TS_ASSERT_EQUALS(det.getID(),id1);
    det.setID(id2);
    TS_ASSERT_EQUALS(det.getID(),id2);
  }

  void testType()
  {
    Detector det("det",0);
    TS_ASSERT_EQUALS(det.type(),"DetectorComponent");
  }

  void testDead()
  {
    Detector det("det",0);
    TS_ASSERT( ! det.isMasked() );
  }

  void testMonitor()
  {
    Detector det("det",0);
    TS_ASSERT( ! det.isMonitor() );
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor() );
    TS_ASSERT( det.isMonitor() );
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor(false) );
    TS_ASSERT( ! det.isMonitor() );
  }

  void testGetNumberParameter()
  {
    Detector det("det",0);
    TS_ASSERT_EQUALS(det.getNumberParameter("testparam").size(), 0);
  }

  void testGetPositionParameter()
  {
    Detector det("det",0);
    TS_ASSERT_EQUALS(det.getPositionParameter("testparam").size(), 0);

  }

  void testGetRotationParameter()
  {
    Detector det("det",0);
    TS_ASSERT_EQUALS(det.getRotationParameter("testparam").size(), 0);
  }

};

#endif
