#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidKernel/IPropertyManager.h"
#include "MDDataObjects/MDIndexCalculator.h"

namespace Mantid{
  namespace MDDataObjects{
	  using namespace Kernel;
	  using namespace Geometry;
	  using namespace API;


    // Register the workspace into the WorkspaceFactory
    DECLARE_WORKSPACE(MDWorkspace)


    // logger for MD workspaces  
    Kernel::Logger& MDWorkspace::g_log =Kernel::Logger::get("MDWorkspaces");


    //----------------------------------------------------------------------------------------------
     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> getDataPoints(boost::shared_ptr<MDImage> imageData)
     {
	   MDPointDescription descr;
       return  boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints>(new MDDataPoints(descr)); //TODO replace with some other factory call.
     }

     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDImage> getImageData(Mantid::Geometry::MDGeometry* geometry)
     {
       return boost::shared_ptr<Mantid::MDDataObjects::MDImage>(new MDImage(geometry));
	 }
//
//
     void MDWorkspace::init(boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile, Mantid::Geometry::MDGeometry* geometry) //TODO: this provides a 'seam' for simplier move to DataHandling in future.
     {
       this->m_spFile = spFile;
       this->m_spMDImage = getImageData(geometry);
       this->m_spDataPoints = getDataPoints(m_spMDImage); //Takes a pointer to the image data in order to be able to extract an up-to-date geometry.
       if(NULL != m_spFile.get())
       {
         this->m_spDataPoints->initialize(m_spMDImage, m_spFile);
         this->m_spFile->read_MDImg_data(*m_spMDImage);
       }
     }
//
void
MDWorkspace::init(std::auto_ptr<IMD_FileFormat> pFile,
                std::auto_ptr<Geometry::MDGeometryBasis> pBasis,
                const Geometry::MDGeometryDescription &geomDescr,
                const MDPointDescription &pd)
{
    // store file reader,
    this->m_spFile   = boost::shared_ptr<IMD_FileFormat>(pFile.get());
    pFile.release();
    // store basis (description?)
    this->m_spMDBasis =boost::shared_ptr<Geometry::MDGeometryBasis>(pBasis.get());
    pBasis.release();

    // create new empty image form of its description and the basis
    this->m_spMDImage  = boost::shared_ptr<MDImage>(new MDImage(geomDescr,*m_spMDBasis));

   // read MDImage data (the keys to the datapoints location) 
	this->m_spFile->read_MDImg_data(*m_spMDImage);
  
   	// constructor MD points-
	this->m_spDataPoints = boost::shared_ptr<MDDataPoints>(new MDDataPoints(pd));
  
    this->m_spDataPoints->initialize(m_spMDImage,m_spFile);
 
 
}

//
void
MDWorkspace::init(boost::shared_ptr<const MDWorkspace> SourceWorkspace,const Mantid::Geometry::MDGeometryDescription *const transf)
{
	this->m_spMDBasis = boost::shared_ptr<MDGeometryBasis>(new MDGeometryBasis(SourceWorkspace->get_const_MDBaisis()));

	// build old or new geometry
	std::auto_ptr<MDGeometry> pGeometry;
	// no changes to new workspace is defined and we are initiating the new workspace as a copy of an old workspace;
	if(!transf){
		std::auto_ptr<MDGeometryDescription> oldShape = std::auto_ptr<MDGeometryDescription>(new MDGeometryDescription(*SourceWorkspace->getGeometry()));
		// we have basis and description, now can build geometry
		pGeometry = std::auto_ptr<MDGeometry>(new MDGeometry(*m_spMDBasis,*oldShape));
	}else{
		pGeometry = std::auto_ptr<MDGeometry>(new MDGeometry(*m_spMDBasis,*transf));
	}
	//
   this->m_spMDImage = boost::shared_ptr<MDImage>(new MDImage(pGeometry.get()));
   // free the pGeometry as it is now resides with MDImage and should not be deleted by auto_ptr;
   pGeometry.release();

    // MDDataPoints have to be constructed here and intiated later after the image is build and points need to be saved
   // fileManager has to be initated for writing workspace but this will happens only when saveWorkspace algorithm is 
   // called and new file name is known. Temporary file manager has to be created if and when datapoint writing is necessary;
   MDDataPointsDescription pixDescr = SourceWorkspace->get_const_MDDPoints().getMDPointDescription();
   this->m_spDataPoints = boost::shared_ptr<MDDataPoints>(new MDDataPoints(pixDescr));
  
   //this->DDataPoints
}

    /** Default constructor - does nothing */
    MDWorkspace::MDWorkspace(unsigned int nDimensions, unsigned int nRecDims)
    {
    }

    //----------------------------------------------------------------------------------------------
   // void MDWorkspace::read_MDImg()
   // {
   //   Geometry::MDGeometryDescription Description;
	  //this->m_spFile->read_MDGeomDescription(Description);
	  ////
	  //this->m_spMDImage->initialize(Description);
   //   //  read image part of the data
   //   this->m_spFile->read_MDImg_data(*this->m_spMDImage);
   //   // alocate memory for pixels;
   //   m_spDataPoints->alloc_pix_array(m_spFile);
   //   m_spMDImage->identify_SP_points_locations();
   // }
    //
    void
    MDWorkspace::read_pix(void)
    {
      if(this->m_spFile.get()){
        m_spFile->read_pix(*m_spDataPoints);
      }else{
		  throw(std::runtime_error("read_pix: file reader has not been defined"));
      }


    }

    size_t 
    MDWorkspace::read_pix_selection(const std::vector<size_t> &cells_nums, size_t &start_cell, std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
    {
      if(!this->m_spFile.get()){
		  throw(std::runtime_error("MDPixels::read_selected_pix: file reader has not been defined"));
      }
      return this->m_spFile->read_pix_subset(*m_spMDImage,cells_nums,start_cell,pix_buf,n_pix_in_buffer);
    } 

    Mantid::Geometry::MDGeometry const * const
      MDWorkspace::getGeometry() const
    {
      return this->m_spMDImage->getGeometry();
    }

    size_t MDWorkspace::getMemorySize(void) const
    {
      return m_spMDImage->getMemorySize() + m_spDataPoints->getMemorySize() ;
    } 

    void  MDWorkspace::write_mdd(void)
    {
      if(this->m_spFile.get()){
         this->m_spFile->write_mdd(*m_spMDImage);
      }else{
		  throw(std::runtime_error("MDPixels::read_selected_pix: file reader has not been defined"));
      }
    }

    void  MDWorkspace::setInstrument(const IInstrument_sptr& instr)
    {
      boost::shared_ptr<Instrument> tmp = boost::dynamic_pointer_cast<Instrument>(instr);
      if (tmp->isParametrized())
      {
        sptr_instrument = tmp->baseInstrument();
        m_parmap = tmp->getParameterMap();
      }
      else
      {
        sptr_instrument=tmp;
      }
    }


    uint64_t MDWorkspace::getNPoints() const
    {
        return this->m_spDataPoints->getNumPixels();
    }

    int MDWorkspace::getNDimensions() const
    {
      return m_spMDImage->getGeometry()->getNumDims();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getXDimension() const
    { 
      return m_spMDImage->getGeometry()->getXDimension(); 
    }

    boost::shared_ptr< const Mantid::Geometry::IMDDimension> MDWorkspace::getYDimension() const
    { 
      return m_spMDImage->getGeometry()->getYDimension();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getZDimension() const
    { 
      return m_spMDImage->getGeometry()->getZDimension();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::gettDimension() const
    { 
      return m_spMDImage->getGeometry()->getTDimension();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getDimension(std::string id) const
    { 
		return m_spMDImage->getGeometry()->get_constDimension(id,true); 
    }

    const std::vector<std::string> MDWorkspace::getDimensionIDs() const
    {
      MDGeometry const * const geometry = m_spMDImage->getGeometry();
      std::vector<boost::shared_ptr<IMDDimension> > vecDimensions = geometry->getDimensions();
      std::vector<std::string> vecDimensionIds(vecDimensions.size());
      for(unsigned int i = 0; i < vecDimensions.size() ; i++)
      {
        vecDimensionIds[i] = vecDimensions[i]->getDimensionId();
      }
      return vecDimensionIds;
    }

    const Mantid::Geometry::SignalAggregate & MDWorkspace::getPoint(unsigned int index) const
    {
      if(index >= this->getNPoints())
      {
        throw std::range_error("Requested point is out of range.");
      }
      std::vector<char>* pix_buf = m_spDataPoints->get_pBuffer();
      float *MDDataPoint =(float *)(&(pix_buf->operator[](0)));
      unsigned int signal_shift = this->getNDimensions();
      unsigned int data_stride  =  m_spDataPoints->getMDPointDescription().sizeofMDDPoint()/sizeof(float);
      size_t base = index*data_stride;
      double signal  = *(MDDataPoint+base+signal_shift);
      double error= *(MDDataPoint+base+signal_shift+1);

      std::vector<coordinate> vertexes;

      IDetector_sptr detector; //TODO determine detector.

      MDPointMap::const_iterator iter = m_mdPointMap.find(index);
      if(m_mdPointMap.end() ==  iter || (*iter).second.getSignal() != signal || (*iter).second.getError() != error)
      {
        m_mdPointMap[index] =  Mantid::Geometry::MDPoint(signal, error, vertexes, detector, this->sptr_instrument);
      }
      return m_mdPointMap[index];
    }

    inline bool MDWorkspace::newCellRequired(const size_t& singleDimensionIndex, const MD_image_point& mdImagePoint) const
    {
      MDCellMap::const_iterator iter = m_mdCellMap.find(singleDimensionIndex);
      //Current rules for determing whether a cell has changed.
      return m_mdCellMap.end() ==  iter || 
        (*iter).second.getSignal() != mdImagePoint.s || 
        (*iter).second.getError() != mdImagePoint.err;
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(unsigned int dim1Increment) const
    {
      MD_image_point point = m_spMDImage->getPoint(dim1Increment);
      MDGeometry const * const geometry = m_spMDImage->getGeometry();
      IMDDimension_sptr xDimension = geometry->getXDimension();

      MDCellMap::const_iterator iter = m_mdCellMap.find(dim1Increment);
      if(true == newCellRequired(dim1Increment, point))
      {
        VecCoordinate vertexes = createLine(dim1Increment, xDimension);
        m_mdCellMap[dim1Increment] = MDCell(point.s, point.err, vertexes);
      }
      return m_mdCellMap[dim1Increment];
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(unsigned int dim1Increment, unsigned int dim2Increment) const
    {
      MD_image_point point = m_spMDImage->getPoint(dim1Increment, dim2Increment);
      MDGeometry const * const geometry = m_spMDImage->getGeometry();
      IMDDimension_sptr xDimension = geometry->getXDimension();
      IMDDimension_sptr yDimension = geometry->getYDimension();

      //The cell map is agnostic of the dimensionality. Request needs to be forulated into a single dimensional form.
      MDWorkspaceIndexCalculator<2> calculator;
      calculator.setDimensionSize(0, xDimension->getNBins());
      calculator.setDimensionSize(1, yDimension->getNBins());
      std::vector<size_t> indexes(2);
      indexes[0] = dim1Increment;
      indexes[1] = dim2Increment;
      size_t singleDimensionIndex = calculator.calculateSingleDimensionIndex(indexes);

      if(singleDimensionIndex > calculator.getIndexUpperBounds())
      {
        throw std::range_error("Requested cell is out of range.");
      }

      if(true == newCellRequired(singleDimensionIndex, point))
      {
        VecCoordinate vertexes = createPolygon(dim1Increment, dim2Increment, xDimension, yDimension);
        m_mdCellMap[singleDimensionIndex] =  Mantid::Geometry::MDCell(point.s, point.err, vertexes);
      }
      return m_mdCellMap[singleDimensionIndex];
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment) const
    {
      MD_image_point point = m_spMDImage->getPoint(dim1Increment, dim2Increment, dim3Increment);
      MDGeometry const * const geometry = m_spMDImage->getGeometry();
      IMDDimension_sptr xDimension = geometry->getXDimension();
      IMDDimension_sptr yDimension = geometry->getYDimension();
      IMDDimension_sptr zDimension = geometry->getZDimension();

      //The cell map is agnostic of the dimensionality. Request needs to be forulated into a single dimensional form.
      MDWorkspaceIndexCalculator<3> calculator;
      calculator.setDimensionSize(0, xDimension->getNBins());
      calculator.setDimensionSize(1, yDimension->getNBins());
      calculator.setDimensionSize(2, zDimension->getNBins());
      size_t indexes[] = {dim1Increment, dim2Increment, dim3Increment};
      VecIndexes vecIndexes(3);
      std::copy(indexes, indexes+3, vecIndexes.begin());
      size_t singleDimensionIndex = calculator.calculateSingleDimensionIndex(vecIndexes);

      if(singleDimensionIndex > calculator.getIndexUpperBounds())
      {
        throw std::range_error("Requested cell is out of range.");
      }
      if(true == newCellRequired(singleDimensionIndex, point))
      {
        VecCoordinate vertexes = createPolyhedron(dim1Increment, dim2Increment, dim3Increment, xDimension, yDimension, zDimension);
        m_mdCellMap[singleDimensionIndex] =  Mantid::Geometry::MDCell(point.s, point.err, vertexes);
      }
      return m_mdCellMap[singleDimensionIndex];
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment, unsigned int dim4Increment) const
    {
      MD_image_point point = m_spMDImage->getPoint(dim1Increment, dim2Increment, dim3Increment, dim4Increment);
      MDGeometry const * const geometry = m_spMDImage->getGeometry();
      IMDDimension_sptr xDimension = geometry->getXDimension();
      IMDDimension_sptr yDimension = geometry->getYDimension();
      IMDDimension_sptr zDimension = geometry->getZDimension();
      IMDDimension_sptr tDimension = geometry->getZDimension();

      //The cell map is agnostic of the dimensionality. Request needs to be forulated into a single dimensional form.
      MDWorkspaceIndexCalculator<4> calculator;
      calculator.setDimensionSize(0, xDimension->getNBins());
      calculator.setDimensionSize(1, yDimension->getNBins());
      calculator.setDimensionSize(2, zDimension->getNBins());
      calculator.setDimensionSize(3, tDimension->getNBins());
      size_t indexes[] = {dim1Increment, dim2Increment, dim3Increment, dim4Increment};
      VecIndexes vecIndexes(4);
      std::copy(indexes, indexes+4, vecIndexes.begin());
      size_t singleDimensionIndex = calculator.calculateSingleDimensionIndex(vecIndexes);

      if(singleDimensionIndex > calculator.getIndexUpperBounds())
      {
        throw std::range_error("Requested cell is out of range.");
      }
      if(true == newCellRequired(singleDimensionIndex, point))
      {
        VecCoordinate vertexes = create4DPolyhedron(dim1Increment, dim2Increment, dim3Increment, dim4Increment, xDimension, yDimension, zDimension, tDimension);
        m_mdCellMap[singleDimensionIndex] =  Mantid::Geometry::MDCell(point.s, point.err, vertexes);
      }
      return m_mdCellMap[singleDimensionIndex];
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(...) const
    {
      throw std::runtime_error("Not implemented");
    }

    std::string MDWorkspace::getWSLocation() const 
    {
      //Forward request to file format.
      return this->m_spFile->getFileName();
    }

    std::string MDWorkspace::getGeometryXML() const
    {
      //Forward request via image and geometry.
      return this->m_spMDImage->getGeometry()->toXMLString();
    }

    VecCoordinate create4DPolyhedron(
        unsigned int dim1Increment,
        unsigned int dim2Increment,
        unsigned int dim3Increment,
        unsigned int dim4Increment,
        IMDDimension_sptr xDimension,
        IMDDimension_sptr yDimension,
        IMDDimension_sptr zDimension,
        IMDDimension_sptr tDimension)
{
  double delta_x = (xDimension->getMaximum() - xDimension->getMinimum()) / xDimension->getNBins();
  double delta_y = (yDimension->getMaximum() - yDimension->getMinimum()) / yDimension->getNBins();
  double delta_z = (zDimension->getMaximum() - zDimension->getMinimum()) / zDimension->getNBins();
  double delta_t = (tDimension->getMaximum() - tDimension->getMinimum()) / tDimension->getNBins();
  //Make two Hexahedrons at each time interval.
  VecCoordinate vertexes(16);
  vertexes[0] = coordinate::createCoordinate4D(dim1Increment * delta_x, dim2Increment * delta_y, dim3Increment * delta_z,dim4Increment);
  vertexes[1] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, dim2Increment * delta_y, dim3Increment* delta_z, dim4Increment);
  vertexes[2] = coordinate::createCoordinate4D(dim1Increment * delta_x, (dim2Increment + 1) * delta_y, dim3Increment* delta_z, dim4Increment);
  vertexes[3] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, (dim2Increment + 1) * delta_y, dim3Increment* delta_z, dim4Increment);
  vertexes[4] = coordinate::createCoordinate4D(dim1Increment * delta_x, dim2Increment * delta_y, (dim3Increment + 1)* delta_z, dim4Increment);
  vertexes[5] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, dim2Increment * delta_y, (dim3Increment + 1)* delta_z, dim4Increment);
  vertexes[6] = coordinate::createCoordinate4D(dim1Increment * delta_x, (dim2Increment + 1) * delta_y, (dim3Increment + 1)* delta_z, dim4Increment);
  vertexes[7] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, (dim2Increment + 1) * delta_y, (dim3Increment+ 1) * delta_z, dim4Increment);
  vertexes[8] = coordinate::createCoordinate4D(dim1Increment * delta_x, dim2Increment * delta_y, dim3Increment * delta_z,delta_t + dim4Increment);
  vertexes[9] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, dim2Increment * delta_y, dim3Increment* delta_z, delta_t + dim4Increment);
  vertexes[10] = coordinate::createCoordinate4D(dim1Increment * delta_x, (dim2Increment + 1) * delta_y, dim3Increment* delta_z, delta_t + dim4Increment);
  vertexes[11] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, (dim2Increment + 1) * delta_y, dim3Increment* delta_z, delta_t + dim4Increment);
  vertexes[12] = coordinate::createCoordinate4D(dim1Increment * delta_x, dim2Increment * delta_y, (dim3Increment + 1)* delta_z, delta_t + dim4Increment);
  vertexes[13] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, dim2Increment * delta_y, (dim3Increment + 1)* delta_z, delta_t + dim4Increment);
  vertexes[14] = coordinate::createCoordinate4D(dim1Increment * delta_x, (dim2Increment + 1) * delta_y, (dim3Increment + 1)* delta_z, delta_t + dim4Increment);
  vertexes[15] = coordinate::createCoordinate4D((dim1Increment + 1) * delta_x, (dim2Increment + 1) * delta_y, (dim3Increment+ 1) * delta_z, delta_t + dim4Increment);
  return vertexes;
}

VecCoordinate createPolyhedron(
    unsigned int dim1Increment,
    unsigned int dim2Increment,
    unsigned int dim3Increment,
    IMDDimension_sptr xDimension,
    IMDDimension_sptr yDimension,
    IMDDimension_sptr zDimension)
{
  double delta_x = (xDimension->getMaximum() - xDimension->getMinimum()) / xDimension->getNBins();
  double delta_y = (yDimension->getMaximum() - yDimension->getMinimum()) / yDimension->getNBins();
  double delta_z = (zDimension->getMaximum() - zDimension->getMinimum()) / zDimension->getNBins();
  //Make a Hexahedron
  VecCoordinate vertexes(8);
  vertexes[0] = coordinate::createCoordinate3D(dim1Increment * delta_x, dim2Increment * delta_y, dim3Increment * delta_z);
  vertexes[1] = coordinate::createCoordinate3D((dim1Increment + 1) * delta_x, dim2Increment * delta_y, dim3Increment* delta_z);
  vertexes[2] = coordinate::createCoordinate3D(dim1Increment * delta_x, (dim2Increment + 1) * delta_y, dim3Increment* delta_z);
  vertexes[3] = coordinate::createCoordinate3D((dim1Increment + 1) * delta_x, (dim2Increment + 1) * delta_y, dim3Increment* delta_z);
  vertexes[4] = coordinate::createCoordinate3D(dim1Increment * delta_x, dim2Increment * delta_y, (dim3Increment + 1)* delta_z);
  vertexes[5] = coordinate::createCoordinate3D((dim1Increment + 1) * delta_x, dim2Increment * delta_y, (dim3Increment + 1)* delta_z);
  vertexes[6] = coordinate::createCoordinate3D(dim1Increment * delta_x, (dim2Increment + 1) * delta_y, (dim3Increment + 1)* delta_z);
  vertexes[7] = coordinate::createCoordinate3D((dim1Increment + 1) * delta_x, (dim2Increment + 1) * delta_y, (dim3Increment+ 1) * delta_z);
  return vertexes;
}

VecCoordinate createPolygon(
    unsigned int dim1Increment,
    unsigned int dim2Increment,
    IMDDimension_sptr xDimension,
    IMDDimension_sptr yDimension)
{
  double delta_x = (xDimension->getMaximum() - xDimension->getMinimum()) / xDimension->getNBins();
  double delta_y = (yDimension->getMaximum() - yDimension->getMinimum()) / yDimension->getNBins();
  //Make a square
  VecCoordinate vertexes(4);
  vertexes[0] = coordinate::createCoordinate2D(dim1Increment * delta_x, dim2Increment * delta_y);
  vertexes[1] = coordinate::createCoordinate2D((dim1Increment + 1) * delta_x, dim2Increment * delta_y);
  vertexes[2] = coordinate::createCoordinate2D(dim1Increment * delta_x, (dim2Increment + 1) * delta_y);
  vertexes[3] = coordinate::createCoordinate2D((dim1Increment + 1) * delta_x, (dim2Increment + 1) * delta_y);
  return vertexes;
}

VecCoordinate createLine(
    unsigned int dim1Increment,
    IMDDimension_sptr xDimension)
{
  double delta_x = (xDimension->getMaximum() - xDimension->getMinimum()) / xDimension->getNBins();
  VecCoordinate vertexes(2);
  //Make a line
  vertexes[0] = coordinate::createCoordinate1D(dim1Increment * delta_x);
  vertexes[1] = coordinate::createCoordinate1D((dim1Increment + 1) * delta_x);
  return vertexes;
}

} // namespace
}
//*********************************************************************************************************************************************************************************
namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
      Mantid::MDDataObjects::MDWorkspace_sptr IPropertyManager::getValue<Mantid::MDDataObjects::MDWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
      Mantid::MDDataObjects::MDWorkspace_const_sptr IPropertyManager::getValue<Mantid::MDDataObjects::MDWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_const_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_const_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // name


