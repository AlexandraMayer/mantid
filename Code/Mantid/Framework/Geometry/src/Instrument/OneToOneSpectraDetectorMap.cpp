//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidKernel/Exception.h"

#include <boost/make_shared.hpp>
#include <iostream>
#include <sstream>

namespace Mantid
{
  namespace Geometry
  {

    /**
     * Default constructor builds an empty map
     */
    OneToOneSpectraDetectorMap::OneToOneSpectraDetectorMap()
    {
      clear();
    }

    /**
     * Constructor taking a start and end
     * @param start :: The starting spectra/detector ID (inclusive)
     * @param end :: The end spectra/detector ID (inclusive)
     */
    OneToOneSpectraDetectorMap::OneToOneSpectraDetectorMap(const specid_t start, const specid_t end)
      : m_start(start), m_end(end)
    {
    }

    /**
     * Clone the current map. Note that this resets the iterator back to the beginning
     */
    OneToOneSpectraDetectorMap * 
    OneToOneSpectraDetectorMap::clone() const 
    { 
      return new OneToOneSpectraDetectorMap(*this); 
    }
   
    /**
     * Get a vector of detectors ids contributing to a spectrum.
     * @param spectrumNo :: The spectrum number (unused)
     * @returns A vector containing the spectrumNumber itself
     * @throws std::out_of_range If the number is greater than the number
     * of elements in the map
     */
    std::vector<detid_t> 
    OneToOneSpectraDetectorMap::getDetectors(const specid_t spectrumNo) const
    {
      if( isValid(spectrumNo) )
      {
        return std::vector<detid_t>(1, detid_t(spectrumNo));
      }
      else
      {
        std::ostringstream msg;
        msg << "OneToOneSpectraDetectorMap::getDetectors - Spectrum " << spectrumNo << " out of range "
            << "[" << m_start << "," << m_end << "]";
        throw std::out_of_range(msg.str());
      }
    }
    
    /**
     * Gets a list of spectra corresponding to a list of detector numbers
     * @param detectorList :: If the list contains elements that are within the map's range
     * then the element is returned
     * @throws std::invalid_argument if the supplied list contains a number outside the
     * range.
     */
    std::vector<specid_t> 
    OneToOneSpectraDetectorMap::getSpectra(const std::vector<detid_t>& detectorList) const
    {
      const size_t nElements(detectorList.size());
      std::vector<specid_t> spectra(nElements);
      for(size_t i = 0; i < nElements; ++i)
      {
        specid_t spectrumNo = static_cast<specid_t>(detectorList[i]);
        if( isValid(spectrumNo) )
        {
          spectra[i] = spectrumNo;
        }
        else
        {
          std::ostringstream msg;
          msg << "OneToOneSpectraDetectorMap::getSpectra - Detector ID " << detectorList[i] 
              << " out of range. " << "[" << m_start << "," << m_end << "]";
          throw std::invalid_argument(msg.str());
        }
      }
      return spectra;
    }
    
    /**
     * Create a map between a single ID & and single ID as this implentation is 1:1
     * @returns A mapping from a single ID to a collection of IDs
     */
    boost::shared_ptr<det2group_map> OneToOneSpectraDetectorMap::createIDGroupsMap() const
    {
      auto mapping = boost::make_shared<det2group_map>();
      for(specid_t i = m_start; i <= m_end; ++i)
      {
        mapping->insert(std::make_pair(i, std::vector<detid_t>(i)));
      }
      return mapping;
    }

    /**
     * Return an iterator pointing at the first element
     * @returns A ISpectraDetectorMap::const_iterator pointing at the first element
     */
    ISpectraDetectorMap::const_iterator OneToOneSpectraDetectorMap::cbegin() const
    {
      return ISpectraDetectorMap::const_iterator(new OneToOneProxy(m_start));
    }

    /**
     * Return an iterator pointing at one past the element
     * @returns A ISpectraDetectorMap::const_iterator pointing at one past the 
     * last element
     */
    ISpectraDetectorMap::const_iterator OneToOneSpectraDetectorMap::cend() const
    {
      return ISpectraDetectorMap::const_iterator(new OneToOneProxy(m_end+1));
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------
    /**
     * Checks if the given spectrum is in range
     * @param spectrumNo :: A spectrum number
     * @returns True if the spectrum number is within the map's range, false otherwise
     */
    bool OneToOneSpectraDetectorMap::isValid(const specid_t spectrumNo) const
    {
      return (spectrumNo >= m_start && spectrumNo <= m_end);
    }

  }
}
