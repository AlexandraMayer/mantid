#ifndef CHECKWORKSPACESMATCHTEST_H_
#define CHECKWORKSPACESMATCHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceGroup_sptr;

using Mantid::DataObjects::EventWorkspace_sptr;

class CheckWorkspacesMatchTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CheckWorkspacesMatchTest *createSuite() { return new CheckWorkspacesMatchTest(); }
  static void destroySuite( CheckWorkspacesMatchTest *suite ) { delete suite; }

  CheckWorkspacesMatchTest() : ws1(WorkspaceCreationHelper::Create2DWorkspace123(2,2))
  {
  }
  
  void testName()
  {
    TS_ASSERT_EQUALS( checker.name(), "CheckWorkspacesMatch" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( checker.version(), 1 );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( checker.initialize() );
    TS_ASSERT( checker.isInitialized() );
  }
  
  void testMatches()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(10,100);
    // A workspace had better match itself!
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), checker.successString() );
    // Same, using the Mantid::API::equals() function
    TS_ASSERT( Mantid::API::equals(ws, ws) );
  }
  
  void testEvent_matches()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",boost::dynamic_pointer_cast<MatrixWorkspace>(ews1)) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",boost::dynamic_pointer_cast<MatrixWorkspace>(ews2)) );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), checker.successString() );

    // Same, using the Mantid::API::equals() function
    TS_ASSERT( Mantid::API::equals(ews1, ews2) );
  }

  void testEvent_different_type()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",boost::dynamic_pointer_cast<MatrixWorkspace>(ews2)) );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), checker.successString() );

    // Same, using the Mantid::API::equals() function
    TS_ASSERT( !Mantid::API::equals(ws1, ews2) );

  }

  void testEvent_different_number_histograms()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(15,20,30);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",boost::dynamic_pointer_cast<MatrixWorkspace>(ews1)) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",boost::dynamic_pointer_cast<MatrixWorkspace>(ews2)) );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), checker.successString() );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ews1, ews2)) );
  }

  void testEvent_differentEventLists()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30, 0.0, 1.0, 2);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",boost::dynamic_pointer_cast<MatrixWorkspace>(ews1)) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",boost::dynamic_pointer_cast<MatrixWorkspace>(ews2)) );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), checker.successString() );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ews1, ews2)) );
  }

  void testEvent_differentBinBoundaries()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30, 15.0, 10.0);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30, 5.0, 10.0);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",boost::dynamic_pointer_cast<MatrixWorkspace>(ews1)) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",boost::dynamic_pointer_cast<MatrixWorkspace>(ews2)) );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), checker.successString() );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ews1, ews2)) );
  }

  void testDifferentSize()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create1DWorkspaceFib(2);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Size mismatch" );
    
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }
  
  void testHistNotHist()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2,true);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Histogram/point-like mismatch" );

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDistNonDist()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->isDistribution(true);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Distribution flag mismatch" );

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentAxisType()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    Mantid::API::Axis* const newAxis = new Mantid::API::NumericAxis(2);
    ws2->replaceAxis(1,newAxis);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() )
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis 1 type mismatch" );

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentAxisTitles()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getAxis(0)->title() = "blah";
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis 0 title mismatch" );

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentAxisUnit()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis 0 unit mismatch" );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentAxisValues()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getAxis(1)->setValue(1,99);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() )
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis 1 values mismatch" ) ;
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentYUnit()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->setYUnit("blah");
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "YUnit mismatch" );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentSpectraMap()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getSpectrum(0)->setSpectrumNo(1234);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Spectrum number mismatch" );

    ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getSpectrum(0)->setDetectorID(99);
    ws2->getSpectrum(1)->setDetectorID(98);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Detector IDs mismatch" );

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentInstruments()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    Mantid::Geometry::Instrument_sptr instrument(new Mantid::Geometry::Instrument("different"));
    ws2->setInstrument(instrument);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Instrument name mismatch" );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentParameterMaps()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->instrumentParameters().addBool(new Mantid::Geometry::Component, "myParam", true);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Instrument ParameterMap mismatch" );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }

  void testDifferentMasking()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->maskBin(0,0);
    ws2->dataY(0)[0] = 2;
    ws2->dataE(0)[0] = 3;
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Masking mismatch" );
    
    Mantid::API::MatrixWorkspace_sptr ws3 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws3->maskBin(0,1);
    ws3->dataY(0)[1] = 2;
    ws3->dataE(0)[1] = 3;

    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws3) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Masking mismatch" );
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT( (!Mantid::API::equals(ws1, ws2)) );
  }
  
  void testDifferentSampleName()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    checker.setProperty("CheckSample",true);

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->mutableSample().setName("different");
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Sample name mismatch" );
  }

  void testDifferentProtonCharge()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    checker.setProperty("CheckSample",true);

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->mutableRun().setProtonCharge(99.99);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Proton charge mismatch" );
  }

  void testDifferentLogs()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    checker.setProperty("CheckSample",true);
    
    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop1",99));
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Different numbers of logs" );
    
    Mantid::API::MatrixWorkspace_sptr ws3 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws3->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop2",99));
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws2) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws3) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Log name mismatch" );
    
    Mantid::API::MatrixWorkspace_sptr ws4 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws4->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop1",100));
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws2) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws4) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Log value mismatch" );
  }

  void test_Input_With_Two_Groups_That_Are_The_Same_Matches()
  {
    // Create a group
    const std::string groupName("TestGroup");
    WorkspaceGroup_sptr group = WorkspaceCreationHelper::CreateWorkspaceGroup(2, 2, 2, groupName);

    doGroupTest(groupName, groupName, Mantid::Algorithms::CheckWorkspacesMatch::successString());

    cleanupGroup(group);
  }

  void test_Input_With_Two_Groups_That_Are_Different_Sizes_Gives_Size_Mismatch()
  {
    // Create a group
    const std::string groupOneName("TestGroupOne");
    WorkspaceGroup_sptr groupOne = WorkspaceCreationHelper::CreateWorkspaceGroup(2, 2, 2, groupOneName);
    const std::string groupTwoName("TestGroupTwo");
    WorkspaceGroup_sptr groupTwo = WorkspaceCreationHelper::CreateWorkspaceGroup(3, 2, 2, groupTwoName);

    doGroupTest(groupOneName, groupTwoName, "GroupWorkspaces size mismatch.");

    cleanupGroup(groupOne);
    cleanupGroup(groupTwo);
  }

  void test_Input_With_A_Group_And_A_Single_Workspace_Gives_Type_Mismatch()
  {
    const std::string groupName("CheckWorkspacesMatch_TestGroup");
    WorkspaceGroup_sptr group = WorkspaceCreationHelper::CreateWorkspaceGroup(2, 2, 2, groupName);
    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    const std::string wsName("CheckWorkspacesMatch_TestWS");
    Mantid::API::AnalysisDataService::Instance().add(wsName,ws2);

    doGroupTest(groupName, wsName, "Type mismatch. One workspace is a group, the other is not.");

    // Cleanup
    cleanupGroup(group);
    Mantid::API::AnalysisDataService::Instance().remove(wsName);
  }

  void test_Input_With_Two_Groups_When_Single_Item_Checking_Is_Disabled()
  {
    // Create a group
    const std::string groupOneName("TestGroupOne");
    WorkspaceGroup_sptr groupOne = WorkspaceCreationHelper::CreateWorkspaceGroup(2, 2, 2, groupOneName);
    const std::string groupTwoName("TestGroupTwo");
    WorkspaceGroup_sptr groupTwo = WorkspaceCreationHelper::CreateWorkspaceGroup(2, 2, 2, groupTwoName);
    Mantid::API::AnalysisDataServiceImpl& dataStore =  Mantid::API::AnalysisDataService::Instance();
    // Extract the zeroth element of groupTwo and add a spurious log
    MatrixWorkspace_sptr zero = boost::dynamic_pointer_cast<MatrixWorkspace>(dataStore.retrieve(groupTwo->getNames()[0]));
    TS_ASSERT(zero);
    using Mantid::Kernel::PropertyWithValue;
    zero->mutableRun().addProperty(new PropertyWithValue<double>("ExtraLog", 10));

    std::map<std::string,std::string> otherProps;
    otherProps.insert(std::make_pair("CheckSample", "1"));

    doGroupTest(groupOneName, groupTwoName, "Different numbers of logs. Inputs=[TestGroupOne_0,TestGroupTwo_0]", otherProps);

    // Cleanup
    cleanupGroup(groupOne);
    cleanupGroup(groupTwo);
  }


private:

  void doGroupTest(const std::string & inputWSOne, const std::string & inputWSTwo,
                   const std::string & expectedResult,
                   const std::map<std::string,std::string> & otherProps = std::map<std::string,std::string>())
  {
    Mantid::Algorithms::CheckWorkspacesMatch matcher;
    matcher.initialize();
    matcher.setPropertyValue("Workspace1", inputWSOne);
    matcher.setPropertyValue("Workspace2", inputWSTwo);
    std::map<std::string,std::string>::const_iterator iend = otherProps.end();
    std::map<std::string,std::string>::const_iterator itr = otherProps.begin();
    for(; itr != iend; ++itr)
    {
      matcher.setPropertyValue(itr->first, itr->second);
    }

    TS_ASSERT_THROWS_NOTHING(matcher.execute());
    TS_ASSERT_EQUALS(matcher.isExecuted(), true);
    TS_ASSERT_EQUALS(matcher.getPropertyValue("Result"), expectedResult);
  }

  void cleanupGroup(const WorkspaceGroup_sptr group)
  {
    group->deepRemoveAll();
    const std::string name = group->getName();
    Mantid::API::AnalysisDataService::Instance().remove(name);
  }

private:
  Mantid::Algorithms::CheckWorkspacesMatch checker;
  const Mantid::API::MatrixWorkspace_sptr ws1;
};

#endif /*CHECKWORKSPACESMATCHTEST_H_*/
