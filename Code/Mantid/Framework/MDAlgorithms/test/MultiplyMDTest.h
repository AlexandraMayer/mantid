#ifndef MANTID_MDALGORITHMS_MULTIPLYMDTEST_H_
#define MANTID_MDALGORITHMS_MULTIPLYMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/MultiplyMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::MDEvents::MDHistoWorkspace_sptr;

/** Note: More detailed tests for the underlying
 * operations are in BinaryOperationMDTest and
 * MDHistoWorkspaceTest.
 *
 */
class MultiplyMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    MultiplyMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MultiplyMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 6.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MultiplyMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 6.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("MultiplyMD", "scalar", "histo_A", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 6.0, 1e-5);
  }

  void test_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("MultiplyMD", "event_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("MultiplyMD", "scalar", "event_A", "out", false /*fails*/);
  }

};


#endif /* MANTID_MDALGORITHMS_MULTIPLYMDTEST_H_ */
