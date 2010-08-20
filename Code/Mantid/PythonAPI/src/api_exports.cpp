//
// Wrappers for classes in the API namespace
//
#include <MantidPythonAPI/api_exports.h>
#include <MantidPythonAPI/stl_proxies.h>
#include <MantidPythonAPI/WorkspaceProxies.h>
#include <string>
#include <ostream>

// API
#include <MantidAPI/ITableWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>
#include <MantidAPI/IEventWorkspace.h>
#include <MantidAPI/Instrument.h>
#include <MantidAPI/ParInstrument.h>
#include <MantidAPI/Sample.h>
#include <MantidAPI/WorkspaceProperty.h>
#include <MantidAPI/FileProperty.h>
#include <MantidAPI/WorkspaceValidators.h>
#include <MantidAPI/FileFinder.h>

#include <MantidPythonAPI/PyAlgorithmWrapper.h>

namespace Mantid
{
namespace PythonAPI
{
  using namespace API;
  using namespace boost::python;

  //@cond
  //---------------------------------------------------------------------------
  // Class export functions
  //---------------------------------------------------------------------------

  void export_frameworkmanager()
  {
    /** 
     * Python Framework class (note that this is not the API::FrameworkManager, there is another in 
     * PythonAPI::FrameworkManager)
     * This is the main class through which Python interacts with Mantid and with the exception of PyAlgorithm and V3D, 
     * is the only one directly instantiable in Python
     */
    class_<FrameworkManagerProxy, FrameworkProxyCallback, boost::noncopyable>("FrameworkManager")
      .def("clear", &FrameworkManagerProxy::clear)
      .def("clearAlgorithms", &FrameworkManagerProxy::clearAlgorithms)
      .def("clearData", &FrameworkManagerProxy::clearData)
      .def("clearInstruments", &FrameworkManagerProxy::clearInstruments)
      .def("createAlgorithm", (createAlg_overload1)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("createAlgorithm", (createAlg_overload2)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("createAlgorithm", (createAlg_overload3)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("createAlgorithm", (createAlg_overload4)&FrameworkManagerProxy::createAlgorithm, 
	   return_value_policy< reference_existing_object >())
      .def("registerPyAlgorithm", &FrameworkManagerProxy::registerPyAlgorithm)
      .def("_observeAlgFactoryUpdates", &FrameworkManagerProxy::observeAlgFactoryUpdates)
      .def("deleteWorkspace", &FrameworkManagerProxy::deleteWorkspace)
      .def("getWorkspaceNames", &FrameworkManagerProxy::getWorkspaceNames)
      .def("getWorkspaceGroupNames", &FrameworkManagerProxy::getWorkspaceGroupNames)
      .def("getWorkspaceGroupEntries", &FrameworkManagerProxy::getWorkspaceGroupEntries)
      .def("createPythonSimpleAPI", &FrameworkManagerProxy::createPythonSimpleAPI)
      .def("sendLogMessage", &FrameworkManagerProxy::sendLogMessage)
      .def("workspaceExists", &FrameworkManagerProxy::workspaceExists)
      .def("getConfigProperty", &FrameworkManagerProxy::getConfigProperty)
      .def("_getRawMatrixWorkspacePointer", &FrameworkManagerProxy::retrieveMatrixWorkspace)
      .def("_getRawTableWorkspacePointer", &FrameworkManagerProxy::retrieveTableWorkspace)
      .def("_getRawWorkspaceGroupPointer", &FrameworkManagerProxy::retrieveWorkspaceGroup)
      .def("_workspaceRemoved", &FrameworkProxyCallback::default_workspaceRemoved)
      .def("_workspaceReplaced", &FrameworkProxyCallback::default_workspaceReplaced)
      .def("_workspaceAdded", &FrameworkProxyCallback::default_workspaceAdded)
      .def("_workspaceStoreCleared", &FrameworkProxyCallback::default_workspaceStoreCleared)
      .def("_algorithmFactoryUpdated", &FrameworkProxyCallback::default_algorithmFactoryUpdated) 
      .def("_setGILRequired", &FrameworkManagerProxy::setGILRequired)
      .staticmethod("_setGILRequired")
    ;
  }

  void export_ialgorithm()
  {
    
    register_ptr_to_python<API::IAlgorithm*>();

    class_< API::IAlgorithm, boost::noncopyable>("IAlgorithm", no_init)
      .def("name", &API::IAlgorithm::name)
      .def("initialize", &API::IAlgorithm::initialize)
      .def("execute", &API::IAlgorithm::execute)
      .def("executeAsync", &API::IAlgorithm::executeAsync)
      .def("isRunningAsync", &API::IAlgorithm::isRunningAsync)
      .def("isInitialized", &API::IAlgorithm::isInitialized)
      .def("isExecuted", &API::IAlgorithm::isExecuted)
      .def("setRethrows", &API::IAlgorithm::setRethrows)
      .def("setPropertyValue", &API::IAlgorithm::setPropertyValue)
      .def("getPropertyValue", &API::IAlgorithm::getPropertyValue)
      .def("getProperties", &API::IAlgorithm::getProperties, return_value_policy< copy_const_reference >())
      ;

    class_< API::Algorithm, bases<API::IAlgorithm>, boost::noncopyable>("IAlgorithm", no_init)
      ;

    class_< API::CloneableAlgorithm, bases<API::Algorithm>, boost::noncopyable>("CloneableAlgorithm", no_init)
      ;
    
    //PyAlgorithmBase
    //Save some typing for all of the templated declareProperty and getProperty methods
#define EXPORT_DECLAREPROPERTY(type, suffix)\
    .def("declareProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, type, const std::string &,const unsigned int))&PyAlgorithmBase::_declareProperty<type>) \
    .def("declareProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, type, Kernel::IValidator<type> &,const std::string &,const unsigned int))&PyAlgorithmBase::_declareProperty<type>) \
    .def("declareListProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, boost::python::list, const std::string &,const unsigned int))&PyAlgorithmBase::_declareListProperty<type>)\
    .def("declareListProperty_"#suffix,(void(PyAlgorithmBase::*)(const std::string &, boost::python::list, Kernel::IValidator<type> &,const std::string &,const unsigned int))&PyAlgorithmBase::_declareListProperty<type>)
    
#define EXPORT_GETPROPERTY(type, suffix)\
    .def("getProperty_"#suffix,(type(PyAlgorithmBase::*)(const std::string &))&PyAlgorithmBase::_getProperty<type>)

#define EXPORT_GETLISTPROPERTY(type, suffix)\
    .def("getListProperty_"#suffix,(std::vector<type>(PyAlgorithmBase::*)(const std::string &))&PyAlgorithmBase::_getListProperty<type>)
    
    class_< PyAlgorithmBase, boost::shared_ptr<PyAlgorithmCallback>, bases<API::CloneableAlgorithm>, 
      boost::noncopyable >("PyAlgorithmBase")
      .enable_pickling()
      .def("_setMatrixWorkspaceProperty", &PyAlgorithmBase::_setMatrixWorkspaceProperty)
      .def("_setTableWorkspaceProperty", &PyAlgorithmBase::_setTableWorkspaceProperty)
      .def("_declareFileProperty", &PyAlgorithmBase::_declareFileProperty)
      .def("_declareMatrixWorkspace", (void(PyAlgorithmBase::*)(const std::string &, const std::string &,const std::string &, const unsigned int))&PyAlgorithmBase::_declareMatrixWorkspace)
      .def("_declareMatrixWorkspace", (void(PyAlgorithmBase::*)(const std::string &, const std::string &,Kernel::IValidator<boost::shared_ptr<API::MatrixWorkspace> >&,const std::string &, const unsigned int))&PyAlgorithmBase::_declareMatrixWorkspace)
      .def("_declareTableWorkspace", &PyAlgorithmBase::_declareTableWorkspace)
      .def("log", &PyAlgorithmBase::getLogger, return_internal_reference<>())
      EXPORT_DECLAREPROPERTY(int, int)
      EXPORT_DECLAREPROPERTY(double, dbl)
      EXPORT_DECLAREPROPERTY(std::string, str)
      EXPORT_DECLAREPROPERTY(bool, bool)
      EXPORT_GETPROPERTY(int, int)
      EXPORT_GETLISTPROPERTY(int, int)
      EXPORT_GETPROPERTY(double, dbl)
      EXPORT_GETLISTPROPERTY(double, dbl)
      EXPORT_GETPROPERTY(bool, bool)
      ;

    //Leave the place tidy 
#undef EXPORT_DECLAREPROPERTY
#undef EXPORT_GETPROPERTY
#undef EXPORT_GETLISTPROPERTY
  }

  void export_workspace()
  {
    /// Shared pointer registration
    register_ptr_to_python<boost::shared_ptr<Workspace> >();
    
    class_<API::Workspace, boost::noncopyable>("Workspace", no_init)
      .def("getTitle", &API::Workspace::getTitle, 
         return_value_policy< copy_const_reference >())
      .def("getComment", &API::MatrixWorkspace::getComment, 
         return_value_policy< copy_const_reference >() )
      .def("getMemorySize", &API::Workspace::getMemorySize)
      .def("getName", &API::Workspace::getName, return_value_policy< copy_const_reference >())
      .def("__str__", &API::Workspace::getName, return_value_policy< copy_const_reference >())
      ;
  }

  // Overloads for binIndexOf function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_binIndexOfOverloads, API::MatrixWorkspace::binIndexOf, 1, 2)
    
  void export_matrixworkspace()
  {
    /// Shared pointer registration
    register_ptr_to_python<boost::shared_ptr<MatrixWorkspace> >();

    // A vector of MatrixWorkspace pointers
    vector_proxy<MatrixWorkspace*>::wrap("stl_vector_matrixworkspace");
 
    //Operator overloads dispatch through the above structure. The typedefs save some typing
    typedef WorkspaceAlgebraProxy::wraptype_ptr(*binary_fn1)(const WorkspaceAlgebraProxy::wraptype_ptr, const WorkspaceAlgebraProxy::wraptype_ptr);
    typedef WorkspaceAlgebraProxy::wraptype_ptr(*binary_fn2)(const WorkspaceAlgebraProxy::wraptype_ptr, double);

    /// Typedef for data access
    typedef MantidVec&(API::MatrixWorkspace::*data_modifier)(int const);

    //MatrixWorkspace class
    class_< API::MatrixWorkspace, bases<API::Workspace>, MatrixWorkspaceWrapper, 
      boost::noncopyable >("MatrixWorkspace", no_init)
      .def("getNumberHistograms", &API::MatrixWorkspace::getNumberHistograms)
      .def("getNumberBins", &API::MatrixWorkspace::blocksize)
      .def("binIndexOf", &API::MatrixWorkspace::binIndexOf, MatrixWorkspace_binIndexOfOverloads() )
      .def("readX", &PythonAPI::MatrixWorkspaceWrapper::readX)
      .def("readY", &PythonAPI::MatrixWorkspaceWrapper::readY)
      .def("readE", &PythonAPI::MatrixWorkspaceWrapper::readE)
      .def("dataX", &PythonAPI::MatrixWorkspaceWrapper::dataX)
      .def("dataY", &PythonAPI::MatrixWorkspaceWrapper::dataY)
      .def("dataE", &PythonAPI::MatrixWorkspaceWrapper::dataE)
      .def("isDistribution", (const bool& (API::MatrixWorkspace::*)() const)&API::MatrixWorkspace::isDistribution, 
         return_value_policy< copy_const_reference >() )
      .def("getInstrument", &API::MatrixWorkspace::getInstrument)
      .def("getDetector", &API::MatrixWorkspace::getDetector)
      .def("getRun", &API::MatrixWorkspace::run, return_internal_reference<>() )
      .def("getSampleInfo", &API::MatrixWorkspace::sample, return_internal_reference<>() )
      //Special methods
      .def("__add__", (binary_fn1)&WorkspaceAlgebraProxy::plus)
      .def("__add__", (binary_fn2)&WorkspaceAlgebraProxy::plus)
      .def("__radd__",(binary_fn2)&WorkspaceAlgebraProxy::rplus)
      .def("__iadd__",(binary_fn1)&WorkspaceAlgebraProxy::inplace_plus)
      .def("__iadd__",(binary_fn2)&WorkspaceAlgebraProxy::inplace_plus)
      .def("__sub__", (binary_fn1)&WorkspaceAlgebraProxy::minus)
      .def("__sub__", (binary_fn2)&WorkspaceAlgebraProxy::minus)
      .def("__rsub__",(binary_fn2)&WorkspaceAlgebraProxy::rminus)
      .def("__isub__",(binary_fn1)&WorkspaceAlgebraProxy::inplace_minus)
      .def("__isub__",(binary_fn2)&WorkspaceAlgebraProxy::inplace_minus)
      .def("__mul__", (binary_fn1)&WorkspaceAlgebraProxy::times)
      .def("__mul__", (binary_fn2)&WorkspaceAlgebraProxy::times)
      .def("__rmul__",(binary_fn2)&WorkspaceAlgebraProxy::rtimes)
      .def("__imul__",(binary_fn1)&WorkspaceAlgebraProxy::inplace_times)
      .def("__imul__",(binary_fn2)&WorkspaceAlgebraProxy::inplace_times)
      .def("__div__", (binary_fn1)&WorkspaceAlgebraProxy::divide)
      .def("__div__", (binary_fn2)&WorkspaceAlgebraProxy::divide)
      .def("__rdiv__", (binary_fn2)&WorkspaceAlgebraProxy::rdivide)
      .def("__idiv__",(binary_fn1)&WorkspaceAlgebraProxy::inplace_divide)
      .def("__idiv__",(binary_fn2)&WorkspaceAlgebraProxy::inplace_divide)
      // Deprecated, here for backwards compatability
      .def("blocksize", &API::MatrixWorkspace::blocksize)
      .def("getSampleDetails", &API::MatrixWorkspace::run, return_internal_reference<>() )
      ;
  }

  void export_tableworkspace()
  {
    // Declare the pointer
    register_ptr_to_python<API::ITableWorkspace_sptr>();
    
    // Table workspace
    // Some function pointers since MSVC can't figure out the function to call when 
    // placing this directly in the .def functions below
    typedef int&(ITableWorkspace::*get_integer_ptr)(const std::string &, int);
    typedef double&(ITableWorkspace::*get_double_ptr)(const std::string &, int);
    typedef std::string&(ITableWorkspace::*get_string_ptr)(const std::string &, int);

    // TableWorkspace class
    class_< ITableWorkspace, bases<API::Workspace>, boost::noncopyable >("ITableWorkspace", no_init)
      .def("getColumnCount", &ITableWorkspace::columnCount)
      .def("getRowCount", &ITableWorkspace::rowCount)
      .def("getColumnNames",&ITableWorkspace::getColumnNames)
      .def("getInt", (get_integer_ptr)&ITableWorkspace::getRef<int>, return_value_policy<copy_non_const_reference>())
      .def("getDouble", (get_double_ptr)&ITableWorkspace::getRef<double>, return_value_policy<copy_non_const_reference>())
      .def("getString", (get_string_ptr)&ITableWorkspace::getRef<std::string>, return_value_policy<copy_non_const_reference>())
     ;
  }

  // WorkspaceGroup
  void export_workspacegroup()
  {
    // Pointer
    register_ptr_to_python<API::WorkspaceGroup_sptr>();
    
    class_< API::WorkspaceGroup, bases<API::Workspace>, 
      boost::noncopyable >("WorkspaceGroup", no_init)
      .def("size", &API::WorkspaceGroup::getNumberOfEntries)
      .def("getNames", &API::WorkspaceGroup::getNames)
      .def("add", &API::WorkspaceGroup::add)
      .def("remove", &API::WorkspaceGroup::remove)
      ;
  }
  
  void export_sample()
  {
    //Pointer
    register_ptr_to_python<API::Sample*>();

    //Sample class
    class_< API::Sample, boost::noncopyable >("Sample", no_init)
      .def("getName", &API::Sample::getName, return_value_policy<copy_const_reference>())
      .def("getGeometryFlag", &API::Sample::getGeometryFlag)
      .def("getThickness", &API::Sample::getThickness)
      .def("getHeight", &API::Sample::getHeight)
      .def("getWidth", &API::Sample::getWidth)
     ;
  }

  void export_run()
  {
    //Pointer
    register_ptr_to_python<API::Run*>();

    //Run class
    class_< API::Run,  boost::noncopyable >("Run", no_init)
      .def("getLogData", (Kernel::Property* (API::Run::*)(const std::string&) const)&Run::getLogData, 
        return_internal_reference<>())
      .def("getLogData", (const std::vector<Kernel::Property*> & (API::Run::*)() const)&Run::getLogData, 
        return_internal_reference<>())
      .def("getProtonCharge", &API::Run::getProtonCharge)
      .def("hasProperty", &API::Run::hasProperty)
      .def("getProperty", &API::Run::getProperty, return_value_policy<return_by_value>())
      .def("getProperties", &API::Run::getProperties, return_internal_reference<>())
     ;
  }

  void export_instrument()
  {
    //Pointer to the interface
    register_ptr_to_python<boost::shared_ptr<API::IInstrument> >();
    
    //IInstrument class
    class_< API::IInstrument, boost::python::bases<Geometry::ICompAssembly>, 
      boost::noncopyable>("IInstrument", no_init)
      .def("getSample", &API::IInstrument::getSample)
      .def("getSource", &API::IInstrument::getSource)
      .def("getComponentByName", &API::IInstrument::getComponentByName)
      ;

    /** Concrete implementations so that Python knows about them */
    
    //Instrument class
    class_< API::Instrument, boost::python::bases<API::IInstrument>, 
	    boost::noncopyable>("Instrument", no_init)
      ;
    //Instrument class
    class_< API::ParInstrument, boost::python::bases<API::IInstrument>, 
	    boost::noncopyable>("ParInstrument", no_init)
      ;
  }

  void export_workspace_property()
  {
    // Tell python about this so I can check if a property is a workspace
    class_< WorkspaceProperty<Workspace>, bases<Kernel::Property>, boost::noncopyable>("WorkspaceProperty", no_init)
      ;
    // Tell python about a MatrixWorkspace property
    class_< WorkspaceProperty<MatrixWorkspace>, bases<Kernel::Property>, boost::noncopyable>("MatrixWorkspaceProperty", no_init)
      ;
    // Tell python about a TableWorkspace property
    class_< WorkspaceProperty<ITableWorkspace>, bases<Kernel::Property>, boost::noncopyable>("TableWorkspaceProperty", no_init)
      ;
    // Tell python about an EventWorkspace
    class_< WorkspaceProperty<IEventWorkspace>, bases<Kernel::Property>, boost::noncopyable>("EventWorkspaceProperty", no_init)
      ;

  }
  
  void export_fileproperty()
  {
    //FileProperty enum
    enum_<FileProperty::FileAction>("FileAction")
      .value("Save", FileProperty::Save)
      .value("OptionalSave", FileProperty::OptionalSave)
      .value("Load", FileProperty::Load)
      .value("OptionalLoad", FileProperty::OptionalLoad)
      ;
  }

 void export_workspacefactory()
  {
    class_< PythonAPI::WorkspaceFactoryProxy, boost::noncopyable>("WorkspaceFactoryProxy", no_init)
      .def("createMatrixWorkspace", &PythonAPI::WorkspaceFactoryProxy::createMatrixWorkspace)
      .staticmethod("createMatrixWorkspace")
      .def("createMatrixWorkspaceFromTemplate",&PythonAPI::WorkspaceFactoryProxy::createMatrixWorkspaceFromTemplate)
      .staticmethod("createMatrixWorkspaceFromTemplate")
      ;
  }

  void export_apivalidators()
  {
    class_<Kernel::IValidator<API::MatrixWorkspace_sptr>, boost::noncopyable>("IValidator_matrix", no_init)
      ;

    // Unit checking
    class_<API::WorkspaceUnitValidator<API::MatrixWorkspace>, 
      bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("WorkspaceUnitValidator", init<std::string>())
      ;
    // Histogram checking
    class_<API::HistogramValidator<API::MatrixWorkspace>, 
      bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("HistogramValidator", init<bool>())
      ;
    // Raw count checker
    class_<API::RawCountValidator<API::MatrixWorkspace>, 
	   bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("RawCountValidator", init<bool>())
      ;
    // Check for common bins
    class_<API::CommonBinsValidator<API::MatrixWorkspace>, 
	   bases<Kernel::IValidator<API::MatrixWorkspace_sptr> > >("CommonBinsValidator")
      ;
	
  }

  void export_file_finder()
  {
    class_<PythonAPI::FileFinderWrapper, boost::noncopyable>("FileFinder", no_init)
      .def("getFullPath", &PythonAPI::FileFinderWrapper::getFullPath)
      .staticmethod("getFullPath")
      .def("findRuns", &PythonAPI::FileFinderWrapper::findRuns)
      .staticmethod("findRuns")
      ;
  }

  void export_api_namespace()
  {
    export_frameworkmanager();
    export_ialgorithm();
    export_workspace();
    export_matrixworkspace();
    export_tableworkspace();
    export_workspacegroup();
    export_sample();
    export_run();
    export_instrument();
    export_workspace_property();
    export_fileproperty();
    export_workspacefactory();
    export_apivalidators();
    export_file_finder();
  }
  //@endcond
}
}
