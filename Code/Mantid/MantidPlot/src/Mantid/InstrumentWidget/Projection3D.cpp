 #ifdef WIN32
#include <windows.h>
#endif
#include "Projection3D.h"
#include "GLActor.h"
#include "GLColor.h"

#include "UnwrappedCylinder.h"
#include "UnwrappedSphere.h"
#include "OpenGLError.h"
#include "DetSelector.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Objects/Object.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <QtOpenGL>
#include <QSpinBox>
#include <QApplication>
#include <QTime>

#include <map>
#include <string>
#include <iostream>
#include <cfloat>
#include <algorithm>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

Projection3D::Projection3D(const InstrumentActor* rootActor,int winWidth,int winHeight)
  :ProjectionSurface(rootActor,Mantid::Geometry::V3D(),Mantid::Geometry::V3D(0,0,1)),
  //m_instrActor(*rootActor),
  m_viewport(new GLViewport),
  m_drawAxes(true),
  m_wireframe(false),
  m_isKeyPressed(false)
{

  IInstrument_const_sptr instr = rootActor->getInstrument();
  std::vector<boost::shared_ptr<IComponent> > allComponents;
  instr->getChildren(allComponents,true);
  std::vector<ComponentID> nonDetectors;
  std::vector<boost::shared_ptr<IComponent> >::const_iterator it = allComponents.begin();
  for(; it != allComponents.end(); ++it)
  {
    IDetector_const_sptr det = boost::dynamic_pointer_cast<IDetector>(*it);
    if (!det)
    {
      nonDetectors.push_back((*it)->getComponentID());
    }
  }

  m_viewport->resize(winWidth,winHeight);
  V3D minBounds,maxBounds;
  m_instrActor->getBoundingBox(minBounds,maxBounds);

  double radius = minBounds.norm();
  double tmp = maxBounds.norm();
  if (tmp > radius) radius = tmp;

  m_viewport->setOrtho(minBounds.X(),maxBounds.X(),
                       minBounds.Y(),maxBounds.Y(),
                       -radius,radius);
  m_trackball = new GLTrackball(m_viewport);
  changeColorMap();
  rootActor->invalidateDisplayLists();
}

Projection3D::~Projection3D()
{
  delete m_trackball;
  delete m_viewport;
}

void Projection3D::init()
{
}

void Projection3D::resize(int w, int h)
{
  if (m_viewport)
  {
    m_viewport->resize(w,h);
    m_viewport->issueGL();
  }
}

void Projection3D::drawSurface(MantidGLWidget*,bool picking)const
{
  OpenGLError::check("GL3DWidget::draw3D()[begin]");

  glEnable(GL_DEPTH_TEST);

  if (m_wireframe)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }
  else
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  m_viewport->issueGL();

  //glClearColor(GLclampf(bgColor.red()/255.0),GLclampf(bgColor.green()/255.0),GLclampf(bgColor.blue()/255.0),1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Reset the rendering options just in case
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Issue the rotation, translation and zooming of the trackball to the object
  m_trackball->IssueRotation();
  
  //glPushMatrix();
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if (m_viewChanged)
  {
    //m_actor3D.update();
    m_viewChanged = false;
  }
  m_instrActor->draw(picking);
  OpenGLError::check("GL3DWidget::draw3D()[scene draw] ");

  //This draws a point at the origin, I guess
  glPointSize(3.0);
  glBegin(GL_POINTS);
  glVertex3d(0.0,0.0,0.0);
  glEnd();

  //Also some axes
  if (m_drawAxes)
  {
    drawAxes();
  }

  QApplication::restoreOverrideCursor();
  //glPopMatrix();
  OpenGLError::check("GL3DWidget::draw3D()");
}

/** Draw 3D axes centered at the origin (if the option is selected)
 *
 */
void Projection3D::drawAxes(double axis_length)const
{
  glPointSize(3.0);
  glLineWidth(3.0);

  //To make sure the lines are colored
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_TEXTURE_2D);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  glColor3f(1.0, 0., 0.);
  glBegin(GL_LINES);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(axis_length, 0.0,0.0);
  glEnd();

  glColor3f(0., 1.0, 0.);
  glBegin(GL_LINES);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(0.0, axis_length, 0.0);
  glEnd();

  glColor3f(0., 0., 1.);
  glBegin(GL_LINES);
  glVertex3d(0.0,0.0,0.0);
  glVertex3d(0.0, 0.0, axis_length);
  glEnd();
}

void Projection3D::mousePressEventMove(QMouseEvent* event)
{
  if (event->buttons() & Qt::MidButton)
  {
    m_trackball->initZoomFrom(event->x(),event->y());
    m_isKeyPressed=true;
    //setSceneLowResolution();
  }
  else if (event->buttons() & Qt::LeftButton)
  {
    m_trackball->initRotationFrom(event->x(),event->y());
    m_isKeyPressed=true;
    //setSceneLowResolution();
  }
  else if(event->buttons() & Qt::RightButton)
  {
    m_trackball->initTranslateFrom(event->x(),event->y());
    m_isKeyPressed=true;
    //setSceneLowResolution();
  }
  OpenGLError::check("GL3DWidget::mousePressEvent");
}

void Projection3D::mouseMoveEventMove(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
      m_trackball->generateRotationTo(event->x(),event->y());
      m_trackball->initRotationFrom(event->x(),event->y());
    }
    else if(event->buttons() & Qt::RightButton)
    { //Translate
      m_trackball->generateTranslationTo(event->x(),event->y());
      m_trackball->initTranslateFrom(event->x(),event->y());
    }
    else if(event->buttons() & Qt::MidButton)
    { //Zoom
      m_trackball->generateZoomTo(event->x(),event->y());
      m_trackball->initZoomFrom(event->x(),event->y());
    }
    m_viewChanged = true;
//  }
  OpenGLError::check("GL3DWidget::mouseMoveEvent");
}

void Projection3D::mouseReleaseEventMove(QMouseEvent*)
{
  m_isKeyPressed=false;
  m_viewChanged = true;
}

void Projection3D::wheelEventMove(QWheelEvent* event)
{
  m_trackball->initZoomFrom(event->x(),event->y());
  m_trackball->generateZoomTo(event->x(),event->y()-event->delta());
  m_viewChanged = true;
}

void Projection3D::changeColorMap()
{
}

void Projection3D::setViewDirection(const QString& input)
{
	if(input.compare("X+")==0)
	{
		m_trackball->setViewToXPositive();
	}
	else if(input.compare("X-")==0)
	{
		m_trackball->setViewToXNegative();
	}
	else if(input.compare("Y+")==0)
	{
		m_trackball->setViewToYPositive();
	}
	else if(input.compare("Y-")==0)
	{
		m_trackball->setViewToYNegative();
	}
	else if(input.compare("Z+")==0)
	{
		m_trackball->setViewToZPositive();
	}
	else if(input.compare("Z-")==0)
	{
		m_trackball->setViewToZNegative();
	}
  updateView();
}

void Projection3D::set3DAxesState(bool on)
{
  m_drawAxes = on;
}

void Projection3D::setWireframe(bool on)
{
  m_wireframe = on;
}

void Projection3D::getSelectedDetectors(QList<int>& dets)
{
  dets.clear();
  if (!hasSelection()) return;
  double xmin,xmax,ymin,ymax,zmin,zmax;
  m_viewport->getInstantProjection(xmin,xmax,ymin,ymax,zmin,zmax);
  QRect rect = selectionRect();
  int w,h;
  m_viewport->getViewport(&w,&h);

  double xLeft = xmin + (xmax - xmin) * rect.left() / w;
  double xRight = xmin + (xmax - xmin) * rect.right() / w;
  double yBottom = ymin + (ymax - ymin) * (h - rect.bottom()) / h;
  double yTop = ymin  + (ymax - ymin) * (h - rect.top()) / h;
  //std::cerr 
  //                       << xLeft << ' ' << xRight << '\n'
  //                       << yBottom << ' ' << yTop << '\n'
  //                       << zmin << ' ' << zmax << "\n\n";
  size_t ndet = m_instrActor->ndetectors();
  Quat rot = m_trackball->getRotation();
  //std::cerr << m_trackball->getModelCenter() << ' ' << rot << std::endl;
  for(int i = 0; i < ndet; ++i)
  {
    boost::shared_ptr<IDetector> det = m_instrActor->getDetector(i);
    V3D pos = det->getPos();
    rot.rotate(pos);
    //std::cerr << "pos=" << pos << std::endl;
    if (pos.X() >= xLeft && pos.X() <= xRight &&
        pos.Y() >= yBottom && pos.Y() <= yTop)
    {
      dets.push_back(int(det->getID()));
    }
  }
}

void Projection3D::componentSelected(Mantid::Geometry::ComponentID id)
{

  IInstrument_const_sptr instr = m_instrActor->getInstrument();

  if (id == NULL || id == instr->getComponentID())
  {
    V3D minBounds,maxBounds;
    m_instrActor->getBoundingBox(minBounds,maxBounds);

    double radius = minBounds.norm();
    double tmp = maxBounds.norm();
    if (tmp > radius) radius = tmp;

    m_viewport->setOrtho(minBounds.X(),maxBounds.X(),
      minBounds.Y(),maxBounds.Y(),
      -radius,radius);
    return;
  }

  IComponent_const_sptr comp = instr->getComponentByID(id);
  V3D pos = comp->getPos();

  V3D compDir = comp->getPos() - instr->getSample()->getPos();
  compDir.normalize();
  V3D up(0,0,1);
  V3D x = up.cross_prod(compDir);
  up = compDir.cross_prod(x);
  Quat rot;
  InstrumentActor::BasisRotation(x,up,compDir,V3D(-1,0,0),V3D(0,1,0),V3D(0,0,-1),rot);

  BoundingBox bbox;
  comp->getBoundingBox(bbox);
  V3D minBounds = bbox.minPoint() + pos;
  V3D maxBounds = bbox.maxPoint() + pos;
  rot.rotate(minBounds);
  rot.rotate(maxBounds);

  // cannot get it right
  //double znear = - minBounds.Z();
  //double zfar = - maxBounds.Z();
  //if (znear > zfar)
  //{
  //  std::swap(znear,zfar);
  //}

  m_viewport->setOrtho(minBounds.X(),maxBounds.X(),
                       minBounds.Y(),maxBounds.Y(),
                       0,1000.);
                       //znear,zfar);


  m_trackball->reset();
  m_trackball->setRotation(rot);
  //m_trackball->setModelCenter(comp->getPos());
}

QString Projection3D::getInfoText()const
{
  if (m_interactionMode == PickMode)
  {
    return getPickInfoText();
  }
  QString text = "Mouse Buttons: Left -- Rotation, Middle -- Zoom, Right -- Translate";
  if( m_drawAxes )
  {
    text += "\nAxes: X = Red; Y = Green; Z = Blue";
  }
  return text;
}
