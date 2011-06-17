#ifndef H_CP3DxN_REBINNING
#define H_CP3DxN_REBINNING
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
//    std::string dataFileName("fe_demo.sqw");
//    std::string dataFileName("c:/Users/wkc26243/Documents/work/MANTID/Test/VATES/fe_demo_bin.sqw");
   std::string dataFileName("test_horace_reader.sqw");
//        std::string dataFileName("fe_E800_8K.sqw");
    Load_MDWorkspace loader;
    loader.initialize();
    loader.setPropertyValue("inFilename",dataFileName);
 
    loader.setPropertyValue("MDWorkspace",workspace_name);
    // describe test workspace;
    Geometry::MDGeometryDescription DefaultDescr(4,3);
    DefaultDescr.pDimDescription(0)->nBins  = 10;
    DefaultDescr.pDimDescription(0)->cut_min = -6.6;
    DefaultDescr.pDimDescription(0)->cut_max =  6.6;
    DefaultDescr.pDimDescription(0)->Tag     = "qx";
 
    DefaultDescr.pDimDescription(1)->nBins  = 10;
    DefaultDescr.pDimDescription(1)->cut_min = -6.6;
    DefaultDescr.pDimDescription(1)->cut_max =  6.6;
    DefaultDescr.pDimDescription(1)->Tag     = "qy";

    DefaultDescr.pDimDescription(2)->nBins  = 10;
    DefaultDescr.pDimDescription(2)->cut_min = -6.6;
    DefaultDescr.pDimDescription(2)->cut_max =  6.6;
    DefaultDescr.pDimDescription(2)->Tag     = "qz";

    DefaultDescr.pDimDescription(3)->nBins  = 10;
    DefaultDescr.pDimDescription(3)->cut_min =  0;
    DefaultDescr.pDimDescription(3)->cut_max =  150;
    DefaultDescr.pDimDescription(3)->Tag     = "en";

    DefaultDescr.nContributedPixels          = 10000000;
    loader.set_test_mode(DefaultDescr);

    loader.execute();

    Workspace_sptr result=AnalysisDataService::Instance().retrieve(workspace_name);
    MDWorkspace*  pOrigin = dynamic_cast<MDWorkspace *>(result.get());
    // no workspace loaded -- no point to continue
    if(!pOrigin)return false;

    return true;

}

class CP3DxN_RebinningTest :  public CxxTest::TestSuite
{
       std::string InputWorkspaceName;
       std::string OutWorkspaceName;
   // test centerpiece rebinning 
       CenterpieceRebinning cpr;
 public:
	

    void testRebinInit(void){

     InputWorkspaceName = "testCP_rebinningIn";
     OutWorkspaceName   = "testCP_rebinningOut";

     TS_ASSERT_THROWS_NOTHING(cpr.initialize());
     TS_ASSERT( cpr.isInitialized() );

     TSM_ASSERT("We should be able to load the initial workspace successfully",load_existing_workspace(InputWorkspaceName));

   
      cpr.setPropertyValue("Input", InputWorkspaceName);      
      cpr.setPropertyValue("Result",OutWorkspaceName);
      cpr.setProperty("KeepPixels",false);
      // set slicing property for the target workspace to the size and shape of the current workspace
      cpr.setTargetGeomDescrEqSource();

    }
    void testGetSlicingProperty(){      

    // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)
  
    }
    void testCPRExec(){
       cpr.setProperty("KeepPixels",false);
        // rebin image into the same grid as an initial image, which should provide the same image as before
        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
    void testRebinnedWSExists(){
        // now test if we can get the resulting workspace

        Workspace_sptr rezWS = AnalysisDataService::Instance().retrieve(OutWorkspaceName);

        MDWorkspace_sptr targetWS = boost::dynamic_pointer_cast<MDWorkspace>(rezWS);
        TSM_ASSERT("The workspace obtained is not target MD workspace",targetWS!=0);

    }
    void testEqRebinCorrectness(){
        // we did rebinning on the full image and initial image have to be equal to the target image; Let's compare them
	    MDWorkspace_sptr inputWS, outWS;
        // Get the workspaces
        inputWS = boost::dynamic_pointer_cast<MDWorkspace>(AnalysisDataService::Instance().retrieve(InputWorkspaceName));
        outWS   = boost::dynamic_pointer_cast<MDWorkspace>(AnalysisDataService::Instance().retrieve(OutWorkspaceName));
         
        const MDImage &OldIMG = inputWS->get_const_MDImage();
        const MDImage &NewIMG = outWS->get_const_MDImage();

        for(size_t i=0;i<OldIMG.getDataSize();i++){
            TSM_ASSERT_DELTA("Old and new images points in this case have to be equal",OldIMG.getSignal(i),NewIMG.getSignal(i),1.e-4);
			/*if(abs(OldIMG.getSignal(i)-NewIMG.getSignal(i))>1.e-4){
				continue;
			}*/
        }
    }

    void testCPRRebinAgainSmaller(){
   // now rebin into slice
  // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)

   // now modify it as we need/want;
        double r0=0;
        pSlicing->pDimDescription("qx")->cut_min = r0;
		pSlicing->pDimDescription("qx")->cut_max = r0+1;
		pSlicing->pDimDescription("qx")->nBins = 200;
		pSlicing->pDimDescription("qy")->cut_min = r0;
		pSlicing->pDimDescription("qy")->cut_max = r0+1;
		pSlicing->pDimDescription("qz")->cut_min = r0;
		pSlicing->pDimDescription("qz")->cut_max = r0+1;
		pSlicing->pDimDescription("en")->cut_max = 50;
		// and set a rotation
		Geometry::OrientedLattice rotator(1,1,1);
		Kernel::DblMatrix Rot = rotator.setUFromVectors(Kernel::V3D(1,1,1),Kernel::V3D(1,-1,-1));
		pSlicing->setRotationMatrix(Rot);
  
   /*     pSlicing->pDimDescription("qx")->nBins = 200;
		pSlicing->pDimDescription("qy")->nBins = 200;
		pSlicing->pDimDescription("qz")->nBins = 200;
		pSlicing->pDimDescription("en")->nBins  = 0;*/


        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
  void testCPRRebinSmallerInto3D(){
   // now rebin into slice
  // retrieve slicing property for modifications
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Property *)(cpr.getProperty("SlicingData")));
      TSM_ASSERT("Slicing property should be easy obtainable from property manager",pSlicing!=0)

   // now modify it as we need/want;
        double r0=0;
        pSlicing->pDimDescription("qx")->cut_min = r0;
		pSlicing->pDimDescription("qx")->cut_max = r0+1;
	    pSlicing->pDimDescription("qx")->nBins   = 1; // integrate over qx dimension and produce 2D+1 dataset;
		// despite that the dataset always remains 3D+1 internaly
	

		pSlicing->pDimDescription("qy")->cut_min = r0;
		pSlicing->pDimDescription("qy")->cut_max = r0+1;
		pSlicing->pDimDescription("qz")->cut_min = r0;
		pSlicing->pDimDescription("qz")->cut_max = r0+1;
		pSlicing->pDimDescription("en")->cut_max = 50;
  
   /*     pSlicing->pDimDescription("qx")->nBins = 200;
		pSlicing->pDimDescription("qy")->nBins = 200;
		pSlicing->pDimDescription("qz")->nBins = 200;
		pSlicing->pDimDescription("en")->nBins  = 0;*/
		// and set a rotation
		Geometry::OrientedLattice rotator(1,1,1);
		Kernel::DblMatrix Rot = rotator.setUFromVectors(Kernel::V3D(0,1,1),Kernel::V3D(1,-1,-1));
		pSlicing->setRotationMatrix(Rot);



        TSM_ASSERT_THROWS_NOTHING("Good rebinning should not throw",cpr.execute());
    }
	 void testClearWorkspaces(){
		 //  not entirely according to standarts, but does not test anything but deletes workpsaces to free memory when running in suite
		 // before the real destructor is called
		 AnalysisDataService::Instance().remove(InputWorkspaceName);
		 AnalysisDataService::Instance().remove(OutWorkspaceName);
	 }
};


#endif
