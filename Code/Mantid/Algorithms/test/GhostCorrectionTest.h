#ifndef GHOSTCORRECTIONTEST_H_
#define GHOSTCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/GhostCorrection.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAlgorithms/AlignDetectors.h"
#include "WorkspaceCreationHelper.hh"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;


class GhostCorrectionTest : public CxxTest::TestSuite
{
public:
  double BIN_DELTA;
  int NUMPIXELS, NUMBINS;

  GhostCorrectionTest()
  {
    BIN_DELTA = 2.0;
    NUMPIXELS = 20;
    NUMBINS = 50;
  }

  //--------------------------------------------------------------------------------------------------------
  void testBasics()
  {
    //12 bytes per record
    TS_ASSERT_EQUALS( sizeof(GhostDestinationValue), 12);

  }

  //--------------------------------------------------------------------------------------------------------
  void testBadInputs()
  {
    std::string wsName("dummy");
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateEventWorkspace(NUMBINS, NUMPIXELS);
    AnalysisDataService::Instance().add(wsName, test_in);

    GhostCorrection gc;
    TS_ASSERT_THROWS_NOTHING(gc.initialize() );

    //Not d-spacing units.
    TS_ASSERT_THROWS( gc.setPropertyValue("InputWorkspace",wsName), std::invalid_argument);

    AnalysisDataService::Instance().remove(wsName);
  }

  //--------------------------------------------------------------------------------------------------------
  void makeFakeEventWorkspace(std::string wsName)
  {
    //Make an event workspace with 2 events in each bin.
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateEventWorkspace(NUMPIXELS, NUMBINS, NUMBINS, 0.0, BIN_DELTA, 2);
    //Fake a d-spacing unit in the data.
    test_in->getAxis(0)->unit() =UnitFactory::Instance().create("dSpacing");
    //Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

  //--------------------------------------------------------------------------------------------------------
  /** Generate a fake ghost correction file **/
  void makeFakeGhostFile(std::string ghostFileName)
  {
    //Open new file
    std::ofstream * handle = new std::ofstream(ghostFileName.c_str(), std::ios::binary);

    GhostDestinationValue ghost;
    for (int pix=0; pix < NUMPIXELS; pix++)
      for (int g=0; g < 16; g++)
      {
        ghost.pixelId = g;
        ghost.weight = g * 1.0;
        //Write to file
        handle->write(reinterpret_cast<char*>(&ghost), sizeof(ghost));
      }
    handle->close();
    delete handle;
  }

  //--------------------------------------------------------------------------------------------------------
  /** Generate a fake ghost correction file **/
  void makeFakeGroupingFile(std::string groupingFile)
  {
    //Open new file, text mode
    std::ofstream * handle = new std::ofstream(groupingFile.c_str(), std::ios::out);
    *handle << "# Fake detector file\n";
    *handle << "# Format: number    UDET    offset    select    group\n";

    //Groups will range from 1 to 5.
    for (int pix=0; pix < NUMPIXELS; pix++)
    {
      *handle << pix << " " << pix << "  0.000   1  " << 1+(pix/4) << "\n";
    }
    handle->close();
    delete handle;
  }


  //--------------------------------------------------------------------------------------------------------
  void xtestExecDummy()
  {
    std::string wsName("dummy");
    std::string outwsName("ghost_correction");
    std::string ghostFileName("FakeGhostMapFile.dat");
    std::string groupingFile("FakeGroupingFile.cal");

    //Make up event workspace in d-spacing units
    this->makeFakeEventWorkspace(wsName);

    //Make the ghost file
    makeFakeGhostFile(ghostFileName);
    //And grouping file
    makeFakeGroupingFile(groupingFile);

    //Checks on the workspace
    EventWorkspace_const_sptr inputW = boost::dynamic_pointer_cast<const EventWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    TS_ASSERT_EQUALS( inputW->getNumberHistograms(), NUMPIXELS);
    IndexToIndexMap * m;
    m = inputW->getDetectorIDToWorkspaceIndexMap();
    TS_ASSERT_EQUALS( m->size(), NUMPIXELS);
    //2 events per bin
    TS_ASSERT_EQUALS( inputW->dataY(0)[0], 2);

    //----- Now do ghost correction ------
    GhostCorrection gc;
    gc.initialize();
    gc.setPropertyValue("InputWorkspace",wsName);
    gc.setPropertyValue("OutputWorkspace",outwsName);
    //Make the same bin parameters
    std::stringstream params;
    params << "0.0," << BIN_DELTA << "," << BIN_DELTA*NUMBINS;
    gc.setPropertyValue("BinParams", params.str());
    gc.setPropertyValue("GroupingFileName", groupingFile);
    gc.setPropertyValue("GhostCorrectionFileName", ghostFileName);

    TS_ASSERT(gc.execute());
    TS_ASSERT(gc.isExecuted());

    //Get the output workspace and check it
    Workspace2D_sptr outWS = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve(outwsName));
    TS_ASSERT( outWS );

    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 5 );
    for (int group=1; group < 6; group++)
    {
      int workspaceIndex = group-1;

      //The way the ghost weights are done will give this expected value
      double expected_value = 0;
      for (int i= (group-1)*4; i < group*4; i++)
        expected_value += i;
      //2 events per input WS bin
      expected_value *= 2.0;
      //And all of the 20 pixels add up in the same group - this is the focussing
      expected_value *= NUMPIXELS;
      //But group #5 is past the 16 ghost indices, so it is 0
      if (group==5) expected_value = 0;

      //Get the data
      MantidVec Y = outWS->dataY(workspaceIndex);
      TS_ASSERT_EQUALS( Y.size(), NUMBINS ); //Proper size
      for (int i=0; i<NUMBINS; i++)
        TS_ASSERT_EQUALS( Y[i], expected_value ); //Proper size
    }


    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(outwsName);
  }



  //--------------------------------------------------------------------------------------------------------
  void xtestExecPG3()
  {
    std::string wsName("pg3_732");
    std::string outwsName("ghost_correction");

    //----- Load some event data --------
    Mantid::DataHandling::LoadEventPreNeXus * eventLoader;
    eventLoader = new LoadEventPreNeXus();
    eventLoader->initialize();
    eventLoader->setPropertyValue("EventFilename", "../../../../Test/Data/sns_event_prenexus/PG3_732_neutron_event.dat");
//    eventLoader->setPropertyValue("MappingFilename", "../../../../Test/Data/sns_event_prenexus/CNCS_TS_2008_08_18.dat");
    eventLoader->setPropertyValue("OutputWorkspace", wsName);
    eventLoader->setMaxEventsToLoad(1000000);
    TS_ASSERT( eventLoader->execute() );

    std::string groupingFile("../../../../Test/Data/sns_event_prenexus/pg3_mantid_det.cal");

    //----- Now do ghost correction ------
    GhostCorrection gc;
    gc.initialize();
    gc.setPropertyValue("InputWorkspace",wsName);
    gc.setPropertyValue("OutputWorkspace",outwsName);
    gc.setPropertyValue("BinParams", "0.0, 0.1, 5.0");
    gc.setPropertyValue("GroupingFileName", groupingFile);
    gc.setPropertyValue("GhostCorrectionFileName", "../../../../Test/Data/sns_event_prenexus/PG3_D664_ghostmap_2010_03_17.dat");

    TS_ASSERT(gc.execute());
    TS_ASSERT(gc.isExecuted());

    //Get the output workspace and check it
    Workspace2D_sptr outWS = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve(outwsName));
    TS_ASSERT( outWS );

    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 4);
    MantidVec Y = outWS->dataY(1);
    TS_ASSERT_EQUALS( Y.size(), 50 ); //Proper size
    for (int i=0; i<50; i++)
      std::cout << i << ": " << Y[i] << "\n";


    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(outwsName);
  }



private:



};


#endif /* GHOSTCORRECTIONTEST_H_ */
