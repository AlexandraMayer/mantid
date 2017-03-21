#ifndef MANTID_DATAHANDLING_SAVEYDATEST_H_
#define MANTID_DATAHANDLING_SAVEYDATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveYDA.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidHistogramData/LinearGenerator.h"


#include "MantidAPI/DetectorInfo.h"




#include <Poco/File.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

using Mantid::DataHandling::SaveYDA;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

static const int NHIST = 3;
int proposal_number = 8;
std::string proposal_title = "Prop";
std::string experiment_team = "Team";
double temperature = 200.25;
double Ei = 1.5;

void addSampleLogData(Workspace2D_sptr /* MatrixWorkspace_sptr*/ ws,const std::string &name,const std::string &value ) {
    Run &run = ws->mutableRun();
    run.addLogData(new Mantid::Kernel::PropertyWithValue<std::string>(name,value));
}

void addSampleLogData(Workspace2D_sptr /*MatrixWorkspace_sptr*/ ws, const std::string &name,const double &value) {
    Run &run = ws->mutableRun();
    run.addLogData(new Mantid::Kernel::PropertyWithValue<double>(name,value));
}

void addAllLogs(Workspace2D_sptr /*MatrixWorkspace_sptr */ws) {
    addSampleLogData(ws,"proposal_number", proposal_number);
    addSampleLogData(ws,"proposal_title",proposal_title );
    addSampleLogData(ws,"experiment_team",experiment_team);
    addSampleLogData(ws,"temperature",temperature);
    addSampleLogData(ws,"Ei",1.5);
}



class SaveYDATest : public CxxTest::TestSuite {
private:
    const std::string name = "ws";
    Workspace2D_sptr /*MatrixWorkspace_sptr*/ wsSave;
    SaveYDA ydaSaver;
    std::string filename;

    std::ifstream in;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveYDATest *createSuite() { return new SaveYDATest(); }
  static void destroySuite( SaveYDATest *suite ) { delete suite; }

  SaveYDATest() {}
  ~SaveYDATest() {}

  void test_init() {

      TS_ASSERT_THROWS_NOTHING(ydaSaver.initialize());
      TS_ASSERT(ydaSaver.isInitialized());

  }


  void test_exec()
  {
    //const std::string name = "ws";

    wsSave =
           // boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
            //(WorkspaceFactory::Instance().create("Workspace2D",2,10,10));
          (makeWorkspace(name));
    //MatrixWorkspace_const_sptr wsSave = wsSave;
    addAllLogs(wsSave);
    //MatrixWorkspace_const_sptr wsSave = wsSave;
    filename = "SaveYDAFile.yaml";
    std::cout << "problem with wsSave?" << wsSave->id() << std::endl;

    TS_ASSERT_THROWS_NOTHING(ydaSaver.initialize());
    TS_ASSERT(ydaSaver.isInitialized());
    TS_ASSERT_THROWS_NOTHING(ydaSaver.setPropertyValue("InputWorkspace",name));
    std::cout << "Input ws" << name << " = " << ydaSaver.getPropertyValue("InputWorkspace") << std::endl;
    std::cout << "ws id" << wsSave->id() << std::endl;
    TS_ASSERT_THROWS_NOTHING(ydaSaver.setPropertyValue("Filename", filename));
    filename = ydaSaver.getPropertyValue("Filename");

    std::cout << "filname = " << filename << std::endl;

    std::cout << "Problem bevore execute ySaver?" << std::endl;

    TS_ASSERT_THROWS_NOTHING(ydaSaver.execute());

    std::cout << "Problem after execute ydaSaver?" << std::endl;
    TS_ASSERT(Poco::File(filename).exists());
    in = std::ifstream(filename.c_str());
    std::cout << "End test exec" << std::endl;


  }

  void test_metadata() {
      std::cout << "in Metadata" << std::endl;

      std::cout << "initilazing ifstream" << std::endl;

      std::string metadata[3];

      for (int i = 0; i < 3; i++) {
           std::getline(in,metadata[i]);
      }

      TS_ASSERT_EQUALS(metadata[0],"Meta:");
      TS_ASSERT_EQUALS(metadata[1],"  format: yaml/frida 2.0");
      TS_ASSERT_EQUALS(metadata[2],"  type: generic tabular data");

  }

  void test_history() {
      std::cout << "in test history" << std::endl;

      std::string history[5];

      for(int i = 0; i< 5; i++) {
          std::getline(in,history[i]);
      }

      TS_ASSERT_EQUALS(history[0], "History:");
      TS_ASSERT_EQUALS(history[1], "  - Proposal number " + std::to_string(proposal_number) );
      TS_ASSERT_EQUALS(history[2], "  - " + proposal_title);
      TS_ASSERT_EQUALS(history[3], "  - " + experiment_team);
      TS_ASSERT_EQUALS(history[4], "  - data reduced with mantid");
  }

  void test_coord() {
      std::cout << "in test coord" << std::endl;

      std::string coord[4];

      for(int i = 0; i < 4; i++) {
          std::getline(in,coord[i]);
      }

      TS_ASSERT_EQUALS(coord[0], "Coord:");
      TS_ASSERT_EQUALS(coord[1], "  x: {name: w, unit: meV}");
      std::cout << coord[1] << std::endl;
      TS_ASSERT_EQUALS(coord[2], "  y: {name: \"S(q,w)\", unit: meV-1}");
      std::cout << coord[2] << std::endl;
      TS_ASSERT_EQUALS(coord[3], "  z: {name: 2th, unit: deg}");
      std::cout << coord[3] << std::endl;

  }

  void test_rpar() {
      std::cout << "in test rPar" << std::endl;

      std::string rpar[9];

      for(int i = 0; i < 9; i++) {
          std::getline(in,rpar[i]);
      }

      TS_ASSERT_EQUALS(rpar[0], "RPar:");
      TS_ASSERT_EQUALS(rpar[1], "  - name: T");
      TS_ASSERT_EQUALS(rpar[2], "    unit: K");
      TS_ASSERT_EQUALS(rpar[3], "    val: " + withoutzeros(temperature));
      TS_ASSERT_EQUALS(rpar[4], "    stdv: 0");
      TS_ASSERT_EQUALS(rpar[5], "  - name: Ei");
      TS_ASSERT_EQUALS(rpar[6], "    unit: meV");
      TS_ASSERT_EQUALS(rpar[7], "    val: " + withoutzeros(Ei));
      TS_ASSERT_EQUALS(rpar[8], "    stdv: 0");

  }

  void test_slices() {
      std::cout << "in test slices" << std::endl;

      std::string slices[9];

      for(int i = 0; i < 9; i++) {
          std::getline(in,slices[i]);
      }

      wsSave->getAxis(1)->setUnit("MomentumTransfer");

      TS_ASSERT_EQUALS(slices[0], "Slices:");
      std::vector<double> bin_centers;
      //ydaSaver.getBinCenters(wsSave->getAxis(1), bin_centers);
      const auto &spectrumInfo = wsSave->spectrumInfo();
      for(size_t i = 0; i < wsSave->getNumberHistograms(); i++) {
          if(!spectrumInfo.isMonitor(i)) {
              double twoTheta = spectrumInfo.twoTheta(i);
              twoTheta = (180*twoTheta)/M_PI;
              /*bin_centers*/bin_centers.push_back(twoTheta);
          }
      }
      std::vector<double> x_centers;
      ydaSaver.getBinCenters(wsSave->getAxis(0), x_centers);
      int slicescounter = 1;
      for(int i = 0; i < 2; i++) {
          TS_ASSERT_EQUALS(slices[slicescounter++], "  - j: " + std::to_string(i));
          if(withoutzeros(bin_centers[i]) == "169.565") {
                  TS_ASSERT_EQUALS(slices[slicescounter++], "    z: [{val: 169.5649999999999}]");
          }else{
                  TS_ASSERT_EQUALS(slices[slicescounter++], "    z: [{val: " + withoutzeros(bin_centers[i]) + "}]");
          }
          std::string x = "    x: [";
          std::string y = "    y: [";
          for(int j = 0; j < 9; j++) {
              x += withoutzeros(x_centers[j]) + ", ";
              y += withoutzeros(wsSave->y(i)[j]) + ", ";
          }
          x += withoutzeros(x_centers[9]) + "]";
          y += withoutzeros(wsSave->y(i)[9]) + "]";
          TS_ASSERT_EQUALS(slices[slicescounter++], x);
          TS_ASSERT_EQUALS(slices[slicescounter++], y);
      }

  }





private:
  Workspace2D_sptr/*MatrixWorkspace_sptr*/ makeWorkspace(const std::string &inwsname) {
        Workspace2D_sptr/* MatrixWorkspace_sptr*/ inws = (WorkspaceCreationHelper::create2DWorkspaceBinned(NHIST,10,1.0));
      std::cout << "is Matrixws "  << inws->id() << std::endl;
      return setUpWs(inwsname,inws);
  }

  Workspace2D_sptr/* MatrixWorkspace_sptr */setUpWs(const std::string &inwsname,Workspace2D_sptr /* MatrixWorkspace_sptr*/ inws) {

      inws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");

      Mantid::API::AnalysisDataService::Instance().add(inwsname, inws);
      Mantid::DataHandling::LoadInstrument loader;
      TS_ASSERT_THROWS_NOTHING(loader.initialize());
      //loader.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
      loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
      loader.setPropertyValue("Workspace", inwsname);
      loader.setPropertyValue("Filename", "INES_Definition.xml");
      TS_ASSERT_THROWS_NOTHING(loader.execute());
      for(int i = 0; i < 3; i++) {
          auto &X = inws->mutableX(i);
          auto &Y = inws->mutableY(i);
          for(int j = 0; j < 11; j++) {
              X[j] = (j+1) * 0.5;
              std::cout << X[j] << std::endl;
              Y[j] =  ((1.1+j))/2*0.5;
              std::cout << Y[j] << std::endl;
          }
      }
      return inws;
  }

  std::string withoutzeros(double val) {
      std::string wthozeros = std::to_string(val);
      wthozeros.erase(wthozeros.find_last_not_of('0') + 1, std::string::npos);
      return wthozeros;
  }


 };

static const int nHistPerformance = 1000;

class SaveYDATestPerformance : public CxxTest::TestSuite {
public:
    void setUp() override {

        std::cout << "\nin setup" << std::endl;
        ///*Workspace2D_sptr*/ ws = WorkspaceCreationHelper::create2DWorkspaceBinned(nHistPerformance,1000,2.0,0.01);
        ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nHistPerformance,1000);
        std::cout << "did make ws2D " << ws << " id? " << ws->id() << std::endl;
        std::cout << "Number of Histogramms " << ws->getNumberHistograms() << std::endl;
        //std::cout << "Problem with AnalysisDataService?" << std::endl;
        //Mantid::API::AnalysisDataService::Instance().add(wsName,ws);
        //std::cout << "NO!" << std::endl;

        ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");


       /*
        Mantid::DataHandling::LoadInstrument loader;
        TS_ASSERT_THROWS_NOTHING(loader.initialize());
        //loader.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
        loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
        loader.setPropertyValue("Workspace", wsName);
        loader.setPropertyValue("Filename", "INES_Definition.xml");
        TS_ASSERT_THROWS_NOTHING(loader.execute());
*/
        /*
        std::cout << "Bin edges? " << std::endl;
        Mantid::HistogramData::BinEdges xs(990,Mantid::HistogramData::LinearGenerator(10.0,1.0));
        std::cout << "no" << std::endl;
        std::cout << "xs";
        for(auto x : xs)
            std::cout << x << std::endl;
        std::cout << "Count Standard deviations? " << std::endl;
        Mantid::HistogramData::CountStandardDeviations errors(998,0.1);
        std::cout << "no" << std::endl;
        std::cout << "error";
        for(auto err : errors)
            std::cout << err << std::endl;
        for (int j=0; j < nHistPerformance; ++j) {
            std::cout << "setBinEdges?" << std::endl;
            ws->setBinEdges(j,xs);
            std::cout << "no" << std::endl;
            std::cout << "setCounts? " << std::endl;
            ws->setCounts(j,998,j+1);
            std::cout << "no" << std::endl;
            ws->setCountStandardDeviations(j,errors);
            ws->getSpectrum(j).setDetectorID(j);
        }
        Mantid::Geometry::Instrument_sptr instr(new Mantid::Geometry::Instrument);
        for (Mantid::detid_t i = 0; i < 1000; i++) {
          Mantid::Geometry::Detector *d = new Mantid::Geometry::Detector("det", i, 0);
          instr->markAsDetector(d);
          //d->setPos(i,i,i);
        }

        ws->setInstrument(instr);
        std::cout << "setInstrument works" << std::endl;
*/
/*
        std::vector<double> dummy(ws->getNumberHistograms(),0.0);
        auto testInst = ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(dummy,dummy,dummy);
        ws->setInstrument(testInst);
        ws->mutableDetectorInfo().setMasked(1,true);
        ws->setDistribution(true);
*/
        Mantid::API::AnalysisDataService::Instance().add(wsName, ws);

        for(int i = 0; i < 1000; i++) {
            std::cout << "i = " << i << std::endl;
            auto &X = ws->mutableX(i);
            auto &Y = ws->mutableY(i);
            for(int j = 0; j < 1001; j++) {
                //std::cout << "j = " << j << std::endl;
                X[j] = (j+1) * 0.5;

                Y[j] =  ((1.1+j))/2*0.5;

            }
        }

        addAllLogs(ws);
        std::cout << "Problem in for?" << std::endl;
        for (int i=0; i < numberOfIterations; i++) {
            std::cout << "iteration nr. " << i << " of " << numberOfIterations;
            //saveAlgPtrs.emplace_back(setupAlg());
            saveAlgPtrs.push_back(setupAlg());
            std::cout << "saveAlgPtrs at " << i << " = " << saveAlgPtrs[i] << std::endl;
        }

    }

    void testSaveYDAPerformance() {
        for(auto alg : saveAlgPtrs) {
            TS_ASSERT_THROWS_NOTHING(alg->execute());
        }
    }

   void tearDown() override {
        for(int i = 0; i < numberOfIterations; i++) {
            delete saveAlgPtrs[i];
            saveAlgPtrs[i] = nullptr;
        }
        Mantid::API::AnalysisDataService::Instance().remove(wsName);
        Poco::File focusedFile(perfilename);
        if(focusedFile.exists())
            focusedFile.remove();
    }

private:
    const int numberOfIterations = 5;
    std::vector<Mantid::DataHandling::SaveYDA *> saveAlgPtrs;
    const std::string wsName = "SaveYDAPerformance";
    const std::string perfilename = "perfSaveYDA.yaml";
    Workspace2D_sptr ws;

    Mantid::DataHandling::SaveYDA *setupAlg() {
        std::cout << "\nMuss hier Fehler sein" << std::endl;
      Mantid::DataHandling::SaveYDA *saver =
          new Mantid::DataHandling::SaveYDA;
      std::cout << "Saver initialized " << saver <<  std::endl;
      TS_ASSERT_THROWS_NOTHING(saver->initialize());
      TS_ASSERT(saver->isInitialized());
      std::cout << " As string " << saver->asString() << std::endl;
      std::cout << "Problem with SetProperty(Filname)? " << std::endl;
      saver->setPropertyValue("Filename", perfilename);
      std::cout << "NO!" << std::endl;
      std::cout << "Problem with SetPropertyValue(InputWorkspace)? " << std::endl;
      std::cout << "Workspace name Problem? wsName = " << wsName << " = " << ws->getName() << std::endl;
      std::cout << "is same? " << (wsName==ws->getName()) << std::endl;
      saver->setPropertyValue("InputWorkspace", wsName);
      std::cout << "NO!" << std::endl;

      saver->setRethrows(true);
      return saver;
    }
};




#endif /* MANTID_DATAHANDLING_SAVEYDATEST_H_ */
