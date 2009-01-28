#ifndef ManagedRawFileWorkspace2DTEST_H_
#define ManagedRawFileWorkspace2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include <iostream>
#include "MantidKernel/ConfigService.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace std;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class ManagedRawFileWorkspace2DTest : public CxxTest::TestSuite
{
public:
  ManagedRawFileWorkspace2DTest()
  {
  }
  
  void testWrongInit()
  {
    ManagedRawFileWorkspace2D ws;
    ws.setTitle("testInit");
    TS_ASSERT_THROWS_NOTHING( ws.initialize(5,5,5) )
    TS_ASSERT_EQUALS( ws.getNumberHistograms(), 5 )
    TS_ASSERT_EQUALS( ws.blocksize(), 5 )
    TS_ASSERT_EQUALS( ws.size(), 25 )
    
    TS_ASSERT_THROWS( ws.dataX(0), std::runtime_error )
  }

  void testSetFile()
  {
    ManagedRawFileWorkspace2D ws;
    ws.setRawFile("../../../../Test/Data/HET15869.RAW",2);

    TS_ASSERT_EQUALS( ws.getNumberHistograms(), 2584 )
    TS_ASSERT_EQUALS( ws.blocksize(), 1675 )
    TS_ASSERT_EQUALS( ws.size(), 4328200 )
    
    TS_ASSERT_THROWS_NOTHING( ws.readX(0) )
  }

  void testCast()
  {
    ManagedRawFileWorkspace2D *ws = new ManagedRawFileWorkspace2D;
    TS_ASSERT( dynamic_cast<ManagedWorkspace2D*>(ws) )
    TS_ASSERT( dynamic_cast<Workspace2D*>(ws) )
    TS_ASSERT( dynamic_cast<Mantid::API::Workspace*>(ws) )
  }

  void testId()
  {
    TS_ASSERT( ! Workspace.id().compare("Workspace2D") )
  }

  void testData()
  {
    ManagedRawFileWorkspace2D ws;
    ws.setRawFile("../../../../Test/Data/HET15869.RAW");

    const std::vector<double>& x0 = ws.readX(0);
    TS_ASSERT_EQUALS( x0[0], 5. )
    TS_ASSERT_EQUALS( x0[10], 7.5 )
    const std::vector<double>& x100 = ws.readX(100);
    TS_ASSERT_EQUALS( x100[0], 5. )
    TS_ASSERT_EQUALS( x100[10], 7.5 )

    const std::vector<double>& y0 = ws.readY(0);
    TS_ASSERT_EQUALS( y0[0], 0. )
    TS_ASSERT_EQUALS( y0[10], 1. )
    const std::vector<double>& y100 = ws.readY(100);
    TS_ASSERT_EQUALS( y100[0], 1. )
    TS_ASSERT_EQUALS( y100[10], 1. )

  }

  void testChanges()
  {
    ManagedRawFileWorkspace2D ws;
    ws.setRawFile("../../../../Test/Data/HET15869.RAW");

    std::vector<double>& y0 = ws.dataY(0);
    double oldValue0 = y0[100];
    y0[100] = 1234.;

    std::vector<double>& y1000 = ws.dataY(1000);
    double oldValue1000 = y1000[200];
    y1000[200] = 4321.;

    TS_ASSERT_EQUALS( ws.dataY(0)[100], 1234. )
    TS_ASSERT_EQUALS( ws.dataY(1000)[200], 4321. )
    TS_ASSERT_EQUALS( ws.readY(0)[100], 1234. )
    TS_ASSERT_EQUALS( ws.readY(1000)[200], 4321. )

  }

  // Test is taken from LoadRawTest
  void testLoadRaw2()
  {
    
    ConfigService::Instance().loadConfig("UseManagedWS.properties");
    LoadRaw2 loader;
    if ( !loader.isInitialized() ) loader.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader.execute(),std::runtime_error);

    std::string inputFile = "../../../../Test/Data/HET15869.RAW";
    // Now set it...
    loader.setPropertyValue("Filename", inputFile);

    std::string outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT(boost::dynamic_pointer_cast<ManagedRawFileWorkspace2D>(output2D) )
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 2584);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(99)) == (output2D->dataX(1734)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(673).size(), output2D->dataY(2111).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(999)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(999)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataX(999)[777], 554.1875);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output2D-> isDistribution() )

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output2D->getSample()->getProtonCharge(), 171.0353, 0.0001 )

    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<IInstrument> i = output2D->getInstrument();
    boost::shared_ptr<Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::Detector> ptrDet103 = boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<Sample> sample = output2D->getSample();
    Property *l_property = sample->getLogData( std::string("../../../../Test/Data/HET15869_TEMP1.txt") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );

    //----------------------------------------------------------------------
    // Tests to check that Loading SpectraDetectorMap is done correctly
    //----------------------------------------------------------------------
    boost::shared_ptr<SpectraDetectorMap> map= output2D->getSpectraMap();

    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(map->nElements(),24964);

    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(map->ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra 2084
    TS_ASSERT_EQUALS(map->ndet(2084),10);
    // Check the id number of all pixels contributing
    std::vector<int> detectorgroup;
    detectorgroup=map->getDetectors(2084);
    std::vector<int>::const_iterator it;
    int pixnum=101191;
    for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
    TS_ASSERT_EQUALS(*it,pixnum++);

    // Test with spectra that does not exist
    // Test that number of pixel=0
    TS_ASSERT_EQUALS(map->ndet(5),0);
    // Test that trying to get the Detector throws.
    std::vector<int> test = map->getDetectors(5);
    TS_ASSERT(test.empty());
    
    ConfigService::Instance().loadConfig("MantidTest.properties");
  }

private:
  ManagedRawFileWorkspace2D Workspace;
};

#endif /*ManagedRawFileWorkspace2DTEST_H_*/
