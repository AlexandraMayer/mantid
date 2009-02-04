#ifndef MANTID_GEOMETRY_OBJECT_H_
#define MANTID_GEOMETRY_OBJECT_H_

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Rules.h"
#include "MantidGeometry/Surface.h"
#include "MantidGeometry/Track.h"
#include "MantidGeometry/Quat.h"
#include "boost/shared_ptr.hpp"


namespace Mantid
{
namespace Geometry
{
/*!
  \class Object
  \brief Global object for object
  \version 1.0
  \date July 2007
  \author S. Ansell

  An object is a collection of Rules and
  surface object

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
	class GeometryHandler;
class DLLExport Object
{
 private:

  static Kernel::Logger& PLog;           ///< The official logger

  int ObjName;       ///< Creation number
  int MatN;          ///< Material Number
  double Tmp;        ///< Temperature (K)
  double density;    ///< Density

  Rule* TopRule;     ///< Top rule [ Geometric scope of object]

  int procPair(std::string& Ln,std::map<int,Rule*>& Rlist,int& compUnit) const;
  CompGrp* procComp(Rule*) const;
  int checkSurfaceValid(const Geometry::V3D&,const Geometry::V3D&) const;
  mutable double AABBxMax, ///< xmax of Asis aligned bounding box cache
	             AABByMax, ///< ymax of Asis aligned bounding box cache
				 AABBzMax, ///< zmax of Asis aligned bounding box cache
				 AABBxMin, ///< xmin of Asis aligned bounding box cache
				 AABByMin, ///< xmin of Asis aligned bounding box cache
				 AABBzMin; ///< zmin of Axis Aligned Bounding Box Cache
  mutable bool  boolBounded; ///< flag true if a bounding box exists, either by getBoundingBox or defineBoundingBox
  int searchForObject(Geometry::V3D&) const;
  int inBoundingBox(const Geometry::V3D&,
	                const double&, const double&, const double&,
	                const double&, const double&, const double& ) const;
  int lineHitsBoundingBox(const Geometry::V3D&, const Geometry::V3D&,
	                              const double&, const double&, const double&,
	                              const double&, const double&, const double& ) const;
  double bbAngularWidth(const Geometry::V3D&,
	                         const double&, const double&, const double&,
	                         const double&, const double&, const double& ) const;
  double getTriangleSolidAngle(const V3D& a, const V3D& b, const V3D& c, const V3D& observer) const;
  /// Geometry Handle for rendering
  boost::shared_ptr<GeometryHandler> handle;
  friend class GeometryHandler;
  /// for solid angle from triangulation
  int NumberOfTriangles() const;
  int NumberOfPoints() const;
  int* getTriangleFaces() const;
  double* getTriangleVertices() const;


 protected:

  std::vector<const Surface*> SurList;  ///< Full surfaces (make a map including complementary object ?)


 public:

  Object();
  Object(const Object&);
  Object& operator=(const Object&);
  virtual ~Object();

  /// Return the top rule
  const Rule* topRule() const { return TopRule; }

  void setName(const int nx) { ObjName=nx; }           ///< Set Name
  void setTemp(const double A) { Tmp=A; }              ///< Set temperature [Kelvin]
  int setObject(const int ON,const std::string& Ln);
  int procString(const std::string& Line);
  void setMaterial(const int MatIndex) { MatN=MatIndex; }  ///< Set Material index
  void setDensity(const double D) { density=D; }       ///< Set Density [Atom/A^3]

  int complementaryObject(const int Cnum,std::string& Ln);     ///< Process a complementary object
  int hasComplement() const;

  int getName() const  { return ObjName; }             ///< Get Name
  int getMat() const { return MatN; }                  ///< Get Material ID
  double getTemp() const { return Tmp; }               ///< Get Temperature [K]
  double getDensity() const { return density; }        ///< Get Density [Atom/A^3]

  int populate(const std::map<int,Surface*>&);
  int createSurfaceList(const int outFlag=0);               ///< create Surface list
  int addSurfString(const std::string&);     ///< Not implemented
  int removeSurface(const int SurfN);
  int substituteSurf(const int SurfN,const int NsurfN,Surface* SPtr);
  void makeComplement();
  void convertComplement(const std::map<int,Object>&);

  virtual void print() const;
  void printTree() const;

  int isValid(const Geometry::V3D&) const;    ///< Check if a point is valid
  int isValid(const std::map<int,int>&) const;  ///< Check if a set of surfaces are valid.
  int isOnSide(const Geometry::V3D&) const;
  int calcValidType(const Geometry::V3D& Pt,const Geometry::V3D& uVec) const;

  std::vector<int> getSurfaceIndex() const;
  /// Get the list of surfaces (const version)
  const std::vector<const Surface*>& getSurfacePtr() const
    { return SurList; }
  /// Get the list of surfaces
  std::vector<const Surface*>& getSurfacePtr()
    { return SurList; }

  std::string cellCompStr() const;
  std::string cellStr(const std::map<int,Object>&) const;

  std::string str() const;
  void write(std::ostream&) const;     ///< MCNPX output

  // INTERSECTION
  int interceptSurface(Geometry::Track&) const;

  // Solid angle - uses triangleSolidAngle unless many (>30000) triangles
  double solidAngle(const Geometry::V3D& observer) const;
  // solid angle via triangulation
  double triangleSolidAngle(const Geometry::V3D& observer) const;
  // solid angle via ray tracing
  double rayTraceSolidAngle(const Geometry::V3D& observer) const;

  // Calculate (or return cached value of) Axis Aligned Bounding box
  void getBoundingBox(double& xmax,double& ymax,double& zmax,double& xmin,double& ymin,double& zmin) const;

  // Define Axis Aligned Bounding box
  void defineBoundingBox(const double& xmax,const double& ymax,const double& zmax,const double& xmin,const double& ymin,const double& zmin);

  // find internal point to object
  int getPointInObject(Geometry::V3D& point) const;

  //Rendering member functions
  void draw() const;
  //Initialize Drawing
  void initDraw() const;
  //Get Geometry Handler
  boost::shared_ptr<GeometryHandler> getGeometryHandler(){return handle;}
  void setGeometryHandler(boost::shared_ptr<GeometryHandler> h);
};

}  // NAMESPACE Geometry
}  // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_OBJECT_H_*/
