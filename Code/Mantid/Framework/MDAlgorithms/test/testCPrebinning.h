#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include <boost/shared_ptr.hpp>


using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;

std::string findTestFileLocation(void);

bool load_existing_workspace(const std::string &workspace_name){
// helper function to load a workpsace -- something a user should do before rebinning

	//	 std::auto_ptr<IMD_FileFormat> pFile = MD_FileFormatFactory::getFileReader("../../../../../Test/VATES/fe_demo.sqw",old_4DMatlabReader);
//    std::string dataFileName("../../../../../Test/VATES/fe_demo.sqw");
//    std::string dataFileName("../../../../../Test/VATES/fe_demo_bin.sqw");
    std::string dataFileName("../../../../Test/AutoTestData/test_horace_reader.sqw");

    Load_MDWorkspace loader;
    loader.initialize();
    loader.setPropertyValue("inFilename",dataFileName);
 
    loader.setPropertyValue("MDWorkspace",workspace_name);
    loader.execute();

    Workspace_sptr result=AnalysisDataService::Instance().retrieve(workspace_name);
    MDWorkspace*  pOrigin = dynamic_cast<MDWorkspace *>(result.get());
    // no workspace loaded -- no point to continue
    if(!pOrigin)return false;

    return true;

}

class testCPrebinning :    public CxxTest::TestSuite
{
       std::string InputWorkspaceName;
   // test centerpiece rebinning 
       CenterpieceRebinning cpr;
 public:
    void testRebinInit(void){

     InputWorkspaceName = "MyTestMDWorkspace";

     TS_ASSERT_THROWS_NOTHING(cpr.initialize());
     TS_ASSERT( cpr.isInitialized() );

     TSM_ASSERT("We should be able to load the initial workspace successfully",load_existing_workspace(InputWorkspaceName));

   
      cpr.setPropertyValue("Input", InputWorkspaceName);      
      cpr.setPropertyValue("Result","OutWorkspace");
      // set slicing property to the size and shape of the current workspace
      cpr.init_slicing_property();

    }
    void testInitSlicingProperty(){      


    // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)

     // now modify it as we need;
        double r0=0;
        pSlicing->pDimDescription("qx")->cut_min = r0;
		pSlicing->pDimDescription("qx")->cut_max = r0+1;
		pSlicing->pDimDescription("qy")->cut_min = r0;
		pSlicing->pDimDescription("qy")->cut_max = r0+1;
		pSlicing->pDimDescription("qz")->cut_min = r0;
		pSlicing->pDimDescription("qz")->cut_max = r0+1;
		pSlicing->pDimDescription("en")->cut_max = 50;
        
    }
    void testCPRExec(){
        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }

    void testCPRExecAgain(){
  // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)

        pSlicing->pDimDescription("qx")->cut_max = 0+2;
        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
    void testRebinningResults(){
        // now test if we have rebinned things properly
        Workspace_sptr rezWS = AnalysisDataService::Instance().retrieve("OutWorkspace");

        MDWorkspace_sptr targetWS = boost::dynamic_pointer_cast<MDWorkspace>(rezWS);
        TSM_ASSERT("The workspace obtained is not target MD workspace",targetWS!=0);

    }
};


#endif
