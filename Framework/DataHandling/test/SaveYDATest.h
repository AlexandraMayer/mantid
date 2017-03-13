#ifndef MANTID_DATAHANDLING_SAVEYDATEST_H_
#define MANTID_DATAHANDLING_SAVEYDATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveYDA.h"

using Mantid::DataHandling::SaveYDA;

class SaveYDATest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveYDATest *createSuite() { return new SaveYDATest(); }
  static void destroySuite( SaveYDATest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_SAVEYDATEST_H_ */