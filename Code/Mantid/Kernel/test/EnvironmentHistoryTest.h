#ifndef ENVIRONMENTHISTORYTEST_H_
#define ENVIRONMENTHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/ConfigService.h"
#include <sstream>

using namespace Mantid::Kernel;

class EnvironmentHistoryTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    std::string correctOutput = "Framework Version: 1\n";
    correctOutput = correctOutput + "OS name: " + ConfigService::Instance().getOSName() + "\n";
    correctOutput = correctOutput + "OS version: " + ConfigService::Instance().getOSVersion() + "\n";
    correctOutput = correctOutput + "username: \n";

    // Not really much to test
    EnvironmentHistory EH;

    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(output << EH);
    TS_ASSERT_EQUALS(output.str(),correctOutput);
  }
};

#endif /* ALGORITHMPARAMETERTEST_H_*/
