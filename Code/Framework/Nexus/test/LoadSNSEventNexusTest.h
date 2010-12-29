#ifndef LOADSNSEVENTNEXUSTEST_H_
#define LOADSNSEVENTNEXUSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>
#include "MantidNexus/LoadSNSEventNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidNexus/LoadLogsFromSNSNexus.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
//using namespace Mantid::DataObjects;


class LoadSNSEventNexusTest : public CxxTest::TestSuite
{
public:


  void testExec()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadSNSEventNexus ld;
        std::string outws_name = "cncs";
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7860_event.nxs");
        ld.setPropertyValue("OutputWorkspace",outws_name);
        ld.setPropertyValue("FilterByTof_Min", "-1e6");
        ld.setPropertyValue("FilterByTof_Max", "1e6");
        ld.setPropertyValue("FilterByTime_Start", "-1e6");
        ld.setPropertyValue("FilterByTime_Stop", "1e6");

        ld.execute();
        TS_ASSERT( ld.isExecuted() );

        DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
        //Valid WS and it is an EventWorkspace
        TS_ASSERT( WS );
        //Pixels have to be padded
        TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
        //Events
        TS_ASSERT_EQUALS( WS->getNumberEvents(), 112266);
        //TOF limits found. There is a pad of +-1 given around the actual TOF founds.
        TS_ASSERT_DELTA( (*WS->refX(0))[0],  44162.6, 0.05);
        TS_ASSERT_DELTA( (*WS->refX(0))[1],  60830.2, 0.05);

        //Check one event from one pixel - does it have a reasonable pulse time
        TS_ASSERT( WS->getEventListPtr(1000)->getEvents()[0].pulseTime() > DateAndTime(int64_t(1e9*365*10)) );

        //Longer, more thorough test
        if (false)
        {
          IAlgorithm_sptr load =  AlgorithmManager::Instance().create("LoadEventPreNeXus", 1);
          load->setPropertyValue("OutputWorkspace", "cncs_pre");
          load->setPropertyValue("EventFilename","../../../../Test/AutoTestData/CNCS_7860_neutron_event.dat");
          load->setPropertyValue("PulseidFilename","../../../../Test/AutoTestData/CNCS_7860_pulseid.dat");
          load->setPropertyValue("MappingFilename","../../../../Test/AutoTestData/CNCS_TS_2008_08_18.dat");
          load->setPropertyValue("PadEmptyPixels","1");
          load->execute();
          TS_ASSERT( load->isExecuted() );
          DataObjects::EventWorkspace_sptr WS2 = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("cncs_pre"));
          //Valid WS and it is an EventWorkspace
          TS_ASSERT( WS2 );

          //Let's compare the proton_charge logs
          Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( WS->mutableRun().getProperty("proton_charge") );
          std::map<DateAndTime, double> logMap = log->valueAsCorrectMap();
          Kernel::TimeSeriesProperty<double> * log2 = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( WS2->mutableRun().getProperty("proton_charge") );
          std::map<DateAndTime, double> logMap2 = log2->valueAsCorrectMap();
          std::map<DateAndTime, double>::iterator it, it2;

          it = logMap.begin();
          it2 = logMap2.begin();
          Kernel::DateAndTime start = it->first;
          for (; it != logMap.end(); )
          {
            //Same times within a millisecond
            //TS_ASSERT_DELTA( it->first, it2->first, DateAndTime::duration_from_seconds(1e-3));
            //Same times?
            TS_ASSERT_LESS_THAN( fabs(DateAndTime::seconds_from_duration(it->first - it2->first)), 1); //TODO: Fix the nexus file times here
            //Same proton charge?
            TS_ASSERT_DELTA( it->second, it2->second, 1e-5);
            it++;
            it2++;
          }

          int pixelID = 2000;

          std::vector<TofEvent> events1 = WS->getEventListPtr(pixelID)->getEvents();
          std::vector<TofEvent> events2 = WS2->getEventListPtr(pixelID)->getEvents();

          //std::cout << events1.size() << std::endl;
          TS_ASSERT_EQUALS( events1.size(), events2.size());
          if (events1.size() == events2.size())
          {
            for (int i=0; i < events1.size(); i++)
            {
              TS_ASSERT_DELTA( events1[i].tof(), events2[i].tof(), 0.05);
              TS_ASSERT_DELTA( events1[i].pulseTime(), events2[i].pulseTime(), 1e9); //TODO:: Fix nexus start times
              //std::cout << (events1[i].pulseTime()-start)/1e9 << " - " << (events2[i].pulseTime()-start)/1e9 << " sec\n";
              //std::cout << "Pulse time diff " << (events1[i].pulseTime() - events2[i].pulseTime())/1e9 << " sec\n";
            }
          }

        }


    }

    void testExec_Filtered()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadSNSEventNexus ld;
        std::string outws_name = "cncs";
        ld.initialize();
        ld.setPropertyValue("OutputWorkspace",outws_name);
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7860_event.nxs");
        ld.setPropertyValue("FilterByTime_Start", "60.0");
        ld.setPropertyValue("FilterByTime_Stop", "120.0");
        ld.setPropertyValue("FilterByTof_Min", "-1e10");
        ld.setPropertyValue("FilterByTof_Max", "1e10");

        ld.execute();
        TS_ASSERT( ld.isExecuted() );

        DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
        //Valid WS and it is an EventWorkspace
        TS_ASSERT( WS );
        //Pixels have to be padded
        TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
        //Events
        TS_ASSERT_EQUALS( WS->getNumberEvents(), 29753);

        //Check one event from one pixel - does it have a reasonable pulse time
        TS_ASSERT( WS->getEventListPtr(7)->getEvents()[0].pulseTime() > DateAndTime(int64_t(1e9*365*10)) );
    }

    void testExec_Monitors()
    {
      Mantid::API::FrameworkManager::Instance();
      LoadSNSEventNexus ld;
      std::string outws_name = "cncs";
      ld.initialize();
      ld.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7860_event.nxs");
      ld.setPropertyValue("OutputWorkspace",outws_name);
      ld.setProperty<bool>("LoadMonitors", true);

      ld.execute();
      TS_ASSERT( ld.isExecuted() );

      std::string mon_outws_name = outws_name + "_monitors";
      MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(mon_outws_name));
      //Valid WS and it is an MatrixWorkspace
      TS_ASSERT( WS );
      //Correct number of monitors found
      TS_ASSERT_EQUALS( WS->getNumberHistograms(), 3 );
      // Check some histogram data
      // TOF
      TS_ASSERT_EQUALS( (*WS->refX(0)).size(), 200002 );
      TS_ASSERT_DELTA( (*WS->refX(0))[1], 1.0, 1e-6 );
      // Data
      TS_ASSERT_EQUALS( WS->dataY(0).size(), 200001 );
      TS_ASSERT_DELTA( WS->dataY(0)[12], 0.0, 1e-6 );
      // Error
      TS_ASSERT_EQUALS( WS->dataE(0).size(), 200001 );
      TS_ASSERT_DELTA( WS->dataE(0)[12], 0.0, 1e-6 );
      // Check geometry for a monitor
      IDetector_sptr mon = WS->getDetector(2);
      TS_ASSERT( mon->isMonitor() );
      TS_ASSERT_EQUALS( mon->getID(), -3 );
      boost::shared_ptr<IComponent> sample = WS->getInstrument()->getSample();
      TS_ASSERT_DELTA( mon->getDistance(*sample), 1.426, 1e-6 );
    }

//    void testExec_TOPAZ()
//    {
//      Mantid::API::FrameworkManager::Instance();
//      LoadSNSEventNexus ld;
//      std::string outws_name = "TOPAZ_1715";
//      ld.initialize();
//      ld.setPropertyValue("Filename","/home/8oz/data/TOPAZ_1715_event.nxs");
//      ld.setPropertyValue("OutputWorkspace",outws_name);
//      ld.setPropertyValue("FilterByTof_Min", "-1e10");
//      ld.setPropertyValue("FilterByTof_Max", "1e10");
//      ld.setPropertyValue("FilterByTime_Start", "-1e10");
//      ld.setPropertyValue("FilterByTime_Stop", "1e10");
//
//      ld.execute();
//      TS_ASSERT( ld.isExecuted() );
//
//      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
//      //Valid WS and it is an EventWorkspace
//      TS_ASSERT( WS );
//      //Pixels have to be padded
//      TS_ASSERT_EQUALS( WS->getNumberHistograms(), 65536 * 15);
//      TS_ASSERT_EQUALS( WS->getNumberEvents(), 2942639);
//    }


};


#endif /*LOADSNSEVENTNEXUSTEST_H_*/



