#ifndef MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertCWPDMDToSpectra.h"
#include "MantidDataHandling/LoadSpiceAscii.h"
#include "MantidMDAlgorithms/ConvertSpiceDataToRealSpace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"

using Mantid::MDAlgorithms::ConvertCWPDMDToSpectra;
using Mantid::DataHandling::LoadSpiceAscii;
using Mantid::MDAlgorithms::ConvertSpiceDataToRealSpace;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class ConvertCWPDMDToSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWPDMDToSpectraTest *createSuite() {
    return new ConvertCWPDMDToSpectraTest();
  }
  static void destroySuite(ConvertCWPDMDToSpectraTest *suite) { delete suite; }

  void test_Init() {
    ConvertCWPDMDToSpectra alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    // Create test workspaces
    createTestWorkspaces();
  }

  /** Unit test to reduce/bin the HB2A data
   * @brief test_ReduceHB2AData
   */
  void test_ReduceHB2AData() {
    // Init
    ConvertCWPDMDToSpectra alg;
    alg.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_dataMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputMonitorWorkspace", m_monitorMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParams", "0, 0.05, 120."));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LinearInterpolateZeroCounts", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ReducedData"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get ouput
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("ReducedData"));
    TS_ASSERT(outws);

    // Check output
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 1);

    const Mantid::MantidVec &vecX = outws->readX(0);
    TS_ASSERT_DELTA(vecX.front(), 0.0, 0.0001);
    TS_ASSERT_DELTA(vecX.back(), 120.0 - 0.05, 0.0001);

    // Sample logs: temperature
    TimeSeriesProperty<double> *tempbseries =
        dynamic_cast<TimeSeriesProperty<double> *>(
            outws->run().getProperty("temp_b"));
    TS_ASSERT(tempbseries);
    TS_ASSERT_EQUALS(tempbseries->size(), 61);
    DateAndTime t0 = tempbseries->nthTime(0);
    DateAndTime t3 = tempbseries->nthTime(3);
    TS_ASSERT_EQUALS(
        (t3.totalNanoseconds() - t0.totalNanoseconds()) / 1000000000, 90);

    // Clean
    AnalysisDataService::Instance().remove("ReducedData");
  }

  /** Unit test to reduce/bin the HB2A data with more options
   * @brief test_ReduceHB2AData
   */
  void Xtest_ReduceHB2ADataMoreOptions() {
    // Init
    ConvertCWPDMDToSpectra alg;
    alg.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_dataMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputMonitorWorkspace", m_monitorMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParams", "0, 0.05, 120."));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LinearInterpolateZeroCounts", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactor", 10.0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("UnitOutput", "Momenum Transfer (Q)"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ReducedData"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get ouput
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("ReducedData"));
    TS_ASSERT(outws);

    // Clean
    AnalysisDataService::Instance().remove("ReducedData");
  }

  void test_Clean() {
    AnalysisDataService::Instance().remove(m_dataMD->name());
    AnalysisDataService::Instance().remove(m_monitorMD->name());
  }

  /** Create workspaces for testing
   * @brief createTestWorkspaces
   */
  void createTestWorkspaces() {
    LoadSpiceAscii spcloader;
    spcloader.initialize();

    // Load HB2A spice file
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("Filename", "HB2A_exp0231_scan0001.dat"));
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("OutputWorkspace", "DataTable"));
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("RunInfoWorkspace", "LogParentWS"));
    TS_ASSERT_THROWS_NOTHING(spcloader.setPropertyValue(
        "DateAndTimeLog", "date,MM/DD/YYYY,time,HH:MM:SS AM"));
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("IgnoreUnlistedLogs", false));
    spcloader.execute();

    // Retrieve the workspaces as the inputs of ConvertSpiceDataToRealSpace
    ITableWorkspace_sptr datatablews =
        boost::dynamic_pointer_cast<ITableWorkspace>(
            AnalysisDataService::Instance().retrieve("DataTable"));
    TS_ASSERT(datatablews);

    MatrixWorkspace_sptr parentlogws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("LogParentWS"));
    TS_ASSERT(parentlogws);

    // Set up ConvertSpiceDataToRealSpace
    ConvertSpiceDataToRealSpace loader;
    loader.initialize();

    loader.setProperty("InputWorkspace", datatablews);
    loader.setProperty("RunInfoWorkspace", parentlogws);
    loader.setProperty("Instrument", "HB2A");
    loader.setPropertyValue("OutputWorkspace", "HB2A_MD");
    loader.setPropertyValue("OutputMonitorWorkspace", "MonitorMDW");

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get on hold of MDWorkspaces for test
    m_dataMD = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve("HB2A_MD"));
    m_monitorMD = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve("HB2A_MD"));
    TS_ASSERT(m_dataMD);
    TS_ASSERT(m_monitorMD);

    // Clean
    AnalysisDataService::Instance().remove(datatablews->name());
    AnalysisDataService::Instance().remove(parentlogws->name());
  }

private:
  IMDEventWorkspace_sptr m_dataMD;
  IMDEventWorkspace_sptr m_monitorMD;
};

#endif /* MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_ */
