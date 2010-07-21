#ifndef APPLYTRANSMISSIONCORRECTIONTEST_H_
#define APPLYTRANSMISSIONCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ApplyTransmissionCorrection.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/SolidAngleCorrection.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class ApplyTransmissionCorrectionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( correction.name(), "ApplyTransmissionCorrection" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( correction.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( correction.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( correction.initialize() )
    TS_ASSERT( correction.isInitialized() )
  }

  void testExec()
  {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/SANS2D/BioSANS_test_data.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    Mantid::DataHandling::MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace",inputWS);
    mover.setPropertyValue("ComponentName","detector1");
    // X = (16-192.0/2.0+0.5)*5.15/1000.0 = -0.409425
    // Y = (95-192.0/2.0+0.5)*5.15/1000.0 = -0.002575
    mover.setPropertyValue("X","0.409425");
    mover.setPropertyValue("Y","0.002575");
    mover.setPropertyValue("Z","6");
    mover.execute();

    // Perform solid angle correction
    Mantid::Algorithms::SolidAngleCorrection solidcorr;
    solidcorr.initialize();
    solidcorr.setPropertyValue("InputWorkspace",inputWS);
    solidcorr.setPropertyValue("OutputWorkspace",inputWS);
    solidcorr.execute();

    const std::string transWS("trans");
    DataObjects::Workspace2D_sptr trans_ws = WorkspaceCreationHelper::Create2DWorkspace154(1,1,1);
    trans_ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Wavelength");
    trans_ws->dataY(0)[0] = 0.6;
    trans_ws->dataE(0)[0] = 0.02;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(transWS, trans_ws);

    if (!correction.isInitialized()) correction.initialize();

    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("InputWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("TransmissionWorkspace",transWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( correction.setPropertyValue("OutputWorkspace",outputWS) )

    TS_ASSERT_THROWS_NOTHING( correction.execute() )

    TS_ASSERT( correction.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )

    // Spot check (multiply by counting time to be on the same scale as the IGOR result)
    double correct_result = 1800.0 * 0.359203;
    int id = 4+Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA( result->dataY(id)[0], correct_result, 0.001 )

    correct_result = 1800.0 * 0.44715;
    id = 176+Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA( result->dataY(id)[0], correct_result, 0.001 )

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(transWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::ApplyTransmissionCorrection correction;
  std::string inputWS;
};

#endif /*APPLYTRANSMISSIONCORRECTIONTEST_H_*/
