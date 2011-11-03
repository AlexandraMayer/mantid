#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VMD.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

class MDHistoWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDHistoWorkspaceTest *createSuite() { return new MDHistoWorkspaceTest(); }
  static void destroySuite( MDHistoWorkspaceTest *suite ) { delete suite; }

  MDHistoWorkspace_sptr two;
  MDHistoWorkspace_sptr three;

  /** Create some fake workspace, 5x5, with 2.0 and 3.0 each */
  MDHistoWorkspaceTest()
  {
    two = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.0);
    three = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 3.0);
  }

  /** Check that a workspace has the right signal/error*/
  void checkWorkspace(MDHistoWorkspace_sptr ws, double expectedSignal, double expectedErrorSquared)
  {
    for (size_t i=0; i < ws->getNPoints(); i++)
    {
      TS_ASSERT_DELTA( ws->getSignalAt(i), expectedSignal, 1e-5 );
      TS_ASSERT_DELTA( ws->getErrorAt(i), expectedErrorSquared, 1e-5 );
    }
  }


  //--------------------------------------------------------------------------------------
  void test_constructor()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 5));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 5));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    TS_ASSERT_EQUALS( ws.getNumDims(), 4);
    TS_ASSERT_EQUALS( ws.getNPoints(), 5*5*5*5);
    TS_ASSERT_EQUALS( ws.getMemorySize(), 5*5*5*5 * sizeof(double)*2);
    TS_ASSERT_EQUALS( ws.getXDimension(), dimX);
    TS_ASSERT_EQUALS( ws.getYDimension(), dimY);
    TS_ASSERT_EQUALS( ws.getZDimension(), dimZ);
    TS_ASSERT_EQUALS( ws.getTDimension(), dimT);

    // The values are cleared at the start
    for (size_t i=0; i <  ws.getNPoints(); i++)
    {
      TS_ASSERT( boost::math::isnan( ws.getSignalAt(i) ));
      TS_ASSERT( boost::math::isnan( ws.getErrorAt(i) ));
      TS_ASSERT( boost::math::isnan( ws.getSignalNormalizedAt(i) ));
      TS_ASSERT( boost::math::isnan( ws.getErrorNormalizedAt(i) ));
    }

    // Setting and getting
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);
    TS_ASSERT_DELTA( ws.getSignalNormalizedAt(5), 2.3456 / 256.0, 1e-5); // Cell volume is 256

    ws.setErrorAt(5,1.234);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);
    TS_ASSERT_DELTA( ws.getErrorNormalizedAt(5), 1.234 / 256.0, 1e-5); // Cell volume is 256

    std::vector<signal_t> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 5*5*5*5);
    TS_ASSERT_DELTA( data[5], 2.3456, 1e-5);

    // Set a different value at every point
    for (size_t i=0; i <  ws.getNPoints(); ++i)
    {
      ws.setSignalAt(i, (signal_t) i);
      ws.setErrorAt(i, (signal_t) i);
    }

    // Test the 4 overloads of each method. Phew!
    TS_ASSERT_DELTA( ws.getSignalAt(1), 1.0, 1e-4);
    TS_ASSERT_DELTA( ws.getSignalAt(1,2), 1.0+2*5.0, 1e-4);
    TS_ASSERT_DELTA( ws.getSignalAt(1,2,3), 1.0+2*5.0+3*25.0, 1e-4);
    TS_ASSERT_DELTA( ws.getSignalAt(1,2,3,4), 1.0+2*5.0+3*25.0+4*125.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorAt(1), 1.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorAt(1,2), 1.0+2*5.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorAt(1,2,3), 1.0+2*5.0+3*25.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorAt(1,2,3,4), 1.0+2*5.0+3*25.0+4*125.0, 1e-4);
    TS_ASSERT_DELTA( ws.getSignalNormalizedAt(1)*256.0, 1.0, 1e-4);
    TS_ASSERT_DELTA( ws.getSignalNormalizedAt(1,2)*256.0, 1.0+2*5.0, 1e-4);
    TS_ASSERT_DELTA( ws.getSignalNormalizedAt(1,2,3)*256.0, 1.0+2*5.0+3*25.0, 1e-4);
    TS_ASSERT_DELTA( ws.getSignalNormalizedAt(1,2,3,4)*256.0, 1.0+2*5.0+3*25.0+4*125.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorNormalizedAt(1)*256.0, 1.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorNormalizedAt(1,2)*256.0, 1.0+2*5.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorNormalizedAt(1,2,3)*256.0, 1.0+2*5.0+3*25.0, 1e-4);
    TS_ASSERT_DELTA( ws.getErrorNormalizedAt(1,2,3,4)*256.0, 1.0+2*5.0+3*25.0+4*125.0, 1e-4);
  }


  //---------------------------------------------------------------------------------------------------
  /** Create a dense histogram with only 2 dimensions */
  void test_constructor_fewerDimensions()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY);

    TS_ASSERT_EQUALS( ws.getNumDims(), 2);
    TS_ASSERT_EQUALS( ws.getNPoints(), 5*5);
    TS_ASSERT_EQUALS( ws.getMemorySize(), 5*5 * sizeof(double)*2);
    TS_ASSERT_EQUALS( ws.getXDimension(), dimX);
    TS_ASSERT_EQUALS( ws.getYDimension(), dimY);
    TS_ASSERT_THROWS_ANYTHING( ws.getZDimension());
    TS_ASSERT_THROWS_ANYTHING( ws.getTDimension());

    // Setting and getting
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);

    ws.setErrorAt(5,1.234);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);

    std::vector<signal_t> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 5*5);
    TS_ASSERT_DELTA( data[5], 2.3456, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Create a dense histogram with 7 dimensions */
  void test_constructor_MoreThanFourDimensions()
  {
    std::vector<MDHistoDimension_sptr> dimensions;
    for (size_t i=0; i<7; i++)
    {
      dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension("Dim", "Dim", "m", -10, 10, 3)));
    }

    MDHistoWorkspace ws(dimensions);

    TS_ASSERT_EQUALS( ws.getNumDims(), 7);
    TS_ASSERT_EQUALS( ws.getNPoints(), 3*3*3*3*3*3*3);
    TS_ASSERT_EQUALS( ws.getMemorySize(), ws.getNPoints() * sizeof(double)*2);

    // Setting and getting
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);

    ws.setErrorAt(5,1.234);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);

    std::vector<signal_t> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 3*3*3*3*3*3*3);
    TS_ASSERT_DELTA( data[5], 2.3456, 1e-5);
  }


  //---------------------------------------------------------------------------------------------------
  void test_getVertexesArray_1D()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoWorkspace ws(dimX);
    coord_t * v;
    size_t numVertices;
    v = ws.getVertexesArray(0,numVertices);
    TS_ASSERT_EQUALS( numVertices, 2 );
    TS_ASSERT_DELTA( v[0], -10.0, 1e-5);
    TS_ASSERT_DELTA( v[1], -6.0, 1e-5);

    v = ws.getVertexesArray(4,numVertices);
    TS_ASSERT_DELTA( v[0], 6.0, 1e-5);
    TS_ASSERT_DELTA( v[1], 10.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getVertexesArray_2D()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 5));
    MDHistoWorkspace ws(dimX, dimY);
    coord_t * v;
    size_t numVertices, i;

    v = ws.getVertexesArray(0,numVertices);
    TS_ASSERT_EQUALS( numVertices, 4 );
    i = 0*2;
    TS_ASSERT_DELTA( v[i+0], -10.0, 1e-5);
    TS_ASSERT_DELTA( v[i+1], -10.0, 1e-5);
    i = 3*2;
    TS_ASSERT_DELTA( v[i+0], -6.0, 1e-5);
    TS_ASSERT_DELTA( v[i+1], -6.0, 1e-5);
    // The opposite corner
    v = ws.getVertexesArray(24,numVertices);
    i = 0*2;
    TS_ASSERT_DELTA( v[i+0], 6.0, 1e-5);
    TS_ASSERT_DELTA( v[i+1], 6.0, 1e-5);
    i = 3*2;
    TS_ASSERT_DELTA( v[i+0], 10.0, 1e-5);
    TS_ASSERT_DELTA( v[i+1], 10.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getVertexesArray_3D()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -9, 10, 5));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -8, 10, 5));
    MDHistoWorkspace ws(dimX, dimY, dimZ);
    coord_t * v;
    size_t numVertices, i;

    v = ws.getVertexesArray(0,numVertices);
    TS_ASSERT_EQUALS( numVertices, 8 );
    i = 0;
    TS_ASSERT_DELTA( v[i+0], -10.0, 1e-5);
    TS_ASSERT_DELTA( v[i+1], -9.0, 1e-5);
    TS_ASSERT_DELTA( v[i+2], -8.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getCenter_3D()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 20));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -9, 10, 19));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -8, 10, 18));
    MDHistoWorkspace ws(dimX, dimY, dimZ);
    VMD v = ws.getCenter(0);
    TS_ASSERT_DELTA( v[0], -9.5, 1e-5);
    TS_ASSERT_DELTA( v[1], -8.5, 1e-5);
    TS_ASSERT_DELTA( v[2], -7.5, 1e-5);
  }


  //---------------------------------------------------------------------------------------------------
  /** Test for a possible seg-fault if nx != ny etc. */
  void test_uneven_numbers_of_bins()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 10));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 20));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 10));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    TS_ASSERT_EQUALS( ws.getNumDims(), 4);
    TS_ASSERT_EQUALS( ws.getNPoints(), 5*10*20*10);
    TS_ASSERT_EQUALS( ws.getMemorySize(), 5*10*20*10 * sizeof(double)*2);

    // Setting and getting
    size_t index = 5*10*20*10-1; // The last point
    ws.setSignalAt(index,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(index), 2.3456, 1e-5);

    // Getter with all indices
    TS_ASSERT_DELTA( ws.getSignalAt(4,9,19,9), 2.3456, 1e-5);

  }

  //---------------------------------------------------------------------------------------------------
  void test_createIterator()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 10));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -9, 10, 10));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -8, 10, 10));
    MDHistoWorkspace ws(dimX, dimY, dimZ);
    IMDIterator * it = ws.createIterator();
    TS_ASSERT( it );
    MDHistoWorkspaceIterator * hwit = dynamic_cast<MDHistoWorkspaceIterator *>(it);
    TS_ASSERT( hwit );
    TS_ASSERT( it->next() );
    it = ws.createIterator(new MDImplicitFunction() );
    TS_ASSERT( it );
  }

  //---------------------------------------------------------------------------------------------------
  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetNonIntegratedDimensions()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 1)); //Integrated.
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 10));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 20));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 10));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);
    Mantid::Geometry::VecIMDDimension_const_sptr vecNonIntegratedDims = ws.getNonIntegratedDimensions();
    TSM_ASSERT_EQUALS("Only 3 of the 4 dimensions should be non-integrated", 3, vecNonIntegratedDims.size());
    TSM_ASSERT_EQUALS("First non-integrated dimension should be Y", "y", vecNonIntegratedDims[0]->getDimensionId());
    TSM_ASSERT_EQUALS("Second non-integrated dimension should be Z", "z", vecNonIntegratedDims[1]->getDimensionId());
    TSM_ASSERT_EQUALS("Third non-integrated dimension should be T", "t", vecNonIntegratedDims[2]->getDimensionId());
  }


  //---------------------------------------------------------------------------------------------------
  void test_getGeometryXML()
  {
    //If POCO xml supported schema validation, we wouldn't need to check xml outputs like this.
    std::string expectedXML = std::string("<DimensionSet>") +
      "<Dimension ID=\"x\">" +
      "<Name>X</Name>" + 
      "<Units>m</Units>" +
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<Dimension ID=\"y\">" +
      "<Name>Y</Name>" + 
      "<Units>m</Units>" +
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<Dimension ID=\"z\">" +
      "<Name>Z</Name>" + 
      "<Units>m</Units>" +
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<Dimension ID=\"t\">" +
      "<Name>T</Name>" + 
      "<Units>m</Units>" +
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<XDimension>" +
      "<RefDimensionId>x</RefDimensionId>" +
      "</XDimension>" +
      "<YDimension>" +
      "<RefDimensionId>y</RefDimensionId>" + 
      "</YDimension>" +
      "<ZDimension>" +
      "<RefDimensionId>z</RefDimensionId>" + 
      "</ZDimension>" +
      "<TDimension>" +
      "<RefDimensionId>t</RefDimensionId>" + 
      "</TDimension>" +
      "</DimensionSet>";

    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 5));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 5));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    std::string actualXML = ws.getGeometryXML();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }


  //---------------------------------------------------------------------------------------------------
  void test_getSignalAtCoord()
  {
    // 2D workspace with signal[i] = i (linear index)
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i=0; i<100; i++)
      ws->setSignalAt(i, double(i));
    IMDWorkspace_sptr iws(ws);
    TS_ASSERT_DELTA( iws->getSignalAtCoord(VMD(0.5, 0.5)), 0.0, 1e-6);
    TS_ASSERT_DELTA( iws->getSignalAtCoord(VMD(1.5, 0.5)), 1.0, 1e-6);
    TS_ASSERT_DELTA( iws->getSignalAtCoord(VMD(1.5, 1.5)), 11.0, 1e-6);
    TS_ASSERT_DELTA( iws->getSignalAtCoord(VMD(9.5, 9.5)), 99.0, 1e-6);
    // Out of range = NaN
    TS_ASSERT( boost::math::isnan(iws->getSignalAtCoord(VMD(-0.01, 2.5)) ) );
    TS_ASSERT( boost::math::isnan(iws->getSignalAtCoord(VMD(3.5, -0.02)) ) );
    TS_ASSERT( boost::math::isnan(iws->getSignalAtCoord(VMD(10.01, 2.5)) ) );
    TS_ASSERT( boost::math::isnan(iws->getSignalAtCoord(VMD(3.5, 10.02)) ) );
  }

  //--------------------------------------------------------------------------------------
  void test_plus_ws()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 3.5 /*errorSquared*/);
    *a += *b;
    checkWorkspace(a, 5.0, 6.0);
  }

  void test_plus_scalar()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    WorkspaceSingleValue b(3.0, sqrt(3.5) );
    *a += b;
    checkWorkspace(a, 5.0, 6.0);
  }

  //--------------------------------------------------------------------------------------
  void test_minus_ws()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 3.5 /*errorSquared*/);
    *a -= *b;
    checkWorkspace(a, 1.0, 6.0);
  }

  void test_minus_scalar()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    WorkspaceSingleValue b(2.0, sqrt(3.5) );
    *a -= b;
    checkWorkspace(a, 1.0, 6.0);
  }

  //--------------------------------------------------------------------------------------
  void test_times_ws()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.0 /*errorSquared*/);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 3.0 /*errorSquared*/);
    *a *= *b;
    checkWorkspace(a, 6.0, 36. * (.5 + 1./3.));
  }

  //--------------------------------------------------------------------------------------
  void test_times_scalar()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.0 /*errorSquared*/);
    WorkspaceSingleValue b(3.0, sqrt(3.0) );
    *a *= b;
    checkWorkspace(a, 6.0, 36. * (.5 + 1./3.));
    // Scalar without error
    MDHistoWorkspace_sptr d = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.0 /*errorSquared*/);
    WorkspaceSingleValue e(3.0, 0);
    *d *= e;
    checkWorkspace(d, 6.0, 9 * 2.0);
  }

  //--------------------------------------------------------------------------------------
  void test_divide_ws()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 3.0 /*errorSquared*/);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.0 /*errorSquared*/);
    *a /= *b;
    checkWorkspace(a, 1.5, 1.5 * 1.5 * (.5 + 1./3.));
  }

  //--------------------------------------------------------------------------------------
  void test_divide_scalar()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 3.0 /*errorSquared*/);
    WorkspaceSingleValue b(2.0, sqrt(2.0) );
    *a /= b;
    checkWorkspace(a, 1.5, 1.5 * 1.5 * (.5 + 1./3.));
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_and()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    MDHistoWorkspace_sptr c = MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    *a &= *b;
    checkWorkspace(a, 1.0, 0.0);
    *b &= *c;
    checkWorkspace(b, 0.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_or()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    MDHistoWorkspace_sptr c = MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    *a |= *b;
    checkWorkspace(a, 1.0, 0.0);
    *b |= *c;
    checkWorkspace(b, 1.0, 0.0);
    *c |= *c;
    checkWorkspace(c, 0.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_xor()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    MDHistoWorkspace_sptr c = MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    *a ^= *b;
    checkWorkspace(a, 0.0, 0.0);
    *b ^= *c;
    checkWorkspace(b, 1.0, 0.0);
    *c ^= *c;
    checkWorkspace(c, 0.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_operatorNot()
  {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    a->operatorNot();
    checkWorkspace(a, 0.0, 0.0);
    b->operatorNot();
    checkWorkspace(b, 1.0, 0.0);
  }


};


#endif /* MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_ */

