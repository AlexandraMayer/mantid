#include <stdexcept>
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/DateAndTime.h"
#include <functional>
#include <math.h>

using std::ostream;
using std::runtime_error;
using std::size_t;
using std::vector;

namespace Mantid
{
namespace DataObjects
{
using Kernel::Exception::NotImplementedError;
using Kernel::DateAndTime;

//==========================================================================
/// --------------------- TofEvent stuff ----------------------------------
//==========================================================================
  /** Constructor, specifying the time of flight and the frame id
   * @param tof time of flight, in microseconds
   * @param pulsetime absolute pulse time of the neutron.
   */
  TofEvent::TofEvent(const double tof, const DateAndTime pulsetime) :
              m_tof(tof), m_pulsetime(pulsetime)
  {
  }

  /** Constructor, copy from another TofEvent object
   * @param rhs Other TofEvent to copy.
   */
  TofEvent::TofEvent(const TofEvent& rhs) :
      m_tof(rhs.m_tof), m_pulsetime(rhs.m_pulsetime)
  {
  }

  /// Empty constructor
  TofEvent::TofEvent() :
            m_tof(0), m_pulsetime(0)
  {
  }

  /// Destructor
  TofEvent::~TofEvent()
  {
  }

  /** () operator: return the tof (X value) of the event.
   * This is useful for std operations like comparisons
   * and std::lower_bound
   */
  double TofEvent::operator()() const
  {
    return this->m_tof;
  }


  /** Copy from another TofEvent object
   * @param rhs Other TofEvent to copy.
   * @return reference to this.
   */
  TofEvent& TofEvent::operator=(const TofEvent& rhs)
  {
    this->m_tof = rhs.m_tof;
    this->m_pulsetime = rhs.m_pulsetime;
    return *this;
  }

  /** Comparison operator.
   * @param rhs: the other TofEvent to compare.
   * @return true if the TofEvent's are identical.*/
  bool TofEvent::operator==(const TofEvent & rhs) const
  {
    return  (this->m_tof == rhs.m_tof) &&
            (this->m_pulsetime == rhs.m_pulsetime);
  }

  /** < comparison operator, using the TOF to do the comparison.
   * @param rhs: the other TofEvent to compare.
   * @return true if this->m_tof < rhs.m_tof*/
  bool TofEvent::operator<(const TofEvent & rhs) const
  {
    return (this->m_tof < rhs.m_tof);
  }

  /** < comparison operator, using the TOF to do the comparison.
   * @param rhs: the other TofEvent to compare.
   * @return true if this->m_tof < rhs.m_tof*/
  bool TofEvent::operator>(const TofEvent & rhs) const
  {
    return (this->m_tof > rhs.m_tof);
  }

  /** < comparison operator, using the TOF to do the comparison.
   * @param rhs_tof: the other time of flight to compare.
   * @return true if this->m_tof < rhs.m_tof*/
  bool TofEvent::operator<(const double rhs_tof) const
  {
    return (this->m_tof < rhs_tof);
  }

  /// Return the time of flight, as a double, in nanoseconds.
  double TofEvent::tof() const
  {
	  return this->m_tof;
  }

  /// Return the frame id
  DateAndTime TofEvent::pulseTime() const
  {
	  return this->m_pulsetime;
  }

  /** Output a string representation of the event to a stream
   * @param os Stream
   * @param event TofEvent to output to the stream
   */
  ostream& operator<<(ostream &os, const TofEvent &event)
  {
    os << event.m_tof << "," << event.m_pulsetime.to_simple_string();
    return os;
  }





  //==========================================================================
  /// --------------------- WeightedEvent stuff ----------------------------------
  //==========================================================================

  /** Constructor, full:
   * @param tof: tof in microseconds.
   * @param pulsetime: absolute pulse time
   * @param weight: weight of this neutron event.
   * @param errorSquared: the square of the error on the event
   */
  WeightedEvent::WeightedEvent(double tof, const Mantid::Kernel::DateAndTime pulsetime, float weight, float errorSquared)
  : TofEvent(tof, pulsetime), m_weight(weight), m_errorSquared(errorSquared)
  {
  }

  /** Constructor, copy from a TofEvent object but add weights
   * @param rhs: TofEvent to copy into this.
   * @param weight: weight of this neutron event.
   * @param errorSquared: the square of the error on the event
  */
  WeightedEvent::WeightedEvent(const TofEvent& rhs, float weight, float errorSquared)
  : TofEvent(rhs.m_tof, rhs.m_pulsetime), m_weight(weight), m_errorSquared(errorSquared)
  {
  }

  /** Constructor, copy from another WeightedEvent object
   * @param rhs: source WeightedEvent
   */
  WeightedEvent::WeightedEvent(const WeightedEvent& rhs)
  : TofEvent(rhs.m_tof, rhs.m_pulsetime), m_weight(rhs.m_weight), m_errorSquared(rhs.m_errorSquared)
  {
  }

  /** Constructor, copy from another WeightedEvent object
   * @param rhs: source TofEvent
   */
  WeightedEvent::WeightedEvent(const TofEvent& rhs)
  : TofEvent(rhs.m_tof, rhs.m_pulsetime), m_weight(1.0), m_errorSquared(1.0)
  {
  }

  /// Empty constructor
  WeightedEvent::WeightedEvent()
  : TofEvent(), m_weight(1.0), m_errorSquared(1.0)
  {
  }

  /// Destructor
  WeightedEvent::~WeightedEvent()
  {
  }


  /// Copy from another WeightedEvent object
  WeightedEvent& WeightedEvent::operator=(const WeightedEvent & rhs)
  {
    this->m_tof = rhs.m_tof;
    this->m_pulsetime = rhs.m_pulsetime;
    this->m_weight = rhs.m_weight;
    this->m_errorSquared = rhs.m_errorSquared;
    return *this;
  }

  /** Comparison operator.
   * @param rhs event to which we are comparing.
   * @return true if all elements of this event are identical
   *  */
  bool WeightedEvent::operator==(const WeightedEvent & rhs) const
  {
    return  (this->m_tof == rhs.m_tof) &&
            (this->m_pulsetime == rhs.m_pulsetime) &&
            (this->m_weight == rhs.m_weight) &&
            (this->m_errorSquared == rhs.m_errorSquared);
  }


  /// Return the weight of the neutron, as a double (it is saved as a float).
  double WeightedEvent::weight() const
  {
    return m_weight;
  }

  /** @return the error of the neutron, as a double (it is saved as a float).
   * Note: this returns the actual error; the value is saved
   * internally as the SQUARED error, so this function calculates sqrt().
   * For more speed, use errorSquared().
   *
   */
  double WeightedEvent::error() const
  {
    return std::sqrt( double(m_errorSquared) );
  }

  /** @return the square of the error for this event.
   * This is how the error is saved internally, so this is faster than error()
   */
  double WeightedEvent::errorSquared() const
  {
    return m_errorSquared;
  }


  /** Output a string representation of the event to a stream
   * @param os Stream
   * @param event WeightedEvent to output to the stream
   */
  ostream& operator<<(ostream &os, const WeightedEvent &event)
  {
    os << event.m_tof << "," << event.m_pulsetime.to_simple_string() << " (W" << event.m_weight << " +- " << event.error() << ")";
    return os;
  }



  //==========================================================================
  /// --------------------- TofEvent Comparators ----------------------------------
  //==========================================================================
  /** Compare two events' TOF, return true if e1 should be before e2.
   * @param e1 first event
   * @param e2 second event
   *  */
  bool compareEventTof(const TofEvent & e1, const TofEvent& e2)
  {
    return (e1.tof() < e2.tof());
  }

  /** Compare two events' FRAME id, return true if e1 should be before e2.
  * @param e1 first event
  * @param e2 second event
  *  */
  bool compareEventPulseTime(const TofEvent& e1, const TofEvent& e2)
  {
    return (e1.pulseTime() < e2.pulseTime());
  }

//  /** Comparison operator by TOF for TofEvents) */
//  bool operator<(const TofEvent & e1, const TofEvent& e2)
//  {
//    return (e1.tof() < e2.tof());
//  }
//
//  /** Comparison operator by TOF for TofEvents) */
//  bool operator>(const TofEvent & e1, const TofEvent& e2)
//  {
//    return (e1.tof() > e2.tof());
//  }
//
//  /** Comparison operator by TOF for TofEvents) */
//  bool operator<(const WeightedEvent & e1, const WeightedEvent& e2)
//  {
//    return (e1.tof() < e2.tof());
//  }





  //==========================================================================
  /** Unary function for searching the event list.
   * Returns true if the event's TOF is >= a value
   * @param event the event being checked.
   */
  class tofGreaterOrEqual: std::unary_function<TofEvent, double>
  {
    /// Comparison variable
    double m_value;
  public:
    /// Constructor: save the value
    tofGreaterOrEqual(double value): m_value(value)
    {  }
    /// () operator: return true if event.tof >= value
    bool operator()(TofEvent event)
    {
        return event.m_tof >= m_value;
    }
  };

  //==========================================================================
  /** Unary function for searching the event list.
   * Returns true if the event's TOF is > a value
   * @param event the event being checked.
   */
  class tofGreater: std::unary_function<TofEvent, double>
  {
    /// Comparison variable
    double m_value;
  public:
    /// Constructor: save the value
    tofGreater(double value): m_value(value)
    {  }
    /// () operator: return true if event.tof > value
    bool operator()(TofEvent event)
    {
        return event.m_tof > m_value;
    }
  };










  //==========================================================================
  // ---------------------- EventList stuff ----------------------------------
  //==========================================================================

  // --- Constructors -------------------------------------------------------------------

  /// Constructor (empty)
  EventList::EventList() :
    has_weights(false), order(UNSORTED), detectorIDs()
  {
  }

  /** Constructor copying from an existing event list
   * @param rhs EventList object to copy*/
  EventList::EventList(const EventList& rhs)
  {
    //Call the copy operator to do the job,
    this->operator=(rhs);
  }

  /** Constructor, taking a vector of events.
   * @param events Vector of TofEvent's */
  EventList::EventList(const std::vector<TofEvent> &events)
  {
    this->events.assign(events.begin(), events.end());
    this->has_weights = false;
    this->order = UNSORTED;
  }

  /// Destructor
  EventList::~EventList()
  {
    // Note: These two lines do not seem to have an effect on releasing memory
    //  at least on Linux. (Memory usage seems to increase event after deleting EventWorkspaces.
    //  Therefore, for performance, they are kept commented:

    //this->events.clear();
    //std::vector<TofEvent>().swap(events); //Trick to release the vector memory.
  }

  // --------------------------------------------------------------------------
  // --- Operators -------------------------------------------------------------------

  /** Copy into this event list from another
   * @param rhs We will copy all the events from that into this object.
   * @return reference to this
   * */
  EventList& EventList::operator=(const EventList& rhs)
  {
    //Copy all data from the rhs.
    this->events.assign(rhs.events.begin(), rhs.events.end());
    this->weightedEvents.assign(rhs.weightedEvents.begin(), rhs.weightedEvents.end());
    this->has_weights = rhs.has_weights;
    this->refX = rhs.refX;
    this->order = rhs.order;
    //Copy the detector ID set
    this->detectorIDs = rhs.detectorIDs;
    return *this;
  }

  // --------------------------------------------------------------------------
  /** Append an event to the histogram.
   * @param event TofEvent to add at the end of the list.
   * @return reference to this
   * */
  EventList& EventList::operator+=(const TofEvent &event)
  {
    if (has_weights)
      this->weightedEvents.push_back(WeightedEvent(event));
    else
      this->events.push_back(event);

    this->order = UNSORTED;    
    return *this;
  }

  // --------------------------------------------------------------------------
  /** Append a list of events to the histogram.
   * @param more_events A vector of events to append.
   * @return reference to this
   * */
  EventList& EventList::operator+=(const std::vector<TofEvent> & more_events)
  {
    if (has_weights)
    {
      //Add default weights to all the un-weighted incoming events from the list.
      // and append to the list
      std::vector<TofEvent>::const_iterator it;
      for(it = more_events.begin(); it != more_events.end(); it++)
        this->weightedEvents.push_back( WeightedEvent(*it) );
    }
    else
    {
      //Simply push the events
      this->events.insert(this->events.end(), more_events.begin(), more_events.end());
    }
    this->order = UNSORTED;    
    return *this;
  }


  // --------------------------------------------------------------------------
  /** Append a WeightedEvent to the histogram.
   * Note: The whole list will switch to weights (a possibly lengthy operation)
   *  if it did not have weights before.
   *
   * @param event WeightedEvent to add at the end of the list.
   * @return reference to this
   * */
  EventList& EventList::operator+=(const WeightedEvent &event)
  {
    if (!has_weights)
      this->switchToWeightedEvents();
    this->weightedEvents.push_back(event);
    this->order = UNSORTED;
    return *this;
  }

  // --------------------------------------------------------------------------
  /** Append a list of events to the histogram.
   * Note: The whole list will switch to weights (a possibly lengthy operation)
   *  if it did not have weights before.
   *
   * @param more_events A vector of events to append.
   * @return reference to this
   * */
  EventList& EventList::operator+=(const std::vector<WeightedEvent> & more_events)
  {
    if (!has_weights)
      this->switchToWeightedEvents();
    //Simply push the events
    this->weightedEvents.insert(weightedEvents.end(), more_events.begin(), more_events.end());
    this->order = UNSORTED;
    return *this;
  }



  // --------------------------------------------------------------------------
  /** Append another EventList to this event list.
   * The event lists are concatenated, and a union of the sets of detector ID's is done.
   * @param more_events Another EventList.
   * @return reference to this
   * */
  EventList& EventList::operator+=(const EventList& more_events)
  {
    if (more_events.hasWeights())
    {
      //We're adding something with weights

      //Make sure that THIS has weights too.
      if (!has_weights)
        this->switchToWeightedEvents();

      //At this point, both have weights. Great!
      const vector<WeightedEvent> & rel = more_events.getWeightedEvents();
      this->weightedEvents.insert(this->weightedEvents.end(), rel.begin(), rel.end());
    }

    if (has_weights && !more_events.hasWeights())
    {
      //THIS has weights, but we're adding something that doesn't.
      const vector<TofEvent> & rel = more_events.getEvents();
      this->operator+=(rel);
    }

    else if (!has_weights && !more_events.hasWeights())
    {
      //Neither has weights. Keep the unweighted event lists.
      const vector<TofEvent> & rel = more_events.getEvents();
      this->events.insert(this->events.end(), rel.begin(), rel.end());
    }

    //No guaranteed order
    this->order = UNSORTED;
    //Do a union between the detector IDs of both lists
    std::set<int>::const_iterator it;
    for (it = more_events.detectorIDs.begin(); it != more_events.detectorIDs.end(); it++ )
      this->detectorIDs.insert( *it );

    return *this;
  }



  // --------------------------------------------------------------------------
  /** SUBTRACT another EventList from this event list.
   * The event lists are concatenated, but the weights of the incoming
   *    list are multiplied by -1.0.
   *
   * @param more_events Another EventList.
   * @return reference to this
   * */
  EventList& EventList::operator-=(const EventList& more_events)
  {
    if (more_events.hasWeights())
    {
      //We're adding something with weights
      //Make sure that THIS has weights too.
      this->switchToWeightedEvents();

      //At this point, both have weights. Great!
      const vector<WeightedEvent> & rel = more_events.getWeightedEvents();
      std::vector<WeightedEvent>::const_iterator it;
      for(it = rel.begin(); it != rel.end(); it++)
        this->weightedEvents.push_back( WeightedEvent(it->m_tof, it->m_pulsetime, -it->m_weight, it->m_errorSquared) );
    }

    if (!more_events.hasWeights())
    {
      //We're adding a list without weights.

      //Make sure that THIS has weights too.
      this->switchToWeightedEvents();

      //Loop through the unweighted input, convert them to -1.0 weight, and concatenate.
      const vector<TofEvent> & rel = more_events.getEvents();
      std::vector<TofEvent>::const_iterator it;
      for(it = rel.begin(); it != rel.end(); it++)
        this->weightedEvents.push_back( WeightedEvent(*it, -1.0, 1.0) );
    }

    //No guaranteed order
    this->order = UNSORTED;

    //NOTE: What to do about detector ID's

    return *this;
  }


  // --------------------------------------------------------------------------
  /** Equality operator between EventList's
   * @param rhs :: other EventList to compare
   */
  bool EventList::operator==(const EventList& rhs) const
  {
    if (this->getNumberEvents() != rhs.getNumberEvents())
      return false;
    if (this->has_weights != rhs.has_weights)
      return false;
    if (events != rhs.events)
      return false;
    if (weightedEvents != rhs.weightedEvents)
      return false;
    return true;
  }

  /** Inequality comparator
  * @param rhs :: other EventList to compare
  */
  bool EventList::operator!=(const EventList& rhs) const
  {
    return (!this->operator==(rhs));
  }


  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it faster.
   * @param event TofEvent to add at the end of the list.
   * */
  void EventList::addEventQuickly(const TofEvent &event)
  {
    if (has_weights)
      this->weightedEvents.push_back(WeightedEvent(event));
    else
      this->events.push_back(event);
  }

  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it faster.
   * @param event TofEvent to add at the end of the list.
   * */
  void EventList::addEventQuickly(const WeightedEvent &event)
  {
    this->weightedEvents.push_back(event);
  }





  // --------------------------------------------------------------------------
  /** Add a detector ID to the set of detector IDs
   *
   * @param detID detector ID to insert in set.
   */
  void EventList::addDetectorID(const int detID)
  {
    this->detectorIDs.insert( detID );
  }

  // --------------------------------------------------------------------------
  /** Return true if the given detector ID is in the list for this eventlist */
  bool EventList::hasDetectorID(const int detID) const
  {
    return ( detectorIDs.find(detID) != detectorIDs.end() );
  }

  // --------------------------------------------------------------------------
  /** Get a const reference to the detector IDs set.
   */
  const std::set<int>& EventList::getDetectorIDs() const
  {
    return this->detectorIDs;
  }

  /** Get a mutable reference to the detector IDs set.
   */
  std::set<int>& EventList::getDetectorIDs()
  {
    return this->detectorIDs;
  }

  /** Return true if the event list has weights */
  bool EventList::hasWeights() const
  {
    return has_weights;
  }

  // -----------------------------------------------------------------------------------------------
  /** Switch the EventList to use WeightedEvents instead
   * of TofEvent.
   */
  void EventList::switchToWeightedEvents()
  {
    //Do nothing if already there
    if (has_weights)
      return;
    has_weights = true;
    weightedEvents.clear();

    //Convert and copy all TofEvents to the weightedEvents list.
    std::vector<TofEvent>::const_iterator it;
    for(it = events.begin(); it != events.end(); it++)
      this->weightedEvents.push_back( WeightedEvent(*it) );

    //Get rid of the old events
    events.clear();
  }




  // ==============================================================================================
  // --- Handling the event list -------------------------------------------------------------------
  // ==============================================================================================

  /** Return the const list of TofEvents contained.
   * @return a const reference to the list of non-weighted events
   * */
  const std::vector<TofEvent> & EventList::getEvents() const
  {
    if (has_weights)
      throw std::runtime_error("EventList::getEvents() called for an EventList that has weights. Use getWeightedEvents().");
    return this->events;
  }

  /** Return the list of TofEvents contained.
   * @return a reference to the list of non-weighted events
   * */
  std::vector<TofEvent>& EventList::getEvents()
  {
    if (has_weights)
      throw std::runtime_error("EventList::getEvents() called for an EventList that has weights. Use getWeightedEvents().");
    return this->events;
  }

  /** Return the list of WeightedEvent contained.
   * @return a reference to the list of weighted events
   * */
  std::vector<WeightedEvent>& EventList::getWeightedEvents()
  {
    if (!has_weights)
      throw std::runtime_error("EventList::getWeightedEvents() called for an EventList that does not have weights. Use getEvents().");
    return this->weightedEvents;
  }

  /** Return the list of WeightedEvent contained.
   * @return a const reference to the list of weighted events
   * */
  const std::vector<WeightedEvent>& EventList::getWeightedEvents() const
  {
    if (!has_weights)
      throw std::runtime_error("EventList::getWeightedEvents() called for an EventList that does not have weights. Use getEvents().");
    return this->weightedEvents;
  }

  /** Clear the list of events and any
   * associated detector ID's.
   * */
  void EventList::clear()
  {
    this->events.clear();
    this->weightedEvents.clear();
    this->detectorIDs.clear();
  }



  // ==============================================================================================
  // --- Sorting functions -----------------------------------------------------
  // ==============================================================================================

  // --------------------------------------------------------------------------
  /** Sort events by TOF or Frame
   * @param order Order by which to sort.
   * */
  void EventList::sort(const EventSortType order) const
  {
    if (order == UNSORTED)
    {
      return; // don't bother doing anything. Why did you ask to unsort?
    }
    else if (order == TOF_SORT)
    {
      this->sortTof();
    }
    else if (order == PULSETIME_SORT)
    {
      this->sortPulseTime();
    }
    else
    {
      throw runtime_error("Invalid sort type in EventList::sort(EventSortType)");
    }
  }

  // --------------------------------------------------------------------------
  /** Sort events by TOF */
  void EventList::sortTof() const
  {
    if (this->order == TOF_SORT)
    {
      return; // nothing to do
    }
    //Perform sort.
    if (has_weights)
      std::sort(weightedEvents.begin(), weightedEvents.end(), compareEventTof);
    else
      std::sort(events.begin(), events.end(), compareEventTof);

    //Save the order to avoid unnecessary re-sorting.
    this->order = TOF_SORT;
  }



//  // MergeSort from: http://en.literateprograms.org/Merge_sort_%28C_Plus_Plus%29#chunk%20def:merge
//  template<typename IT, typename VT> void insert(IT begin, IT end, const VT &v)
//  {
//    while(begin+1!=end && *(begin+1)<v) {
//      std::swap(*begin, *(begin+1));
//      ++begin;
//    }
//    *begin=v;
//  }
//
//  template<typename IT> void merge(IT begin, IT begin_right, IT end)
//  {
//    for(;begin<begin_right; ++begin) {
//      if(*begin>*begin_right) {
//        typename std::iterator_traits<IT>::value_type v(*begin);
//        *begin=*begin_right;
//        insert(begin_right, end, v);
//      }
//    }
//  }
//
//  template<typename IT> void mergesort(IT begin, IT end)
//  {
//    size_t size(end-begin);
//    //std::cout << "mergesort called on " << size << "\n";
//    if(size<2) return;
//
//    IT begin_right=begin+size/2;
//
//    mergesort(begin, begin_right);
//    mergesort(begin_right, end);
//    merge(begin, begin_right, end);
//  }


  //----------------------------------------------------------------------------------------------------
  /** Merge two sorted lists into one sorted vector.
   *
   * @tparam T :: the type in the vector.
   * @param begin1 :: iterator at the start of the first list.
   * @param end1 :: iterator at the end of the first list.
   * @param begin2 :: iterator at the start of the second list.
   * @param end2 :: iterator at the end of the second list.
   * @param result_vector :: a vector (by reference) that will be filled with the result.
   * */
  template<typename T>
  void merge(typename std::vector<T>::iterator begin1, typename std::vector<T>::iterator end1,
             typename std::vector<T>::iterator begin2, typename std::vector<T>::iterator end2,
             typename std::vector<T> & result_vector)
  {
    typename std::vector<T>::iterator it1=begin1;
    typename std::vector<T>::iterator it2=begin2;
    while (!((it1 == end1) && (it2 == end2)))
    {
      if (it1 == end1)
      {
        // Only it2 makes sense
        result_vector.push_back(*it2);
        it2++;
      }
      else if (it2 == end2)
      {
        // Only it1 makes sense
        result_vector.push_back(*it1);
        it1++;
      }
      else
      {
        // Both iterators are valid. Which is smaller?
        if (*it1 < *it2)
        {
          result_vector.push_back(*it1);
          it1++;
        }
        else
        {
          result_vector.push_back(*it2);
          it2++;
        }
      }
    }
  }



  //----------------------------------------------------------------------------------------------------
  /** Perform a parallelized sort on a provided vector, using 2 threads.
   * NOTE: Will temporarily use twice the memory used by the incoming vector.
   *
   * @param vec :: a vector, by refe/rence, that will be sorted-in place.
   */
  template<typename T>
  void parallel_sort2(typename std::vector<T> & vec)
  {
    size_t size = vec.size();

    typename std::vector<T>::iterator begin = vec.begin();
    typename std::vector<T>::iterator middle = begin + size/2;
    typename std::vector<T>::iterator end = vec.end();

    PRAGMA_OMP(parallel sections)
    {
      PRAGMA_OMP(section)
      {
        std::sort(begin, middle);
        //std::cout << " ----------- Part 1 --------------\n"; for (typename std::vector<T>::iterator it = begin; it != middle; it++) std::cout << *it << "\n";
      }
      PRAGMA_OMP(section)
      {
        std::sort(middle, end);
        // std::cout << " ----------- Part 2 --------------\n";for (typename std::vector<T>::iterator it = middle; it != end; it++) std::cout << *it << "\n";
      }
    }

    // Now merge back
    typename std::vector<T> temp;
    merge(begin, middle, middle, end, temp);

    //std::cout << " ----------- Part 1+2 --------------\n"; for (typename std::vector<T>::iterator it = temp.begin(); it != temp.end(); it++) std::cout << *it << "\n";
    // Swap storage with the temp vector
    vec.swap(temp);
    // Which we can now clear
    temp.clear();
  }


  //----------------------------------------------------------------------------------------------------
  /** Perform a parallelized sort on a provided vector, using 4 threads.
   * NOTE: Will temporarily use twice the memory used by the incoming vector.
   *
   * @param vec :: a vector, by refe/rence, that will be sorted-in place.
   */
  template<typename T>
  void parallel_sort4(std::vector<T> & vec)
  {
    //int num_cores = PARALLEL_NUMBER_OF_THREADS;
    size_t size = vec.size();

    typename std::vector<T>::iterator begin = vec.begin();
    typename std::vector<T>::iterator middle1 = begin + size/4;
    typename std::vector<T>::iterator middle2 = begin + size/2;
    typename std::vector<T>::iterator middle3 = begin + 3*size/4;
    typename std::vector<T>::iterator end = vec.end();

    PRAGMA_OMP(parallel sections)
    {
      PRAGMA_OMP(section)
      {
        std::sort(begin, middle1);
      }
      PRAGMA_OMP(section)
      {
        std::sort(middle1, middle2);
      }
      PRAGMA_OMP(section)
      {
        std::sort(middle2, middle3);
      }
      PRAGMA_OMP(section)
      {
        std::sort(middle3, end);
      }
    }

    // Now merge back
    typename std::vector<T> temp1, temp2;
    //PRAGMA_OMP(parallel sections)
    {
      //PRAGMA_OMP(section)
      {
        merge(begin, middle1, middle1, middle2, temp1);
      }
      //PRAGMA_OMP(section)
      {
        merge(middle2, middle3, middle3, end, temp2);
      }
    }

    // We can clear the incoming vector to free up memory now,
    //  because it is copied already in temp1, temp2
    vec.clear();

    // Final merge
    typename std::vector<T> temp;
    merge(temp1.begin(), temp1.end(), temp2.begin(), temp2.end(), temp);

    // Swap storage with the temp vector
    vec.swap(temp);
    // Which we can now clear
    temp.clear();
  }


  // --------------------------------------------------------------------------
  /** Sort events by TOF, using two threads.
   *
   * Performance for 5e7 events:
   *  - 40.5 secs with sortTof() (one thread)
   *  - 21.1 secs with sortTof2() (two threads)
   *  - 18.2 secs with sortTof4() (four threads)
   * Performance gain tends to go up with longer event lists.
   * */
  void EventList::sortTof2() const
  {
    if (this->order == TOF_SORT)
    {
      return; // nothing to do
    }
    if (has_weights)
      parallel_sort2(weightedEvents);
    else
      parallel_sort2(events);

    //Save the order to avoid unnecessary re-sorting.
    this->order = TOF_SORT;
  }

  // --------------------------------------------------------------------------
  /** Sort events by TOF, using four threads.
   *
   * Performance for 5e7 events:
   *  - 40.5 secs with sortTof() (one thread)
   *  - 21.1 secs with sortTof2() (two threads)
   *  - 18.2 secs with sortTof4() (four threads)
   * Performance gain tends to go up with longer event lists.
   * */
  void EventList::sortTof4() const
  {
    if (this->order == TOF_SORT)
    {
      return; // nothing to do
    }

    if (has_weights)
      parallel_sort4(weightedEvents);
    else
      parallel_sort4(events);

    //Save the order to avoid unnecessary re-sorting.
    this->order = TOF_SORT;
  }



  // --------------------------------------------------------------------------
  /** Sort events by Frame */
  void EventList::sortPulseTime() const
  {
    if (this->order == PULSETIME_SORT)
    {
      return; // nothing to do
    }
    //Perform sort.
    if (has_weights)
      std::sort(weightedEvents.begin(), weightedEvents.end(), compareEventPulseTime);
    else
      std::sort(events.begin(), events.end(), compareEventPulseTime);
    //Save the order to avoid unnecessary re-sorting.
    this->order = PULSETIME_SORT;
  }

  // --------------------------------------------------------------------------
  /** Return true if the event list is sorted by TOF */
  bool EventList::isSortedByTof() const
  {
    return (this->order == TOF_SORT);
  }

  // --------------------------------------------------------------------------
  /** Reverse the histogram boundaries and the associated events if they are sorted. */
  void EventList::reverse()
  {
    // reverse the histogram bin parameters
    MantidVec x = this->refX.access();
    std::reverse(x.begin(), x.end());
    this->refX.access() = x;

    // flip the events if they are tof sorted
    if (this->isSortedByTof())
    {
      if (has_weights)
        std::reverse(this->weightedEvents.begin(), this->weightedEvents.end());
      else
        std::reverse(this->events.begin(), this->events.end());
      //And we are still sorted! :)
    }
    else
      this->order = UNSORTED;
  }

  // --------------------------------------------------------------------------
  /** Return the number of events in the list.
   * NOTE: If the events have weights, this returns the NUMBER of WeightedEvent's in the
   * list, and NOT the sum of their weights (which may be two different numbers).
   *
   * @return the number of events in the list.
   *  */
  size_t EventList::getNumberEvents() const
  {
    if (has_weights)
      return this->weightedEvents.size();
    else
      return this->events.size();
  }


  // --------------------------------------------------------------------------
  /** Return the memory used by the EventList. */
  size_t EventList::getMemorySize() const
  {
    if (has_weights)
      return this->weightedEvents.size() * sizeof(WeightedEvent) + sizeof(EventList);
    else
      return this->events.size() * sizeof(TofEvent) + sizeof(EventList);
  }


  // --------------------------------------------------------------------------
  /** Return the size of the histogram data.
   * @return the size of the histogram representation of the data (size of Y) **/
  size_t EventList::histogram_size() const
  {
    size_t x_size = refX->size();
    if (x_size > 1)
      return x_size - 1;
    else
      return 0;
  }



  // ==============================================================================================
  // --- Setting the Histrogram X axis, without recalculating the histogram -----------------------
  // ==============================================================================================

  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   */
  void EventList::setX(const MantidVecPtr::ptr_type& X)
  {
    this->refX = X;
  }

  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   */
  void EventList::setX(const MantidVecPtr& X)
  {
    this->refX = X;
  }

  /** Set the x-component for the histogram view. This will NOT cause the histogram to be calculated.
   * @param X :: The vector of doubles to set as the histogram limits.
   */
  void EventList::setX(const MantidVec& X)
  {
    this->refX.access()=X;
  }


  // ==============================================================================================
  // --- Return Data Vectors --------------------------------------------------
  // ==============================================================================================

  // Note: these will only be called from a const class; e.g
  // const EventList el;
  // el.dataX(); <<<<< this works
  // EventList el2;
  // el2.dataX(); <<<<< this throws an error.

  /** Returns the x data.
   * @return a const reference to the X (bin) vector.
   *  */
  const MantidVec& EventList::dataX() const
  {
    return *(this->refX);
  }

  /** Returns a reference to the X data
   * @return a cow_ptr to the X (bin) vector.
   * */
  Kernel::cow_ptr<MantidVec> EventList::getRefX() const
  {
    return refX;
  }


  /** Calculates and returns a pointer to the Y histogrammed data.
   * Remember to delete your pointer after use!
   *
   * @return a pointer to a MantidVec
   */
  MantidVec * EventList::dataY() const
  {
    MantidVec * Y = new MantidVec();
    generateCountsHistogram(*this->refX, *Y);
    return Y;
  }


  /** Calculates and returns a pointer to the E histogrammed data.
   * Remember to delete your pointer after use!
   *
   * @return a pointer to a MantidVec
   */
  MantidVec * EventList::dataE() const
  {
    if (has_weights)
    {
      MantidVec Y;
      MantidVec * E = new MantidVec();
      generateHistogramsForWeights(*this->refX, Y, *E);
      //Y is unused.
      return E;
    }
    else
    {
      MantidVec Y;
      generateCountsHistogram(*this->refX, Y);
      MantidVec * E = new MantidVec();
      generateErrorsHistogram(Y, *E);
      return E;
    }
  }





  // --------------------------------------------------------------------------
  /** Utility function:
   * Returns the iterator into events of the first TofEvent with
   * tof() > seek_tof
   * Will return this->events.end() if nothing is found!
   *
   * @param seek_tof tof to find (typically the first bin X[0])
   * @return iterator where the first event matching it is.
   */
  std::vector<TofEvent>::iterator EventList::findFirstEvent(const double seek_tof) const
  {
    std::vector<TofEvent>::iterator itev;
    itev = this->events.begin();

    //if tof < X[0], that means that you need to skip some events
    while ((itev != this->events.end()) && (itev->tof() < seek_tof))
      itev++;
    // Better fix would be to use a binary search instead of the linear one used here.

    return itev;
  }

  // --------------------------------------------------------------------------
  /** Utility function:
   * Returns the iterator into events of the first TofEvent with
   * tof() > seek_tof
   * Will return this->events.end() if nothing is found!
   *
   * @param seek_tof tof to find (typically the first bin X[0])
   * @return iterator where the first event matching it is.
   */
  std::vector<WeightedEvent>::iterator EventList::findFirstWeightedEvent(const double seek_tof) const
  {
    std::vector<WeightedEvent>::iterator itev;
    itev = this->weightedEvents.begin();

    //if tof < X[0], that means that you need to skip some events
    while ((itev != this->weightedEvents.end()) && (itev->tof() < seek_tof))
      itev++;
    // Better fix would be to use a binary search instead of the linear one used here.
    return itev;
  }

  // --------------------------------------------------------------------------
  /** Generates both the Y and E (error) histograms
   * for an EventList with or without WeightedEvents.
   *
   * @param X: x-bins supplied
   * @param Y: counts returned
   * @param E: errors returned
   */
  void EventList::generateHistogram(const MantidVec& X, MantidVec& Y, MantidVec& E) const
  {
    if (has_weights)
    {
      // Make both, with weights
      this->generateHistogramsForWeights(X, Y, E);
    }
    else
    {
      // Make the single ones
      this->generateCountsHistogram(X, Y);
      this->generateErrorsHistogram(Y, E);
    }
  }


  // --------------------------------------------------------------------------
  /** Generates both the Y and E (error) histograms
   * for an EventList with WeightedEvents.
   *
   * @param X: x-bins supplied
   * @param Y: counts returned
   * @param E: errors returned
   * @throw runtime_error if the EventList does not have weighted events
   */
  void EventList::generateHistogramsForWeights(const MantidVec& X, MantidVec& Y, MantidVec& E) const
  {
    if (!has_weights)
      throw std::runtime_error("EventList::generateHistogramsForWeights() called for a non-weighted EventList. Try generateCountsHistogram() instead.");

    //For slight speed=up.
    size_t x_size = X.size();

    if (x_size <= 1)
    {
      //X was not set. Return an empty array.
      Y.resize(0, 0);
      return;
    }

    //Sort the events by tof
    this->sortTof();
    //Clear the Y data, assign all to 0.
    Y.resize(x_size-1, 0.0);
    //Clear the Error data, assign all to 0.
    // Note: Errors will be squared until the last step.
    E.resize(x_size-1, 0.0);

    //---------------------- Histogram without weights ---------------------------------

    //Do we even have any events to do?
    if (this->weightedEvents.size() > 0)
    {
      //Iterate through all events (sorted by tof)
      std::vector<WeightedEvent>::iterator itev = this->findFirstWeightedEvent(X[0]);
      // The above can still take you to end() if no events above X[0], so check again.
      if (itev == this->weightedEvents.end()) return;

      //Find the first bin
      size_t bin=0;
      //The tof is greater the first bin boundary, so we need to find the first bin
      double tof = itev->tof();
      while (bin < x_size-1)
      {
        //Within range?
        if ((tof >= X[bin]) && (tof < X[bin+1]))
        {
          //Add up the weight (convert to double before adding, to preserve precision)
          Y[bin] += double(itev->m_weight);
          E[bin] += double(itev->m_errorSquared); //square of error
          break;
        }
        ++bin;
      }
      //Go to the next event, we've already binned this first one.
      ++itev;

      //Keep going through all the events
      while ((itev != this->weightedEvents.end()) && (bin < x_size-1))
      {
        tof = itev->tof();
        while (bin < x_size-1)
        {
          //Within range?
          if ((tof >= X[bin]) && (tof < X[bin+1]))
          {
            //Add up the weight (convert to double before adding, to preserve precision)
            Y[bin] += double(itev->m_weight);
            E[bin] += double(itev->m_errorSquared); //square of error
            break;
          }
          ++bin;
        }
        ++itev;
      }
    } // end if (there are any events to histogram)

    // Now do the sqrt of all errors
    typedef double (*uf)(double);
    uf dblSqrt = std::sqrt;
    std::transform(E.begin(), E.end(), E.begin(), dblSqrt);


  }



  // --------------------------------------------------------------------------
  /** Fill a histogram given specified histogram bounds. Does not modify
   * the eventlist (const method).
   * @param X The x bins
   * @param Y The generated counts histogram
   */
  void EventList::generateCountsHistogram(const MantidVec& X, MantidVec& Y) const
  {
    //Call the weights function, if needed
    if (has_weights)
    {
      MantidVec E;
      this->generateHistogramsForWeights(X, Y, E);
    }

    //For slight speed=up.
    size_t x_size = X.size();

    if (x_size <= 1)
    {
      //X was not set. Return an empty array.
      Y.resize(0, 0);
      return;
    }

    //Sort the events by tof
    this->sortTof();
    //Clear the Y data, assign all to 0.
    Y.resize(x_size-1, 0);

    //---------------------- Histogram without weights ---------------------------------

    //Do we even have any events to do?
    if (this->events.size() > 0)
    {
      //Iterate through all events (sorted by tof)
      std::vector<TofEvent>::iterator itev = this->findFirstEvent(X[0]);
      // The above can still take you to end() if no events above X[0], so check again.
      if (itev == this->events.end()) return;

      //Find the first bin
      size_t bin=0;

      //The tof is greater the first bin boundary, so we need to find the first bin
      double tof = itev->tof();
      while (bin < x_size-1)
      {
        //Within range?
        if ((tof >= X[bin]) && (tof < X[bin+1]))
        {
          Y[bin]++;
          break;
        }
        ++bin;
      }
      //Go to the next event, we've already binned this first one.
      ++itev;

      //Keep going through all the events
      while ((itev != this->events.end()) && (bin < x_size-1))
      {
        tof = itev->tof();
        while (bin < x_size-1)
        {
          //Within range?
          if ((tof >= X[bin]) && (tof < X[bin+1]))
          {
            Y[bin]++;
            break;
          }
          ++bin;
        }
        ++itev;
      }
    } // end if (there are any events to histogram)


  }

  // --------------------------------------------------------------------------
  /**
   * Generate the Error histogram for the provided counts histogram.
   * It simply returns the sqrt of the number of counts for each bin.
   *
   * @param Y The counts histogram
   * @param E The generated error histogram
   */
  void EventList::generateErrorsHistogram(const MantidVec& Y, MantidVec& E) const
  {
    if (has_weights)
      throw std::runtime_error("EventList::generateErrorsHistogram() called for a weighted EventList. Try generateHistogramsForWeights() instead.");

    // Fill the vector for the errors, containing sqrt(count)
    E.resize(Y.size(), 0);

    // windows can get confused about std::sqrt
    typedef double (*uf)(double);
    uf dblSqrt = std::sqrt;
    std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);

  }



  // --------------------------------------------------------------------------
  /** Integrate the events between a range of X values, or all events.
   *
   * @param minX minimum X bin to use in integrating.
   * @param maxX maximum X bin to use in integrating.
   * @param entireRange set to true to use the entire range. minX and maxX are then ignored!
   * @return the integrated number of events.
   */
  double EventList::integrate(const double minX, const double maxX, const bool entireRange) const
  {
    if (!entireRange)
    {
      //The event list must be sorted by TOF!
      this->sortTof();
    }

    if (has_weights)
    {
      //Nothing in the list?
      if (weightedEvents.size() == 0)
        return 0.0;

      // Iterators for limits - whole range by default
      std::vector<WeightedEvent>::iterator lowit, highit;
      lowit=weightedEvents.begin();
      highit=weightedEvents.end();

      //But maybe we don't want the entire range?
      if (!entireRange)
      {
        //If a silly range was given, return 0.
        if (maxX < minX)
          return 0.0;

        // If the first element is lower that the xmin then search for new lowit
        if (lowit->tof() < minX)
          lowit = std::lower_bound(weightedEvents.begin(),weightedEvents.end(),minX);
        // If the last element is higher that the xmax then search for new lowit
        if ((highit-1)->tof() > maxX)
        {
          highit = std::upper_bound(lowit,weightedEvents.end(), TofEvent(maxX, 0), compareEventTof);
        }
      }

      // Sum up all the weights
      double sum(0.0);
      std::vector<WeightedEvent>::iterator it;
      for (it = lowit; it != highit; it++)
        sum += it->weight();

      //Give it
      return sum;
    }
    else
    {
      //Nothing in the list?
      if (events.size() == 0)
        return 0.0;

      // Iterators for limits - whole range by default
      std::vector<TofEvent>::iterator lowit, highit;
      lowit=events.begin();
      highit=events.end();

      //But maybe we don't want the entire range?
      if (!entireRange)
      {
        //If a silly range was given, return 0.
        if (maxX < minX)
          return 0.0;

        // If the first element is lower that the xmin then search for new lowit
        if (lowit->tof() < minX)
          lowit = std::lower_bound(events.begin(),events.end(),minX);
        // If the last element is higher that the xmax then search for new lowit
        if ((highit-1)->tof() > maxX)
        {
          highit = std::upper_bound(lowit,events.end(), TofEvent(maxX, 0), compareEventTof);
        }
      }

      // The distance between the two iterators (they are NOT inclusive) = the number of events
      double sum = std::distance(lowit, highit);

      //Give it
      return sum;
    }

  }




  // ==============================================================================================
  // ----------- Conversion Functions (changing tof values) ---------------------------------------
  // ==============================================================================================

  // --------------------------------------------------------------------------
  /**
   * Convert the time of flight by tof'=tof*factor+offset
   * @param factor The value to scale the time-of-flight by
   * @param offset The value to shift the time-of-flight by
   */
  void EventList::convertTof(const double factor, const double offset)
  {
    if (this->getNumberEvents() <= 0)  return;

    if (factor == 1.)
    {
      this->addTof(offset);
      return;
    }
    if (offset == 0.)
    {
      this->scaleTof(factor);
      return;
    }

    //Convert the list
    this->convertTof_onList(factor, offset);

    // fix the histogram parameter
    MantidVec x = this->refX.access();
    for (MantidVec::iterator iter = x.begin(); iter != x.end(); ++iter)
      *iter = (*iter) * factor + offset;
    this->refX.access() = x;

    if (factor < 0.)
      this->reverse();
  }


  // --------------------------------------------------------------------------
  /** Private function to do the conversion factor work
   * on either the TofEvent list or the WeightedEvent list.
   * Does NOT reverse the event list if the factor < 0
   *
   * @param factor multiply by this
   * @param offset add this
   */
  void EventList::convertTof_onList(const double factor, const double offset)
  {
    if (has_weights)
    {
      // iterate through all events
      for (std::vector<WeightedEvent>::iterator iter = this->weightedEvents.begin();
           iter != this->weightedEvents.end(); iter++)
        iter->m_tof = iter->m_tof * factor + offset;
    }
    else
    {
      // iterate through all events
      for (std::vector<TofEvent>::iterator iter = this->events.begin();
           iter != this->events.end(); iter++)
        iter->m_tof = iter->m_tof * factor + offset;
    }

  }

  // --------------------------------------------------------------------------
  /**
   * Convert the units in the TofEvent's m_tof field to
   *  some other value, by scaling by a multiplier.
   * @param factor conversion factor (e.g. multiply TOF by this to get d-spacing)
   */
  void EventList::scaleTof(const double factor)
  {
    //Do we even have any events to do?
    if (this->getNumberEvents() <= 0)  return;

    //Do the event list
    convertTof_onList(factor, 0.0);

    //Scale the X vector too
    MantidVec x = this->refX.access();
    std::transform(x.begin(), x.end(), x.begin(),
                   std::bind2nd(std::multiplies<double>(), factor));

    if (factor < 0.)
      this->reverse();

  }

  // --------------------------------------------------------------------------
  /** Add an offset to the TOF of each event in the list.
   *
   * @param offset The value to shift the time-of-flight by
   */
  void EventList::addTof(const double offset)
  {
    if (this->getNumberEvents() <= 0)  return;

    //Do the event list
    convertTof_onList(1.0, offset);

    // fix the histogram vector
    MantidVec x = this->refX.access();
    std::transform(x.begin(), x.end(), x.begin(),
                   std::bind2nd(std::plus<double>(), offset));
    this->refX.access() = x;
  }



  // --------------------------------------------------------------------------
  /**
   * Mask out events that have a tof between tofMin and tofMax (inclusively).
   * Events are removed from the list.
   * @param tofMin lower bound of TOF to filter out
   * @param tofMax upper bound of TOF to filter out
   */
  void EventList::maskTof(const double tofMin, const double tofMax)
  {
    if (tofMax <= tofMin)
      throw std::runtime_error("EventList::maskTof: tofMax must be > tofMin");

    //Start by sorting by tof
    this->sortTof();

    if (has_weights)
    {
      //Find the index of the first tofMin
      std::vector<WeightedEvent>::iterator it_first = std::find_if(this->weightedEvents.begin(), this->weightedEvents.end(), tofGreaterOrEqual(tofMin));
      if (it_first != weightedEvents.end())
      {
        //Something was found
        //Look for the first one > tofMax
        std::vector<WeightedEvent>::iterator it_last = std::find_if(it_first, this->weightedEvents.end(), tofGreater(tofMax));
        //it_last will either be at the end (if not found) or before it.
        //Erase this range from the vector
        weightedEvents.erase(it_first, it_last);
        //Done! Sorting is still valid, no need to redo.
      }
    }
    else
    {
      //Find the index of the first tofMin
      std::vector<TofEvent>::iterator it_first = std::find_if(this->events.begin(), this->events.end(), tofGreaterOrEqual(tofMin));
      if (it_first != events.end())
      {
        //Something was found
        //Look for the first one > tofMax
        std::vector<TofEvent>::iterator it_last = std::find_if(it_first, this->events.end(), tofGreater(tofMax));
        //it_last will either be at the end (if not found) or before it.
        //Erase this range from the vector
        events.erase(it_first, it_last);
        //Done! Sorting is still valid, no need to redo.
      }
    }

  }

  /** Fill a vector with the list of TOFs
   *  @param A reference to the vector to be filled
   */
  void EventList::getTofs(std::vector<double>& tofs) const
  {
    // Set the capacity of the vector to avoid multiple resizes
    tofs.reserve(this->getNumberEvents());

    // iterate through all events
    if (has_weights)
    {
      std::vector<WeightedEvent>::iterator iter;
      for (iter = this->weightedEvents.begin(); iter != this->weightedEvents.end(); iter++)
        tofs.push_back(iter->m_tof);
    }
    else
    {
      std::vector<TofEvent>::iterator iter;
      for (iter = this->events.begin(); iter != this->events.end(); iter++)
        tofs.push_back(iter->m_tof);
    }

  }


  // --------------------------------------------------------------------------
  /**
   * Set a list of TOFs to the current event list. Modify the units if necessary.
   * NOTE: This function does not check if the list sizes are the same.
   *
   * @param T The vector of doubles to set the tofs to.
   */
  void EventList::setTofs(const MantidVec &T)
  {
    if (T.empty())
    {
      return;
    }
    size_t x_size = T.size();
    if (has_weights)
    {
      for (size_t i = 0; i < x_size; ++i)
        weightedEvents[i].m_tof = T[i];
    }
    else
    {
      for (size_t i = 0; i < x_size; ++i)
        events[i].m_tof = T[i];
    }
  }



  // ==============================================================================================
  // ----------- MULTIPLY AND DIVIDE ---------------------------------------
  // ==============================================================================================


  //------------------------------------------------------------------------------------------------
  /** Multiply the weights in this event list by an error-less scalar.
   * Use multiply(value,error) if you wish to multiply by a real variable with an error!
   *
   * The event list switches to WeightedEvent's if needed.
   * Note that if the multiplier is exactly 1.0, the list is NOT switched to WeightedEvents - nothing happens.
   *
   * Given A = the weight; the scalar = a.
   *  - The weight is simply \f$ aA \f$
   *  - The error \f$ \sigma_A \f$ becomes \f$ \sigma_{aA} = a \sigma_{A} \f$
   *
   * @param value: multiply all weights by this amount.
   */
  void EventList::multiply(const double value)
  {
    // Do nothing if multiplying by exactly one
    if (value == 1.0)
      return;

    //Switch to weights if needed.
    this->switchToWeightedEvents();

    //Square of the value's error
    double valSquared = value * value;

    std::vector<WeightedEvent>::iterator itev;
    for (itev = this->weightedEvents.begin(); itev != this->weightedEvents.end(); itev++)
    {
      itev->m_errorSquared = (itev->m_errorSquared * valSquared);
      itev->m_weight *= value;
    }
  }


  //------------------------------------------------------------------------------------------------
  /** Operator to multiply the weights in this EventList by an error-less scalar.
   * Use multiply(value,error) if you wish to multiply by a real variable with an error!
   *
   * The event list switches to WeightedEvent's if needed.
   * Note that if the multiplier is exactly 1.0, the list is NOT switched to WeightedEvents - nothing happens.
   *
   * @param value multiply by this
   * @return reference to this
   */
  EventList& EventList::operator*=(const double value)
  {
    this->multiply(value);
    return *this;
  }


  //------------------------------------------------------------------------------------------------
  /** Multiply the weights in this event list by a scalar variable with an error.
   * If the error is exactly 0.0, multiply(value) is called instead, which
   * uses the different error propagation formula for an error-less scalar.
   *
   * The event list switches to WeightedEvent's if needed.
   * Note that if the multiplier is exactly 1.0 and the error is exactly 0.0, the list is NOT switched to WeightedEvents - nothing happens.
   *
   * Given:
   *  - A is the weight, variance \f$\sigma_A \f$
   *  - B is the scalar multiplier, variance \f$\sigma_B \f$
   *
   * The error propagation formula used is:
   *
   * \f[ \left(\frac{\sigma_f}{f}\right)^2 = \left(\frac{\sigma_A}{A}\right)^2 + \left(\frac{\sigma_B}{B}\right)^2 + 2\frac{\sigma_A\sigma_B}{AB}\rho_{AB} \f]
   *
   * \f$ \rho_{AB} \f$ is the covariance between A and B, which we take to be 0 (uncorrelated variables).
   * Therefore, this reduces to:
   * \f[ \sigma_{AB}^2 = B^2 \sigma_A^2 + A^2 \sigma_B ^ 2  \f]
   *
   * @param value: multiply all weights by this amount.
   * @param error: error on 'value'. Can be 0.
   */
  void EventList::multiply(const double value, const double error)
  {
    if (error==0.0)
    {
      this->multiply(value);
      return;
    }

    //Switch to weights if needed.
    this->switchToWeightedEvents();

    //Square of the value's error
    double errorSquared = error * error;
    double valueSquared = value * value;

    std::vector<WeightedEvent>::iterator itev;
    for (itev = this->weightedEvents.begin(); itev != this->weightedEvents.end(); itev++)
    {
      itev->m_errorSquared = itev->m_errorSquared*valueSquared  +  errorSquared * itev->m_weight*itev->m_weight;
      itev->m_weight *= value;
    }
  }


  //------------------------------------------------------------------------------------------------
  /** Multiply the weights in this event list by a histogram.
   * The event list switches to WeightedEvent's if needed.
   * NOTE: no unit checks are made (or possible to make) to compare the units of X and tof() in the EventList.
   *
   * The formula used for calculating the error on the neutron weight is:
   * \f[ \sigma_{AB}^2 = B^2 \sigma_A^2 + A^2 \sigma_B ^ 2  \f]
   * ... where A is the weight, and B is the scalar multiplier for the histogram bin that A is in,
   *  \f$\sigma_X \f$ is the variance of the given variable:
   *
   * @param X: bins of the multiplying histogram.
   * @param Y: value to multiply the weights.
   * @param E: error on the value to multiply.
   * @throw invalid_argument if the sizes of X, Y, E are not consistent.
   */
  void EventList::multiply(const MantidVec & X, const MantidVec & Y, const MantidVec & E)
  {
    //Validate inputs
    if ((X.size() < 2) || (Y.size() != E.size()) || (X.size() != 1+Y.size()) )
      throw std::invalid_argument("EventList::multiply() was given invalid size or inconsistent histogram arrays.");

    //Switch to weights if needed.
    this->switchToWeightedEvents();

    //Sorting by tof is necessary for the algorithm
    this->sortTof();
    size_t x_size = X.size();

    //Iterate through all events (sorted by tof)
    std::vector<WeightedEvent>::iterator itev = this->findFirstWeightedEvent(X[0]);
    // The above can still take you to end() if no events above X[0], so check again.
    if (itev == this->weightedEvents.end()) return;

    //Find the first bin
    size_t bin=0;

    //Multiplier values
    double value;
    double error;
    double valueSquared;
    double errorSquared;

    //If the tof is greater the first bin boundary, so we need to find the first bin
    double tof = itev->tof();
    while (bin < x_size-1)
    {
      //Within range?
      if ((tof >= X[bin]) && (tof < X[bin+1]))
        break; //Stop increasing bin
      ++bin;
    }

    //New bin! Find what you are multiplying!
    value = Y[bin];
    error = E[bin];
    valueSquared = value*value;
    errorSquared = error*error;

    //Keep going through all the events
    while ((itev != this->weightedEvents.end()) && (bin < x_size-1))
    {
      tof = itev->tof();
      while (bin < x_size-1)
      {
        //Event is Within range?
        if ((tof >= X[bin]) && (tof < X[bin+1]))
        {
          //Process this event. Multilpy and calculate error.
          itev->m_errorSquared = itev->m_errorSquared*valueSquared  +  errorSquared * itev->m_weight*itev->m_weight;
          itev->m_weight *= value;
          break; //out of the bin-searching-while-loop
        }
        ++bin;
        if( bin >= x_size - 1 ) break;

        //New bin! Find what you are multiplying!
        value = Y[bin];
        error = E[bin];
        valueSquared = value*value;
        errorSquared = error*error;
      }
      ++itev;
    }
  }


  //------------------------------------------------------------------------------------------------
  /** Divide the weights in this event list by a histogram.
   * The event list switches to WeightedEvent's if needed.
   * NOTE: no unit checks are made (or possible to make) to compare the units of X and tof() in the EventList.
   * This calls multiply(X,Y,E, divide=true) to do division there.
   *
   * @param X: bins of the multiplying histogram.
   * @param Y: value to multiply the weights.
   * @param E: error on the value to multiply.
   * @throw invalid_argument if the sizes of X, Y, E are not consistent.
   */
  void EventList::divide(const MantidVec & X, const MantidVec & Y, const MantidVec & E)
  {
    //Validate inputs
    if ((X.size() < 2) || (Y.size() != E.size()) || (X.size() != 1+Y.size()) )
      throw std::invalid_argument("EventList::divide() was given invalid size or inconsistent histogram arrays.");

    //Switch to weights if needed.
    this->switchToWeightedEvents();

    //Sorting by tof is necessary for the algorithm
    this->sortTof();
    size_t x_size = X.size();

    //Iterate through all events (sorted by tof)
    std::vector<WeightedEvent>::iterator itev = this->findFirstWeightedEvent(X[0]);
    // The above can still take you to end() if no events above X[0], so check again.
    if (itev == this->weightedEvents.end()) return;

    //Find the first bin
    size_t bin=0;

    //Multiplier values
    double value;
    double error;
    double valError_over_value_squared;

    //If the tof is greater the first bin boundary, so we need to find the first bin
    double tof = itev->tof();
    while (bin < x_size-1)
    {
      //Within range?
      if ((tof >= X[bin]) && (tof < X[bin+1]))
        break; //Stop increasing bin
      ++bin;
    }

    //New bin! Find what you are multiplying!
    value = Y[bin];
    error = E[bin];

    // --- Division case ---
    if (value == 0)
    {
      value = std::numeric_limits<float>::quiet_NaN(); //Avoid divide by zero
      error = 0;
      valError_over_value_squared = 0;
    }
    else
      valError_over_value_squared = error * error / (value * value);



    //Keep going through all the events
    while ((itev != this->weightedEvents.end()) && (bin < x_size-1))
    {
      tof = itev->tof();
      while (bin < x_size-1)
      {
        //Event is Within range?
        if ((tof >= X[bin]) && (tof < X[bin+1]))
        {
          //Process this event. Divide and calculate error.
          double newWeight = itev->m_weight / value;
          itev->m_errorSquared = newWeight * newWeight *
                                ((itev->m_errorSquared / (itev->m_weight*itev->m_weight)) + valError_over_value_squared);
          itev->m_weight = newWeight;
          break; //out of the bin-searching-while-loop
        }
        ++bin;
        if( bin >= x_size - 1 ) break;

        //New bin! Find what you are multiplying!
        value = Y[bin];
        error = E[bin];

        // --- Division case ---
        if (value == 0)
        {
          value = std::numeric_limits<float>::quiet_NaN(); //Avoid divide by zero
          error = 0;
          valError_over_value_squared = 0;
        }
        else
          valError_over_value_squared = error * error / (value * value);

      }
      ++itev;
    }
  }


  //------------------------------------------------------------------------------------------------
  /** Divide the weights in this event list by an error-less scalar
   * The event list switches to WeightedEvent's if needed.
   * This simply calls the equivalent function: multiply(1.0/value).
   *
   * @param value: divide all weights by this amount.
   * @throw std::invalid_argument if value == 0; cannot divide by zero.
   */
  void EventList::divide(const double value)
  {
    this->divide(value, 0.0);
  }

  //------------------------------------------------------------------------------------------------
  /** Operator to divide the weights in this EventList by an error-less scalar.
   * Use divide(value,error) if your scalar has an error!
   * This simply calls the equivalent function: multiply(1.0/value).
   *
   * @param value divide by this
   * @return reference to this
   * @throw std::invalid_argument if value == 0; cannot divide by zero.
   */
  EventList& EventList::operator/=(const double value)
  {
    this->divide(value, 0.0);
    return *this;
  }

  //------------------------------------------------------------------------------------------------
  /** Divide the weights in this event list by a scalar with an error.
   * The event list switches to WeightedEvent's if needed.
   * This simply calls the equivalent function: multiply(1.0/value, error/(value*value)).
   *
   * @param value: divide all weights by this amount.
   * @param error: error on 'value'. Can be 0.
   * @throw std::invalid_argument if value == 0; cannot divide by zero.
   */
  void EventList::divide(const double value, const double error)
  {
    if (value == 0.0)
      throw std::invalid_argument("EventList::divide() called with value of 0.0. Cannot divide by zero.");
    //Do nothing if dividing by exactly 1.0, no error
    else if (value == 1.0 && error == 0.0)
      return;

    //Switch to weights if needed.
    this->switchToWeightedEvents();

    //Square of the value's error
    double valError_over_value_Squared = error * error / (value * value);

    std::vector<WeightedEvent>::iterator itev;
    for (itev = this->weightedEvents.begin(); itev != this->weightedEvents.end(); itev++)
    {
      double newValue = itev->m_weight / value;
      itev->m_errorSquared = newValue * newValue * ((itev->m_errorSquared / (itev->m_weight * itev->m_weight)) + valError_over_value_Squared);
      itev->m_weight = newValue;
    }
  }




  // ==============================================================================================
  // ----------- SPLITTING AND FILTERING ---------------------------------------
  // ==============================================================================================

  //------------------------------------------------------------------------------------------------
  /** Filter this EventList into an output EventList, using
   * keeping only events within the >= start and < end pulse times.
   * Detector IDs and the X axis are copied as well.
   *
   * @param start start time (absolute)
   * @param stop end time (absolute)
   * @param output reference to an event list that will be output.
   */
  void EventList::filterByPulseTime(DateAndTime start, DateAndTime stop, EventList & output) const
  {
    //Start by sorting the event list by pulse time.
    this->sortPulseTime();
    //Clear the output
    output.clear();
    //Copy the detector IDs
    output.detectorIDs = this->detectorIDs;
    output.refX = this->refX;

    //Iterate through all events (sorted by pulse time)

    if (has_weights)
    {
      //Make sure the output is also with weights
      output.switchToWeightedEvents();
      std::vector<WeightedEvent>::iterator itev = this->weightedEvents.begin();

      //Find the first event with m_pulsetime >= start
      while ((itev != this->weightedEvents.end()) && (itev->m_pulsetime < start))
        itev++;

      while ((itev != this->weightedEvents.end()) && (itev->m_pulsetime < stop))
      {
        //Add the copy to the output
        output.addEventQuickly(*itev);
        ++itev;
      }
    }
    else
    {
      std::vector<TofEvent>::iterator itev = this->events.begin();

      //Find the first event with m_pulsetime >= start
      while ((itev != this->events.end()) && (itev->m_pulsetime < start))
        itev++;

      while ((itev != this->events.end()) && (itev->m_pulsetime < stop))
      {
        //Add the copy to the output
        output.addEventQuickly(*itev);
        ++itev;
      }
    }

  }


  //------------------------------------------------------------------------------------------------
  /** Perform an in-place filtering on a vector of either TofEvent's or WeightedEvent's
   *
   * @param splitter :: a TimeSplitterType where all the entries (start/end time) indicate events
   *     that will be kept. Any other events will be deleted.
   * @param events :: either this->events or this->weightedEvents.
   */
  template< class T >
  void EventList::filterInPlaceHelper(Kernel::TimeSplitterType & splitter, typename std::vector<T> & events)
  {
    //Iterate through the splitter at the same time
    Kernel::TimeSplitterType::iterator itspl = splitter.begin();
    DateAndTime start, stop;
    int index;

    // Iterate for the input
    typename std::vector<T>::iterator itev = events.begin();

    // Iterator for the outputted list; will follow the input except when events are dropped.
    typename std::vector<T>::iterator itOut = events.begin();

    //This is the time of the first section. Anything before is thrown out.
    while (itspl != splitter.end())
    {
      //Get the splitting interval times and destination
      start = itspl->start();
      stop = itspl->stop();
      index = itspl->index();

      //Skip the events before the start of the time
      while ((itev != events.end()) && (itev->m_pulsetime < start))
        itev++;

      // Are we aligned in the input vs output?
      bool copyingInPlace = (itOut == itev);
      if (copyingInPlace)
      {
        while ((itev != events.end()) && (itev->m_pulsetime < stop))
          ++itev;
        // Make sure the iterators still match
        itOut = itev;
      }
      else
      {
        //Go through all the events that are in the interval (if any)
        while ((itev != events.end()) && (itev->m_pulsetime < stop))
        {
          if (index >= 0)
          {
            // Copy the input Event to the output iterator position.
            // Strictly speaking, this is not necessary if itOut == itev; but the extra check would likely
            //  slow down the filtering in the 99% of cases where itOut != itev.
            *itOut = *itev;
            // And go up a spot in the output iterator.
            ++itOut;
          }
          ++itev;
        }
      }

      //Go to the next interval
      itspl++;
      //But if we reached the end, then we are done.
      if (itspl==splitter.end())
        break;

      //No need to keep looping through the filter if we are out of events
      if (itev == events.end())
        break;

    } //Looping through entries in the splitter vector

    // Ok, now resize the event list to reflect the fact that it (probably) shrank
    events.resize( (itOut-events.begin()) );
  }


  //------------------------------------------------------------------------------------------------
  /** Use a TimeSplitterType to filter the event list in place.
   *
   * @param splitter :: a TimeSplitterType where all the entries (start/end time) indicate events
   *     that will be kept. Any other events will be deleted.
   */
  void EventList::filterInPlace(Kernel::TimeSplitterType & splitter)
  {
    //Start by sorting the event list by pulse time.
    this->sortPulseTime();

    if (has_weights)
      filterInPlaceHelper(splitter, this->weightedEvents);
    else
      filterInPlaceHelper(splitter, this->events);
  }





  //------------------------------------------------------------------------------------------------
  /** Split the event list into n outputs, operating on a vector of either TofEvent's or WeightedEvent's
   *
   * @param splitter :: a TimeSplitterType giving where to split
   * @param outputs :: a vector of where the split events will end up. The # of entries in there should
   *        be big enough to accommodate the indices.
   * @param events :: either this->events or this->weightedEvents.
   */
  template< class T >
  void EventList::splitByTimeHelper(Kernel::TimeSplitterType & splitter, std::vector< EventList * > outputs, typename std::vector<T> & events) const
  {
    int numOutputs = outputs.size();

    //Iterate through the splitter at the same time
    Kernel::TimeSplitterType::iterator itspl = splitter.begin();
    DateAndTime start, stop;
    int index;

    //Iterate through all events (sorted by tof)
    typename std::vector<T>::iterator itev = events.begin();

    //This is the time of the first section. Anything before is thrown out.
    while (itspl != splitter.end())
    {
      //Get the splitting interval times and destination
      start = itspl->start();
      stop = itspl->stop();
      index = itspl->index();

      //Skip the events before the start of the time
      while ((itev != events.end()) && (itev->m_pulsetime < start))
        itev++;

      //Go through all the events that are in the interval (if any)
      while ((itev != events.end()) && (itev->m_pulsetime < stop))
      {
        //Copy the event into another
        const T eventCopy(*itev);
        if ((index >= 0) && (index < numOutputs))
        {
          EventList * myOutput = outputs[index];
          //Add the copy to the output
          myOutput->addEventQuickly(eventCopy);
        }
        ++itev;
      }

      //Go to the next interval
      itspl++;
      //But if we reached the end, then we are done.
      if (itspl==splitter.end())
        break;

      //No need to keep looping through the filter if we are out of events
      if (itev == events.end())
        break;
    }
    //Done!
  }


  //------------------------------------------------------------------------------------------------
  /** Split the event list into n outputs
   *
   * @param splitter a TimeSplitterType giving where to split
   * @param outputs a vector of where the split events will end up. The # of entries in there should
   *        be big enough to accommodate the indices.
   */
  void EventList::splitByTime(Kernel::TimeSplitterType & splitter, std::vector< EventList * > outputs) const
  {
    //Start by sorting the event list by pulse time.
    this->sortPulseTime();

    //Initialize all the outputs
    int numOutputs = outputs.size();
    for (int i=0; i<numOutputs; i++)
    {
      outputs[i]->clear();
      outputs[i]->detectorIDs = this->detectorIDs;
      outputs[i]->refX = this->refX;
      if (has_weights)
        outputs[i]->switchToWeightedEvents();
    }

    //Do nothing if there are no entries
    if (splitter.size() <= 0)
      return;

    if (has_weights)
      splitByTimeHelper(splitter, outputs, this->weightedEvents);
    else
      splitByTimeHelper(splitter, outputs, this->events);
  }



} /// namespace DataObjects
} /// namespace Mantid
