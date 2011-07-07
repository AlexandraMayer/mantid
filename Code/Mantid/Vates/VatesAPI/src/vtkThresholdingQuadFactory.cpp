#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h" 
#include "vtkQuad.h"
#include "MantidGeometry/MDGeometry/Coordinate.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include <vector>
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidAPI/IMDWorkspace.h"

using Mantid::API::IMDWorkspace;

namespace Mantid
{

  namespace VATES
  {

    vtkThresholdingQuadFactory::vtkThresholdingQuadFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName) : m_scalarName(scalarName), m_thresholdRange(thresholdRange)
    {
    }

          /**
  Assigment operator
  @param other : vtkThresholdingQuadFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  vtkThresholdingQuadFactory& vtkThresholdingQuadFactory::operator=(const vtkThresholdingQuadFactory& other)
  {
    if(this != &other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_thresholdRange = other.m_thresholdRange;
      this->m_workspace = other.m_workspace;
    }
    return *this;
  }

  /**
  Copy Constructor
  @param other : instance to copy from.
  */
  vtkThresholdingQuadFactory::vtkThresholdingQuadFactory(const vtkThresholdingQuadFactory& other)
  {
   this->m_scalarName = other.m_scalarName;
   this->m_thresholdRange = other.m_thresholdRange;
   this->m_workspace = other.m_workspace;
  }

    vtkDataSet* vtkThresholdingQuadFactory::create() const
    {
      validate();
      //use the successor factory's creation method if this type cannot handle the dimensionality of the workspace.
      const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
      if(nonIntegratedSize != vtkDataSetFactory::TwoDimensional)
      {
        return m_successor->create();
      }
      else
      {
        const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
        const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );

        const double maxX = m_workspace-> getXDimension()->getMaximum();
        const double minX = m_workspace-> getXDimension()->getMinimum();
        const double maxY = m_workspace-> getYDimension()->getMaximum();
        const double minY = m_workspace-> getYDimension()->getMinimum();

        double incrementX = (maxX - minX) / (nBinsX-1);
        double incrementY = (maxY - minY) / (nBinsY-1);

        const int imageSize = (nBinsX ) * (nBinsY );
        vtkPoints *points = vtkPoints::New();
        points->Allocate(static_cast<int>(imageSize));

        vtkFloatArray * signal = vtkFloatArray::New();
        signal->Allocate(imageSize);
        signal->SetName(m_scalarName.c_str());
        signal->SetNumberOfComponents(1);

        //The following represent actual calculated positions.
        double posX, posY;

        UnstructuredPoint unstructPoint;
        float signalScalar;
        const int nPointsX = nBinsX;
        const int nPointsY = nBinsY;
        Plane plane(nPointsX);

        //Loop through dimensions
        for (int i = 0; i < nPointsX; i++)
        {
          posX = minX + (i * incrementX); //Calculate increment in x;
          Column column(nPointsY);
          for (int j = 0; j < nPointsY; j++)
          {
            posY = minY + (j * incrementY); //Calculate increment in y;

            signalScalar = static_cast<float>(m_workspace->getSignalNormalizedAt(i, j));

            if (boost::math::isnan( signalScalar ) || !m_thresholdRange->inRange(signalScalar))
            {
              //Flagged so that topological and scalar data is not applied.
              unstructPoint.isSparse = true;
            }
            else
            {
              if ((i < (nBinsX -1)) && (j < (nBinsY - 1)))
              {
                signal->InsertNextValue(signalScalar);
              }
              unstructPoint.isSparse = false;
            }
            unstructPoint.pointId = points->InsertNextPoint(posX, posY, 0);
            column[j] = unstructPoint;

          }
          plane[i] = column;
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
            //Only create topologies for those cells which are not sparse.
            if (!plane[i][j].isSparse)
            {
              vtkQuad* quad = vtkQuad::New();
              quad->GetPointIds()->SetId(0, plane[i][j].pointId);
              quad->GetPointIds()->SetId(1, plane[i + 1][j].pointId);
              quad->GetPointIds()->SetId(2, plane[i + 1][j + 1].pointId); 
              quad->GetPointIds()->SetId(3, plane[i][j + 1].pointId);
              visualDataSet->InsertNextCell(VTK_QUAD, quad->GetPointIds());
            }

          }
        }

        points->Delete();
        signal->Delete();
        visualDataSet->Squeeze();
        return visualDataSet;
      }
    }

    vtkDataSet* vtkThresholdingQuadFactory::createMeshOnly() const
    {
      throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
    }

    vtkFloatArray* vtkThresholdingQuadFactory::createScalarArray() const
    {
      throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
    }

    void vtkThresholdingQuadFactory::initialize(Mantid::API::Workspace_sptr wspace_sptr)
    {
      m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(wspace_sptr);
      validate();
      // When the workspace can not be handled by this type, take action in the form of delegation.
      const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
      if(nonIntegratedSize != vtkDataSetFactory::TwoDimensional)
      {
        if(this->hasSuccessor())
        {
          m_successor->initialize(m_workspace);
          return;
        }
        else
        {
          throw std::runtime_error("There is no successor factory set for this vtkThresholdingQuadFactory type");
        }
      }

      //Setup range values according to whatever strategy object has been injected.
      m_thresholdRange->setWorkspace(m_workspace);
      m_thresholdRange->calculate();
    }

    void vtkThresholdingQuadFactory::validate() const
    {
      if(NULL == m_workspace.get())
      {
        throw std::runtime_error("IMDWorkspace is null");
      }
    }

    vtkThresholdingQuadFactory::~vtkThresholdingQuadFactory()
    {

    }
  }
}
