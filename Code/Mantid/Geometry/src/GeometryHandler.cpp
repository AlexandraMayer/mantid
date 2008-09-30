#include "MantidGeometry/GeometryHandler.h"

namespace Mantid
{
	namespace Geometry
	{

		/** Constructor
		*  @param[in] comp 
		*  This geometry handler will be ObjComponent's geometry handler
		*/
		GeometryHandler::GeometryHandler(ObjComponent *comp):Obj()
		{
			ObjComp=comp;
			ObjComp->setGeometryHandler(this);
			boolTriangulated=true;
			boolIsInitialized = false;
		}

		/** Constructor  
		*  @param[in] obj 
		*  This geometry handler will be Object's geometry handler
		*/
		GeometryHandler::GeometryHandler(boost::shared_ptr<Object> obj):Obj(obj.get())
		{
			ObjComp=NULL;
			Obj->setGeometryHandler(this);
			boolTriangulated=false;
			boolIsInitialized=false;
		}

		/** Constructor  
		*  @param[in] obj 
		*  This geometry handler will be Object's geometry handler
		*/
		GeometryHandler::GeometryHandler(Object* obj):Obj(obj)
		{
			ObjComp=NULL;
			Obj->setGeometryHandler(this);
			boolTriangulated=false;
			boolIsInitialized=false;
		}

		/// Destructor
		GeometryHandler::~GeometryHandler()
		{
			ObjComp=NULL;
		}
	}
}