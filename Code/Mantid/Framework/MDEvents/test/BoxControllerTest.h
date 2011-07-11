#ifndef BOXPLITCONTROLLER_TEST_H
#define BOXPLITCONTROLLER_TEST_H

#include <cxxtest/TestSuite.h>

#include "MantidMDEvents/BoxController.h"
#include <memory>
#include <map>

using namespace Mantid;
using namespace Mantid::MDEvents;

class BoxControllerTest :    public CxxTest::TestSuite
{
public:

  void test_Constructor()
  {
    BoxController sc(2);
    TS_ASSERT_EQUALS( sc.getNDims(), 2);
  }

  void test_willSplit()
  {
    BoxController sc(2);
    sc.setMaxDepth(4);
    sc.setSplitThreshold(10);
    TS_ASSERT( sc.willSplit(100,3) );
    TS_ASSERT( !sc.willSplit(100,4) );
    TS_ASSERT( !sc.willSplit(2,3) );
    TS_ASSERT( !sc.willSplit(100,5) );
  }

  void test_getSplitInto()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 1000);
    TS_ASSERT_EQUALS( sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(1), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(2), 10);
    sc.setSplitInto(1,5);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 500);
    TS_ASSERT_EQUALS( sc.getSplitInto(0), 10);
    TS_ASSERT_EQUALS( sc.getSplitInto(1), 5);
    TS_ASSERT_EQUALS( sc.getSplitInto(2), 10);

  }

  void test_maxDepth()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    sc.setMaxDepth(6);
    TS_ASSERT_EQUALS( sc.getMaxDepth(), 6);
  }

  void test_IDs()
  {
    BoxController sc(3);
    // No IDs given out yet
    TS_ASSERT_EQUALS( sc.getMaxId(), 0 );
    // Start handing some out
    TS_ASSERT_EQUALS( sc.getNextId(), 0 );
    TS_ASSERT_EQUALS( sc.getNextId(), 1 );
    TS_ASSERT_EQUALS( sc.getNextId(), 2 );
    // You've got 3 given.
    TS_ASSERT_EQUALS( sc.getMaxId(), 3 );

    sc.setSplitInto(10);
    sc.setMaxDepth(6);
    TS_ASSERT_EQUALS( sc.getMaxDepth(), 6);
  }

  void test_maxNumBoxes()
  {
    BoxController sc(3);
    sc.setSplitInto(10);
    TS_ASSERT_EQUALS( sc.getNumSplit(), 1000);
    sc.setMaxDepth(6);
    {
      const std::vector<double> & max = sc.getMaxNumMDBoxes();
      TS_ASSERT_DELTA( max[0], 1, 1e-2);
      TS_ASSERT_DELTA( max[1], 1e3, 1e-2);
      TS_ASSERT_DELTA( max[2], 1e6, 1e-2);
      TS_ASSERT_DELTA( max[3], 1e9, 1e-2);
    }

    {
      // If you split into a different number, the values get reset too.
      sc.setSplitInto(5);
      TS_ASSERT_EQUALS( sc.getNumSplit(), 125);
      const std::vector<double> & max = sc.getMaxNumMDBoxes();
      TS_ASSERT_DELTA( max[0], 1, 1e-2);
      TS_ASSERT_DELTA( max[1], 125, 1e-2);
      TS_ASSERT_DELTA( max[2], 125*125, 1e-2);
    }
  }



  void doTest_numBoxes(BoxController & bc, size_t expectedNumEntries)
  {
    const std::vector<size_t> & num = bc.getNumMDBoxes();
    TS_ASSERT_EQUALS( num.size(), expectedNumEntries);
    TS_ASSERT_EQUALS( num[0], 1);
    TS_ASSERT_EQUALS( num[1], 0);

    // Average depth is 0 = all at level 0.
    TS_ASSERT_DELTA( bc.getAverageDepth(), 0.0, 1e-5 );

    bc.trackNumBoxes(0);
    TS_ASSERT_EQUALS( num[0], 0);
    TS_ASSERT_EQUALS( num[1], 100);

    // All at depth 1.0
    TS_ASSERT_DELTA( bc.getAverageDepth(), 1.0, 1e-5 );

    bc.trackNumBoxes(1);
    bc.trackNumBoxes(1);
    TS_ASSERT_EQUALS( num[0], 0);
    TS_ASSERT_EQUALS( num[1], 98);
    TS_ASSERT_EQUALS( num[2], 200);

    // Mostly at depth 1.0
    TS_ASSERT_DELTA( bc.getAverageDepth(), 1.02, 1e-5 );
  }

  /* Try setting these values in different orders */
  void test_trackNumBoxes1()
  {
    BoxController bc(2);
    bc.setSplitInto(10);
    bc.setMaxDepth(4);
    doTest_numBoxes(bc, 5);
  }

  /* This used to give wrong values */
  void test_trackNumBoxes2()
  {
    BoxController bc(2);
    bc.setMaxDepth(4);
    bc.setSplitInto(10);
    bc.setMaxDepth(10);
    doTest_numBoxes(bc, 11);
  }




};

#endif
