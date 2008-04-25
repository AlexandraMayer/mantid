#ifndef CHAINEDOPERATORTEST_H_
#define CHAINEDOPERATORTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/Plus.h"
#include "MantidAlgorithms/Minus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;


class ComplexOpTest : public Algorithm
{
public:

  ComplexOpTest() : Algorithm() {}
  virtual ~ComplexOpTest() {}
  void init() 
  {
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_1","",Direction::Input));
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_2","",Direction::Input));
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));   
  }
  void exec() 
  {
    Workspace_sptr in_work1 = getProperty("InputWorkspace_1");
    Workspace_sptr in_work2 = getProperty("InputWorkspace_2");

    Workspace_sptr out_work = (in_work1 + in_work2)/(in_work1 - in_work2);
    setProperty("OutputWorkspace",out_work);
  }
  virtual const std::string name() const {return "ComplexOpTest";}
  virtual const int version() const {return(1);}

};

class ChainedOperatorTest : public CxxTest::TestSuite
{
public:

  void testChainedOperator()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    Workspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    Workspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    ComplexOpTest alg;

    std::string wsNameIn1 = "testChainedOperator_in21";
    std::string wsNameIn2 = "testChainedOperator_in22";
    std::string wsNameOut = "testChainedOperator_out";
    AnalysisDataService::Instance().add(wsNameIn1, work_in1);
    AnalysisDataService::Instance().add(wsNameIn2, work_in2);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace_1",wsNameIn1);
    alg.setPropertyValue("InputWorkspace_2",wsNameIn2);    
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    Workspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = AnalysisDataService::Instance().retrieve(wsNameOut));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsNameIn1);
    AnalysisDataService::Instance().remove(wsNameIn2);
    AnalysisDataService::Instance().remove(wsNameOut);
  }

private:

  void checkData( Workspace_sptr work_in1,  Workspace_sptr work_in2, Workspace_sptr work_out1)
  {
    int ws2LoopCount;
    if (work_in2->size() > 0)
    {
      ws2LoopCount = work_in1->size()/work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

    for (int i = 0; i < work_out1->size(); i++)
    {
      checkDataItem(work_in1,work_in2,work_out1,i,i/ws2LoopCount);
    }
  }

  void checkDataItem (Workspace_sptr work_in1,  Workspace_sptr work_in2, Workspace_sptr work_out1, int i, int ws2Index)
  {
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig2 = work_in2->dataY(ws2Index/work_in1->blocksize())[ws2Index%work_in1->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA((sig1 + sig2)/(sig1-sig2), sig3, 0.0001);
      //Note err calculation not checked due to complexity.
  }
  
};

#endif /*CHAINEDOPERATORTEST_H_*/
