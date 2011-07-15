#ifndef MANTID_ALGORITHMS_REBIN2DTEST_H_
#define MANTID_ALGORITHMS_REBIN2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidKernel/Timer.h"

using Mantid::Algorithms::Rebin2D;
using namespace Mantid::API;

namespace
{
  /**
   * Shared methods between the unit test and performance test
   */

  /// Return the input workspace. All Y values are 2 and E values sqrt(2)
  MatrixWorkspace_sptr makeInputWS(const bool distribution, const bool perf_test = false, const bool small_bins = false)
  {
    size_t nhist(0), nbins(0);
    double x0(0.0), deltax(0.0);

    if( perf_test )
    {
      nhist = 500;
      nbins = 400;
      x0 = 100.;
      deltax = 100.;
    }
    else
    {
      nhist = 10;
      nbins = 10;
      x0 = 5.0;
      if( small_bins )
      {
        deltax = 0.1;
      }
      else
      {
        deltax = distribution ? 2.0 : 1.0;
      }
    }

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(int(nhist), int(nbins), x0, deltax);

    // We need something other than a spectrum axis, call this one theta
    NumericAxis* const thetaAxis = new NumericAxis(nhist);
    for(size_t i = 0; i < nhist; ++i)
    {
      thetaAxis->setValue(i, static_cast<double>(i));
    }
    ws->replaceAxis(1, thetaAxis);

    if( distribution )
    {
      Mantid::API::WorkspaceHelpers::makeDistribution(ws);
    }

    return ws;
  }

  MatrixWorkspace_sptr runAlgorithm(MatrixWorkspace_sptr inputWS,
                                    const std::string & axis1Params, 
                                    const std::string & axis2Params)
  {
    // Name of the output workspace.
    std::string outWSName("Rebin2DTest_OutputWS");
    
    Rebin2D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis1Binning", axis1Params) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis2Binning", axis2Params) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(outWSName));
    TS_ASSERT(outputWS);
    return outputWS;
  }

}

class Rebin2DTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    Rebin2D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_Rebin2D_With_Axis2_Unchanged()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS(false); //10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS = runAlgorithm(inputWS, "5.,2.,15.", "-0.5,1,9.5");

    checkData(outputWS, 6, 10, false, true);
  }

  void test_Rebin2D_With_Axis1_Unchanged()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS(false); //10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS = runAlgorithm(inputWS, "5.,1.,15.", "-0.5,2,9.5");
    checkData(outputWS, 11, 5, false, false);
  }

  void test_Rebin2D_With_Input_Distribution()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS(true); //10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS = runAlgorithm(inputWS, "5.,4.,25.", "-0.5,1,9.5");
    checkData(outputWS, 6, 10, true, true);
  }

  void test_Rebin2D_With_Bin_Width_Less_Than_One_And_Not_Distribution()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS(false, false, true); //10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS = runAlgorithm(inputWS, "5.,0.2,6.", "-0.5,1,9.5");
    checkData(outputWS, 6, 10, false, true, true);
  }
  

private:
  
  void checkData(MatrixWorkspace_const_sptr outputWS, const size_t nxvalues, const size_t nhist,
                 const bool dist, const bool onAxis1, const bool small_bins = false)
  {
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nhist);
    TS_ASSERT_EQUALS(outputWS->isDistribution(), dist);
    // Axis sizes
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->length(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->length(), nhist);
    TS_ASSERT_EQUALS(outputWS->readX(0).size(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->readY(0).size(), nxvalues - 1);

    Mantid::API::Axis *newYAxis = outputWS->getAxis(1);
    const double epsilon(1e-08);
    for(size_t i = 0; i < nhist; ++i)
    {
      for(size_t j = 0; j < nxvalues - 1; ++j)
      {
        std::ostringstream os;
        os << "Bin " << i << "," << j;
        if( onAxis1 )
        {
          if( small_bins )
          {
            
            TS_ASSERT_DELTA(outputWS->readX(i)[j], 5.0 + 0.2*static_cast<double>(j), epsilon);
          }
          else
          {
            if( dist ) 
            {
              TS_ASSERT_DELTA(outputWS->readX(i)[j], 5.0 + 4.0*static_cast<double>(j), epsilon);
            }
            else 
            {
              TS_ASSERT_DELTA(outputWS->readX(i)[j], 5.0 + 2.0*static_cast<double>(j), epsilon);
            }
          }
        }
        else
        {
          TS_ASSERT_DELTA(outputWS->readX(i)[j], 5.0 + static_cast<double>(j), epsilon);
        }
        if( dist )
        {
          TS_ASSERT_DELTA(outputWS->readY(i)[j], 1.0, epsilon);
          TS_ASSERT_DELTA(outputWS->readE(i)[j], 0.5, epsilon);
        }
        else
        {
          TSM_ASSERT_DELTA(os.str(), outputWS->readY(i)[j], 4.0, epsilon);
          TS_ASSERT_DELTA(outputWS->readE(i)[j], 2.0, epsilon);
        }

      }    
      // Final X boundary
      if( small_bins )
      {
        TS_ASSERT_DELTA(outputWS->readX(i)[nxvalues-1], 6.0, epsilon);
      }
      else
      {
        if( dist ) 
        {
          TS_ASSERT_DELTA(outputWS->readX(i)[nxvalues-1], 25.0, epsilon);        
        }
        else
        {
          TS_ASSERT_DELTA(outputWS->readX(i)[nxvalues-1], 15.0, epsilon);        
        }
      }
      if( onAxis1 )
      {
        // The new Y axis value should be the centre point bin values
        TS_ASSERT_DELTA((*newYAxis)(i), static_cast<double>(i), epsilon);
      }
      else
      {
        // The new Y axis value should be the centre point bin values
        TS_ASSERT_DELTA((*newYAxis)(i), 0.5 + 2.0*static_cast<double>(i), epsilon);
      }
    }
    // Clean up
    AnalysisDataService::Instance().remove(outputWS->getName());
  }

};


//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class Rebin2DTestPerformance : public CxxTest::TestSuite
{
  
public:
  
  void test_On_Large_Workspace()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS(false, true);
    runAlgorithm(inputWS, "100,200,41000", "-0.5,2,499.5");
  }
  

};


#endif /* MANTID_ALGORITHMS_REBIN2DTEST_H_ */

