#ifndef TIMESERIESPROPERTYTEST_H_
#define TIMESERIESPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>
#include <ctime>
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/DateAndTime.h"
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid::Kernel;

class TimeSeriesPropertyTest : public CxxTest::TestSuite
{
public:
  void setUp()
  {
    iProp = new TimeSeriesProperty<int>("intProp");
    dProp = new TimeSeriesProperty<double>("doubleProp");
    sProp = new TimeSeriesProperty<std::string>("stringProp");
  }

  void tearDown()
  {
    delete iProp;
    delete dProp;
    delete sProp;
  }

  void test_Constructor()
  {
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT( ! iProp->name().compare("intProp") );
    TS_ASSERT( ! iProp->documentation().compare("") );
    TS_ASSERT( typeid( std::multimap<DateAndTime, int> ) == *iProp->type_info()  );
    //TS_ASSERT( iProp->isDefault() )

    TS_ASSERT( ! dProp->name().compare("doubleProp") );
    TS_ASSERT( ! dProp->documentation().compare("") );
    TS_ASSERT( typeid( std::multimap<DateAndTime, double> ) == *dProp->type_info()  );
    ///TS_ASSERT( dProp->isDefault() )

    TS_ASSERT( ! sProp->name().compare("stringProp") );
    TS_ASSERT( ! sProp->documentation().compare("") );
    TS_ASSERT( typeid( std::multimap<DateAndTime, std::string> ) == *sProp->type_info()  );
    //TS_ASSERT( sProp->isDefault() )
  }

  void test_SetValue()
  {
    TS_ASSERT_THROWS( iProp->setValue("1"), Exception::NotImplementedError );
    TS_ASSERT_THROWS( dProp->setValue("5.5"), Exception::NotImplementedError );
    TS_ASSERT_THROWS( sProp->setValue("aValue"), Exception::NotImplementedError );
  }

  void test_AddValue()
  {
    const std::string tester("2007-11-30T16:17:00");
    TS_ASSERT( iProp->addValue(tester,1) );
    TS_ASSERT( iProp->addValue("2007-11-30T16:17:10",1) );

    TS_ASSERT( dProp->addValue("2007-11-30T16:17:00",9.99) );
    TS_ASSERT( dProp->addValue("2007-11-30T16:17:10",5.55) );

    TS_ASSERT( sProp->addValue("2007-11-30T16:17:00","test") );
    TS_ASSERT( sProp->addValue("2007-11-30T16:17:10","test2") );

    //Now try the other overloads
    TimeSeriesProperty<int> otherProp("otherProp");
    TS_ASSERT( otherProp.addValue( static_cast<std::time_t>( 123 ), 1) );
    TS_ASSERT( otherProp.addValue( boost::posix_time::second_clock::local_time(), 1) );

    const std::string dString = dProp->value();
    TS_ASSERT_EQUALS( dString.substr(0,27), "2007-Nov-30 16:17:00  9.99\n" );
    const std::string iString = iProp->value();
    TS_ASSERT_EQUALS( iString.substr(0,24), "2007-Nov-30 16:17:00  1\n" );
    const std::string sString = sProp->value();
    TS_ASSERT_EQUALS( sString.substr(0,27), "2007-Nov-30 16:17:00  test\n" );
  }

  void test_Casting()
  {
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(iProp), static_cast<Property*>(0) );
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(dProp), static_cast<Property*>(0) );
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(sProp), static_cast<Property*>(0) );
  }


  //----------------------------------------------------------------------------
  void test_AdditionOperator()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );

    TimeSeriesProperty<int> * log2  = new TimeSeriesProperty<int>("MyIntLog2");
    TS_ASSERT( log2->addValue("2007-11-30T16:18:00",3) );
    TS_ASSERT( log2->addValue("2007-11-30T16:18:10",4) );
    TS_ASSERT( log2->addValue("2007-11-30T16:18:11",5) );

    TS_ASSERT_EQUALS( log->size(), 2);

    //Concatenate the lists
    (*log) += log2;

    TS_ASSERT_EQUALS( log->size(), 5);
  }

  //----------------------------------------------------------------------------
  /// Ticket 2097: This caused an infinite loop
  void test_AdditionOperatorOnYourself()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );

    (*log) += log;
    // There is now a check and trying to do this does nothing.
    TS_ASSERT_EQUALS( log->size(), 2);
  }

  //----------------------------------------------------------------------------
  void test_filterByTime_and_getTotalValue()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );

    TS_ASSERT_EQUALS( log->realSize(), 6);
    TS_ASSERT_EQUALS( log->getTotalValue(), 21);
    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");

    //Since the filter is < stop, the last one is not counted, so there are  3 taken out.

    log->filterByTime(start, stop);
    TS_ASSERT_EQUALS( log->realSize(), 3);
    TS_ASSERT_EQUALS( log->getTotalValue(), 9);
  }


  //----------------------------------------------------------------------------
  /// Ticket #2591
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);

    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    log->filterByTime(start, stop);

    // Still there!
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);
  }


  //----------------------------------------------------------------------------
  /// Ticket #2591
  void test_filterByTime_ifOnlyOneValue_assumes_constant_instead_2()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("1990-01-01",1) );
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);

    DateAndTime start = DateAndTime("2007-11-30T16:17:10");
    DateAndTime stop = DateAndTime("2007-11-30T16:17:40");
    log->filterByTime(start, stop);

    // Still there!
    TS_ASSERT_EQUALS( log->realSize(), 1);
    TS_ASSERT_EQUALS( log->getTotalValue(), 1);
  }



  //----------------------------------------------------------------------------
  void test_makeFilterByValue()
  {
    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",2.0) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",2.01) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );

    TS_ASSERT_EQUALS( log->realSize(), 6);

    TimeSplitterType splitter;
    log->makeFilterByValue(splitter, 1.8, 2.2, 1.0);

    TS_ASSERT_EQUALS( splitter.size(), 2);
    SplittingInterval s;
    DateAndTime t;

    s = splitter[0];
    t = DateAndTime("2007-11-30T16:17:09");
    TS_ASSERT_DELTA( s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:11");
    TS_ASSERT_DELTA( s.stop(), t, 1e-3);

    s = splitter[1];
    t = DateAndTime("2007-11-30T16:17:29");
    TS_ASSERT_DELTA( s.start(), t, 1e-3);
    t = DateAndTime("2007-11-30T16:17:41");
    TS_ASSERT_DELTA( s.stop(), t, 1e-3);


  }




  //----------------------------------------------------------------------------
  void test_splitByTime_and_getTotalValue()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:00",7) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:10",8) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:20",9) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:30",10) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:40",11) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:50",12) );
    TS_ASSERT_EQUALS( log->realSize(), 12);

    //Make the outputs
    //    std::vector< TimeSeriesProperty<int> * > outputs2;
    std::vector< Property * > outputs;
    for (std::size_t i=0; i<5; i++)
    {
      TimeSeriesProperty<int> * newlog  = new TimeSeriesProperty<int>("MyIntLog");
      outputs.push_back(newlog);
      //outputs2.push_back(newlog);
    }

    //Make a splitter
    DateAndTime start, stop;
    TimeSplitterType splitter;
    start = DateAndTime("2007-11-30T16:17:10");
    stop = DateAndTime("2007-11-30T16:17:40");
    splitter.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:55");
    stop = DateAndTime("2007-11-30T16:17:56");
    splitter.push_back( SplittingInterval(start, stop, 1) );

    start = DateAndTime("2007-11-30T16:17:56");
    stop = DateAndTime("2007-11-30T16:18:01");
    splitter.push_back( SplittingInterval(start, stop, 2) ); //just one entry

    start = DateAndTime("2007-11-30T16:18:09");
    stop = DateAndTime("2007-11-30T16:18:21");
    splitter.push_back( SplittingInterval(start, stop, 3) );

    start = DateAndTime("2007-11-30T16:18:45");
    stop = DateAndTime("2007-11-30T16:22:50");
    splitter.push_back( SplittingInterval(start, stop, 4) );

    log->splitByTime(splitter, outputs);

    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[0])->realSize(), 3);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[1])->realSize(), 0);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[2])->realSize(), 1);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[3])->realSize(), 2);
    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[4])->realSize(), 1);

  }


  //----------------------------------------------------------------------------
  void test_splitByTime_withOverlap()
  {
    TimeSeriesProperty<int> * log  = new TimeSeriesProperty<int>("MyIntLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:00",7) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:10",8) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:20",9) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:30",10) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:40",11) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:50",12) );
    TS_ASSERT_EQUALS( log->realSize(), 12);

    //Make the outputs
    //    std::vector< TimeSeriesProperty<int> * > outputs2;
    std::vector< Property * > outputs;
    for (std::size_t i=0; i<1; i++)
    {
      TimeSeriesProperty<int> * newlog  = new TimeSeriesProperty<int>("MyIntLog");
      outputs.push_back(newlog);
    }

    //Make a splitter
    DateAndTime start, stop;
    TimeSplitterType splitter;
    start = DateAndTime("2007-11-30T16:17:10");
    stop = DateAndTime("2007-11-30T16:17:40");
    splitter.push_back( SplittingInterval(start, stop, 0) );

    start = DateAndTime("2007-11-30T16:17:35");
    stop = DateAndTime("2007-11-30T16:17:59");
    splitter.push_back( SplittingInterval(start, stop, 0) );

    log->splitByTime(splitter, outputs);

    TS_ASSERT_EQUALS( dynamic_cast< TimeSeriesProperty<int> * >(outputs[0])->realSize(), 5);

  }

  //----------------------------------------------------------------------------
  void test_statistics()
  {
    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
    TS_ASSERT( log->addValue("2007-11-30T16:17:00",1) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:10",2) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:20",3) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:30",4) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:40",5) );
    TS_ASSERT( log->addValue("2007-11-30T16:17:50",6) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:00",7) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:10",8) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:20",9) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:30",10) );
    TS_ASSERT( log->addValue("2007-11-30T16:18:40",11) );
    TS_ASSERT_EQUALS( log->realSize(), 11);

    TimeSeriesPropertyStatistics stats = log->getStatistics();

    TS_ASSERT_DELTA( stats.minimum, 1.0, 1e-3);
    TS_ASSERT_DELTA( stats.maximum, 11.0, 1e-3);
    TS_ASSERT_DELTA( stats.median, 6.0, 1e-3);
    TS_ASSERT_DELTA( stats.mean, 6.0, 1e-3);
    TS_ASSERT_DELTA( stats.duration, 100.0, 1e-3);
    TS_ASSERT_DELTA( stats.standard_deviation, 3.1622, 1e-3);
  }

  void test_empty_statistics()
  {
    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
    TimeSeriesPropertyStatistics stats = log->getStatistics();
    TS_ASSERT( boost::math::isnan(stats.minimum) );
    TS_ASSERT( boost::math::isnan(stats.maximum) );
    TS_ASSERT( boost::math::isnan(stats.median) );
    TS_ASSERT( boost::math::isnan(stats.mean) );
    TS_ASSERT( boost::math::isnan(stats.standard_deviation) );
    TS_ASSERT( boost::math::isnan(stats.duration) );
  }

  void test_PlusEqualsOperator_Incompatible_Types_dontThrow()
  {
    // Adding incompatible types together should not throw, but issue a warning in the log

    TimeSeriesProperty<double> * log  = new TimeSeriesProperty<double>("MydoubleLog");
    TimeSeriesProperty<int> * logi  = new TimeSeriesProperty<int>("MyIntLog");
    PropertyWithValue<double> * val  = new PropertyWithValue<double>("MySimpleDouble", 1.23);

    log->operator+=(val);
    log->operator+=(logi);
    logi->operator+=(log);
    val->operator+=(log);
    val->operator+=(logi);
  }


private:
  TimeSeriesProperty<int> *iProp;
  TimeSeriesProperty<double> *dProp;
  TimeSeriesProperty<std::string> *sProp;
};

#endif /*TIMESERIESPROPERTYTEST_H_*/
