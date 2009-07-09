#ifndef MANTID_GEOMETRY_IDETECTOR_H_
#define MANTID_GEOMETRY_IDETECTOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IComponent.h"
#include <boost/shared_ptr.hpp>
#include <stdexcept>

namespace Mantid
{
namespace Geometry
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class V3D;

/** Interface class for detector objects.

    @author Russell Taylor, Tessella Support Services plc
    @date 08/04/2008

    Copyright &copy; 2008-9 STFC Rutherford Appleton Laboratory

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
class DLLExport IDetector//: public virtual IComponent
{
public:
  /// Get the detector ID
  virtual int getID() const = 0;

  /// Get the absolute position of this detector
  virtual V3D getPos() const = 0;

  /** Get the distance of this detector object from another Component
   *  @param comp The component to give the distance to
   *  @return The distance
   */
  virtual double getDistance(const IComponent& comp) const = 0;

  /** Gives the angle of this detector object with respect to an axis
   *  @param observer The point to calculate the angle relative to (typically the sample position)
   *  @param axis     The axis to which the required angle is relative
   *  @return The angle in radians
   */
  virtual double getTwoTheta(const V3D& observer, const V3D& axis) const = 0;

  /// Gives the phi of this detector object in radians
  virtual double getPhi() const = 0;
  
  /** Gives the approximate angle subtended by the detector
   *  @param observer The point from which the detector is being viewed
   *  @return The solid angle in steradians
   *  @throw NullPointerException If geometrical form of the detector has not been provided in the instrument definition file
   */
  virtual double solidAngle(const V3D& observer) const = 0;

  /// Indicates whether the detector has been masked
  virtual bool isMasked() const = 0;

  /// Indicates whether this is a monitor detector
  virtual bool isMonitor() const = 0;

  /// Must return a pointer to itself if derived from IComponent
  virtual IComponent* getComponent(){throw std::runtime_error("This detecor class does not inherit from IComponent.");}

  /// (Empty) Constructor
	IDetector() {}
	/// Virtual destructor
	virtual ~IDetector() {}
};

/// Shared pointer to IDetector
typedef boost::shared_ptr<Mantid::Geometry::IDetector> IDetector_sptr;
/// Shared pointer to IDetector (const version)
typedef boost::shared_ptr<const Mantid::Geometry::IDetector> IDetector_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_IDETECTOR_H_*/
