#ifndef PYTHONOBJECTINSTANTIATORTEST_H_
#define PYTHONOBJECTINSTANTIATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include "MantidKernel/ConfigService.h"

#include <boost/python/object.hpp>

using Mantid::PythonInterface::PythonObjectInstantiator;
using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;

class PythonObjectInstantiatorTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PythonObjectInstantiatorTest *createSuite() { return new PythonObjectInstantiatorTest(); }
  static void destroySuite( PythonObjectInstantiatorTest *suite ) { delete suite; }

public:
  //
  PythonObjectInstantiatorTest() : m_creator(NULL)
  {
    Py_Initialize();
  }

  ~PythonObjectInstantiatorTest()
  {
    delete m_creator;
    //Py_Finalize(); //- Causes Python 2.4 to fail. Need to find out why
  }

  void test_Bare_Pointer()
  {
    PythonObjectInstantiator<IAlgorithm> *factory = getInstantiator();
    IAlgorithm *alg = factory->createUnwrappedInstance();
    TS_ASSERT(alg);
    TS_ASSERT_EQUALS(alg->name(), "PyAlg");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "PythonAlgorithms");
  }

  void test_Shared_Pointer()
   {
     PythonObjectInstantiator<IAlgorithm> *factory = getInstantiator();
     IAlgorithm_sptr alg = factory->createInstance();
     TS_ASSERT(alg);
     TS_ASSERT_EQUALS(alg->name(), "PyAlg");
     TS_ASSERT_EQUALS(alg->version(), 1);
     TS_ASSERT_EQUALS(alg->category(), "PythonAlgorithms");
   }

private:
  PythonObjectInstantiator<IAlgorithm> *getInstantiator()
  {
    if( !m_creator )
    {
      std::string propDir = Mantid::Kernel::ConfigService::Instance().getPropertiesDir();
#ifdef _WIN32
      propDir += "\\"; // Escape the last backslash, on Windows, so python isn't confused
#endif
      //Assume this is where the mantid package is too
      std::string code = "import sys\n"
        "sys.path.append(r'" + propDir + "')\n"
        "from mantid.api import Algorithm\n"
        "class PyAlg(Algorithm):\n"
        "  pass\n";
      PyRun_SimpleString(code.c_str());
      PyObject *main = PyImport_AddModule("__main__");
      TS_ASSERT(main);
      boost::python::object cls(boost::python::handle<>(PyObject_GetAttrString(main, "PyAlg")));
      TS_ASSERT(cls);
      m_creator = new PythonObjectInstantiator<IAlgorithm>(cls);
    }
    return m_creator;
  }

  /// Instantiator instance
  PythonObjectInstantiator<IAlgorithm> *m_creator;
};

#endif /* PYTHONOBJECTINSTANTIATORTEST_H_ */
