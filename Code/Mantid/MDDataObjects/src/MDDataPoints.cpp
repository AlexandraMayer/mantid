#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDImage.h"

namespace Mantid{
namespace MDDataObjects{
using namespace Mantid::Kernel;
/// this class is idle at the moment as all renbinning functionality is wired up through the file reader;

// logger for MD workspaces  
Kernel::Logger& MDDataPoints::g_log =Kernel::Logger::get("MDWorkspaces");
// default names for signal and error fields
const char *DefaultSignalTags[]={"S","Err","iRun","iDet","iEn"};


//------------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param spImage: ???
 * */
MDDataPoints::MDDataPoints(boost::shared_ptr<const MDImage> spImage,const MDDataPointsDescription &descr):
  description(descr),
  memBased(false),
  n_data_points(0),
  n_fields(9),
  data_buffer_size(0),
  data_buffer(NULL),
  m_spMDImage(spImage)
{

  std::vector<std::string> dim_tags= m_spMDImage->getGeometry()->getBasisTags();
  std::vector<std::string> signal_tags(DefaultSignalTags,DefaultSignalTags+4);
  this->field_tag.reserve(n_fields);
  this->field_tag.insert(field_tag.end(),dim_tags.begin(),dim_tags.end());
  this->field_tag.insert(field_tag.end(),signal_tags.begin(),signal_tags.end());

  int nDims = m_spMDImage->getGeometry()->getNumDims();
  this->box_min.assign(nDims,FLT_MAX);
  this->box_max.assign(nDims,-FLT_MAX);
}


//------------------------------------------------------------------------------------------------
/** */
size_t MDDataPoints::getNumPixels(boost::shared_ptr<IMD_FileFormat> spFile)
{
  if(this->n_data_points>0){   return n_data_points;
  }

  if(spFile.get()){
    this->n_data_points = spFile->getNPix();
  }else{
    this->g_log.information("MDPixels::getNumPixels: Attemting to get number of pixels from undefined dataset");
    n_data_points = 0;
    throw(Kernel::Exception::NullPointerException("getNumPixels","File reader has not beed defined yet"));
  }
  return n_data_points;
}

//------------------------------------------------------------------------------------------------
/** */
void MDDataPoints::set_field_length(const std::vector<unsigned int> &in_fields)
{
  this->n_fields=(unsigned int)in_fields.size();
  this->field_length  = in_fields;

  this->field_start.assign(n_fields+1,0);
  for(unsigned int i=1;i<=n_fields;i++){
    field_start[i]=field_start[i-1]+field_length[i-1];
  }


}
//***************************************************************************************
void MDDataPoints::alloc_pix_array(boost::shared_ptr<IMD_FileFormat> spFile)
{
  if(this->data_buffer){
    // if it is already allocated and big enough, do nothing
    size_t nPix= this->getNumPixels(spFile);

    size_t buf_size = (nPix<PIX_BUFFER_SIZE)?nPix:PIX_BUFFER_SIZE;
    if(buf_size!=this->data_buffer_size){
      delete [] this->data_buffer;
      this->data_buffer=NULL;
    }else{
      return;
    }
  }
  unsigned int nDims = this->m_spMDImage->getGeometry()->getNumDims();

  this->box_min.assign(nDims,FLT_MAX);
  this->box_max.assign(nDims,-FLT_MAX);

  field_length.assign(n_fields,4);
  this->set_field_length(field_length);
  size_t nPix= this->getNumPixels(spFile);

  this->data_buffer_size = (nPix<PIX_BUFFER_SIZE)?nPix:PIX_BUFFER_SIZE;

  data_buffer = new char[data_buffer_size*field_start[n_fields]];


}
//***************************************************************************************
MDDataPoints::~MDDataPoints()
{
  if(data_buffer){
    delete [] data_buffer;
    data_buffer = NULL;
  }
  n_data_points=0;
  n_fields     =0;
  field_start.clear();
  field_length.clear();

}

}
}
