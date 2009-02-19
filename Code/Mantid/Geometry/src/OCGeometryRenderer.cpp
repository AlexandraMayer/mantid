#include "MantidGeometry/OCGeometryRenderer.h"
#include "MantidGeometry/IObjComponent.h"
#include <climits>
#ifdef _WIN32
#include "windows.h"
#endif
#include "GL/gl.h"
#include <gp_Pnt.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Poly_Triangulation.hxx>

namespace Mantid
{
	namespace Geometry
	{

		/**
		* Constructor
		* Generated the display list
		*/
		OCGeometryRenderer::OCGeometryRenderer()
		{
			boolDisplaylistCreated=false;
			iDisplaylistId=UINT_MAX;
		}

		/**
		* Destructor
		* Deletes the display list
		*/
		OCGeometryRenderer::~OCGeometryRenderer()
		{
		}

		/**
		* Renders Object surface given as OpenCascade topology shape
		* @param ObjSurf object's surface stored in topology shape
		*/
		void OCGeometryRenderer::Render(TopoDS_Shape* ObjSurf)
		{
			glCallList(iDisplaylistId);
		}

		/**
		* Render ObjComponent
		* @param ObjComp input to render
		*/
		void OCGeometryRenderer::Render(IObjComponent *ObjComp)
		{
			if(ObjComp==NULL)return;
			glPushMatrix();
			V3D pos  =ObjComp->getPos();
			Quat rot =ObjComp->getRotation();
			double rotGL[16];
			rot.GLMatrix(rotGL);
			glTranslated(pos[0],pos[1],pos[2]);
			glMultMatrixd(rotGL);
			V3D scaleFactor=ObjComp->getScaleFactor();
			glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
			ObjComp->drawObject();
			glPopMatrix();
		}

		/**
		* Initialze the object surface for rendering
		* @param ObjSurf input to create display list
		*/
		void OCGeometryRenderer::Initialize(TopoDS_Shape* ObjSurf)
		{
			if(!boolDisplaylistCreated||glIsList(iDisplaylistId)==GL_FALSE)
			{
				iDisplaylistId=glGenLists(1);
				glNewList(iDisplaylistId,GL_COMPILE); //Construct display list for object representation
				glBegin(GL_TRIANGLES);
				//Here goes the traversing through TopoDS_Shape triangles
				RenderTopoDS(ObjSurf);
				glEnd();
				glEndList();
//				WriteVTK(ObjSurf);
				boolDisplaylistCreated=true;
			}
		}

		/**
		* Initializes creates a display for the input ObjComponent
		* @param ObjComp input object component for creating display
		*/
		void OCGeometryRenderer::Initialize(IObjComponent *ObjComp)
		{
			glPushMatrix();
			V3D pos  =ObjComp->getPos();
			Quat rot =ObjComp->getRotation();
			double rotGL[16];
			rot.GLMatrix(rotGL);
			glTranslated(pos[0],pos[1],pos[2]);
			glMultMatrixd(rotGL);
			V3D scaleFactor=ObjComp->getScaleFactor();
			glScaled(scaleFactor[0],scaleFactor[1],scaleFactor[2]);
			ObjComp->drawObject();
			glPopMatrix();
		}

		/**
		 * Renders TopoDS Shape by traversing through the TopoDS_Shape 
		 */
		void OCGeometryRenderer::RenderTopoDS(TopoDS_Shape* ObjSurf)
		{
			if(ObjSurf!=NULL&&!ObjSurf->IsNull())
			{
				TopExp_Explorer Ex;
				for(Ex.Init(*ObjSurf,TopAbs_FACE);Ex.More();Ex.Next())
				{
					TopoDS_Face F=TopoDS::Face(Ex.Current());
					TopLoc_Location L;
					Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
					TColgp_Array1OfPnt tab(1,(facing->NbNodes()));
					tab = facing->Nodes();
					Poly_Array1OfTriangle tri(1,facing->NbTriangles());
					tri = facing->Triangles();
					for (Standard_Integer i=1;i<=(facing->NbTriangles());i++) {
						Poly_Triangle trian = tri.Value(i);
						Standard_Integer index1,index2,index3,M,N;
						trian.Get(index1,index2,index3);
						gp_Pnt point1=tab.Value(index1);
						gp_Pnt point2=tab.Value(index2);
						gp_Pnt point3=tab.Value(index3);
						gp_XYZ pt1 = tab.Value(index1).XYZ();
						gp_XYZ pt2 = tab.Value(index2).XYZ();
						gp_XYZ pt3 = tab.Value(index3).XYZ();

						gp_XYZ v1 = pt2-pt1;
						gp_XYZ v2 = pt3-pt2;

						gp_XYZ normal = v1^v2;
						normal.Normalize();
						glNormal3d(normal.X(),normal.Y(),normal.Z());
						glVertex3d(point1.X(),point1.Y(),point1.Z());
						glVertex3d(point2.X(),point2.Y(),point2.Z());
						glVertex3d(point3.X(),point3.Y(),point3.Z());		
					}		
				}
			}
		}

		void OCGeometryRenderer::WriteVTK(TopoDS_Shape* out)
		{
			FILE *fp=fopen("C:\\outputcascade.vtk","w+");
			fprintf(fp,"# vtk DataFile Version 2.0 \nOpenCascade data\nASCII\n");
			fprintf(fp,"DATASET POLYDATA\n");
//			BRepMesh::Mesh(out,0.1);
			TopExp_Explorer Ex;
			int countVert=0;
			int countFace=0;
			for(Ex.Init(*out,TopAbs_FACE);Ex.More();Ex.Next())
			{
				TopoDS_Face F=TopoDS::Face(Ex.Current());
				TopLoc_Location L;
				Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
				countVert+=facing->NbNodes();
				countFace+=facing->NbTriangles();
			}	
			fprintf(fp,"POINTS %d float\n",countVert);
			for(Ex.Init(*out,TopAbs_FACE);Ex.More();Ex.Next())
			{
				TopoDS_Face F=TopoDS::Face(Ex.Current());
				TopLoc_Location L;
				Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
				TColgp_Array1OfPnt tab(1,(facing->NbNodes()));
				tab = facing->Nodes();
				for (Standard_Integer i=1;i<=(facing->NbNodes());i++) {
					gp_Pnt pnt=tab.Value(i);
					fprintf(fp,"%lf %lf %lf\n",pnt.X(),pnt.Y(),pnt.Z());
				}
			}
			fprintf(fp,"POLYGONS %d %d\n",countFace,countFace*4);
			int maxindex=0;
			for(Ex.Init(*out,TopAbs_FACE);Ex.More();Ex.Next())
			{
				TopoDS_Face F=TopoDS::Face(Ex.Current());
				TopLoc_Location L;
				Handle (Poly_Triangulation) facing=BRep_Tool::Triangulation(F,L);
				TColgp_Array1OfPnt tab(1,(facing->NbNodes()));
				tab = facing->Nodes();
				Poly_Array1OfTriangle tri(1,facing->NbTriangles());
				tri = facing->Triangles();
				for (Standard_Integer i=1;i<=(facing->NbTriangles());i++) {
					Poly_Triangle trian = tri.Value(i);
					Standard_Integer index1,index2,index3,M,N;
					trian.Get(index1,index2,index3);
					fprintf(fp,"3 %d %d %d\n",maxindex+index1-1,maxindex+index2-1,maxindex+index3-1);
				}		
				maxindex+=facing->NbNodes();
			}
			fclose(fp);
		}

	}
}
