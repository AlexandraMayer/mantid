#ifndef MANTID_ALGORITHMS_SOFQW2TEST_H_
#define MANTID_ALGORITHMS_SOFQW2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQW2.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

#include <iomanip>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class SofQW2Test : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    SofQW2 alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }


  void test_exec()
  {
    SofQW2 sqw;
    sqw.initialize();

    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","IRS26173_ipg.nxs");
    const std::string inputWS = "inputWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    Mantid::API::MatrixWorkspace_sptr inWS;
    TS_ASSERT_THROWS_NOTHING( inWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(inputWS)) );
    WorkspaceHelpers::makeDistribution(inWS);

    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("InputWorkspace",inputWS) );
    const std::string outputWS = "result";
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("QAxisBinning","0.5,0.25,2") );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EMode","Indirect") );
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EFixed","1.84") );

    TS_ASSERT_THROWS_NOTHING( sqw.execute() );
    TS_ASSERT( sqw.isExecuted() );

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );

    TS_ASSERT_EQUALS( result->getAxis(0)->length(), 1904 );
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "DeltaE" );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(0), -0.5590, 0.0001 );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(999), -0.0971, 0.0001 );
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(1900), 0.5728, 0.0001 );

    TS_ASSERT_EQUALS( result->getAxis(1)->length(), 7 );
    TS_ASSERT_EQUALS( result->getAxis(1)->unit()->unitID(), "MomentumTransfer" );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(0), 0.5 );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(3), 1.25 );
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(6), 2.0 );

    int indices[6] = {1160, 1145, 1200, 99, 1654, 1025};

    for( size_t i = 0; i < 6; ++i )
    {
      std::cerr << std::setprecision(12) << result->readY(i)[indices[i]] << "  " << result->readE(i)[indices[i]] << "\n";
    }

    const double delta(1e-08);
    TS_ASSERT_DELTA( result->readY(0)[1160], 13.71406100 , delta);
    TS_ASSERT_DELTA( result->readE(0)[1160], 0.062926550, delta);
    TS_ASSERT_DELTA( result->readY(1)[1145], 5.681229516, delta);
    TS_ASSERT_DELTA( result->readE(1)[1145], 0.040755760, delta);
    TS_ASSERT_DELTA( result->readY(2)[1200], 1.662663838, delta);
    TS_ASSERT_DELTA( result->readE(2)[1200], 0.021415982, delta);
    TS_ASSERT_DELTA( result->readY(3)[99],   0.042476730, delta);
    TS_ASSERT_DELTA( result->readE(3)[99],   0.007617287, delta);
    TS_ASSERT_DELTA( result->readY(4)[1654], 0.01732786, delta);
    TS_ASSERT_DELTA( result->readE(4)[1654], 0.00182863, delta);
    TS_ASSERT_DELTA( result->readY(5)[1025], 0.05377700, delta);
    TS_ASSERT_DELTA( result->readE(5)[1025], 0.00408489, delta);

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);    
  }
};


#endif /* MANTID_ALGORITHMS_SOFQW2TEST_H_ */

