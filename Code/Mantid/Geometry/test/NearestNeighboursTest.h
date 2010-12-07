#ifndef MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS
#define MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS

#include <cxxtest/TestSuite.h>

// Header for class we're testing
#include "MantidGeometry/Instrument/NearestNeighbours.h"

// other headers
#include "ComponentCreationHelpers.hh"

#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <map>

using namespace Mantid::Geometry;

/**
* Everything must be in one test or the instrument/detector list goes AWOL.
*/

class testNearestNeighbours : public CxxTest::TestSuite
{
public:
  void testNeighbourFinding()
  {
    // Create Instrument and make it Parameterised
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    ParameterMap_sptr pmap(new ParameterMap());
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));
    std::map<int, IDetector_sptr> m_detectors = m_instrument->getDetectors();

    // Check instrument was created to our expectations
    TS_ASSERT_THROWS_NOTHING(ParameterMap_sptr p_map = m_instrument->getParameterMap(););
    TS_ASSERT_EQUALS(m_detectors.size(), 18);

    // Check distances calculated in NearestNeighbours compare with those using getDistance on component
    std::map<int, double> distances = m_detectors[5]->getNeighbours();
    std::map<int, double>::iterator distIt;

    // We should have 8 neighbours when not specifying a range.
    TS_ASSERT_EQUALS(distances.size(), 8);

    for ( distIt = distances.begin(); distIt != distances.end(); ++distIt )
    {
      double nnDist = distIt->second;
      double gmDist = m_detectors[5]->getDistance(*(m_detectors[distIt->first]));
      TS_ASSERT_EQUALS(nnDist, gmDist);
    }

    // Check that the 'radius' option works as expected
    distances = m_detectors[14]->getNeighbours(0.0002);
    TS_ASSERT_EQUALS(distances.size(), 2);
  }
};

#endif /* MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS */