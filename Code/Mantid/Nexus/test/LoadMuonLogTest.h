#ifndef LOADMUONLOGTEST_H_
#define LOADMUONLOGTEST_H_
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataHandling/LoadInstrument.h"
//

#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadMuonLog.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Component.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

class LoadMuonLogTest : public CxxTest::TestSuite
{
public:

  LoadMuonLogTest()
  {
	  //initialise framework manager to allow logging
	//Mantid::API::FrameworkManager::Instance().initialize();
  }
  void testInit()
  {
    TS_ASSERT( !loader.isInitialized() );
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT( loader.isInitialized() );
  }


  void testExecWithNexusDatafile()
  {
    //if ( !loader.isInitialized() ) loader.initialize();

    LoadMuonLog loaderNexusFile;
    loaderNexusFile.initialize();

	  // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    loaderNexusFile.setPropertyValue("Filename", inputFile);

    outputSpace = "LoadMuonLogTest-nexusdatafile";
    loaderNexusFile.setPropertyValue("Workspace", outputSpace);
    // Create an empty workspace and put it in the AnalysisDataService
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace1D");

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(outputSpace, ws));

	  std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderNexusFile.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderNexusFile.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(outputSpace));


    TS_ASSERT_THROWS_NOTHING(loaderNexusFile.execute());

    TS_ASSERT( loaderNexusFile.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));

    boost::shared_ptr<Sample> sample = output->getSample();

    // obtain the expected log data which was read from the Nexus file (NXlog)

    Property *l_property = sample->getLogData( std::string("BEAMLOG_CURRENT") );
    TimeSeriesProperty<double> *l_timeSeriesDouble1 = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble1->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,27), "2006-Nov-21 07:03:08  182.8" );

    l_property = sample->getLogData( std::string("BEAMLOG_FREQ") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
	TS_ASSERT_EQUALS( timeSeriesString.substr(0,24), "2006-Nov-21 07:03:08  50" );

  }

private:
  LoadMuonLog loader;
  std::string inputFile;
  std::string outputSpace;
  std::string inputSpace;

};

#endif /*LOADMUONLOGTEST_H_*/
