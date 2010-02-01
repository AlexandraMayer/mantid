#ifndef MANTID_PYTHONAPI_PYTHONINTERFACEFUNCTIONS_H_
#define MANTID_PYTHONAPI_PYTHONINTERFACEFUNCTIONS_H_

#include <MantidPythonAPI/FrameworkManagerProxy.h>
#include <boost/python/call_method.hpp>
#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonAPI
  {
    /** 
	A set of routines for interfacing with Python and performing various C++ -> Python conversions.
	
	@author ISIS, STFC
	@date 13/01/2010
	
	Copyright &copy; 2007 STFC Rutherford Appleton Laboratories
	
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
	
	File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    */
  
    // @cond UNDOC
    // We have to perform some magic due to threading issues.  There are 2 scenarios:
    // 1) If asynchronous execution of code is requested from currently executing Python code then we must acquire the GIL before we
    //    perform an call up to python;
    // 2) If asynchronous execution of code is requested from outside of running Python code then the GIL is not required since there is
    //    no other thread to lock against and worse still if we try then we get a deadlock

    // See http://docs.python.org/c-api/init.html#thread-state-and-the-global-interpreter-lock for more information on the GIL

    /**
     * A simple class that implements an RAII interface for the Python GIL. The GIL is a Python global that needs to be acquired by
     * a thread, other than the interpreter thread, that wishs to execute Python code
     */
    class PythonLocker
    {
    public:
      ///Constructor
      PythonLocker() : m_tstate(PyGILState_UNLOCKED), m_locked(false)
      {
      }

      void lock()
      {
	m_tstate = PyGILState_Ensure();
	m_locked = true;
      }
      ///Destructor
      ~PythonLocker()
      {
	if( m_locked )
	{
	  PyGILState_Release(m_tstate);
	}
      }
    private:
      /// Store the thread state
      PyGILState_STATE m_tstate;
      /// If we've locked the state
      bool m_locked;
    };

    /// Handle a Python error state
    void handle_python_error();
    /// A structure t handle default returns for template functions
    template<typename ResultType>
    struct DefaultReturn
    {
    };

#define DECLARE_DEFAULTRETURN(type, value)\
    template<>\
    struct DefaultReturn<type>\
    {\
      type operator()()\
      {\
	return value;\
      }\
    };\
    template<>\
    struct DefaultReturn<const type>\
    {\
      const type operator()()\
      {\
	return value;\
      }\
    };

    DECLARE_DEFAULTRETURN(int, 0)
    DECLARE_DEFAULTRETURN(bool, false)
    DECLARE_DEFAULTRETURN(std::string, std::string())
    
    /**
     * MG 14/01/2010
     * 
     * The boost::python call_method function, through much template/macro trickery, is able to take an 
     * arbitrary number of arguments of any type. Since emulating this would be extremely time 
     * consuming we'll stick to simply defining functions as needed, there shouldn't be too many.
     *
     * This does lead to some code replication as all locking stuff needs defining in each separate template
     * function. If we had C++0x we could do it with variadic templates but alas we are not there yet.
     */
    /** @name No argument Python calls */
    //@{
    /**
     * Perform a call to a python function that takes no arguments and returns a value
     */
    template<typename ResultType>
    struct PyCall_NoArg
    {

      static ResultType dispatch(PyObject *object, const std::string & func_name)
      {
	PythonLocker gil;
	if( FrameworkManagerProxy::requireGIL() )
	{
	  gil.lock();
	}
	try 
	{
	  return boost::python::call_method<ResultType>(object, func_name.c_str());
	}
	catch(boost::python::error_already_set&)
	{
	  handle_python_error();
	}
	DefaultReturn<ResultType> r;
	return r();
      }
    };
    ///Specialization for void return type
    template<>
    struct PyCall_NoArg<void>
    {

      static void dispatch(PyObject *object, const std::string & func_name)
      {
	PythonLocker gil;
	if( FrameworkManagerProxy::requireGIL() )
	{
	  gil.lock();
	}
	PyThreadState *tstate = PyThreadState_GET();
	try
	{
	  boost::python::call_method<void>(object, func_name.c_str());
	}
	catch(boost::python::error_already_set&)
	{
	  PyThreadState_Swap(tstate);
	  handle_python_error();
	}
      }
    };
    //@}

    /** @name Single argument Python calls */
    //@{
    /**
     * Perform a call to a python function that takes a single argument and returns a value
     */
    template<typename ResultType, typename ArgType>
    struct PyCall_OneArg
    {

      static ResultType dispatch(PyObject *object, const std::string & func_name, const ArgType & arg)
      {
	PythonLocker gil;
	if( FrameworkManagerProxy::requireGIL() )
	{
	  gil.lock();
	}
	try 
	{
	  return boost::python::call_method<ResultType>(object, func_name.c_str(), arg);
	}
	catch(boost::python::error_already_set&)
	{
	  handle_python_error();
	}
	DefaultReturn<ResultType> r;
	return r();
      }
    };
    ///Specialization for void return type
    template<typename ArgType>
    struct PyCall_OneArg<void, ArgType>
    {

      static void dispatch(PyObject *object, const std::string & func_name, const ArgType & arg)
      {
	PythonLocker gil;
	if( FrameworkManagerProxy::requireGIL() )
	{
	  gil.lock();
	}
	try 
	{
	  boost::python::call_method<void>(object, func_name.c_str(), arg);
	}
	catch(boost::python::error_already_set&)
	{
	  handle_python_error();
	}
      }

     };
    //@}

    namespace Conversions
    {
      /** @name Conversion functions */
      //@{
      /// Convert a Boost Python list to a std::vector of the requested type
      template<typename TYPE>
      std::vector<TYPE> convertToStdVector(const boost::python::list & pylist)
      {
	int length = boost::python::extract<int>(pylist.attr("__len__")());
	std::vector<TYPE> seq_std(length, TYPE());
	if( length == 0 )
	{
	  return seq_std;
	}

	for( int i = 0; i < length; ++i )
	{
	  boost::python::extract<TYPE> cppobj(pylist[i]);
	  if( cppobj.check() )
	  {
	    seq_std[i] = cppobj();
	  }
	}
	return seq_std;
      }
    }
    //@}
    //@endcond
    
  }
}



#endif //MANTID_PYTHONAPI_PYTHONINTERFACEFUNCTIONS_H_
