#ifndef MANTID_API_SPECTRADETECTORMAP_H_
#define MANTID_API_SPECTRADETECTORMAP_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/ISpectraDetectorMap.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/cow_ptr.h"

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

namespace Mantid
{

#ifndef HAS_UNORDERED_MAP_H
  /// Map with key = spectrum number, value = workspace index
  typedef std::map<specid_t,size_t> spec2index_map;
  /// Map with key = workspace index, value = spectrum number
  typedef std::map<size_t, specid_t> index2spec_map;
#else
  /// Map with key = spectrum number, value = workspace index
  typedef std::tr1::unordered_map<specid_t,size_t> spec2index_map;
  /// Map with key = workspace index, value = spectrum number
  typedef std::tr1::unordered_map<size_t,specid_t> spec2index_map;
#endif

  namespace API
  {
    /** SpectraDetectorMap provides a multimap between Spectra number (int)
	and detector ID (UDET). For efficiency, an unordered_multimaop is used. The TR1/unordered_map
	header is not included in MVSC++ Express Edition so an alternative with multimap is
	provided.

	@author Laurent C Chapon, ISIS, RAL
	@date 29/04/2008

	Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

	This file is part of Mantid.

	Mantid is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	Mantid is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
	Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport SpectraDetectorMap : public Geometry::ISpectraDetectorMap
    {
    public:
      // The cow_ptr in which SpectraDetectorMap is held in Workspace needs access to the copy constructor
      friend class Kernel::cow_ptr<SpectraDetectorMap>;
  
#ifndef HAS_UNORDERED_MAP_H
      /// Spectra Detector map typedef
      typedef std::multimap<specid_t,detid_t> smap;
      /// Spectra Detector map iterator typedef
      typedef std::multimap<specid_t,detid_t>::const_iterator smap_it;
#else
      /// Spectra Detector map typedef
      typedef std::tr1::unordered_multimap<specid_t,detid_t> smap;
      /// Spectra Detector map iterator typedef
      typedef std::tr1::unordered_multimap<specid_t,detid_t>::const_iterator smap_it;
#endif
  
      /// Constructor
      SpectraDetectorMap();
      /// Virtual destructor
      virtual ~SpectraDetectorMap();
      /// Populate the Map with _spec and _udet C array
      void populate(const specid_t* _spec, const detid_t* _udet, int64_t nentries);
      /// Populate with a simple 1-1 correspondance between spec and udet; 
      /// from start (inclusive) to end (exclusive).
      void populateSimple(const detid_t start, const detid_t end);
      /// Populate with a vector of pixel IDs
      void populateWithVector(const std::vector<detid_t>& udetList);

      /// Link a list of UDETs to the given spectrum
      void addSpectrumEntries(const specid_t spectrum, const std::vector<detid_t>& udetList);
      void addSpectrumEntries(const specid_t spectrum, const std::set<detid_t>& detectorIDs);

      /// Move a detector from one spectrum to another
      void remap(const specid_t oldSpectrum, const specid_t newSpectrum);
      /// Empties the map
      void clear();
      /// Return number of detectors contributing to this spectrum
      std::size_t ndet(const specid_t spectrum_number) const;
      /// Get a vector of detectors ids contributing to a spectrum
      std::vector<detid_t> getDetectors(const specid_t spectrum_number) const;
      /// Gets a list of spectra corresponding to a list of detector numbers
      std::vector<specid_t> getSpectra(const std::vector<detid_t>& detectorList) const;
      /// Return the size of the map
      std::size_t nElements() const {return m_s2dmap.size();}
      /// Tests two maps for equality
      bool operator==(const SpectraDetectorMap& other) const;
      /// Tests two maps for inequality
      bool operator!=(const SpectraDetectorMap& other) const;

      /**@name Iterate over the whole map */
      //@{
      /// Setup the map for iteration from the beginning
      virtual void moveIteratorToStart() const;
      /// Returns whether a next element exists
      virtual bool hasNext() const;
      /// Advance the iterator to the next element
      virtual void advanceIterator() const;
      /// Returns the current element of the sequence
      virtual specid_t getCurrentSpectrum() const;
      //@}

    private:
      /// Copy Contructor
      SpectraDetectorMap(const SpectraDetectorMap& copy);
      /// internal spectra detector map instance
      smap m_s2dmap;
      /// intermal iterator
      mutable smap_it m_citr;
      /// Static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace API
} // namespace Mantid

#endif /*MANTID_API_SPECTRADETECTORMAP_H_*/
