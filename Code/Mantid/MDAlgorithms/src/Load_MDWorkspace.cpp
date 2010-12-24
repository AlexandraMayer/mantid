#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "Poco/Path.h"

namespace Mantid{
    namespace MDAlgorithms{
        using namespace Mantid;
        using namespace MDDataObjects;
        using namespace Kernel;
        using namespace API;
        using namespace Geometry;


// Register the class into the algorithm factory
DECLARE_ALGORITHM(Load_MDWorkspace)
// logger for loading workspaces  
Kernel::Logger& Load_MDWorkspace::ldmdws_log =Kernel::Logger::get("MD-Algorithms");


Load_MDWorkspace::Load_MDWorkspace(void): API::Algorithm() 
{}

/** Destructor     */
Load_MDWorkspace::~Load_MDWorkspace()
{
 
}
//
void
Load_MDWorkspace::init()
{
    std::vector<std::string> ext(1);
    ext[0]=".sqw";
    declareProperty(new API::FileProperty("inFilename","", API::FileProperty::Load,ext), "The file containing initial MD dataset");
    declareProperty(new WorkspaceProperty<MDWorkspace>("MDWorkspace","",Direction::Output),"final MD workspace");
    // prospective property TODO: implement
    declareProperty((new Kernel::PropertyWithValue<bool>("TryLoadPixels",false,Direction::InOut)),
        "This property specifies if user wants to try loading in memory"
        " all pixels(events) availible in the MD workspace; If there is enough memory system will try to do it but may fail");
}
//
void 
Load_MDWorkspace::exec()
{

MDWorkspace_sptr inputWS;
std::string workspaceFileName;

  // get the workspace and overwrite it if it already exists
   inputWS = this->getProperty("MDWorkspace");
   if(inputWS){
       ldmdws_log.information()<<" Existing workspace will be overwtitten\n";
   }else{
       inputWS = MDWorkspace_sptr(new MDWorkspace());
       AnalysisDataService::Instance().addOrReplace("MDWorkspace", inputWS);
   }
   this->setProperty("MDWorkspace",inputWS);


   if(existsProperty("inFilename")){
       workspaceFileName = this->getPropertyValue("inFilename");
       workspaceFileName = Poco::Path(Poco::Path::current()).resolve(workspaceFileName).toString();
   }

   std::auto_ptr<IMD_FileFormat> pFile = MD_FileFormatFactory::getFileReader(workspaceFileName.c_str());
   if(!pFile.get()){
         ldmdws_log.error()<<" can not obtain file reader for MD file";
         throw(Kernel::Exception::FileError("can not get proper file reader",workspaceFileName));
   }
   std::auto_ptr<MDGeometryBasis> pBasis = std::auto_ptr<MDGeometryBasis>(new Geometry::MDGeometryBasis());
   
   pFile->read_basis(*pBasis);

   MDGeometryDescription geomDescr(pBasis->getNumDims(),pBasis->getNumReciprocalDims());

	// read the geometry description
   pFile->read_MDGeomDescription(geomDescr);

	// obtain the MDPoint description now (and MDPointsDescription in a future)
	MDPointDescription pd = pFile->read_pointDescriptions();


    inputWS->init(pFile,pBasis,geomDescr,pd);
   // currently is_initiated supported only for image;
  //  if(!inputWS->is_initiated()){
    // throw and end
  //  }

    IMD_FileFormat *pReader = inputWS->get_pFileReader();

    pReader->read_MDImg_data(*inputWS->get_spMDImage());
    //TODO: should be moved into datapoints;
    inputWS->get_spMDImage()->identify_SP_points_locations();


    //TODO: if enough memory and we want to place pixels in memory -- read MDDatapoints
    // should also allocate memory for points somewhere here. 
   //  pReader->read_pix(*(inputWS->get_spMDDPoints()));

}
} // end namespaces
}