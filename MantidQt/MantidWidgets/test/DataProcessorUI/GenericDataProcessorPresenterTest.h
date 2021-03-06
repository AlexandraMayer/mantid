#ifndef MANTID_MANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERTEST_H
#define MANTID_MANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ProgressableViewMockObject.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class GenericDataProcessorPresenterTest : public CxxTest::TestSuite {

private:
  DataProcessorWhiteList createReflectometryWhiteList() {

    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace", "", true, "TOF_");
    whitelist.addElement("Angle", "ThetaIn", "");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "",
                         true, "TRANS_");
    whitelist.addElement("Q min", "MomentumTransferMin", "");
    whitelist.addElement("Q max", "MomentumTransferMax", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    return whitelist;
  }

  std::map<std::string, DataProcessorPreprocessingAlgorithm>
  createReflectometryPreprocessMap() {

    return std::map<std::string, DataProcessorPreprocessingAlgorithm>{
        {"Run(s)", DataProcessorPreprocessingAlgorithm(
                       "Plus", "TOF_",
                       std::set<std::string>{"LHSWorkspace", "RHSWorkspace",
                                             "OutputWorkspace"})},
        {"Transmission Run(s)",
         DataProcessorPreprocessingAlgorithm(
             "CreateTransmissionWorkspaceAuto", "TRANS_",
             std::set<std::string>{"FirstTransmissionRun",
                                   "SecondTransmissionRun",
                                   "OutputWorkspace"})}};
  }

  DataProcessorProcessingAlgorithm createReflectometryProcessor() {

    return DataProcessorProcessingAlgorithm(
        "ReflectometryReductionOneAuto",
        std::vector<std::string>{"IvsQ_binned_", "IvsQ_", "IvsLam_"},
        std::set<std::string>{"ThetaIn", "ThetaOut", "InputWorkspace",
                              "OutputWorkspace", "OutputWorkspaceWavelength",
                              "FirstTransmissionRun", "SecondTransmissionRun"});
  }

  DataProcessorPostprocessingAlgorithm createReflectometryPostprocessor() {

    return DataProcessorPostprocessingAlgorithm(
        "Stitch1DMany", "IvsQ_",
        std::set<std::string>{"InputWorkspaces", "OutputWorkspace"});
  }

  ITableWorkspace_sptr
  createWorkspace(const std::string &wsName,
                  const DataProcessorWhiteList &whitelist) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    const int ncols = static_cast<int>(whitelist.size());

    auto colGroup = ws->addColumn("str", "Group");
    colGroup->setPlotType(0);

    for (int col = 0; col < ncols; col++) {
      auto column = ws->addColumn("str", whitelist.colNameFromColIndex(col));
      column->setPlotType(0);
    }

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  void createTOFWorkspace(const std::string &wsName,
                          const std::string &runNumber = "") {
    auto tinyWS =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();
    auto inst = tinyWS->getInstrument();

    inst->getParameterMap()->addDouble(inst.get(), "I0MonitorIndex", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "PointDetectorStart", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "PointDetectorStop", 1.0);
    inst->getParameterMap()->addDouble(inst.get(), "LambdaMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "LambdaMax", 10.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorBackgroundMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorBackgroundMax",
                                       10.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorIntegralMin", 0.0);
    inst->getParameterMap()->addDouble(inst.get(), "MonitorIntegralMax", 10.0);

    tinyWS->mutableRun().addLogData(
        new PropertyWithValue<double>("Theta", 0.12345));
    if (!runNumber.empty())
      tinyWS->mutableRun().addLogData(
          new PropertyWithValue<std::string>("run_number", runNumber));

    AnalysisDataService::Instance().addOrReplace(wsName, tinyWS);
  }

  void createMultiPeriodTOFWorkspace(const std::string &wsName,
                                     const std::string &runNumber = "") {

    createTOFWorkspace(wsName + "_1", runNumber);
    createTOFWorkspace(wsName + "_2", runNumber);

    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(
        AnalysisDataService::Instance().retrieve(wsName + "_1"));
    group->addWorkspace(
        AnalysisDataService::Instance().retrieve(wsName + "_2"));

    AnalysisDataService::Instance().addOrReplace(wsName, group);
  }

  ITableWorkspace_sptr
  createPrefilledWorkspace(const std::string &wsName,
                           const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
        << "12345"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "ProcessingInstructions='0'";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "ProcessingInstructions='0'";
    row = ws->appendRow();
    row << "1"
        << "24681"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"

        << "";
    row = ws->appendRow();
    row << "1"
        << "24682"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"

        << "";
    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledWorkspaceThreeGroups(const std::string &wsName,
                                      const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
        << "12345"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "24681"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "24682"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "2"
        << "30000"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "2"
        << "30001"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledWorkspaceWithTrans(const std::string &wsName,
                                    const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "0"
        << "12345"
        << "0.5"
        << "11115"
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"

        << "";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << "11116"
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"

        << "";
    row = ws->appendRow();
    row << "1"
        << "24681"
        << "0.5"
        << "22221"
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"

        << "";
    row = ws->appendRow();
    row << "1"
        << "24682"
        << "1.5"
        << "22222"
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"

        << "";
    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static GenericDataProcessorPresenterTest *createSuite() {
    return new GenericDataProcessorPresenterTest();
  }
  static void destroySuite(GenericDataProcessorPresenterTest *suite) {
    delete suite;
  }

  GenericDataProcessorPresenterTest() { FrameworkManager::Instance(); }

  void testConstructor() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    // We don't the view we will handle yet, so none of the methods below should
    // be
    // called
    EXPECT_CALL(mockDataProcessorView, setTableList(_)).Times(0);
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, _)).Times(0);
    EXPECT_CALL(mockDataProcessorView, addActionsProxy()).Times(0);
    // Constructor
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));

    // Check that the presenter updates the whitelist adding columns 'Group' and
    // 'Options'
    auto whitelist = presenter.getWhiteList();
    TS_ASSERT_EQUALS(whitelist.size(), 8);
    TS_ASSERT_EQUALS(whitelist.colNameFromColIndex(0), "Run(s)");
    TS_ASSERT_EQUALS(whitelist.colNameFromColIndex(7), "Options");
  }

  void testPresenterAcceptsViews() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;

    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());

    // When the presenter accepts the views, expect the following:
    // Expect that the list of actions is published
    EXPECT_CALL(mockDataProcessorView, addActionsProxy()).Times(Exactly(1));
    // Expect that the list of settings is populated
    EXPECT_CALL(mockDataProcessorView, loadSettings(_)).Times(Exactly(1));
    // Expect that the list of tables is populated
    EXPECT_CALL(mockDataProcessorView, setTableList(_)).Times(Exactly(1));
    // Expect that the layout containing pre-processing, processing and
    // post-processing options is created
    std::vector<std::string> stages = {"Pre-process", "Pre-process", "Process",
                                       "Post-process"};
    std::vector<std::string> algorithms = {
        "Plus", "CreateTransmissionWorkspaceAuto",
        "ReflectometryReductionOneAuto", "Stitch1DMany"};

    // Expect that the autocompletion hints are populated
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, 7))
        .Times(Exactly(1));
    // Now accept the views
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
  }

  void testSaveNew() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    presenter.notify(DataProcessorPresenter::NewTableFlag);

    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace"))
        .Times(1)
        .WillOnce(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::SaveFlag);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("TestWorkspace"));
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testSaveExisting() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace")).Times(0);
    presenter.notify(DataProcessorPresenter::SaveFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testSaveAs() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace"))
        .Times(1)
        .WillOnce(Return(""));
    presenter.notify(DataProcessorPresenter::SaveAsFlag);

    // The user hits "save as" and and enters "Workspace" for a name
    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace"))
        .Times(1)
        .WillOnce(Return("Workspace"));
    presenter.notify(DataProcessorPresenter::SaveAsFlag);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));
    ITableWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->columnCount(), 9);

    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testAppendRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "append row" twice with no rows selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has been modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testAppendRowSpecify() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "append row" twice, with the second row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has been modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testAppendRowSpecifyPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "append row" once, with the second, third, and fourth row
    // selected.
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testAppendRowSpecifyGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "append row" once, with the first group selected.
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testAppendGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "append row" once, with the first group selected.
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendGroupFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testAppendGroupSpecifyPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspaceThreeGroups("TestWorkspace",
                                        presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    std::set<int> grouplist;
    grouplist.insert(0);
    grouplist.insert(1);

    // The user hits "append group" once, with the first and second groups
    // selected.
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter.notify(DataProcessorPresenter::AppendGroupFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table was modified correctly
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 7);
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "2");
    TS_ASSERT_EQUALS(ws->String(6, GroupCol), "2");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testDeleteRowNone() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "delete row" with no rows selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(0);
    presenter.notify(DataProcessorPresenter::DeleteRowFlag);

    // The user hits save
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has not lost any rows
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testDeleteRowSingle() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "delete row" with the second row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(0);
    presenter.notify(DataProcessorPresenter::DeleteRowFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24682");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testDeleteRowPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "delete row" with the first three rows selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::DeleteRowFlag);

    // The user hits save
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check the rows were deleted as expected
    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "24682");
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testDeleteGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "delete group" with no groups selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::DeleteGroupFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testDeleteGroupPlural() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspaceThreeGroups("TestWorkspace",
                                        presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);
    grouplist.insert(1);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "delete row" with the second row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter.notify(DataProcessorPresenter::DeleteGroupFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 2);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "30000");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "30001");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "2");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "2");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcess() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params = \"0.1\""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspaces were created as expected
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12345"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12346"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12346");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12346");
    AnalysisDataService::Instance().remove("TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testTreeUpdatedAfterProcess() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    auto ws =
        createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    ws->String(0, ThetaCol) = "";
    ws->String(1, ScaleCol) = "";
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params = \"0.1\""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter.notify(DataProcessorPresenter::ProcessFlag);
    presenter.notify(DataProcessorPresenter::SaveFlag);

    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT(ws->String(0, ThetaCol) != "");
    TS_ASSERT(ws->String(1, ScaleCol) != "");

    // Check output workspaces were created as expected
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12345"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12346"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12346");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12346");
    AnalysisDataService::Instance().remove("TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testTreeUpdatedAfterProcessMultiPeriod() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    auto ws =
        createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    ws->String(0, ThetaCol) = "";
    ws->String(0, ScaleCol) = "";
    ws->String(1, ThetaCol) = "";
    ws->String(1, ScaleCol) = "";
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    createMultiPeriodTOFWorkspace("TOF_12345", "12345");
    createMultiPeriodTOFWorkspace("TOF_12346", "12346");

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params = \"0.1\""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter.notify(DataProcessorPresenter::ProcessFlag);
    presenter.notify(DataProcessorPresenter::SaveFlag);

    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(0, ThetaCol), "22.5");
    TS_ASSERT_EQUALS(ws->String(0, ScaleCol), "1");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "22.5");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "1");

    // Check output workspaces were created as expected
    // Check output workspaces were created as expected
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12345"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12346"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    // Tidy up
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessOnlyRowsSelected() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first two rows
    // selected
    // This means we will process the selected rows but we will not
    // post-process them
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _)).Times(0);
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params = \"0.1\""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspaces were created as expected
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12345"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("TOF_12346"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12346");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12346");
    AnalysisDataService::Instance().remove("TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testProcessWithNotebook() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillRepeatedly(Return("Params = \"0.1\""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(1);
    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12346");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12346");
    AnalysisDataService::Instance().remove("TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  /*
  * Test processing workspaces with non-standard names, with
  * and without run_number information in the sample log.
  */
  void testProcessCustomNames() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    auto ws = createWorkspace("TestWorkspace", presenter.getWhiteList());
    TableRow row = ws->appendRow();
    row << "1"
        << "dataA"
        << "0.7"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "ProcessingInstructions='0'";
    row = ws->appendRow();
    row << "1"
        << "dataB"
        << "2.3"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "ProcessingInstructions='0'";

    createTOFWorkspace("dataA");
    createTOFWorkspace("dataB");

    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params = \"0.1\""));

    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspaces were created as expected
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_dataA"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_binned_TOF_dataB"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_dataA"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_dataB"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_dataA"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_dataB"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_dataA_TOF_dataB"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("dataA");
    AnalysisDataService::Instance().remove("dataB");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_dataA");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_dataB");
    AnalysisDataService::Instance().remove("IvsQ_TOF_dataA");
    AnalysisDataService::Instance().remove("IvsQ_TOF_dataB");
    AnalysisDataService::Instance().remove("IvsLam_TOF_dataA");
    AnalysisDataService::Instance().remove("IvsLam_TOF_dataB");
    AnalysisDataService::Instance().remove("IvsQ_TOF_dataA_TOF_dataB");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testBadWorkspaceType() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    // Wrong types
    ws->addColumn("int", "StitchGroup");
    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "ThetaIn");
    ws->addColumn("str", "TransRun(s)");
    ws->addColumn("str", "Qmin");
    ws->addColumn("str", "Qmax");
    ws->addColumn("str", "dq/q");
    ws->addColumn("str", "Scale");
    ws->addColumn("str", "Options");

    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // We should receive an error
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(1);

    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testBadWorkspaceLength() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // Because we to open twice, get an error twice
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(2);
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(2)
        .WillRepeatedly(Return("TestWorkspace"));

    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "StitchGroup");
    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "ThetaIn");
    ws->addColumn("str", "TransRun(s)");
    ws->addColumn("str", "Qmin");
    ws->addColumn("str", "Qmax");
    ws->addColumn("str", "dq/q");
    ws->addColumn("str", "Scale");
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    // Try to open with too few columns
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    ws->addColumn("str", "OptionsA");
    ws->addColumn("str", "OptionsB");
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", ws);

    // Try to open with too many columns
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPromptSaveAfterAppendRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // User hits "append row"
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // The user will decide not to discard their changes
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _))
        .Times(1)
        .WillOnce(Return(false));

    // Then hits "new table" without having saved
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace"))
        .Times(1)
        .WillOnce(Return("Workspace"));
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _)).Times(0);
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPromptSaveAfterAppendGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // User hits "append group"
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendGroupFlag);

    // The user will decide not to discard their changes
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _))
        .Times(1)
        .WillOnce(Return(false));

    // Then hits "new table" without having saved
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace"))
        .Times(1)
        .WillOnce(Return("Workspace"));
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _)).Times(0);
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPromptSaveAfterDeleteRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // User hits "append row" a couple of times
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // The user saves
    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace"))
        .Times(1)
        .WillOnce(Return("Workspace"));
    presenter.notify(DataProcessorPresenter::SaveFlag);

    //...then deletes the 2nd row
    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::DeleteRowFlag);

    // The user will decide not to discard their changes when asked
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _))
        .Times(1)
        .WillOnce(Return(false));

    // Then hits "new table" without having saved
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _)).Times(0);
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPromptSaveAfterDeleteGroup() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // User hits "append group" a couple of times
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendGroupFlag);
    presenter.notify(DataProcessorPresenter::AppendGroupFlag);

    // The user saves
    EXPECT_CALL(mockMainPresenter, askUserString(_, _, "Workspace"))
        .Times(1)
        .WillOnce(Return("Workspace"));
    presenter.notify(DataProcessorPresenter::SaveFlag);

    //...then deletes the 2nd row
    std::set<int> grouplist;
    grouplist.insert(1);
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter.notify(DataProcessorPresenter::DeleteGroupFlag);

    // The user will decide not to discard their changes when asked
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _))
        .Times(1)
        .WillOnce(Return(false));

    // Then hits "new table" without having saved
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    // The user saves
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // The user tries to create a new table again, and does not get bothered
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _)).Times(0);
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    AnalysisDataService::Instance().remove("Workspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPromptSaveAndDiscard() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // User hits "append row" a couple of times
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // Then hits "new table", and decides to discard
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _))
        .Times(1)
        .WillOnce(Return(true));
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    // These next two times they don't get prompted - they have a new table
    presenter.notify(DataProcessorPresenter::NewTableFlag);
    presenter.notify(DataProcessorPresenter::NewTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPromptSaveOnOpen() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());

    // User hits "append row"
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::AppendRowFlag);

    // and tries to open a workspace, but gets prompted and decides not to
    // discard
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _))
        .Times(1)
        .WillOnce(Return(false));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // the user does it again, but discards
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // the user does it one more time, and is not prompted
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    EXPECT_CALL(mockMainPresenter, askUserYesNo(_, _)).Times(0);
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testExpandSelection() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    auto ws = createWorkspace("TestWorkspace", presenter.getWhiteList());
    TableRow row = ws->appendRow();
    row << "0"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 0
    row = ws->appendRow();
    row << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 1
    row = ws->appendRow();
    row << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 2
    row = ws->appendRow();
    row << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 3
    row = ws->appendRow();
    row << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 4
    row = ws->appendRow();
    row << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 5
    row = ws->appendRow();
    row << "3"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 6
    row = ws->appendRow();
    row << "4"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 7
    row = ws->appendRow();
    row << "4"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 8
    row = ws->appendRow();
    row << "5"
        << ""
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"

        << ""; // Row 9

    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    std::map<int, std::set<int>> selection;
    std::set<int> expected;

    selection[0].insert(0);
    expected.insert(0);

    // With row 0 selected, we shouldn't expand at all
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter.notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With 0,1 selected, we should finish with groups 0,1 selected
    selection.clear();
    selection[0].insert(0);
    selection[1].insert(0);

    expected.clear();
    expected.insert(0);
    expected.insert(1);

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter.notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With 1,6 selected, we should finish with groups 1,3 selected
    selection.clear();
    selection[1].insert(0);
    selection[3].insert(0);

    expected.clear();
    expected.insert(1);
    expected.insert(3);

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter.notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With 4,8 selected, we should finish with groups 2,4 selected
    selection.clear();
    selection[2].insert(1);
    selection[4].insert(2);

    expected.clear();
    expected.insert(2);
    expected.insert(4);

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(ContainerEq(expected)))
        .Times(1);
    presenter.notify(DataProcessorPresenter::ExpandSelectionFlag);

    // With nothing selected, we should finish with nothing selected
    selection.clear();
    expected.clear();

    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, setSelection(_)).Times(0);
    presenter.notify(DataProcessorPresenter::ExpandSelectionFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testGroupRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    auto ws = createWorkspace("TestWorkspace", presenter.getWhiteList());
    TableRow row = ws->appendRow();
    row << "0"
        << "0"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 0
    row = ws->appendRow();
    row << "0"
        << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 1
    row = ws->appendRow();
    row << "0"
        << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 2
    row = ws->appendRow();
    row << "0"
        << "3"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 3

    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> selection;
    selection[0].insert(0);
    selection[0].insert(1);

    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(selection));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::GroupRowsFlag);
    presenter.notify(DataProcessorPresenter::SaveFlag);

    // Check that the table has been modified correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "");
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "2");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "3");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "0");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "1");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testGroupRowsNothingSelected() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    auto ws = createWorkspace("TestWorkspace", presenter.getWhiteList());
    TableRow row = ws->appendRow();
    row << "0"
        << "0"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 0
    row = ws->appendRow();
    row << "0"
        << "1"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 1
    row = ws->appendRow();
    row << "0"
        << "2"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 2
    row = ws->appendRow();
    row << "0"
        << "3"
        << ""
        << ""
        << ""
        << ""
        << ""
        << "1"
        << ""; // Row 3

    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents()).Times(0);
    presenter.notify(DataProcessorPresenter::GroupRowsFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testClearRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits "clear selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::ClearSelectedFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the group ids have been set correctly
    TS_ASSERT_EQUALS(ws->String(0, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(3, GroupCol), "1");

    // Make sure the selected rows are clear
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "");
    TS_ASSERT_EQUALS(ws->String(2, ThetaCol), "");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "");
    TS_ASSERT_EQUALS(ws->String(2, TransCol), "");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "");
    TS_ASSERT_EQUALS(ws->String(2, QMinCol), "");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "");
    TS_ASSERT_EQUALS(ws->String(2, QMaxCol), "");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "");
    TS_ASSERT_EQUALS(ws->String(2, DQQCol), "");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "");
    TS_ASSERT_EQUALS(ws->String(2, ScaleCol), "");

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testCopyRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);

    const std::string expected =
        "0\t12346\t1.5\t\t1.4\t2.9\t0.04\t1\tProcessingInstructions='0'";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::CopySelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testCopyEmptySelection() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(std::string())).Times(1);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    presenter.notify(DataProcessorPresenter::CopySelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testCopyRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);
    rowlist[1].insert(1);

    const std::string expected =
        "0\t12345\t0.5\t\t0.1\t1.6\t0.04\t1\tProcessingInstructions='0'\n"
        "0\t12346\t1.5\t\t1.4\t2.9\t0.04\t1\tProcessingInstructions='0'\n"
        "1\t24681\t0.5\t\t0.1\t1.6\t0.04\t1\t\n"
        "1\t24682\t1.5\t\t1.4\t2.9\t0.04\t1\t";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::CopySelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testCutRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);

    const std::string expected =
        "0\t12346\t1.5\t\t1.4\t2.9\t0.04\t1\tProcessingInstructions='0'";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::CutSelectedFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24682");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testCutRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    const std::string expected =
        "0\t12345\t0.5\t\t0.1\t1.6\t0.04\t1\tProcessingInstructions='0'\n"
        "0\t12346\t1.5\t\t1.4\t2.9\t0.04\t1\tProcessingInstructions='0'\n"
        "1\t24681\t0.5\t\t0.1\t1.6\t0.04\t1\t";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, setClipboard(expected));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::CutSelectedFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    // Check the only unselected row is left behind
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "24682");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPasteRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);

    const std::string clipboard = "6\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(1, OptionsCol), "abc");
    // Row is going to be pasted into the group where row in clipboard
    // belongs, i.e. group 0
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPasteNewRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    const std::string clipboard = "1\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    presenter.notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 5);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(4, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(4, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(4, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(4, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(4, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(4, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, OptionsCol), "abc");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPasteRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(1);
    rowlist[1].insert(0);

    const std::string clipboard = "6\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc\n"
                                  "2\t345\t2.7\t123\t2.1\t4.3\t2.17\t3\tdef";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    presenter.notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(1, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(1, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(1, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(1, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(1, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(1, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(1, GroupCol), "0");
    TS_ASSERT_EQUALS(ws->String(1, OptionsCol), "abc");

    TS_ASSERT_EQUALS(ws->String(2, RunCol), "345");
    TS_ASSERT_EQUALS(ws->String(2, ThetaCol), "2.7");
    TS_ASSERT_EQUALS(ws->String(2, TransCol), "123");
    TS_ASSERT_EQUALS(ws->String(2, QMinCol), "2.1");
    TS_ASSERT_EQUALS(ws->String(2, QMaxCol), "4.3");
    TS_ASSERT_EQUALS(ws->String(2, DQQCol), "2.17");
    TS_ASSERT_EQUALS(ws->String(2, ScaleCol), "3");
    TS_ASSERT_EQUALS(ws->String(2, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(2, OptionsCol), "def");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPasteNewRows() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    const std::string clipboard = "1\t123\t0.5\t456\t1.2\t3.4\t3.14\t5\tabc\n"
                                  "1\t345\t2.7\t123\t2.1\t4.3\t2.17\t3\tdef";

    // The user hits "copy selected" with the second and third rows selected
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(clipboard));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    presenter.notify(DataProcessorPresenter::PasteSelectedFlag);

    // The user hits "save"
    presenter.notify(DataProcessorPresenter::SaveFlag);

    auto ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
        "TestWorkspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    // Check the unselected rows were unaffected
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "12345");
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "12346");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "24681");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "24682");

    // Check the values were pasted correctly
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "123");
    TS_ASSERT_EQUALS(ws->String(4, ThetaCol), "0.5");
    TS_ASSERT_EQUALS(ws->String(4, TransCol), "456");
    TS_ASSERT_EQUALS(ws->String(4, QMinCol), "1.2");
    TS_ASSERT_EQUALS(ws->String(4, QMaxCol), "3.4");
    TS_ASSERT_EQUALS(ws->String(4, DQQCol), "3.14");
    TS_ASSERT_EQUALS(ws->String(4, ScaleCol), "5");
    TS_ASSERT_EQUALS(ws->String(4, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(4, OptionsCol), "abc");

    TS_ASSERT_EQUALS(ws->String(5, RunCol), "345");
    TS_ASSERT_EQUALS(ws->String(5, ThetaCol), "2.7");
    TS_ASSERT_EQUALS(ws->String(5, TransCol), "123");
    TS_ASSERT_EQUALS(ws->String(5, QMinCol), "2.1");
    TS_ASSERT_EQUALS(ws->String(5, QMaxCol), "4.3");
    TS_ASSERT_EQUALS(ws->String(5, DQQCol), "2.17");
    TS_ASSERT_EQUALS(ws->String(5, ScaleCol), "3");
    TS_ASSERT_EQUALS(ws->String(5, GroupCol), "1");
    TS_ASSERT_EQUALS(ws->String(5, OptionsCol), "def");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPasteEmptyClipboard() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // Empty clipboard
    EXPECT_CALL(mockDataProcessorView, getClipboard())
        .Times(1)
        .WillRepeatedly(Return(std::string()));
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren()).Times(0);
    presenter.notify(DataProcessorPresenter::PasteSelectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testImportTable() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);
    EXPECT_CALL(mockMainPresenter,
                runPythonAlgorithm("try:\n  algm = LoadTBLDialog()\n  print "
                                   "algm.getPropertyValue(\"OutputWorkspace\")"
                                   "\nexcept:\n  pass\n"));
    presenter.notify(DataProcessorPresenter::ImportTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testExportTable() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);
    EXPECT_CALL(mockMainPresenter,
                runPythonAlgorithm(
                    "try:\n  algm = SaveTBLDialog()\nexcept:\n  pass\n"));
    presenter.notify(DataProcessorPresenter::ExportTableFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotRowWarn() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    createTOFWorkspace("TOF_12345", "12345");
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));

    // We should be warned
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _));
    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));
    presenter.notify(DataProcessorPresenter::PlotRowFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotEmptyRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);
    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(std::set<int>()));
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _));
    // Append an empty row to our table
    presenter.notify(DataProcessorPresenter::AppendRowFlag);
    // Attempt to plot the empty row (should result in critical warning)
    presenter.notify(DataProcessorPresenter::PlotRowFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotGroupWithEmptyRow() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    createTOFWorkspace("TOF_12345", "12345");
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);
    std::set<int> grouplist;
    grouplist.insert(0);
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(2)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(2)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _));
    // Open up our table with one row
    presenter.notify(DataProcessorPresenter::OpenTableFlag);
    // Append an empty row to the table
    presenter.notify(DataProcessorPresenter::AppendRowFlag);
    // Attempt to plot the group (should result in critical warning)
    presenter.notify(DataProcessorPresenter::PlotGroupFlag);
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotGroupWarn() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    createTOFWorkspace("TOF_12345", "12345");
    createTOFWorkspace("TOF_12346", "12346");
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _));
    // The user hits "plot groups" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    presenter.notify(DataProcessorPresenter::PlotGroupFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("TOF_12345");
    AnalysisDataService::Instance().remove("TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testWorkspaceNamesNoTrans() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    std::vector<std::string> row0 = {"12345", "0.5",  "",  "0.1",
                                     "0.3",   "0.04", "1", ""};
    std::vector<std::string> row1 = {"12346", "0.5",  "",  "0.1",
                                     "0.3",   "0.04", "1", ""};
    std::map<int, std::vector<std::string>> group = {{0, row0}, {1, row1}};

    // Test the names of the reduced workspaces
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row0, "prefix_1_"),
                     "prefix_1_TOF_12345");
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row1, "prefix_2_"),
                     "prefix_2_TOF_12346");
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row0), "TOF_12345");
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row1), "TOF_12346");
    // Test the names of the post-processed ws
    TS_ASSERT_EQUALS(
        presenter.getPostprocessedWorkspaceName(group, "new_prefix_"),
        "new_prefix_TOF_12345_TOF_12346");
    TS_ASSERT_EQUALS(presenter.getPostprocessedWorkspaceName(group),
                     "TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testWorkspaceNamesWithTrans() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspaceWithTrans("TestWorkspace",
                                      presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    std::vector<std::string> row0 = {"12345", "0.5",  "11115", "0.1",
                                     "0.3",   "0.04", "1",     ""};
    std::vector<std::string> row1 = {"12346", "0.5",  "11116", "0.1",
                                     "0.3",   "0.04", "1",     ""};
    std::map<int, std::vector<std::string>> group = {{0, row0}, {1, row1}};

    // Test the names of the reduced workspaces
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row0, "prefix_1_"),
                     "prefix_1_TOF_12345_TRANS_11115");
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row1, "prefix_2_"),
                     "prefix_2_TOF_12346_TRANS_11116");
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row0),
                     "TOF_12345_TRANS_11115");
    TS_ASSERT_EQUALS(presenter.getReducedWorkspaceName(row1),
                     "TOF_12346_TRANS_11116");
    // Test the names of the post-processed ws
    TS_ASSERT_EQUALS(
        presenter.getPostprocessedWorkspaceName(group, "new_prefix_"),
        "new_prefix_TOF_12345_TRANS_11115_TOF_12346_TRANS_11116");
    TS_ASSERT_EQUALS(presenter.getPostprocessedWorkspaceName(group),
                     "TOF_12345_TRANS_11115_TOF_12346_TRANS_11116");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testWorkspaceNameWrongData() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspaceWithTrans("TestWorkspace",
                                      presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");

    std::vector<std::string> row0 = {"12345", "0.5"};
    std::vector<std::string> row1 = {"12346", "0.5"};
    std::map<int, std::vector<std::string>> group = {{0, row0}, {1, row1}};

    // Test the names of the reduced workspaces
    TS_ASSERT_THROWS_ANYTHING(presenter.getReducedWorkspaceName(row0));
    TS_ASSERT_THROWS_ANYTHING(presenter.getPostprocessedWorkspaceName(group));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  /// Tests the reduction when no pre-processing algorithms are given

  void testProcessNoPreProcessing() {

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    // We don't know the view we will handle yet, so none of the methods below
    // should be called
    EXPECT_CALL(mockDataProcessorView, setTableList(_)).Times(0);
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, _)).Times(0);
    // Constructor (no pre-processing)
    GenericDataProcessorPresenter presenter(createReflectometryWhiteList(),
                                            createReflectometryProcessor(),
                                            createReflectometryPostprocessor());
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));

    // Check that the presenter has updated the whitelist adding columns 'Group'
    // and 'Options'
    auto whitelist = presenter.getWhiteList();
    TS_ASSERT_EQUALS(whitelist.size(), 8);
    TS_ASSERT_EQUALS(whitelist.colNameFromColIndex(0), "Run(s)");
    TS_ASSERT_EQUALS(whitelist.colNameFromColIndex(7), "Options");

    // When the presenter accepts the views, expect the following:
    // Expect that the list of settings is populated
    EXPECT_CALL(mockDataProcessorView, loadSettings(_)).Times(Exactly(1));
    // Expect that the list of tables is populated
    EXPECT_CALL(mockDataProcessorView, setTableList(_)).Times(Exactly(1));
    // Expect that the autocompletion hints are populated
    EXPECT_CALL(mockDataProcessorView, setOptionsHintStrategy(_, 7))
        .Times(Exactly(1));
    // Now accept the views
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    std::set<int> grouplist;
    grouplist.insert(0);

    createTOFWorkspace("12345", "12345");
    createTOFWorkspace("12346", "12346");

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions()).Times(0);
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params = \"0.1\""));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspaces were created as expected
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("12345"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_TOF_12346"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("12346"));
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12345");
    AnalysisDataService::Instance().remove("12345");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12346");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12346");
    AnalysisDataService::Instance().remove("12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotRowPythonCode() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);
    createTOFWorkspace("IvsQ_binned_TOF_12345", "12345");
    createTOFWorkspace("IvsQ_binned_TOF_12346", "12346");

    std::map<int, std::set<int>> rowlist;
    rowlist[0].insert(0);
    rowlist[0].insert(1);

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(0);
    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(rowlist));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(std::set<int>()));

    std::string pythonCode =
        "base_graph = None\nbase_graph = "
        "plotSpectrum(\"IvsQ_binned_TOF_12345\", 0, True, window = "
        "base_graph)\nbase_graph = plotSpectrum(\"IvsQ_binned_TOF_12346\", 0, "
        "True, window = base_graph)\nbase_graph.activeLayer().logLogAxes()\n";

    EXPECT_CALL(mockMainPresenter, runPythonAlgorithm(pythonCode)).Times(1);
    presenter.notify(DataProcessorPresenter::PlotRowFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testPlotGroupPythonCode() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);
    createTOFWorkspace("IvsQ_TOF_12345_TOF_12346");

    std::set<int> group = {0};

    // We should be warned
    EXPECT_CALL(mockMainPresenter, giveUserWarning(_, _)).Times(0);
    // The user hits "plot rows" with the first row selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(group));

    std::string pythonCode =
        "base_graph = None\nbase_graph = "
        "plotSpectrum(\"IvsQ_TOF_12345_TOF_12346\", 0, True, window = "
        "base_graph)\nbase_graph.activeLayer().logLogAxes()\n";

    EXPECT_CALL(mockMainPresenter, runPythonAlgorithm(pythonCode)).Times(1);
    presenter.notify(DataProcessorPresenter::PlotGroupFlag);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }

  void testNoPostProcessing() {
    // Test very basic functionality of the presenter when no post-processing
    // algorithm is defined

    NiceMock<MockDataProcessorView> mockDataProcessorView;
    MockProgressableView mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;
    GenericDataProcessorPresenter presenter(createReflectometryWhiteList(),
                                            createReflectometryProcessor());
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // Calls that should throw
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::AppendGroupFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::DeleteGroupFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::GroupRowsFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::ExpandSelectionFlag));
    TS_ASSERT_THROWS_ANYTHING(
        presenter.notify(DataProcessorPresenter::PlotGroupFlag));
    TS_ASSERT_THROWS(presenter.getPostprocessedWorkspaceName(
                         std::map<int, std::vector<std::string>>()),
                     std::runtime_error);
  }

  void testPostprocessMap() {
    NiceMock<MockDataProcessorView> mockDataProcessorView;
    NiceMock<MockProgressableView> mockProgress;
    NiceMock<MockMainPresenter> mockMainPresenter;

    std::map<std::string, std::string> postprocesssMap = {{"dQ/Q", "Params"}};
    GenericDataProcessorPresenter presenter(
        createReflectometryWhiteList(), createReflectometryPreprocessMap(),
        createReflectometryProcessor(), createReflectometryPostprocessor(),
        postprocesssMap);
    presenter.acceptViews(&mockDataProcessorView, &mockProgress);
    presenter.accept(&mockMainPresenter);

    // Open a table
    createPrefilledWorkspace("TestWorkspace", presenter.getWhiteList());
    EXPECT_CALL(mockDataProcessorView, getWorkspaceToOpen())
        .Times(1)
        .WillRepeatedly(Return("TestWorkspace"));
    presenter.notify(DataProcessorPresenter::OpenTableFlag);

    createTOFWorkspace("12345", "12345");
    createTOFWorkspace("12346", "12346");

    std::set<int> grouplist;
    grouplist.insert(0);

    // We should not receive any errors
    EXPECT_CALL(mockMainPresenter, giveUserCritical(_, _)).Times(0);

    // The user hits the "process" button with the first group selected
    EXPECT_CALL(mockDataProcessorView, getSelectedChildren())
        .Times(1)
        .WillRepeatedly(Return(std::map<int, std::set<int>>()));
    EXPECT_CALL(mockDataProcessorView, getSelectedParents())
        .Times(1)
        .WillRepeatedly(Return(grouplist));
    EXPECT_CALL(mockMainPresenter, getPreprocessingOptions())
        .Times(2)
        .WillRepeatedly(Return(std::map<std::string, std::string>()));
    EXPECT_CALL(mockMainPresenter, getProcessingOptions())
        .Times(2)
        .WillRepeatedly(Return(""));
    EXPECT_CALL(mockMainPresenter, getPostprocessingOptions())
        .Times(1)
        .WillOnce(Return("Params='-0.10'"));
    EXPECT_CALL(mockDataProcessorView, getEnableNotebook())
        .Times(1)
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockDataProcessorView, requestNotebookPath()).Times(0);

    presenter.notify(DataProcessorPresenter::ProcessFlag);

    // Check output workspace was stitched with params = '-0.04'
    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("IvsQ_TOF_12345_TOF_12346"));

    MatrixWorkspace_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "IvsQ_TOF_12345_TOF_12346");
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[0], 0.100, 1e-5);
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[1], 0.104, 1e-5);
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[2], 0.10816, 1e-5);
    TSM_ASSERT_DELTA(
        "Logarithmic rebinning should have been applied, with param 0.04",
        out->x(0)[3], 0.11248, 1e-5);

    // Tidy up
    AnalysisDataService::Instance().remove("TestWorkspace");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12345");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12345");
    AnalysisDataService::Instance().remove("12345");
    AnalysisDataService::Instance().remove("IvsQ_binned_TOF_12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12346");
    AnalysisDataService::Instance().remove("IvsLam_TOF_12346");
    AnalysisDataService::Instance().remove("12346");
    AnalysisDataService::Instance().remove("IvsQ_TOF_12345_TOF_12346");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockDataProcessorView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockMainPresenter));
  }
};

#endif /* MANTID_MANTIDWIDGETS_GENERICDATAPROCESSORPRESENTERTEST_H */
