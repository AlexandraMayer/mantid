#ifndef MANTID_GEOMETRY_OBJCOMPONENT_H_
#define MANTID_GEOMETRY_OBJCOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/Component.h"
#include "MantidGeometry/Track.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{
namespace Geometry
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class Object;

/** Object Component class, this class brings together the physical attributes of the component
    to the positioning and geometry tree.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
    @author Russell Taylor, Tessella Support Services plc
    @date 26/06/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
class DLLExport ObjComponent : public Component
{
public:
  ///type string
  virtual std::string type() {return "PhysicalComponent";}

  // Looking to get rid of the first of these constructors in due course (and probably add others)
  explicit ObjComponent(const std::string& name, Component* parent=0);
  explicit ObjComponent(const std::string& name, boost::shared_ptr<Object> shape, Component* parent=0);
  virtual ~ObjComponent();

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  virtual Component* clone() const {return new ObjComponent(*this);}

  bool isValid(const V3D& point) const;
  bool isOnSide(const V3D& point) const;
  int interceptSurface(Track& track) const;
  double solidAngle(const V3D& observer) const;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const;
  int getPointInObject(V3D& point) const;

protected:
  ObjComponent(const ObjComponent&);

private:
  /// Private, unimplemented copy assignment operator
  ObjComponent& operator=(const ObjComponent&);

  const V3D factorOutComponentPosition(const V3D& point) const;
  const V3D takeOutRotation(V3D point) const;

  /// The phyical geometry representation
  // Made a const pointer to a const object. Since this is a shared object we shouldn't be
  // exposing non-const methods of Object through this class.
  const boost::shared_ptr<const Object> shape;
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_OBJCOMPONENT_H_*/
