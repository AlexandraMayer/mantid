#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
//#include "MantidGeometry/Instrument/ParObjCompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidObject.h"
#include "CompAssemblyActor.h"
#include "ObjComponentActor.h"
#include "ObjCompAssemblyActor.h"
#include "RectangularDetectorActor.h"
#include <cfloat>

using Mantid::Geometry::IInstrument;
using Mantid::Geometry::IComponent;
using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::ObjCompAssembly;
//using Mantid::Geometry::ParObjCompAssembly;
using Mantid::Geometry::ComponentID;
using Mantid::Geometry::RectangularDetector;
using Mantid::Geometry::RectangularDetector;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::Object;


/**
 * This is default constructor for CompAssembly Actor
 * @param withDisplayList :: true to create a display list for the compassembly and its subcomponents
 */
CompAssemblyActor::CompAssemblyActor(bool withDisplayList):
    ICompAssemblyActor(withDisplayList)
{
}

/**
 * This is a constructor for CompAssembly Actor
 * @param objs            :: list of objects that are used by IObjCompenent actors and will be filled with the new objects
 * @param id              :: ComponentID of this object of CompAssembly
 * @param ins             :: Instrument
 * @param withDisplayList :: true to create a display list for the compassembly and its subcomponents
 */
CompAssemblyActor::CompAssemblyActor(
    boost::shared_ptr<std::map<const boost::shared_ptr<const Object>,MantidObject*> >& objs,
    ComponentID id,boost::shared_ptr<IInstrument> ins,
    bool withDisplayList):
    ICompAssemblyActor(withDisplayList)
{
  // Initialises
  mId=id;
  mInstrument=ins;
  mObjects=objs;
  this->setName( ins->getName() );
  //Create the subcomponent actors
  initChilds(withDisplayList);
}

/**
 * Destructor which removes the actors created by this object
 */
CompAssemblyActor::~CompAssemblyActor()
{
  //Remove all the child CompAssembly Actors
  for(std::vector<ICompAssemblyActor*>::iterator iAssem=mChildCompAssemActors.begin();iAssem!=mChildCompAssemActors.end();iAssem++)
    delete (*iAssem);
  mChildCompAssemActors.clear();
  //Remove all the child ObjComponent Actors
  for(std::vector<ObjComponentActor*>::iterator iObjComp=mChildObjCompActors.begin();iObjComp!=mChildObjCompActors.end();iObjComp++)
    delete (*iObjComp);
  mChildObjCompActors.clear();
}

/**
 * This function is concrete implementation that renders the Child ObjComponents and Child CompAssembly's
 */
void CompAssemblyActor::define()
{
  mColor->paint(GLColor::MATERIAL);
  mColor->paint(GLColor::PLAIN);
  //Only draw the CompAssembly Children only if they are visible
  if(mVisible)
  {
    //Iterate through the ObjCompActor children and draw them
    for(std::vector<ObjComponentActor*>::iterator itrObjComp=mChildObjCompActors.begin();itrObjComp!=mChildObjCompActors.end();itrObjComp++)
    {
      (*itrObjComp)->getColor()->paint(GLColor::MATERIAL);
      (*itrObjComp)->getColor()->paint(GLColor::PLAIN);
      //Only draw the ObjCompActor if its visible
      if((*itrObjComp)->getVisibility())
      {
        //std::cout << (*itrObjComp)->getName() << " is gonna draw. From define()\n";
        (*itrObjComp)->draw();
      }
      else
      {
        //std::cout << (*itrObjComp)->getName() << " is not visible\n";
      }
    }
    //Iterate through the CompAssemblyActor children and draw them
    for(std::vector<ICompAssemblyActor*>::iterator itrObjAssem=mChildCompAssemActors.begin();itrObjAssem!=mChildCompAssemActors.end();itrObjAssem++)
    {
      //std::cout << (*itrObjAssem)->getName() << " is gonna draw. From define()\n";
      (*itrObjAssem)->draw();
    }
  }
  else
  {
    //std::cout << this->getName() << " is not visible\n";
  }
}

//------------------------------------------------------------------------------------------------
/**
 * This method draws the children with the ColorID rather than the children actual color. used in picking the component
 */
void CompAssemblyActor::drawUsingColorID()
{
  //Check whether this assembly and its child components can be drawn
  if(mVisible)
  {
    //Iterate throught the children with the starting reference color mColorStartID and incrementing
    int rgb=mColorStartID;
    int r,g,b;
    for(std::vector<ObjComponentActor*>::iterator itrObjComp=mChildObjCompActors.begin();itrObjComp!=mChildObjCompActors.end();itrObjComp++)
    {
      r=(rgb/65536);
      g=((rgb%65536)/256);
      b=((rgb%65536)%256);
      glColor3f(r/255.0,g/255.0,b/255.0);
      if((*itrObjComp)->getVisibility())
        (*itrObjComp)->draw();
      rgb++;
    }
    for(std::vector<ICompAssemblyActor*>::iterator itrObjAssem=mChildCompAssemActors.begin();itrObjAssem!=mChildCompAssemActors.end();itrObjAssem++)
    {
      (*itrObjAssem)->drawUsingColorID();
    }
  }
}

//------------------------------------------------------------------------------------------------
/**
 * Initialises the CompAssembly Children and creates actors for each children
 * @param withDisplayList :: the new actors for the children with same display list attribute as parent
 */
void CompAssemblyActor::initChilds(bool withDisplayList)
{
  boost::shared_ptr<IComponent> CompPtr;
  if(mId == mInstrument->getComponentID())
    CompPtr=mInstrument;
  else
    CompPtr=mInstrument->getComponentByID(mId);
  //bounding box of the overall instrument
  Mantid::Geometry::V3D minBound;
  Mantid::Geometry::V3D maxBound;
  //Iterate through CompAssembly children
  boost::shared_ptr<ICompAssembly> CompAssemPtr=boost::dynamic_pointer_cast<ICompAssembly>(CompPtr);
  if(CompAssemPtr!=boost::shared_ptr<ICompAssembly>())
  {
    int nChild=CompAssemPtr->nelements();
    for(int i=0;i<nChild;i++)
    {
      boost::shared_ptr<IComponent> ChildCompPtr=(*CompAssemPtr)[i];
      boost::shared_ptr<ICompAssembly> ChildCAPtr=boost::dynamic_pointer_cast<ICompAssembly>(ChildCompPtr);

      //If the child is a CompAssembly then create a CompAssemblyActor for the child
      if(ChildCAPtr!=boost::shared_ptr<ICompAssembly>())
      {
        boost::shared_ptr<ObjCompAssembly> ChildOCAPtr=boost::dynamic_pointer_cast<ObjCompAssembly>(ChildCompPtr);
        boost::shared_ptr<RectangularDetector> ChildRDPtr=boost::dynamic_pointer_cast<RectangularDetector>(ChildCompPtr);

        if (ChildRDPtr)
        {
          //If the child is a RectangularDetector, then create a RectangularDetectorActor for it.
          RectangularDetectorActor* iActor = new RectangularDetectorActor(ChildRDPtr);
          iActor->getBoundingBox(minBound,maxBound);
          AppendBoundingBox(minBound,maxBound);
          mNumberOfDetectors+=iActor->getNumberOfDetectors();
          mChildCompAssemActors.push_back(iActor);
        }
        else if (ChildOCAPtr)
        {
          ObjCompAssemblyActor* iActor =
              new ObjCompAssemblyActor(mObjects,ChildCAPtr->getComponentID(),mInstrument,withDisplayList);
          iActor->getBoundingBox(minBound,maxBound);
          AppendBoundingBox(minBound,maxBound);
          mNumberOfDetectors+=iActor->getNumberOfDetectors();
          mChildCompAssemActors.push_back(iActor);
        }
        else
        {
          CompAssemblyActor* iActor=new CompAssemblyActor(mObjects,ChildCAPtr->getComponentID(),mInstrument,withDisplayList);
          iActor->getBoundingBox(minBound,maxBound);
          AppendBoundingBox(minBound,maxBound);
          mNumberOfDetectors+=iActor->getNumberOfDetectors();
          mChildCompAssemActors.push_back(iActor);
        }
      }
      else //it has to be a ObjComponent child, create a ObjComponentActor for the child use the same display list attribute
      {
        boost::shared_ptr<Mantid::Geometry::IObjComponent> ChildObjPtr = boost::dynamic_pointer_cast<Mantid::Geometry::IObjComponent>(ChildCompPtr);
        ObjComponentActor* iActor = new ObjComponentActor(getMantidObject(ChildObjPtr->shape(),withDisplayList), ChildObjPtr,false);
        iActor->getBoundingBox(minBound,maxBound);
        AppendBoundingBox(minBound,maxBound);
        mChildObjCompActors.push_back(iActor);
        mNumberOfDetectors++;
      }
    }
  }
}


//------------------------------------------------------------------------------------------------
/**
 * This method checks the list of Objects in mObjects for obj, if found returns the MantidObject. if the obj is not found
 * then creates a new MantidObject for obj and adds to the list of mObjects and returns the newly created MantidObject.
 * @param obj             :: Object input for which MantidObject needed
 * @param withDisplayList :: whether the new MantidObject if needed uses the display list attribute
 * @return  the MantidObject corresponding to the obj.
 */
MantidObject*	CompAssemblyActor::getMantidObject(const boost::shared_ptr<const Mantid::Geometry::Object> obj, bool withDisplayList)
{
  if (!obj)
  {
    throw std::runtime_error("An instrument component does not have a shape.");
  }
  std::map<const boost::shared_ptr<const Object>,MantidObject*>::iterator iObj=mObjects->find(obj);
  if(iObj==mObjects->end()) //create an new Mantid Object
  {
    MantidObject* retObj=new MantidObject(obj,withDisplayList);
    retObj->draw();
    mObjects->insert(mObjects->begin(),std::pair<const boost::shared_ptr<const Object>,MantidObject*>(obj,retObj));
    return retObj;
  }
  return (*iObj).second;
}


//------------------------------------------------------------------------------------------------
/**
 * Concrete implementation of init method of GLObject. this method draws the children
 */
void CompAssemblyActor::init()
{
  for_each(mChildObjCompActors.begin(),mChildObjCompActors.end(),std::mem_fun(&GLActor::draw));
  for_each(mChildCompAssemActors.begin(),mChildCompAssemActors.end(),std::mem_fun(&GLActor::draw));
}

//------------------------------------------------------------------------------------------------
/**
 * This method adds the detector ids of the children of CompAssembly to the input idList.
 * @param idList :: output list of detector ids for child detectors
 */
void CompAssemblyActor::appendObjCompID(std::vector<int>& idList)
{
  for(std::vector<ObjComponentActor*>::const_iterator iObjComp=mChildObjCompActors.begin();iObjComp!=mChildObjCompActors.end(); ++iObjComp)
  {
    //If the object is a detector or a RectangularDetector, it will append the ID(s)
    (*iObjComp)->appendObjCompID(idList);
  }

  for(std::vector<ICompAssemblyActor*>::const_iterator iAssem=mChildCompAssemActors.begin();iAssem!=mChildCompAssemActors.end(); ++iAssem)
  {
    (*iAssem)->appendObjCompID(idList);
  }
}

//------------------------------------------------------------------------------------------------
/**
 * The colors are set using the iterator of the color list. The order of the detectors
 * in this color list was defined by the calls to appendObjCompID().
 *
 * @param list :: Color list iterator
 * @return the number of detectors
 */
int CompAssemblyActor::setInternalDetectorColors(std::vector<boost::shared_ptr<GLColor> >::iterator & list)
{
  int count=0;
  for(std::vector<ObjComponentActor*>::iterator iObjComp=mChildObjCompActors.begin();iObjComp!=mChildObjCompActors.end();iObjComp++)
  {
    //Each child will set its color and increment the list
    int num=(*iObjComp)->setInternalDetectorColors(list);
    count+=num;
  }

  for(std::vector<ICompAssemblyActor*>::iterator iAssem = mChildCompAssemActors.begin();iAssem!=mChildCompAssemActors.end();iAssem++)
  {
    //Each assembly does the same
    int num=(*iAssem)->setInternalDetectorColors(list);
    //list+=num;
    count+=num;
  }
  //std::cout << "CompAssemblyActor::setInternalDetectorColors() called with "<< count << " entries added.\n";
  return count;
}


//------------------------------------------------------------------------------------------------
/**
 * This method redraws the CompAssembly children compassembly actors redraw. this method is used to redraw all the children
 */
void CompAssemblyActor::redraw()
{
  mChanged=true;
  for(std::vector<ICompAssemblyActor*>::iterator iAssem=mChildCompAssemActors.begin();iAssem!=mChildCompAssemActors.end();iAssem++)
    (*iAssem)->redraw();
  construct();
}


//------------------------------------------------------------------------------------------------
/**
 * Set the starting color reference for CompAssembly
 * @param rgb :: input color id
 * @return  the number of color ids that are used.
 */
int CompAssemblyActor::setStartingReferenceColor(int rgb)
{
  mColorStartID=rgb;
  int val = rgb;
  for(std::vector<ObjComponentActor*>::iterator itrObjComp=mChildObjCompActors.begin();itrObjComp!=mChildObjCompActors.end();itrObjComp++)
    {
      (*itrObjComp)->setStartingReferenceColor(val);
      val++;
    }
  for(std::vector<ICompAssemblyActor*>::iterator itrObjAssem=mChildCompAssemActors.begin();itrObjAssem!=mChildCompAssemActors.end();itrObjAssem++)
    {
      val+=(*itrObjAssem)->setStartingReferenceColor(val);
    }
  return val-rgb;
}


//------------------------------------------------------------------------------------------------
/**
 * This method searches the child actors for the input rgb color and returns the detector id corresponding to the rgb color. if the
 * detector is not found then returns -1.
 * @param  rgb :: input color id
 * @return the detector id of the input rgb color.if detector id not found then returns -1
 */
int CompAssemblyActor::findDetectorIDUsingColor(int rgb)
{
  int n_comp_actors = static_cast<int>(mChildObjCompActors.size());
  int diff_rgb = rgb - this->mColorStartID;
  if(diff_rgb >= 0 && diff_rgb < n_comp_actors)
  {
    const boost::shared_ptr<IDetector>  iDec= boost::dynamic_pointer_cast<IDetector>((mChildObjCompActors[diff_rgb])->getObjComponent());
    if(iDec!=boost::shared_ptr<IDetector>())
    {
      return iDec->getID();
    }
  }

  for(std::vector<ICompAssemblyActor*>::iterator iAssem=mChildCompAssemActors.begin();iAssem!=mChildCompAssemActors.end();iAssem++)
  {
    int colorStart = (*iAssem)->getColorStartID();
    if(rgb >= colorStart && rgb < (*iAssem)->getNumberOfDetectors()+colorStart )
      return (*iAssem)->findDetectorIDUsingColor(rgb);
  }
  return -1;
}



//------------------------------------------------------------------------------------------------
/**
 * Append the bounding box CompAssembly bounding box
 * @param minBound :: min point of the bounding box
 * @param maxBound :: max point of the bounding box
 */
void CompAssemblyActor::AppendBoundingBox(const Mantid::Geometry::V3D& minBound,const Mantid::Geometry::V3D& maxBound)
{
  if(minBoundBox[0]>minBound[0]) minBoundBox[0]=minBound[0];
  if(minBoundBox[1]>minBound[1]) minBoundBox[1]=minBound[1];
  if(minBoundBox[2]>minBound[2]) minBoundBox[2]=minBound[2];
  if(maxBoundBox[0]<maxBound[0]) maxBoundBox[0]=maxBound[0];
  if(maxBoundBox[1]<maxBound[1]) maxBoundBox[1]=maxBound[1];
  if(maxBoundBox[2]<maxBound[2]) maxBoundBox[2]=maxBound[2];
}

