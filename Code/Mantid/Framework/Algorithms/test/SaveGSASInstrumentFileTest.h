#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SaveGSASInstrumentFile.h"

using Mantid::Algorithms::SaveGSASInstrumentFile;

class SaveGSASInstrumentFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveGSASInstrumentFileTest *createSuite() { return new SaveGSASInstrumentFileTest(); }
  static void destroySuite( SaveGSASInstrumentFileTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_ */