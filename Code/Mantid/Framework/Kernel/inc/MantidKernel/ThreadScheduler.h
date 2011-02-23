#ifndef THREADSCHEDULER_H_
#define THREADSCHEDULER_H_

#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/MultiThreaded.h"
#include <vector>
#include <deque>
#include <map>

namespace Mantid
{

namespace Kernel
{

  /** The ThreadScheduler object defines how tasks are
   * allocated to threads and in what order.
   * It holds the queue of tasks.
   *
    @author Janik Zikovsky, SNS
    @date Feb 7, 2011
  
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
  
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

  //===========================================================================
  //===========================================================================
  //===========================================================================
  class DLLExport ThreadScheduler
  {
  public:
    /** Constructor
     */
    ThreadScheduler()
    : m_cost(0), m_costExecuted(0)
    {
    }
  
    //-----------------------------------------------------------------------------------
    /** Add a Task to the queue.
     * @param newTask :: Task to add to queue
     */
    virtual void push(Task * newTask) = 0;

    //-----------------------------------------------------------------------------------
    /** Retrieves the next Task to execute.
     * @param threadnum :: ID of the calling thread.
     */
    virtual Task * pop(size_t threadnum) = 0;

    //-----------------------------------------------------------------------------------
    /** Signal to the scheduler that a task is complete. The
     * scheduler may release mutexes, etc.
     *
     * @param task :: the Task that was completed.
     */
    virtual void finished(Task * task)
    { (void) task; //Ignore argument
    }
  

    //-----------------------------------------------------------------------------------
    /// Returns the size of the queue
    virtual size_t size() = 0;

    /// Empty out the queue
    virtual void clear() = 0;


    //-------------------------------------------------------------------------------
    /// Returns the total cost of all Task's in the queue.
    double totalCost()
    {
      return m_cost;
    }

    //-------------------------------------------------------------------------------
    /// Returns the total cost of all Task's in the queue.
    double totalCostExecuted()
    {
      return m_costExecuted;
    }

  protected:
    /// Total cost of all tasks
    double m_cost;
    /// Accumulated cost of tasks that have been executed (popped)
    double m_costExecuted;
    /// Mutex to prevent simultaneous access to the queue.
    Mutex m_queueLock;

  };

  
  //===========================================================================
  //===========================================================================
  //===========================================================================
  /** A First-In-First-Out Thread Scheduler.
   * 
   * A queue of tasks is maintained and are run in the order
   * they were submitted.
   * 
   */
  class DLLExport ThreadSchedulerFIFO : public ThreadScheduler
  {
  public:
    ThreadSchedulerFIFO()
      : ThreadScheduler()
    {}
  
    //-------------------------------------------------------------------------------
    void push(Task * newTask)
    {
      // Cache the total cost
      m_queueLock.lock();
      m_cost += newTask->cost();
      m_queue.push_back(newTask);
      m_queueLock.unlock();
    }

    //-------------------------------------------------------------------------------
    virtual Task * pop(size_t threadnum)
    {
      (void) threadnum; //Ignore argument
      if (this->size() > 0)
      {
        m_queueLock.lock();
        //TODO: Would a try/catch block be smart here?
        Task * temp = m_queue.front();
        m_queue.pop_front();
        m_queueLock.unlock();
        return temp;
      }
      else
        return NULL;
    }
  
    //-------------------------------------------------------------------------------
    size_t size()
    {
      m_queueLock.lock();
      size_t temp = m_queue.size();
      m_queueLock.unlock();
      return temp;
    }
  
    //-------------------------------------------------------------------------------
    void clear()
    {
      m_queueLock.lock();
      m_queue.clear();
      m_cost = 0;
      m_costExecuted = 0;
      m_queueLock.unlock();
    }

  protected:
    std::deque<Task*> m_queue;
  };




  //===========================================================================
  //===========================================================================
  //===========================================================================
  /** A Last-In-First-Out Thread Scheduler.
   *
   * A queue of tasks is maintained;
   * the last Task added is the first one returned.
   *
   */
  class DLLExport ThreadSchedulerLIFO : public ThreadSchedulerFIFO
  {

    //-------------------------------------------------------------------------------
    Task * pop(size_t threadnum)
    {
      (void) threadnum; //Ignore argument
      if (this->size() > 0)
      {
        m_queueLock.lock();
        Task * temp = m_queue.back();
        m_queue.pop_back();
        m_queueLock.unlock();
        return temp;
      }
      else
        return NULL;
    }
  };




  //===========================================================================
  //===========================================================================
  //===========================================================================
  /** A Largest Cost Thread Scheduler.
   *
   * The scheduled tasks are run so that the most time-consuming
   * (highest cost) taks are run first.
   * This tends to optimize task allocation the best.
   * http://en.wikipedia.org/wiki/Bin_packing_problem
   *
   * Interally, it uses a multimap to keep elements sorted while inserting them.
   */
  class DLLExport ThreadSchedulerLargestCost : public ThreadScheduler
  {
  public:
    ThreadSchedulerLargestCost()
      : ThreadScheduler()
    {}

    //-------------------------------------------------------------------------------
    void push(Task * newTask)
    {
      // Cache the total cost
      m_queueLock.lock();
      m_cost += newTask->cost();
      m_map.insert( std::pair<double, Task*>(newTask->cost(), newTask) );
      m_queueLock.unlock();
    }

    //-------------------------------------------------------------------------------
    virtual Task * pop(size_t threadnum)
    {
      (void) threadnum; //Ignore argument
      //TODO: Thread-safety
      if (this->size() > 0)
      {
        m_queueLock.lock();
        // Since the map is sorted by cost, we want the LAST item.
        std::multimap<double, Task*>::iterator it = m_map.end();
        it--;
        Task * temp = it->second;
        m_map.erase(it);
        m_queueLock.unlock();
        return temp;
      }
      else
        return NULL;
    }

    //-------------------------------------------------------------------------------
    size_t size()
    {
      m_queueLock.lock();
      size_t temp = m_map.size();
      m_queueLock.unlock();
      return temp;
    }

    //-------------------------------------------------------------------------------
    void clear()
    {
      m_queueLock.lock();
      m_map.clear();
      m_cost = 0;
      m_costExecuted = 0;
      m_queueLock.unlock();
    }

  protected:
    /// A multimap keeps tasks sorted by the key (cost)
    std::multimap<double, Task*> m_map;
  };






} // namespace Kernel
} // namespace Mantid

#endif /* THREADSCHEDULER_H_ */
