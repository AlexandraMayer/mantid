#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/BinMD.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include "MantidKernel/Strings.h"
#include "MantidKernel/VMD.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::coord_t;


class BinMDTest : public CxxTest::TestSuite
{

private:

  //Helper class. Mock Implicit function.
  class MockImplicitFunction : public Mantid::Geometry::MDImplicitFunction
  {
  public:
    using MDImplicitFunction::isPointContained; // Avoids Intel compiler warning.
    virtual bool isPointContained(const Mantid::coord_t *)
    {
      return false;
    }
    virtual std::string getName() const
    {
      return "MockImplicitFunction";
    }
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunction()   {;}
  };

  //Helper class. Builds mock implicit functions.
  class MockImplicitFunctionBuilder : public Mantid::API::ImplicitFunctionBuilder
  {
  public:
    Mantid::Geometry::MDImplicitFunction* create() const
    {
      return new MockImplicitFunction;
    }
  };

  //Helper class. Parses mock Implicit Functions.
  class MockImplicitFunctionParser : public Mantid::API::ImplicitFunctionParser
  {
  public:
    MockImplicitFunctionParser() : Mantid::API::ImplicitFunctionParser(NULL){}
    Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* /*functionElement*/)
    {
      return new MockImplicitFunctionBuilder;
    }
    void setSuccessorParser(Mantid::API::ImplicitFunctionParser* /*successor*/){}
    void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* /*parser*/){}
  };


public:

  void testSetup()
  {
    using namespace Mantid::Kernel;
    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunction> >("MockImplicitFunction"); 
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParser >("MockImplicitFunctionParser"); 
  }

  void test_Init()
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  

  /** Test the algo
  * @param nameX : name of the axis
  * @param expected_signal :: how many events in each resulting bin
  * @param expected_numBins :: how many points/bins in the output
  */
  void do_test_exec(std::string functionXML,
      std::string name1, std::string name2, std::string name3, std::string name4,
      double expected_signal,
      size_t expected_numBins,
      bool IterateEvents=false,
      size_t numEventsPerBox=1,
      VMD expectBasisX = VMD(1,0,0), VMD expectBasisY = VMD(0,1,0), VMD expectBasisZ = VMD(0,0,1))
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, numEventsPerBox);
    in_ws->addExperimentInfo(ExperimentInfo_sptr(new ExperimentInfo));
    AnalysisDataService::Instance().addOrReplace("BinMDTest_ws", in_ws);
    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000*numEventsPerBox);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimX", name1));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimY", name2));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimZ", name3));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimT", name4));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ImplicitFunctionXML",functionXML));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IterateEvents", IterateEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinMDTest_ws"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

    TS_ASSERT( alg.isExecuted() );

    MDHistoWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MDHistoWorkspace>(
        AnalysisDataService::Instance().retrieve("BinMDTest_ws")); )
    TS_ASSERT(out);
    if(!out) return;

    // Took 6x6x6 bins in the middle of the box
    TS_ASSERT_EQUALS(out->getNPoints(), expected_numBins);
    // Every box has a single event summed into it, so 1.0 weight
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      if (functionXML == "")
      {
        // Nothing rejected
        TS_ASSERT_DELTA(out->getSignalAt(i), expected_signal, 1e-5);
        TS_ASSERT_DELTA(out->getErrorAt(i), sqrt(expected_signal), 1e-5);
      }
      else
      {
        // All NAN cause of implicit function
        TS_ASSERT( boost::math::isnan( out->getSignalAt(i) ) ); //The implicit function should have ensured that no bins were present.
      }
    }
    // check basis vectors
    TS_ASSERT_EQUALS( out->getBasisVector(0), expectBasisX);
    if (out->getNumDims() > 1) { TS_ASSERT_EQUALS( out->getBasisVector(1), expectBasisY); }
    if (out->getNumDims() > 2) { TS_ASSERT_EQUALS( out->getBasisVector(2), expectBasisZ); }
    const CoordTransform * ctFrom = out->getTransformFromOriginal();
    TS_ASSERT(ctFrom);
    // Experiment Infos were copied
    TS_ASSERT_EQUALS( out->getNumExperimentInfo(), in_ws->getNumExperimentInfo());

    AnalysisDataService::Instance().remove("BinMDTest_ws");
  }

  void test_exec_3D()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_3D_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_3D_scrambled_order()
  { do_test_exec("", "Axis1,2.0,8.0, 6", "Axis0,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, false /*IterateEvents*/, 1,
      VMD(0,1,0), VMD(1,0,0), VMD(0,0,1));
  }

  void test_exec_3D_scrambled_order_IterateEvents()
  { do_test_exec("", "Axis1,2.0,8.0, 6", "Axis0,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ , 1,
      VMD(0,1,0), VMD(1,0,0), VMD(0,0,1));
  }

  void test_exec_3D_unevenSizes()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 6", "", 2.0 /*signal*/, 6*6*3 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_3D_unevenSizes_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 6", "", 2.0 /*signal*/, 6*6*3 /*# of bins*/, true /*IterateEvents*/ );
  }


  void test_exec_2D()
  { // Integrate over the 3rd dimension
    do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "", "", 1.0*10.0 /*signal*/, 6*6 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_2D_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "", "", 1.0*10.0 /*signal*/, 6*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_2D_largeBins()
  {
    do_test_exec("", "Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "", "", 4.0*10.0 /*signal*/, 3*3 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_2D_largeBins_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "", "", 4.0*10.0 /*signal*/, 3*3 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_2D_scrambledAndUnevent()
  { do_test_exec("", "Axis0,2.0,8.0, 3", "Axis2,2.0,8.0, 6", "", "", 2.0*10.0 /*signal*/, 3*6 /*# of bins*/, false /*IterateEvents*/, 1,
      VMD(1,0,0), VMD(0,0,1));
  }

  void test_exec_2D_scrambledAndUnevent_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 3", "Axis2,2.0,8.0, 6", "", "", 2.0*10.0 /*signal*/, 3*6 /*# of bins*/, true /*IterateEvents*/ , 1,
      VMD(1,0,0), VMD(0,0,1));
  }

  void test_exec_1D()
  { // Integrate over 2 dimensions
    do_test_exec("", "Axis2,2.0,8.0, 6", "", "", "", 1.0*100.0 /*signal*/, 6 /*# of bins*/, false /*IterateEvents*/, 1,
        VMD(0,0,1) );
  }

  void test_exec_1D_IterateEvents()
  { do_test_exec("", "Axis2,2.0,8.0, 6", "", "", "", 1.0*100.0 /*signal*/, 6 /*# of bins*/, true /*IterateEvents*/, 1,
      VMD(0,0,1)  );
  }

  void test_exec_1D_IterateEvents_boxCompletelyContained()
  { do_test_exec("", "Axis2,2.0,8.0, 1", "", "", "", 20*6.0*100.0 /*signal*/, 1 /*# of bins*/, true /*IterateEvents*/, 20 /*numEventsPerBox*/,
      VMD(0,0,1) );
  }



  void test_exec_with_impfunction()
  {
    //This describes the local implicit function that will always reject bins. so output workspace should have zero.
    std::string functionXML = std::string("<Function>")+
        "<Type>MockImplicitFunction</Type>"+
        "<ParameterList>"+
        "</ParameterList>"+
        "</Function>";
    do_test_exec(functionXML,  "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, false /*IterateEvents*/ );
  }
  void test_exec_with_impfunction_IterateEvents()
  { //This describes the local implicit function that will always reject bins. so output workspace should have zero.
    std::string functionXML = std::string("<Function>")+
        "<Type>MockImplicitFunction</Type>"+
        "<ParameterList>"+
        "</ParameterList>"+
        "</Function>";
    do_test_exec(functionXML,  "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ );
  }










  /** Test the algorithm, with a coordinate transformation.
   *
  * @param binsX : # of bins in the output
  * @param expected_signal :: how many events in each resulting bin
  * @param expected_numBins :: how many points/bins in the output
  * @param FlipYBasis :: flip the Y basis vector
  */
  void do_test_transform(int binsX, int binsY, int binsZ,
      double expected_signal,
      size_t expected_numBins,
      bool IterateEvents,
      bool ForceOrthogonal,
      bool FlipYBasis = false)
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    // Make a workspace with events along a regular grid that is rotated and offset along x,y
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 0);
    in_ws->splitBox();
    double theta = 0.1;
    VMD origin(-2.0, -3.0, -4.0);
    for (coord_t ox=0.5; ox<10; ox++)
      for (coord_t oy=0.5; oy<10; oy++)
        for (coord_t oz=0.5; oz<10; oz++)
        {
          coord_t x = ox*cos(theta) - oy*sin(theta) + origin[0];
          coord_t y = oy*cos(theta) + ox*sin(theta) + origin[1];
          coord_t z = oz + origin[2];
          coord_t center[3] = {x,y,z};
          MDLeanEvent<3> ev(1.0, 1.0, center);
//          std::cout << x << "," << y << "," << z << std::endl;
          in_ws->addEvent(ev);
        }
    in_ws->refreshCache();


    // Build the transformation (from eventWS to binned workspace)
    CoordTransformAffine ct(3,3);

    // Build the basis vectors, a 0.1 rad rotation along +Z
    double angle = 0.1;
    VMD baseX(cos(angle), sin(angle), 0.0);
    VMD baseY(-sin(angle), cos(angle), 0.0);
    if (FlipYBasis)
    {
      baseY = baseY * -1.;
      // Adjust origin to be at the upper left corner of the square
      origin = origin + VMD(-sin(angle), cos(angle), 0) * 10.0;
    }
    VMD baseZ(0.0, 0.0, 1.0);
    // Make a bad (i.e. non-orthogonal) input, to get it fixed.
    if (ForceOrthogonal)
    {
      baseY = VMD(0.0, 1.0, 0);
      baseZ = VMD(0.5, 0.5, 0.5);
    }

    AnalysisDataService::Instance().addOrReplace("BinMDTest_ws", in_ws);
    if (false)
    {
      // Save to NXS file for testing
      FrameworkManager::Instance().exec("SaveMD", 4,
          "InputWorkspace", "BinMDTest_ws",
          "Filename", "BinMDTest_ws_rotated.nxs");
    }

    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AxisAligned", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVectorX", "OutX,m," + baseX.toString(",") + ",10," + Strings::toString(binsX) ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVectorY", "OutY,m," + baseY.toString(",") + ",10," + Strings::toString(binsY) ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVectorZ", "OutZ,m," + baseZ.toString(",") + ",10," + Strings::toString(binsZ) ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVectorT", ""));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Origin", origin.toString(",") ));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ForceOrthogonal", ForceOrthogonal ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ImplicitFunctionXML",""));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IterateEvents", IterateEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinMDTest_ws"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

    TS_ASSERT( alg.isExecuted() );

    MDHistoWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<MDHistoWorkspace>(
        AnalysisDataService::Instance().retrieve("BinMDTest_ws")); )
    TS_ASSERT(out);
    if(!out) return;

    // Took 6x6x6 bins in the middle of the box
    TS_ASSERT_EQUALS(out->getNPoints(), expected_numBins);
    // Every box has a single event summed into it, so 1.0 weight
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      // Nothing rejected
      TS_ASSERT_DELTA(out->getSignalAt(i), expected_signal, 1e-5);
      TS_ASSERT_DELTA(out->getErrorAt(i), std::sqrt(expected_signal), 1e-5);
    }

    // check basis vectors
    TS_ASSERT_EQUALS( out->getBasisVector(0), baseX);
    if (!ForceOrthogonal)
    { TS_ASSERT_EQUALS( out->getBasisVector(1), baseY);
      TS_ASSERT_EQUALS( out->getBasisVector(2), baseZ); }

    const CoordTransform * ctFrom = out->getTransformFromOriginal();
    TS_ASSERT(ctFrom);
    const CoordTransform * ctTo = out->getTransformToOriginal();
    TS_ASSERT(ctTo);
    if (!ctTo) return;

    // Round-trip transform
    coord_t originalPoint[3] = {1.0, 2.0, 3.0};
    coord_t * transformedPoint = new coord_t[3];
    coord_t * originalBack = new coord_t[3];
    ctFrom->apply(originalPoint, transformedPoint);
    ctTo->apply(transformedPoint, originalBack);
    for (size_t d=0; d<3; d++)
    { TS_ASSERT_DELTA( originalPoint[d], originalBack[d], 1e-5); }

    AnalysisDataService::Instance().remove("BinMDTest_ws");
  }


  void test_exec_with_transform()
  {
    do_test_transform(10, 10, 10,
        1.0 /*signal*/, 1000 /*# of bins*/, true /*IterateEvents*/,
        false /* Dont force orthogonal */);
  }

  void test_exec_with_transform_unevenSizes()
  {
    do_test_transform(5, 10, 2,
        10*1.0 /*signal*/, 100 /*# of bins*/, true /*IterateEvents*/,
        false /* Dont force orthogonal */ );
  }

  void test_exec_with_transform_ForceOrthogonal()
  {
    do_test_transform(5, 10, 2,
        10*1.0 /*signal*/, 100 /*# of bins*/, true /*IterateEvents*/,
        true /* Do force orthogonal */ );
  }

  /** Change the handedness of the basis vectors by flipping the Y vector */
  void test_exec_with_transform_flipping_Y_basis()
  {
    do_test_transform(10, 10, 10,
        1.0 /*signal*/, 1000 /*# of bins*/, true /*IterateEvents*/,
        false /* Dont force orthogonal */,
        true /* Flip sign of Y basis vector*/);
  }



  //---------------------------------------------------------------------------------------------
  /** Check that two MDHistos have the same values .
   *
   * @param binned1Name :: original, binned direct from MDEW
   * @param binned2Name :: binned from a MDHisto
   * @param origWS :: both should have this as its originalWorkspace
   */
  void do_compare_histo(std::string binned1Name, std::string binned2Name, std::string origWS)
  {
    MDHistoWorkspace_sptr binned1 = boost::dynamic_pointer_cast<MDHistoWorkspace>(AnalysisDataService::Instance().retrieve(binned1Name));
    MDHistoWorkspace_sptr binned2 = boost::dynamic_pointer_cast<MDHistoWorkspace>(AnalysisDataService::Instance().retrieve(binned2Name));
    TS_ASSERT_EQUALS( binned1->getOriginalWorkspace()->getName(), origWS);
    TS_ASSERT_EQUALS( binned2->getOriginalWorkspace()->getName(), origWS);
    TS_ASSERT(binned2);
    size_t numErrors = 0;
    for (size_t i=0; i < binned1->getNPoints(); i++)
    {
      double diff = std::abs(binned1->getSignalAt(i) - binned2->getSignalAt(i));
      if (diff > 1e-6) numErrors++;
      TS_ASSERT_DELTA( binned1->getSignalAt(i), binned2->getSignalAt(i), 1e-5);
    }
    TS_ASSERT_EQUALS( numErrors, 0);
  }

  /** Common setup for double-binning tests */
  void do_prepare_comparison()
  {
    AnalysisDataService::Instance().remove("mdew");
    AnalysisDataService::Instance().remove("binned0");
    AnalysisDataService::Instance().remove("binned1");
    AnalysisDataService::Instance().remove("binned2");

    // ---- Start with empty MDEW ----
    FrameworkManager::Instance().exec("CreateMDWorkspace", 16,
        "Dimensions", "2",
        "Extents", "-10,10,-10,10",
        "Names", "x,y",
        "Units", "m,m",
        "SplitInto", "4",
        "SplitThreshold", "100",
        "MaxRecursionDepth", "20",
        "OutputWorkspace", "mdew");

    // Give fake uniform data
    FrameworkManager::Instance().exec("FakeMDEventData", 6,
        "InputWorkspace", "mdew",
        "UniformParams", "1000",
        "RandomSeed", "1234");
  }


  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, axis-aligned */
  void test_exec_Aligned_then_nonAligned()
  {
    do_prepare_comparison();
    // Bin aligned to original. Coordinates remain the same
    FrameworkManager::Instance().exec("BinMD", 10,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "1",
        "AlignedDimX", "x, -10, 10, 10",
        "AlignedDimY", "y, -10, 10, 10");

    // Bin, non-axis-aligned, with translation
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "binned0",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVectorX", "rx,m, 1.0, 0.0, 20.0, 10",
        "BasisVectorY", "ry,m, 0.0, 1.0, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "-10, -10");

    do_compare_histo("binned0", "binned1", "mdew");
  }


  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, not axis-aligned */
  void test_exec_nonAligned_then_nonAligned_rotation()
  {
    do_prepare_comparison();

    // Bin NOT aligned to original, with translation
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "0",
        "BasisVectorX", "rx,m, 1.0, 0.0, 20.0, 10",
        "BasisVectorY", "ry,m, 0.0, 1.0, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "-10, -10");

    // Bin with some rotation (10 degrees)
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVectorX", "rx,m, 0.98, 0.17, 20.0, 10",
        "BasisVectorY", "ry,m, -.17, 0.98, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "-10, -10");
    // Bin the binned output with the opposite rotation
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "binned1",
        "OutputWorkspace", "binned2",
        "AxisAligned", "0",
        "BasisVectorX", "rrx,m, 0.98, -.17, 20.0, 10",
        "BasisVectorY", "rry,m, 0.17, 0.98, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "0, 0");
    // Check they are the same
    do_compare_histo("binned0", "binned2", "mdew");
  }


  //---------------------------------------------------------------------------------------------
  /** Bin a MDHistoWorkspace that was itself binned from a MDEW, not axis-aligned, with translation */
  void test_exec_nonAligned_then_nonAligned_translation()
  {
    do_prepare_comparison();

    // Bin aligned to original
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "0",
        "BasisVectorX", "rx,m, 1.0, 0.0, 20.0, 10",
        "BasisVectorY", "ry,m, 0.0, 1.0, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "-10, -10");

    // Bin with a translation. -10,-10 in MDEW becomes 0,0 in binned1
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned1",
        "AxisAligned", "0",
        "BasisVectorX", "rx,m, 1.0, 0.0, 20.0, 10",
        "BasisVectorY", "ry,m, 0.0, 1.0, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "-10, -10");

    // Bin the binned output with the opposite translation
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "binned1",
        "OutputWorkspace", "binned2",
        "AxisAligned", "0",
        "BasisVectorX", "rrx,m, 1.0, 0.0, 20.0, 10",
        "BasisVectorY", "rry,m, 0.0, 1.0, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "0, 0");

    // Check they are the same
    do_compare_histo("binned0", "binned2", "mdew");
  }

  //---------------------------------------------------------------------------------------------
  /** Can't do Axis-Aligned on a MDHisto because the transformation gets too annoying. */
  void test_exec_Aligned_onMDHisto_fails()
  {
    do_prepare_comparison();

    // Bin NOT aligned to original, translated. Coordinates change.
    FrameworkManager::Instance().exec("BinMD", 14,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned0",
        "AxisAligned", "0",
        "BasisVectorX", "rx,m, 1.0, 0.0, 20.0, 10",
        "BasisVectorY", "ry,m, 0.0, 1.0, 20.0, 10",
        "ForceOrthogonal", "1",
        "Origin", "-10, -10");

    // Bin aligned to binned0. This is not allowed!
    IAlgorithm_sptr alg = FrameworkManager::Instance().exec("BinMD", 10,
        "InputWorkspace", "binned0",
        "OutputWorkspace", "binned1",
        "AxisAligned", "1",
        "AlignedDimX", "rx, 0, 20, 10",
        "AlignedDimY", "ry, 0, 20, 10");
    TS_ASSERT( !alg->isExecuted() );
  }


};


class BinMDTestPerformance : public CxxTest::TestSuite
{
public:
  MDEventWorkspace3Lean::sptr in_ws;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinMDTestPerformance *createSuite() { return new BinMDTestPerformance(); }
  static void destroySuite( BinMDTestPerformance *suite ) { delete suite; }

  BinMDTestPerformance()
  {
    in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    in_ws->getBoxController()->setSplitThreshold(2000);
    in_ws->splitAllIfNeeded(NULL);
    AnalysisDataService::Instance().addOrReplace("BinMDTest_ws", in_ws);
    FrameworkManager::Instance().exec("FakeMDEventData", 4,
        "InputWorkspace", "BinMDTest_ws",
        "UniformParams", "1000000");
    // 1 million random points
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000*1000);
    TS_ASSERT_EQUALS( in_ws->getBoxController()->getMaxId(), 1001 );
  }

  ~BinMDTestPerformance()
  {
    AnalysisDataService::Instance().remove("BinMDTest_ws");
  }

  void do_test(std::string binParams, bool IterateEvents)
  {
    BinMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimX", "Axis0," + binParams));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimY", "Axis1," + binParams));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimZ", "Axis2," + binParams));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDimT", ""));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IterateEvents", IterateEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinMDTest_ws_histo"));
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );
  }


  void test_3D_60cube()
  {
    for (size_t i=0; i<1; i++)
      do_test("2.0,8.0, 60", false);
  }

  void test_3D_tinyRegion_60cube()
  {
    for (size_t i=0; i<1; i++)
      do_test("5.3,5.4, 60", false);
  }

  void test_3D_60cube_IterateEvents()
  {
    for (size_t i=0; i<1; i++)
      do_test("2.0,8.0, 60", true);
  }

  void test_3D_tinyRegion_60cube_IterateEvents()
  {
    for (size_t i=0; i<1; i++)
      do_test("5.3,5.4, 60", true);
  }

  void test_3D_1cube_IterateEvents()
  {
    for (size_t i=0; i<1; i++)
      do_test("2.0,8.0, 1", true);
  }

};


#endif /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_ */

