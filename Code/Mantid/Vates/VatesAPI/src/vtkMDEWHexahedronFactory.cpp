#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidVatesAPI/vtkMDEWHexahedronFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::Kernel::CPUTimer;

namespace Mantid
{
  namespace VATES
  {
  
  /*Constructor
  @Param thresholdRange : Threshold range strategy
  @scalarName : Name for scalar signal array.
  */
  vtkMDEWHexahedronFactory::vtkMDEWHexahedronFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t maxDepth) :
  m_thresholdRange(thresholdRange), m_scalarName(scalarName), m_maxDepth(maxDepth)
  {
  }

  /// Destructor
  vtkMDEWHexahedronFactory::~vtkMDEWHexahedronFactory()
  {
  }

  //-------------------------------------------------------------------------------------------------
  /* Generate the vtkDataSet from the objects input MDEventWorkspace (of a given type an dimensionality 3+)
   *
   * @return a fully constructed vtkUnstructuredGrid containing geometric and scalar data.
  */
  template<typename MDE, size_t nd>
  void vtkMDEWHexahedronFactory::doCreate(typename MDEventWorkspace<MDE, nd>::sptr ws) const
  {
    bool VERBOSE = true;
    CPUTimer tim;

    // First we get all the boxes, up to the given depth; with or wo the slice function
    std::vector<IMDBox<MDE,nd> *> boxes;
    if (this->slice)
      ws->getBox()->getBoxes(boxes, m_maxDepth, true, this->sliceImplicitFunction);
    else
      ws->getBox()->getBoxes(boxes, m_maxDepth, true);


    vtkIdType numBoxes = boxes.size();
    vtkIdType imageSizeActual = 0;
    
    if (VERBOSE) std::cout << tim << " to retrieve the " << numBoxes << " boxes down to depth " << m_maxDepth << std::endl;

    // Create 8 points per box.
    vtkPoints *points = vtkPoints::New();
    points->Allocate(numBoxes * 8);
    points->SetNumberOfPoints(numBoxes * 8);
    
    // One scalar per box
    vtkFloatArray * signals = vtkFloatArray::New();
    signals->Allocate(numBoxes);
    signals->SetName(m_scalarName.c_str());
    signals->SetNumberOfComponents(1);
    //signals->SetNumberOfValues(numBoxes);

    // To cache the signal
    float * signalArray = new float[numBoxes];

    // True for boxes that we will use
    bool * useBox = new bool[numBoxes];
    memset(useBox, 0, sizeof(bool)*numBoxes);
    
    // Create the data set
    vtkUnstructuredGrid * visualDataSet = vtkUnstructuredGrid::New();
    this->dataSet = visualDataSet;
    visualDataSet->Allocate(numBoxes);

    std::vector<Mantid::Geometry::Coordinate> coords;

    vtkIdList * hexPointList = vtkIdList::New();
    hexPointList->SetNumberOfIds(8);

    // This can be parallelized
    PRAGMA_OMP( parallel for schedule (dynamic) )
    for (int ii=0; ii<int(boxes.size()); ii++)
    {
      // Get the box here
      size_t i = size_t(ii);
      IMDBox<MDE,nd> * box = boxes[i];
      Mantid::signal_t signal_normalized= box->getSignalNormalized();

      if (!boost::math::isnan( signal_normalized ) && m_thresholdRange->inRange(signal_normalized))
      {
        // Cache the signal and using of it
        signalArray[i] = float(signal_normalized);
        useBox[i] = true;

        //Get the coordinates.
        size_t numVertexes = 0;
        coord_t * coords;

        // If slicing down to 3D, specify which dimensions to keep.
        if (this->slice)
          coords = box->getVertexesArray(numVertexes, 3, this->sliceMask);
        else
          coords = box->getVertexesArray(numVertexes);

        if (numVertexes == 8)
        {
          //Iterate through all coordinates. Candidate for speed improvement.
          for(size_t v = 0; v < numVertexes; v++)
          {
            coord_t * coord = coords + v*3;
            // Set the point at that given ID
            points->SetPoint(i*8 + v, coord[0], coord[1], coord[2]);
          }

        } // valid number of vertexes returned

        // Free memory
        delete [] coords;
      }
    } // For each box

    if (VERBOSE) std::cout << tim << " to create the necessary points." << std::endl;

    for (size_t i=0; i<boxes.size(); i++)
    {
      if (useBox[i])
      {
        // The bare point ID
        vtkIdType pointIds = i * 8;

        //Add signal
        signals->InsertNextValue(signalArray[i]);

        hexPointList->SetId(0, pointIds + 0); //xyx
        hexPointList->SetId(1, pointIds + 1); //dxyz
        hexPointList->SetId(2, pointIds + 3); //dxdyz
        hexPointList->SetId(3, pointIds + 2); //xdyz
        hexPointList->SetId(4, pointIds + 4); //xydz
        hexPointList->SetId(5, pointIds + 5); //dxydz
        hexPointList->SetId(6, pointIds + 7); //dxdydz
        hexPointList->SetId(7, pointIds + 6); //xdydz

        //Add cells
        visualDataSet->InsertNextCell(VTK_HEXAHEDRON, hexPointList);
        imageSizeActual++;
      }
    } // for each box.

    //Shrink to fit
    signals->Squeeze();
    visualDataSet->Squeeze();

    //Add points
    visualDataSet->SetPoints(points);
    //Add scalars
    visualDataSet->GetCellData()->SetScalars(signals);

    if (VERBOSE) std::cout << tim << " to create " << imageSizeActual << " hexahedrons." << std::endl;

  }


  //-------------------------------------------------------------------------------------------------
  /*
  Generate the vtkDataSet from the objects input IMDEventWorkspace
  @Return a fully constructed vtkUnstructuredGrid containing geometric and scalar data.
  */
  vtkDataSet* vtkMDEWHexahedronFactory::create() const
  {
    validate();

    size_t nd = m_workspace->getNumDims();
    if (nd > 3)
    {
      // Slice from >3D down to 3D
      this->slice = true;
      this->sliceMask = new bool[nd];
      this->sliceImplicitFunction = new MDImplicitFunction();

      // Make the mask of dimensions
      // TODO: Smarter mapping
      for (size_t d=0; d<nd; d++)
        this->sliceMask[d] = (d<3);

      // Define where the slice is in 4D
      // TODO: Where to slice? Right now is just 0
      std::vector<coord_t> point(nd, 0);

      // Define two opposing planes that point in all higher dimensions
      std::vector<coord_t> normal1(nd, 0);
      std::vector<coord_t> normal2(nd, 0);
      for (size_t d=3; d<nd; d++)
      {
        normal1[d] = +1.0;
        normal2[d] = -1.0;
      }
      // This creates a 0-thickness region to slice in.
      sliceImplicitFunction->addPlane( MDPlane(normal1, point) );
      sliceImplicitFunction->addPlane( MDPlane(normal2, point) );

      coord_t pointA[4] = {0, 0, 0, -1.0};
      coord_t pointB[4] = {0, 0, 0, +2.0};
    }
    else
    {
      // Direct 3D, so no slicing
      this->slice = false;
    }

    // Macro to call the right instance of the
    CALL_MDEVENT_FUNCTION(this->doCreate, m_workspace);

    // Clean up
    if (this->slice)
    {
      delete this->sliceMask;
      delete this->sliceImplicitFunction;
    }

    // The macro does not allow return calls, so we used a member variable.
    return this->dataSet;
  }
  
  /*
  Create as Mesh Only. Legacy method
  @Return Nothing. throws on invoke.
  */
  vtkDataSet* vtkMDEWHexahedronFactory::createMeshOnly() const
  {
    throw std::runtime_error("Invalid usage. Cannot call vtkMDEWHexahedronFactory::createMeshOnly()");
  }

  /*
  Create as Mesh Only. Legacy method
  @Return Nothing. throws on invoke.
  */
  vtkFloatArray* vtkMDEWHexahedronFactory::createScalarArray() const
  {
    throw std::runtime_error("Invalid usage. Cannot call vtkMDEWHexahedronFactory::createScalarArray()");
  }

 /*
  Initalize the factory with the workspace. This allows top level decision on what factory to use, but allows presenter/algorithms to pass in the
  dataobjects (workspaces) to run against at a later time. If workspace is not an IMDEventWorkspace, throws an invalid argument exception.
  @Param ws : Workspace to use.
  */
  void vtkMDEWHexahedronFactory::initialize(Mantid::API::Workspace_sptr ws)
  {
    this->m_workspace = boost::dynamic_pointer_cast<IMDEventWorkspace>(ws);
    if(!m_workspace)
      throw std::invalid_argument("Workspace is null or not IMDEventWorkspace");
  }

  /// Validate the current object.
  void vtkMDEWHexahedronFactory::validate() const
  { 
    if(!m_workspace)
    {
      throw std::runtime_error("Invalid vtkMDEWHexahedronFactory. Workspace is null");
    }
    if (m_workspace->getNumDims() < 3)
      throw std::runtime_error("Invalid vtkMDEWHexahedronFactory. Workspace must have at least 3 dimensions.");
  }

  /** Sets the recursion depth to a specified level in the workspace.
  */
  void vtkMDEWHexahedronFactory::setRecursionDepth(size_t depth)
  {
    m_maxDepth = depth;
  }

  }
}
