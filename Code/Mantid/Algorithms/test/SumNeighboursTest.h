#ifndef SumNeighboursTEST_H_
#define SumNeighboursTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SumNeighbours.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/LoadSNSEventNexus.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SumNeighboursTest : public CxxTest::TestSuite
{
public:

//  SumNeighboursTest()
//  {
//    outputSpace1 = "SNAP_sum";
//    inputSpace = "SNAP";
//
//    Mantid::NeXus::LoadSNSEventNexus loader;
//    loader.initialize();
//    //loader.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7850_event.nxs");
//    loader.setPropertyValue("Filename","/home/janik/data/SNAP_4105_event.nxs");
//    loader.setPropertyValue("OutputWorkspace",inputSpace);
//    loader.execute();
//
//  }
//
//  ~SumNeighboursTest()
//  {}
//
//  void xtestBadInputs()
//  {
//    alg.initialize();
//    TS_ASSERT( alg.isInitialized() );
//    alg.setPropertyValue("InputWorkspace",inputSpace) ;
//    alg.setPropertyValue("OutputWorkspace",outputSpace1) ;
//    alg.setPropertyValue("SumX","12"); //! This is not valid :)
//    alg.setPropertyValue("SumY","16");
//    alg.execute();
//    TS_ASSERT( !alg.isExecuted());
//  }
//
//
//  void testExec_SNAP()
//  {
//    alg.initialize();
//    TS_ASSERT( alg.isInitialized() );
//
//    // Set the properties
//    alg.setPropertyValue("InputWorkspace",inputSpace) ;
//    alg.setPropertyValue("OutputWorkspace",outputSpace1) ;
//    alg.setPropertyValue("SumX","4");
//    alg.setPropertyValue("SumY","16");
//    //alg.setPropertyValue("DetectorNames","E1,E2,E3,E4,E5,E6,E7,E8,E9,W1,W2,W3,W4,W5,W6,W7,W8,W9");
//    //alg.setPropertyValue("DetectorNames","bank1,bank2,bank3");
//
//    alg.execute();
//
//    //TS_ASSERT_EQUALS(
//  }


private:
  SumNeighbours alg;   // Test with range limits
  std::string outputSpace1;
  std::string outputSpace2;
  std::string inputSpace;
};

#endif /*SumNeighboursTEST_H_*/


