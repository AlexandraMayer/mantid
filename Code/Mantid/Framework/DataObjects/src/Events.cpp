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
  /** Constructor, specifying the time of flight only
   * @param tof time of flight, in microseconds
   */
  TofEvent::TofEvent(const double tof) :
              m_tof(tof), m_pulsetime(0)
  {
  }

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
   * @return :: double, the tof (X value) of the event.
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

  /** Constructor, tof only:
   * @param time_of_flight: tof in microseconds.
   */
  WeightedEvent::WeightedEvent(double time_of_flight)
  : TofEvent(time_of_flight), m_weight(1.0), m_errorSquared(1.0)
  {
  }

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


} // DataObjects
} // Mantid

