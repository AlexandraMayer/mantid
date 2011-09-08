#ifndef MANTID_GEOMETRY_ORIENTEDLATTICETEST_H_
#define MANTID_GEOMETRY_ORIENTEDLATTICETEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <iomanip>
#include <MantidKernel/Matrix.h>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidNexusCPP/NexusTestHelper.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Matrix;
using Mantid::NexusCPP::NexusTestHelper;

class OrientedLatticeTest : public CxxTest::TestSuite
{
public:

  void test_Simple()
  {
    // test constructors, access to some of the variables
    OrientedLattice u1,u2(3,4,5),u3(2,3,4,85.,95.,100),u4;
    u4=u2;
    TS_ASSERT_EQUALS(u1.a1(),1);
    TS_ASSERT_EQUALS(u1.alpha(),90);
    TS_ASSERT_DELTA(u2.b1(),1./3.,1e-10);
    TS_ASSERT_DELTA(u2.alphastar(),90,1e-10);
    TS_ASSERT_DELTA(u4.volume(),1./u2.recVolume(),1e-10);
    u2.seta(3);
    TS_ASSERT_DELTA(u2.a(),3,1e-10);
  }

  void test_Advanced()
  {
    // test more advanced calculations
    // the new Gstar shold yield a=2.5, b=6, c=8, alpha=93, beta=88, gamma=97.
    DblMatrix newGstar(3,3);
    newGstar[0][0]=0.162546756312;
    newGstar[0][1]=0.00815256992072;
    newGstar[0][2]=-0.00145274558861;
    newGstar[1][0]=newGstar[0][1];
    newGstar[1][1]=0.028262965555;
    newGstar[1][2]=0.00102046431298;
    newGstar[2][0]=newGstar[0][2];
    newGstar[2][1]=newGstar[1][2];
    newGstar[2][2]=0.0156808990098;

    OrientedLattice u;
    u.recalculateFromGstar(newGstar);
    TS_ASSERT_DELTA(u.a(),2.5,1e-10);
    TS_ASSERT_DELTA(u.b(),6,1e-10);
    TS_ASSERT_DELTA(u.c(),8,1e-10);
    TS_ASSERT_DELTA(u.alpha(),93,1e-10);
    TS_ASSERT_DELTA(u.beta(),88,1e-10);
    TS_ASSERT_DELTA(u.gamma(),97,1e-10);

    // get the some elements of the B matrix
    TS_ASSERT_DELTA(u.getB()[0][0],0.403170877311,1e-10);
    TS_ASSERT_DELTA(u.getB()[2][0],0.0,1e-10);
    TS_ASSERT_DELTA(u.getB()[0][2],-0.00360329991666,1e-10);
    TS_ASSERT_DELTA(u.getB()[2][2],0.125,1e-10);
    // d spacing for direct lattice at (1,1,1) (will automatically check dstar)
    TS_ASSERT_DELTA(u.d(1.,1.,1.),2.1227107587,1e-10);
    // angle
    TS_ASSERT_DELTA(u.recAngle(1,1,1,1,0,0,angRadians),0.471054990614,1e-10);
  }


  void test_nexus()
  {
    NexusTestHelper th(false);
    th.createFile("OrientedLatticeTest.nxs");
    DblMatrix U(3,3,true);
    OrientedLattice u(1,2,3, 90, 89, 88);
    u.saveNexus(th.file, "lattice");
    th.reopenFile();

    OrientedLattice u2;
    u2.loadNexus(th.file, "lattice");
    // Was it reloaded correctly?
    TS_ASSERT_DELTA( u2.a(), 1.0, 1e-5);
    TS_ASSERT_DELTA( u2.b(), 2.0, 1e-5);
    TS_ASSERT_DELTA( u2.c(), 3.0, 1e-5);
    TS_ASSERT_DELTA( u2.alpha(), 90.0, 1e-5);
    TS_ASSERT_DELTA( u2.beta(), 89.0, 1e-5);
    TS_ASSERT_DELTA( u2.gamma(), 88.0, 1e-5);
  }



  /** @author Alex Buts */
  void testUnitRotation()
  {
    OrientedLattice theCell;
    DblMatrix rot;
    TSM_ASSERT_THROWS_NOTHING("The unit transformation should not throw",theCell.setUFromVectors(V3D(1,0,0),V3D(0,1,0)));
    rot = theCell.getUB();

    std::vector<double> Rot = rot.get_vector();
    std::vector<double> rez(9,0);
    rez[0]=1;
    rez[4]=1;
    rez[8]=1;
    double err=0;
    for(int i=0;i<9;i++){
      err += (rez[i]-Rot[i])*(rez[i]-Rot[i]);
    }
    TSM_ASSERT_DELTA("This should produce proper permutation matrix defined as a vector",0,err,1.e-6);

  }

  /** @author Alex Buts */
  void testParallelProjThrows()
  {
    OrientedLattice theCell;
    DblMatrix rot;
    TSM_ASSERT_THROWS("The transformation to plane defined by two parallel vectors should throw",
        theCell.setUFromVectors(V3D(0,1,0),V3D(0,1,0)),std::invalid_argument);
    rot = theCell.getUB();
  }

  /** @author Alex Buts */
  void testPermutations()
  {
    OrientedLattice theCell;
    DblMatrix rot;
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(V3D(0,1,0),V3D(1,0,0)));
    rot = theCell.getUB();

    std::vector<double> Rot = rot.get_vector();
    std::vector<double> rez(9,0);
    rez[1]=1;
    rez[3]=1;
    rez[8]=-1;
    double err=0;
    for(int i=0;i<9;i++){
      err += (rez[i]-Rot[i])*(rez[i]-Rot[i]);
    }
    TSM_ASSERT_DELTA("This should produce proper permutation matrix defined as a vector",0,err,1.e-6);

  }

  /** @author Alex Buts */
  void testRotations2D()
  {
    OrientedLattice theCell;
    DblMatrix rot;
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(V3D(1,1,0),V3D(1,-1,0)));
    rot = theCell.getUB();
    V3D dir0(sqrt(2.),0,0);

    std::vector<double> Rot = rot.get_vector();
    double x = Rot[0]*dir0.X()+Rot[3]*dir0.Y()+Rot[6]*dir0.Z();
    double y = Rot[1]*dir0.X()+Rot[4]*dir0.Y()+Rot[7]*dir0.Z();
    double z = Rot[2]*dir0.X()+Rot[5]*dir0.Y()+Rot[8]*dir0.Z();

    TSM_ASSERT_DELTA("X-coord shoud be 1",1,x,1.e-5);
    TSM_ASSERT_DELTA("Y-coord shoud be 1",1,y,1.e-5);
    TSM_ASSERT_DELTA("Z-coord shoud be 0",0,z,1.e-5);
  }

  /** @author Alex Buts */
  void testRotations3D()
  {
    OrientedLattice theCell;
    DblMatrix rot;
    // two orthogonal vectors
    V3D ort1(sqrt(2.),-1,-1);
    V3D ort2(sqrt(2.),1,1);
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(ort1,ort2));
    rot = theCell.getUB();

    V3D dir(1,0,0);
    V3D xx = ort1.cross_prod(ort2);
    //double pp = xx.scalar_prod(dir); // dir should belong to ort1,ort2 plain

    double p1=dir.scalar_prod(ort1)/ort1.norm();
    double p2=dir.scalar_prod(ort2)/ort2.norm();

    std::vector<double> Rot = rot.get_vector();
    double x = Rot[0]*dir.X()+Rot[3]*dir.Y()+Rot[6]*dir.Z();
    double y = Rot[1]*dir.X()+Rot[4]*dir.Y()+Rot[7]*dir.Z();
    double z = Rot[2]*dir.X()+Rot[5]*dir.Y()+Rot[8]*dir.Z();

    TSM_ASSERT_DELTA("X-coord should be 1/sqrt(2)",p1,x,1.e-5);
    TSM_ASSERT_DELTA("Y-coord shlule be 1/sqrt(2)",p2,y,1.e-5);
    TSM_ASSERT_DELTA("Z-coord should be 0"  ,0, z,1.e-5);
  }

  /** @author Alex Buts */
  void testRotations3DNonOrthogonal()
  {
    OrientedLattice theCell(1,2,3,30,60,45);
    DblMatrix rot;
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(V3D(1,0,0),V3D(0,1,0)));
    rot = theCell.getUB();

    V3D dir(1,1,1);

    std::vector<double> Rot = rot.get_vector();
    double x = Rot[0]*dir.X()+Rot[3]*dir.Y()+Rot[6]*dir.Z();
    double y = Rot[1]*dir.X()+Rot[4]*dir.Y()+Rot[7]*dir.Z();
    double z = Rot[2]*dir.X()+Rot[5]*dir.Y()+Rot[8]*dir.Z();
    // this freeses the interface but unclear how to propelry indentify the
    TSM_ASSERT_DELTA("X-coord should be specified correctly",1.4915578672621419,x,1.e-5);
    TSM_ASSERT_DELTA("Y-coord should be specified correctly",0.18234563931714265,y,1.e-5);
    TSM_ASSERT_DELTA("Z-coord should be specified correctly",-0.020536948488997286,z,1.e-5);
  }



};


#endif /* MANTID_GEOMETRY_UNITCELLTEST_H_ */

