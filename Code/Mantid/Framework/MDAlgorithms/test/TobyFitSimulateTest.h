#ifndef TOBYFITSIMULATETEST_H_
#define TOBYFITSIMULATETEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <boost/scoped_ptr.hpp> 
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidMDAlgorithms/TobyFitSimulate.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

//// Add a concrete IMDDimension class
//namespace Mantid
//{
//  namespace Geometry
//  {
//    class DLLExport TestIMDDimension : public IMDDimension
//    {
//    public:
//      virtual std::string getName() const { return("TestX"); }
//      virtual std::string getUnits() const { return("TestUnits"); }
//      virtual std::string getDimensionId() const { return("TestX"); }
//      virtual bool getIsIntegrated() const {return(0);}
//      virtual double getMaximum() const {return(1.0);}
//      virtual double getMinimum() const {return(0.0);}
//      virtual size_t getNBins() const {return(2);}
//      virtual std::string toXMLString() const { return "";}
//      virtual void setRange(size_t /*nBins*/, double /*min*/, double /*max*/){ };
//      virtual double getX(size_t)const {throw std::runtime_error("Not Implemented");}
//
//      TestIMDDimension() {};
//      ~TestIMDDimension() {};
//    };
//  }
//}
//
//// Test Cut data
//class DLLExport TestCut : public IMDWorkspace
//{
//private:
//   int m_points;
//   size_t m_cells;
//
//   std::vector<Mantid::Geometry::MDCell> m_mdcells;
//
//public:
//
//
//      virtual uint64_t getNPoints() const
//      {
//        return m_points;
//      }
//
//      /// return ID specifying the workspace kind
//      virtual const std::string id() const {return "TestIMDDWorkspace";}
//      /// Get the footprint in memory in bytes - return 0 for now
//      virtual size_t getMemorySize() const {return 0;};
//
//      virtual std::string getGeometryXML() const
//      {
//        throw std::runtime_error("Not implemented");
//      }
//
//   TestCut()
//   {
//      m_points=0;
//      m_cells=0;
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//   }
//
//   TestCut(std::vector<Mantid::Geometry::MDCell> pContribCells ) :
//           m_mdcells(pContribCells)
//   {
//      m_cells=pContribCells.size();
//      m_points=0;
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//      this->addDimension(new Mantid::Geometry::TestIMDDimension());
//   }
//   ~TestCut() {};
//};

class TobyFitSimulateTest : public CxxTest::TestSuite
{
private:
  boost::shared_ptr<MDHistoWorkspace> myCut;
  boost::shared_ptr<MDHistoWorkspace> outCut;
  std::string FakeWSname;

//  std::vector<MDCell> pContribCells;
//  std::vector<Mantid::Geometry::MDPoint> pnts1,pnts2;
//
//  //Helper constructional method - based on code from MD_CELL_TEST
//  // Returns a cell with one or 2 points depending on npnts
//  static Mantid::Geometry::MDCell constructMDCell(int npnts)
//  {
//    using namespace Mantid::Geometry;
//    std::vector<Coordinate> vertices;
//    Coordinate c = Coordinate::createCoordinate4D(4, 3, 2, 1);
//    vertices.push_back(c);
//
//    std::vector<boost::shared_ptr<MDPoint> > points;
//    if(npnts==1) {
//       points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(16,4,1,2,3,0)) );
//    }
//    else if(npnts==2) {
//       points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(25,5,1,2,3,1)) );
//       points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(36,6,1,2,3,2)) );
//    }
//
//    return  MDCell(points, vertices);
//  }
//
//
//  // Code from MDPoint test
//  class DummyDetector : public Mantid::Geometry::Detector
//  {
//  public:
//    DummyDetector(std::string name) : Mantid::Geometry::Detector(name, 0, NULL) {}
//    ~DummyDetector() {}
//  };
//
//  class DummyInstrument : public Mantid::Geometry::Instrument
//  {
//  public:
//    DummyInstrument(std::string name) : Mantid::Geometry::Instrument(name) {}
//    ~DummyInstrument() {}
//  };
//
//  //Helper constructional method.
//  static Mantid::Geometry::MDPoint* constructMDPoint(double s, double e, double x, double y, double z, double t)
//  {
//    using namespace Mantid::Geometry;
//    std::vector<Coordinate> vertices;
//    Coordinate c = Coordinate::createCoordinate4D(x, y, z, t);
//    vertices.push_back(c);
//    IDetector_sptr detector = IDetector_sptr(new DummyDetector("dummydetector"));
//    Instrument_sptr instrument = Instrument_sptr(new DummyInstrument("dummyinstrument"));
//    return new MDPoint(s, e, vertices, detector, instrument);
//  }

public:

  // create a test data set of 3 pixels contributing to 2 points to 1 cut
  void testInit()
  {
    FakeWSname = "test_FakeMDWS";

    // Fake MDWorkspace with 2x2x2x2 bins
    myCut = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4, 2, 2.0);
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(FakeWSname, myCut) );

//    pContribCells.push_back(constructMDCell(1));
//    pContribCells.push_back(constructMDCell(2));
//
//    myCut = boost::shared_ptr<TestCut> (new TestCut(pContribCells) ) ;
//    TS_ASSERT_EQUALS(myCut->getNPoints(),0);
//    TS_ASSERT_THROWS_ANYTHING(myCut->getPoint(0));
//    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(FakeWSname, myCut) );
//
//    outCut = AnalysisDataService::Instance().retrieveWS<TestCut>(FakeWSname);
//    TS_ASSERT_EQUALS(outCut->getNPoints(),0);
//    TS_ASSERT_EQUALS(myCut->getXDimension()->getNBins(),2);
//
//    std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > contributingPoints;
//    std::vector<Mantid::Geometry::Coordinate> vertices;
//
//    // test that cells and points are as expected
//    int firstCell = 0;
//    int secondCell = 1;
//    const SignalAggregate& firstMDCell=myCut->getCell(firstCell);
//    TS_ASSERT_THROWS_NOTHING( contributingPoints=firstMDCell.getContributingPoints() );
//    TS_ASSERT_EQUALS(contributingPoints.size(),1);
//    const SignalAggregate& secondMDCell=myCut->getCell(secondCell);
//    TS_ASSERT_THROWS_NOTHING( contributingPoints=secondMDCell.getContributingPoints() );
//    TS_ASSERT_EQUALS(contributingPoints.size(),2);
//    TS_ASSERT_THROWS_NOTHING(vertices=contributingPoints.at(0)->getVertexes());
//    TS_ASSERT_EQUALS(vertices.size(),1);
//    TS_ASSERT_EQUALS(vertices.at(0).gett(),1);
//    TS_ASSERT_EQUALS(vertices.at(0).getX(),1);

  }

  class TestableTobyFitSimulate : public Mantid::MDAlgorithms::TobyFitSimulate
  {
  public:
    TestableTobyFitSimulate() : TobyFitSimulate() {}
    ~TestableTobyFitSimulate() {}

    double wrapBose(double eps,double temp) { return TobyFitSimulate::bose(eps,temp) ;};

  };

  void testExecSimulate()
  {
    using namespace Mantid::MDAlgorithms;

    // testing basic functions
    TestableTobyFitSimulate tfSim;
    double temp=100.,eps=1.;
    TSM_ASSERT_DELTA("Bose(100,1) incorrect ",9.127015,tfSim.wrapBose(eps,temp),1.e-4);
    temp=100.; eps=-1;
    TSM_ASSERT_DELTA("Bose(100,1) incorrect ",8.127015,tfSim.wrapBose(eps,temp),1.e-4);
    temp=100.; eps=0.;
    TSM_ASSERT_DELTA("Bose(100,1) incorrect ",8.617347,tfSim.wrapBose(eps,temp),1.e-4);

  }
  void testTidyUp()
  {
  }

};

#endif /*TOBYFITSIMULATETEST_H_*/
