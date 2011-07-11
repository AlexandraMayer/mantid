#include "MantidKernel/Task.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDGridBox.h"
#include <ostream>
#include "MantidKernel/Strings.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

// These pragmas ignores the warning in the ctor where "d<nd-1" for nd=1.
// This is okay (though would be better if it were for only that function
#if (defined(__INTEL_COMPILER))
#pragma warning disable 196
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif


/** If defined, then signal caching is performed as events are added. Otherwise,
 * refreshCache() has to be called.
 */
#undef MDEVENTS_MDGRIDBOX_ONGOING_SIGNAL_CACHE

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  //===============================================================================================
  //-----------------------------------------------------------------------------------------------
  /** Empty constructor. Used when loading from NXS files.
   * */
  TMDE(MDGridBox)::MDGridBox()
   : IMDBox<MDE, nd>(), numBoxes(0), nPoints(0)
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param box :: MDBox containing the events to split */
  TMDE(MDGridBox)::MDGridBox(MDBox<MDE, nd> * box)
   : IMDBox<MDE, nd>(box),
     nPoints(0)
  {
    BoxController_sptr bc = box->getBoxController();
    if (!bc)
      throw std::runtime_error("MDGridBox::ctor(): No BoxController specified in box.");

    // Steal the ID from the parent box that is being split.
    this->setId( box->getId() );

    // How many is it split?
    for (size_t d=0; d<nd; d++)
      split[d] = bc->getSplitInto(d);

    // Compute sizes etc.
    size_t tot = computeFromSplit();
    if (tot == 0)
      throw std::runtime_error("MDGridBox::ctor(): Invalid splitting criterion (one was zero).");

    // Calculate the volume
    double volume = 1;
    for (size_t d=0; d<nd; d++)
      volume *= boxSize[d];
    double inverseVolume = 1.0 / volume;

    // Create the array of MDBox contents.
    boxes.clear();
    boxes.reserve(tot);
    numBoxes = tot;

    size_t indices[nd];
    for (size_t d=0; d<nd; d++) indices[d] = 0;
    for (size_t i=0; i<tot; i++)
    {
      // Create the box
      // (Increase the depth of this box to one more than the parent (this))
      MDBox<MDE,nd> * myBox = new MDBox<MDE,nd>(bc, this->m_depth + 1);

      // Set the extents of this box.
      for (size_t d=0; d<nd; d++)
      {
        coord_t min = this->extents[d].min + boxSize[d] * double(indices[d]);
        myBox->setExtents(d, min, min + boxSize[d]);
      }
      myBox->setInverseVolume(inverseVolume); // Set the cached inverse volume
      boxes.push_back(myBox);

      // Increment the indices, rolling back as needed
      indices[0]++;
      for (size_t d=0; d<nd-1; d++) //This is not run if nd=1; that's okay, you can ignore the warning
      {
        if (indices[d] >= split[d])
        {
          indices[d] = 0;
          indices[d+1]++;
        }
      }
    } // for each box

    // Now distribute the events that were in the box before
    this->addEvents(box->getEvents());
    // Copy the cached numbers from the incoming box. This is quick - don't need to refresh cache
    this->nPoints = box->getNPoints();
  }



  //-----------------------------------------------------------------------------------------------
  /** Compute some data from the split[] array and the extents.
   * @return :: the total number of boxes */
  TMDE(
  size_t MDGridBox)::computeFromSplit()
  {
    // Do some computation based on how many splits per each dim.
    size_t tot = 1;
    diagonalSquared = 0;
    for (size_t d=0; d<nd; d++)
    {
      // Cumulative multiplier, for indexing
      splitCumul[d] = tot;
      tot *= split[d];
      // Length of the side of a box in this dimension
      boxSize[d] = (this->extents[d].max - this->extents[d].min) / double(split[d]);
      // Accumulate the squared diagonal length.
      diagonalSquared += boxSize[d] * boxSize[d];
    }

    return tot;

  }

  //-----------------------------------------------------------------------------------------------
  /// Destructor
  TMDE(MDGridBox)::~MDGridBox()
  {
    // Delete all contained boxes (this should fire the MDGridBox destructors recursively).
    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
      delete *it;
    boxes.clear();
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDGridBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
    {
      (*it)->clear();
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDGridBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDGridBox)::getNPoints() const
  {
    //Use the cached value
    return nPoints;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of un-split MDBoxes in this box (recursively including all children)
   * @return :: the total # of MDBoxes in all children */
  TMDE(
  size_t MDGridBox)::getNumMDBoxes() const
  {
    size_t total = 0;
    typename boxVector_t::const_iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
    {
      total += (*it)->getNumMDBoxes();
    }
    return total;
  }


  //-----------------------------------------------------------------------------------------------
  /** @return The number of children of the MDGridBox, not recursively
   */
  TMDE(
  size_t MDGridBox)::getNumChildren() const
  {
    return numBoxes;
  }

  //-----------------------------------------------------------------------------------------------
  /** Get a child box
   * @param index :: index into the array, within range 0..getNumChildren()-1
   * @return the child IMDBox pointer.
   */
  template <typename MDE, size_t nd>
  IMDBox<MDE,nd> * MDGridBox<MDE,nd>::getChild(size_t index)
  {
    return boxes[index];
  }

  //-----------------------------------------------------------------------------------------------
  /** Helper function to get the index into the linear array given
   * an array of indices for each dimension (0 to nd)
   * @param indices :: array of size[nd]
   * @return size_t index into boxes[].
   */
  TMDE(
  inline size_t MDGridBox)::getLinearIndex(size_t * indices) const
  {
    size_t out_linear_index = 0;
    for (size_t d=0; d<nd; d++)
      out_linear_index += (indices[d] * splitCumul[d]);
    return out_linear_index;
  }


  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache of nPoints, signal and error,
   * by adding up all boxes (recursively).
   * MDBoxes' totals are used directly.
   *
   * @param ts :: ThreadScheduler pointer to perform the caching
   *  in parallel. If NULL, it will be performed in series.
   */
  TMDE(
  void MDGridBox)::refreshCache(ThreadScheduler * ts)
  {
    // Clear your total
    nPoints = 0;
    this->m_signal = 0;
    this->m_errorSquared = 0;
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] = 0;

    typename boxVector_t::iterator it;
    typename boxVector_t::iterator it_end = boxes.end();

    if (!ts)
    {
      //--------- Serial -----------
      for (it = boxes.begin(); it != it_end; it++)
      {
        IMDBox<MDE,nd> * ibox = *it;

        // Refresh the cache (does nothing for MDBox)
        ibox->refreshCache();

        // Add up what's in there
        nPoints += ibox->getNPoints();
        this->m_signal += ibox->getSignal();
        this->m_errorSquared += ibox->getErrorSquared();

        // And track the centroid
        for (size_t d=0; d<nd; d++)
          this->m_centroid[d] += ibox->getCentroid(d) * ibox->getSignal();
      }
    }
    else
    {
      //---------- Parallel refresh --------------
      throw std::runtime_error("Not implemented");
    }

  }


  // -------------------------------------------------------------------------------------------
  /** Cache the centroid of this box and all sub-boxes.
   * @param ts :: ThreadScheduler for parallel processing.
   */
  TMDE(
  void MDGridBox)::refreshCentroid(Kernel::ThreadScheduler * ts)
  {
    UNUSED_ARG(ts);

    // Start at 0.0
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] = 0;

    // Signal was calculated before (when adding)
    // Keep 0.0 if the signal is null. This avoids dividing by 0.0
    if (this->m_signal == 0) return;

    typename boxVector_t::iterator it;
    typename boxVector_t::iterator it_end = boxes.end();

    if (!ts)
    {
      //--------- Serial -----------
      for (it = boxes.begin(); it != it_end; it++)
      {
        IMDBox<MDE,nd> * ibox = *it;

        // Refresh the centroid of all sub-boxes.
        ibox->refreshCentroid();

        signal_t iBoxSignal = ibox->getSignal();

        // And track the centroid
        for (size_t d=0; d<nd; d++)
          this->m_centroid[d] += ibox->getCentroid(d) * iBoxSignal;
      }
    }
    else
    {
      //---------- Parallel refresh --------------
      throw std::runtime_error("Not implemented");
    }

    // Normalize centroid by the total signal
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] /= this->m_signal;
  }


  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDGridBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    //out->insert(out->begin(), data.begin(), data.end());
    return out;
  }

  //-----------------------------------------------------------------------------------------------
  /** Return all boxes contained within.
   *
   * @param outBoxes :: vector to fill
   * @param maxDepth :: max depth value of the returned boxes.
   * @param leafOnly :: if true, only add the boxes that are no more subdivided (leaves on the tree)
   */
  TMDE(
  void MDGridBox)::getBoxes(std::vector<IMDBox<MDE,nd> *> & outBoxes, size_t maxDepth, bool leafOnly)
  {
    // Add this box, unless we only want the leaves
    if (!leafOnly)
      outBoxes.push_back(this);

    if (this->getDepth() + 1 <= maxDepth)
    {
      for (size_t i=0; i<numBoxes; i++)
      {
        // Recursively go deeper, if needed
        boxes[i]->getBoxes(outBoxes, maxDepth, leafOnly);
      }
    }
    else
    {
      // Oh, we reached the max depth and want only leaves.
      // ... so we consider this box to be a leaf too.
      if (leafOnly)
        outBoxes.push_back(this);
    }

  }


  //-----------------------------------------------------------------------------------------------
  /** Split a box that is contained in the GridBox, at the given index,
   * into a MDGridBox.
   *
   * Thread-safe as long as 'index' is different for all threads.
   *
   * @param index :: index into the boxes vector.
   *        Warning: No bounds check is made, don't give stupid values!
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL for no recursive splitting.
   */
  TMDE(
  void MDGridBox)::splitContents(size_t index, ThreadScheduler * ts)
  {
    // You can only split it if it is a MDBox (not MDGridBox).
    MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[index]);
    if (!box) return;
    // Track how many MDBoxes there are in the overall workspace
    this->m_BoxController->trackNumBoxes(box->getDepth());
    // Construct the grid box
    MDGridBox<MDE, nd> * gridbox = new MDGridBox<MDE, nd>(box);
    // Delete the old ungridded box
    delete boxes[index];
    // And now we have a gridded box instead of a boring old regular box.
    boxes[index] = gridbox;

    if (ts)
    {
      // Create a task to split the newly create MDGridBox.
      ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitAllIfNeeded, &*gridbox, ts) ) );
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Goes through all the sub-boxes and splits them if they contain
   * enough events to be worth it.
   *
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL to do it serially.
   */
  TMDE(
  void MDGridBox)::splitAllIfNeeded(ThreadScheduler * ts)
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
      if (box)
      {
        // Plain MD-Box. Does it need to split?
        if (this->m_BoxController->willSplit(box->getNPoints(), box->getDepth() ))
        {
          // The MDBox needs to split into a grid box.
          if (!ts)
          {
            // ------ Perform split serially (no ThreadPool) ------
            MDGridBox<MDE, nd> * gridBox = new MDGridBox<MDE, nd>(box);
            // Track how many MDBoxes there are in the overall workspace
            this->m_BoxController->trackNumBoxes(box->getDepth());
            // Replace in the array
            boxes[i] = gridBox;
            // Delete the old box
            delete box;
            // Now recursively check if this NEW grid box's contents should be split too
            gridBox->splitAllIfNeeded(NULL);
          }
          else
          {
            // ------ Perform split in parallel (using ThreadPool) ------
            // So we create a task to split this MDBox,
            // Task is : this->splitContents(i, ts);
            ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitContents, &*this, i, ts) ) );
          }
        }
      }
      else
      {
        // It should be a MDGridBox
        MDGridBox<MDE, nd> * gridBox = dynamic_cast<MDGridBox<MDE, nd>*>(boxes[i]);
        if (gridBox)
        {
          // Now recursively check if this old grid box's contents should be split too
          if (!ts || (this->nPoints < this->m_BoxController->getAddingEvents_eventsPerTask()))
            // Go serially if there are only a few points contained (less overhead).
            gridBox->splitAllIfNeeded(ts);
          else
            // Go parallel if this is a big enough gridbox.
            // Task is : gridBox->splitAllIfNeeded(ts);
            ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitAllIfNeeded, &*gridBox, ts) ) );
        }
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a single MDEvent to the grid box. If the boxes
   * contained within are also gridded, this will recursively push the event
   * down to the deepest level.
   * Warning! No bounds checking is done (for performance). It must
   * be known that the event is within the bounds of the grid box before adding.
   *
   * Note! nPoints, signal and error must be re-calculated using refreshCache()
   * after all events have been added.
   *
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  inline void MDGridBox)::addEvent( const MDE & event)
  {
    size_t index = 0;
    for (size_t d=0; d<nd; d++)
    {
      coord_t x = event.getCenter(d);
      int i = int((x - this->extents[d].min) / boxSize[d]);
      // NOTE: No bounds checking is done (for performance).
      //if (i < 0 || i >= int(split[d])) return;

      // Accumulate the index
      index += (i * splitCumul[d]);
    }

    // Add it to the contained box
    boxes[index]->addEvent(event);

    // Track the total signal
#ifdef MDEVENTS_MDGRIDBOX_ONGOING_SIGNAL_CACHE
    statsMutex.lock();
    this->m_signal += event.getSignal();
    this->m_errorSquared += event.getErrorSquared();
    statsMutex.unlock();
#endif
  }





  //-----------------------------------------------------------------------------------------------
  /** Save the box and contents to an open nexus file.
   *
   * @param groupName :: name of the group to save in.
   * @param file :: Nexus File object
   */
  TMDE(
  void MDGridBox)::saveNexus(const std::string & groupName, ::NeXus::File * file)
  {
    // Create the group.
    file->makeGroup(groupName, "NXMDGridBox", 1);

    // First, save the data common to all IMDBoxes.
    IMDBox<MDE,nd>::saveNexus(groupName, file);

    // More attributes
    file->putAttr("numBoxes", int(this->numBoxes));
    file->putAttr("nPoints", int(this->nPoints)); // TODO: should be a long

    // The splitting, as a vector
    std::vector<int> splitVec;
    for (size_t d=0; d<nd; d++)
      splitVec.push_back( int(split[d]) );
    file->writeData("split", splitVec);

    // The rest can be calculated when loading.

    // Now save each box contained within
    for (size_t i=0; i<numBoxes; i++)
    {
      IMDBox<MDE,nd> * ibox = boxes[i];

      // Name sub-boxes "box0, box1", etc
      std::ostringstream mess;
      mess << "box" << i;
      std::string subGroupName = mess.str();

      ibox->saveNexus(subGroupName, file);
    }

    file->closeGroup();
  }


  //-----------------------------------------------------------------------------------------------
  /** Load the box and contents from an open nexus file.
   *
   * @param file :: Nexus File object
   */
  TMDE(
  void MDGridBox)::loadNexus(::NeXus::File * file)
  {
    // The group is already open
    file->initGroupDir();

    // Load common stuff
    IMDBox<MDE,nd>::loadNexus(file);

    // Some attributes
    int ival; //TODO: Use long; nexus needs to support it.
    file->getAttr("numBoxes", ival);
    if (ival < 0) throw std::runtime_error("MDGridBox::loadNexus: Error loading NXS file. numBoxs must be >= 0. Found at path " + file->getPath());
    this->numBoxes = size_t(ival);

    file->getAttr("nPoints", ival);
    if (ival < 0) throw std::runtime_error("MDGridBox::loadNexus: Error loading NXS file. nPoints must be >= 0. Found at path " + file->getPath());
    this->nPoints = size_t(ival);

    // Load the splitting info
    std::vector<int> splitVec;
    file->openData("split");
    file->getData(splitVec);
    file->closeData();

    if (splitVec.size() != nd)
      throw std::runtime_error("MDGridBox::loadNexus: Invalid splitting criterion (does not match number of dimensions). Found at path " + file->getPath());

    for (size_t d=0; d<nd; d++)
      split[d] = splitVec[d];

    // This computes the common stuff from it
    size_t tot = computeFromSplit();
    if (tot == 0)
      throw std::runtime_error("MDGridBox::loadNexus: Invalid splitting criterion (one was zero). Found at path " + file->getPath());
    if (tot != numBoxes)
      throw std::runtime_error("MDGridBox::loadNexus: Invalid splitting criterion (total number of boxes does not match numBoxes). Found at path " + file->getPath());

    // Clear all the boxes
    boxes.resize(numBoxes, NULL);

    // Iterate through all sub groups
    std::pair<std::string, std::string> name_class;
    name_class = file->getNextEntry();
    while (name_class.first != "NULL")
    {
      std::string name = name_class.first;

      // Find the index using the name of the group
      if (name.size() > 3)
      {
        // Take out the "box" part of the name
        std::string indexStr = name.substr(3, name.size()-3);
        // Extract the index number
        size_t index;
        Kernel::Strings::convert(indexStr, index);

        if (index >= numBoxes)
          throw std::runtime_error("MDGridBox::loadNexus: Invalid index found when parsing group " + name + ". Found at path " + file->getPath());

        if (name_class.second == "NXMDBox")
        {
          file->openGroup(name_class.first, name_class.second);
          MDBox<MDE,nd> * box = new MDBox<MDE,nd>();
          box->loadNexus(file);
          file->closeGroup();
          boxes[index] = box;
        }
        else if (name_class.second == "NXMDGridBox")
        {
          file->openGroup(name_class.first, name_class.second);
          MDGridBox<MDE,nd> * box = new MDGridBox<MDE,nd>();
          box->loadNexus(file);
          file->closeGroup();
          boxes[index] = box;
        }
        else
        {
          // std::cout << name_class.first << ", " << name_class.second << std::endl;
          if (name_class.second != "SDS")
            throw std::runtime_error("MDGridBox::loadNexus: Unexpected sub-group class (" + name_class.second + "). Found at path " + file->getPath());
        }
      }
      // Next one
      name_class = file->getNextEntry();
    }

    // Check that all boxes were created
    for (size_t i=0; i<numBoxes; i++)
    {
      if (!boxes[i])
        throw std::runtime_error("MDGridBox::loadNexus: A sub-box was not loaded and is NULL. Found at path " + file->getPath());
    }
  }



  //-----------------------------------------------------------------------------------------------
  /** Perform centerpoint binning of events, with bins defined
   * in axes perpendicular to the axes of the workspace.
   *
   * @param bin :: MDBin object giving the limits of events to accept.
   * @param fullyContained :: optional bool array sized [nd] of which dimensions are known to be fully contained (for MDSplitBox)
   */
  TMDE(
  void MDGridBox)::centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const
  {

    // The MDBin ranges from index_min to index_max (inclusively) if each dimension. So
    // we'll need to make nested loops from index_min[0] to index_max[0]; from index_min[1] to index_max[1]; etc.
    int index_min[nd];
    int index_max[nd];
    // For running the nested loop, counters of each dimension. These are bounded by 0..split[d]
    size_t counters_min[nd];
    size_t counters_max[nd];

    for (size_t d=0; d<nd; d++)
    {
      int min,max;

      // The min index in this dimension (we round down - we'll include this edge)
      if (bin.m_min[d] >= this->extents[d].min)
      {
        min = int((bin.m_min[d] - this->extents[d].min) / boxSize[d]);
        counters_min[d] = min;
      }
      else
      {
        min = -1; // Goes past the edge
        counters_min[d] = 0;
      }

      // If the minimum is bigger than the number of blocks in that dimension, then the bin is off completely in
      //  that dimension. There is nothing to integrate.
      if (min >= static_cast<int>(split[d]))
        return;
      index_min[d] = min;

      // The max index in this dimension (we round UP, but when we iterate we'll NOT include this edge)
      if (bin.m_max[d] < this->extents[d].max)
      {
        max = int(ceil((bin.m_max[d] - this->extents[d].min) / boxSize[d])) - 1;
        counters_max[d] = max+1; // (the counter looping will NOT include counters_max[d])
      }
      else
      {
        max = int(split[d]); // Goes past THAT edge
        counters_max[d] = max; // (the counter looping will NOT include max)
      }

      // If the max value is before the min, that means NOTHING is in the bin, and we can return
      if ((max < min) || (max < 0))
        return;
      index_max[d] = max;

      //std::cout << d << " from " << std::setw(5) << index_min[d] << " to " << std::setw(5)  << index_max[d] << "inc" << std::endl;
    }

    // If you reach here, than at least some of bin is overlapping this box
    size_t counters[nd];
    for (size_t d=0; d<nd; d++)
      counters[d] = counters_min[d];

    bool allDone = false;
    while (!allDone)
    {
      size_t index = getLinearIndex(counters);
      //std::cout << index << ": " << counters[0] << ", " << counters[1] << std::endl;

      // Find if the box is COMPLETELY held in the bin.
      bool completelyWithin = true;
      for(size_t dim=0; dim<nd; dim++)
        if ((static_cast<int>(counters[dim]) <= index_min[dim]) ||
            (static_cast<int>(counters[dim]) >= index_max[dim]))
        {
          // The index we are at is at the edge of the integrated area (index_min or index_max-1)
          // That means that the bin only PARTIALLY covers this MDBox
          completelyWithin = false;
          break;
        }

      if (completelyWithin)
      {
        // Box is completely in the bin.
        //std::cout << "Box at index " << counters[0] << ", " << counters[1] << " is entirely contained.\n";
        // Use the aggregated signal and error
        bin.m_signal += boxes[index]->getSignal();
        bin.m_errorSquared += boxes[index]->getErrorSquared();
      }
      else
      {
        // Perform the binning
        boxes[index]->centerpointBin(bin,fullyContained);
      }

      // Increment the counter(s) in the nested for loops.
      allDone = Utils::nestedForLoopIncrement(nd, counters, counters_max, counters_min);
    }

  }




  //-----------------------------------------------------------------------------------------------
  /** General (non-axis-aligned) centerpoint binning method.
   * TODO: TEST THIS!
   *
   * @param bin :: a MDBin object giving the limits, aligned with the axes of the workspace,
   *        of where the non-aligned bin MIGHT be present.
   * @param function :: a ImplicitFunction that will evaluate true for any coordinate that is
   *        contained within the (non-axis-aligned) bin.
   */
  TMDE(
  void MDGridBox)::generalBin(MDBin<MDE,nd> & bin, Mantid::API::ImplicitFunction & function) const
  {
    // The MDBin ranges from index_min to index_max (inclusively) if each dimension. So
    // we'll need to make nested loops from index_min[0] to index_max[0]; from index_min[1] to index_max[1]; etc.
    int index_min[nd];
    int index_max[nd];
    // For running the nested loop, counters of each dimension. These are bounded by 0..split[d]
    size_t counters_min[nd];
    size_t counters_max[nd];

    for (size_t d=0; d<nd; d++)
    {
      int min,max;

      // The min index in this dimension (we round down - we'll include this edge)
      if (bin.m_min[d] >= this->extents[d].min)
      {
        min = int((bin.m_min[d] - this->extents[d].min) / boxSize[d]);
        counters_min[d] = min;
      }
      else
      {
        min = -1; // Goes past the edge
        counters_min[d] = 0;
      }

      // If the minimum is bigger than the number of blocks in that dimension, then the bin is off completely in
      //  that dimension. There is nothing to integrate.
      if (min >= static_cast<int>(split[d]))
        return;
      index_min[d] = min;

      // The max index in this dimension (we round UP, but when we iterate we'll NOT include this edge)
      if (bin.m_max[d] < this->extents[d].max)
      {
        max = int(ceil((bin.m_max[d] - this->extents[d].min) / boxSize[d])) - 1;
        counters_max[d] = max+1; // (the counter looping will NOT include counters_max[d])
      }
      else
      {
        max = int(split[d]); // Goes past THAT edge
        counters_max[d] = max; // (the counter looping will NOT include max)
      }

      // If the max value is before the min, that means NOTHING is in the bin, and we can return
      if ((max < min) || (max < 0))
        return;
      index_max[d] = max;

      //std::cout << d << " from " << std::setw(5) << index_min[d] << " to " << std::setw(5)  << index_max[d] << "inc" << std::endl;
    }

    // If you reach here, than at least some of bin is overlapping this box


    // We start by looking at the vertices at every corner of every box contained,
    // to see which boxes are partially contained/fully contained.

    // One entry with the # of vertices in this box contained; start at 0.
    size_t * verticesContained = new size_t[numBoxes];
    memset( verticesContained, 0, numBoxes * sizeof(size_t) );

    // Set to true if there is a possibility of the box at least partly touching the integration volume.
    bool * boxMightTouch = new bool[numBoxes];
    memset( boxMightTouch, 0, numBoxes * sizeof(bool) );

    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
    size_t maxVertices = 1 << nd;

    // The index to the vertex in each dimension
    size_t * vertexIndex = Utils::nestedForLoopSetUp(nd, 0);

    // This is the index in each dimension at which we start looking at vertices
    size_t * vertices_min = Utils::nestedForLoopSetUp(nd, 0);
    for (size_t d=0; d<nd; ++d)
    {
      vertices_min[d] = counters_min[d];
      vertexIndex[d] = vertices_min[d]; // This is where we start
    }

    // There is one more vertex in each dimension than there are boxes we are considering
    size_t * vertices_max = Utils::nestedForLoopSetUp(nd, 0);
    for (size_t d=0; d<nd; ++d)
      vertices_max[d] = counters_max[d]+1;

    size_t * boxIndex = Utils::nestedForLoopSetUp(nd, 0);
    size_t * indexMaker = Utils::nestedForLoopSetUpIndexMaker(nd, split);

    bool allDone = false;
    while (!allDone)
    {
      // Coordinates of this vertex
      coord_t vertexCoord[nd];
      for (size_t d=0; d<nd; ++d)
        vertexCoord[d] = double(vertexIndex[d]) * boxSize[d] + this->extents[d].min;

      // Is this vertex contained?
      if (function.evaluate(vertexCoord))
      {
        // Yes, this vertex is contained within the integration volume!
//        std::cout << "vertex at " << vertexCoord[0] << ", " << vertexCoord[1] << ", " << vertexCoord[2] << " is contained\n";

        // This vertex is shared by up to 2^nd adjacent boxes (left-right along each dimension).
        for (size_t neighb=0; neighb<maxVertices; ++neighb)
        {
          // The index of the box is the same as the vertex, but maybe - 1 in each possible combination of dimensions
          bool badIndex = false;
          // Build the index of the neighbor
          for (size_t d=0; d<nd;d++)
          {
            boxIndex[d] = vertexIndex[d] - ((neighb & (1 << d)) >> d); //(this does a bitwise and mask, shifted back to 1 to subtract 1 to the dimension)
            // Taking advantage of the fact that unsigned(0)-1 = some large POSITIVE number.
            if (boxIndex[d] >= split[d])
            {
              badIndex = true;
              break;
            }
          }
          if (!badIndex)
          {
            // Convert to linear index
            size_t linearIndex = Utils::nestedForLoopGetLinearIndex(nd, boxIndex, indexMaker);
            // So we have one more vertex touching this box that is contained in the integration volume. Whew!
            verticesContained[linearIndex]++;
//            std::cout << "... added 1 vertex to box " << boxes[linearIndex]->getExtentsStr() << "\n";
          }
        }
      }

      // Increment the counter(s) in the nested for loops.
      allDone = Utils::nestedForLoopIncrement(nd, vertexIndex, vertices_max, vertices_min);
    }

    // OK, we've done all the vertices. Now we go through and check each box.
    size_t numFullyContained = 0;
    //size_t numPartiallyContained = 0;

    // We'll iterate only through the boxes with (bin)
    size_t counters[nd];
    for (size_t d=0; d<nd; d++)
      counters[d] = counters_min[d];

    allDone = false;
    while (!allDone)
    {
      size_t index = getLinearIndex(counters);
      IMDBox<MDE,nd> * box = boxes[index];

      // Is this box fully contained?
      if (verticesContained[index] >= maxVertices)
      {
        // Use the integrated sum of signal in the box
        bin.m_signal += box->getSignal();
        bin.m_errorSquared += box->getErrorSquared();
        numFullyContained++;
      }
      else
      {
        // The box MAY be contained. Need to evaluate every event

        // box->generalBin(bin,function);
      }

      // Increment the counter(s) in the nested for loops.
      allDone = Utils::nestedForLoopIncrement(nd, counters, counters_max, counters_min);
    }

//    std::cout << "Depth " << this->getDepth() << " with " << numFullyContained << " fully contained; " << numPartiallyContained << " partial. Signal = " << signal <<"\n";

    delete [] verticesContained;
    delete [] boxMightTouch;
    delete [] vertexIndex;
    delete [] vertices_max;
    delete [] boxIndex;
    delete [] indexMaker;

  }





  //-----------------------------------------------------------------------------------------------
  /** Integrate the signal within a sphere; for example, to perform single-crystal
   * peak integration.
   * The CoordTransform object could be used for more complex shapes, e.g. "lentil" integration, as long
   * as it reduces the dimensions to a single value.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param[out] signal :: set to the integrated signal
   * @param[out] errorSquared :: set to the integrated squared error.
   */
  TMDE(
  void MDGridBox)::integrateSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const
  {
    // We start by looking at the vertices at every corner of every box contained,
    // to see which boxes are partially contained/fully contained.

    // One entry with the # of vertices in this box contained; start at 0.
    size_t * verticesContained = new size_t[numBoxes];
    memset( verticesContained, 0, numBoxes * sizeof(size_t) );

    // Set to true if there is a possibility of the box at least partly touching the integration volume.
    bool * boxMightTouch = new bool[numBoxes];
    memset( boxMightTouch, 0, numBoxes * sizeof(bool) );

    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
    size_t maxVertices = 1 << nd;

    // The number of vertices in each dimension is the # split[d] + 1
    size_t * vertices_max = Utils::nestedForLoopSetUp(nd, 0);
    for (size_t d=0; d<nd; ++d)
      vertices_max[d] = split[d]+1;

    // The index to the vertex in each dimension
    size_t * vertexIndex = Utils::nestedForLoopSetUp(nd, 0);
    size_t * boxIndex = Utils::nestedForLoopSetUp(nd, 0);
    size_t * indexMaker = Utils::nestedForLoopSetUpIndexMaker(nd, split);

    bool allDone = false;
    while (!allDone)
    {
      // Coordinates of this vertex
      coord_t vertexCoord[nd];
      for (size_t d=0; d<nd; ++d)
        vertexCoord[d] = double(vertexIndex[d]) * boxSize[d] + this->extents[d].min;

      // Is this vertex contained?
      coord_t out[nd];
      radiusTransform.apply(vertexCoord, out);
      if (out[0] < radiusSquared)
      {
        // Yes, this vertex is contained within the integration volume!
//        std::cout << "vertex at " << vertexCoord[0] << ", " << vertexCoord[1] << ", " << vertexCoord[2] << " is contained\n";

        // This vertex is shared by up to 2^nd adjacent boxes (left-right along each dimension).
        for (size_t neighb=0; neighb<maxVertices; ++neighb)
        {
          // The index of the box is the same as the vertex, but maybe - 1 in each possible combination of dimensions
          bool badIndex = false;
          // Build the index of the neighbor
          for (size_t d=0; d<nd;d++)
          {
            boxIndex[d] = vertexIndex[d] - ((neighb & (1 << d)) >> d); //(this does a bitwise and mask, shifted back to 1 to subtract 1 to the dimension)
            // Taking advantage of the fact that unsigned(0)-1 = some large POSITIVE number.
            if (boxIndex[d] >= split[d])
            {
              badIndex = true;
              break;
            }
          }
          if (!badIndex)
          {
            // Convert to linear index
            size_t linearIndex = Utils::nestedForLoopGetLinearIndex(nd, boxIndex, indexMaker);
            // So we have one more vertex touching this box that is contained in the integration volume. Whew!
            verticesContained[linearIndex]++;
//            std::cout << "... added 1 vertex to box " << boxes[linearIndex]->getExtentsStr() << "\n";
          }
        }
      }

      // Increment the counter(s) in the nested for loops.
      allDone = Utils::nestedForLoopIncrement(nd, vertexIndex, vertices_max);
    }

    // OK, we've done all the vertices. Now we go through and check each box.
    size_t numFullyContained = 0;
    size_t numPartiallyContained = 0;

    for (size_t i=0; i < numBoxes; ++i)
    {
      IMDBox<MDE, nd> * box = boxes[i];
      // Box partially contained?
      bool partialBox = false;

      // Is this box fully contained?
      if (verticesContained[i] >= maxVertices)
      {
        // Use the integrated sum of signal in the box
        signal += box->getSignal();
        errorSquared += box->getErrorSquared();

//        std::cout << "box at " << i << " (" << box->getExtentsStr() << ") is fully contained. Vertices = " << verticesContained[i] << "\n";

        numFullyContained++;
        // Go on to the next box
        continue;
      }

      if (verticesContained[i] == 0)
      {
        // There is a chance that this part of the box is within integration volume,
        // even if no vertex of it is.
        coord_t boxCenter[nd];
        box->getCenter(boxCenter);

        // Distance from center to the peak integration center
        coord_t out[nd];
        radiusTransform.apply(boxCenter, out);

        if (out[0] < diagonalSquared*0.72 + radiusSquared)
        {
          // If the center is closer than the size of the box, then it MIGHT be touching.
          // (We multiply by 0.72 (about sqrt(2)) to look for half the diagonal).
          // NOTE! Watch out for non-spherical transforms!
//          std::cout << "box at " << i << " is maybe touching\n";
          partialBox = true;
        }
      }
      else
      {
        partialBox = true;
//        std::cout << "box at " << i << " has a vertex touching\n";
      }

      // We couldn't rule out that the box might be partially contained.
      if (partialBox)
      {
        // Use the detailed integration method.
        box->integrateSphere(radiusTransform, radiusSquared, signal, errorSquared);
//        std::cout << ".signal=" << signal << "\n";
        numPartiallyContained++;
      }
    } // (for each box)

//    std::cout << "Depth " << this->getDepth() << " with " << numFullyContained << " fully contained; " << numPartiallyContained << " partial. Signal = " << signal <<"\n";

    delete [] verticesContained;
    delete [] boxMightTouch;
    delete [] vertexIndex;
    delete [] vertices_max;
    delete [] boxIndex;
    delete [] indexMaker;
  }


  //-----------------------------------------------------------------------------------------------
  /** Find the centroid of all events contained within by doing a weighted average
   * of their coordinates.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param[out] centroid :: array of size [nd]; its centroid will be added
   * @param[out] signal :: set to the integrated signal
   */
  TMDE(
  void MDGridBox)::centroidSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      // Go through each contained box
      IMDBox<MDE, nd> * box = boxes[i];
      coord_t boxCenter[nd];
      box->getCenter(boxCenter);

      // Distance from center to the peak integration center
      coord_t out[nd];
      radiusTransform.apply(boxCenter, out);

      if (out[0] < diagonalSquared*0.72 + radiusSquared)
      {
        // If the center is closer than the size of the box, then it MIGHT be touching.
        // (We multiply by 0.72 (about sqrt(2)) to look for half the diagonal).
        // NOTE! Watch out for non-spherical transforms!

        // Go down one level to keep centroiding
        box->centroidSphere(radiusTransform, radiusSquared, centroid, signal);
      }
    } // (for each box)
  }


}//namespace MDEvents

}//namespace Mantid

