#ifndef MD_IMAGE_H
#define MD_IMAGE_H
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MDImageDatatypes.h"
#include <boost/shared_ptr.hpp>



/**
 * The kernel of the main class for visualisation and analysis operations,
 * which keeps the data itself and brief information about the data dimensions
 * (its organisation in the 1D array)
 *
 * This is equivalent of multidimensional Horace dataset without detailed pixel
 * information (the largest part of dnd dataset).
 *
 *
 * Note, Janik Zikovsky: It is my understanding that:
 *
 * MDImage is a dense representation of a specific slice of a larger data set,
 * generated from a MDWorkspace by (some kind of) rebinning algorithm.
 *

    @author Alex Buts, RAL ISIS
    @date 28/09/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
namespace Mantid{
namespace MDDataObjects{
//
class DLLExport MDImage
{
public:
  /// Embedded type information
  typedef Mantid::Geometry::MDGeometryOld GeometryType;

  /// default constructor
  MDImage(Mantid::Geometry::MDGeometryOld* p_MDGeometry=NULL);

  /// the constructor which builds empty image from geometry the description (calling proper geometry initialise inside)
  MDImage(const Geometry::MDGeometryDescription &Description, const Geometry::MDGeometryBasis & pBasis);

  // destructor
  virtual ~MDImage();

  //**********************************************************************************************************************************
  // Functions to obtain parts of MD image as 3D points for visualisation:
  //
  /** function returns vector of points left after the selection has been applied to the multidimensinal image
   *
   * @param selection :: -- vector of indexes, which specify which dimensions are selected and the location of the selected point
   *                     e.g. selection[0]=10 -- selects the index 10 in the last expanded dimension or
   *                     selection.assign(2,10) for 4-D dataset lead to 2D image extracted from 4D image at range of points (:,:,10,10);
   *                     throws if attempeted to select more dimensions then the number of expanded dimensions
   *                     if the selection index extends beyond of range for an dimension, function returns last point in this dimension.
   */
  void getPointData(const std::vector<unsigned int> &selection,std::vector<point3D> & image_data)const;

  /// the same as getPointData(std::vector<unsigned int> &selection) but select inial (0) coordinates for all dimensions > 3
  void getPointData(std::vector<point3D> & image_data)const;

  /// returns the size of the Image array as 1D array (number of cells)
  size_t getDataSize(void)const{return MD_IMG_array.data_size;}

  /// returns the size occupied by the data part of the MD_IMG_array;
  virtual long getMemorySize()const{return static_cast<long>(MD_IMG_array.data_array_size*sizeof(MD_image_point));}

  /// get constant pointer (reference for GCC not to complain) to geometry for modification in algorithms. (sp may be better?)
  Geometry::MDGeometryOld &  getGeometry(){ return *pMDGeometry; }
  /// get const pointer to const geometry for everything else
  Geometry::MDGeometryOld const & get_const_MDGeometry()const{ return *pMDGeometry; }

  //******************************************************************************************************
  //******************************************************************************************************
  /** initialises image, build with empty constructor, to working state.
   *  If called on existing image, ignores basis and tries to reshape image as stated in description	*/
  virtual void initialize(const Geometry::MDGeometryDescription &Description, const Geometry::MDGeometryBasis *const pBasis=NULL);
  bool is_initialized(void)const;

  /// get acces to the internal image dataset for further modifications; throws if dataset is undefinded;
  MD_image_point      * get_pData(void);
  MD_image_point const* get_const_pData(void)const;

  /// get acces to the whole MD Image data structure;
  MD_img_data         * get_pMDImgData(void){return &MD_IMG_array;}
  MD_img_data  const  & get_MDImgData(void)const{return MD_IMG_array;}


  //*****************************************************************************************************************************************************************
  // IMPORTANT non-trivial functions for an algorithm developer:
  //
  /** function returns the number of primary pixels (MDdatapoints, events) contributed into the image. it is used to verify if the image has been read properly or
   *  read at all and is consistent with the MDDataPoints class in the workspace, because the image is used to calculate the the locations of the MDDataPoints in
   *  the memory and on HDD. As the image and MDDataPoinsts classes were separated
   *  artificially to fit Mantid concepts, a writer of any algorithm which change image have to use correct procedures (below) to modify number of MDDPoints contributing into the image
   *  to be sure that the number of the contributing points is actually equal to the sum of all npix fields in MD_image_point *data array */
  uint64_t getNMDDPoints()const{return MD_IMG_array.npixSum;}

  /** the function used to verify if the npixSum is indeed equal to the sum of all npix fields in MDImage data array; It runs sum of all pixels in image and if this sum is not equal to
   *  actual npixSum, sets the value of MD_IMG_array.npixSum to actual sum of pixels and throws std::invalid_arguments which can be catched and dealt upon */
  void validateNPix(void);

  /** function has to be used by algorithms which change the values of the particular MDDataFiels to modify the value of the pixels sum*/
  void setNpix(uint64_t newNPix){MD_IMG_array.npixSum=newNPix;}
  //*****************************************************************************************************************************************************************


  //Temporary fix to get 1-D image data
  MD_image_point getPoint(size_t index)
  {
    return this->MD_IMG_array.data[index];
  }

  //Temporary fix to get 3-D image data.
  MD_image_point getPoint(size_t i,size_t j)       const
  {
    return this->MD_IMG_array.data[nCell(i, j)];
  }

  //Temporary fix to get 3-D image data.
  MD_image_point getPoint(size_t i,size_t j,size_t k)       const
  {
    return this->MD_IMG_array.data[nCell(i, j, k)];
  }

  //Temporary fix to get4-D image data.
  MD_image_point getPoint(size_t i, size_t j, size_t k, size_t t) const
  {
    return this->MD_IMG_array.data[nCell(i, j, k, t)];
  }

  /// get signal as 1D array; Generally for debug purposes or in case of 1D image;
  double  getSignal(size_t i)     const
  {
    return this->MD_IMG_array.data[i].s;
  }

protected:
  // dimensions strides in linear order; formulated in this way for faster access;
  size_t nd2,nd3,nd4,nd5,nd6,nd7,nd8,nd9,nd10,nd11;

  //*************************************************
  //*************************************************
  //
  // location of cell in 1D data array shaped as 4 or less dimensional array;
  size_t nCell(size_t i)                    const{ return (i);}
  size_t nCell(size_t i,size_t j)              const{ return (i+j*nd2); }
  size_t nCell(size_t i,size_t j,size_t k)        const{ return (i+j*nd2+k*nd3); }
  size_t nCell(size_t i,size_t j,size_t k, size_t n) const{ return (i+j*nd2+k*nd3+n*nd4);}

  /// MD workspace logger
  static Kernel::Logger& g_log;

private:

  std::auto_ptr<Geometry::MDGeometryOld> pMDGeometry;

  //
  MD_img_data    MD_IMG_array;

  // probably temporary
  MDImage & operator = (const MDImage & other);
  // copy constructor;
  MDImage(const MDImage & other);

  /** function reshapes the geomerty of the array according to the pAxis array request; resets the data size in total MD_IMG_array  */
  void reshape_geometry(const Geometry::MDGeometryDescription &transf);

  /// Clear all allocated memory as in the destructor; user usually do not need to call this function unless wants to clear image manually and then initiate it again
  ///	Otherwise it used internaly for reshaping the object for e.g. changing from defaults to something else.
  void clear_class();

  /// function allocates memory for the MD image and resets all necessary auxilary settings;
  void alloc_image_data();

  /// function sets the shape of existing MD array according to MDGeometryOld;
  void set_imgArray_shape();

};
typedef boost::shared_ptr<Mantid::MDDataObjects::MDImage> MDImage_sptr;
//
}
}
#endif
