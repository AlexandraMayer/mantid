#include "MantidMDAlgorithms/BoxInterpreter.h"
#include "MantidMDAlgorithms/Box3DImplicitFunction.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"

namespace Mantid
{
namespace MDAlgorithms
{

boxVector BoxInterpreter::walkTree(CompositeImplicitFunction* compFunc) const
{
  using namespace Mantid::API;
  boxVector flattenedboxes;
  functionVector nestedFuncs = compFunc->getFunctions();
  for (unsigned int i = 0; i < nestedFuncs.size(); i++)
  {
    if (CompositeImplicitFunction::functionName()  == nestedFuncs[i]->getName())
    {
      CompositeImplicitFunction* compFunc =
        dynamic_cast<Mantid::MDAlgorithms::CompositeImplicitFunction*> (nestedFuncs[i].get());

      boxVector boxes = walkTree(compFunc); //recursive walk
      flattenedboxes.insert(flattenedboxes.end(), boxes.begin(), boxes.end());
    }
    else if (BoxImplicitFunction::functionName()  == nestedFuncs[i]->getName())
    {
      boost::shared_ptr<BoxImplicitFunction> spBox =
             boost::dynamic_pointer_cast<BoxImplicitFunction>(nestedFuncs[i]);

      flattenedboxes.push_back(spBox);
    }
  }
  return flattenedboxes;

}

std::vector<double> BoxInterpreter::operator()(Mantid::Geometry::MDImplicitFunction* implicitFunction) const
{
  return Execute(implicitFunction);
}

std::vector<double> BoxInterpreter::Execute(Mantid::Geometry::MDImplicitFunction* implicitFunction) const
{
  std::vector<double> endBox(6, 0);
  Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction =
      dynamic_cast<Mantid::MDAlgorithms::CompositeImplicitFunction*> (implicitFunction);

  boxVector flattenedboxes;
  if (compFunction != NULL)
  {
    //Flatten out box functions
    flattenedboxes = walkTree(compFunction);

    boost::shared_ptr<BoxImplicitFunction> box = flattenedboxes[0];
    double minX = box->getLowerX();
    double minY = box->getLowerY();
    double minZ = box->getLowerZ();
    double maxX = box->getUpperX();
    double maxY = box->getUpperY();
    double maxZ = box->getUpperZ();
    for (unsigned int i = 0; i < flattenedboxes.size(); i++)
    {
      box = flattenedboxes[i];
      minX = minX > box->getLowerX() ? minX : box->getLowerX();
      minY = minY > box->getLowerY() ? minY : box->getLowerY();
      minZ = minZ > box->getLowerZ() ? minZ : box->getLowerZ();
      maxX = maxX < box->getUpperX() ? maxX : box->getUpperX();
      maxY = maxY < box->getUpperY() ? maxY : box->getUpperY();
      maxZ = maxZ < box->getUpperZ() ? maxZ : box->getUpperZ();
    }

    endBox[0] = minX;
    endBox[1] = maxX;
    endBox[2] = minY;
    endBox[3] = maxY;
    endBox[4] = minZ;
    endBox[5] = maxZ;
  }

  return endBox;
}


boxVector BoxInterpreter::getAllBoxes(Mantid::Geometry::MDImplicitFunction* implicitFunction) const
{
  Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction =
      dynamic_cast<Mantid::MDAlgorithms::CompositeImplicitFunction*> (implicitFunction);

  boxVector flattenedboxes;
  if (compFunction != NULL)
  {
    //Flatten out box functions
    flattenedboxes = walkTree(compFunction);
  }
  return flattenedboxes;
}

}
}
