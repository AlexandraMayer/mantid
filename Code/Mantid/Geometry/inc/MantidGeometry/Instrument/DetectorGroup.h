#ifndef MANTID_GEOMETRY_DETECTORGROUP_H_
#define MANTID_GEOMETRY_DETECTORGROUP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>

namespace Mantid
{
namespace Geometry
{
/** Holds a collection of detectors.
    Responds to IDetector methods as though it were a single detector.
    Currently, detectors in a group are treated as pointlike (or at least)
    homogenous entities. This means that it's up to the use to make
    only sensible groupings of similar detectors since no weighting according
    to solid angle size takes place and the DetectorGroup's position is just
    a simple average of its constituents.

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
class DLLExport DetectorGroup : public virtual IDetector
{
public:
  DetectorGroup(const std::vector<IDetector_sptr>& dets);
  virtual ~DetectorGroup();

  void addDetector(IDetector_sptr det);

  // IDetector methods
  int getID() const;
  V3D getPos() const;
	double getDistance(const IComponent& comp) const;
  double getTwoTheta(const V3D& observer, const V3D& axis) const;
  double getPhi() const;
  double solidAngle(const V3D& observer) const; 
	bool isMasked() const;
	bool isMonitor() const;
  bool isValid(const V3D& point) const;
  virtual bool isOnSide(const V3D& point) const;
  ///Try to find a point that lies within (or on) the object
  int getPointInObject(V3D& point) const;


private:
  /// Private, unimplemented copy constructor
  DetectorGroup(const DetectorGroup&);
  /// Private, unimplemented copy assignment operator
  DetectorGroup& operator=(const DetectorGroup&);

  /// The ID of this effective detector
  int m_id;
  /// The type of collection used for the detectors
  ///          - a map of detector pointers with the detector ID as the key
  // May want to change this to a hash_map in due course
  typedef std::map<int, IDetector_sptr> DetCollection;
  /// The collection of grouped detectors
  DetCollection m_detectors;

  /// required to for the function getRelativeRot() in IComponent interface, left at the default value
  const Quat m_unImplementedForInterface;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
  
	// functions inherited from IComponent
  Component* clone() const{ return NULL; }
  const ComponentID getComponentID(void) const{ return NULL; }
  boost::shared_ptr<const IComponent> getParent() const
  {
    return boost::shared_ptr<const IComponent>();
  }
  std::string getName() const{return "";}
  void setParent(IComponent*){}
  void setName(const std::string&){}

  void setPos(double, double, double){}
  void setPos(const V3D&){}
  void setRot(const Quat&){}
  void copyRot(const IComponent&){}
  int interceptSurface(Track&) const{ return -10; }
  void translate(const V3D&){}
  void translate(double, double, double){}
  void rotate(const Quat&){}
  void rotate(double,const V3D&){}
  V3D getRelativePos() const{ return V3D(); }
  const Quat& getRelativeRot() const{ return m_unImplementedForInterface; }
  const Quat getRotation() const{ return Quat(); }
  void printSelf(std::ostream&) const{}

  // functions inherited from IObjComponent

  void getBoundingBox(double &, double &, double &, double &, double &, double &) const{};

  void draw() const{};
  void drawObject() const{};
  void initDraw() const{};

  /// Returns the shape of the Object
  const boost::shared_ptr<const Object> Shape() const
  {
    return boost::shared_ptr<const Object>();
  }

  void setScaleFactor(double,double, double);
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_DETECTORGROUP_H_*/
