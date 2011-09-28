#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/CoordTransformAffineParser.h"
#include "MantidMDEvents/CoordTransformAligned.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include <boost/algorithm/string.hpp>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidMDEvents/CoordTransformAffine.h"

using Mantid::Kernel::CPUTimer;
using Mantid::Kernel::EnabledWhenProperty;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(BinToMDHistoWorkspace)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BinToMDHistoWorkspace::BinToMDHistoWorkspace()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BinToMDHistoWorkspace::~BinToMDHistoWorkspace()
  {
  }


  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void BinToMDHistoWorkspace::initDocs()
  {
    this->setWikiSummary("Take a [[MDEventWorkspace]] and bin into into a dense, multi-dimensional histogram workspace ([[MDHistoWorkspace]]).");
    this->setOptionalMessage("Take a MDEventWorkspace and bin into into a dense, multi-dimensional histogram workspace (MDHistoWorkspace).");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BinToMDHistoWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");

    // Properties for specifying the slice to perform.
    this->initSlicingProps();

    // --------------- Processing methods and options ---------------------------------------
    std::string grp = "Methods";
    declareProperty(new PropertyWithValue<std::string>("ImplicitFunctionXML","",Direction::Input),
        "XML string describing the implicit function determining which bins to use.");
    setPropertyGroup("ImplicitFunctionXML", grp);

    declareProperty(new PropertyWithValue<bool>("IterateEvents",true,Direction::Input),
        "Alternative binning method where you iterate through every event, placing them in the proper bin.\n"
        "This may be faster for workspaces with few events and lots of output bins.");
    setPropertyGroup("IterateEvents", grp);

    declareProperty(new PropertyWithValue<bool>("Parallel",false,Direction::Input),
        "Temporary parameter: true to run in parallel. This is ignored for file-backed workspaces, where running in parallel makes things slower due to disk thrashing.");
    setPropertyGroup("Parallel", grp);

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "A name for the output MDHistoWorkspace.");

  }



  //----------------------------------------------------------------------------------------------
  /** Bin the contents of a MDBox
   *
   * @param box :: pointer to the MDBox to bin
   * @param chunkMin :: the minimum index in each dimension to consider "valid" (inclusive)
   * @param chunkMax :: the maximum index in each dimension to consider "valid" (exclusive)
   */
  template<typename MDE, size_t nd>
  inline void BinToMDHistoWorkspace::binMDBox(MDBox<MDE, nd> * box, size_t * chunkMin, size_t * chunkMax)
  {
    // An array to hold the rotated/transformed coordinates
    coord_t * outCenter = new coord_t[outD];

    // Evaluate whether the entire box is in the same bin
    if (box->getNPoints() > (1 << nd) * 2)
    {
      // There is a check that the number of events is enough for it to make sense to do all this processing.
      size_t numVertexes = 0;
      coord_t * vertexes = box->getVertexesArray(numVertexes);

      // All vertexes have to be within THE SAME BIN = have the same linear index.
      size_t lastLinearIndex = 0;
      bool badOne = false;

      for (size_t i=0; i<numVertexes; i++)
      {
        // Cache the center of the event (again for speed)
        const coord_t * inCenter = vertexes + i * nd;

        // Now transform to the output dimensions
        m_transform->apply(inCenter, outCenter);

        // To build up the linear index
        size_t linearIndex = 0;
        // To mark VERTEXES outside range
        badOne = false;

        /// Loop through the dimensions on which we bin
        for (size_t bd=0; bd<outD; bd++)
        {
          // What is the bin index in that dimension
          coord_t x = outCenter[bd];
          size_t ix = size_t(x);
          // Within range (for this chunk)?
          if ((x >= 0) && (ix >= chunkMin[bd]) && (ix < chunkMax[bd]))
          {
            // Build up the linear index
            linearIndex += indexMultiplier[bd] * ix;
          }
          else
          {
            // Outside the range
            badOne = true;
            break;
          }
        } // (for each dim in MDHisto)

        // Is the vertex at the same place as the last one?
        if (!badOne)
        {
          if ((i > 0) && (linearIndex != lastLinearIndex))
          {
            // Change of index
            badOne = true;
            break;
          }
          lastLinearIndex = linearIndex;
        }

        // Was the vertex completely outside the range?
        if (badOne)
          break;
      } // (for each vertex)

      delete [] vertexes;

      if (!badOne)
      {
        // Yes, the entire box is within a single bin
//        std::cout << "Box at " << box->getExtentsStr() << " is within a single bin.\n";
        // Add the CACHED signal from the entire box
        signals[lastLinearIndex] += box->getSignal();
        errors[lastLinearIndex] += box->getErrorSquared();
        // And don't bother looking at each event. This may save lots of time loading from disk.
        delete [] outCenter;
        return;
      }
    }

    // If you get here, you could not determine that the entire box was in the same bin.
    // So you need to iterate through events.

    const std::vector<MDE> & events = box->getConstEvents();
    typename std::vector<MDE>::const_iterator it = events.begin();
    typename std::vector<MDE>::const_iterator it_end = events.end();
    for (; it != it_end; it++)
    {
      // Cache the center of the event (again for speed)
      const coord_t * inCenter = it->getCenter();

      // Now transform to the output dimensions
      m_transform->apply(inCenter, outCenter);

      // To build up the linear index
      size_t linearIndex = 0;
      // To mark events outside range
      bool badOne = false;

      /// Loop through the dimensions on which we bin
      for (size_t bd=0; bd<outD; bd++)
      {
        // What is the bin index in that dimension
        coord_t x = outCenter[bd];
        size_t ix = size_t(x);
        // Within range (for this chunk)?
        if ((x >= 0) && (ix >= chunkMin[bd]) && (ix < chunkMax[bd]))
        {
          // Build up the linear index
          linearIndex += indexMultiplier[bd] * ix;
        }
        else
        {
          // Outside the range
          badOne = true;
          break;
        }
      } // (for each dim in MDHisto)

      if (!badOne)
      {
        signals[linearIndex] += it->getSignal();
        errors[linearIndex] += it->getErrorSquared();
      }
    }
    // Done with the events list
    box->releaseEvents();

    delete [] outCenter;
  }


  //----------------------------------------------------------------------------------------------
  /** Perform binning by iterating through every event and placing them in the output workspace
   *
   * @param ws :: MDEventWorkspace of the given type.
   */
  template<typename MDE, size_t nd>
  void BinToMDHistoWorkspace::binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    BoxController_sptr bc = ws->getBoxController();

    // Start with signal at 0.0
    outWS->setTo(0.0, 0.0);

    // Cache some data to speed up accessing them a bit
    indexMultiplier = new size_t[outD];
    for (size_t d=0; d<outD; d++)
    {
      if (d > 0)
        indexMultiplier[d] = outWS->getIndexMultiplier()[d-1];
      else
        indexMultiplier[d] = 1;
    }
    signals = outWS->getSignalArray();
    errors = outWS->getErrorSquaredArray();

    // The dimension (in the output workspace) along which we chunk for parallel processing
    // TODO: Find the smartest dimension to chunk against
    size_t chunkDimension = 0;

    // How many bins (in that dimension) per chunk.
    // Try to split it so each core will get 2 tasks:
    int chunkNumBins =  int(binDimensions[chunkDimension]->getNBins() / (Mantid::Kernel::ThreadPool::getNumPhysicalCores() * 2));
    if (chunkNumBins < 1) chunkNumBins = 1;

    // Do we actually do it in parallel?
    bool doParallel = getProperty("Parallel");
    // Not if file-backed!
    if (bc->isFileBacked()) doParallel = false;
    if (!doParallel)
      chunkNumBins = int(binDimensions[chunkDimension]->getNBins());

    // Total number of steps
    size_t progNumSteps = 0;
    if (prog) prog->setNotifyStep(0.1);
    if (prog) prog->resetNumSteps(100, 0.00, 1.0);

    // Run the chunks in parallel. There is no overlap in the output workspace so it is
    // thread safe to write to it..
    PRAGMA_OMP( parallel for schedule(dynamic,1) if (doParallel) )
    for(int chunk=0; chunk < int(binDimensions[chunkDimension]->getNBins()); chunk += chunkNumBins)
    {
      PARALLEL_START_INTERUPT_REGION
      // Region of interest for this chunk.
      size_t * chunkMin = new size_t[outD];
      size_t * chunkMax = new size_t[outD];
      for (size_t bd=0; bd<outD; bd++)
      {
        // Same limits in the other dimensions
        chunkMin[bd] = 0;
        chunkMax[bd] = binDimensions[bd]->getNBins();
      }
      // Parcel out a chunk in that single dimension dimension
      chunkMin[chunkDimension] = size_t(chunk);
      if (size_t(chunk+chunkNumBins) > binDimensions[chunkDimension]->getNBins())
        chunkMax[chunkDimension] = binDimensions[chunkDimension]->getNBins();
      else
        chunkMax[chunkDimension] = size_t(chunk+chunkNumBins);

      // Build an implicit function (it needs to be in the space of the MDEventWorkspace)
      MDImplicitFunction * function = this->getImplicitFunctionForChunk(nd, chunkMin, chunkMax);

      // Use getBoxes() to get an array with a pointer to each box
      std::vector<IMDBox<MDE,nd>*> boxes;
      // Leaf-only; no depth limit; with the implicit function passed to it.
      ws->getBox()->getBoxes(boxes, 1000, true, function);

      // Sort boxes by file position IF file backed. This reduces seeking time, hopefully.
      if (bc->isFileBacked())
        IMDBox<MDE, nd>::sortBoxesByFilePos(boxes);

      // For progress reporting, the # of boxes
      if (prog)
      {
        PARALLEL_CRITICAL(BinToMDHistoWorkspace_progress)
        {
          std::cout << "Chunk " << chunk << ": found " << boxes.size() << " boxes within the implicit function." << std::endl;
          progNumSteps += boxes.size();
          prog->setNumSteps( progNumSteps );
        }
      }

      // Go through every box for this chunk.
      for (size_t i=0; i<boxes.size(); i++)
      {
        MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
        // Perform the binning in this separate method.
        if (box)
          this->binMDBox(box, chunkMin, chunkMax);

        // Progress reporting
        if (prog) prog->report();

      }// for each box in the vector
      PARALLEL_END_INTERUPT_REGION
    } // for each chunk in parallel
    PARALLEL_CHECK_INTERUPT_REGION



    // Now the implicit function
    if (implicitFunction)
    {
      prog->report("Applying implicit function.");
      signal_t nan = std::numeric_limits<signal_t>::quiet_NaN();
      outWS->applyImplicitFunction(implicitFunction, nan, nan);
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Templated method to apply the binning operation to the particular
   * MDEventWorkspace passed in.
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void BinToMDHistoWorkspace::do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    bool DODEBUG = true;

    CPUTimer tim;

    // Number of output binning dimensions found
    size_t outD = binDimensions.size();

    //Since the costs are not known ahead of time, use a simple FIFO buffer.
    ThreadScheduler * ts = new ThreadSchedulerFIFO();

    // Create the threadpool with: all CPUs, a progress reporter
    ThreadPool tp(ts, 0, prog);

    // Big efficiency gain is obtained by grouping a few bins per task.
    size_t binsPerTask = 100;

    // For progress reporting, the approx  # of tasks
    if (prog)
      prog->setNumSteps( int(outWS->getNPoints()/100) );

    // The root-level box.
    IMDBox<MDE,nd> * rootBox = ws->getBox();

    // This is the limit to loop over in each dimension
    size_t * index_max = new size_t[outD];
    for (size_t bd=0; bd<outD; bd++) index_max[bd] = binDimensions[bd]->getNBins();

    // Cache a calculation to convert indices x,y,z,t into a linear index.
    size_t * index_maker = new size_t[outD];
    Utils::NestedForLoop::SetUpIndexMaker(outD, index_maker, index_max);

    int numPoints = int(outWS->getNPoints());

    // Run in OpenMP with dynamic scheduling and a smallish chunk size (binsPerTask)
    // Right now, not parallel for file-backed systems.
    bool fileBacked = (ws->getBoxController()->getFile() != NULL);
    PRAGMA_OMP(parallel for schedule(dynamic, binsPerTask) if (!fileBacked)  )
    for (int i=0; i < numPoints; i++)
    {
      PARALLEL_START_INTERUPT_REGION

      size_t linear_index = size_t(i);
      // nd >= outD in all cases so this is safe.
      size_t index[nd];

      // Get the index at each dimension for this bin.
      Utils::NestedForLoop::GetIndicesFromLinearIndex(outD, linear_index, index_maker, index_max, index);

      // Construct the bin and its coordinates
      MDBin<MDE,nd> bin;
      for (size_t bd=0; bd<outD; bd++)
      {
        // Index in this binning dimension (i_x, i_y, etc.)
        size_t idx = index[bd];
        // Dimension in the MDEventWorkspace
        size_t d = dimensionToBinFrom[bd];
        // Corresponding extents
        bin.m_min[d] = binDimensions[bd]->getX(idx);
        bin.m_max[d] = binDimensions[bd]->getX(idx+1);
      }
      bin.m_index = linear_index;

      bool dimensionsUsed[nd];
      for (size_t d=0; d<nd; d++)
        dimensionsUsed[d] = (d<3);

      // Check if the bin is in the ImplicitFunction (if any)
      bool binContained = true;
      if (implicitFunction)
      {
        binContained = implicitFunction->isPointContained(bin.m_min); //TODO. Correct argument passed to this method?
      }

      if (binContained)
      {
        // Array of bools set to true when a dimension is fully contained (binary splitting only)
        bool fullyContained[nd];
        for (size_t d=0; d<nd; d++)
          fullyContained[d] = false;

        // This will recursively bin into the sub grids
        rootBox->centerpointBin(bin, fullyContained);

        // Save the data into the dense histogram
        outWS->setSignalAt(linear_index, bin.m_signal);
        outWS->setErrorAt(linear_index, bin.m_errorSquared);
      }

      // Report progress but not too often.
      if (((linear_index % 100) == 0) && prog ) prog->report();

      PARALLEL_END_INTERUPT_REGION
    } // (for each linear index)
    PARALLEL_CHECK_INTERUPT_REGION

    if (DODEBUG) std::cout << tim << " to run the openmp loop.\n";

    delete [] index_max;
    delete [] index_maker;
  }














  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BinToMDHistoWorkspace::exec()
  {
    // Input MDEventWorkspace
    in_ws = getProperty("InputWorkspace");
    // Look at properties, create either axis-aligned or general transform.
    this->createTransform();

    // De serialize the implicit function
    std::string ImplicitFunctionXML = getPropertyValue("ImplicitFunctionXML");
    implicitFunction = NULL;
    if (!ImplicitFunctionXML.empty())
      implicitFunction = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(ImplicitFunctionXML);

    prog = new Progress(this, 0, 1.0, 1); // This gets deleted by the thread pool; don't delete it in here.

    // Create the dense histogram. This allocates the memory
    outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(binDimensions));

    // Saves the geometry transformation from original to binned in the workspace
    outWS->setTransformFromOriginal( this->m_transformFromOriginal );
    outWS->setTransformToOriginal( this->m_transformToOriginal );
    for (size_t i=0; i<m_bases.size(); i++)
      outWS->setBasisVector(i, m_bases[i]);
    outWS->setOrigin( this->m_origin );

    // Wrapper to cast to MDEventWorkspace then call the function
    bool IterateEvents = getProperty("IterateEvents");
    if (!m_axisAligned)
    {
      g_log.notice() << "Algorithm does not currently support IterateEvents=False if AxisAligned=False. Setting IterateEvents=True." << std::endl;
      IterateEvents = true;
    }

    if (IterateEvents)
    {
      CALL_MDEVENT_FUNCTION(this->binByIterating, in_ws);
    }
    else
    {
      CALL_MDEVENT_FUNCTION(this->do_centerpointBin, in_ws);
    }

    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));
  }



} // namespace Mantid
} // namespace MDEvents
