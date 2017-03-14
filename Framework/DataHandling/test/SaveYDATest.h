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

using Mantid::DataHandling::SaveYDA;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

static const int NHIST = 3;

class SaveYDATest : public CxxTest::TestSuite {
private:
    SaveYDA ydaSaver;
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
    const std::string name = "ws";

    Mantid::API::MatrixWorkspace_sptr wsSave =
           // boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
            //(WorkspaceFactory::Instance().create("Workspace2D",2,10,10));
            makeWorkspace(name);

    for(int i = 0; i < 2; i++) {
        auto &X = wsSave->mutableX(i);
        auto &Y = wsSave->mutableY(i);
        auto &E = wsSave->mutableE(i);

        for(int j = 0; j < 10; j++) {
            X[j] = 3.5 *j /1.4;
            Y[j] = (1.5 + i) * (0.65 + 1. + X[j]);
            E[j] = 0.;

        }

    }
    std::string filename = "SaveYDAFile.dat";

    TS_ASSERT_THROWS_NOTHING(ydaSaver.setPropertyValue("InputWorkspace",name));
    TS_ASSERT_THROWS_NOTHING(ydaSaver.initialize());
    TS_ASSERT(ydaSaver.isInitialized());
    TS_ASSERT_THROWS_NOTHING(ydaSaver.setPropertyValue("Filename", filename));
    filename = ydaSaver.getPropertyValue("Filename");

    TS_ASSERT_THROWS_NOTHING(ydaSaver.execute());

    TS_ASSERT(Poco::File(filename).exists());
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
      return inws;
  }


};


#endif /* MANTID_DATAHANDLING_SAVEYDATEST_H_ */
