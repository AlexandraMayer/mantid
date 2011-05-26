#ifndef LOADDETECTORINFOTEST_H_
#define LOADDETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <Poco/Path.h>
#include <Poco/File.h>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

/* choose an instrument to test, we could test all instruments
 * every time but I think a detailed test on the smallest workspace
 * is enough as the other workspaces take a long time to process (Steve Williams)
 */

//MARI
static const std::string RAWFILE =  "MAR11015.raw";
static const double TIMEOFF  =      3.9;
static const int MONITOR =          2;  
static const int NUMRANDOM =        7;
static const int DETECTS[NUMRANDOM]={4101,4804,1323,1101,3805,1323,3832};

class LoadDetectorInfoTest : public CxxTest::TestSuite
{



public:
  static LoadDetectorInfoTest *createSuite() { return new LoadDetectorInfoTest(); }
  static void destroySuite(LoadDetectorInfoTest *suite) { delete suite; }

  void testLoadDat()
  {// also tests changing X-values with   -same bins, different offsets

    LoadDetectorInfo grouper;

    TS_ASSERT_EQUALS( grouper.name(), "LoadDetectorInfo" );
    TS_ASSERT_EQUALS( grouper.version(), 1 );
    TS_ASSERT_EQUALS( grouper.category(), "DataHandling\\Detectors" );
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() );
    TS_ASSERT( grouper.isInitialized() );

    // Set up a small workspace for testing
    makeSmallWS();
    grouper.setPropertyValue("Workspace", m_InoutWS);
    grouper.setPropertyValue("DataFilename", m_DatFile);
    grouper.setPropertyValue("RelocateDets", "1");

    grouper.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_const_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(m_InoutWS));

    const ParameterMap& pmap = WS->instrumentParameters();

    for ( int j = 0; j < NDETECTS; ++j)
    {

      boost::shared_ptr<IDetector> detector =WS->getInstrument()->getDetector(j);
      boost::shared_ptr<IComponent> comp =
          boost::dynamic_pointer_cast<IComponent>(detector);

      const IComponent* baseComp = detector->getComponent();

      Parameter_sptr par = pmap.get(baseComp,"3He(atm)");
      // this is only for PSD detectors, code 3
      if ( code[j] == "3" )
      {
        TS_ASSERT(par);
        TS_ASSERT_EQUALS(par->asString(), castaround(pressure[j]));
        par = pmap.get(baseComp,"wallT(m)");
        TS_ASSERT(par);
        TS_ASSERT_EQUALS(par->asString(), castaround(wallThick[j]).substr(0,par->asString().length()));
      }
      else TS_ASSERT ( ! par );

      const V3D pos = detector->getPos();
      V3D expected;
      if( j == 1 ) // Monitors are fixed and unaffected
      {
	expected = V3D(0,0,0);
      }
      else
      {
	expected.spherical(boost::lexical_cast<double>(det_l2[j]), 
			   boost::lexical_cast<double>(det_theta[j]), 
			   boost::lexical_cast<double>(det_phi[j]));
      }
      TS_ASSERT_EQUALS(expected, pos);

    }

    // ensure that the loops below are entered into
    TS_ASSERT( WS->getNumberHistograms() > 0 );
    TS_ASSERT( WS->readX(0).size() > 0);
    // test sharing X-value arrays
    const double *previous = (&(WS->dataX(0)[0]));
    for (std::size_t k = 1; k < WS->getNumberHistograms(); ++k)
    {
      if ( k == 3 )// the third and fourth times are the same so their array should be shared
      {
        TS_ASSERT_EQUALS( previous, &(WS->dataX(k)[0]) );
      }
      else TS_ASSERT_DIFFERS( previous, &(WS->dataX(k)[0]) );

      previous = &(WS->dataX(k)[0]);
    }
    // test x offsets
    for (std::size_t x = 0; x < WS->getNumberHistograms(); x++ )
    {
      for (int j = 0; j < static_cast<int>(WS->readX(0).size()); j++ )
      {
        if ( x == DAT_MONTOR_IND )
        {
          TS_ASSERT_DELTA( WS->readX(x)[j], boost::lexical_cast<double>(delta[x]), 1e-6 );
        }
        else
        {
          TS_ASSERT_DELTA( WS->readX(x)[j], -boost::lexical_cast<double>(delta[x]), 1e-6 );
        }
      }
    }

    AnalysisDataService::Instance().remove(m_InoutWS);
  }

  void testDifferentBinsDifferentOffsets()
  {
    LoadDetectorInfo info;
    info.initialize();
    info.isInitialized();
    // Set up a small workspace for testing
    makeSmallWS();

    info.setPropertyValue("Workspace", m_InoutWS);
    info.setPropertyValue("DataFilename", m_DatFile);

    MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(m_InoutWS));
    //change a bin boundary, so they're not common anymore, only difference from the last test
    const int alteredHist = 4, alteredBin = 1;
    const double alteredAmount = 1e-4;

    WS->dataX(alteredHist)[alteredBin] =
        WS->dataX(alteredHist)[alteredBin] + alteredAmount;

    TS_ASSERT_THROWS_NOTHING(info.execute());
    TS_ASSERT( info.isExecuted() );

    // test x offsets
    TS_ASSERT( WS->getNumberHistograms() > 0 );
    for (int x = 0; x < static_cast<int>(WS->getNumberHistograms()); x++ )
    {
      for (int j = 0; j < static_cast<int>(WS->readX(0).size()); j++ )
      {
        if ( x == alteredHist && j == alteredBin)
        {
          TS_ASSERT_DELTA( WS->readX(x)[j], -boost::lexical_cast<double>(delta[x])+alteredAmount, 1e-6 );
          continue;
        }

        if ( x == DAT_MONTOR_IND )
        {
          TS_ASSERT_DELTA( WS->readX(x)[j], boost::lexical_cast<double>(delta[x]), 1e-6 );
        }
        else
        {
          TS_ASSERT_DELTA( WS->readX(x)[j], -boost::lexical_cast<double>(delta[x]), 1e-6 );
        }
      }
    }

    AnalysisDataService::Instance().remove(m_InoutWS);
  }


  void testFromRaw()
  {
    LoadDetectorInfo grouper;

    TS_ASSERT_THROWS_NOTHING( grouper.initialize() );
    TS_ASSERT( grouper.isInitialized() );

    loadRawFile();
    MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(m_MariWS));

    // check the X-values for a sample of spectra avoiding the monitors
    const int firstIndex = 5, lastIndex = 690;
    grouper.setPropertyValue("Workspace", m_MariWS);
    grouper.setPropertyValue("DataFilename", m_rawFile); 
    grouper.setPropertyValue("RelocateDets", "1");

    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    ParameterMap& pmap = WS->instrumentParameters();

    // read the parameters from some random detectors, they're parameters are all set to the same thing
    for ( int i = 0; i < NUMRANDOM; ++i)
    {
      int detID = DETECTS[i];
      boost::shared_ptr<IDetector> detector =WS->getInstrument()->getDetector(detID);

      const IComponent* baseComp = detector->getComponent();
      Parameter_sptr par = pmap.get(baseComp,"3He(atm)");

      TS_ASSERT(par);
      TS_ASSERT_EQUALS(par->asString(), castaround("10.0"));
      par = pmap.get(baseComp,"wallT(m)");
      TS_ASSERT(par);

      TS_ASSERT_EQUALS(par->asString(), castaround("0.0008").substr(0,6));
    }
    
    // Test that a random detector has been moved
    IDetector_sptr det = WS->getInstrument()->getDetector(DETECTS[0]);
    TS_ASSERT_EQUALS(V3D(0,0.2406324,4.014795), det->getPos());

    // all non-monitors should share the same array
    /**/    const double *first = &WS->readX(firstIndex)[0];
    for (int i = firstIndex+1; i < lastIndex+1; ++i)
    {
      TS_ASSERT_EQUALS( first, &(WS->readX(i)[0]) );
    }

    // the code above proves that the X-values for each histogram are the same so just check one histogram
    TS_ASSERT( WS->readX(1).size() > 0 );

    // the time of flight values that matter are the differences between the detector values and the monitors
    for (int j = 0; j < static_cast<int>(WS->readX(firstIndex).size()); j++ )
    {// we're assuming here that the second spectrum (index 1) is a monitor
      TS_ASSERT_DELTA( WS->readX(firstIndex)[j] - WS->readX(MONITOR)[j], -TIMEOFF, 1e-6 );
    }

    AnalysisDataService::Instance().remove(m_MariWS);
  }

  void loadRawFile()
  {
    LoadRaw3 loader;
    loader.initialize();

    loader.setPropertyValue("Filename", m_rawFile);
    loader.setPropertyValue("OutputWorkspace", m_MariWS);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

  LoadDetectorInfoTest() :
    m_InoutWS("loaddetectorinfotest_input_workspace"),
    m_DatFile("loaddetectorinfotest_filename.dat"),
    m_MariWS("MARfromRaw")
  {
    // create a .dat file in the current directory that we'll load later
    writeDatFile();
    m_rawFile = RAWFILE;
  }

  ~LoadDetectorInfoTest()
  {
    Poco::File(m_DatFile).remove();
  }

  // Set up a small workspace for testing
  void makeSmallWS()
  {
    MatrixWorkspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", NDETECTS, NBINS+1, NBINS);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    Mantid::MantidVecPtr xs, errors, data[NDETECTS];
    xs.access().resize(NBINS+1, 0.0);
    errors.access().resize(NBINS, 1.0);
    int detIDs[NDETECTS];
    int specNums[NDETECTS];
    for (int j = 0; j < NDETECTS; ++j)
    {
      space2D->setX(j,xs);
      data[j].access().resize(NBINS, j + 1);  // the y values will be different for each spectra (1+index_number) but the same for each bin
      space2D->setData(j, data[j], errors);
      space2D->getAxis(1)->spectraNo(j) = j+1;  // spectra numbers are also 1 + index_numbers because this is the tradition
      detIDs[j] = j;
      specNums[j] = j+1;
    }

    Instrument_sptr instr = boost::dynamic_pointer_cast<Instrument>(space->getBaseInstrument());

    Detector *d = new Detector("det",0,0);
    instr->markAsDetector(d);
    Detector *d1 = new Detector("det",1,0);
    instr->markAsDetector(d1);
    Detector *d2 = new Detector("det",2,0);
    instr->markAsDetector(d2);
    Detector *d3 = new Detector("det",3,0);
    instr->markAsDetector(d3);
    Detector *d4 = new Detector("det",4,0);
    instr->markAsDetector(d4);
    Detector *d5 = new Detector("det",5,0);
    instr->markAsDetector(d5);

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space->replaceSpectraMap(new SpectraDetectorMap(specNums, detIDs, NDETECTS));

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(m_InoutWS, space);
  }

private:
  const std::string m_InoutWS, m_DatFile, m_MariWS;
  std::string m_rawFile;
  enum constants { NDETECTS = 6, NBINS = 4, NOTUSED = -123456, DAT_MONTOR_IND = 1};
  static const std::string delta[NDETECTS], pressure[NDETECTS], wallThick[NDETECTS], code[NDETECTS], 
    det_l2[NDETECTS], det_theta[NDETECTS], det_phi[NDETECTS];

  void writeDatFile()
  {
    std::ofstream file(m_DatFile.c_str());
    file << "DETECTOR.DAT writen by LoadDetecs" << std::endl;
    file << 165888  <<  " " << 14 << std::endl;
    file << "det no.  offset    l2     code     theta        phi         w_x         w_y         w_z         f_x         f_y         f_z         a_x         a_y         a_z        det_1       det_2       det_3       det4" << std::endl;
    for( int i = 0; i < NDETECTS; ++i )
    {
      file << i  << "\t" << delta[i]<< "\t"  << det_l2[i] << "\t"  << code[i]<< "\t"  << det_theta[i]<< "\t"   << det_phi[i]<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED<< "\t"   << NOTUSED << "\t"  << NOTUSED<< "\t"   << NOTUSED<< "\t"  << NOTUSED<< "\t"  << pressure[i]<< "\t"  << wallThick[i]<< "\t"  << NOTUSED << std::endl;
    }
    file.close();
  }
  std::string castaround(std::string floatNum)
  {
    return boost::lexical_cast<std::string>(boost::lexical_cast<double>(floatNum));
  }
};

const std::string LoadDetectorInfoTest::delta[] = {"4", "4.500", "4.500", "4.500", "-6.00", "0.000"};
const std::string LoadDetectorInfoTest::pressure[] = {"10.0000", "10.0000", "10.0000", "10.0001", "10.000",  "10.0001"};
const std::string LoadDetectorInfoTest::wallThick[] = {"0.00080", "0.00080", "0.00080", "-0.00080", "0.00080",  "9.500"};
const std::string LoadDetectorInfoTest::code[] = {"3", "1", "3", "3", "3",  "3"};
const std::string LoadDetectorInfoTest::det_l2[] = {"1.5", "1.5", "1.5", "1.5", "1.5", "1.5"};
const std::string LoadDetectorInfoTest::det_theta[] = {"30", "35", "40", "45", "50", "55"};
const std::string LoadDetectorInfoTest::det_phi[] = {"-105", "-110", "-115", "-120", "-125", "-130"};

#endif /*LOADDETECTORINFOTEST_H_*/
