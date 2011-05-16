#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/TimeToTimeStep.h"

namespace Mantid
{
namespace VATES
{
  template<typename TimeMapper>
  vtkStructuredGridFactory<TimeMapper>::vtkStructuredGridFactory(const std::string& scalarName, const double timeValue) : 
  m_scalarName(scalarName), m_timeValue(timeValue), m_meshOnly(false)
  {
  }

  template<typename TimeMapper>
  vtkStructuredGridFactory<TimeMapper>::vtkStructuredGridFactory(const vtkStructuredGridFactory<TimeMapper>& other):
  m_workspace(other.m_workspace), m_scalarName(other.m_scalarName), m_timeValue(other.m_timeValue), m_meshOnly(other.m_meshOnly)
  {
  }

  template<typename TimeMapper>
  vtkStructuredGridFactory<TimeMapper> & vtkStructuredGridFactory<TimeMapper>::operator=(const vtkStructuredGridFactory<TimeMapper>& other)
  {
    if (this != &other)
    {
      m_meshOnly = other.m_meshOnly;
      m_scalarName = other.m_scalarName;
      m_timeValue = other.m_timeValue;
      m_workspace = other.m_workspace;
    }
    return *this;
  }


  template<typename TimeMapper>
  vtkStructuredGridFactory<TimeMapper>::vtkStructuredGridFactory() :  m_scalarName(""), m_timeValue(0),  m_meshOnly(true)
  {
  }

  template<typename TimeMapper>
  vtkStructuredGridFactory<TimeMapper> vtkStructuredGridFactory<TimeMapper>::constructAsMeshOnly()
  {
    return vtkStructuredGridFactory<TimeMapper>();
  }

  template<typename TimeMapper>
  vtkStructuredGridFactory<TimeMapper>::~vtkStructuredGridFactory()
  {
  }

  template<typename TimeMapper>
  void vtkStructuredGridFactory<TimeMapper>::initialize(Mantid::API::IMDWorkspace_sptr workspace)
  {
    m_workspace = workspace;
    validate();

    double tMax = m_workspace->getTDimension()->getMaximum();
    double tMin = m_workspace->getTDimension()->getMinimum();
    size_t nbins = m_workspace->getTDimension()->getNBins();

    m_timeMapper = TimeMapper::construct(tMin, tMax, nbins);
  }

  template<typename TimeMapper>
  void vtkStructuredGridFactory<TimeMapper>::validate() const
  {
    if(NULL == m_workspace.get())
    {
      throw std::runtime_error("IMDWorkspace is null");
    }
    else
    {
      if(NULL == m_workspace->getTDimension().get())
      {
        throw std::runtime_error("Missing time dimension in IMDWorkspace.");
      }
    }
  }

  template<typename TimeMapper>
  vtkStructuredGrid* vtkStructuredGridFactory<TimeMapper>::create() const
  {
    validate();
    vtkStructuredGrid* visualDataSet = this->createMeshOnly();
    vtkFloatArray* scalarData = this->createScalarArray();
    visualDataSet->GetCellData()->AddArray(scalarData);
    scalarData->Delete();
    return visualDataSet;
  }

  template<typename TimeMapper>
  vtkStructuredGrid* vtkStructuredGridFactory<TimeMapper>::createMeshOnly() const
  {
    validate();
    const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
    const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );
    const int nBinsZ = static_cast<int>( m_workspace->getZDimension()->getNBins() );

    const double maxX = m_workspace->getXDimension()->getMaximum();
    const double minX = m_workspace->getXDimension()->getMinimum();
    const double maxY = m_workspace->getYDimension()->getMaximum();
    const double minY = m_workspace->getYDimension()->getMinimum();
    const double maxZ = m_workspace->getZDimension()->getMaximum();
    const double minZ = m_workspace->getZDimension()->getMinimum();

    double incrementX = (maxX - minX) / nBinsX;
    double incrementY = (maxY - minY) / nBinsY;
    double incrementZ = (maxZ - minZ) / nBinsZ;

    const size_t imageSize = (nBinsX + 1) * (nBinsY + 1) * (nBinsZ + 1);
    vtkStructuredGrid* visualDataSet = vtkStructuredGrid::New();
    vtkPoints *points = vtkPoints::New();
    points->Allocate(static_cast<int>(imageSize));

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

  template<typename TimeMapper>
  vtkFloatArray* vtkStructuredGridFactory<TimeMapper>::createScalarArray() const
  {
    validate();
    if(true == m_meshOnly)
    {
      throw std::runtime_error("This vtkStructuredGridFactory factory has not been constructed with all the information required to create scalar data.");
    }

    //Add scalar data to the mesh.
    vtkFloatArray* scalars = vtkFloatArray::New();

    const int sizeX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
    const int sizeY = static_cast<int>( m_workspace->getYDimension()->getNBins() );
    const int sizeZ = static_cast<int>( m_workspace->getZDimension()->getNBins() );
    scalars->Allocate(sizeX * sizeY * sizeZ);
    scalars->SetName(m_scalarName.c_str());

    for (int i = 0; i < sizeX; i++)
    {
      for (int j = 0; j < sizeY; j++)
      {
        for (int k = 0; k < sizeZ; k++)
        {
          // Create an image from the point data.
          double signal =  m_workspace->getSignalAt(i, j, k, m_timeMapper(m_timeValue));
          // Insert scalar data.
          scalars->InsertNextValue(static_cast<float>(signal));
        }
      }
    }
    scalars->Squeeze();
    return scalars;
  }

  template class vtkStructuredGridFactory<TimeStepToTimeStep>;

  template class vtkStructuredGridFactory<TimeToTimeStep>;

}
}

