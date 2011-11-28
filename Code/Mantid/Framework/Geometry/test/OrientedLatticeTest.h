#ifndef MANTID_GEOMETRY_ORIENTEDLATTICETEST_H_
#define MANTID_GEOMETRY_ORIENTEDLATTICETEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <iomanip>
#include <MantidKernel/Matrix.h>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidKernel/NexusTestHelper.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::NexusTestHelper;

class OrientedLatticeTest : public CxxTest::TestSuite
{
public:

  /// test constructors, access to some of the variables
  void test_Simple()
  {

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

  void test_hklFromQ()
  {
    OrientedLattice u;
    DblMatrix UB(3,3,true);
    u.setUB(UB);

    // Convert to and from HKL
    V3D hkl = u.hklFromQ(V3D(1.0, 2.0, 3.0));
    double dstar = u.dstar(hkl[0], hkl[1], hkl[2]);
    TS_ASSERT_DELTA( dstar, sqrt(1+4.0+9.0), 1e-4); // The d-spacing after a round trip matches the Q we put in
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



  /** @author Alex Buts, fixed by Andrei Savici */
  void testUnitRotation()
  {
    OrientedLattice theCell;
    DblMatrix rot,expected(3,3);
    TSM_ASSERT_THROWS_NOTHING("The unit transformation should not throw",theCell.setUFromVectors(V3D(1,0,0),V3D(0,1,0)));
    rot = theCell.getUB();
    /*this should give
      / 0 1 0 \
      | 0 0 1 |
      \ 1 0 0 /
      */
    expected[0][1]=1.;
    expected[1][2]=1.;
    expected[2][0]=1.;
    TSM_ASSERT("This should produce proper permutation matrix",rot.equals(expected,1e-8));

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

  /** @author Alex Buts, fixed by Andrei Savici */
  void testPermutations()
  {
    OrientedLattice theCell;
    DblMatrix rot,expected(3,3);
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(V3D(0,1,0),V3D(1,0,0)));
    rot = theCell.getUB();
    /*this should give
      / 1 0 0 \
      | 0 0 -1 |
      \ 0 1 0 /
      */
    expected[0][0]=1.;
    expected[1][2]=-1.;
    expected[2][1]=1.;
    TSM_ASSERT("This should produce proper permutation matrix",rot.equals(expected,1e-8));

  }

  /** @author Alex Buts fixed by Andrei Savici*/
  void testRotations2D()
  {
    OrientedLattice theCell;
    DblMatrix rot;
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(V3D(1,1,0),V3D(1,-1,0)));
    rot = theCell.getUB();
    V3D dir0(sqrt(2.),0,0),rez,expected(1,0,1);
    rez=rot*dir0;
    // should be (1,0,1)
    TSM_ASSERT_EQUALS("vector should be (1,0,1)",rez,expected);
  }

  /** @author Alex Buts fixed by Andrei Savici*/
  void testRotations3D()
  {
    OrientedLattice theCell;
    DblMatrix rot;
    // two orthogonal vectors
    V3D ort1(sqrt(2.),-1,-1);
    V3D ort2(sqrt(2.),1,1);
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(ort1,ort2));
    rot = theCell.getUB();

    V3D dir(1,0,0),result,expected(sqrt(0.5),0,sqrt(0.5));
    result=rot*dir;
    TSM_ASSERT_EQUALS("vector should be (sqrt(0.5),0,sqrt(0.5))",result,expected);

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

  ///Test consistency for setUFromVectors
  void testconsistency()
  {
    OrientedLattice theCell(2,2,2,90,90,90);
    V3D u(1,2,0), v(-2,1,0),expected1(0,0,1),expected2(1,0,0),res1,res2;
    DblMatrix rot;
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(u,v));
    rot = theCell.getUB();
    res1=rot*u; res1.normalize();
    res2=rot*v; res2.normalize();
    TSM_ASSERT_EQUALS("Ub*u should be along the beam",res1,expected1);
    TSM_ASSERT_EQUALS("Ub*v should be along the x direction",res2,expected2);
  }

  /// test getting u and v vectors
  void testuvvectors()
  {
    OrientedLattice theCell(1,2,3,30,60,45);
    DblMatrix rot;
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",theCell.setUFromVectors(V3D(1,2,0),V3D(-1,1,0)));
    rot = theCell.getUB();
    V3D u=theCell.getuVector(),v=theCell.getvVector(),expected1(0,0,1),expected2(1,0,0),res1,res2;
    res1=rot*u; res1.normalize();
    res2=rot*v; res2.normalize();
    TSM_ASSERT_EQUALS("Ub*u should be along the beam",res1,expected1);
    TSM_ASSERT_EQUALS("Ub*v should be along the x direction",res2,expected2);
  }

};


#endif /* MANTID_GEOMETRY_UNITCELLTEST_H_ */

