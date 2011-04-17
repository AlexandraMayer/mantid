#ifndef VTK_THRESHOLDING_UNSTRUCTURED_GRID_FACTORY_TEST_H_
#define VTK_THRESHOLDING_UNSTRUCTURED_GRID_FACTORY_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"


class vtkThresholdingUnstructuredGridFactoryTest: public CxxTest::TestSuite
{

private:

  ///Helper class. Concrete instance of IMDDimension.
  class FakeIMDDimension: public Mantid::Geometry::IMDDimension
  {
  private:
    std::string m_id;
  public:
    FakeIMDDimension(std::string id) : m_id(id) {}
    std::string getName() const {throw std::runtime_error("Not implemented");}
    std::string getUnits() const {throw std::runtime_error("Not implemented");}
    std::string getDimensionId() const {return m_id;}
    double getMaximum() const {return 10;}
    double getMinimum() const {return 0;};
    size_t getNBins() const {return 10;};
    std::string toXMLString() const {throw std::runtime_error("Not implemented");};
    double getX(size_t) const {throw std::runtime_error("Not implemented");};
    virtual ~FakeIMDDimension()
    {
    }
  };

  /// Mock IMDDimension.
  class MockIMDWorkspace: public Mantid::API::IMDWorkspace
  {
  public:

    MOCK_CONST_METHOD0(id, const std::string());
    MOCK_CONST_METHOD0(getMemorySize, size_t());
    MOCK_CONST_METHOD1(getPoint,const Mantid::Geometry::SignalAggregate&(unsigned int index));
    MOCK_CONST_METHOD1(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment));
    MOCK_CONST_METHOD2(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment, unsigned int dim2Increment));
    MOCK_CONST_METHOD3(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment));
    MOCK_CONST_METHOD4(getCell,const Mantid::Geometry::SignalAggregate&(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment, unsigned int dim4Increment));

    MOCK_CONST_METHOD0(getWSLocation,std::string());
    MOCK_CONST_METHOD0(getGeometryXML,std::string());

    MOCK_CONST_METHOD0(getXDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getYDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getZDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD0(getTDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>());
    MOCK_CONST_METHOD1(getDimension,boost::shared_ptr<const Mantid::Geometry::IMDDimension>(std::string id));
    MOCK_METHOD1(getDimensionNum,boost::shared_ptr<Mantid::Geometry::IMDDimension>(size_t index));
    MOCK_CONST_METHOD0(getDimensionIDs,const std::vector<std::string>());
    MOCK_CONST_METHOD0(getNPoints, uint64_t());
    MOCK_CONST_METHOD0(getNumDims, size_t());
    MOCK_CONST_METHOD4(getSignalAt, double(size_t index1, size_t index2, size_t index3, size_t index4));

    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw std::runtime_error("Not Implemented");
    }

    virtual ~MockIMDWorkspace() {}
  };

  public:

  void testThresholds()
  {

    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getSignalAt(_, _, _, _)).Times(AtLeast(1)).WillRepeatedly(Return(1));
    EXPECT_CALL(*pMockWs, getXDimension()).Times(9).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(9).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("y"))));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(9).WillRepeatedly(Return(IMDDimension_const_sptr(
        new FakeIMDDimension("z"))));

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Set up so that only cells with signal values == 1 should not be filtered out by thresholding.

    vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep> inside(ws_sptr, "signal", 0, timeMapper,
        0, 2);
    vtkUnstructuredGrid* insideProduct = inside.create();

    vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep> below(ws_sptr, "signal", 0, timeMapper,
        0, 0.5);
    vtkUnstructuredGrid* belowProduct = below.create();

    vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep> above(ws_sptr, "signal", 0, timeMapper,
        2, 3);
    vtkUnstructuredGrid* aboveProduct = above.create();

    TS_ASSERT_EQUALS((9*9*9), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfCells());
  }

  void testSignalAspects()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    using namespace testing;
    TimeStepToTimeStep timeMapper;

    MockIMDWorkspace* pMockWs = new MockIMDWorkspace;
    EXPECT_CALL(*pMockWs, getSignalAt(_, _, _, _)).WillRepeatedly(Return(1)); //Shouldn't access getSignal At
    EXPECT_CALL(*pMockWs, getXDimension()).Times(AtLeast(1)).WillRepeatedly(Return(
        IMDDimension_const_sptr(new FakeIMDDimension("x"))));
    EXPECT_CALL(*pMockWs, getYDimension()).Times(AtLeast(1)).WillRepeatedly(Return(
        IMDDimension_const_sptr(new FakeIMDDimension("y"))));
    EXPECT_CALL(*pMockWs, getZDimension()).Times(AtLeast(1)).WillRepeatedly(Return(
        IMDDimension_const_sptr(new FakeIMDDimension("z"))));
    EXPECT_CALL(*pMockWs, getTDimension()).Times(0);

    Mantid::API::IMDWorkspace_sptr ws_sptr(pMockWs);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep> factory =
        vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep> (ws_sptr, "signal", 0, timeMapper);

    vtkDataSet* product = factory.create();
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    const int correctCellNumber = 9 * 9 * 9;
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    product->Delete();
  }

  void testIsVtkDataSetFactory()
  {
    //TODO
  }

};

#endif
