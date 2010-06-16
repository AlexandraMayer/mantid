#ifndef SPECTRADETECTORMAPTEST_H_
#define SPECTRADETECTORMAPTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidKernel/Exception.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class SpectraDetectorMapTest : public CxxTest::TestSuite
{
public:
  SpectraDetectorMapTest()
  {
    offset = 100000;
    length = 100;
    populateSDMap(sdMap,length,offset);
  }

  void testPopulate()
  {
    TS_ASSERT_EQUALS(sdMap.nElements(),length);
  }

  void testAddSpectrumEntries()
  {
    //use my own local sdmap as I will be altering it
    SpectraDetectorMap sdMapLocal;
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),0);
    std::vector<int> dets;
    dets.push_back(10);
    dets.push_back(20);
    TS_ASSERT_THROWS_NOTHING(sdMapLocal.addSpectrumEntries(1,dets));
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),2);
    TS_ASSERT_EQUALS(sdMapLocal.ndet(1),2);
    std::vector<int> detsOut = sdMapLocal.getDetectors(1);
    TS_ASSERT_EQUALS(detsOut.size(),2);
    TS_ASSERT_EQUALS(detsOut[0],10);
    TS_ASSERT_EQUALS(detsOut[1],20);
//    //Overloaded to add a single detector for a single pixel
//    TS_ASSERT_THROWS_NOTHING(sdMapLocal.addSpectrumEntries(3, 12));
//    std::vector<int> detsOut3 = sdMapLocal.getDetectors(3);
//    TS_ASSERT_EQUALS(detsOut3.size(),1);
//    TS_ASSERT_EQUALS(detsOut3[0],12);
  }
  
  void testClear()
  {
    //use my own local sdmap as I will be altering it
    SpectraDetectorMap sdMapLocal;
    populateSDMap(sdMapLocal,length,offset);
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),length);
    
    TS_ASSERT_THROWS_NOTHING(sdMapLocal.clear());
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),0);
  }
  
  void testNdet()
  {
    for (int i = 0; i < length; i++)
    {
      TS_ASSERT_EQUALS(sdMap.ndet(offset+i),1);
    }
  }

  void testGetDetectors()
  {
    for (int i = 0; i < length; i++)
    {
      std::vector<int> dvec = sdMap.getDetectors(offset+i);
      TS_ASSERT_EQUALS(dvec.size(),1);
      TS_ASSERT_EQUALS(dvec[0],i);
    }
  }

  void testRemap()
  {
    //use my own local sdmap as I will be altering it
    SpectraDetectorMap sdMapLocal;
    populateSDMap(sdMapLocal,length,offset);

    TS_ASSERT_EQUALS(sdMapLocal.nElements(),length);

    //remap to a new spectra that doesn't exist -> no action
    sdMapLocal.remap(offset,offset+length+1);
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),length);

    //remap to a new spectra that does exist
    sdMapLocal.remap(offset,offset+1);
    TS_ASSERT_EQUALS(sdMapLocal.ndet(offset),0);
    TS_ASSERT_EQUALS(sdMapLocal.ndet(offset+1),2);
  }


  void testGetSpectra()
  {
    //create a vector of detectorids to map
    std::vector<int> dets;
    int detLength=20;
    for (int i = 0; i < detLength; i++)
    {
      dets.push_back(i);
    }

    //remap them
    std::vector<int> spectra = sdMap.getSpectra(dets);
    for (int i = 0; i < detLength; i++)
    {
      TS_ASSERT_EQUALS(spectra[i],dets[i]+offset);
    }
  }

  void testOperatorEquals()
  {
    TS_ASSERT( sdMap == sdMap )
  }

  void testOperatorNotEquals()
  {
    SpectraDetectorMap sdMapLocal;
    TS_ASSERT( sdMap != sdMapLocal )
  }

private:
  void populateSDMap(SpectraDetectorMap& sdMap, int length, const int offset)
  {
    int* udet = new int [length];
    int* spec = new int [length];
    for (int i = 0; i < length; i++)
    {
      spec[i] = i+offset;
      udet[i] = i;
    }
    sdMap.populate(spec,udet,length);
    delete [] spec;
    delete [] udet;
  } 

  SpectraDetectorMap sdMap;
  int offset;
  int length;
};

#endif /*SPECTRADETECTORMAPTEST_H_*/
