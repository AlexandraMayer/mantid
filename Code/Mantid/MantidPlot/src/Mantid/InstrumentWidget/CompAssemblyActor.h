#ifndef COMPASSEMBLY_ACTOR__H_
#define COMPASSEMBLY_ACTOR__H_
#include "GLActor.h"
#include "ICompAssemblyActor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"
/*!
  \class  CompAssemblyActor
  \brief  This class wraps the ICompAssembly into Actor.
  \author Srikanth Nagella
  \date   March 2009
  \version 1.0

  This class has the implementation for calling the children of ICompAssembly's IObjComponent to render themselves
  and call the ICompAssemblys. This maintains the count of the children for easy lookup.

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
namespace Mantid
{

namespace Geometry
{
  class IInstrument;
  class ICompAssembly;
  class Object;
  class V3D;
}

}

class MantidObject;
class ObjComponentActor;

class CompAssemblyActor : public ICompAssemblyActor
{
private:
  void AppendBoundingBox(const Mantid::Geometry::V3D& minBound,const Mantid::Geometry::V3D& maxBound);
protected:
  std::vector<ObjComponentActor*> mChildObjCompActors;     ///< List of ObjComponent Actors
  std::vector<ICompAssemblyActor*> mChildCompAssemActors;   ///< List of CompAssembly Actors
  void initChilds(bool);
  void init();
  void redraw();
  void appendObjCompID(std::vector<int>&);
  MantidObject*	getMantidObject(const boost::shared_ptr<const Mantid::Geometry::Object>,bool withDisplayList);
  int setInternalDetectorColors(std::vector<boost::shared_ptr<GLColor> >::iterator& list);
  int findDetectorIDUsingColor(int rgb);
public:
  CompAssemblyActor(bool withDisplayList);                       ///< Constructor
  CompAssemblyActor(boost::shared_ptr<std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*> >& ,  Mantid::Geometry::ComponentID id, boost::shared_ptr<Mantid::Geometry::IInstrument> ins,bool withDisplayList); ///< Constructor
  virtual ~CompAssemblyActor();								   ///< Destructor
  int  setStartingReferenceColor(int rgb);
  virtual std::string type()const {return "CompAssemblyActor";} ///< Type of the GL object
  void define();  ///< Method that defines ObjComponent geometry. Calls ObjComponent draw method
  void drawUsingColorID();
  //void addToUnwrappedList(UnwrappedCylinder& cylinder, QList<UnwrappedDetectorCyl>& list);
  void detectorCallback(DetectorCallback* callback)const;
};

#endif /*GLTRIANGLE_H_*/

