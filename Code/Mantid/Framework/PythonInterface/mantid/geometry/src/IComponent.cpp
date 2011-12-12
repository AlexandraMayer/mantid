#include "MantidGeometry/IComponent.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IComponent;
using namespace boost::python;

void export_IComponent()
{
  register_ptr_to_python<boost::shared_ptr<IComponent> >();

  class_<IComponent, boost::noncopyable>("IComponent", no_init)
    .def("get_pos", &IComponent::getPos, "Returns the absolute position of the component")
    .def("get_distance", &IComponent::getDistance, "Returns the distance, in metres, between this and the given component")
    .def("get_name", &IComponent::getName, "Returns the name of the component")
    .def("type", &IComponent::type, "Returns the type of the component represented as a string")
    ;
  

}

