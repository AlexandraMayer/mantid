#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace Geometry {
namespace {
// static logger object
Kernel::Logger g_log("Detector");
}

using Kernel::V3D;
using Kernel::Quat;

/** Constructor for a parametrized Detector
 * @param base: the base (un-parametrized) IComponent
 * @param map: pointer to the ParameterMap
 * */
Detector::Detector(const Detector *base, const ParameterMap *map)
    : ObjComponent(base, map), m_id(base->m_id) {}

/** Constructor
 *  @param name :: The name of the component
 *  @param id :: Index for the component
 *  @param parent :: The parent component
 */
Detector::Detector(const std::string &name, int id, IComponent *parent)
    : IDetector(), ObjComponent(name, parent), m_id(id) {}

/** Constructor
 *  @param name :: The name of the component
 *  @param id :: Index for the component
 *  @param shape ::  A pointer to the object describing the shape of this
 * component
 *  @param parent :: The parent component
 */
Detector::Detector(const std::string &name, int id,
                   boost::shared_ptr<Object> shape, IComponent *parent)
    : IDetector(), ObjComponent(name, shape, parent), m_id(id) {}

/** Gets the detector id
 *  @returns the detector id
 */
detid_t Detector::getID() const {
  if (m_map) {
    const Detector *d = dynamic_cast<const Detector *>(m_base);
    if (d) {
      return d->getID();
    }
  }

  return m_id;
}

/// Get the distance between the detector and another component
///@param comp :: The other component
///@return The distance
double Detector::getDistance(const IComponent &comp) const {
  return ObjComponent::getDistance(comp);
}

/// Get the twotheta angle between the detector and an observer
///@param observer :: The observer position
///@param axis :: The axis
///@return The angle
double Detector::getTwoTheta(const V3D &observer, const V3D &axis) const {
  const V3D sampleDetVec = this->getPos() - observer;
  return sampleDetVec.angle(axis);
}

/**
Get the two theata angle signed according the quadrant
@param observer :: The observer position
@param axis :: The axis
@param instrumentUp :: instrument up direction.
@return The angle
*/
double Detector::getSignedTwoTheta(const V3D &observer, const V3D &axis,
                                   const V3D &instrumentUp) const {
  const V3D sampleDetVec = this->getPos() - observer;
  double angle = sampleDetVec.angle(axis);

  V3D cross = axis.cross_prod(sampleDetVec);
  V3D normToSurface = axis.cross_prod(instrumentUp);
  if (normToSurface.scalar_prod(cross) < 0) {
    angle *= -1;
  }
  return angle;
}

/// Get the phi angle between the detector with reference to the origin
///@return The angle
double Detector::getPhi() const {
  double phi = 0.0, dummy;
  this->getPos().getSpherical(dummy, dummy, phi);
  return phi * M_PI / 180.0;
}
/**
 * Calculate the phi angle between detector and beam, and then offset.
 * @param offset in radians
 * @return offset phi angle in radians.
 */
double Detector::getPhiOffset(const double &offset) const {
  double avgPos = getPhi();
  double phiOut = avgPos;
  if (avgPos < 0) {
    phiOut = -(offset + avgPos);
  } else {
    phiOut = offset - avgPos;
  }
  return phiOut;
}

/** returns the detector's topology, namely, the meaning of the detector's
* angular measurements.
*     It is different in cartesian and cylindrical (surrounding the beam)
* coordinate system
*/
det_topology Detector::getTopology(V3D &center) const {
  center = this->getPos();
  return rect;
}

/// Helper for legacy access mode. Returns a reference to the ParameterMap.
const ParameterMap &Detector::parameterMap() const { return *m_map; }

/// Helper for legacy access mode. Returns the index of the detector.
size_t Detector::index() const { return m_index; }

/// Helper for legacy access mode. Sets the index of the detector.
void Detector::setIndex(const size_t index) { m_index = index; }

} // Namespace Geometry
} // Namespace Mantid
