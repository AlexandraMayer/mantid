#ifndef MINUSTEST_H_
#define MINUSTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/Minus.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class MinusTest : public CxxTest::TestSuite
{
public:

  void xtestInit()
  {
    Minus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(    alg.setPropertyValue("LHSWorkspace","test_in21") );
    TS_ASSERT_THROWS_NOTHING(    alg.setPropertyValue("RHSWorkspace","test_in22") );
    TS_ASSERT_THROWS_NOTHING(    alg.setPropertyValue("OutputWorkspace","test_out2") );
  }

  void testExec1D1D()
  {
    int sizex = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    Minus alg;

    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","test_in11");
    alg.setPropertyValue("RHSWorkspace","test_in12");
    alg.setPropertyValue("OutputWorkspace","test_out1");
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out1")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_out1");
    AnalysisDataService::Instance().remove("test_in11");
    AnalysisDataService::Instance().remove("test_in12");

  }

  void testExec1D1DRand()
  {
    int sizex = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(sizex);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    Minus alg;

    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","test_in11");
    alg.setPropertyValue("RHSWorkspace","test_in12");
    alg.setPropertyValue("OutputWorkspace","test_out1");
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out1")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_out1");
    AnalysisDataService::Instance().remove("test_in11");
    AnalysisDataService::Instance().remove("test_in12");

  }


  void testExec2D2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);

    Minus alg;

    AnalysisDataService::Instance().add("test_in21", work_in1);
    AnalysisDataService::Instance().add("test_in22", work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","test_in21");
    alg.setPropertyValue("RHSWorkspace","test_in22");
    alg.setPropertyValue("OutputWorkspace","test_out2");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out2")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_in21");
    AnalysisDataService::Instance().remove("test_in22");
    AnalysisDataService::Instance().remove("test_out2");

  }

  void testExec1D2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);

    Minus alg;
    std::string wsName_EW = "test_in1D2D21";
    std::string wsName_2D = "test_in1D2D22";
    std::string wsNameOut = "test_out1D2D";
    AnalysisDataService::Instance().add(wsName_EW, work_in1);
    AnalysisDataService::Instance().add(wsName_2D, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName_EW);
    alg.setPropertyValue("RHSWorkspace",wsName_2D);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName_EW);
    AnalysisDataService::Instance().remove(wsName_2D);
    AnalysisDataService::Instance().remove(wsNameOut);

  }

  void testExec1DRand2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(sizex);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Minus alg;

    std::string wsName_EW = "test_in1D2Dv1";
    std::string wsName_2D = "test_in1D2Dv2";
    std::string wsNameOut = "test_out1D2Dv";
    AnalysisDataService::Instance().add(wsName_EW, work_in1);
    AnalysisDataService::Instance().add(wsName_2D, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName_EW);
    alg.setPropertyValue("RHSWorkspace",wsName_2D);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName_EW);
    AnalysisDataService::Instance().remove(wsName_2D);
    AnalysisDataService::Instance().remove(wsNameOut);
  }

  void testExec2D1DVertical()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace123(1,sizey);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Minus alg;

    std::string wsName_EW = "test_in2D1Dv1";
    std::string wsName_2D = "test_in2D1Dv2";
    std::string wsNameOut = "test_out2D1Dv";
    AnalysisDataService::Instance().add(wsName_EW, work_in1);
    AnalysisDataService::Instance().add(wsName_2D, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName_EW);
    alg.setPropertyValue("RHSWorkspace",wsName_2D);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName_EW);
    AnalysisDataService::Instance().remove(wsName_2D);
    AnalysisDataService::Instance().remove(wsNameOut);
  }

  void testExec2D2DbyOperatorOverload()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    MatrixWorkspace_sptr work_out1 = work_in1 - work_in2;

    checkData(work_in1, work_in2, work_out1);
  }

    void testExec1DSingleValue()
  {
    int sizex = 10;
    // Register the workspace in the data service

    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    Minus alg;

    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","test_in11");
    alg.setPropertyValue("RHSWorkspace","test_in12");
    alg.setPropertyValue("OutputWorkspace","test_out1");
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out1")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_out1");
    AnalysisDataService::Instance().remove("test_in11");
    AnalysisDataService::Instance().remove("test_in12");

  }

  void testExec2DSingleValue()
  {
    int sizex = 5,sizey=300;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);

    Minus alg;

    std::string wsName_EW = "test_in2D1D21";
    std::string wsName_2D = "test_in2D1D22";
    std::string wsNameOut = "test_out2D1D";
    AnalysisDataService::Instance().add(wsName_EW, work_in1);
    AnalysisDataService::Instance().add(wsName_2D, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName_EW);
    alg.setPropertyValue("RHSWorkspace",wsName_2D);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName_EW);
    AnalysisDataService::Instance().remove(wsName_2D);
    AnalysisDataService::Instance().remove(wsNameOut);

  }

  void testCompoundAssignment()
  {
    MatrixWorkspace_sptr a = WorkspaceCreationHelper::CreateWorkspaceSingleValue(3);
    const Workspace_const_sptr b = a;
    MatrixWorkspace_sptr c = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
    a -= 5;
    TS_ASSERT_EQUALS(a->readY(0)[0],-2);
    TS_ASSERT_EQUALS(a,b);
    a -= c;
    TS_ASSERT_EQUALS(a->readY(0)[0],-4);
    TS_ASSERT_EQUALS(a,b);
  }

  int numBins;
  int numPixels;
  std::string wsName_EW, wsName_2D, wsNameOut;
  EventWorkspace_sptr work_in1;
  Workspace2D_sptr work_in2;


  void EventSetup()
  {
    numBins = 100;
    numPixels = 500;
    //Workspace with 2 events per bin; 100 bins from 0 to 100
    work_in1 = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);

    //2D workspace with 0.5 in each bin.
    work_in2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(numPixels, numBins, 0.0, 1.0);
    for (int pix=0; pix < numPixels; pix++)
    {
      work_in2->dataY(pix).assign(numBins, 0.5);
      work_in2->dataE(pix).assign(numBins, sqrt(0.5));
    }

    //Put the stuff in the data service
    wsName_EW = "test_inA";
    wsName_2D = "test_inB";
    wsNameOut = "test_out";
    AnalysisDataService::Instance().add(wsName_EW, work_in1);
    AnalysisDataService::Instance().add(wsName_2D, work_in2);
  }

  void EventTeardown()
  {
    AnalysisDataService::Instance().remove(wsName_EW);
    AnalysisDataService::Instance().remove(wsName_2D);
    AnalysisDataService::Instance().remove(wsNameOut);
  }


  void test_EventWorkspace_minus_Workspace2D()
  {
    EventSetup();

    //Do the minus
    Minus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName_EW);
    alg.setPropertyValue("RHSWorkspace",wsName_2D);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    //The output!
    MatrixWorkspace_const_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));
    // The output is NOT an EventWorkspace
    EventWorkspace_const_sptr eventOut = boost::dynamic_pointer_cast<const EventWorkspace>(work_out1);
    TS_ASSERT(!eventOut);
    MatrixWorkspace_const_sptr work_in1_const;
    TS_ASSERT_THROWS_NOTHING(work_in1_const = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName_EW)));

    //Compare
    for (int pix=0; pix < numPixels; pix+=499)
      for (int i=0; i < numBins; i++)
      {
        //Output should be 1.5 everywhere
        TS_ASSERT_DELTA(  work_out1->dataY(pix)[i], 1.50, 1e-5);
        //And the error is the sum of the square of the incoming errors
        TS_ASSERT_DELTA(  work_out1->dataE(pix)[i], sqrt(2.50), 1e-5);
        //Incoming event workspace should still have 2.0 for values
        TS_ASSERT_DELTA(  work_in1_const->dataY(pix)[i], 2.00, 1e-5);
        //And error
        TS_ASSERT_DELTA(  work_in1_const->dataE(pix)[i], sqrt(2.0), 1e-5);
      }

    EventTeardown();
  }

  void test_Workspace2D_minus_EventWorkspace()
  {
    EventSetup();

    //Do the minus
    Minus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName_2D);
    alg.setPropertyValue("RHSWorkspace",wsName_EW);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    //The output!
    MatrixWorkspace_const_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));
    // The output is NOT an EventWorkspace
    EventWorkspace_const_sptr eventOut = boost::dynamic_pointer_cast<const EventWorkspace>(work_out1);
    TS_ASSERT(!eventOut);
    MatrixWorkspace_const_sptr work_in1_const;
    TS_ASSERT_THROWS_NOTHING(work_in1_const = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName_EW)));

    //Compare
    for (int pix=0; pix < numPixels; pix+=499)
      for (int i=0; i < numBins; i++)
      {
        //0.5-2 = -1.5
        TS_ASSERT_DELTA(  work_out1->dataY(pix)[i], -1.50, 1e-5);
        //And the error is the sum of the square of the incoming errors
        TS_ASSERT_DELTA(  work_out1->dataE(pix)[i], sqrt(2.50), 1e-5);
        //Incoming event workspace should still have 2.0 for values
        TS_ASSERT_DELTA(  work_in1_const->dataY(pix)[i], 2.00, 1e-5);
        //And error
        TS_ASSERT_DELTA(  work_in1_const->dataE(pix)[i], sqrt(2.0), 1e-5);
      }

    EventTeardown();
  }



  void test_EventWorkspace_minus_EventWorkspace()
  {
    EventSetup();

    //Do the minus
    Minus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName_EW);
    alg.setPropertyValue("RHSWorkspace",wsName_EW);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    //The output!
    MatrixWorkspace_const_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));
    //And it is still an EventWorkspace?
    EventWorkspace_const_sptr eventOut = boost::dynamic_pointer_cast<const EventWorkspace>(work_out1);
    TS_ASSERT(eventOut);
    if (eventOut)
    {
      TS_ASSERT_EQUALS( eventOut->getNumberEvents(), numBins * numPixels * 2 * 2 );
      TS_ASSERT_EQUALS( eventOut->getEventType(), Mantid::API::WEIGHTED );
    }

    MatrixWorkspace_const_sptr work_in1_const;
    TS_ASSERT_THROWS_NOTHING(work_in1_const = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName_EW)));

    //Compare
    for (int pix=0; pix < numPixels; pix+=49)
      for (int i=0; i < numBins; i+=7)
      {
        //Output should be 1.5 everywhere
        TS_ASSERT_DELTA(  work_out1->dataY(pix)[i], 0.0, 1e-5);
        //And the error is the sum of the square of the incoming errors
        TS_ASSERT_DELTA(  work_out1->dataE(pix)[i], sqrt(4.0), 1e-5);

        //Incoming event workspace should still have 2.0 for values
        TS_ASSERT_DELTA(  work_in1_const->dataY(pix)[i], 2.00, 1e-5);
        //And error
        TS_ASSERT_DELTA(  work_in1_const->dataE(pix)[i], sqrt(2.0), 1e-5);
      }

    EventTeardown();
  }



private:
  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1)
  {
    //default to a horizontal loop orientation
    checkData(work_in1,work_in2,work_out1,0);
  }

  // loopOrientation 0=Horizontal, 1=Vertical
  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1, int loopOrientation)
  {
    int ws2LoopCount;
    if (work_in2->size() > 0)
    {
      ws2LoopCount = work_in1->size()/work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

    for (int i = 0; i < work_out1->size(); i++)
    {
      int ws2Index = i;
    
      if (ws2LoopCount > 1)
      {
        if (loopOrientation == 0)
        {
          ws2Index = i%ws2LoopCount;
        }
        else
        {
          ws2Index = i/ws2LoopCount;
        }
      }
      checkDataItem(work_in1,work_in2,work_out1,i,ws2Index);
    }
  }

  void checkDataItem (MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1, int i, int ws2Index)
  {
    //printf("I=%d\tws2Index=%d\n",i,ws2Index);
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig2 = work_in2->dataY(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      TS_ASSERT_DELTA(sig1 - sig2, sig3, 0.0001);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double err3(sqrt((err1*err1) + (err2*err2)));     
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
  }

};

#endif /*MINUSTEST_H_*/
