#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>

using Mantid::API::IPeaksWorkspace;
using Mantid::API::IPeaksWorkspace_sptr;
using Mantid::API::IPeak;
using Mantid::API::ExperimentInfo;
using Mantid::API::ITableWorkspace;

using namespace boost::python;

void export_IPeaksWorkspace()
{
  register_ptr_to_python<IPeaksWorkspace_sptr>();

  // IPeaksWorkspace class
  class_< IPeaksWorkspace, bases<ITableWorkspace, ExperimentInfo>, boost::noncopyable >("IPeaksWorkspace", no_init)
    .def("getNumberPeaks", &IPeaksWorkspace::getNumberPeaks, "Returns the number of peaks within the workspace")
    .def("addPeak", &IPeaksWorkspace::addPeak, "Add a peak to the workspace")
    .def("removePeak", &IPeaksWorkspace::removePeak, "Remove a peak from the workspace")
    .def("getPeak", &IPeaksWorkspace::getPeakPtr, return_internal_reference<>(), "Returns a peak at the given index" )
    .def("createPeak", &IPeaksWorkspace::createPeak, return_internal_reference<>(), "Create a Peak and return it")
      ;

  DECLARE_SINGLEVALUETYPEHANDLER(IPeaksWorkspace, Mantid::Kernel::DataItem_sptr);

}

