#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/ThreadPool.h"
#include <numeric>

using namespace boost::posix_time;

namespace Mantid
{
namespace DataObjects
{

  DECLARE_WORKSPACE(EventWorkspace)

  using Kernel::Exception::NotImplementedError;
  using namespace Mantid::Kernel;

  // get a reference to the logger
  Kernel::Logger& EventWorkspace::g_log
                 = Kernel::Logger::get("EventWorkspace");

  //---- Constructors -------------------------------------------------------------------
  EventWorkspace::EventWorkspace() : m_bufferedDataY(), m_bufferedDataE(),
      m_cachedNumberOfEvents(0)
  {
  }

  EventWorkspace::~EventWorkspace()
  {
    //std::cout << "EventWorkspace " << this->getName() << " is being deleted.\n";
    //Make sure you free up the memory in the MRUs
    for (size_t i=0; i < m_bufferedDataY.size(); i++)
      if (m_bufferedDataY[i])
      {
        m_bufferedDataY[i]->clear();
        delete m_bufferedDataY[i];
      };
    m_bufferedDataY.clear();

    for (size_t i=0; i < m_bufferedDataE.size(); i++)
      if (m_bufferedDataE[i])
      {
        m_bufferedDataE[i]->clear();
        delete m_bufferedDataE[i];
      };
    m_bufferedDataE.clear();

    //Go through the event list and clear them?
    EventListVector::iterator i = this->data.begin();
    for( ; i != this->data.end(); ++i )
    {
      //Deleting the event list should call its destructor to release the vector memory.
      delete (*i);
    }
  }

  //-----------------------------------------------------------------------------
  /** Returns true if the EventWorkspace is safe for multithreaded operations.
   */
  bool EventWorkspace::threadSafe() const
  {
    //Return false if ANY event list is not sorted. You can't have 2 threads trying to sort the
    //  same event list simultaneously.
    for (int i=0; i<static_cast<int>(data.size()); i++)
      if (!data[i]->isSortedByTof())
        return false;
    return true;
  }


  //-----------------------------------------------------------------------------
  /** Initialize the pixels
    *  @param NVectors :: The number of vectors/histograms/detectors in the workspace. Does not need
    *         to be set, but needs to be > 0
    *  @param XLength :: The number of X data points/bin boundaries in each vector (ignored)
    *  @param YLength :: The number of data/error points in each vector (ignored)
   */
  void EventWorkspace::init(const int &NVectors, const int &XLength, const int &YLength)
  {
    (void) YLength; //Avoid compiler warning

    // Check validity of arguments
    if (NVectors <= 0)
    {
      g_log.error("Negative or 0 Number of Pixels specified to EventWorkspace::init");
      throw std::out_of_range("Negative or 0 Number of Pixels specified to EventWorkspace::init");
    }
    //Initialize the data
    m_noVectors = NVectors;
    data.resize(m_noVectors, NULL);
    //Make sure SOMETHING exists for all initialized spots.
    for (int i=0; i < m_noVectors; i++)
      data[i] = new EventList();
    this->done_loading_data = false;

    //Create axes.
    m_axes.resize(2);
    //I'm not sure what the heck this first axis is supposed to be; copying from Workspace2D
    m_axes[0] = new API::RefAxis(XLength, this);
    // Ok, looks like the second axis is supposed to be the spectrum # for each entry in the workspace index
    //  I have no idea why it is such a convoluted way of doing it.
    m_axes[1] = new API::SpectraAxis(m_noVectors);

  }

  //-----------------------------------------------------------------------------
  /**
   * Copy all of the data (event lists) from the source workspace to this workspace.
   *
   * @param source: EventWorkspace from which we are taking data.
   * @param sourceStartWorkspaceIndex: index in the workspace of source where we start
   *          copying the data. This index will be 0 in the "this" workspace.
   *          Default: -1, meaning copy all.
   * @param sourceEndWorkspaceIndex: index in the workspace of source where we stop.
   *          It is inclusive = source[sourceEndWorkspaceIndex[ WILL be copied.
   *          Default: -1, meaning copy all.
   *
   */
  void EventWorkspace::copyDataFrom(const EventWorkspace& source, int sourceStartWorkspaceIndex, int sourceEndWorkspaceIndex)
  {
    //Start with nothing.
    this->clearData(); //properly de-allocates memory!

    //Copy the vector of EventLists
    EventListVector source_data = source.data;
    EventListVector::iterator it;
    EventListVector::iterator it_start = source_data.begin();
    EventListVector::iterator it_end = source_data.end();
    int source_data_size = static_cast<int>(source_data.size());

    //Do we copy only a range?
    if ((sourceStartWorkspaceIndex >=0) && (sourceStartWorkspaceIndex < source_data_size)
        && (sourceEndWorkspaceIndex >= 0) && (sourceEndWorkspaceIndex < source_data_size)
        && (sourceEndWorkspaceIndex >= sourceStartWorkspaceIndex))
    {
      it_start = source_data.begin() + sourceStartWorkspaceIndex;
      it_end = source_data.begin() + sourceEndWorkspaceIndex + 1;
    }

    for (it = it_start; it != it_end; it++ )
    {
      //Create a new event list, copying over the events
      EventList * newel = new EventList( **it );
      this->data.push_back(newel);
    }
    //Save the number of vectors
    m_noVectors = this->data.size();

    this->clearMRU();
    //The data map is useless now
    this->data_map.clear();
    //This marker needs to be set.
    this->done_loading_data = true;
  }


  //-----------------------------------------------------------------------------
  /// The total size of the workspace
  /// @returns the number of single indexable items in the workspace
  int EventWorkspace::size() const
  {
    return this->data.size() * this->blocksize();
  }

  //-----------------------------------------------------------------------------
  /// Get the blocksize, aka the number of bins in the histogram
  /// @returns the number of bins in the Y data
  int EventWorkspace::blocksize() const
  {
    // Pick the first pixel to find the blocksize.
    EventListVector::const_iterator it = data.begin();
    if (it == data.end())
    {
      throw std::range_error("EventWorkspace::blocksize, no pixels in workspace, therefore cannot determine blocksize (# of bins).");
    }
    else
    {
      return (*it)->histogram_size();
    }
  }

  //-----------------------------------------------------------------------------
  /** Get the number of histograms, usually the same as the number of pixels or detectors. 
  @returns the number of histograms / event lists
  */
  int EventWorkspace::getNumberHistograms() const
  {
    return this->data.size();
  }

  //-----------------------------------------------------------------------------
  /// The total number of events across all of the spectra.
  /// @returns The total number of events
  size_t EventWorkspace::getNumberEvents() const
  {
    size_t total = 0;
    for (EventListVector::const_iterator it = this->data.begin();
        it != this->data.end(); it++) {
      total += (*it)->getNumberEvents();
    }
    return total;
  }

  //-----------------------------------------------------------------------------
  /** Get the EventType of the most-specialized EventList in the workspace
   *
   * @return the EventType of the most-specialized EventList in the workspace
   */
  Mantid::API::EventType EventWorkspace::getEventType() const
  {
    Mantid::API::EventType out = Mantid::API::TOF;
    for (EventListVector::const_iterator it = this->data.begin();
        it != this->data.end(); it++)
    {
      Mantid::API::EventType thisType = (*it)->getEventType();
      if (static_cast<int>(out) < static_cast<int>(thisType))
      {
        out = thisType;
        // This is the most-specialized it can get.
        if (out == Mantid::API::WEIGHTED_NOTIME) return out;
      }
    }
    return out;
  }


  //-----------------------------------------------------------------------------
  /// Returns true always - an EventWorkspace always represents histogramm-able data
  /// @returns If the data is a histogram - always true for an eventWorkspace
  bool EventWorkspace::isHistogramData() const
  {
    return true;
  }

  //-----------------------------------------------------------------------------
  /** Return how many entries in the Y MRU list are used.
   * Only used in tests. It only returns the 0-th MRU list size.
   * @return :: number of entries in the MRU list.
   */
  int EventWorkspace::MRUSize() const
  {
    return this->m_bufferedDataY[0]->size();
  }

  //-----------------------------------------------------------------------------
  /** Clears the MRU lists */
  void EventWorkspace::clearMRU() const
  {
    //Make sure you free up the memory in the MRUs
    for (size_t i=0; i < m_bufferedDataY.size(); i++)
      if (m_bufferedDataY[i])
      {
        m_bufferedDataY[i]->clear();
      };

    for (size_t i=0; i < m_bufferedDataE.size(); i++)
      if (m_bufferedDataE[i])
      {
        m_bufferedDataE[i]->clear();
      };
  }

  //-----------------------------------------------------------------------------
  /** Clear the data[] vector and delete
   * any EventList objects in it
   */
  void EventWorkspace::clearData()
  {
    m_noVectors = data.size();
    for (int i=0; i < m_noVectors; i++)
    {
      if (data[i])
        delete data[i];
    }
    data.clear();
    m_noVectors = 0;
  }

  //-----------------------------------------------------------------------------
  /// Returns the amount of memory used in bytes
  size_t EventWorkspace::getMemorySize() const
  {
    size_t  total = 0;

    //Add up the two buffers
    total += (this->m_bufferedDataY.size() + this->m_bufferedDataE.size()) * this->blocksize() * sizeof(double);

    // Add the memory from all the event lists
    for (EventListVector::const_iterator it = this->data.begin();
        it != this->data.end(); it++)
    {
      total += (*it)->getMemorySize();
    }

    total += m_run->getMemorySize();

    total += this->getMemorySizeForXAxes();

    // Return in bytes
    return total;
  }

  //-----------------------------------------------------------------------------
  // --- Data Access ----
  //-----------------------------------------------------------------------------

  /** Used during data loading: gets, or CREATES an EventList object for a given pixelid.
   * If never referred to before, an empty EventList (holding just the right pixel ID) is returned.
   * Not thread-safe!
   *
   * @param pixelID :: pixelID of this event list. This is not necessarily the same as the workspace Index.
   * @returns A reference to the eventlist
   */
  EventList& EventWorkspace::getEventListAtPixelID(const int pixelID)
  {
    if (this->done_loading_data)
      throw std::runtime_error("EventWorkspace::getEventListAtPixelID called after doneLoadingData(). Try getEventList() instead.");
    //An empty entry will be made if needed
    EventListMap::iterator it = this->data_map.find(pixelID);
    if (it == this->data_map.end())
    {
      //Need to make a new one!
      EventList * newel = new EventList();
      //Set the (single) entry in the detector ID set
      newel->addDetectorID( pixelID );
      //Save it in the map
      this->data_map[pixelID] = newel;
      return (*newel);
    }
    else
    {
      //Already exists; return it (deref the pointer)
      return *(it->second);
    }
  }

  //-----------------------------------------------------------------------------
  /** Get an EventList object at the given workspace index number
   * @param workspace_index :: The histogram workspace index number.
   * @returns A reference to the eventlist
   */
  EventList& EventWorkspace::getEventList(const int workspace_index)
  {
    EventList * result = data[workspace_index];
    if (!result)
      throw std::runtime_error("EventWorkspace::getEventList: NULL EventList found.");
    else
      return *result;
  }

  //-----------------------------------------------------------------------------
  /** Get a const EventList object at the given workspace index number
   * @param workspace_index :: The workspace index number.
   * @returns A const reference to the eventlist
   */
  const EventList& EventWorkspace::getEventList(const int workspace_index) const
  {
    EventList * result = data[workspace_index];
    if (!result)
      throw std::runtime_error("EventWorkspace::getEventList (const): NULL EventList found.");
    else
      return *result;
  }


  //-----------------------------------------------------------------------------
  /// Get an EventList pointer at the given workspace index number
  EventList * EventWorkspace::getEventListPtr(const int workspace_index)
  {
    return data[workspace_index];
  }


  //-----------------------------------------------------------------------------
  /** Either return an existing EventList from the list, or
   * create a new one if needed and expand the list.
   * NOTE: After you are done adding event lists, call doneAddingEventLists()
   *  to finalize the stuff that needs to.
   **
   * @param workspace_index :: The workspace index number.
   * @return An event list (new or existing) at the index provided
   */
  EventList& EventWorkspace::getOrAddEventList(const int workspace_index)
    {
    if (workspace_index < 0)
      throw std::invalid_argument("EventWorkspace::getOrAddEventList: workspace_index < 0 is not valid.");

    int old_size = static_cast<int>(data.size());
    if (workspace_index >= old_size)
    {
      //Increase the size of the eventlist lists.
      for (int wi = old_size; wi <= workspace_index; wi++)
      {
        //Need to make a new one!
        EventList * newel = new EventList();
        //Add to list
        this->data.push_back(newel);
      }
      m_noVectors = data.size();
    }

    //Now it should be safe to return the value
    EventList * result = data[workspace_index];
    if (!result)
      throw std::runtime_error("EventWorkspace::getOrAddEventList: NULL EventList found.");
    else
      return *result;
  }


  //-----------------------------------------------------------------------------
  /** Pad the workspace with empty event lists for all the
   * detectors in the instrument.
   * Can do it in parallel, though my time tests show it takes MORE time in parallel :(
   * This calls doneAddingEventLists() to finalize after the end.
   *
   * @param parallel: set to true to perform this padding in parallel, which
   *        may increase speed, though my tests show it slows it down.
   *
   */
  void EventWorkspace::padPixels(bool parallel)
  {

    //Build a vector with the pixel IDs in order.
    std::vector<int> pixelIDs;

    std::map<int, Geometry::IDetector_sptr> detector_map = this->getInstrument()->getDetectors();
    std::map<int, Geometry::IDetector_sptr>::iterator it;
    for (it = detector_map.begin(); it != detector_map.end(); it++)
    {
      //Go through each pixel in the map, but forget monitors.
      if (!it->second->isMonitor())
      {
        pixelIDs.push_back(it->first); //it->first is detector ID #
      }
    }
    int numpixels = pixelIDs.size();

    //Remove all old EventLists and resize the vector to hold everything
    this->clearData();
    data.resize(numpixels);
    m_noVectors = numpixels;

    //Do each block in parallel
    PARALLEL_FOR_IF(parallel)
    for (int i = 0; i < numpixels; i++)
    {
      //Create an event list for here
      EventList * newel = new EventList();
      //Set the (single) entry in the detector ID set
      newel->addDetectorID( pixelIDs[i] );
      //Save it in the list
      data[i] = newel;
    }

    //Finalize by building the spectra map, etc.
    this->makeAxis1();

    //Now, make the spectra map (index -> detector ID)
    API::SpectraDetectorMap& myMap = mutableSpectraMap();
    myMap.clear();
    myMap.populateWithVector(pixelIDs);

    //Clearing the MRU list is a good idea too.
    this->clearMRU();

    //Marker makes it okay to go on.
    done_loading_data = true;



//    //First, we build a list of blocks of pixel IDs
//    std::vector< std::vector<int> * > pixel_blocks;
//    //How many pixels per block? Use a large enough size to make sense to launch a thread.
//    int block_size = 1000;
//
//    //initialize a block
//    int i=0;
//    int numpixels=0;
//    std::vector<int> * block = new std::vector<int>();
//
//    //Go through all the detectors
//    std::map<int, Geometry::IDetector_sptr> detector_map = this->getInstrument()->getDetectors();
//    std::map<int, Geometry::IDetector_sptr>::iterator it;
//    for (it = detector_map.begin(); it != detector_map.end(); it++)
//    {
//      //Go through each pixel in the map, but forget monitors.
//      if (!it->second->isMonitor())
//      {
//        block->push_back(it->first); //it->first is detector ID #
//        i++;
//        if (i >= block_size)
//        {
//          //Filled up a block!
//          pixel_blocks.push_back(block);
//          numpixels += block->size();
//          block = new std::vector<int>();
//          i = 0;
//        }
//      }
//    }
//
//    //Push the last block
//    if (i > 0)
//    {
//      pixel_blocks.push_back(block);
//      numpixels += block->size();
//    }
//
//    int numblocks = static_cast<int>(pixel_blocks.size());
//    std::vector< std::vector<EventList *> * > el_blocks(numblocks);
//    //Do each block in parallel
//    PARALLEL_FOR_IF(parallel)
//    for (i = 0; i < numblocks; i++)
//    {
//      std::vector<int> * myblock = pixel_blocks[i];
//      int myblock_size = myblock->size();
//      std::vector<EventList *> * my_el_block = new std::vector<EventList *>(); //(myblock_size);
//
//      for (int j=0; j<myblock_size; j++)
//      {
//        //Create an event list for here
//        EventList * newel = new EventList();
//        //Set the (single) entry in the detector ID set
//        newel->addDetectorID( (*myblock)[j] );
//        //Save it in the local event list block.
//        //(*my_el_block)[j] = newel;
//        my_el_block->push_back( newel );
//      }
//
//      //Save the local event list block in the master list.
//      el_blocks[i] = my_el_block;
//      delete myblock;
//    }
//
//    //Clear any old event lists.
//    this->clearData();
//
//    //Now we append the blocks but NOT in parallel.
//    for (i = 0; i < numblocks; i++)
//    {
//      //Copy all the EventLists from the block into the main list
//      this->data.insert(data.end(), el_blocks[i]->begin(), el_blocks[i]->end());
//      //Done with that.
//      delete el_blocks[i];
//    }
//    m_noVectors = data.size();
//
//    //Finalize by building the spectra map, etc.
//    this->doneAddingEventLists();
  }

  //-----------------------------------------------------------------------------
  /** Generate the spectra map (map between spectrum # and detector IDs)
   * by using the info in each EventList.
   */
  void EventWorkspace::makeSpectraMap()
  {
    //Flush the existing one
    API::SpectraDetectorMap& myMap = mutableSpectraMap();
    myMap.clear();

    //Go through all the spectra
    for (int wi=0; wi<this->m_noVectors; wi++)
    {
      //And copy over the set of detector IDs. Not the smartest way to do it, but we are kinda stuck.
      myMap.addSpectrumEntries(wi, this->data[wi]->getDetectorIDs() );
    }
  }



  //-----------------------------------------------------------------------------
  /** Generate the axes[1] (the mapping between workspace index and spectrum number)
   * as a stupid 1:1 map.
   */
  void EventWorkspace::makeAxis1()
  {
    //We create a spectra-type axis that holds the spectrum # at each workspace index.
    //  It is a simple 1-1 map (workspace index = spectrum #)
    delete m_axes[1];
    API::SpectraAxis * ax1 = new API::SpectraAxis(m_noVectors);
    ax1->populateSimple(m_noVectors);
    m_axes[1] = ax1;
  }


  //-----------------------------------------------------------------------------
  /** Call this method when you are done manually adding event lists
   * at specific workspace indices.
   * The spectra map and axis#1 are populated:
   *      makeSpectraMap() to map to detector IDs
   *      makeAxis1() to map workspace index to spectrum number
   */
  void EventWorkspace::doneAddingEventLists()
  {
    //Make the wi to spectra map
    this->makeAxis1();

    //Now, make the spectra map (index -> detector ID)
    this->makeSpectraMap();

    //Clearing the MRU list is a good idea too.
    this->clearMRU();

    //Marker makes it okay to go on.
    done_loading_data = true;
  }


  //-----------------------------------------------------------------------------
  /** Call this method when loading event data is complete (adding events
   * by calling getEventListAtPixelId() )
   *
   * The map of pixelid to spectrum # is generated.
   * Also, a simple 1:1 map of spectrum # to pixel id (detector #) is generated
   * */
  void EventWorkspace::doneLoadingData()
  {
    //Ok, we need to take the data_map, and turn it into a data[] vector.

    //Clear and delete any old EventList's
    this->clearData();

    //Now resize
    m_noVectors = this->data_map.size();
    this->data.resize(m_noVectors, NULL);

    int counter = 0;
    EventListMap::iterator it;
    for (it = this->data_map.begin(); it != this->data_map.end(); it++)
    {
      //Copy the pointer to the event list in there.
      this->data[counter] = it->second;
      //Increase the workspace index
      counter++;
    }

    //Make the wi to spectra map
    this->makeAxis1();

    //Now, make the spectra map (index -> detector ID)
    this->makeSpectraMap();

    //Now clear the data_map. We don't need it anymore
    this->data_map.clear();

    //Cache the # of events (for getMemorySize)
    this->m_cachedNumberOfEvents = this->getNumberEvents();

    //Set the flag for raising errors later
    this->done_loading_data = true;
  }


  //-----------------------------------------------------------------------------
  /// Return the data X vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index :: the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataX(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataX cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  /// Return the data Y vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index :: the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataY(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataY cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  /// Return the data E vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index :: the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataE(const int index)
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataE cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }




  //-----------------------------------------------------------------------------
  // --- Const Data Access ----
  //-----------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  /** This function makes sure that there are enough data
   * buffers (MRU's) for E for the number of threads requested.
   */
  void EventWorkspace::ensureEnoughBuffersE(int thread_num) const
  {
    if (thread_num < 0)
      throw std::runtime_error("EventWorkspace::ensureEnoughBuffersE() called with a negative thread number.");

    PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
    {
      if (static_cast<int>(m_bufferedDataE.size()) <= thread_num)
      {
        m_bufferedDataE.resize(thread_num+1);
        for (size_t i=0; i < m_bufferedDataE.size(); i++)
        {
          if (!m_bufferedDataE[i])
            m_bufferedDataE[i] = new mru_list(50); //Create a MRU list with this many entries.

        }
      }
    }
  }

  //---------------------------------------------------------------------------
  /** This function makes sure that there are enough data
   * buffers (MRU's) for Y for the number of threads requested.
   */
  void EventWorkspace::ensureEnoughBuffersY(int thread_num) const
  {
    if (thread_num < 0)
      throw std::runtime_error("EventWorkspace::ensureEnoughBuffersY() called with a negative thread number.");

    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      if (static_cast<int>(m_bufferedDataY.size()) <= thread_num)
      {
        m_bufferedDataY.resize(thread_num+1);
        for (size_t i=0; i < m_bufferedDataY.size(); i++)
        {
          if (!m_bufferedDataY[i])
            m_bufferedDataY[i] = new mru_list(50); //Create a MRU list with this many entries.

        }
      }
    }
  }


  //---------------------------------------------------------------------------
  /// Return the const data X vector at a given workspace index
  const MantidVec& EventWorkspace::dataX(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    return this->data[index]->dataX();
  }



  //---------------------------------------------------------------------------
  /// Return the const data Y vector at a given workspace index
  const MantidVec& EventWorkspace::dataY(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");

    // This is the thread number from which this function was called.
    int thread = PARALLEL_THREAD_NUMBER;
    this->ensureEnoughBuffersY(thread);

    //Is the data in the mrulist?
    MantidVecWithMarker * data;
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      data = m_bufferedDataY[thread]->find(index);
    }

    if (data == NULL)
    {
      //Create the MRU object
      data = new MantidVecWithMarker(index);
      //Set the Y data in it
      MantidVec E_ignored;
      this->data[index]->generateHistogram( *this->data[index]->getRefX(), data->m_data, E_ignored, true);

      //Lets save it in the MRU
      MantidVecWithMarker * oldData;
      PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
      {
        oldData = m_bufferedDataY[thread]->insert(data);
      }

      //And clear up the memory of the old one, if it is dropping out.
      if (oldData)
        delete oldData;
    }
    return data->m_data;
  }


  //---------------------------------------------------------------------------
  /// Return the const data E vector at a given workspace index
  const MantidVec& EventWorkspace::dataE(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");

    // This is the thread number from which this function was called.
    int thread = PARALLEL_THREAD_NUMBER;
    this->ensureEnoughBuffersE(thread);

    //Is the data in the mrulist?
    MantidVecWithMarker * data;
    PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
    {
      data = m_bufferedDataE[thread]->find(index);
    }

    if (data == NULL)
    {
      //Get a handle on the event list.
      const EventList * el = this->data[index];

      //Create the MRU object
      data = new MantidVecWithMarker(index);

      //Now use that to get E -- Y values are generated from another function
      MantidVec Y;
      el->generateHistogram(*(el->getRefX()), Y, data->m_data);

      //Lets save it in the MRU
      MantidVecWithMarker * oldData;
      PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
      {
        oldData = m_bufferedDataE[thread]->insert(data);
      }
      //And clear up the memory of the old one, if it is dropping out.
      if (oldData)
      {
        //std::cout << "Dropping " << oldData->m_index << " from thread " << omp_get_thread_num() << "\n";
        delete oldData;
      }
    }
    return data->m_data;
  }

  //---------------------------------------------------------------------------
  /// Get a pointer to the x data at the given workspace index
  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const int index) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::refX, histogram number out of range");
    return this->data[index]->getRefX();
  }


  //---------------------------------------------------------------------------
  /** Returns a read-only (i.e. const) reference to both the Y
   * and E arrays.
   * This will retrieve both from the MRU list if they are there, or will
   * generate both histograms at the same time, to save speed.
   *
   * @param index :: workspace index to retrieve.
   * @param[out] Y :: reference to the pointer to the const data vector
   * @param[out] E :: reference to the pointer to the const error vector
   */
  void EventWorkspace::readYE(int const index, MantidVec const*& Y, MantidVec const*& E) const
  {
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::readYE, histogram number out of range");

    // This is the thread number from which this function was called.
    int thread = PARALLEL_THREAD_NUMBER;
    this->ensureEnoughBuffersE(thread);
    this->ensureEnoughBuffersY(thread);

    //Is the data in the mrulist?
    MantidVecWithMarker * data;
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      data = m_bufferedDataY[thread]->find(index);
    }
    MantidVecWithMarker * data_errors;
    PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
    {
      data_errors = m_bufferedDataE[thread]->find(index);
    }

    if ((data == NULL) || (data_errors == NULL))
    {
      //Get a handle on the event list.
      const EventList * el = this->data[index];

      //Create the MRU object
      data = new MantidVecWithMarker(index);
      data_errors = new MantidVecWithMarker(index);

      //Now use that to get E -- Y values are generated from another function
      el->generateHistogram(*(el->getRefX()), data->m_data, data_errors->m_data);

      //Lets save it in the MRUs
      MantidVecWithMarker * oldData;

      PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
      {        oldData = m_bufferedDataY[thread]->insert(data);      }
      if (oldData)
        delete oldData;

      PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
      {        oldData = m_bufferedDataE[thread]->insert(data_errors);      }
      if (oldData)
        delete oldData;

    }

    Y = &data->m_data;
    E = &data_errors->m_data;
  }


  //-----------------------------------------------------------------------------
  // --- Histogramming ----
  //-----------------------------------------------------------------------------
  /*** Set a histogram X vector. Should only be called after doneLoadingData().
   * @param index :: Workspace histogram index to set.
   * @param x :: The X vector of histogram bins to use.
   */
  void EventWorkspace::setX(const int index, const Kernel::cow_ptr<MantidVec> &x)
  {
    if (!this->done_loading_data)
      throw std::runtime_error("EventWorkspace::setX called before doneLoadingData().");
    if ((index >= this->m_noVectors) || (index < 0))
      throw std::range_error("EventWorkspace::setX, histogram number out of range");
    this->data[index]->setX(x);
  }


  /*** Set a histogram X vector but create a COW pointer for it. Should only be called after doneLoadingData().
   * @param index :: Workspace histogram index to set.
   * @param x :: The X vector of histogram bins to use.
   */
  void EventWorkspace::setX(const int index, const MantidVec &X)
  {
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.assign(X.begin(), X.end());
    this->setX(index, axis);
  }


  //-----------------------------------------------------------------------------
  /*** Set all histogram X vectors. Should only be called after doneLoadingData().
   * @param x :: The X vector of histogram bins to use.
   */
  void EventWorkspace::setAllX(Kernel::cow_ptr<MantidVec> &x)
  {
    if (!this->done_loading_data)
      throw std::runtime_error("EventWorkspace::setAllX called before doneLoadingData().");
    //int counter=0;
    EventListVector::iterator i = this->data.begin();
    for( ; i != this->data.end(); ++i )
    {
      (*i)->setX(x);
    }

    //Clear MRU lists now, free up memory
    this->clearMRU();
  }

  //-----------------------------------------------------------------------------
  /** Task for sorting an event list */
  class EventSortingTask : public Task
  {
  public:
    /// ctor
    EventSortingTask(const EventWorkspace * WS, int wiStart, int wiStop, EventSortType sortType, size_t howManyCores, Mantid::API::Progress * prog)
    : Task(), m_wiStart(wiStart), m_wiStop(wiStop),  m_sortType(sortType), m_howManyCores(howManyCores), m_WS(WS), prog(prog)
    {
      m_cost = 0;
      if (m_wiStop > m_WS->getNumberHistograms())
        m_wiStop = m_WS->getNumberHistograms();

      for (int wi=m_wiStart; wi < m_wiStop; wi++)
      {
        double n = m_WS->getEventList(wi).getNumberEvents();
        // Sorting time is approximately n * ln (n)
        m_cost += n * log(n);
      }

      if (! ((m_howManyCores == 1)||(m_howManyCores == 2)||(m_howManyCores == 4)))
        throw std::invalid_argument("howManyCores should be 1,2 or 4.");
    }

    // Execute the sort as specified.
    void run()
    {
      if (!m_WS) return;
      for (int wi=m_wiStart; wi < m_wiStop; wi++)
      {
        if (m_sortType != TOF_SORT)
          m_WS->getEventList(wi).sort(m_sortType);
        else
        {
          if (m_howManyCores == 1)
            m_WS->getEventList(wi).sort(m_sortType);
          else if (m_howManyCores == 2)
            m_WS->getEventList(wi).sortTof2();
          else if (m_howManyCores == 4)
            m_WS->getEventList(wi).sortTof4();
        }
        // Report progress
        if (prog) prog->report("Sorting");
      }
    }

  private:
    /// Start workspace index to process
    int m_wiStart;
    /// Stop workspace index to process
    int m_wiStop;
    /// How to sort
    EventSortType m_sortType;
    /// How many cores for each sort
    size_t m_howManyCores;
    /// EventWorkspace on which to sort
    const EventWorkspace * m_WS;
    /// Optional Progress dialog.
    Mantid::API::Progress * prog;
  };


  //-----------------------------------------------------------------------------
  /*** Sort all event lists. Uses a parallelized algorithm
   * @param sortType :: How to sort the event lists.
   * @param prog :: a progress report object. If the pointer is not NULL, each event list will call prog.report() once.
   */
  void EventWorkspace::sortAll(EventSortType sortType, Mantid::API::Progress * prog) const
  {
    int num_threads;
    num_threads = ThreadPool::getNumPhysicalCores();
    g_log.debug() << num_threads << " cores found. ";

    // Initial chunk size: set so that each core will be called for 20 tasks.
    // (This is to avoid making too small tasks.)
    int chunk_size = m_noVectors/(num_threads*20);
    if (chunk_size < 1) chunk_size = 1;

    // Sort with 1 core per event list by default
    size_t howManyCores = 1;
    // And auto-detect how many threads
    size_t howManyThreads = 0;
    if (m_noVectors < num_threads*10)
    {
      // If you have few vectors, sort with 2 cores.
      chunk_size = 1;
      howManyCores = 2;
      howManyThreads = num_threads / 2 + 1;
    }
    else if (m_noVectors < num_threads)
    {
      // If you have very few vectors, sort with 4 cores.
      chunk_size = 1;
      howManyCores = 4;
      howManyThreads = num_threads / 4 + 1;
    }
    g_log.debug() << "Performing sort with " << howManyCores << " cores per EventList, in " << howManyThreads << " threads, using a chunk size of " << chunk_size << ".\n";

    // Create the thread pool, and optimize by doing the longest sorts first.
    ThreadPool pool(new ThreadSchedulerLargestCost(), howManyThreads);
    for (int i=0; i < m_noVectors; i+=chunk_size)
    {
      pool.schedule(new EventSortingTask(this, i, i+chunk_size, sortType, howManyCores, prog));
    }

    // Now run it all
    pool.joinAll();
  }



  //-----------------------------------------------------------------------------
  /** Return the time of the first pulse received, by accessing the run's
   * sample logs to find the proton_charge
   *
   * @return the time of the first pulse
   * @throw runtime_error if the log is not found; or if it is empty.
   */
  Kernel::DateAndTime EventWorkspace::getFirstPulseTime() const
  {
    TimeSeriesProperty<double>* log = dynamic_cast<TimeSeriesProperty<double>*> (this->run().getLogData("proton_charge"));
    if (!log)
      throw std::runtime_error("EventWorkspace::getFirstPulseTime: No TimeSeriesProperty called 'proton_charge' found in the workspace.");
    DateAndTime startDate = log->firstTime();
    //Return as DateAndTime.
    return startDate;
  }

  //-----------------------------------------------------------------------------
  /** Return the time of the last pulse received, by accessing the run's
   * sample logs to find the proton_charge
   *
   * @return the time of the first pulse
   * @throw runtime_error if the log is not found; or if it is empty.
   */
  Kernel::DateAndTime EventWorkspace::getLastPulseTime() const
  {
    TimeSeriesProperty<double>* log = dynamic_cast<TimeSeriesProperty<double>*> (this->run().getLogData("proton_charge"));
    if (!log)
      throw std::runtime_error("EventWorkspace::getFirstPulseTime: No TimeSeriesProperty called 'proton_charge' found in the workspace.");
    DateAndTime stopDate = log->lastTime();
    //Return as DateAndTime.
    return stopDate;
  }


  //---------------------------------------------------------------------------------------
  /** Integrate all the spectra in the matrix workspace within the range given.
   * Default implementation, can be overridden by base classes if they know something smarter!
   *
   * @param out :: returns the vector where there is one entry per spectrum in the workspace. Same
   *            order as the workspace indices.
   * @param minX :: minimum X bin to use in integrating.
   * @param maxX :: maximum X bin to use in integrating.
   * @param entireRange :: set to true to use the entire range. minX and maxX are then ignored!
   */
  void EventWorkspace::getIntegratedSpectra(std::vector<double> & out, const double minX, const double maxX, const bool entireRange) const
  {
    //Start with empty vector
    out.resize(this->getNumberHistograms(), 0.0);

    //We can run in parallel since there is no cross-reading of event lists
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int wksp_index = 0; wksp_index < this->getNumberHistograms(); wksp_index++)
    {
      // Get Handle to data
      EventList * el = this->data[wksp_index];

      //Let the eventList do the integration
      out[wksp_index] = el->integrate(minX, maxX, entireRange);
    }
  }



} // namespace DataObjects
} // namespace Mantid


///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef, Mantid::DataObjects::EventWorkspace>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::DataObjects::EventWorkspace>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::EventWorkspace>;

namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
    Mantid::DataObjects::EventWorkspace_sptr IPropertyManager::getValue<Mantid::DataObjects::EventWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type (EventWorkspace_sptr)";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::EventWorkspace_const_sptr IPropertyManager::getValue<Mantid::DataObjects::EventWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::EventWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type (EventWorkspace_sptr)";
        throw std::runtime_error(message);
      }
    }
  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
