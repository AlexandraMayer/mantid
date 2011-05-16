#ifndef IMD_MATRIX_WORKSPACETEST_H_
#define IMD_MATRIX_WORKSPACETEST_H_

//Tests the MatrixWorkspace as an IMDWorkspace.

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidAPI/MatrixWSIndexCalculator.h"

using std::size_t;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid { namespace DataObjects {
  
  class MatrixWorkspaceTester : public MatrixWorkspace
  {
  public:
    MatrixWorkspaceTester() : MatrixWorkspace() {}
    virtual ~MatrixWorkspaceTester() {}

    // Empty overrides of virtual methods
    virtual size_t getNumberHistograms() const { return 1;}
    const std::string id() const {return "MatrixWorkspaceTester";}
    void init(const size_t&, const size_t& j, const size_t&)
    {
      vec.resize(j,1.0);
      m_dataX.resize(j, vec);
      m_dataY.resize(j, vec);
      m_dataE.resize(j, vec);
      
      // Put an 'empty' axis in to test the getAxis method
      m_axes.push_back(new NumericAxis(1));
      m_axes[0]->title() = "1";
      m_axes.push_back(new NumericAxis(1));
      m_axes[1]->title() = "2";
    }
    bool isHistogramData() const {return true;}
    size_t size() const {return vec.size();}
    size_t blocksize() const {return vec.size();}
    MantidVec& dataX(size_t const i) {return m_dataX.at(i);}
    MantidVec& dataY(size_t const i) {return m_dataY.at(i);}
    MantidVec& dataE(size_t const i) {return m_dataE.at(i);}
    MantidVec& dataDx(size_t const i) {return m_dataE.at(i);}
    const MantidVec& dataX(size_t const i) const {return m_dataX.at(i);}
    const MantidVec& dataY(size_t const i) const {return m_dataY.at(i);}
    const MantidVec& dataE(size_t const i) const {return m_dataE.at(i);}
    const MantidVec& dataDx(size_t const i) const {return m_dataE.at(i);}
    Kernel::cow_ptr<MantidVec> refX(const size_t) const {return Kernel::cow_ptr<MantidVec>();}
    void setX(const size_t, const Kernel::cow_ptr<MantidVec>&) {}

  private:
    std::vector<MantidVec> m_dataX;
    std::vector<MantidVec> m_dataY;
    std::vector<MantidVec> m_dataE;
    MantidVec vec;
    int spec;
  };
}}// namespace

using Mantid::DataObjects::MatrixWorkspaceTester;

//Test the MatrixWorkspace as an IMDWorkspace.
class IMDWorkspaceTest : public CxxTest::TestSuite
{
private:

  Mantid::DataObjects::MatrixWorkspaceTester workspace;

public:

  IMDWorkspaceTest()
  {
    workspace.setTitle("workspace");
    workspace.initialize(2,4,3);
    for (int i = 0; i < 4; ++i)
    {
      workspace.dataX(0)[i] = i;
      workspace.dataX(1)[i] = i+4;
    }

    for (int i = 0; i < 3; ++i)
    {
      workspace.dataY(0)[i] = i*10;
      workspace.dataE(0)[i] = sqrt(workspace.dataY(0)[i]);
      workspace.dataY(1)[i] = i*100;
      workspace.dataE(1)[i] = sqrt(workspace.dataY(1)[i]);     
    }
  }

  void testGetXDimension() 
  { 
    MatrixWorkspaceTester matrixWS;
    matrixWS.init(1, 1, 1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dimension = matrixWS.getXDimension();
    std::string id = dimension->getDimensionId();
    TSM_ASSERT_EQUALS("Dimension-X does not have the expected dimension id.", "xDimension", id);
  }


  void testGetYDimension()
  {
    MatrixWorkspaceTester matrixWS;
    matrixWS.init(1, 1, 1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dimension = matrixWS.getYDimension();
    std::string id = dimension->getDimensionId();
    TSM_ASSERT_EQUALS("Dimension-Y does not have the expected dimension id.", "yDimension", id);
  }

  void testGetZDimension()
  {
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getZDimension(), std::logic_error);
  }

  void testGettDimension()
  {
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Current implementation should throw runtime error.", matrixWS.getTDimension(), std::logic_error);
  }

  void testGetDimensionThrows()
  {
    MatrixWorkspaceTester matrixWS;
    matrixWS.init(1,1,1);
    TSM_ASSERT_THROWS("Id doesn't exist. Should throw during find routine.", matrixWS.getDimension("3"), std::overflow_error);
  }

  void testGetDimension()
  {
    MatrixWorkspaceTester matrixWS;
    matrixWS.init(1,1,1);
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> dim = matrixWS.getDimension("yDimension");
    TSM_ASSERT_EQUALS("The dimension id found is not the same as that searched for.", "yDimension", dim->getDimensionId());
  }

  void testGetDimensionOverflow()
  {
    MatrixWorkspaceTester matrixWS;
    matrixWS.init(1,1,1);
    TSM_ASSERT_THROWS("The dimension does not exist. Attempting to get it should throw", matrixWS.getDimension("1"), std::overflow_error);
  }

  void testGetNPoints()
  {
    MatrixWorkspaceTester matrixWS;
    matrixWS.init(5, 5, 5);
    TSM_ASSERT_EQUALS("The expected number of points have not been returned.", 5, matrixWS.getNPoints());
  }

  void testGetCellElipsisParameterVersion()
  {
    MatrixWorkspaceTester matrixWS;
    TSM_ASSERT_THROWS("Cannot access higher dimensions should throw logic error.", matrixWS.getCell(1, 1, 1), std::logic_error);
    TSM_ASSERT_THROWS("Cannot access higher dimensions should throw logic error.", matrixWS.getCell(1, 1, 1, 1), std::logic_error);
    TSM_ASSERT_THROWS("Cannot access higher dimensions should throw logic error.", matrixWS.getCell(1, 1, 1, 1, 1, 1, 1, 1, 1), std::logic_error);
  }

  void testGetHistogramIndex()
  {
    MatrixWSIndexCalculator indexCalculator(5);
    HistogramIndex histogramIndexA = indexCalculator.getHistogramIndex(4);
    HistogramIndex histogramIndexB = indexCalculator.getHistogramIndex(5);
    HistogramIndex histogramIndexC = indexCalculator.getHistogramIndex(10);
    TSM_ASSERT_EQUALS("histogram index has not been calculated correctly.", 0, histogramIndexA);
    TSM_ASSERT_EQUALS("histogram index has not been calculated correctly.", 1, histogramIndexB);
    TSM_ASSERT_EQUALS("histogram index has not been calculated correctly.", 2, histogramIndexC);
  }

  void testGetBinIndex()
  {
    MatrixWSIndexCalculator indexCalculator(5);
    BinIndex binIndexA = indexCalculator.getBinIndex(4, 0);
    BinIndex binIndexB = indexCalculator.getBinIndex(12, 2);
    TSM_ASSERT_EQUALS("bin index has not been calculated correctly.", 4, binIndexA);
    TSM_ASSERT_EQUALS("bin index has not been calculated correctly.", 2, binIndexB);
  }


  void testGetCellSingleParameterVersion()
  {
    const Mantid::Geometry::SignalAggregate& cell = workspace.getCell(1);
    const Mantid::Geometry::SignalAggregate& point = workspace.getPoint(1);

    TSM_ASSERT_EQUALS("Signal values not correct. The cell should be the same as a point for the matrix ws.", point.getSignal(), cell.getSignal());
    TSM_ASSERT_EQUALS("Error values not correct. The cell should be the same as a point for the matrix ws.", point.getError(), cell.getError() );
  }

  void testGetPoint()
  {
    const Mantid::Geometry::SignalAggregate& pointA = workspace.getPoint(5);
    TSM_ASSERT_EQUALS("The expected mdpoint has not been returned on the basis of signal.", 100, pointA.getSignal());
    TSM_ASSERT_EQUALS("The expected mdpoint has not been returned on the basis of error.", 10, pointA.getError());
  }

  void testGetPointVertexes()
  {
    const Mantid::Geometry::SignalAggregate& pointA = workspace.getPoint(4);
    std::vector<coordinate> vertexes = pointA.getVertexes();
    TSM_ASSERT_EQUALS("Wrong number of vertexes returned", 4, vertexes.size());

    TSM_ASSERT_EQUALS("The v0 x-value is incorrect.", 4, vertexes.at(0).getX());
    TSM_ASSERT_EQUALS("The v0 y-value is incorrect.", 1, vertexes.at(0).getY());

    TSM_ASSERT_EQUALS("The v1 x-value is incorrect.", 5, vertexes.at(1).getX());
    TSM_ASSERT_EQUALS("The v1 y-value is incorrect.", 1, vertexes.at(1).getY());

    TSM_ASSERT_EQUALS("The v2 x-value is incorrect.", 4, vertexes.at(2).getX());
    TSM_ASSERT_EQUALS("The v2 y-value is incorrect.", 2, vertexes.at(2).getY());

    TSM_ASSERT_EQUALS("The v3 x-value is incorrect.", 5, vertexes.at(3).getX());
    TSM_ASSERT_EQUALS("The v3 y-value is incorrect.", 2, vertexes.at(3).getY());
  }

  void testGetWSLocationThrows()
  {
    TSM_ASSERT_THROWS("Getting the workspace location should not be possible.", workspace.getWSLocation(), std::logic_error);
  }

  void testGetGeometryXMLThrows()
  {
    //Characterisation test for existing behaviour.
    TSM_ASSERT_THROWS("No proper formulation of an underlying geometry yet in MatrixWorkspace", workspace.getGeometryXML(), std::runtime_error);
  }

};

#endif /*IMD_MATRIX_WORKSPACETEST_H_*/
