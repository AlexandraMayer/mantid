#ifndef LOADMUONNEXUS2TEST_H_
#define LOADMUONNEXUS2TEST_H_


// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
//

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadMuonNexus2.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
//using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMuonNexus2Test : public CxxTest::TestSuite
{
public:
  
  void testExec()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "../../../../Test/Nexus/Muon-v2/argus0026287.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 192 for file inputFile = "argus0026287.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 192);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(11)[686], 9);
    TS_ASSERT_EQUALS( output2D->dataY(12)[686], 7);
    TS_ASSERT_EQUALS( output2D->dataY(13)[686], 7);

    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(11)[686], 3);
    TS_ASSERT_DELTA( output2D->dataE(12)[686], 2.646,0.001);
    TS_ASSERT_DELTA( output2D->dataE(13)[686], 2.646,0.001);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.992,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output-> isDistribution() )

    /*
    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    IInstrument_sptr i = output->getInstrument();
    Mantid::Geometry::IObjComponent_sptr source = i->getSource();
    
    Mantid::Geometry::IObjComponent_sptr samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 10.0,0.01);


    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    Mantid::Geometry::Detector *ptrDet103 = dynamic_cast<Mantid::Geometry::Detector*>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
    */
    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    Property *l_property = output->sample().getLogData( std::string("temperature_1_log") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::map<dateAndTime, double> asMap = l_timeSeriesDouble ->valueAsMap();
    TS_ASSERT_EQUALS(l_timeSeriesDouble->size(),37);
    TS_ASSERT_EQUALS(l_timeSeriesDouble->nthValue(10),180.0);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,25), "2008-Sep-11 14:17:41  180" );
    //check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), "GaAs")
    
	
    //----------------------------------------------------------------------
    // Tests to check that Loading SpectraDetectorMap is done correctly
    //----------------------------------------------------------------------
    const SpectraDetectorMap& map = output->spectraMap();
    
    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(map.nElements(),192);
    
   // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(map.ndet(6),1);
    
    std::vector<int> detectorgroup = map.getDetectors(100);
    TS_ASSERT_EQUALS(detectorgroup.size(),1);
    TS_ASSERT_EQUALS(detectorgroup.front(),100);


    AnalysisDataService::Instance().remove(outputSpace);
   
  }

  void testMinMax()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "../../../../Test/Nexus/Muon-v2/argus0026287.nxs");
    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumMin","10");
    nxLoad.setPropertyValue("SpectrumMax","20");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 11);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(7)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(10).size() );
    // Check one particular value
    //TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);
    // Check that the error on that value is correct
    //TS_ASSERT_EQUALS( output2D->dataE(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    //TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testList()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "../../../../Test/Nexus/Muon-v2/argus0026287.nxs");
    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumList","1,10,20");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(0)) == (output2D->dataX(2)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(0).size(), output2D->dataY(1).size() );
    // Check one particular value
    //TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);
    // Check that the error on that value is correct
    //TS_ASSERT_EQUALS( output2D->dataE(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    //TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    AnalysisDataService::Instance().remove(outputSpace);
  }
  void testMinMax_List()
  {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "../../../../Test/Nexus/Muon-v2/argus0026287.nxs");
    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumMin","10");
    nxLoad.setPropertyValue("SpectrumMax","20");
    nxLoad.setPropertyValue("SpectrumList","30,40,50");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 14);
    TS_ASSERT_EQUALS( output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(7)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(10).size() );
    // Check one particular value
    //TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);
    // Check that the error on that value is correct
    //TS_ASSERT_EQUALS( output2D->dataE(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    //TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );

    AnalysisDataService::Instance().remove(outputSpace);
  }
};
#endif /*LOADMUONNEXUS2TEST_H_*/
