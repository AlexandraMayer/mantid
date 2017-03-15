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
#include <Poco/File.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

using Mantid::DataHandling::SaveYDA;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

static const int NHIST = 3;

class SaveYDATest : public CxxTest::TestSuite {
private:
    const std::string name = "ws";
    Mantid::API::MatrixWorkspace_sptr wsSave;
    SaveYDA ydaSaver;
    std::string filename;
    int proposal_number = 8;
    std::string proposal_title = "Prop";
    std::string experiment_team = "Team";
    double temperature = 200.25;
    double Ei = 1.5;
    std::ifstream in;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveYDATest *createSuite() { return new SaveYDATest(); }
  static void destroySuite( SaveYDATest *suite ) { delete suite; }

  SaveYDATest() {}

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
            makeWorkspace(name);
    addAllLogs(wsSave);

    filename = "SaveYDAFile.yaml";



    TS_ASSERT_THROWS_NOTHING(ydaSaver.setPropertyValue("InputWorkspace",name));
    TS_ASSERT_THROWS_NOTHING(ydaSaver.initialize());
    TS_ASSERT(ydaSaver.isInitialized());
    TS_ASSERT_THROWS_NOTHING(ydaSaver.setPropertyValue("Filename", filename));
    filename = ydaSaver.getPropertyValue("Filename");

    std::cout << "Problem bevore execute ySaver?" << std::endl;

    TS_ASSERT_THROWS_NOTHING(ydaSaver.execute());

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
      TS_ASSERT_EQUALS(history[1], "  - \"proposal number: " + std::to_string(proposal_number) + "\"" );
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
      TS_ASSERT_EQUALS(coord[1], "  - x: {name: w, unit: meV}");
      TS_ASSERT_EQUALS(coord[2], "  - y: {name: \"S(q,w)\", unit: meV-1}");
      TS_ASSERT_EQUALS(coord[3], "  - z: {name: 2th, unit: deg}");

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

      TS_ASSERT_EQUALS(slices[0], "Slices:");
      std::vector<double> bin_centers;
      ydaSaver.getBinCenters(wsSave->getAxis(1), bin_centers);
      std::vector<double> x_centers;
      ydaSaver.getBinCenters(wsSave->getAxis(0), x_centers);
      int slicescounter = 1;
      for(int i = 0; i < 2; i++) {
          TS_ASSERT_EQUALS(slices[slicescounter++], "  - j: " + std::to_string(i));
          TS_ASSERT_EQUALS(slices[slicescounter++], "    z: [{val: " + withoutzeros(bin_centers[i]) + "}]");
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
  MatrixWorkspace_sptr makeWorkspace(const std::string &inwsname) {
        MatrixWorkspace_sptr inws = WorkspaceCreationHelper::create2DWorkspaceBinned(NHIST,10,1.0);
        return setUpWs(inwsname,inws);
  }

  MatrixWorkspace_sptr setUpWs(const std::string &inwsname, MatrixWorkspace_sptr inws) {
      inws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");

      Mantid::API::AnalysisDataService::Instance().add(inwsname, inws);
      Mantid::DataHandling::LoadInstrument loader;
      TS_ASSERT_THROWS_NOTHING(loader.initialize());
      loader.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
      loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
      loader.setPropertyValue("Workspace", inwsname);
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

  void addAllLogs(MatrixWorkspace_sptr ws) {
      addSampleLogData(ws,"proposal_number", proposal_number);
      addSampleLogData(ws,"proposal_title",proposal_title );
      addSampleLogData(ws,"experiment_team",experiment_team);
      addSampleLogData(ws,"temperature",temperature);
      addSampleLogData(ws,"Ei",1.5);
  }

  void addSampleLogData(MatrixWorkspace_sptr ws,const std::string &name,const std::string &value ) {
      Run &run = ws->mutableRun();
      run.addLogData(new Mantid::Kernel::PropertyWithValue<std::string>(name,value));
  }

  void addSampleLogData(MatrixWorkspace_sptr ws, const std::string &name,const double &value) {
      Run &run = ws->mutableRun();
      run.addLogData(new Mantid::Kernel::PropertyWithValue<double>(name,value));
  }


};


#endif /* MANTID_DATAHANDLING_SAVEYDATEST_H_ */
