#ifndef LOADINSTRUMENTTEST_H_
#define LOADINSTRUMENTTEST_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "MantidAPI/ExperimentInfo.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstrumentTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    TS_ASSERT( !loader.isInitialized() );
    loader.initialize();
    TS_ASSERT( loader.isInitialized() );
  }

  void testExecHET()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestHET";
    int histogramNumber = 2584;
    int timechannels = 100;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    //loop to create data
    for (int i = 0; i < histogramNumber; i++)
    {
      boost::shared_ptr<Mantid::MantidVec> timeChannelsVec(new Mantid::MantidVec),v(new Mantid::MantidVec),e(new Mantid::MantidVec);
      timeChannelsVec->resize(timechannels);
      v->resize(timechannels);
      e->resize(timechannels);
      //timechannels
      for (int j = 0; j < timechannels; j++)
      {
        (*timeChannelsVec)[j] = j*100;
        (*v)[j] = (i+j)%256;
        (*e)[j] = (i+j)%78;
      }
      // Populate the workspace.
      ws2D->setX(i, timeChannelsVec);
      ws2D->setData(i, v, e);
    }
    ws2D->generateSpectraMap();

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
    // We want to test id the spectra mapping changes
    TS_ASSERT_EQUALS(ws2D->getSpectrum(0)->getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(ws2D->getSpectrum(256)->getSpectrumNo(), 257);
    TS_ASSERT_EQUALS(ws2D->getNumberHistograms(), 2584);
    
    loader.setPropertyValue("Filename", "HET_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    loader.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loader.execute());

    TS_ASSERT( loader.isExecuted() );

    TS_ASSERT_EQUALS ( loader.getPropertyValue("MonitorList"), "601,602,603,604" );

//    std::vector<detid_t> dets = ws2D->getInstrument()->getDetectorIDs();
//    std::cout << dets.size() << " detectors in the instrument" << std::endl;
//    for (size_t i=0; i<dets.size(); i++)
//    {
//      if (i % 10 == 0) std::cout << std::endl;
//      std::cout << dets[i] << ", ";
//    }

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<const Instrument> i = output->getInstrument()->baseInstrument();
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<const IDetector> ptrDet103 = i->getDetector(103);
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
    double d = ptrDet103->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.512,0.0001);
    double cmpDistance = ptrDet103->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.512,0.0001);

    // test if detector with det_id=603 has been marked as a monitor
    boost::shared_ptr<const IDetector> ptrMonitor = i->getDetector(601);
    TS_ASSERT( ptrMonitor->isMonitor() );

    // Spectra mapping has been updated
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(0), 1);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(255), 256);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(256), 257);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(257), 258);

    std::set<detid_t> ids_from_map = output->getSpectrum(257)->getDetectorIDs();
    IDetector_const_sptr det_from_ws = output->getDetector(257);
    TS_ASSERT_EQUALS(ids_from_map.size(), 1);
    TS_ASSERT_EQUALS(*ids_from_map.begin(), 602);
    TS_ASSERT_EQUALS(det_from_ws->getID(), 602);

    // also a few tests on the last detector and a test for the one beyond the last
    boost::shared_ptr<const IDetector> ptrDetLast = i->getDetector(413256);
    TS_ASSERT_EQUALS( ptrDetLast->getID(), 413256);
    TS_ASSERT_EQUALS( ptrDetLast->getName(), "pixel");
    TS_ASSERT_THROWS(i->getDetector(413257), Exception::NotFoundError);

    // Test input data is unchanged
    Workspace2D_sptr output2DInst = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584
    TS_ASSERT_EQUALS( output2DInst->getNumberHistograms(), histogramNumber);

    // Check running algorithm for same XML file leads to same instrument object being attached
    boost::shared_ptr<Instrument> instr(new Instrument());
    output->setInstrument(instr);
    TS_ASSERT_EQUALS( output->getInstrument()->baseInstrument(), instr );
    LoadInstrument loadAgain;
    TS_ASSERT_THROWS_NOTHING( loadAgain.initialize() );
    loadAgain.setPropertyValue("Filename", inputFile);
    loadAgain.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING( loadAgain.execute() );
    TS_ASSERT_EQUALS( output->getInstrument()->baseInstrument(), i );

    // Valid-from/to
    Kernel::DateAndTime validFrom("1900-01-31T23:59:59");
    Kernel::DateAndTime validTo("2100-01-31 23:59:59");
    TS_ASSERT_EQUALS( i->getValidFromDate(), validFrom);
    TS_ASSERT_EQUALS( i->getValidToDate(), validTo);

    AnalysisDataService::Instance().remove(wsName);
  }

  /** Check the GEM instrument */
  void evaluate_GEM(MatrixWorkspace_sptr output)
  {
    boost::shared_ptr<Instrument> i = output->getInstrument();
    boost::shared_ptr<const IObjComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -17.0,0.01);

    boost::shared_ptr<const IObjComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const IDetector> ptrDet =i->getDetector(101001);
    TS_ASSERT_EQUALS( ptrDet->getID(), 101001);
    TS_ASSERT_DELTA( ptrDet->getPos().X(),  0.2607, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Y(), -0.1505, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(),  2.3461, 0.0001);
    double d = ptrDet->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.3653,0.0001);
    double cmpDistance = ptrDet->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.3653,0.0001);

    // test if detector with det_id=621 has been marked as a monitor
    boost::shared_ptr<const IDetector> ptrMonitor = i->getDetector(621);
    TS_ASSERT( ptrMonitor->isMonitor() );

    // test if shape on for 1st monitor which is located at (0,0,-10.78)
    boost::shared_ptr<const IDetector> ptrMonitorShape = i->getDetector(611);
    TS_ASSERT( ptrMonitorShape->isMonitor() );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0.0,0.0,0.001)+ptrMonitorShape->getPos()) );
    TS_ASSERT( ptrMonitorShape->isValid(V3D(0.0,0.0,-0.01)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(0.0,0.0,-0.04)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(-2.1,-2.01,-2.01)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(100,100,100)+ptrMonitorShape->getPos()) );
    TS_ASSERT( !ptrMonitorShape->isValid(V3D(-200.0,-200.0,-2000.1)+ptrMonitorShape->getPos()) );

    // test of some detector...
    boost::shared_ptr<const IDetector> ptrDetShape = i->getDetector(101001);
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.0)+ptrDetShape->getPos()) );

    // Only one element in spectrum
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(0), 1);
    TS_ASSERT_EQUALS(output->getSpectrum(0)->getDetectorIDs().size(), 1);
  }

  void testExecGEM()
  {
    LoadInstrument loaderGEM;

    TS_ASSERT_THROWS_NOTHING(loaderGEM.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestGEM";
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    loaderGEM.setPropertyValue("Filename", "GEM_Definition.xml");
    inputFile = loaderGEM.getPropertyValue("Filename");

    loaderGEM.setPropertyValue("Workspace", wsName);
    loaderGEM.setPropertyValue("RewriteSpectraMap", "0"); //Do not overwrite the spectra map

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderGEM.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderGEM.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderGEM.execute());
    TS_ASSERT( loaderGEM.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));
    evaluate_GEM(output);
    AnalysisDataService::Instance().remove(wsName);


    // OK, now do it using a XML text.
    std::string xmlText, str;
    std::ifstream in;
    in.open(inputFile.c_str());
    getline(in,str);
    while ( in ) {
      xmlText += str + "\n";
      getline(in,str);  }

    // Re-add an empty WS
    ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    LoadInstrument alg;
    alg.initialize();
    alg.setPropertyValue("Filename", "GEM_Definition.xml"); // File won't be loaded
    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("XMLText", xmlText);
    alg.setPropertyValue("RewriteSpectraMap", "0"); //Do not overwrite the spectra map
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));
    // Check that it's the same output
    evaluate_GEM(output);
  }

  /* Manually load into an ExperimentInfo instead of a workspace */
  void test_execManually()
  {
    LoadInstrument alg;
    ExperimentInfo_sptr ei(new ExperimentInfo());
    alg.setParametersManually(ei, "", "GEM", "");
    TS_ASSERT_THROWS_NOTHING(alg.execManually(););
    TS_ASSERT_EQUALS ( ei->getInstrument()->getName(), "GEM");
  }

  void testExecSLS()
  {
    LoadInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestSLS";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderSLS.setPropertyValue("Filename", "SANDALS_Definition.xml");
    inputFile = loaderSLS.getPropertyValue("Filename");

    loaderSLS.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT( loaderSLS.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<const Instrument> i = output->getInstrument();
    boost::shared_ptr<const IObjComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -11.016,0.01);

    boost::shared_ptr<const IObjComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(101);
    TS_ASSERT_EQUALS( ptrDet->getID(), 101);

    boost::shared_ptr<const IDetector> ptrMonitor = i->getDetector(1);
    TS_ASSERT( ptrMonitor->isMonitor() );

    boost::shared_ptr<const IDetector> ptrDetShape = i->getDetector(102);
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.000001)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.005,0.1,0.000002)+ptrDetShape->getPos()) );


    // test of sample shape
    TS_ASSERT( samplepos->isValid(V3D(0.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(0.0,0.0,0.05)+samplepos->getPos()) );

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecNIMROD()
  {
    LoadInstrument loaderNIMROD;

    TS_ASSERT_THROWS_NOTHING(loaderNIMROD.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestNIMROD";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderNIMROD.setPropertyValue("Filename", "NIM_Definition.xml");
    inputFile = loaderNIMROD.getPropertyValue("Filename");

    loaderNIMROD.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderNIMROD.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderNIMROD.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderNIMROD.execute());

    TS_ASSERT( loaderNIMROD.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<const Instrument> i = output->getInstrument();

    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(20201001);
    TS_ASSERT_EQUALS( ptrDet->getName(), "det 1");
    TS_ASSERT_EQUALS( ptrDet->getID(), 20201001);
    TS_ASSERT_DELTA( ptrDet->getPos().X(),  -0.0909, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Y(), 0.3983, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(),  4.8888, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }


  void testExecHRP()
  {
    InstrumentDataService::Instance().remove("HRPD_Definition.xml");

    LoadInstrument loaderHRP;

    TS_ASSERT_THROWS_NOTHING(loaderHRP.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestHRPD";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderHRP.setPropertyValue("Filename", "HRPD_Definition.xml");
    inputFile = loaderHRP.getPropertyValue("Filename");

    loaderHRP.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderHRP.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderHRP.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderHRP.execute());

    TS_ASSERT( loaderHRP.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<const Instrument> i = output->getInstrument();

    boost::shared_ptr<const IDetector> ptrDetShape = i->getDetector(3100);
    TS_ASSERT_EQUALS( ptrDetShape->getName(), "Det0");

    // Test of backscattering detector
    TS_ASSERT( ptrDetShape->isValid(V3D(0.002,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(-0.002,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(0.003,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(-0.003,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(-0.0069,0.0227,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(-0.0071,0.0227,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(-0.0069,0.0227,0.000009)+ptrDetShape->getPos()) );
    TS_ASSERT( !ptrDetShape->isValid(V3D(-0.0069,0.0227,0.011)+ptrDetShape->getPos()) );

    // test if a dummy parameter has been read in
    boost::shared_ptr<const IComponent> comp = i->getComponentByName("bank_90degnew");
    TS_ASSERT_EQUALS( comp->getName(), "bank_90degnew");

    ParameterMap& paramMap = output->instrumentParameters();

    Parameter_sptr param = paramMap.getRecursive(&(*comp), "S", "fitting");
    const FitParameter& fitParam4 = param->value<FitParameter>();
    TS_ASSERT( fitParam4.getTie().compare("") == 0 );
    TS_ASSERT( fitParam4.getFunction().compare("BackToBackExponential") == 0 );

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecIDF_for_unit_testing() // IDF stands for Instrument Definition File
  {
    LoadInstrument loaderIDF;

    TS_ASSERT_THROWS_NOTHING(loaderIDF.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestIDF";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    loaderIDF.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml");
    inputFile = loaderIDF.getPropertyValue("Filename");

    loaderIDF.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderIDF.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderIDF.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderIDF.execute());

    TS_ASSERT( loaderIDF.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<const Instrument> i = output->getInstrument();
    boost::shared_ptr<const IObjComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -17.0,0.01);

    boost::shared_ptr<const IObjComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const IDetector> ptrDet1 = i->getDetector(1);
    TS_ASSERT_EQUALS( ptrDet1->getID(), 1);
    TS_ASSERT_DELTA( ptrDet1->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Y(), 10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Z(),  0.0, 0.0001);
    double d = ptrDet1->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    double cmpDistance = ptrDet1->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);

    boost::shared_ptr<const IDetector> ptrDet2 = i->getDetector(2);
    TS_ASSERT_EQUALS( ptrDet2->getID(), 2);
    TS_ASSERT_DELTA( ptrDet2->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Y(), -10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Z(),  0.0, 0.0001);
    d = ptrDet2->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    cmpDistance = ptrDet2->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);


    // test if detectors face sample
    TS_ASSERT( !ptrDet1->isValid(V3D(0.02,0.0,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(-0.02,0.0,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( ptrDet1->isValid(V3D(0.0,0.02,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,-0.02,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,0.0,0.02)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,0.0,-0.02)+ptrDet1->getPos()) );

    TS_ASSERT( !ptrDet2->isValid(V3D(0.02,0.0,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(-0.02,0.0,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.02,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( ptrDet2->isValid(V3D(0.0,-0.02,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.0,0.02)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.0,-0.02)+ptrDet2->getPos()) );

    boost::shared_ptr<const IDetector> ptrDet3 = i->getDetector(3);
    TS_ASSERT( !ptrDet3->isValid(V3D(0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(-0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,-0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( ptrDet3->isValid(V3D(0.0,0.0,0.02)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.0,-0.02)+ptrDet3->getPos()) );

    boost::shared_ptr<const IDetector> ptrDet4 = i->getDetector(4);
    TS_ASSERT( !ptrDet4->isValid(V3D(0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(-0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,-0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.0,0.02)+ptrDet4->getPos()) );
    TS_ASSERT( ptrDet4->isValid(V3D(0.0,0.0,-0.02)+ptrDet4->getPos()) );

    // test of facing as a sub-element of location
    boost::shared_ptr<const IDetector> ptrDet5 = i->getDetector(5);
    TS_ASSERT( !ptrDet5->isValid(V3D(0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( ptrDet5->isValid(V3D(-0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,-0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,0.02)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,-0.02)+ptrDet5->getPos()) );

    // test of infinite-cone.
    boost::shared_ptr<const IDetector> ptrDet6 = i->getDetector(6);
    TS_ASSERT( !ptrDet6->isValid(V3D(0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(-0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,-0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.0,0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-1.02)+ptrDet6->getPos()) );

    // test of (finite) cone.
    boost::shared_ptr<const IDetector> ptrDet7 = i->getDetector(7);
    TS_ASSERT( !ptrDet7->isValid(V3D(0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(-0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,-0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,0.02)+ptrDet7->getPos()) );
    TS_ASSERT( ptrDet7->isValid(V3D(0.0,0.0,-0.02)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,-1.02)+ptrDet7->getPos()) );

    // test of hexahedron.
    boost::shared_ptr<const IDetector> ptrDet8 = i->getDetector(8);
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.8,0.8,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.8,0.8,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,-0.02)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,2.02)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.5,0.5,0.1)+ptrDet8->getPos()) );

    // test for "cuboid-rotating-test".
    boost::shared_ptr<const IDetector> ptrDet10 = i->getDetector(10);
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,0.05)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,-0.05)+ptrDet10->getPos()) );
    boost::shared_ptr<const IDetector> ptrDet11 = i->getDetector(11);
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.0,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.0,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.01,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.01,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,0.05)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,-0.05)+ptrDet11->getPos()) );

    // test for "infinite-cylinder-test".
    boost::shared_ptr<const IDetector> ptrDet12 = i->getDetector(12);
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,-0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,-0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(-0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( !ptrDet12->isValid(V3D(2.0,0.0,0.0)+ptrDet12->getPos()) );

    // test for "finite-cylinder-test".
    boost::shared_ptr<const IDetector> ptrDet13 = i->getDetector(13);
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.1)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(0.0,0.0,-0.1)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,-0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(-0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(2.0,0.0,0.0)+ptrDet13->getPos()) );

    // test for "complement-test".
    boost::shared_ptr<const IDetector> ptrDet14 = i->getDetector(14);
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,-0.04)+ptrDet14->getPos()) );
    TS_ASSERT( ptrDet14->isValid(V3D(0.0,0.0,-0.06)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.04,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( ptrDet14->isValid(V3D(0.0,0.06,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.06,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.51,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.51,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,0.51)+ptrDet14->getPos()) );

    // test for "rotation-of-element-test".
    boost::shared_ptr<const IDetector> ptrDet15 = i->getDetector(15);
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,-0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(0.09,0.0,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(-0.09,0.0,0.01)+ptrDet15->getPos()) );
    boost::shared_ptr<const IDetector> ptrDet16 = i->getDetector(16);
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,0.09)+ptrDet16->getPos()) );
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,-0.09)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    boost::shared_ptr<const IDetector> ptrDet17 = i->getDetector(17);
    TS_ASSERT( ptrDet17->isValid(V3D(0.0,0.09,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( ptrDet17->isValid(V3D(0.0,-0.09,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( !ptrDet17->isValid(V3D(0.09,0.0,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( !ptrDet17->isValid(V3D(-0.09,0.0,0.01)+ptrDet17->getPos()) );

    // test of sample shape
    TS_ASSERT( samplepos->isValid(V3D(0.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(0.0,0.0,0.05)+samplepos->getPos()) );
    TS_ASSERT( samplepos->isValid(V3D(10.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(10.0,0.0,0.05)+samplepos->getPos()) );

    // test of source shape
    TS_ASSERT( source->isValid(V3D(0.0,0.0,0.005)+source->getPos()) );
    TS_ASSERT( !source->isValid(V3D(0.0,0.0,-0.005)+source->getPos()) );
    TS_ASSERT( !source->isValid(V3D(0.0,0.0,0.02)+source->getPos()) );

    // Check absence of distinct physical instrument
    TS_ASSERT( !i->getPhysicalInstrument() );

    AnalysisDataService::Instance().remove(wsName);
  }


  void testExecIDF_for_unit_testing2() // IDF stands for Instrument Definition File
  {
    LoadInstrument loaderIDF2;

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestIDF2";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Path to test input file assumes Test directory checked out from SVN
    loaderIDF2.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
    inputFile = loaderIDF2.getPropertyValue("Filename");

    loaderIDF2.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderIDF2.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderIDF2.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());

    TS_ASSERT( loaderIDF2.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<const Instrument> i = output->getInstrument();

    boost::shared_ptr<const IDetector> ptrDetShape = i->getDetector(1100);
    TS_ASSERT_EQUALS( ptrDetShape->getID(), 1100);

    // Test of monitor shape
    boost::shared_ptr<const IDetector> ptrMonShape = i->getDetector(1001);
    TS_ASSERT( ptrMonShape->isValid(V3D(0.002,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.002,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(0.003,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.003,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0069,0.0227,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0071,0.0227,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0069,0.0227,0.009)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0069,0.0227,0.011)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.1242,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0621,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,-0.0621,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0641,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0651,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0595,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0641,0.01)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0641,0.011)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0651,0.01)+ptrMonShape->getPos()) );

    AnalysisDataService::Instance().remove(wsName);
  }

    void testExec_RectangularDetector()
    {
      LoadInstrument loaderIDF2;
      loaderIDF2.initialize();
      //create a workspace with some sample data
      wsName = "RectangularDetector";
      Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
      Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
      //put this workspace in the data service
      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

      // Path to test input file assumes Test directory checked out from SVN
      loaderIDF2.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_RECTANGULAR_UNIT_TESTING.xml");
      inputFile = loaderIDF2.getPropertyValue("Filename");
      loaderIDF2.setPropertyValue("Workspace", wsName);
      loaderIDF2.execute();
      TS_ASSERT( loaderIDF2.isExecuted() );

      // Get back the saved workspace
      MatrixWorkspace_sptr output;
      TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));
      boost::shared_ptr<const Instrument> i = output->getInstrument();

      // Now the XY detector in bank1
      boost::shared_ptr<const RectangularDetector> bank1 = boost::dynamic_pointer_cast<const RectangularDetector>( i->getComponentByName("bank1") );
      TS_ASSERT( bank1 );
      if (!bank1) return;

      //Right # of x columns?
      TS_ASSERT_EQUALS( bank1->nelements(), 100);

      //Positions according to formula
      TS_ASSERT_DELTA( bank1->getAtXY(0,0)->getPos().X(), -0.1, 1e-4 );
      TS_ASSERT_DELTA( bank1->getAtXY(0,0)->getPos().Y(), -0.2, 1e-4 );
      TS_ASSERT_DELTA( bank1->getAtXY(1,0)->getPos().X(), -0.098, 1e-4 );
      TS_ASSERT_DELTA( bank1->getAtXY(1,1)->getPos().Y(), -0.198, 1e-4 );

      //Some IDs
      TS_ASSERT_EQUALS( bank1->getAtXY(0,0)->getID(), 1000);
      TS_ASSERT_EQUALS( bank1->getAtXY(0,1)->getID(), 1001);
      TS_ASSERT_EQUALS( bank1->getAtXY(1,0)->getID(), 1300);
      TS_ASSERT_EQUALS( bank1->getAtXY(1,1)->getID(), 1301);

      //The total number of detectors
      detid2det_map dets;
      i->getDetectors(dets);
      TS_ASSERT_EQUALS( dets.size(), 100*200 * 2);

      AnalysisDataService::Instance().remove(wsName);
  }

  void testNeutronicPositions()
  {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl& IDS = InstrumentDataService::Instance();
    IDS.clear();

    LoadInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/INDIRECT_Definition.xml");
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    loader.setProperty("Workspace", ws );
    TS_ASSERT( loader.execute() );

    // This kind of IDF should lead to 2 instrument definitions - the physical and the neutronic
    // But only 1 goes into the IDS (the neutronic instrument holds the physical instrument within itself)
    TS_ASSERT_EQUALS( IDS.size(), 1 );
    std::string name("INDIRECT_Definition.xml2011-08-25T12:00:00");
    TS_ASSERT( IDS.doesExist(name) );

    // Retrieve the neutronic instrument from the InstrumentDataService
    Instrument_const_sptr neutronicInst = IDS.retrieve(name);
    // And pull out a handle to the physical instrument from within the neutronic one
    Instrument_const_sptr physicalInst = neutronicInst->getPhysicalInstrument();
    // They should not be the same object
    TS_ASSERT_DIFFERS( physicalInst.get(), neutronicInst.get() );
    // Not true in general, but in this case we should not be getting a paramaterized instrument
    TS_ASSERT( ! physicalInst->isParametrized() );

    // Check the positions of the 6 detectors in the physical instrument
    TS_ASSERT_EQUALS( physicalInst->getDetector(1000)->getPos(), V3D(0,0,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1001)->getPos(), V3D(0,1,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1002)->getPos(), V3D(1,0,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1003)->getPos(), V3D(1,1,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1004)->getPos(), V3D(2,0,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1005)->getPos(), V3D(2,1,0) );

    // Check the right instrument ended up on the workspace
    TS_ASSERT_EQUALS( neutronicInst.get(), ws->getInstrument()->baseInstrument().get() );
    // Check the neutronic positions
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1000)->getPos(), V3D(2,2,0) );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1001)->getPos(), V3D(2,3,0) );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1002)->getPos(), V3D(3,2,0) );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1003)->getPos(), V3D(3,3,0) );
    // Note that one of the physical pixels doesn't exist in the neutronic space
    TS_ASSERT_THROWS( neutronicInst->getDetector(1004), Exception::NotFoundError );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1005)->getPos(), V3D(4,3,0) );

    // Check the monitor is in the same place in each instrument
    TS_ASSERT_EQUALS( physicalInst->getMonitor(1)->getPos(), neutronicInst->getMonitor(1)->getPos() );
    // ...but is not the same object
    TS_ASSERT_DIFFERS( physicalInst->getMonitor(1).get(), neutronicInst->getMonitor(1).get() );

    // Clean up
    IDS.clear();
  }


//
//    /** Compare the old and new SNAP instrument definitions **/
//    void xtestExecSNAPComparison_SLOW() // This test is slow!
//    {
//      LoadInstrument * loaderIDF2;
//      MatrixWorkspace_sptr output;
//      Workspace_sptr ws;
//      Workspace2D_sptr ws2D;
//
//      std::cout << "Loading the NEW snap geometry\n";
//      loaderIDF2 = new LoadInstrument();
//      loaderIDF2->initialize();
//      wsName = "SNAP_NEW";
//      ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
//      ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
//      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
//      loaderIDF2->setPropertyValue("Filename", "SNAP_Definition.xml");
//      inputFile = loaderIDF2->getPropertyValue("Filename");
//      loaderIDF2->setPropertyValue("Workspace", wsName);
//      loaderIDF2->execute();
//      TS_ASSERT( loaderIDF2->isExecuted() );
//      output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
//      boost::shared_ptr<Instrument> i_new = output->getInstrument();
//      TS_ASSERT_EQUALS( i_new->getName(), "SNAP");
//
//      TS_ASSERT_EQUALS( i_new->nelements(), 21);
//
//      std::cout << "Loading the OLD snap geometry\n";
//      loaderIDF2 = new LoadInstrument();
//      loaderIDF2->initialize();
//      wsName = "SNAP_OLD";
//      ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
//      ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
//      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
//      loaderIDF2->setPropertyValue("Filename", "SNAPOLD_Definition.xml");
//      inputFile = loaderIDF2->getPropertyValue("Filename");
//      loaderIDF2->setPropertyValue("Workspace", wsName);
//      loaderIDF2->execute();
//      TS_ASSERT( loaderIDF2->isExecuted() );
//      output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
//      boost::shared_ptr<Instrument> i_old = output->getInstrument();
//      TS_ASSERT_EQUALS( i_old->getName(), "SNAPOLD");
//
//      std::cout << "Comparing\n";
//
//      TS_ASSERT_EQUALS( i_new->nelements(), i_old->nelements());
//
//      //Compare the list of detectors
//      std::map<int, Geometry::IDetector_sptr> bank_new = i_new->getDetectors();
//      std::map<int, Geometry::IDetector_sptr> bank_old = i_old->getDetectors();
//      TS_ASSERT_EQUALS( bank_new.size(), bank_old.size());
//      TS_ASSERT_EQUALS( bank_new.size(), 65536*18 + 1); //Plus one for the monitor
//
//      std::map<int, Geometry::IDetector_sptr>::iterator it;
//      int count = 0;
//      for (it = bank_new.begin(); it != bank_new.end(); it++)
//      {
//        count++;
//        Geometry::IDetector_sptr det_new = it->second;
//        Geometry::IDetector_sptr det_old = bank_old[it->first];
//        //Compare their positions
//        TS_ASSERT_EQUALS( det_new->getPos(), det_old->getPos() );
//      }
//
//      TS_ASSERT_LESS_THAN( 65536*18, count);
//
//
//    }


//    /** Compare the old and new PG3 instrument definitions **/
//    void xtestExecPG3Comparison_SLOW() // This test is slow!
//    {
//      LoadInstrument * loaderIDF2;
//      MatrixWorkspace_sptr output;
//      Workspace_sptr ws;
//      Workspace2D_sptr ws2D;
//
//      std::cout << "Loading the NEW geometry\n";
//      loaderIDF2 = new LoadInstrument();
//      loaderIDF2->initialize();
//      wsName = "PG3_NEW";
//      ws = WorkspaceFactory::Instance().create("Workspace2D",1,2,1);
//      ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
//      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
//      loaderIDF2->setPropertyValue("Filename", "PG3_Definition.xml");
//      inputFile = loaderIDF2->getPropertyValue("Filename");
//      loaderIDF2->setPropertyValue("Workspace", wsName);
//      loaderIDF2->execute();
//      TS_ASSERT( loaderIDF2->isExecuted() );
//      output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
//      boost::shared_ptr<Instrument> i_new = output->getInstrument();
//      TS_ASSERT_EQUALS( i_new->getName(), "PG3");
//
//      std::cout << "Loading the OLD geometry\n";
//      loaderIDF2 = new LoadInstrument();
//      loaderIDF2->initialize();
//      wsName = "PG3_OLD";
//      ws = WorkspaceFactory::Instance().create("Workspace2D",1,2,1);
//      ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
//      TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
//      loaderIDF2->setPropertyValue("Filename", "PG3OLD_Definition.xml");
//      inputFile = loaderIDF2->getPropertyValue("Filename");
//      loaderIDF2->setPropertyValue("Workspace", wsName);
//      loaderIDF2->execute();
//      TS_ASSERT( loaderIDF2->isExecuted() );
//      output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
//      boost::shared_ptr<Instrument> i_old = output->getInstrument();
//      TS_ASSERT_EQUALS( i_old->getName(), "PG3OLD");
//
//      std::cout << "Comparing #\n";
//
//      TS_ASSERT_EQUALS( i_new->nelements(), i_old->nelements());
//
//
//      std::cout << "Comparing banks\n";
//
//      //Compare the list of detectors
//      std::map<int, Geometry::IDetector_sptr> bank_new = i_new->getDetectors();
//      std::map<int, Geometry::IDetector_sptr> bank_old = i_old->getDetectors();
//      TS_ASSERT_EQUALS( bank_new.size(), bank_old.size());
//
//      std::cout << "Comparing detectors\n";
//
//      std::map<int, Geometry::IDetector_sptr>::iterator it;
//      int count = 0;
//      for (it = bank_new.begin(); it != bank_new.end(); it++)
//      {
//        count++;
//        Geometry::IDetector_sptr det_new = it->second;
//        Geometry::IDetector_sptr det_old = bank_old[it->first];
//        if (det_new && det_old)
//        {
//          //Compare their positions
//          TS_ASSERT_EQUALS( det_new->getPos(), det_old->getPos() );
//        }
//        else
//        {
//          std::cout << "Detector at "<< count << " was not initialized.\n";
//        }
//      }
//    }





private:
  LoadInstrument loader;
  std::string inputFile;
  std::string wsName;

};



class LoadInstrumentTestPerformance : public CxxTest::TestSuite
{
public:
  MatrixWorkspace_sptr ws;

  void setUp()
  {
    ws = WorkspaceCreationHelper::Create2DWorkspace(1,2);
  }

  void doTest(std::string filename, size_t numTimes = 1)
  {
    for (size_t i=0; i < numTimes; ++i)
    {
      // Remove any existing instruments, so each time they are loaded.
      InstrumentDataService::Instance().clear();
      // Load it fresh
      LoadInstrument loader;
      loader.initialize();
      loader.setProperty("Workspace", ws);
      loader.setPropertyValue("Filename", filename);
      loader.execute();
      TS_ASSERT( loader.isExecuted() );
    }
  }

  void test_GEM()
  {
    doTest("GEM_Definition.xml", 10);
  }

  void test_WISH()
  {
    doTest("WISH_Definition.xml", 1);
  }

  void test_BASIS()
  {
    doTest("BASIS_Definition.xml", 5);
  }

  void test_CNCS()
  {
    doTest("CNCS_Definition.xml", 5);
  }

  void test_SEQUOIA()
  {
    doTest("SEQUOIA_Definition.xml", 5);
  }

  void test_POWGEN_2011()
  {
    doTest("POWGEN_Definition_2011-02-25.xml", 10);
  }

  void test_TOPAZ_2010()
  {
    doTest("TOPAZ_Definition_2010.xml", 1);
  }

  void test_TOPAZ_2011()
  {
    doTest("TOPAZ_Definition_2011-01-01.xml", 1);
  }

  void test_SNAP()
  {
    doTest("SNAP_Definition.xml", 1);
  }

};


#endif /*LOADINSTRUMENTTEST_H_*/

