#ifndef H_MD_Pixels
#define H_MD_Pixels

#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MDDataObjects/MDDataPoint.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include <boost/shared_ptr.hpp>

/** Class to support operations on single data pixels, as obtained from the instrument.
 *  Currently it contains information on the location of the pixel in
    the reciprocal space but this can change as this information
    can be computed at the run time.
    
    @author Alex Buts, RAL ISIS
    @date 01/10/2010

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

namespace Mantid
{

namespace MDDataObjects
{

  class MDImage;
  class MDDataPointsDescription: public MDPointDescription
  {
  public:
	  MDDataPointsDescription(const MDPointDescription &descr):
	  /// this one describes structure of single pixel as it is defined and written on HDD
      MDPointDescription(descr)
	  { }
  private:
	  // this has not been defined at the moment, but will provide the description for possible lookup tables
	  std::vector<double> lookup_tables;
  };
  
  //**********************************************************************************************************************
  class DLLExport MDDataPoints
  {
  public:
    MDDataPoints(boost::shared_ptr<const MDImage> pImageData,const MDDataPointsDescription &description);
    ~MDDataPoints();

    /// check if the pixels are all in memory;
    bool isMemoryBased(void)const{return memBased;}
    /// function returns numnber of pixels (dataPoints) contributiong into the MD-data
    size_t getNumPixels(boost::shared_ptr<IMD_FileFormat> spFile);

    size_t getMemorySize()const{return data_buffer_size*1;}

    // Accessors & Mutators used mainly for IO Operations on the dataset
    /// function returns minimal value for dimension i
    double &rPixMin(unsigned int i){return *(&box_min[0]+i);}
    /// function returns maximal value for dimension i
    double &rPixMax(unsigned int i){return *(&box_max[0]+i);}
     /// returns the pointer to the location of the data buffer; the data has to be processed through MDDataPoint then
    void * get_pBuffer(void){return data_buffer;}
    /// return the size of the buffer allocated for pixels
    size_t get_pix_bufSize(void)const{return data_buffer_size;} //TODO: refactor out.
     // initiates memory for part of the pixels, which should be located in memory;
    void alloc_pix_array(boost::shared_ptr<IMD_FileFormat> spFile);

    /** function returns the part of the colum-names which corresponds to the dimensions information;
     * the order of the ID corresponds to the order of the data in the datatables */
    std::vector<std::string> getDimensionsID(void)const{return description.getDimensionsID();}
  protected:

    /** the parameter identify if the class data are file or memory based
     * usually it is le based and memory used for small datasets, debugging
     * or in a future when PC are big
     */
    bool memBased;

  private:
	  // The class which describes the structure of sinle data point (pixel)
	 MDDataPointsDescription description;

    /// the data, describing the detector pixels
    unsigned long  n_data_points;  //< number of data points contributing in dataset

  
    /// minimal values of ranges the data pixels are in; size is nDimensions
    std::vector<double> box_min;
    /// maximal values of ranges the data pixels are in; size is nDimensions
    std::vector<double> box_max;

     //
    unsigned int pixel_size;        //<the size of the pixel (DataPoint)(single point of data in reciprocal space) in bytes
    size_t  data_buffer_size;       //< size the data buffer in pixels (data_points) rather then in char;
    void *data_buffer;

    // private for the time being but may be needed in a future
    MDDataPoints(const MDDataPoints& p);
    MDDataPoints & operator = (const MDDataPoints & other);

    boost::shared_ptr<const MDImage> m_spMDImage; //Allows access to current geometry owned by MDImage.
 
    /// Common logger for logging all MD Workspace operations;
    static Kernel::Logger& g_log;
  };
}
}
#endif
