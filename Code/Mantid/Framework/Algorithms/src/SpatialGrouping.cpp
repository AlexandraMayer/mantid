//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SpatialGrouping.h"

#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"

#include "MantidAPI/FileProperty.h"

#include <map>

#include <fstream>
#include <iostream>

#include <algorithm>

namespace
{
/*
* Comparison operator for use in std::sort when dealing with a vector of
* std::pair<int,double> where int is DetectorID and double is distance from
* centre point.
* Needs to be outside of SpatialGrouping class because of the way STL handles
* passing functions as arguments.
* @param left :: element to compare
* @param right :: element to compare
* @return true if left should come before right in the order
*/
static bool compareIDPair(const std::pair<int64_t,double> & left, const std::pair<int64_t,double> & right)
{
  return ( left.second < right.second );
}
}

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpatialGrouping)

/// Sets documentation strings for this algorithm
void SpatialGrouping::initDocs()
{
  this->setWikiSummary(" This algorithm creates an XML grouping file, which can be used in [[GroupDetectors]] or [[ReadGroupsFromFile]], which groups the detectors of an instrument based on the distance between the detectors. It does this by querying the [http://doxygen.mantidproject.org/classMantid_1_1Geometry_1_1Detector.html#a3abb2dd5dca89d759b848489360ff9df getNeighbours] method on the Detector object. ");
  this->setOptionalMessage("This algorithm creates an XML grouping file, which can be used in GroupDetectors or ReadGroupsFromFile, which groups the detectors of an instrument based on the distance between the detectors. It does this by querying the getNeighbours method on the Detector object.");
}


/**
* init() method implemented from Algorithm base class
*/
void SpatialGrouping::init()
{
  declareProperty(new Mantid::API::WorkspaceProperty<>("InputWorkspace","",Mantid::Kernel::Direction::Input),"The input workspace.");
  declareProperty(new Mantid::API::FileProperty("Filename", "", Mantid::API::FileProperty::Save, ".xml"));
  declareProperty("SearchDistance", 2.5, Mantid::Kernel::Direction::Input);
  declareProperty("GridSize", 3, Mantid::Kernel::Direction::Input);
}

/**
* exec() method implemented from Algorithm base class
*/
void SpatialGrouping::exec()
{
  Mantid::API::MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  double searchDist = getProperty("SearchDistance");
  m_pix = searchDist;
  int gridSize = getProperty("GridSize");
  int nNeighbours = ( gridSize * gridSize ) - 1;
  
  Mantid::Geometry::IInstrument_sptr m_instrument = inputWorkspace->getInstrument();
  m_instrument->getDetectors(m_detectors);
  
  Mantid::API::Progress prog(this, 0.0, 1.0, m_detectors.size());
    
  for ( std::map<detid_t, Mantid::Geometry::IDetector_sptr>::iterator detIt = m_detectors.begin(); detIt != m_detectors.end(); ++detIt )
  {
    prog.report();

    // We are not interested in Monitors and we don't want them to be included in
    // any of the other lists
    if ( detIt->second->isMonitor() )
    {
      m_included[detIt->first] = false;
      continue;
    }

    // Or detectors already flagged as included in a group
    std::map<detid_t, bool>::iterator inclIt = m_included.find(detIt->first);
    if ( inclIt != m_included.end() )
    {
      continue;
    }

    std::map<detid_t, double> nearest;

    const double empty = EMPTY_DBL();
    Mantid::Geometry::V3D scale(empty,empty,empty);
    Mantid::Geometry::BoundingBox bbox(empty,empty,empty,empty,empty,empty);

    createBox(detIt->second, bbox, scale);

    bool extend = true;
    while ( ( nNeighbours > static_cast<detid_t>(nearest.size()) ) && extend )
    {
      extend = expandNet(nearest, detIt->second, nNeighbours, bbox, scale);
    }

    if ( static_cast<detid_t>(nearest.size()) != nNeighbours ) continue;

    // if we've gotten to this point, we want to go and make the group list.
    std::vector<int> group;
    m_included[detIt->first] = true;
    group.push_back(detIt->first);
    std::map<detid_t, double>::iterator nrsIt;
    for ( nrsIt = nearest.begin(); nrsIt != nearest.end(); ++nrsIt )
    {
      m_included[nrsIt->first] = true;
      group.push_back(nrsIt->first);
    }
    m_groups.push_back(group);
  }

  if ( m_groups.size() == 0 )
  {
    g_log.warning() << "No groups generated." << std::endl;
    return;
  }

  // Create grouping XML file
  g_log.information() << "Creating XML Grouping File." << std::endl;
  std::vector<std::vector<detid_t> >::iterator grpIt;
  std::ofstream xml;
  std::string fname = getPropertyValue("Filename");
  
  // Check to see whether we need to append .xml to the name.
  size_t fnameXMLappend = fname.find(".xml");
  if ( fnameXMLappend == std::string::npos )
  {
    fnameXMLappend = fname.find(".XML"); // check both 'xml' and 'XML'
    if ( fnameXMLappend == std::string::npos )
      fname = fname + ".xml";
  }

  // set the property again so the user can retrieve the stored result.
  setPropertyValue("Filename", fname);

  xml.open(fname.c_str());

  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
    << "<!-- XML Grouping File created by SpatialGrouping Algorithm -->\n"
    << "<detector-grouping>\n";

  int grpID = 1;
  for ( grpIt = m_groups.begin(); grpIt != m_groups.end(); ++grpIt )
  {
    xml << "<group name=\"group" << grpID++ << "\"><detids val=\"" << (*grpIt)[0];
    for ( size_t i = 1; i < (*grpIt).size(); i++ )
    {
      xml << "," << (*grpIt)[i];
    }
    xml << "\"/></group>\n";
  }

  xml << "</detector-grouping>";

  xml.close();

  g_log.information() << "Finished creating XML Grouping File." << std::endl;

}

/**
* This method will, using the NearestNeighbours methods, expand our view on the nearby detectors from
* the standard eight closest that are recorded in the graph.
* @param nearest :: neighbours found in previous requests
* @param det :: pointer to the central detector, for calculating distances
* @param noNeighbours :: number of neighbours that must be found (in total, including those already found)
* @param bbox :: BoundingBox object representing the search region
* @param scale :: V3D object used for scaling in determination of distances
* @return true if neighbours were found matching the parameters, false otherwise
*/
bool SpatialGrouping::expandNet(std::map<detid_t,double> & nearest, Mantid::Geometry::IDetector_sptr det,
    const size_t & noNeighbours, const Mantid::Geometry::BoundingBox & bbox, const Mantid::Geometry::V3D & scale)
{
  const detid_t incoming = nearest.size();

  const detid_t centDetID = det->getID();
  std::map<detid_t, double> potentials;

  // Special case for first run for this detector
  if ( incoming == 0 )
  {
    potentials = det->getNeighbours();
  }
  else
  {
    for ( std::map<detid_t,double>::iterator nrsIt = nearest.begin(); nrsIt != nearest.end(); ++nrsIt )
    {
      std::map<detid_t, double> results;
      results = m_detectors[nrsIt->first]->getNeighbours();
      for ( std::map<detid_t, double>::iterator resIt = results.begin(); resIt != results.end(); ++resIt )
      {
        potentials[resIt->first] = resIt->second;
      }
    }
  }

  for ( std::map<detid_t,double>::iterator potIt = potentials.begin(); potIt != potentials.end(); ++potIt )
  {
    // We do not want to include the detector in it's own list of nearest neighbours
    if ( potIt->first == centDetID ) { continue; }

    // Or detectors that are already in the nearest list passed into this function
    std::map<detid_t, double>::iterator nrsIt = nearest.find(potIt->first);
    if ( nrsIt != nearest.end() ) { continue; }

    // We should not include detectors already included in a group (or monitors for that matter)
    std::map<detid_t,bool>::iterator inclIt = m_included.find(potIt->first);
    if ( inclIt != m_included.end() ) { continue; }

    // If we get this far, we need to determine if the detector is of a suitable distance
    Mantid::Geometry::V3D pos = m_detectors[potIt->first]->getPos();
    if ( ! bbox.isPointInside(pos) ) { continue; }
    
    // Add any that have survived to this point to the nearest
    // But first we want to "scale" the distance attribute
    pos -= det->getPos();
    pos /= scale;
    double distance = pos.norm();
    
    nearest[potIt->first] = distance;
  }

  if ( static_cast<detid_t>(nearest.size()) == incoming ) { return false; }

  if ( static_cast<detid_t>(nearest.size()) > noNeighbours )
  {
    sortByDistance(nearest, noNeighbours);
  }

  return true;
}

/**
* This method will trim the result set down to the specified number required by sorting
* the results and removing those that are the greatest distance away.
* @param input :: map of values that need to be sorted, will be modified by the method
* @param noNeighbours :: number of elements that should be kept
*/
void SpatialGrouping::sortByDistance(std::map<detid_t,double> & input, const size_t & noNeighbours)
{
  std::vector<std::pair<detid_t,double> > order(input.begin(), input.end());

  std::sort(order.begin(), order.end(), compareIDPair);

  size_t current = order.size();
  size_t lose = current - noNeighbours;
  if ( lose < 1 ) return;

  for ( size_t i = 1; i <= lose; i++ )
  {
    input.erase(order[current-i].first);
  }

}
/**
* Creates a bounding box representing the area in which to search for neighbours, and a scaling vector representing the dimensions
* of the detector
* @param det :: input detector
* @param bndbox :: reference to BoundingBox object (changed by this function)
* @param scale :: reference to V3D object (changed by this function)
*/
void SpatialGrouping::createBox(boost::shared_ptr<Mantid::Geometry::IDetector> det, Mantid::Geometry::BoundingBox & bndbox, Mantid::Geometry::V3D & scale)
{
  boost::shared_ptr<Mantid::Geometry::Detector> detector = boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(det);
  
  Mantid::Geometry::BoundingBox bbox;
  detector->getBoundingBox(bbox);
    
  double xmax = bbox.xMax();
  double ymax = bbox.yMax();
  double zmax = bbox.zMax();
  double xmin = bbox.xMin();
  double ymin = bbox.yMin();
  double zmin = bbox.zMin();

  scale.setX((xmax-xmin));
  scale.setY((ymax-ymin));
  scale.setZ((zmax-zmin));

  double factor = 2.0 * m_pix;

  growBox(xmin, xmax, factor);
  growBox(ymin, ymax, factor);
  growBox(zmin, zmax, factor);

  bndbox = Mantid::Geometry::BoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
}

/**
* Enlarges values given by a certain factor. Used to "grow" the BoundingBox object.
* @param min :: min value (changed by this function)
* @param max :: max value (changed by this function)
* @param factor :: factor by which to grow the values
*/
void SpatialGrouping::growBox(double & min, double & max, const double & factor)
{
  double rng = max - min;
  double mid = ( max + min ) / 2.0;
  double halfwid = rng / 2.0;
  min = mid - ( factor * halfwid );
  max = mid + ( factor * halfwid );
}

} // namespace Algorithms
} // namespace Mantid
