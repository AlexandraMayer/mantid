#ifndef MANTID_ALGORITHMS_BASICINSTRUMENTINFOTEST_H_
#define MANTID_ALGORITHMS_BASICINSTRUMENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/BasicInstrumentInfo.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class BasicInstrumentInfoTest : public CxxTest::TestSuite {
public:
  static BasicInstrumentInfoTest *createSuite() {
    return new BasicInstrumentInfoTest();
  }
  static void destroySuite(BasicInstrumentInfoTest *suite) { delete suite; }

  BasicInstrumentInfoTest() {
    workspace = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
        1, 2, false);
  }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(BasicInstrumentInfo(*workspace));
  }

  void test_getInstrument() {
    auto info = BasicInstrumentInfo(*workspace);
    // This might fail if we get a null instrument, so we test for throw. Since
    // workspace->getInstrument() creates a copy of the instrument there is no
    // point in attempting to verify that the pointer is "correct".
    TS_ASSERT_THROWS_NOTHING(info.getInstrument());
  }

  void test_getSource() {
    auto info = BasicInstrumentInfo(*workspace);
    TS_ASSERT_THROWS_NOTHING(info.getSource());
  }

  void test_getSample() {
    auto info = BasicInstrumentInfo(*workspace);
    TS_ASSERT_THROWS_NOTHING(info.getSample());
  }

private:
  EventWorkspace_sptr workspace;
};

#endif /* MANTID_ALGORITHMS_BASICINSTRUMENTINFOTEST_H_ */
