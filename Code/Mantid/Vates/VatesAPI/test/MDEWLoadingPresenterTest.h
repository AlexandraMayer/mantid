#ifndef MDEW_LOADING_PRESENTER_TEST_H_
#define MDEW_LOADING_PRESENTER_TEST_H_ 

#include <cxxtest/TestSuite.h>
#include <vtkUnstructuredGrid.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include "MantidVatesAPI/MDEWLoadingPresenter.h"

using namespace testing;
using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class MDEWLoadingPresenterTest : public CxxTest::TestSuite
{

private: 

  /*
  Helper class allows the behaviour of the abstract base type to be tested. Derives from target abstract class providing 
  dummy implemenations of pure virtual methods.
  */
  class ConcreteMDEWLoadingPresenter : public MDEWLoadingPresenter<MockMDLoadingView>
  {
  private:
    typedef MDEWLoadingPresenter<MockMDLoadingView> BaseClass;
  public:
    ConcreteMDEWLoadingPresenter(MockMDLoadingView* view) : MDEWLoadingPresenter<MockMDLoadingView>("", view)
    {
    }

    virtual vtkDataSet* execute(vtkDataSetFactory*, ProgressAction&)
    {
      return vtkUnstructuredGrid::New(); 
    }
    
    virtual void executeLoadMetadata()
    {
    }

    virtual bool canReadFile() const
    {
      return true;
    }

    virtual bool shouldLoad()
    {
      //Forwarding method
      return BaseClass::shouldLoad();
    }

    ~ConcreteMDEWLoadingPresenter(){}
  };


public:

void testShouldLoadFirstTimeRound()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2); 
  EXPECT_CALL(view, getLoadInMemory()).Times(2); 
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Should NOT request load on second usage. Should have it's state syncrhonised with view and the view hasn't changed!", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testTimeChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2); 
  EXPECT_CALL(view, getLoadInMemory()).Times(2); 
  EXPECT_CALL(view, getTime()).Times(2)
    .WillOnce(Return(0)) 
    .WillOnce(Return(1));// Time has changed on 2nd call
  EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Time has changed, but that shouldn't trigger load", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testLoadInMemoryChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2); 
  EXPECT_CALL(view, getLoadInMemory()).Times(2)
    .WillOnce(Return(true)) 
    .WillOnce(Return(false)); // Load in memory changed
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Load in memory changed. this SHOULD trigger re-load", presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}

void testDepthChanged()
{
  MockMDLoadingView view;
  EXPECT_CALL(view, getRecursionDepth()).Times(2)
    .WillOnce(Return(10)) 
    .WillOnce(Return(100)); // Recursion depth changed.
  EXPECT_CALL(view, getLoadInMemory()).Times(2);
  EXPECT_CALL(view, getTime()).Times(2);
  EXPECT_CALL(view, updateAlgorithmProgress(_)).Times(0);

  ConcreteMDEWLoadingPresenter presenter(&view);
  TSM_ASSERT("Should request load on first usage.", presenter.shouldLoad());
  TSM_ASSERT("Depth has changed, but that shouldn't trigger load", !presenter.shouldLoad());
  
  TSM_ASSERT("View not used as expected.", Mock::VerifyAndClearExpectations(&view));
}



};

#endif