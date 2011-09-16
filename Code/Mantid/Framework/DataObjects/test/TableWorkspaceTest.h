#ifndef TESTTABLEWORKSPACE_
#define TESTTABLEWORKSPACE_

#include <vector> 
#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/TableWorkspace.h" 
#include "MantidAPI/TableRow.h" 
#include "MantidAPI/ColumnFactory.h" 

class Class
{
public:
    int d;
    Class():d(0){} 
private:
    Class(const Class&);
};

DECLARE_TABLEPOINTERCOLUMN(Class,Class);

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;


template<class T> 
class TableColTestHelper:public TableColumn<T>
{
public:
    TableColTestHelper<T>(T value){
        std::vector<T> &dat = TableColumn<T>::data();
        dat.resize(1);
        dat[0]=value;
    }

};


class TableWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testTCcast(){
      TableColTestHelper<float> Tcf(1.);
      float frez=(float)Tcf[0];
      TSM_ASSERT_DELTA("1 float not converted, type: "+std::string(typeid(frez).name()),1.,frez,1.e-5);

      TableColTestHelper<double> Tdr(1.);
      double drez=(double)Tcf[0];
      TSM_ASSERT_DELTA("2 double not converted, type: "+std::string(typeid(drez).name()),1.,drez,1.e-5);

      TableColTestHelper<int> Tci(1);
      int irez=(int)Tci[0];
      TSM_ASSERT_EQUALS("3 integer not converted, type: "+std::string(typeid(irez).name()),1,irez);

      TableColTestHelper<int64_t> Tcl(1);
      int64_t lrez=(int64_t)Tcl[0];
      TSM_ASSERT_EQUALS("4 int64_t not converted, type: "+std::string(typeid(lrez).name()),1,lrez);

      TableColTestHelper<long> Tcls(1);
      long  lsrez = (long)Tcls[0];
      TSM_ASSERT_EQUALS("5 long not converted, type: "+std::string(typeid(lsrez).name()),1,lsrez);

     TableColTestHelper<size_t> Tcst(1);
      size_t  strez = (size_t)Tcst[0];
      TSM_ASSERT_EQUALS("6 size_t not converted, type: "+std::string(typeid(strez).name()),1,lsrez);

//
      TableColTestHelper<float> Tcf2(-1.);
      frez=(float)Tcf2[0];
      TSM_ASSERT_DELTA("7 float not converted, type: "+std::string(typeid(float).name()),-1.,frez,1.e-5);

      TableColTestHelper<double> Tdr2(-1.);
      drez=(double)Tcf2[0];
      TSM_ASSERT_DELTA("8 double not converted, type: "+std::string(typeid(double).name()),-1.,drez,1.e-5);

      TableColTestHelper<int> Tci2(-1);
      irez=(int)Tci2[0];
      TSM_ASSERT_EQUALS("9 integer not converted, type: "+std::string(typeid(int).name()),-1,irez);

      TableColTestHelper<int64_t> Tcl2(-1);
      lrez=(int64_t)Tcl2[0];
      TSM_ASSERT_EQUALS("10 int64_t not converted, type: "+std::string(typeid(int64_t).name()),-1,lrez);

      TableColTestHelper<long> Tcls2(-1);
      lsrez = (long)Tcls2[0];
      TSM_ASSERT_EQUALS("11 long not converted, type: "+std::string(typeid(long).name()),-1,lsrez);

  }
  void testAll()
  {
    TableWorkspace tw(3);
    tw.addColumn("int","Number");
    tw.addColumn("str","Name");
    tw.addColumn("V3D","Position");
    tw.addColumn("Class","class");

    TS_ASSERT_EQUALS(tw.rowCount(),3);
    TS_ASSERT_EQUALS(tw.columnCount(),4);

    tw.getRef<int>("Number",1) = 17;
    tw.cell<std::string>(2,1) = "STRiNG";

    ColumnVector<int> cNumb = tw.getVector("Number");
    TS_ASSERT_EQUALS(cNumb[1],17);

    ColumnVector<string> str = tw.getVector("Name");
    TS_ASSERT_EQUALS(str.size(),3);
    TS_ASSERT_EQUALS(str[2],"STRiNG");

    ColumnVector<Class> cl = tw.getVector("class");
    TS_ASSERT_EQUALS(cl.size(),3);

    for(int i=0;i<cNumb.size();i++)
        cNumb[i] = i+1;

    tw.insertRow(2);
    cNumb[2] = 4;
    TS_ASSERT_EQUALS(tw.rowCount(),4);
    TS_ASSERT_EQUALS(cNumb[3],3);

    tw.setRowCount(10);
    TS_ASSERT_EQUALS(tw.rowCount(),10);
    TS_ASSERT_EQUALS(cNumb[3],3);

    tw.removeRow(3);
    TS_ASSERT_EQUALS(tw.rowCount(),9);
    TS_ASSERT_EQUALS(cNumb[3],0);

    tw.setRowCount(2);
    TS_ASSERT_EQUALS(tw.rowCount(),2);
    TS_ASSERT_EQUALS(cNumb[1],2);

    str[0] = "First"; str[1] = "Second";
    cl[0].d = 11; cl[1].d = 22;

    vector<string> names;
    names.push_back("Number");
    names.push_back("Name");
    names.push_back("class");

  }

  void testRow()
  {
    TableWorkspace tw(2);
    tw.addColumn("int","Number");
    tw.addColumn("double","Ratio");
    tw.addColumn("str","Name");
    tw.addColumn("bool","OK");

    TableRow row = tw.getFirstRow();

    TS_ASSERT_EQUALS(row.row(),0);

    row << 18 << 3.14 << "FIRST";

    TS_ASSERT_EQUALS(tw.Int(0,0),18);
    TS_ASSERT_EQUALS(tw.Double(0,1),3.14);
    TS_ASSERT_EQUALS(tw.String(0,2),"FIRST");

    if (row.next())
    {
        row << 36 << 6.28 << "SECOND";
    }

    int i;
    double r;
    std::string str;
    row.row(1);
    row>>i>>r>>str;

    TS_ASSERT_EQUALS(i,36);
    TS_ASSERT_EQUALS(r,6.28);
    TS_ASSERT_EQUALS(str,"SECOND");

    for(int i=0;i<5;i++)
    {
        TableRow row = tw.appendRow();
        int j = row.row();
        std::ostringstream ostr;
        ostr<<"Number "<<j;
        row << 18*j << 3.14*j << ostr.str() << (j%2 == 0);
    }

    TS_ASSERT_EQUALS(tw.rowCount(),7);

    TableRow row1 = tw.getRow(2);
    TS_ASSERT_EQUALS(row1.row(),2);

    do
    {
        TS_ASSERT_EQUALS(row1.Int(0),row1.row()*18);
        TS_ASSERT_EQUALS(row1.Double(1),row1.row()*3.14);
        std::istringstream istr(row1.String(2));
        std::string str;
        int j;
        istr>>str>>j;
        TS_ASSERT_EQUALS(str,"Number");
        TS_ASSERT_EQUALS(j,row1.row());
        row1.Bool(3) = !tw.Bool(row1.row(),3);
        TS_ASSERT_EQUALS(tw.Bool(row1.row(),3), static_cast<Mantid::API::Boolean>(row1.row()%2 != 0));
    }while(row1.next());

  }

  void testOutOfRange()
  {
      TableWorkspace tw(2);
      tw.addColumn("str","Name");
      tw.addColumn("int","Number");
      TS_ASSERT_THROWS(tw.String(0,1),std::runtime_error);
      TS_ASSERT_THROWS(tw.Int(0,3),std::range_error);
      TS_ASSERT_THROWS(tw.Int(3,1),std::range_error);

      {
        TableRow row = tw.appendRow();
        TS_ASSERT_THROWS(row << "One" << 1 << 2,std::range_error);
      }

      {
        std::string str;
        int i;
        double d;
        TableRow row = tw.getFirstRow();
        TS_ASSERT_THROWS(row >> str >> i >> d,std::range_error);
      }

      {
        TableRow row = tw.getFirstRow();
        TS_ASSERT_THROWS(row.row(3),std::range_error);
      }
  }

  void testBoolean()
  {
  try
  {
      TableWorkspace tw(10);
      tw.addColumn("int","Number");
      tw.addColumn("bool","OK");

      TableRow row = tw.getFirstRow();
      do
      {
          int i = row.row();
          row << i << (i % 2 == 0);
      }
      while(row.next());

      TableColumn_ptr<bool> bc = tw.getColumn("OK");

      //std::vector<bool>& bv = bc->data();  // doesn't work
      //std::vector<Boolean>& bv = bc->data();  // works
      //bool& br = bc->data()[1]; // doesn't work
      //bool b = bc->data()[1]; // works
      bc->data()[1] = true;

      for(int i=0;i<tw.rowCount();i++)
      {
//          std::cerr<<bc->data()[i]<<'\n';
      }
  }
  catch(std::exception& e)
  {
      std::cerr<<"Error: "<<e.what()<<'\n';
  }
  }
  void testFindMethod()
  {
      TableWorkspace tw;
      tw.addColumn("str","Name");
      tw.addColumn("str","Format");
      tw.addColumn("str","Format Version");
      tw.addColumn("str","Format Type");
      tw.addColumn("str","Create Time");
     
       for (int i=1;i<10;++i)
      {
         std::stringstream s;
         TableRow t = tw.appendRow();
          s<<i;
          std::string name="Name";
          name+=s.str();
         
          t<<name;
          std::string format="Format";
          format+=s.str();
          t<<format;
          std::string formatver="Format Version"; 
          formatver+=s.str();
          t<<formatver;
          std::string formattype="Format Type";
          formattype+=s.str();
          t<<formattype;
          std::string creationtime="Creation Time";
          creationtime+=s.str();
          t<<creationtime;

      }
      std::string searchstr="Name3";
      int row=0;const int col=0;
      tw.find(searchstr,row,col);
      TS_ASSERT_EQUALS(row,2);

      const int formatcol=2;
      searchstr="Format Version8";
      tw.find(searchstr,row,formatcol);
      TS_ASSERT_EQUALS(row,7);

  }

  void testClone()
  {
    TableWorkspace tw(1);
      tw.addColumn("str","X");
      tw.addColumn("str","Y");
      tw.addColumn("str","Z");
   
    tw.getColumn(0)->cell<std::string>(0) = "a";
    tw.getColumn(1)->cell<std::string>(0) = "b";
    tw.getColumn(2)->cell<std::string>(0) = "c";

    boost::scoped_ptr<TableWorkspace> cloned(tw.clone());

    //Check clone is same as original.
    TS_ASSERT_EQUALS(tw.columnCount(), cloned->columnCount());
    TS_ASSERT_EQUALS(tw.rowCount(), cloned->rowCount());
    TS_ASSERT_EQUALS("a", cloned->getColumn(0)->cell<std::string>(0));
    TS_ASSERT_EQUALS("b", cloned->getColumn(1)->cell<std::string>(0));
    TS_ASSERT_EQUALS("c", cloned->getColumn(2)->cell<std::string>(0));

  }



};
#endif /*TESTTABLEWORKSPACE_*/
