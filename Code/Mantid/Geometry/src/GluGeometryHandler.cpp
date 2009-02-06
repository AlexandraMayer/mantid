#include "gts.h"
#include "MantidGeometry/Object.h"
#include "MantidGeometry/ObjComponent.h"
#include "MantidGeometry/GeometryHandler.h"
#include "MantidGeometry/GtsGeometryRenderer.h"
#include "MantidGeometry/GluGeometryHandler.h"
#include "MantidGeometry/GluGeometryRenderer.h"

namespace Mantid
{
	namespace Geometry
	{
		GluGeometryHandler::GluGeometryHandler(IObjComponent *comp):GeometryHandler(comp)
		{
			Renderer    = new GluGeometryRenderer();
		}

		GluGeometryHandler::GluGeometryHandler(boost::shared_ptr<Object> obj):GeometryHandler(obj)
		{
			Renderer    =new GluGeometryRenderer();
		}

		GluGeometryHandler::GluGeometryHandler(Object* obj):GeometryHandler(obj)
		{
			Renderer    =new GluGeometryRenderer();
		}

		GluGeometryHandler::~GluGeometryHandler()
		{
			if(Renderer    !=NULL) delete Renderer;
		}

		GeometryHandler* GluGeometryHandler::createInstance(IObjComponent *comp)
		{
			return new GluGeometryHandler(comp);
		}

		GeometryHandler* GluGeometryHandler::createInstance(boost::shared_ptr<Object> obj)
		{
			return new GluGeometryHandler(obj);
		}

		void GluGeometryHandler::Triangulate()
		{
			//Check whether Object is triangulated otherwise triangulate
			//Doesn't have to do anything because we are not going to triangulate anything
		}

		void GluGeometryHandler::Render()
		{
			if(Obj!=NULL){
				switch(type){
					case CUBOID:
						(dynamic_cast<GluGeometryRenderer*>(Renderer))->RenderCube(Point1,Point2,Point3,Point4);
						break;
					case SPHERE:
						(dynamic_cast<GluGeometryRenderer*>(Renderer))->RenderSphere(center,radius);
						break;
					case CYLINDER:
						(dynamic_cast<GluGeometryRenderer*>(Renderer))->RenderCylinder(center,axis,radius,height);
						break;
					case CONE:
						(dynamic_cast<GluGeometryRenderer*>(Renderer))->RenderCone(center,axis,radius,height);
						break;
				}
			}else if(ObjComp!=NULL){
				Renderer->Render(ObjComp);
			}
		}

        void GluGeometryHandler::GetObjectGeom(int& mytype, std::vector<Geometry::V3D>& vectors, double& myradius, double myheight)
        {
            mytype=0;
            if(Obj!=NULL)
            {
				switch(type){
					case CUBOID:
                        mytype=1;
						vectors.push_back(Point1);vectors.push_back(Point2);
                        vectors.push_back(Point3);vectors.push_back(Point4);
						break;
					case SPHERE:
                        mytype=2;
						vectors.push_back(center);myradius=radius;
						break;
					case CYLINDER:
                        mytype=3;
						vectors.push_back(center);myradius=radius;
                        vectors.push_back(axis);myheight=height;
						break;
					case CONE:
                        mytype=4;
						vectors.push_back(center);myradius=radius;
                        vectors.push_back(axis);myheight=height;
						break;
				}
            }
        }

		void GluGeometryHandler::Initialize()
		{
			if(Obj!=NULL){
				//There is no initialization or probably call render
				Render();
			}else if(ObjComp!=NULL){
//				Renderer->Initialize(ObjComp);
			}
		}

		void GluGeometryHandler::setCuboid(V3D p1,V3D p2,V3D p3,V3D p4)
		{
			type=CUBOID;
			Point1=p1;
			Point2=p2;
			Point3=p3;
			Point4=p4;
		}
		void GluGeometryHandler::setSphere(V3D c,double r)
		{
			type=SPHERE;
			center=c;
			radius=r;
		}
		void GluGeometryHandler::setCylinder(V3D c,V3D a,double r,double h)
		{
			type=CYLINDER;
			center=c;
			axis=a;
			radius=r;
			height=h;
		}
		void GluGeometryHandler::setCone(V3D c,V3D a,double r,double h)
		{
			type=CONE;
			center=c;
			axis=a;
			radius=r;
			height=h;
		}
	}
}