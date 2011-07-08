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
#include <limits>
#include <numeric>
#include "MantidAPI/ISpectrum.h"

using namespace boost::posix_time;
using Mantid::API::ISpectrum;

namespace Mantid
{
namespace DataObjects
{

  DECLARE_WORKSPACE(EventWorkspace)

  using Kernel::Exception::NotImplementedError;
  using std::size_t;
  using namespace Mantid::Kernel;

  // get a reference to the logger
  Kernel::Logger& EventWorkspace::g_log
                 = Kernel::Logger::get("EventWorkspace");

  //---- Constructors -------------------------------------------------------------------
  EventWorkspace::EventWorkspace() :
      mru(new EventWorkspaceMRU)
  {
  }

  EventWorkspace::~EventWorkspace()
  {
    delete mru;

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
    for (size_t i=0; i<data.size(); i++)
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
  void EventWorkspace::init(const size_t &NVectors, const size_t &XLength, const size_t &YLength)
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
    for (size_t i=0; i < m_noVectors; i++)
      data[i] = new EventList(mru, specid_t(i));

    //Create axes.
    m_axes.resize(2);
    //I'm not sure what the heck this first axis is supposed to be; copying from Workspace2D
    m_axes[0] = new API::RefAxis(XLength, this);
    // Spectrum axis
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
  void EventWorkspace::copyDataFrom(const EventWorkspace& source, size_t sourceStartWorkspaceIndex, size_t sourceEndWorkspaceIndex)
  {
    //Start with nothing.
    this->clearData(); //properly de-allocates memory!

    //Copy the vector of EventLists
    EventListVector source_data = source.data;
    EventListVector::iterator it;
    EventListVector::iterator it_start = source_data.begin();
    EventListVector::iterator it_end = source_data.end();
    size_t source_data_size = source_data.size();

    //Do we copy only a range?
    if( sourceEndWorkspaceIndex == size_t(-1) ) sourceEndWorkspaceIndex = source_data_size - 1;
    if ((sourceStartWorkspaceIndex < source_data_size) && (sourceEndWorkspaceIndex < source_data_size)
        && (sourceEndWorkspaceIndex >= sourceStartWorkspaceIndex))
    {
      it_start += sourceStartWorkspaceIndex;
      it_end = source_data.begin() + sourceEndWorkspaceIndex + 1;
    }


    for (it = it_start; it != it_end; it++ )
    {
      //Create a new event list, copying over the events
      EventList * newel = new EventList( **it );
      // Make sure to update the MRU to point to THIS event workspace.
      newel->setMRU(this->mru);
      this->data.push_back(newel);
    }
    //Save the number of vectors
    m_noVectors = this->data.size();

    // Regenerate the dumb spectramap
    this->generateSpectraMap();
    this->clearMRU();
  }


  //-----------------------------------------------------------------------------
  /// The total size of the workspace
  /// @returns the number of single indexable items in the workspace
  size_t EventWorkspace::size() const
  {
    return this->data.size() * this->blocksize();
  }

  //-----------------------------------------------------------------------------
  /// Get the blocksize, aka the number of bins in the histogram
  /// @returns the number of bins in the Y data
  size_t EventWorkspace::blocksize() const
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
  size_t EventWorkspace::getNumberHistograms() const
  {
    return this->data.size();
  }


  //--------------------------------------------------------------------------------------------
  /// Return the underlying ISpectrum ptr at the given workspace index.
  Mantid::API::ISpectrum * EventWorkspace::getSpectrum(const size_t index)
  {
    if (index>=m_noVectors)
      throw std::range_error("EventWorkspace::getSpectrum, workspace index out of range");
    return data[index];
  }

  /// Return the underlying ISpectrum ptr at the given workspace index.
  const Mantid::API::ISpectrum * EventWorkspace::getSpectrum(const size_t index) const
  {
    if (index>=m_noVectors)
      throw std::range_error("EventWorkspace::getSpectrum, workspace index out of range");
    return data[index];
  }











  //-----------------------------------------------------------------------------


  double EventWorkspace::getTofMin() const
  {
    double tmin = std::numeric_limits<double>::max();
    double temp;
    size_t numWorkspace = this->data.size();
    for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
    {
      const EventList evList = this->getEventList(workspaceIndex);
      if (evList.getNumberEvents() > 0)
      {
        temp = evList.getTofMin();
        if (temp < tmin)
          tmin = temp;
      }
    }
    return tmin;
  }

  double EventWorkspace::getTofMax() const
  {
    double tmax = -1.*std::numeric_limits<double>::max(); // min is a small number, not negative
    double temp;
    size_t numWorkspace = this->data.size();
    for (size_t workspaceIndex = 0; workspaceIndex < numWorkspace; workspaceIndex++)
    {
      const EventList evList = this->getEventList(workspaceIndex);
      if (evList.getNumberEvents() > 0)
      {
        temp = evList.getTofMax();
        if (temp > tmax)
          tmax = temp;
      }
    }
    return tmax;
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
  /** Switch all event lists to the given event type
   *
   * @param type :: EventType to switch to
   */
  void EventWorkspace::switchEventType(const Mantid::API::EventType type)
  {
    for (EventListVector::const_iterator it = this->data.begin();
        it != this->data.end(); it++)
    {
      (*it)->switchTo(type);
    }
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
  size_t EventWorkspace::MRUSize() const
  {
    return mru->MRUSize();
  }

  //-----------------------------------------------------------------------------
  /** Clears the MRU lists */
  void EventWorkspace::clearMRU() const
  {
    mru->clear();
  }

  //-----------------------------------------------------------------------------
  /** Clear the data[] vector and delete
   * any EventList objects in it
   */
  void EventWorkspace::clearData()
  {
    m_noVectors = data.size();
    for (size_t i=0; i < m_noVectors; i++)
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

    //TODO: Add the MRU buffer

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

  //-----------------------------------------------------------------------------
  /** Get an EventList object at the given workspace index number
   * @param workspace_index :: The histogram workspace index number.
   * @returns A reference to the eventlist
   */
  EventList& EventWorkspace::getEventList(const size_t workspace_index)
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
  const EventList& EventWorkspace::getEventList(const size_t workspace_index) const
  {
    EventList * result = data[workspace_index];
    if (!result)
      throw std::runtime_error("EventWorkspace::getEventList (const): NULL EventList found.");
    else
      return *result;
  }


  //-----------------------------------------------------------------------------
  /** Get an EventList pointer at the given workspace index number
   * @param workspace_index :: index into WS
   * @return an EventList pointer at the given workspace index number
   */
  EventList * EventWorkspace::getEventListPtr(const size_t workspace_index)
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
  EventList& EventWorkspace::getOrAddEventList(const size_t workspace_index)
  {
    size_t old_size = data.size();
    if (workspace_index >= old_size)
    {
      //Increase the size of the eventlist lists.
      for (size_t wi = old_size; wi <= workspace_index; wi++)
      {
        //Need to make a new one!
        EventList * newel = new EventList(mru, specid_t(wi) );
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


  // 
  //-----------------------------------------------------------------------------
  /** Pad the workspace with empty event lists for all the spectra in the workspace.
   * This requires that a non-empty spectra-detector map
   * Can do it in parallel, though my time tests show it takes MORE time in parallel :(
   * This calls doneAddingEventLists() to finalize after the end.
   *
   * @param parallel: set to true to perform this padding in parallel, which
   *        may increase speed, though my tests show it slows it down.
   *
   */
  void EventWorkspace::padSpectra()
  {
    using Geometry::ISpectraDetectorMap;
    const ISpectraDetectorMap & spectramap = this->spectraMap();
    const size_t numSpectra = spectramap.nSpectra();
    if( numSpectra == 0 )
    {
      throw std::runtime_error("EventWorkspace::padSpectra - The spectra-detector map has not been "
                               "populated.");
    }
    API::Axis *ax1 = getAxis(1);
    if( !ax1 )
    {
      throw std::runtime_error("EventWorkspace::padSpectra - NULL axis 1");
    }
    if( !ax1->isSpectra() )
    {
      throw std::runtime_error("EventWorkspace::padSpectra - Axis 1 is not a SpectraAxis");
    }
    
    // Remove all old EventLists and resize the vector to hold everything
    this->clearData();
    data.resize(numSpectra);
    m_noVectors = numSpectra;
    
    for( size_t wi = 0; wi < numSpectra; ++wi )
    {
      const specid_t specNo = ax1->spectraNo(wi);
      //Create an event list for here
      EventList * newel = new EventList(mru, specNo);
      newel->addDetectorID( specNo );
      newel->setSpectrumNo( specNo );
      //Save it in the list
      data[wi] = newel;
    }           
    
    //Clearing the MRU list is a good idea too.
    this->clearMRU();
  }

  void EventWorkspace::deleteEmptyLists()
  {
    // figure out how much data to copy
    size_t orig_length = this->data.size();
    size_t new_length = 0;
    for (size_t i = 0; i < orig_length; i++)
    {
      if (!(this->data[i]->empty()))
        new_length++;
    }

    // copy over the data
    EventListVector notEmpty;
    notEmpty.reserve(new_length);
    for (size_t i = 0; i < orig_length; i++)
    {
      if (!(this->data[i]->empty()))
        notEmpty.push_back(this->data[i]);
      else
        delete this->data[i];
    }

    // replace the old vector
    this->data.swap(notEmpty);

    // fix spectra map
    this->m_noVectors = this->data.size();
    this->generateSpectraMap();

    //Clearing the MRU list is a good idea too.
    this->clearMRU();
  }

//  //-----------------------------------------------------------------------------
//  /** Generate the spectra map (map between spectrum # and detector IDs)
//   * by using the info in each EventList.
//   */
//  void EventWorkspace::makeSpectraMap()
//  {
//    API::Axis *ax1 = getAxis(1);
//    if( !ax1 )
//    {
//      throw std::runtime_error("EventWorkspace::makeSpectraMap - NULL axis 1");
//    }
//    if( !ax1->isSpectra() )
//    {
//      throw std::runtime_error("EventWorkspace::makeSpectraMap - Axis 1 is not a SpectraAxis");
//    }
//
//    API::SpectraDetectorMap *newMap = new API::SpectraDetectorMap;
//
//    //Go through all the spectra
//    for (size_t wi=0; wi<this->m_noVectors; wi++)
//    {
//      const std::set<detid_t> ids = this->data[wi]->getDetectorIDs();
//      newMap->addSpectrumEntries(*(ids.begin()), ids);
//    }
//    this->replaceSpectraMap(newMap);
//  }
//
//  //-----------------------------------------------------------------------------
//  /** Generate the axes[1] (the mapping between workspace index and spectrum number)
//   * as a stupid 1:1 map.
//   */
//  void EventWorkspace::makeAxis1()
//  {
//    // We create a spectra-type axis that holds the spectrum # at each workspace index.
//    // based off the current spectra map if it has entries
//    delete m_axes[1];
//    if( this->spectraMap().nElements() > 0 )
//    {
//      m_axes[1] = new API::SpectraAxis(this->m_noVectors,this->spectraMap());
//    }
//    else
//    {
//      m_axes[1] = new API::SpectraAxis(m_noVectors);
//    }
//  }


  //-----------------------------------------------------------------------------
  /** Call this method when you are done manually adding event lists
   * at specific workspace indices.
   * The spectra map and axis#1 are populated:
   *      makeSpectraMap() to map to detector IDs
   *      makeAxis1() to map workspace index to spectrum number
   */
  void EventWorkspace::doneAddingEventLists()
  {
    //Now, make the spectra map (index -> detector ID)
    //Make the wi to spectra map
    this->generateSpectraMap();

    //Clearing the MRU list is a good idea too.
    this->clearMRU();
  }



  //-----------------------------------------------------------------------------
  /// Return the data X vector at a given workspace index
  /// Note: the MRUlist should be cleared before calling getters for the Y or E data
  /// @param index :: the workspace index to return
  /// @returns A reference to the vector of binned X values
  MantidVec& EventWorkspace::dataX(const size_t index)
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    return this->data[index]->dataX();
  }

  /// Return the data X error vector at a given workspace index
  /// Note: the MRUlist should be cleared before calling getters for the Y or E data
  /// @param index :: the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataDx(const size_t index)
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataDx, histogram number out of range");
    return this->data[index]->dataDx();
  }

  /// Return the data Y vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index :: the workspace index to return
  /// @returns A reference to the vector of binned Y values
  MantidVec& EventWorkspace::dataY(const size_t index)
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataY cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  /// Return the data E vector at a given workspace index
  /// Note: these non-const access methods will throw NotImplementedError
  /// @param index :: the workspace index to return
  /// @returns A reference to the vector of binned error values
  MantidVec& EventWorkspace::dataE(const size_t index)
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    throw NotImplementedError("EventWorkspace::dataE cannot return a non-const array: you can't modify the histogrammed data in an EventWorkspace!");
  }

  //-----------------------------------------------------------------------------
  // --- Const Data Access ----
  //-----------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  /** @return the const data X vector at a given workspace index
   * @param index :: workspace index   */
  const MantidVec& EventWorkspace::dataX(const size_t index) const
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataX, histogram number out of range");
    return this->data[index]->dataX();
  }

  /** @return the const data X error vector at a given workspace index
   * @param index :: workspace index   */
  const MantidVec& EventWorkspace::dataDx(const size_t index) const
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataDx, histogram number out of range");
    return this->data[index]->dataDx();
  }



  //---------------------------------------------------------------------------
  /** @return the const data Y vector at a given workspace index
   * @param index :: workspace index   */
  const MantidVec& EventWorkspace::dataY(const size_t index) const
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataY, histogram number out of range");
    const MantidVec& out = this->data[index]->constDataY();
    return out;
  }


  //---------------------------------------------------------------------------
  /** @return the const data E (error) vector at a given workspace index
   * @param index :: workspace index   */
  const MantidVec& EventWorkspace::dataE(const size_t index) const
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::dataE, histogram number out of range");
    const MantidVec& out = this->data[index]->constDataE();
    return out;
  }

  //---------------------------------------------------------------------------
  /** @return a pointer to the X data vector at a given workspace index
   * @param index :: workspace index   */
  Kernel::cow_ptr<MantidVec> EventWorkspace::refX(const size_t index) const
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::refX, histogram number out of range");
    return this->data[index]->ptrX();
  }



  //-----------------------------------------------------------------------------
  // --- Histogramming ----
  //-----------------------------------------------------------------------------
  /*** Set a histogram X vector.
   * Performance note: use setAllX() if you are setting identical X for all pixels.
   *
   * @param index :: Workspace histogram index to set.
   * @param x :: The X vector of histogram bins to use.
   */
  void EventWorkspace::setX(const size_t index, const Kernel::cow_ptr<MantidVec> &x)
  {
    if (index >= this->m_noVectors)
      throw std::range_error("EventWorkspace::setX, histogram number out of range");
    this->data[index]->setX(x);

    //TOD: M<ove this inside EVentList::setX()
    // Assume that any manual changing of X invalidates the MRU list entry for this index
    // This deletes only the entry at this index.
    this->mru->deleteIndex(this->data[index]->getSpectrumNo());
  }


  /*** Set a histogram X vector but create a COW pointer for it.
   * Performance note: use setAllX() if you are setting identical X for all pixels.
   *
   * @param index :: Workspace histogram index to set.
   * @param x :: The X vector of histogram bins to use.
   */
  void EventWorkspace::setX(const size_t index, const MantidVec &X)
  {
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.assign(X.begin(), X.end());
    this->setX(index, axis);
  }


  //-----------------------------------------------------------------------------
  /*** Set all histogram X vectors.
   * @param x :: The X vector of histogram bins to use.
   */
  void EventWorkspace::setAllX(Kernel::cow_ptr<MantidVec> &x)
  {
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
    EventSortingTask(const EventWorkspace * WS, size_t wiStart, size_t wiStop, EventSortType sortType, size_t howManyCores, Mantid::API::Progress * prog)
    : Task(), m_wiStart(wiStart), m_wiStop(wiStop),  m_sortType(sortType), m_howManyCores(howManyCores), m_WS(WS), prog(prog)
    {
      m_cost = 0;
      if (m_wiStop > m_WS->getNumberHistograms())
        m_wiStop = m_WS->getNumberHistograms();

      for (size_t wi=m_wiStart; wi < m_wiStop; wi++)
      {
        double n = static_cast<double>(m_WS->getEventList(wi).getNumberEvents());
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
      for (size_t wi=m_wiStart; wi < m_wiStop; wi++)
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
    size_t m_wiStart;
    /// Stop workspace index to process
    size_t m_wiStop;
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
    size_t num_threads;
    num_threads = ThreadPool::getNumPhysicalCores();
    g_log.debug() << num_threads << " cores found. ";

    // Initial chunk size: set so that each core will be called for 20 tasks.
    // (This is to avoid making too small tasks.)
    size_t chunk_size = m_noVectors/(num_threads*20);
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
    for (size_t i=0; i < m_noVectors; i+=chunk_size)
    {
      pool.schedule(new EventSortingTask(this, i, i+chunk_size, sortType, howManyCores, prog));
    }

    // Now run it all
    pool.joinAll();
  }



  //-----------------------------------------------------------------------------
  /** Return the time of the first pulse received, by accessing the run's
   * sample logs to find the proton_charge.
   *
   * NOTE, JZ: Pulse times before 1991 (up to 100) are skipped. This is to avoid
   * a DAS bug at SNS around Mar 2011 where the first pulse time is Jan 1, 1990.
   *
   * @return the time of the first pulse
   * @throw runtime_error if the log is not found; or if it is empty.
   */
  Kernel::DateAndTime EventWorkspace::getFirstPulseTime() const
  {
    TimeSeriesProperty<double>* log = dynamic_cast<TimeSeriesProperty<double>*> (this->run().getLogData("proton_charge"));
    if (!log)
      throw std::runtime_error("EventWorkspace::getFirstPulseTime: No TimeSeriesProperty called 'proton_charge' found in the workspace.");
    DateAndTime startDate;
    DateAndTime reference("1991-01-01");

    int i=0;
    startDate = log->nthTime(i);

    // Find the first pulse after 1991
    while (startDate < reference && i < 100)
    {
      i++;
      startDate = log->nthTime(i);
    }

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
    for (int wksp_index = 0; wksp_index < int(this->getNumberHistograms()); wksp_index++)
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
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected EventWorkspace.";
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
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const EventWorkspace.";
        throw std::runtime_error(message);
      }
    }
  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
