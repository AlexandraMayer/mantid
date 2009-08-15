#ifdef WIN32
#include <windows.h>
#endif
#include "GLTrackball.h"
#include "GLViewport.h"
#define _USE_MATH_DEFINES true
#include <cmath>
#include <GL/gl.h>

GLTrackball::GLTrackball(GLViewport* parent):_viewport(parent)
{
	reset();
	// Rotation speed defines as such is equal 1 in relative units,
	// i.e. the trackball will follow exactly the displacement of the mouse
	// on the sceen. The factor 180/M_PI is simply rad to deg conversion. THis
	// prevent recalculation of this factor every time a generateRotationTo call is issued.
  _rotationspeed=180/M_PI;
	_modelCenter=Mantid::Geometry::V3D(0.0,0.0,0.0);
	hasOffset=false;
}
GLTrackball::~GLTrackball()
{
}
void GLTrackball::initRotationFrom(int a,int b)
{
	projectOnSphere(a,b,_lastpoint);
}
void GLTrackball::generateRotationTo(int a,int b)
{
	Mantid::Geometry::V3D _newpoint;
	projectOnSphere(a,b,_newpoint);
	Mantid::Geometry::V3D diff(_lastpoint);
	// Difference between old point and new point
	diff-=_newpoint;
	// Angle is given in degrees as the dot product of the two vectors
  double angle=_rotationspeed*_newpoint.angle(_lastpoint);
	diff=_lastpoint.cross_prod(_newpoint);
	// Create a quaternion from the angle and vector direction
	Mantid::Geometry::Quat temp(angle,diff);
	// Left multiply
	temp*=_quaternion;
	// Assignment of _quaternion
	_quaternion(temp);
	// Get the corresponding OpenGL rotation matrix
  _quaternion.GLMatrix(&_rotationmatrix[0]);
  return;
}

void GLTrackball::initTranslateFrom(int a,int b)
{
	generateTranslationPoint(a,b,_lastpoint);
}

void GLTrackball::generateTranslationTo(int a, int b)
{
	Mantid::Geometry::V3D _newpoint;
	generateTranslationPoint(a,b,_newpoint);
	// This is now the difference
	_newpoint-=_lastpoint;
	double x,y;
	_viewport->getTranslation(x,y);
	_viewport->setTranslation(x+_newpoint[0],y+_newpoint[1]);
}

void GLTrackball::initZoomFrom(int a,int b)
{
	if (a<=0 || b<=0)
		return;
	double x,y,z=0;
  int _viewport_w, _viewport_h;
  _viewport->getViewport(&_viewport_w,&_viewport_h);
	if(a>=_viewport_w || b>=_viewport_h)
		return;
	x=static_cast<double>((_viewport_w-a));
	y=static_cast<double>((b-_viewport_h));
	_lastpoint(x,y,z);
}

void GLTrackball::generateZoomTo(int a, int b)
{
	double x,y,z=0;
  int _viewport_w, _viewport_h;
  _viewport->getViewport(&_viewport_w,&_viewport_h);
	if(a>=_viewport_w || b>=_viewport_h||a <= 0||b<=0)return;
	x=static_cast<double>((_viewport_w-a));
	y=static_cast<double>((b-_viewport_h));
	if(y==0) y=_lastpoint[1];
	double diff= _lastpoint[1]/y ;
	diff*=_viewport->getZoomFactor();
	_viewport->setZoomFactor(diff);
}


void GLTrackball::IssueRotation() const
{
	if (_viewport)
	{
		// Translate if offset is defined
		if (hasOffset)
			glTranslated(_modelCenter[0],_modelCenter[1],_modelCenter[2]);
		// Rotate with respect to the centre
		glMultMatrixd(_rotationmatrix);
		// Translate back
		if (hasOffset)
			glTranslated(-_modelCenter[0],-_modelCenter[1],-_modelCenter[2]);
	}
	return;
}

void GLTrackball::setModelCenter(const Mantid::Geometry::V3D& center)
{
	_modelCenter=center;
	if (_modelCenter.nullVector())
		hasOffset=false;
	else
		hasOffset=true;
}

Mantid::Geometry::V3D GLTrackball::getModelCenter() const
{
	return _modelCenter;
}

void GLTrackball::projectOnSphere(int a,int b,Mantid::Geometry::V3D& point)
{
	// z initiaised to zero if out of the sphere
  double x,y,z=0;
  int _viewport_w, _viewport_h;
  _viewport->getViewport(&_viewport_w,&_viewport_h);
	x=static_cast<double>((2.0*a-_viewport_w)/_viewport_w);
	y=static_cast<double>((_viewport_h-2.0*b)/_viewport_h);
	double norm=x*x+y*y;
	if (norm>1.0) // The point is inside the sphere
	{
		norm=sqrt(norm);
		x/=norm;
		y/=norm;
	}
	else // The point is outside the sphere, so project to nearest point on circle
		z=sqrt(1.0-norm);
	// Set-up point
	point(x,y,z);
}

void GLTrackball::generateTranslationPoint(int a,int b,Mantid::Geometry::V3D& point)
{
	double x,y,z=0.0;
	int _viewport_w, _viewport_h;
	double xmin,xmax,ymin,ymax,zmin,zmax;
	_viewport->getViewport(&_viewport_w,&_viewport_h);
	_viewport->getProjection(xmin,xmax,ymin,ymax,zmin,zmax);
	x=static_cast<double>((xmin+((xmax-xmin)*((double)a/(double)_viewport_w))));
	y=static_cast<double>((ymin+((ymax-ymin)*(_viewport_h-b)/_viewport_h)));
	double factor=_viewport->getZoomFactor();
	x*=factor;
	y*=factor;
	// Assign new values to point
	point(x,y,z);
}
void GLTrackball::setRotationSpeed(double r)
{
	// Rotation speed needs to contains conversion to degrees.
	//
	if (r>0) _rotationspeed=r*180.0/M_PI;
}
void GLTrackball::setViewport(GLViewport* v)
{
    if (v) _viewport=v;
}

void GLTrackball::reset()
{
	//Reset rotation,scale and translation
	_quaternion.init();
  _quaternion.GLMatrix(&_rotationmatrix[0]);
	_viewport->setTranslation(0.0,0.0);
	_viewport->setZoomFactor(1.0);
}

void GLTrackball::setViewToXPositive()
{
	reset();
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(1.0,0.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToYPositive()
{
	reset();
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(0.0,1.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToZPositive()
{
	reset();
	_quaternion.init();
	_quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToXNegative()
{
	reset();
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(-1.0,0.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToYNegative()
{
	reset();
	Mantid::Geometry::Quat tempy(Mantid::Geometry::V3D(0.0,0.0,1.0),Mantid::Geometry::V3D(0.0,-1.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::setViewToZNegative()
{
	reset();
	Mantid::Geometry::Quat tempy(180.0,Mantid::Geometry::V3D(0.0,1.0,0.0));
	_quaternion=tempy;
	_quaternion.GLMatrix(&_rotationmatrix[0]);
}

void GLTrackball::rotateBoundingBox(double& xmin,double& xmax,double& ymin,double& ymax,double& zmin,double& zmax)
{
	// Defensive
	if (xmin>xmax) std::swap(xmin,xmax);
	if (ymin>ymax) std::swap(ymin,ymax);
	if (zmin>zmax) std::swap(zmin,zmax);
	// Get the min and max of the cube, and remove centring offset
	Mantid::Geometry::V3D minT(xmin,ymin,zmin), maxT(xmax,ymax,zmax);
	minT-=_modelCenter;
	maxT-=_modelCenter;
	// Get the rotation matrix
	double rotMatr[16];
	_quaternion.GLMatrix(&rotMatr[0]);
	// Now calculate new min and max depending on the sign of matrix components
	// Much faster than creating 8 points and rotate them. The new min (max)
	// can only be obtained by summing the smallest (largest) components
	//
	Mantid::Geometry::V3D minV, maxV;
	// Looping on rows of matrix
	int index;
	for (int i=0;i<3;i++)
	{
		for (int j=0;j<3;j++)
		{
			index=j+i*4; // The OpenGL matrix is linear and represent a 4x4 matrix but only the 3x3 upper-left inner part
			// contains the rotation
			minV[j]+=(rotMatr[index]>0)?rotMatr[index]*minT[i]:rotMatr[index]*maxT[i];
			maxV[j]+=(rotMatr[index]>0)?rotMatr[index]*maxT[i]:rotMatr[index]*minT[i];
		}
	}
	// Re-apply offset
	minV+=_modelCenter;
	maxV+=_modelCenter;
	// Adjust value.
	xmax=maxV[0]; ymax=maxV[1]; zmax=maxV[2];
	xmin=minV[0]; ymin=minV[1]; zmin=minV[2];
	return;
}

void GLTrackball::setRotation(const Mantid::Geometry::Quat& quat)
{
	_quaternion=quat;
	_quaternion.GLMatrix(&_rotationmatrix[0]);
}
