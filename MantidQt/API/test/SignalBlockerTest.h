#ifndef MANTID_API_SIGNALBLOCKERTEST_H_
#define MANTID_API_SIGNALBLOCKERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtAPI/SignalBlocker.h"
#include <QObject>

using MantidQt::API::SignalBlocker;

class SignalBlockerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SignalBlockerTest *createSuite() { return new SignalBlockerTest(); }
  static void destroySuite(SignalBlockerTest *suite) { delete suite; }

  void test_obtain_and_release_behaviour() {
    QObject *toBlock = new QObject;
    {
      SignalBlocker<QObject> scopedObj(toBlock);
      TSM_ASSERT("Should now block", toBlock->signalsBlocked());
    }
    TSM_ASSERT("Should no longer block", !toBlock->signalsBlocked());
    delete toBlock;
  }

  void test_access_pointer() {
    QObject *toBlock = new QObject;
    SignalBlocker<QObject> scopedObj(toBlock);
    TSM_ASSERT("Should now block", scopedObj->signalsBlocked());
    scopedObj.release();
    delete toBlock;
  }
};

#endif /* MANTID_API_SIGNALBLOCKERTEST_H_ */
