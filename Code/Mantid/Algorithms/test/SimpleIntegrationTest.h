#ifndef SIMPLEINTEGRATIONTEST_H_
#define SIMPLEINTEGRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SimpleIntegrationTest : public CxxTest::TestSuite
{
public:

  SimpleIntegrationTest()
  {
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    double *a = new double[25];
    double *e = new double[25];
    for (int i = 0; i < 25; ++i)
    {
      a[i]=i;
      e[i]=sqrt(double(i));
    }
    for (int j = 0; j < 5; ++j) {
      for (int k = 0; k < 6; ++k) {
        space2D->dataX(j)[k] = k;
      }
      space2D->setData(j, boost::shared_ptr<Mantid::MantidVec>(new Mantid::MantidVec(a+(5*j), a+(5*j)+5)),
          boost::shared_ptr<Mantid::MantidVec>(new Mantid::MantidVec(e+(5*j), e+(5*j)+5)));
    }
    // Register the workspace in the data service
    AnalysisDataService::Instance().add("testSpace", space);

  }

  ~SimpleIntegrationTest()
  {}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg.setPropertyValue("InputWorkspace","testSpace");
    outputSpace = "IntegrationOuter";
    alg.setPropertyValue("OutputWorkspace",outputSpace);

    alg.setPropertyValue("RangeLower","0.1");
    alg.setPropertyValue("RangeUpper","4.0");
    alg.setPropertyValue("StartWorkspaceIndex","2");
    alg.setPropertyValue("EndWorkspaceIndex","4");

    TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg2.setPropertyValue("InputWorkspace","testSpace");
    alg2.setPropertyValue("OutputWorkspace","out2");

    TS_ASSERT_THROWS_NOTHING( alg3.initialize());
    TS_ASSERT( alg3.isInitialized() );

    // Set the properties
    alg3.setPropertyValue("InputWorkspace","testSpace");
    alg3.setPropertyValue("OutputWorkspace","out3");
    alg3.setPropertyValue("RangeLower","0.1");
    alg3.setPropertyValue("RangeUpper","4.5");
    alg3.setPropertyValue("StartWorkspaceIndex","2");
    alg3.setPropertyValue("EndWorkspaceIndex","4");
    alg3.setPropertyValue("IncludePartialBins","1");
  }

  void testRangeNoPartialBins()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    int max;
    TS_ASSERT_EQUALS( max = output2D->getNumberHistograms(), 3)
    double yy[3] = {36,51,66};
    for (int i = 0; i < max; ++i)
    {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

      TS_ASSERT_EQUALS( x.size(), 2 )
      TS_ASSERT_EQUALS( y.size(), 1 );
      TS_ASSERT_EQUALS( e.size(), 1 );

      TS_ASSERT_EQUALS( x[0], 1.0 );
      TS_ASSERT_EQUALS( x[1], 4.0 );
      TS_ASSERT_EQUALS( y[0], yy[i] );
      TS_ASSERT_DELTA( e[0], sqrt(yy[i]), 0.001 );
    }
  }

  void testNoRangeNoPartialBins()
  {
    if ( !alg2.isInitialized() ) alg2.initialize();

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS( alg2.setPropertyValue("StartWorkspaceIndex","-1"), std::invalid_argument) ;

    TS_ASSERT_THROWS_NOTHING( alg2.execute());
    TS_ASSERT( alg2.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out2"));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 5)
    TS_ASSERT_EQUALS( output2D->dataX(0)[0], 0 );
    TS_ASSERT_EQUALS( output2D->dataX(0)[1], 5 );
    TS_ASSERT_EQUALS( output2D->dataY(0)[0], 10 );
    TS_ASSERT_EQUALS( output2D->dataY(4)[0], 110 );
    TS_ASSERT_DELTA ( output2D->dataE(2)[0], 7.746, 0.001 );
  }

  void testRangeWithPartialBins()
  {
    if ( !alg3.isInitialized() ) alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.execute());
    TS_ASSERT( alg3.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out3"));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    int max;
    TS_ASSERT_EQUALS( max = output2D->getNumberHistograms(), 3)
    const double yy[3] = {52.,74.,96.};
    const double ee[3] = {6.899,8.240,9.391};
    for (int i = 0; i < max; ++i)
    {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

      TS_ASSERT_EQUALS( x.size(), 2 )
      TS_ASSERT_EQUALS( y.size(), 1 );
      TS_ASSERT_EQUALS( e.size(), 1 );

      TS_ASSERT_EQUALS( x[0], 0.1 );
      TS_ASSERT_EQUALS( x[1], 4.5 );
      TS_ASSERT_EQUALS( y[0], yy[i] );
      TS_ASSERT_DELTA( e[0], ee[i], 0.001 );
    }

    //Test that the same values occur for a distribution
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve("testSpace"));
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    input2D->isDistribution(true);
    //Replace workspace
    AnalysisDataService::Instance().addOrReplace("testSpace", input2D);
    
    alg3.execute();
    //Retest
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out3"));

    output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS( max = output2D->getNumberHistograms(), 3)
    for (int i = 0; i < max; ++i)
    {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

      TS_ASSERT_EQUALS( x.size(), 2 )
      TS_ASSERT_EQUALS( y.size(), 1 );
      TS_ASSERT_EQUALS( e.size(), 1 );

      TS_ASSERT_EQUALS( x[0], 0.1 );
      TS_ASSERT_EQUALS( x[1], 4.5 );
      TS_ASSERT_EQUALS( y[0], yy[i] );
      TS_ASSERT_DELTA( e[0], ee[i], 0.001 );
    }
  }


private:
  SimpleIntegration alg;   // Test with range limits
  SimpleIntegration alg2;  // Test without limits
  SimpleIntegration alg3; // Test with range and partial bins
  std::string outputSpace;
};

#endif /*SIMPLEINTEGRATIONTEST_H_*/
