#ifndef VTKPEAKMARKERFACTORY_TEST_H_
#define VTKPEAKMARKERFACTORY_TEST_H_

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace ::testing;
using namespace Mantid::VATES;
using Mantid::VATES::vtkPeakMarkerFactory;

class MockPeak : public Peak
{
public:
  MOCK_CONST_METHOD0(getHKL, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQLabFrame, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQSampleFrame, Mantid::Kernel::V3D (void));
};

class MockPeaksWorkspace : public PeaksWorkspace
{
public:
  MOCK_METHOD1(setInstrument, void (Mantid::Geometry::Instrument_const_sptr inst));
  MOCK_METHOD0(getInstrument, Mantid::Geometry::Instrument_const_sptr ());
  MOCK_CONST_METHOD0(clone, Mantid::DataObjects::PeaksWorkspace*());
  MOCK_CONST_METHOD0(getNumberPeaks, int());
  MOCK_METHOD1(removePeak, void (const int peakNum) );
  MOCK_METHOD1(addPeak, void (const IPeak& ipeak));
  MOCK_METHOD1(getPeak, Mantid::API::IPeak & (const int peakNum));
  MOCK_METHOD2(createPeak, Mantid::API::IPeak* (Mantid::Kernel::V3D QLabFrame, double detectorDistance));
};

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkPeakMarkerFactoryTest: public CxxTest::TestSuite
{

public:

  void do_test(MockPeak & peak1, vtkPeakMarkerFactory::ePeakDimensions dims)
  {
    boost::shared_ptr<MockPeaksWorkspace> pw_ptr(new MockPeaksWorkspace());
    MockPeaksWorkspace & pw = *pw_ptr;

    //Peaks workspace will return 5 identical peaks
    EXPECT_CALL( pw, getNumberPeaks()).WillOnce(Return(5));
    EXPECT_CALL( pw, getPeak(_)).WillRepeatedly( ReturnRef( peak1 ));

    vtkPeakMarkerFactory factory("signal", dims);
    factory.initialize(pw_ptr);
    vtkDataSet * set = factory.create();
    TS_ASSERT(set);
    TS_ASSERT_EQUALS( set->GetNumberOfPoints(), 5);
    TS_ASSERT_EQUALS(set->GetPoint(0)[0], 1.0);
    TS_ASSERT_EQUALS(set->GetPoint(0)[1], 2.0);
    TS_ASSERT_EQUALS(set->GetPoint(0)[2], 3.0);

    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&pw));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&peak1));
  }

  void test_q_lab()
  {
    MockPeak peak1;
    EXPECT_CALL( peak1, getQLabFrame()).Times(5).WillRepeatedly( Return( V3D(1,2,3) ));
    EXPECT_CALL( peak1, getHKL()).Times(0);
    EXPECT_CALL( peak1, getQSampleFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_Q_lab);
  }

  void test_q_sample()
  {
    MockPeak peak1;
    EXPECT_CALL( peak1, getQSampleFrame()).Times(5).WillRepeatedly( Return( V3D(1,2,3) ));
    EXPECT_CALL( peak1, getHKL()).Times(0);
    EXPECT_CALL( peak1, getQLabFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_Q_sample);
  }

  void test_hkl()
  {
    MockPeak peak1;
    EXPECT_CALL( peak1, getHKL()).Times(5).WillRepeatedly( Return( V3D(1,2,3) ));
    EXPECT_CALL( peak1, getQLabFrame()).Times(0);
    EXPECT_CALL( peak1, getQSampleFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_HKL);
  }

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkPeakMarkerFactory factory("signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateMeshOnlyThrows()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.createMeshOnly() , std::runtime_error);
  }

  void testCreateScalarArrayThrows()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.createScalarArray() , std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testTypeName()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory ("signal");
    TS_ASSERT_EQUALS("vtkPeakMarkerFactory", factory.getFactoryTypeName());
  }

};


#endif
