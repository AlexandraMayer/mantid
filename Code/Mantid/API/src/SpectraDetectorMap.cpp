#include "MantidKernel/Exception.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <iostream>

namespace Mantid
{
  namespace API
  {

    // Get a reference to the logger
    Kernel::Logger& SpectraDetectorMap::g_log = Kernel::Logger::get("SpectraDetectorMap");

    SpectraDetectorMap::SpectraDetectorMap() : m_s2dmap()
    {}

    SpectraDetectorMap::SpectraDetectorMap(const SpectraDetectorMap& copy) : m_s2dmap(copy.m_s2dmap)
    {}

    SpectraDetectorMap::~SpectraDetectorMap()
    {}

    void SpectraDetectorMap::populate(const int* _spectable, const int* _udettable, int nentries)
    {
      m_s2dmap.clear();
      if (nentries<=0)
      {
        g_log.error("Populate : number of entries should be > 0");
        throw std::invalid_argument("Populate : number of entries should be > 0");
      }
      for (int i=0; i<nentries; ++i)
      {
        m_s2dmap.insert(std::pair<int,int>(*_spectable,*_udettable)); // Insert current detector with Spectra number as key 
        ++_spectable;
        ++_udettable;
      }
      return;
    }

    void SpectraDetectorMap::addSpectrumEntries(const int spectrum, const std::vector<int>& udetList)
    {
      std::vector<int>::const_iterator it;
      for (it = udetList.begin(); it != udetList.end(); ++it)
      {
        m_s2dmap.insert(std::pair<int,int>(spectrum,*it));
      }
    }
    
    /** Moves all detectors assigned to a particular spectrum number to a different one.
    *  Does nothing if the oldSpectrum number does not exist in the map.
    *  @param oldSpectrum The spectrum number to be removed and have its detectors reassigned
    *  @param newSpectrum The spectrum number to map the detectors to
    */
    void SpectraDetectorMap::remap(const int oldSpectrum, const int newSpectrum)
    {
      // Do nothing if the two spectrum numbers given are the same
      if (oldSpectrum == newSpectrum) return;
      // For now at least, the newSpectrum must already exist in the map
      if ( ndet(newSpectrum) == 0 )
      {
        g_log.error("Invalid value of newSpectrum. It is forbidden to create a new spectrum number with this method.");
        return;
      }
      // Get the list of detectors that contribute to the old spectrum
      std::vector<int> dets = getDetectors(oldSpectrum);

      // Add them to the map with the new spectrum number as the key
      std::vector<int>::const_iterator it;
      for (it = dets.begin(); it != dets.end(); ++it)
      {
        m_s2dmap.insert( std::pair<int,int>(newSpectrum,*it) );
      }
      // Finally, remove the old spectrum number from the map
      m_s2dmap.erase(oldSpectrum);
    }

    /// Empties the map - use with care!
    void SpectraDetectorMap::clear()
    {
      m_s2dmap.clear();
    }
    
    const int SpectraDetectorMap::ndet(const int spectrum_number) const
    {
      return m_s2dmap.count(spectrum_number);
    }

    std::vector<int> SpectraDetectorMap::getDetectors(const int spectrum_number) const
    {
      std::vector<int> detectors;

      if ( ! ndet(spectrum_number) )
      {
        // Will just return an empty vector
        return detectors;
      }
      std::pair<smap_it,smap_it> det_range=m_s2dmap.equal_range(spectrum_number);
      for (smap_it it=det_range.first; it!=det_range.second; ++it)
      {
        detectors.push_back(it->second);
      }
      return detectors;
    }

    /** Gets a list of spectra corresponding to a list of detector numbers.
    *  @param detectorList A list of detector Ids
    *  @return A vexctor where matching indices correspond to the relevant spectra id
    */
    std::vector<int> SpectraDetectorMap::getSpectra(const std::vector<int>& detectorList) const
    {
      std::vector<int> spectraList;
      spectraList.reserve(detectorList.size());

      //invert the sdmap into a dsMap
      std::multimap<int,int> dsMap;  
      for (smap_it it = m_s2dmap.begin();  it != m_s2dmap.end(); ++it)
      {
        std::pair<int,int> valuePair(it->second,it->first);
        dsMap.insert(valuePair);
      }

      //use the dsMap to translate the detectorlist and populate the spectralist
      for (std::vector<int>::const_iterator it = detectorList.begin();  it != detectorList.end(); ++it)
      {
        try
        {
          std::multimap<int,int>::iterator found = dsMap.find(*it);
          int spectra = found != dsMap.end()? found->second : 0;
          spectraList.push_back(spectra);
        }
        catch (std::runtime_error&)
        {
          //spectra was not found enter 0
          spectraList.push_back(0);
        }
      }
      return spectraList;
    }


  } // Namespace API 
} // Namespace Mantid
