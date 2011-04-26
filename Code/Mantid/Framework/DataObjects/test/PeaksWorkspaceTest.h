#ifndef MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <cmath>
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeaksWorkspaceTest : public CxxTest::TestSuite
{
public:

  int removeFile( std::string outfile)
  {
     return remove( outfile.c_str());
  }

  bool sameFileContents( std::string file1, std::string file2)
  {
    std::ifstream in1( file1.c_str() );
    std::ifstream in2( file2.c_str() );

    std::string s1, s2;
    while (in1.good() && in2.good())
    {
      std::getline(in1,s1);
      std::getline(in2,s2);
      s1 = Strings::replace(s1, "\r", "");
      s2 = Strings::replace(s2, "\r", "");
      if (s1 != s2)
        return false;
    }
    if( in1.good() || in2.good())
      return false;
    return true;


//    bool done = false;
//    char c1,c2;
//    while( !done)
//    {
//      if( !in1.good() || !in2.good())
//        done = true;
//      else
//      {
//        c1= in1.get();
//        c2= in2.get();
//        if( c1 != c2)
//          return false;
//      }
//    }
//    if( in1.good() || in2.good())
//      return false;
//    return true;
//    return true;
  }



  void test_Something()
  {
    std::vector<std::string>ext;
    ext.push_back(std::string("peaks"));
    Mantid::API::FileProperty fProp("Filename","",FileProperty::Load,ext, Mantid::Kernel::Direction::Input );
    fProp.setValue("Ni1172A.peaks");

    PeaksWorkspace pw;

  }

//    std::string infile(fProp.value());
//
//    TS_ASSERT_THROWS_NOTHING( pw.append(infile));
//
//    std::string outfile( infile);
//
//
//    outfile.append("1");
//    removeFile( outfile );
//    TS_ASSERT_THROWS_NOTHING( pw.write(outfile ));
//
//    TS_ASSERT( sameFileContents(infile,outfile));
//
//    TS_ASSERT_EQUALS( removeFile( outfile ),0);
//
//    //Check base data read in correctly
//    V3D hkl=pw.get_hkl(6);
//    V3D test(5,3,-3);
//    V3D save(hkl);
//    hkl -= test;
//    TS_ASSERT_LESS_THAN(hkl.norm(),.00001);
//    TS_ASSERT( save==pw.get_hkl(6));
//
//
//    V3D position =pw.getPosition(6);
//    V3D ptest;
//    ptest.spherical(.45647,1.3748*180/M_PI,2.52165*180/M_PI);
//    V3D psave(position);
//
//    position -=ptest;
//    TS_ASSERT_LESS_THAN(position.norm(),.001);
//    TS_ASSERT( psave==pw.getPosition(6));
//
//    TS_ASSERT_LESS_THAN(abs( 187.25-pw.get_column(6)),.05 );
//    TS_ASSERT_LESS_THAN(abs( 121.29-pw.get_row(6)),.05 );
//    TS_ASSERT_LESS_THAN(abs( 283.13-pw.get_time_channel(6)),.05 );
//    TS_ASSERT_LESS_THAN(abs( 17-pw.getPeakCellCount(6)),.05 );
//    TS_ASSERT_LESS_THAN(abs( 4571.82-pw.getPeakIntegrationCount(6)),.05 );
//    TS_ASSERT_LESS_THAN(abs( 88.13-pw.getPeakIntegrationError(6)),.01 );
//    TS_ASSERT_LESS_THAN(abs(10 -pw.getReflag(6)),.001 );
//
//
//    TS_ASSERT_EQUALS( 1172,pw.cell<int>(6,pw.IrunNumCol) );
//    TS_ASSERT_EQUALS(3 , pw.get_Bank(6));
//    TS_ASSERT_LESS_THAN(abs( 10000-pw.getMonitorCount(6)),.1 );
//
//    TS_ASSERT_LESS_THAN( abs(18-pw.get_L1(6)),.0001);
//    TS_ASSERT_LESS_THAN( abs(0-pw.get_time_offset(6)),.001);
//
//    V3D sampOrient = pw.getSampleOrientation(6);
//    V3D soSave(sampOrient);
//    V3D soTest(164.96,45,0);
//    soTest *=M_PI/180;
//    sampOrient -=soTest;
//    TS_ASSERT_LESS_THAN( sampOrient.norm(), .001);
//    TS_ASSERT( soSave ==pw.getSampleOrientation(6) );
//
//    TS_ASSERT_LESS_THAN( abs(.5203-pw.get_dspacing(6)),.001);
//
//    TS_ASSERT_LESS_THAN( abs(.660962-pw.get_wavelength(6)),.001);
//
//    TS_ASSERT_LESS_THAN( abs(1/.5203-pw.get_Qmagnitude(6)),.004);
//
//
//    V3D Qlab= pw.get_Qlab(6);
// //  McStas==back,up,beam
//    V3D QlabTest(-1.2082262,.8624681,-1.220807);
//    Qlab -=QlabTest;
//    TS_ASSERT_LESS_THAN( Qlab.norm(), .001);
//
//    V3D QlabR= pw.get_QXtal(6);
//     //  McStas==back,up,beam
//    V3D QlabRSave = V3D(QlabR);
//    V3D QlabRTest(.55290407,1.4642019,1.1155452);
//     QlabR -=QlabRTest;
//     TS_ASSERT_LESS_THAN( QlabR.norm(), .001);
//
//     Matrix<double> mat(3,3);
//       int beam =2;
//       int up =1;
//       int back =0;
//       int base1=0;
//       int base2 = 1;
//       int base3 =2;
//       mat[beam][base1]=-0.36622801;
//       mat[back][base1]=-0.17730089;
//       mat[up][base1]=-0.29209167;
//       mat[beam][base2]=-0.52597409;
//       mat[back][base2]=0.20688301;
//       mat[up][base2]=0.00279823;
//       mat[beam][base3]=-0.15912011;
//       mat[back][base3]=0.36190018;
//       mat[up][base3]=-0.28579932;
//
//      pw.sethkls( mat, .1, false, 1);
//      Matrix<double>matSave = Matrix<double>(mat);
//      mat.Invert();
//      QlabRSave = pw.get_QXtal(6);
//
//      V3D hklD =  (mat*QlabRSave)-pw.get_hkl(6);
//      TS_ASSERT_LESS_THAN( hklD.norm() ,.001);
//      TS_ASSERT_EQUALS( pw.getReflag(6),10);
//
//      pw.sethkls( matSave, .1, true, 2);
//      hklD =  (mat*QlabRSave)-pw.get_hkl(6);
//      TS_ASSERT_LESS_THAN( hklD.norm() ,.001);
//      TS_ASSERT_EQUALS( pw.getReflag(6),10);
//
//
//      pw.sethkls( matSave, .01, false, 1);
//      TS_ASSERT_EQUALS( pw.get_hkl(6).norm(),0);
//      TS_ASSERT_EQUALS( pw.getReflag(6),0);
//
//      pw.sethkls( matSave, .1, true, 2);
//      hklD =  (mat*QlabRSave)-pw.get_hkl(6);
//      TS_ASSERT_LESS_THAN( hklD.norm() ,.001);
//      TS_ASSERT_EQUALS( pw.getReflag(6),20);
//
//     //Now test out the various sets. Not all sets are possible.
//     V3D testV(3,5,-6);
//     pw.sethkl( testV, 6);
//     TS_ASSERT( pw.get_hkl(6)==testV);
//
//     pw.setPeakCount(23,6);
//     TS_ASSERT_EQUALS( pw.getPeakCellCount(6),23);
//
//     pw.setPeakIntegrateCount( 235,6);
//     TS_ASSERT_EQUALS( pw.getPeakIntegrationCount(6),235);
//
//     //add set row, col ,chan, time
//
//     pw.setPeakIntegrateError( 15,6);
//     TS_ASSERT_EQUALS(pw.getPeakIntegrationError(6),15 );
//
//
//     pw.setReflag(35,6);
//     TS_ASSERT_EQUALS( pw.getReflag(6) , 35);
//
//     V3D pos(12,3,-5);
//     pw.setPeakPos( pos, 6);
//     TS_ASSERT_EQUALS( pw.getPosition(6), pos);
//
//     pw.setTime( 1280,6);
//     TS_ASSERT_EQUALS( pw.getTime( 6), 1280 );
//
//     pw.setRowColChan( 5,8,200,6);
//     TS_ASSERT_EQUALS( pw.get_row(6) , 5);
//     TS_ASSERT_EQUALS( pw.get_column(6) , 8);
//     TS_ASSERT_EQUALS( pw.get_time_channel(6) , 200);
//  }



  void test_constructor()
  {
    PeaksWorkspace pw;
    //pw.addPeak(p);
  }
};


#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */

