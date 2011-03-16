#ifndef MDBOXTEST_H
#define MDBOXTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MultiThreaded.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/BoxController.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDBoxTest :    public CxxTest::TestSuite
{

public:
  void test_default_constructor()
  {
    MDBox<MDEvent<3>,3> b3;
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);
    TS_ASSERT_EQUALS( b3.getDepth(), 0);
  }

  void test_constructor()
  {
    BoxController_sptr sc( new BoxController(3));
    MDBox<MDEvent<3>,3> b3(sc, 2);
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getBoxController(), sc);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);
    TS_ASSERT_EQUALS( b3.getDepth(), 2);
  }

  void test_setExtents()
  {
    MDBox<MDEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    TS_ASSERT_DELTA(b.getExtents(0).min, -10.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(0).max, +10.0, 1e-6);

    b.setExtents(1, -4.0, 6.0);
    TS_ASSERT_DELTA(b.getExtents(1).min, -4.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(1).max, +6.0, 1e-6);

    TS_ASSERT_THROWS( b.setExtents(2, 0, 1.0), std::invalid_argument);
  }


  void test_addEvent()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getNPoints(), 1)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*1, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*1, 1e-5);
  }

  void test_clear()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getNPoints(), 2)
    TS_ASSERT_DELTA( b.getSignal(), 2.4, 1e-5)
    b.clear();
    TS_ASSERT_EQUALS( b.getNPoints(), 0)
    TS_ASSERT_DELTA( b.getSignal(), 0.0, 1e-5)
    TS_ASSERT_DELTA( b.getErrorSquared(), 0.0, 1e-5)
  }

  void test_getEvents()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    TS_ASSERT_EQUALS( b.getEvents().size(), 3);
    TS_ASSERT_EQUALS( b.getEvents()[2].getSignal(), 4.0);
  }

  void test_getEventsCopy()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(4.0, 3.4);
    b.addEvent(ev);
    b.addEvent(ev);
    b.addEvent(ev);
    std::vector<MDEvent<2> > * events;
    events = b.getEventsCopy();
    TS_ASSERT_EQUALS( events->size(), 3);
    TS_ASSERT_EQUALS( (*events)[2].getSignal(), 4.0);
  }

  void test_sptr()
  {
    MDBox<MDEvent<3>,3>::sptr a( new MDBox<MDEvent<3>,3>());
    TS_ASSERT_EQUALS( sizeof(a), 16);
  }


  /** Add a vector of events */
  void test_addEvents()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    std::vector< MDEvent<2> > vec;
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);
    vec.push_back(ev);
    vec.push_back(ev);
    vec.push_back(ev);
    b.addEvents(vec);
    TS_ASSERT_EQUALS( b.getNPoints(), 3)
    TS_ASSERT_DELTA( b.getEvents()[2].getSignal(), 1.2, 1e-5)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*3, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*3, 1e-5);
  }


  /** Try to add a large number of events in parallel
   * to the same MDBox, to make sure it is thread-safe.
   */
  void test_addEvent_inParallel()
  {
    MDBox<MDEvent<2>,2> b;
    MDEvent<2> ev(1.2, 3.4);
    ev.setCenter(0, 2.0);
    ev.setCenter(1, 3.0);

    int num = 5e5;
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < num; i++)
    {
      b.addEvent(ev);
    }

    TS_ASSERT_EQUALS( b.getNPoints(), num)
    // Did it keep a running total of the signal and error?
    TS_ASSERT_DELTA( b.getSignal(), 1.2*num, 1e-5*num);
    TS_ASSERT_DELTA( b.getErrorSquared(), 3.4*num, 1e-5*num);

  }

  void test_bad_splitter()
  {
    BoxController_sptr sc( new BoxController(4));
    sc->setSplitThreshold(10);
    typedef MDBox<MDEvent<3>,3> MACROS_ARE_DUMB; //...since they get confused by commas
    TS_ASSERT_THROWS( MACROS_ARE_DUMB b3(sc), std::invalid_argument);
  }


  void test_splitter()
  {
    BoxController_sptr sc( new BoxController(3));
    sc->setSplitThreshold(10);
    MDBox<MDEvent<3>,3> b3(sc);
    TS_ASSERT_EQUALS( b3.getNumDims(), 3);
    TS_ASSERT_EQUALS( b3.getNPoints(), 0);

    MDEvent<3> ev(1.2, 3.4);
    std::vector< MDEvent<3> > vec;
    for(int i=0; i < 12; i++) vec.push_back(ev);
    b3.addEvents( vec );

    TS_ASSERT_EQUALS( b3.getBoxController(), sc);

  }
};


#endif

