#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidAPI/IMDWorkspace.h"

using Mantid::API::IMDWorkspace;

namespace Mantid
{
namespace VATES
{

  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::vtkThresholdingUnstructuredGridFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const double timestep) :
  m_timestep(timestep), m_scalarName(scalarName), m_thresholdRange(thresholdRange)
  {
  }

    /**
  Assigment operator
  @param other : vtkThresholdingHexahedronFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>& vtkThresholdingUnstructuredGridFactory<TimeMapper>::operator=(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other)
  {
    if(this != &other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_thresholdRange = other.m_thresholdRange;
      this->m_workspace = other.m_workspace;
      this->m_timestep = other.m_timestep;
      this->m_timeMapper = other.m_timeMapper;
    }
    return *this;
  }

  /**
  Copy Constructor
  @param other : instance to copy from.
  */
  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::vtkThresholdingUnstructuredGridFactory(const vtkThresholdingUnstructuredGridFactory<TimeMapper>& other)
    : m_workspace(other.m_workspace), m_timestep(other.m_timestep), m_scalarName( other.m_scalarName), m_timeMapper(other.m_timeMapper), m_thresholdRange(other.m_thresholdRange)
  {
    
  }


  template<typename TimeMapper>
  void vtkThresholdingUnstructuredGridFactory<TimeMapper>::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(workspace);
    // Check that a workspace has been provided.
    validateWsNotNull();
    // When the workspace can not be handled by this type, take action in the form of delegation.
    size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if(nonIntegratedSize != vtkDataSetFactory::FourDimensional)
    {
      if(this->hasSuccessor())
      {
        m_successor->initialize(m_workspace);
        return;
      }
      else
      {
        throw std::runtime_error("There is no successor factory set for this vtkThresholdingUnstructuredGridFactory type");
      }
    }

    double tMax = m_workspace->getTDimension()->getMaximum();
    double tMin = m_workspace->getTDimension()->getMinimum();
    size_t nbins = m_workspace->getTDimension()->getNBins();

    m_timeMapper = TimeMapper::construct(tMin, tMax, nbins);

    //Setup range values according to whatever strategy object has been injected.
    m_thresholdRange->setWorkspace(m_workspace);
    m_thresholdRange->calculate();
  }

  template<typename TimeMapper>
  void vtkThresholdingUnstructuredGridFactory<TimeMapper>::validateWsNotNull() const
  {
    
    if(NULL == m_workspace.get())
    {
      throw std::runtime_error("IMDWorkspace is null");
    }
  }
  

  template<typename TimeMapper>
  void vtkThresholdingUnstructuredGridFactory<TimeMapper>::validate() const
  {
    validateWsNotNull();
  }

  template<typename TimeMapper>
  vtkDataSet* vtkThresholdingUnstructuredGridFactory<TimeMapper>::create() const
  {
    validate();

    size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
    if(nonIntegratedSize != vtkDataSetFactory::FourDimensional)
    {
      return m_successor->create();
    }

    else
    {
      const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
      const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );
      const int nBinsZ = static_cast<int>( m_workspace->getZDimension()->getNBins() );

      const double maxX = m_workspace-> getXDimension()->getMaximum();
      const double minX = m_workspace-> getXDimension()->getMinimum();
      const double maxY = m_workspace-> getYDimension()->getMaximum();
      const double minY = m_workspace-> getYDimension()->getMinimum();
      const double maxZ = m_workspace-> getZDimension()->getMaximum();
      const double minZ = m_workspace-> getZDimension()->getMinimum();

      double incrementX = (maxX - minX) / (nBinsX-1);
      double incrementY = (maxY - minY) / (nBinsY-1);
      double incrementZ = (maxZ - minZ) / (nBinsZ-1);

      const int imageSize = (nBinsX ) * (nBinsY ) * (nBinsZ );
      vtkPoints *points = vtkPoints::New();
      points->Allocate(static_cast<int>(imageSize));

      vtkFloatArray * signal = vtkFloatArray::New();
      signal->Allocate(imageSize);
      signal->SetName(m_scalarName.c_str());
      signal->SetNumberOfComponents(1);

      //The following represent actual calculated positions.
      double posX, posY, posZ;

      UnstructuredPoint unstructPoint;
      float signalScalar;
      const int nPointsX = nBinsX;
      const int nPointsY = nBinsY;
      const int nPointsZ = nBinsZ;
      PointMap pointMap(nPointsX);

      //Loop through dimensions
      for (int i = 0; i < nPointsX; i++)
      {
        posX = minX + (i * incrementX); //Calculate increment in x;
        Plane plane(nPointsY);
        for (int j = 0; j < nPointsY; j++)
        {

          posY = minY + (j * incrementY); //Calculate increment in y;
          Column col(nPointsZ);
          for (int k = 0; k < nPointsZ; k++)
          {

            posZ = minZ + (k * incrementZ); //Calculate increment in z;
            signalScalar = static_cast<float>(m_workspace->getSignalNormalizedAt(i, j, k, m_timeMapper(m_timestep)));

            if (boost::math::isnan( signalScalar ) || !m_thresholdRange->inRange(signalScalar))
            {
              //Flagged so that topological and scalar data is not applied.
              unstructPoint.isSparse = true;
            }
            else
            {
              if ((i < (nBinsX -1)) && (j < (nBinsY - 1)) && (k < (nBinsZ -1)))
              {
                signal->InsertNextValue(signalScalar);
              }
              unstructPoint.isSparse = false;
            }
            unstructPoint.pointId = points->InsertNextPoint(posX, posY, posZ);
            col[k] = unstructPoint;
          }
          plane[j] = col;
        }
        pointMap[i] = plane;
      }

      points->Squeeze();
      signal->Squeeze();

      vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
      visualDataSet->Allocate(imageSize);
      visualDataSet->SetPoints(points);
      visualDataSet->GetCellData()->SetScalars(signal);

      for (int i = 0; i < nBinsX - 1; i++)
      {
        for (int j = 0; j < nBinsY -1; j++)
        {
          for (int k = 0; k < nBinsZ -1; k++)
          {
            //Only create topologies for those cells which are not sparse.
            if (!pointMap[i][j][k].isSparse)
            {
              // create a hexahedron topology
              vtkHexahedron* hexahedron = createHexahedron(pointMap, i, j, k);
              visualDataSet->InsertNextCell(VTK_HEXAHEDRON, hexahedron->GetPointIds());
              hexahedron->Delete();
            }
          }
        }
      }

      points->Delete();
      signal->Delete();
      visualDataSet->Squeeze();
      return visualDataSet;
    }
  }

  template<typename TimeMapper>
  inline vtkHexahedron* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createHexahedron(PointMap& pointMap, const int& i, const int& j, const int& k) const
  {
    vtkIdType id_xyz = pointMap[i][j][k].pointId;
    vtkIdType id_dxyz = pointMap[i + 1][j][k].pointId;
    vtkIdType id_dxdyz = pointMap[i + 1][j + 1][k].pointId;
    vtkIdType id_xdyz = pointMap[i][j + 1][k].pointId;

    vtkIdType id_xydz = pointMap[i][j][k + 1].pointId;
    vtkIdType id_dxydz = pointMap[i + 1][j][k + 1].pointId;
    vtkIdType id_dxdydz = pointMap[i + 1][j + 1][k + 1].pointId;
    vtkIdType id_xdydz = pointMap[i][j + 1][k + 1].pointId;

    //create the hexahedron
    vtkHexahedron *theHex = vtkHexahedron::New();
    theHex->GetPointIds()->SetId(0, id_xyz);
    theHex->GetPointIds()->SetId(1, id_dxyz);
    theHex->GetPointIds()->SetId(2, id_dxdyz);
    theHex->GetPointIds()->SetId(3, id_xdyz);
    theHex->GetPointIds()->SetId(4, id_xydz);
    theHex->GetPointIds()->SetId(5, id_dxydz);
    theHex->GetPointIds()->SetId(6, id_dxdydz);
    theHex->GetPointIds()->SetId(7, id_xdydz);
    return theHex;
  }


  template<typename TimeMapper>
  vtkThresholdingUnstructuredGridFactory<TimeMapper>::~vtkThresholdingUnstructuredGridFactory()
  {
  }

  template<typename TimeMapper>
  vtkDataSet* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createMeshOnly() const
  {
    throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
  }

  template<typename TimeMapper>
  vtkFloatArray* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createScalarArray() const
  {
    throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
  }

  template class vtkThresholdingUnstructuredGridFactory<TimeToTimeStep>;
  template class vtkThresholdingUnstructuredGridFactory<TimeStepToTimeStep>;

}
}
