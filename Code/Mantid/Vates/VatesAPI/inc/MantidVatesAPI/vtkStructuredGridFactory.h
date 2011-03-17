
#ifndef GENERATESTRUCTUREDGRID_H_
#define GENERATESTRUCTUREDGRID_H_

/** Creates a vtkStructuredGrid (mesh only) from a MDImage.

 @author Owen Arnold, Tessella plc
 @date 11/01/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include <vtkStructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkCellData.h>
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MDDataObjects/MDWorkspace.h"

namespace Mantid
{
namespace VATES
{

template<typename Image, typename TimeMapper>
class DLLExport vtkStructuredGridFactory : public vtkDataSetFactory
{
public:

  /// Constructor
  vtkStructuredGridFactory(boost::shared_ptr<Image> image, const std::string& scalarName, const double timeValue, const TimeMapper& timeMapper);

  /// Assignment operator
  vtkStructuredGridFactory& operator=(const vtkStructuredGridFactory<Image, TimeMapper>& other);

  /// Copy constructor.
  vtkStructuredGridFactory(const vtkStructuredGridFactory<Image, TimeMapper>& other);

  /// Destructor
  ~vtkStructuredGridFactory();

  /// Constructional method. Have to explicitly ask for a mesh only version.
  static vtkStructuredGridFactory constructAsMeshOnly(boost::shared_ptr<Image> image, const TimeMapper& timeMapper);

  /// Factory method
  vtkStructuredGrid* create() const;

  /// Generates the geometry of the mesh only.
  vtkStructuredGrid* createMeshOnly() const;

  /// Generates a scalar array for signal.
  vtkFloatArray* createScalarArray() const;

private:

  /// Private constructor for use by constructional static member
  vtkStructuredGridFactory(boost::shared_ptr<Image> image, const TimeMapper& timeMapper);

  boost::shared_ptr<Image> m_image;
  std::string m_scalarName;
  double m_timeValue;
  bool m_meshOnly;
  TimeMapper m_timeMapper;
};

template<typename Image, typename TimeMapper>
vtkStructuredGridFactory<Image, TimeMapper>::vtkStructuredGridFactory(boost::shared_ptr<Image> image, const std::string& scalarName, const double timeValue, const TimeMapper& timeMapper) :
    m_image(image), m_scalarName(scalarName), m_timeValue(timeValue), m_meshOnly(false), m_timeMapper(timeMapper)
{
}

template<typename Image, typename TimeMapper>
vtkStructuredGridFactory<Image, TimeMapper>::vtkStructuredGridFactory(const vtkStructuredGridFactory<Image, TimeMapper>& other):
    m_image(other.m_image), m_scalarName(other.m_scalarName), m_timeValue(other.m_timeValue), m_meshOnly(other.m_meshOnly), m_timeMapper(other.m_timeMapper)
{
}

template<typename Image, typename TimeMapper>
vtkStructuredGridFactory<Image, TimeMapper> & vtkStructuredGridFactory<Image, TimeMapper>::operator=(const vtkStructuredGridFactory<Image, TimeMapper>& other)
{
    if (this != &other)
    {
        m_meshOnly = other.m_meshOnly;
        m_scalarName = other.m_scalarName;
        m_timeValue = other.m_timeValue;
        m_image = other.m_image;
    }
    return *this;
}


template<typename Image, typename TimeMapper>
vtkStructuredGridFactory<Image, TimeMapper>::vtkStructuredGridFactory(boost::shared_ptr<Image> image, const TimeMapper& timeMapper) : m_image(image), m_scalarName(""), m_timeValue(0),  m_meshOnly(true), m_timeMapper(timeMapper)
{
}

template<typename Image, typename TimeMapper>
vtkStructuredGridFactory<Image, TimeMapper> vtkStructuredGridFactory<Image, TimeMapper>::constructAsMeshOnly(boost::shared_ptr<Image> image, const TimeMapper& timeMapper)
{
   return vtkStructuredGridFactory<Image, TimeMapper>(image, timeMapper);
}

template<typename Image, typename TimeMapper>
vtkStructuredGridFactory<Image, TimeMapper>::~vtkStructuredGridFactory()
{
}

template<typename Image, typename TimeMapper>
vtkStructuredGrid* vtkStructuredGridFactory<Image, TimeMapper>::create() const
{
  vtkStructuredGrid* visualDataSet = this->createMeshOnly();
  vtkFloatArray* scalarData = this->createScalarArray();
  visualDataSet->GetCellData()->AddArray(scalarData);
  scalarData->Delete();
  return visualDataSet;
}

template<typename Image, typename TimeMapper>
vtkStructuredGrid* vtkStructuredGridFactory<Image, TimeMapper>::createMeshOnly() const
{
  typename Image::GeometryType const * const pGeometry = m_image->getGeometry();

  const int nBinsX = pGeometry->getXDimension()->getNBins();
  const int nBinsY = pGeometry->getYDimension()->getNBins();
  const int nBinsZ = pGeometry->getZDimension()->getNBins();

  const double maxX = pGeometry->getXDimension()->getMaximum();
  const double minX = pGeometry->getXDimension()->getMinimum();
  const double maxY = pGeometry->getYDimension()->getMaximum();
  const double minY = pGeometry->getYDimension()->getMinimum();
  const double maxZ = pGeometry->getZDimension()->getMaximum();
  const double minZ = pGeometry->getZDimension()->getMinimum();

  double incrementX = (maxX - minX) / nBinsX;
  double incrementY = (maxY - minY) / nBinsY;
  double incrementZ = (maxZ - minZ) / nBinsZ;

  const int imageSize = (nBinsX + 1) * (nBinsY + 1) * (nBinsZ + 1);
  vtkStructuredGrid* visualDataSet = vtkStructuredGrid::New();
  vtkPoints *points = vtkPoints::New();
  points->Allocate(imageSize);

  //The following represent actual calculated positions.
  double posX, posY, posZ;
  //Loop through dimensions
  double currentXIncrement, currentYIncrement;

  int nPointsX = nBinsX+1;
  int nPointsY = nBinsY+1;
  int nPointsZ = nBinsZ+1;
  for (int i = 0; i < nPointsX; i++)
  {
    currentXIncrement = i*incrementX;
    for (int j = 0; j < nPointsY; j++)
    {
      currentYIncrement = j*incrementY;
      for (int k = 0; k < nPointsZ; k++)
      {
        posX = minX + currentXIncrement; //Calculate increment in x;
        posY = minY + currentYIncrement; //Calculate increment in y;
        posZ = minZ + k*incrementZ; //Calculate increment in z;
        points->InsertNextPoint(posX, posY, posZ);
      }
    }
  }

  //Attach points to dataset.
  visualDataSet->SetPoints(points);
  visualDataSet->SetDimensions(nPointsZ, nPointsY, nPointsX);
  points->Delete();
  return visualDataSet;
}

template<typename Image, typename TimeMapper>
vtkFloatArray* vtkStructuredGridFactory<Image, TimeMapper>::createScalarArray() const
{
  if(true == m_meshOnly)
  {
    throw std::runtime_error("This vtkStructuredGridFactory factory has not been constructed with all the information required to create scalar data.");
  }

  using namespace MDDataObjects;
  //Get geometry nested type information.
  typename Image::GeometryType const * const pGeometry = m_image->getGeometry();

  //Add scalar data to the mesh.
  vtkFloatArray* scalars = vtkFloatArray::New();

  const int sizeX = pGeometry->getXDimension()->getNBins();
  const int sizeY = pGeometry->getYDimension()->getNBins();
  const int sizeZ = pGeometry->getZDimension()->getNBins();
  scalars->Allocate(sizeX * sizeY * sizeZ);
  scalars->SetName(m_scalarName.c_str());
  MD_image_point point;
  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        // Create an image from the point data.

        point = m_image->getPoint(i, j, k, m_timeMapper(m_timeValue));
        // Insert scalar data.
        scalars->InsertNextValue(point.s);
      }
    }
  }
  scalars->Squeeze();
  return scalars;
}

}
}



#endif
