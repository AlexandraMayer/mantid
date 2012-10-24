#ifndef MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_
#define MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/EditInstrumentGeometry.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidGeometry/IDetector.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>


using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class EditInstrumentGeometryTest : public CxxTest::TestSuite
{
public:

  /** Test algorithm initialization
    */
  void test_Initialize()
  {

    EditInstrumentGeometry editdetector;
    TS_ASSERT_THROWS_NOTHING(editdetector.initialize());
    TS_ASSERT(editdetector.isInitialized());

  }

  /** Test for a workspace containing a single spectrum
    */
  void test_SingleSpectrum()
  {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr workspace2d = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 100, false);
    API::AnalysisDataService::Instance().add("inputWS", workspace2d);

    // 3. Set Property
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Workspace", "inputWS") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("SpectrumIDs","1") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("L2","3.45") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Polar","90.09") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Azimuthal","1.84") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setProperty("NewInstrument", false) );

    // 4. Run
    TS_ASSERT_THROWS_NOTHING( editdetector.execute() );
    TS_ASSERT( editdetector.isExecuted() );

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve("inputWS")) );

    API::ISpectrum* spectrum1 = workspace->getSpectrum(0);
    Geometry::Instrument_const_sptr instrument = workspace->getInstrument();

    std::set<detid_t> detids = spectrum1->getDetectorIDs();
    TS_ASSERT_EQUALS(detids.size(), 1);
    detid_t detid = 0;
    std::set<detid_t>::iterator it;
    for (it = detids.begin(); it != detids.end(); ++it){
      detid = *it;
    }
    Geometry::IDetector_const_sptr detector = instrument->getDetector(detid);
    double r, tth, phi;
    detector->getPos().getSpherical(r, tth, phi);
    TS_ASSERT_DELTA(r, 3.45, 0.000001);
    TS_ASSERT_DELTA(tth, 90.09, 0.000001);
    TS_ASSERT_DELTA(phi, 1.84, 0.000001);

  }

  /** Check detector parameter
    */
  void checkDetectorParameters(API::MatrixWorkspace_sptr workspace, size_t wsindex, double realr, double realtth, double realphi)
  {
    API::ISpectrum* spectrum1 = workspace->getSpectrum(wsindex);
    Geometry::Instrument_const_sptr instrument = workspace->getInstrument();

    std::set<detid_t> detids = spectrum1->getDetectorIDs();
    TS_ASSERT_EQUALS(detids.size(), 1);
    detid_t detid = 0;
    std::set<detid_t>::iterator it;
    for (it = detids.begin(); it != detids.end(); ++it){
      detid = *it;
    }
    Geometry::IDetector_const_sptr detector = instrument->getDetector(detid);
    double r, tth, phi;
    detector->getPos().getSpherical(r, tth, phi);
    TS_ASSERT_DELTA(r, realr, 0.000001);
    TS_ASSERT_DELTA(tth, realtth, 0.000001);
    TS_ASSERT_DELTA(phi, realphi, 0.000001);

  }

  /** Unit test to edit instrument parameters of all spectrums (>1)
    */
  void test_MultipleWholeSpectrumEdit()
  {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr workspace2d = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100, false);
    API::AnalysisDataService::Instance().add("inputWS2", workspace2d);


    // 3. Set Property
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Workspace", "inputWS2") );
    // TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("SpectrumIDs","3072,19456,40960,55296,74752,93184") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("SpectrumIDs","1,2,3,4,5,6") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("L2","1.1,2.2,3.3,4.4,5.5,6.6") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Polar","90.1,90.2,90.3,90.4,90.5,90.6") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Azimuthal","1,2,3,4,5,6") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setProperty("NewInstrument", false) );

    // 4. Run
    TS_ASSERT_THROWS_NOTHING( editdetector.execute() );
    TS_ASSERT( editdetector.isExecuted() );

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve("inputWS2")) );

    checkDetectorParameters(workspace, 0, 1.1, 90.1, 1.0);
    checkDetectorParameters(workspace, 1, 2.2, 90.2, 2.0);
    checkDetectorParameters(workspace, 3, 4.4, 90.4, 4.0);
    checkDetectorParameters(workspace, 5, 6.6, 90.6, 6.0);

  }


  /** Unit test to edit instrument parameters of all spectrums (>1)
    */
  void test_MultiplePartialSpectrumEdit()
  {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr workspace2d =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100, false);
    API::AnalysisDataService::Instance().add("inputWS3", workspace2d);

    // 3.1 Set Property
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Workspace", "inputWS3") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("SpectrumIDs","1,2,3") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("L2","1.1,2.2,3.3") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Polar","90.1,90.2,90.3") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Azimuthal","1,2,3") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setProperty("NewInstrument", false) );

    // 3.2 Run
    editdetector.execute();
    TS_ASSERT(!editdetector.isExecuted() );

  }

};


#endif /* MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_ */

