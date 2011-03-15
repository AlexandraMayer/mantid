#ifndef MANTID_KERNEL_TIMESERIESPROPERTY_H_
#define MANTID_KERNEL_TIMESERIESPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include <iostream>
#include <limits>
#include <map>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <cctype>
#include "boost/date_time/posix_time/posix_time.hpp"

namespace Mantid
{
namespace Kernel
{

//================================================================================================
/** Struct holding some useful statistics for a TimeSeriesProperty
 *
 */
struct TimeSeriesPropertyStatistics
{
  /// Minimum value
  double minimum;
  /// Maximum value
  double maximum;
  /// Mean value
  double mean;
  /// Median value
  double median;
  /// standard_deviation of the values
  double standard_deviation;
  /// Duration in seconds
  double duration;
};

//================================================================================================
/** 
 A specialised Property class for holding a series of time-value pairs.
 Required by the LoadLog class.
 
 @author Russell Taylor, Tessella Support Services plc
 @date 26/11/2007
 @author Anders Markvardsen, ISIS, RAL
 @date 12/12/2007

 Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template<typename TYPE>
class DLLExport TimeSeriesProperty: public Property
{
private:
  /// typedef for the storage of a TimeSeries
  typedef std::multimap<DateAndTime, TYPE> timeMap;

  /// Holds the time series data 
  timeMap m_propertySeries;

  /// The number of values (or time intervals) in the time series. It can be different from m_propertySeries.size()
  int m_size;

public:
  /** Constructor
   *  @param name :: The name to assign to the property
   */
  explicit TimeSeriesProperty(const std::string &name) :
    Property(name, typeid(timeMap)), m_propertySeries(), m_size()
  {
  }

  /// Virtual destructor
  virtual ~TimeSeriesProperty()
  {
  }

  /// 'Virtual copy constructor'
  Property* clone() { return new TimeSeriesProperty<TYPE>(*this); }

  /** Return the memory used by the property, in bytes */
  size_t getMemorySize() const
  {
    //Rough estimate
    return m_propertySeries.size() * (sizeof(TYPE) + sizeof(DateAndTime));
  }

  /** Just returns the property (*this) unless overridden
  *  @param rhs a property that is merged in some descendent classes
  *  @return a property with the value
  */
  virtual TimeSeriesProperty& merge(Property * rhs)
  {
    return operator+=(rhs);
  }

  //--------------------------------------------------------------------------------------
  /** Add the value of another property
  * @param right the property to add
  * @return the sum
  */
  virtual TimeSeriesProperty& operator+=( Property const * right )
  {
    TimeSeriesProperty const * rhs = dynamic_cast< TimeSeriesProperty const * >(right);

    if (rhs)
    {
      if (rhs != this)
      {
        //Concatenate the maps!
        m_propertySeries.insert(rhs->m_propertySeries.begin(), rhs->m_propertySeries.end());
      }
      else
      {
        // Do nothing if appending yourself to yourself. The net result would be the same anyway
      }

      //Count the REAL size.
      m_size = static_cast<int>(m_propertySeries.size());

    }
    else
      g_log.warning() << "TimeSeriesProperty " << this->name() << " could not be added to another property of the same name but incompatible type.\n";

    return *this;
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Filter out a run by time. Takes out any TimeSeriesProperty log entries outside of the given
   *  absolute time range.
   * EXCEPTION: If there is only one entry in the list, it is considered to mean
   * "constant" so the value is kept even if the time is outside the range.
   *
   * @param start :: Absolute start time. Any log entries at times >= to this time are kept.
   * @param stop :: Absolute stop time. Any log entries at times < than this time are kept.
   */
  void filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop)
  {
    // Do nothing for single (constant) value
    if (m_propertySeries.size() <= 1)
      return;

    typename timeMap::iterator it;
    for (it = m_propertySeries.begin(); it != m_propertySeries.end(); /*increment within loop*/)
    {
      DateAndTime t = it->first;
      // Avoid iterator invalidation by caching current value and incrementing ahead of erase.
      typename timeMap::iterator it_current = it;
      ++it;
      if ((t < start) || (t >= stop))
      {
        //Is outside the range we are keeping, so erase that.
        m_propertySeries.erase(it_current);
      }
    }
    //Cache the size for later. Any filtered TSP's will have to fend for themselves.
    m_size = static_cast<int>(m_propertySeries.size());
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Split out a time series property by time intervals.
   *
   * NOTE: If the input TSP has a single value, it is assumed to be a constant
   *  and so is not split, but simply copied to all outputs.
   *
   * @param splitter :: a TimeSplitterType object containing the list of intervals and destinations.
   * @param outputs :: A vector of output TimeSeriesProperty pointers of the same type.
   */
  void splitByTime(TimeSplitterType& splitter, std::vector< Property * > outputs) const
  {
    size_t numOutputs = outputs.size();
    if (numOutputs <= 0)
      return;

    std::vector< TimeSeriesProperty<TYPE> *> outputs_tsp;
    //Clear the outputs before you start
    for (size_t i=0; i < numOutputs; i++)
    {
      TimeSeriesProperty<TYPE> * myOutput = dynamic_cast< TimeSeriesProperty<TYPE> * >(outputs[i]);
      if (myOutput)
      {
        outputs_tsp.push_back(myOutput);
        if (this->m_propertySeries.size() == 1)
        {
          // Special case for TSP with a single entry = just copy.
          myOutput->m_propertySeries = this->m_propertySeries;
          myOutput->m_size = 1;
        }
        else
        {
          myOutput->m_propertySeries.clear();
          myOutput->m_size=0;
        }
      }
      else
      {
        outputs_tsp.push_back( NULL );
      }
    }

    // Special case for TSP with a single entry = just copy.
    if (this->m_propertySeries.size() == 1)
      return;

    //We will be iterating through all the entries in the the map
    typename timeMap::const_iterator it;
    it = m_propertySeries.begin();

    //And at the same time, iterate through the splitter
    Kernel::TimeSplitterType::iterator itspl = splitter.begin();

    //Info of each splitter
    DateAndTime start, stop;
    int index;

    while (itspl != splitter.end())
    {
      //Get the splitting interval times and destination
      start = itspl->start();
      stop = itspl->stop();
      index = itspl->index();

      //Skip the events before the start of the time
      while ((it != this->m_propertySeries.end()) && (it->first < start))
        it++;

      //Go through all the events that are in the interval (if any)
      while ((it != this->m_propertySeries.end()) && (it->first < stop))
      {
        if ((index >= 0) && (index < static_cast<int>(numOutputs)))
        {
          TimeSeriesProperty<TYPE> * myOutput = outputs_tsp[index];
          //Copy the log out to the output
          if (myOutput)
            myOutput->addValue(it->first, it->second);
        }
        ++it;
      }

      //Go to the next interval
      itspl++;
      //But if we reached the end, then we are done.
      if (itspl==splitter.end())
        break;

      //No need to keep looping through the filter if we are out of events
      if (it == this->m_propertySeries.end())
        break;

    } //Looping through entries in the splitter vector

    //Make sure all entries have the correct size recorded in m_size.
    for (std::size_t i=0; i < numOutputs; i++)
    {
      TimeSeriesProperty<TYPE> * myOutput = dynamic_cast< TimeSeriesProperty<TYPE> * >(outputs[i]);
      if (myOutput)
        myOutput->m_size = myOutput->realSize();
    }

  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Fill a TimeSplitterType that will filter the events by matching
   * log values >= min and < max. Creates SplittingInterval's where
   * times match the log values, and going to index==0.
   *
   * @param split :: Splitter that will be filled.
   * @param min :: min value
   * @param max :: max value
   * @param TimeTolerance :: offset added to times in seconds.
   */
  void makeFilterByValue(TimeSplitterType& split, TYPE min, TYPE max, double TimeTolerance)
  {
    //Do nothing if the log is empty.
    if (m_propertySeries.size() == 0)
      return;

    typename timeMap::iterator it;
    bool lastGood = false;
    bool isGood;
    time_duration tol = DateAndTime::duration_from_seconds( TimeTolerance );
    int numgood = 0;
    DateAndTime lastTime, t;
    DateAndTime start, stop;

    for (it = m_propertySeries.begin(); it != m_propertySeries.end(); it++)
    {
      lastTime = t;
      //The new entry
      t = it->first;
      TYPE val = it->second;

      //A good value?
      isGood = ((val >= min) && (val < max));
      if (isGood)
        numgood++;

      if (isGood != lastGood)
      {
        //We switched from bad to good or good to bad

        if (isGood)
        {
          //Start of a good section
          start = t - tol;
        }
        else
        {
          //End of the good section
          if (numgood == 1)
          {
            //There was only one point with the value. Use the last time, + the tolerance, as the end time
            stop = lastTime + tol;
            split.push_back( SplittingInterval(start, stop, 0) );
          }
          else
          {
            //At least 2 good values. Save the end time
            stop = lastTime + tol;
            split.push_back( SplittingInterval(start, stop, 0) );
          }
          //Reset the number of good ones, for next time
          numgood = 0;
        }
        lastGood = isGood;
      }
    }

    if (numgood > 0)
    {
      //The log ended on "good" so we need to close it using the last time we found
      stop = t + tol;
      split.push_back( SplittingInterval(start, stop, 0) );
      numgood = 0;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /**  Return the time series as a correct C++ map<DateAndTime, TYPE>. All values
   * are included.
   *
   * @return time series property values as map
   */
  std::map<DateAndTime, TYPE> valueAsCorrectMap() const
  {
    std::map<DateAndTime, TYPE> asMap;
    if (m_propertySeries.size() == 0)
      return asMap;
    typename timeMap::const_iterator p = m_propertySeries.begin();
    for (; p != m_propertySeries.end(); p++)
      asMap[p->first] = p->second;

    return asMap;
  }

  //-----------------------------------------------------------------------------------------------
  /**  Return the time series's values as a vector<TYPE>
   *  @return the time series's values as a vector<TYPE>
   */
  std::vector<TYPE> valuesAsVector() const
  {
    std::vector<TYPE> out;
    if (m_propertySeries.size() == 0)
      return out;
    typename timeMap::const_iterator p = m_propertySeries.begin();
    for (; p != m_propertySeries.end(); p++)
      out.push_back(p->second);
    return out;
  }

  //-----------------------------------------------------------------------------------------------
  /**  Return the time series's times as a vector<DateAndTime>
    */
  std::vector<DateAndTime> timesAsVector() const
  {
    std::vector<DateAndTime> out;
    if (m_propertySeries.size() == 0)
      return out;
    typename timeMap::const_iterator p = m_propertySeries.begin();
    for (; p != m_propertySeries.end(); p++)
      out.push_back(p->first);
    return out;
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a value to the map
   *  @param time :: The time as a boost::posix_time::ptime value
   *  @param value :: The associated value
   *  @return True if insertion successful (i.e. identical time not already in map
   */
  bool addValue(const Kernel::DateAndTime &time, const TYPE value)
  {
    m_size++;
    return m_propertySeries.insert(typename timeMap::value_type(time, value)) != m_propertySeries.end();
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a value to the map
   *  @param time :: The time as a string in the format: (ISO 8601) yyyy-mm-ddThh:mm:ss
   *  @param value :: The associated value
   *  @return True if insertion successful (i.e. identical time not already in map
   */
  bool addValue(const std::string &time, const TYPE value)
  {
    return addValue(Kernel::DateAndTime(time), value);
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a value to the map
   *  @param time :: The time as a time_t value
   *  @param value :: The associated value
   *  @return True if insertion successful (i.e. identical time not already in map
   */
  bool addValue(const std::time_t &time, const TYPE value)
  {
    Kernel::DateAndTime dt;
    dt.set_from_time_t(time);
    return addValue(dt, value);
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the last time
   *  @return Value
   */
  DateAndTime lastTime() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.rbegin()->first;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the first value
   *  @return Value
   */
  TYPE firstValue() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.begin()->second;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the first time
   *  @return Value
   */
  DateAndTime firstTime() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.begin()->first;
  }

  //-----------------------------------------------------------------------------------------------
  /// Clones the property
  TimeSeriesProperty<TYPE>* clone() const
  {
    TimeSeriesProperty<TYPE>* p = new TimeSeriesProperty<TYPE> (name());
    p->m_propertySeries = m_propertySeries;
    p->m_size = m_size;
    return p;
  }

  //-----------------------------------------------------------------------------------------------
  /// Returns the number of values at UNIQUE time intervals in the time series
  int size() const
  {
    return m_size;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the real size of the time series property map:
   * the number of entries, including repeated ones.
   */
  int realSize() const
  {
    return static_cast<int>(m_propertySeries.size());
  }





  // ==== The following functions are specific to the odd mechanism of FilterByLogValue =========



  //-----------------------------------------------------------------------------------------------
  /* Get the time series property as a string of 'time  value'
   *
   * @return time series property as a string
   */
  std::string value() const
  {
    std::stringstream ins;

    typename timeMap::const_iterator p = m_propertySeries.begin();

    while (p != m_propertySeries.end())
    {
      try
      {
        ins << p->first.to_simple_string();
        ins << "  " << p->second << std::endl;
      }
      catch (...)
      {
        //Some kind of error; for example, invalid year, can occur when converying boost time.
        ins << "Error Error" << std::endl;
      }
      p++;
    }

    return ins.str();
  }

  //-----------------------------------------------------------------------------------------------
  /**  New method to return time series value pairs as std::vector<std::string>
   *
   * @return time series property values as a string vector "<time_t> value"
   */
  std::vector<std::string> time_tValue() const
  {
    std::vector<std::string> values;
    values.reserve(m_propertySeries.size());

    typename timeMap::const_iterator p = m_propertySeries.begin();

    while (p != m_propertySeries.end())
    {
      std::stringstream line;
      line << p->first.to_simple_string() << " " << p->second;
      values.push_back(line.str());
      p++;
    }

    return values;
  }

  //-----------------------------------------------------------------------------------------------
  /**  Return the time series as a C++ map<DateAndTime, TYPE>
   *
   * WARNING: THIS ONLY RETURNS UNIQUE VALUES, AND SKIPS ANY REPEATED VALUES!
   *   USE AT YOUR OWN RISK! Try valueAsCorrectMap() instead.
   * @return time series property values as map
   */
  std::map<DateAndTime, TYPE> valueAsMap() const
  {
    std::map<DateAndTime, TYPE> asMap;
    if (m_propertySeries.size() == 0)
      return asMap;
    typename timeMap::const_iterator p = m_propertySeries.begin();
    TYPE d = p->second;
    for (; p != m_propertySeries.end(); p++)
    {
      //Skips any entries where the value was unchanged.
      if (p != m_propertySeries.begin() && p->second == d) continue;
      d = p->second;
      asMap[p->first] = d;
    }

    return asMap;
  }


  //-----------------------------------------------------------------------------------------------
  /** Not implemented in this class
   *  @throw Exception::NotImplementedError Not yet implemented
   * @return Nothing in this case
   */
  std::string setValue(const std::string&)
  {
    throw Exception::NotImplementedError("Not implemented in this class");
  }


  //-----------------------------------------------------------------------------------------------
  /** Clears and creates a TimeSeriesProperty from these parameters:
   *
   *  @param start_time :: The reference time as a boost::posix_time::ptime value
   *  @param time_sec :: A vector of time offset (from start_time) in seconds.
   *  @param new_values :: A vector of values, each corresponding to the time offset in time_sec.
   *    Vector sizes must match.
   */
  void create(const Kernel::DateAndTime &start_time, const std::vector<double> & time_sec, const std::vector<TYPE> & new_values)
  {
    if (time_sec.size() != new_values.size())
      throw std::invalid_argument("TimeSeriesProperty::create: mismatched size for the time and values vectors.");

    // Make the times(as seconds) into a vector of DateAndTime in one go.
    std::vector<DateAndTime> times;
    DateAndTime::createVector(start_time, time_sec, times);

    m_size = 0;
    m_propertySeries.clear();
    // Give a guess as to where it goes
    typename std::multimap<Kernel::DateAndTime, TYPE>::iterator iter = m_propertySeries.begin();
    std::size_t num = new_values.size();
    for (std::size_t i=0; i < num; i++)
    {
      // By providing a guess iterator to the insert method, it speeds inserting up by a good amount.
      iter = m_propertySeries.insert(iter, typename timeMap::value_type(times[i], new_values[i]));
    }

//    // Make a list of the pairs to insert
//    std::vector<typename std::pair<Kernel::DateAndTime, TYPE> > list;
//    std::size_t num = new_values.size();
//    for (std::size_t i=0; i < num; i++)
//    {
//      list.push_back( typename std::pair<Kernel::DateAndTime, TYPE>(times[i], new_values[i]) );
//    }
//    // Insert the whole list in one go.
//    m_propertySeries.insert(list.begin(), list.end());

    m_size = static_cast<int>(m_propertySeries.size());
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the value at a particular time
   *  @param t :: time
   *  @return Value at time \a t
   */
  TYPE getSingleValue(const DateAndTime& t) const
  {
    typename timeMap::const_reverse_iterator it = m_propertySeries.rbegin();
    for (; it != m_propertySeries.rend(); it++)
      if (it->first <= t)
        return it->second;
    if (m_propertySeries.size() == 0)
      return TYPE();
    else
      return m_propertySeries.begin()->second;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns total value, added up for all times
   *  @return Total value from all times
   */
  TYPE getTotalValue() const
  {
    typename timeMap::const_iterator it = m_propertySeries.begin();
    TYPE total = 0;
    for (; it != m_propertySeries.end(); it++)
      total += it->second;
    return total;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns n-th value in an incredibly inefficient way.
   *  @param n :: index
   *  @return Value 
   */
  TYPE nthValue(int n) const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");

    typename timeMap::const_iterator it = m_propertySeries.begin();
    for (std::size_t j = 0; it != m_propertySeries.end(); it++)
    {
      if (m_propertySeries.count(it->first) > 1)
        continue;
      if (j == static_cast<size_t>(n))
        return it->second;
      j++;
    }

    return m_propertySeries.rbegin()->second;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns n-th time. NOTE: Complexity is order(n)!
   *  @param n :: index
   *  @return DateAndTime
   */
  Kernel::DateAndTime nthTime(int n) const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");

    typename timeMap::const_iterator it = m_propertySeries.begin();
    for (std::size_t j = 0; it != m_propertySeries.end(); it++)
    {
      if (j == static_cast<size_t>(n))
        return it->first;
      j++;
    }

    return m_propertySeries.rbegin()->first;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the last value
   *  @return Value 
   */
  TYPE lastValue() const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");
    return m_propertySeries.rbegin()->second;
  }



  //-----------------------------------------------------------------------------------------------
  /** Returns n-th valid time interval, in a very inefficient way.
   *  @param n :: index
   *  @return n-th time interval
   */
  TimeInterval nthInterval(int n) const
  {
    if (m_propertySeries.empty())
      throw std::runtime_error("TimeSeriesProperty is empty");

    typename timeMap::const_iterator it = m_propertySeries.begin();
    DateAndTime t = it->first;
    for (size_t j = 0; it != m_propertySeries.end(); it++)
    {
      if (m_propertySeries.count(it->first) > 1)
        continue;
      if (j == static_cast<size_t>(n))
      {
        typename timeMap::const_iterator it1 = it;
        it1++;
        if (it1 != m_propertySeries.end())
          return TimeInterval(it->first, it1->first);
        else
        {
          //We are at the end of the series
          // ---> Previous functionality = use a d = 1/10th of the total time. Doesn't make sense to me!
          // ----> time_duration d = (m_propertySeries.rbegin()->first - m_propertySeries.begin()->first) / 10;

          //Use the previous interval instead
          typename timeMap::const_iterator it2 = it;
          it2--;
          time_duration d = it->first - it2->first;

          //Make up an end time.
          DateAndTime endTime = it->first + d;
          return TimeInterval(it->first, endTime);
        }
        if (it1 != m_propertySeries.end() && it1->first == it->first)
          continue;
      }
      t = it->first;
      j++;
    }

    return TimeInterval();
  }


  //-----------------------------------------------------------------------------------------------
  /** Divide the property into  allowed and disallowed time intervals according to \a filter.
   Repeated time-value pairs (two same time and value entries) mark the start of a gap in the values. 
   The gap ends and an allowed time interval starts when a single time-value is met.
   @param filter :: The filter mask to apply
   */
  void filterWith(const TimeSeriesProperty<bool>* filter)
  {
    std::map<DateAndTime, bool> fmap = filter->valueAsMap();
    std::map<DateAndTime, bool>::const_iterator f = fmap.begin();
	if(fmap.empty()) return;
    typename timeMap::iterator it = m_propertySeries.begin();
    if (f->first < it->first)// expand this series
    {
      m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
      it = m_propertySeries.begin();
    }
    bool hide = !f->second;
    bool finished = false;
    // At the start f->first >= it->first
    for (; f != fmap.end(); hide = !(f++)->second)
    {
      // element next to it
      typename timeMap::iterator it1 = it;
      it1++;
      if (it1 == m_propertySeries.end())
      {
        if (f->first < it->first && hide)
          if (m_propertySeries.count(it->first) == 1)
            m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
        finished = true;
        break;
      }
      else
        // we want f to be between it and it1
        while (f->first < it->first || f->first >= it1->first)
        {
          // we are still in the scope of the previous filter value, and if it 'false' we must hide the interval
          if (hide)
          {
            if (m_propertySeries.count(it->first) == 1)
            {
              it = m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
            }
            if (it1 != m_propertySeries.end() && m_propertySeries.count(it1->first) == 1 && f->first
                > it1->first)
              m_propertySeries.insert(typename timeMap::value_type(it1->first, it1->second));
          }
          it++;
          it1 = it;
          it1++;
          if (it1 == m_propertySeries.end())
          {
            if (hide && f->first != it->first)
            {
              if (m_propertySeries.count(it->first) == 1)
                it = m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
              it1 = m_propertySeries.end();
            }
            finished = true;
            break;
          }
        };
      bool gap = m_propertySeries.count(it->first) > 1;
      if (gap && it1 != m_propertySeries.end())
      {
        if (f->second == true)
          it = m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
      }
      else
      {
        if (f->second == false)
        {
          if (f->first != it->first)
            m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
          it = m_propertySeries.insert(typename timeMap::value_type(f->first, it->second));
        }
      }
    }
    // If filter stops in the middle of this series with value 'false' (meaning hide the rest of the values)
    if (!finished && fmap.rbegin()->second == false)
    {
      for (; it != m_propertySeries.end(); it++)
        if (m_propertySeries.count(it->first) == 1)
          it = m_propertySeries.insert(typename timeMap::value_type(it->first, it->second));
    }

    // Extend this series if the filter end later than the data
    if (finished && f != fmap.end())
    {
      TYPE v = m_propertySeries.rbegin()->second;
      for (; f != fmap.end(); f++)
      {
        m_propertySeries.insert(typename timeMap::value_type(f->first, v));
        if (!f->second)
          m_propertySeries.insert(typename timeMap::value_type(f->first, v));
      }
    }

    countSize();
  }


  //-----------------------------------------------------------------------------------------------
  /// Restores the property to the unsorted state
  void clearFilter()
  {
    std::map<DateAndTime, TYPE> pmap = valueAsMap();
    m_propertySeries.clear();
    m_size = 0;
    if (pmap.size() == 0)
      return;
    TYPE val = pmap.begin()->second;
    typename std::map<DateAndTime, TYPE>::const_iterator it = pmap.begin();
    addValue(it->first, it->second);
    for (; it != pmap.end(); it++)
    {
      if (it->second != val)
        addValue(it->first, it->second);
      val = it->second;
    }
  }



  //-----------------------------------------------------------------------------------------------
  /** Updates m_size.
   * TODO: Warning! COULD BE VERY SLOW, since it counts each entry each time.
   */
  void countSize()
  {
    m_size = 0;
    if (m_propertySeries.size() == 0)
      return;
    typename timeMap::const_iterator it = m_propertySeries.begin();
    for (; it != m_propertySeries.end(); it++)
    {
      if (m_propertySeries.count(it->first) == 1)
        m_size++;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /**  Check if str has the right time format 
   *   @param str :: The string to check
   *   @return True if the format is correct, false otherwise.
   */
  static bool isTimeString(const std::string &str)
  {
      if (str.size() < 19) return false;
      if (!isdigit(str[0])) return false;
      if (!isdigit(str[1])) return false;
      if (!isdigit(str[2])) return false;
      if (!isdigit(str[3])) return false;
      if (!isdigit(str[5])) return false;
      if (!isdigit(str[6])) return false;
      if (!isdigit(str[8])) return false;
      if (!isdigit(str[9])) return false;
      if (!isdigit(str[11])) return false;
      if (!isdigit(str[12])) return false;
      if (!isdigit(str[14])) return false;
      if (!isdigit(str[15])) return false;
      if (!isdigit(str[17])) return false;
      if (!isdigit(str[18])) return false;
      return true;
  }
  
  //-----------------------------------------------------------------------------------------------
  /** This doesn't check anything -we assume these are always valid
   * 
   *  @returns an empty string ""
   */
  std::string isValid() const { return ""; }


  //-----------------------------------------------------------------------------------------------
  /* Not implemented in this class
   * @throw Exception::NotImplementedError Not yet implemented
   */
  std::string getDefault() const
  {
    throw Exception::NotImplementedError("TimeSeries properties don't have defaults");
  }

  //-----------------------------------------------------------------------------------------------
  ///Not used in this class and always returns false
  bool isDefault() const { return false; }

  /**
   * Return a TimeSeriesPropertyStatistics struct containing the
   * statistics of this TimeSeriesProperty object.
   */
  TimeSeriesPropertyStatistics getStatistics()
  {
    TimeSeriesPropertyStatistics out;
    Mantid::Kernel::Statistics raw_stats
                       = Mantid::Kernel::getStatistics(this->valuesAsVector());
    out.mean = raw_stats.mean;
    out.standard_deviation = raw_stats.standard_deviation;
    out.median = raw_stats.median;
    out.minimum = raw_stats.minimum;
    out.maximum = raw_stats.maximum;
    if (this->size() > 0)
    {
      out.duration = DateAndTime::seconds_from_duration(this->lastTime() - this->firstTime());
    }
    else
    {
      out.duration = std::numeric_limits<double>::quiet_NaN();
    }

    return out;
  }


  /// Static reference to the logger class
  static Logger& g_log;

};


template <typename TYPE>
Logger& TimeSeriesProperty<TYPE>::g_log = Logger::get("TimeSeriesProperty");

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_TIMESERIESPROPERTY_H_*/
