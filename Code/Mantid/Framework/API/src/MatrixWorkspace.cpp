#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IMDWorkspace.h" 
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidAPI/MatrixWSIndexCalculator.h"
#include "MantidKernel/PhysicalConstants.h"

#include <numeric>
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/DateAndTime.h"

using Mantid::Kernel::DateAndTime;
using Mantid::Kernel::TimeSeriesProperty;

namespace Mantid
{
  namespace API
  {
    using std::size_t;
    using namespace Geometry;
    using Kernel::V3D;

    Kernel::Logger& MatrixWorkspace::g_log = Kernel::Logger::get("MatrixWorkspace");
    const std::string MatrixWorkspace::xDimensionId = "xDimension";
    const std::string MatrixWorkspace::yDimensionId = "yDimension";

    /// Default constructor
    MatrixWorkspace::MatrixWorkspace() : 
      IMDWorkspace(), ExperimentInfo(),
      m_axes(), m_isInitialized(false),
      m_spectraMap(new Geometry::OneToOneSpectraDetectorMap),
      m_YUnit(), m_YUnitLabel(), m_isDistribution(false),
      m_masks(), m_indexCalculator(),
      m_nearestNeighbours()
    {}

    /// Destructor
    // RJT, 3/10/07: The Analysis Data Service needs to be able to delete workspaces, so I moved this from protected to public.
    MatrixWorkspace::~MatrixWorkspace()
    {
      for (unsigned int i = 0; i < m_axes.size(); ++i)
      {
        delete m_axes[i];
      }
    }

    /** Initialize the workspace. Calls the protected init() method, which is implemented in each type of
    *  workspace. Returns immediately if the workspace is already initialized.
    *  @param NVectors :: The number of spectra in the workspace (only relevant for a 2D workspace
    *  @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
    *  @param YLength :: The number of data/error points in each vector (must all be the same)
    */
    void MatrixWorkspace::initialize(const size_t &NVectors, const size_t &XLength, const size_t &YLength)
    {
      // Check validity of arguments
      if (NVectors == 0 || XLength == 0 || YLength == 0)
      {
        g_log.error("All arguments to init must be positive and non-zero");
        throw std::out_of_range("All arguments to init must be positive and non-zero");
      }

      // Bypass the initialization if the workspace has already been initialized.
      if (m_isInitialized) return;

//      // Setup a default 1:1 spectra map that goes from 1->NVectors
//      // Do this before derived init so that it can be replaced if necessary
//      this->replaceSpectraMap(new Geometry::OneToOneSpectraDetectorMap(1,static_cast<specid_t>(NVectors)));

      // Invoke init() method of the derived class inside a try/catch clause
      try
      {
        this->init(NVectors, XLength, YLength);
      }
      catch(std::runtime_error& ex)
      {
        g_log.error() << "Error initializing the workspace" << ex.what() << std::endl;
        throw;
      }

      m_indexCalculator =  MatrixWSIndexCalculator(this->blocksize());
      // Indicate that this Algorithm has been initialized to prevent duplicate attempts.
      m_isInitialized = true;
    }


    //---------------------------------------------------------------------------------------
    /** Set the title of the workspace
     *
     *  @param t :: The title
     */
    void MatrixWorkspace::setTitle(const std::string& t)
    {
      Workspace::setTitle(t);
      
      // A MatrixWorkspace contains uniquely one Run object, hence for this workspace
      // keep the Run object run_title property the same as the workspace title
      m_run.access().addProperty("run_title",t, true);        
    }


    //---------------------------------------------------------------------------------------
    /** Get the workspace title
     *
     *  @return The title
     */
    const std::string MatrixWorkspace::getTitle() const
    {
      if ( m_run->hasProperty("run_title") )
      {
        std::string title = m_run->getProperty("run_title")->value();
        return title;
      }
      else      
        return Workspace::getTitle();
    }


    //---------------------------------------------------------------------------------------
    /** Get a const reference to the SpectraDetectorMap associated with this workspace.
    *  Can ONLY be taken as a const reference!
    *
    *  @return The SpectraDetectorMap
    */
    const Geometry::ISpectraDetectorMap& MatrixWorkspace::spectraMap() const
    {
      return *m_spectraMap;
    }

    //---------------------------------------------------------------------------------------
    /** Replace the current spectra map with a new one. This object takes ownership.
     * This will fill in the detector ID lists in each spectrum of the workspace
     * for backwards compatibility.
     *
     * @param spectraMap :: A pointer to a new SpectraDetectorMap.
     */
    void MatrixWorkspace::replaceSpectraMap(const Geometry::ISpectraDetectorMap * spectraMap)
    {
      //g_log.notice() << "MatrixWorkspace::replaceSpectraMap() is being deprecated." << std::endl;
      m_spectraMap.reset(spectraMap);
      // The neighbour map needs to be rebuilt
      m_nearestNeighbours.reset();
      try
      {
        this->updateSpectraUsingMap();
      }
      catch (std::exception &e)
      {
        g_log.error() << "Error in MatrixWorkspace::replaceSpectraMap(): " << e.what() << std::endl;
      }
    }

    /** Using the current spectraDetectorMap,
     * this will fill in the detector ID lists in each spectrum of the workspace
    * for backwards compatibility.
    */
    void MatrixWorkspace::updateSpectraUsingMap()
    {
      // The Axis1 needs to be set correctly for the ISpectraDetectorMap to make any sense
      if (m_axes.size() < 2) throw std::runtime_error("MatrixWorkspace::updateSpectraUsingMap() needs a SpectraAxis at index 1 to work.");
      SpectraAxis * ax1 = dynamic_cast<SpectraAxis *>(m_axes[1]);
      if (!ax1)
        throw std::runtime_error("MatrixWorkspace::updateSpectraUsingMap() needs a SpectraAxis at index 1 to work.");
      if (ax1->length() < this->getNumberHistograms())
        throw std::runtime_error("MatrixWorkspace::updateSpectraUsingMap(): the SpectraAxis is shorter than the number of histograms! Cannot run.");

      for (size_t wi=0; wi < this->getNumberHistograms(); wi++)
      {
        specid_t specNo = ax1->spectraNo(wi);

        ISpectrum * spec = getSpectrum(wi);
        spec->setSpectrumNo(specNo);

        std::vector<detid_t> dets = m_spectraMap->getDetectors(specNo);
        spec->clearDetectorIDs();
        spec->addDetectorIDs(dets);
      }
    }


    //---------------------------------------------------------------------------------------
    /**
     * Rebuild the default spectra mapping for a workspace. If a non-empty
     * instrument is set then the default maps each detector to a spectra with
     * the same ID. If an empty instrument is set then a 1:1 map from 1->NHistograms
     * is created. If axis one contains a spectra axis then this method also
     * rebuilds this axis to match the generated mapping.
     * @param includeMonitors :: If false the monitors are not included
     */
    void MatrixWorkspace::rebuildSpectraMapping(const bool includeMonitors)
    {
      if( sptr_instrument->nelements() == 0 )
      {
        return;
      }

      SpectraDetectorMap *spectramap = new SpectraDetectorMap;
      std::vector<detid_t> pixelIDs = this->getInstrument()->getDetectorIDs(!includeMonitors);

      if( m_axes.size() > 1 && m_axes[1]->isSpectra() )
      {
        delete m_axes[1];
        m_axes[1] = new SpectraAxis(pixelIDs.size(), false);
      }
      else
      {
        if (m_axes.size() == 0) m_axes.push_back( new NumericAxis(this->blocksize()) );
        m_axes.push_back(  new SpectraAxis(pixelIDs.size(), false) );
      }

      try
      {
        size_t index = 0;
        std::vector<detid_t>::const_iterator iend = pixelIDs.end();
        for( std::vector<detid_t>::const_iterator it = pixelIDs.begin();
             it != iend; ++it )
        {
          // The detector ID
          const detid_t detId = *it;
          // By default: Spectrum number = index +  1
          const specid_t specNo = specid_t(index + 1);
          // We keep the entry in the spectraDetectorMap. TODO: Deprecate spectraDetectorMap entirely.
          spectramap->addSpectrumEntries(specNo, std::vector<detid_t>(1, detId));

          // Also set the spectrum number in the axis(1). TODO: Remove this, it is redundant (but it is stuck everywhere)
          m_axes[1]->setValue(index, specNo);

          if (index < this->getNumberHistograms())
          {
            ISpectrum * spec = getSpectrum(index);
            spec->setSpectrumNo(specNo);
            spec->setDetectorID(detId);
          }

          index++;
        }

        // equivalent of replaceSpectraMap TODO: DEPRECATE
        m_spectraMap.reset(spectramap);
        m_nearestNeighbours.reset();

      }
      catch (std::runtime_error & e)
      {
        g_log.error() << "MatrixWorkspace::rebuildSpectraMapping() error:" << std::endl;
        throw e;
      }

    }




    //---------------------------------------------------------------------------------------
    /** Working off the spectrum numbers and
     * lists of detector IDs in each spectrum,
     * create the SpectraDetectorMap, and the axis(1).
     *
     * For BACKWARDS-COMPATIBILITY.
     *
     */
    void MatrixWorkspace::generateSpectraMap()
    {
      // We create a spectra-type axis that holds the spectrum # at each workspace index.

      if( m_axes.size() > 1 )
      {
        delete m_axes[1];
        m_axes[1] = new SpectraAxis(this->getNumberHistograms(), false);
      }
      else
        m_axes.push_back(  new SpectraAxis(this->getNumberHistograms(), false) );

      API::Axis *ax1 = getAxis(1);
      API::SpectraDetectorMap *newMap = new API::SpectraDetectorMap;

      //Go through all the spectra
      for (size_t wi=0; wi<this->getNumberHistograms(); wi++)
      {
        ISpectrum * spec = getSpectrum(wi);
        specid_t specNo = spec->getSpectrumNo();
        newMap->addSpectrumEntries(specNo, spec->getDetectorIDs());
        ax1->setValue(wi, specNo);
        //std::cout << "generateSpectraMap : wi " << wi << " specNo " << specNo << " detID " << *spec->getDetectorIDs().begin() << std::endl;
      }

      // Equivalent of replaceSpectraMap(newMap);
      m_spectraMap.reset(newMap);
      m_nearestNeighbours.reset();
    }



    //---------------------------------------------------------------------------------------
    /**
     * Handles the building of the NearestNeighbours object, if it has not already been
     * populated for this parameter map.
     * @param comp :: Object used for determining the Instrument
     */
    void MatrixWorkspace::buildNearestNeighbours(const IComponent *comp) const
    {
      if( !m_spectraMap )
      {
        throw Kernel::Exception::NullPointerException("MatrixWorkspace::buildNearestNeighbours",
                  "SpectraDetectorMap");
      }

      if ( !m_nearestNeighbours )
      {
        // Get pointer to Instrument
        boost::shared_ptr<const IComponent> parent(comp, NoDeleting());
        while ( parent->getParent() )
        {
          parent = parent->getParent();
        }
        boost::shared_ptr<const Instrument> inst = boost::dynamic_pointer_cast<const Instrument>(parent);
        if ( inst )
        {
          m_nearestNeighbours.reset(new NearestNeighbours(inst, *m_spectraMap));
        }
        else
        {
          throw Mantid::Kernel::Exception::NullPointerException("ParameterMap: buildNearestNeighbours",
                parent->getName());
        }
      }
    }

    //---------------------------------------------------------------------------------------
    /**
     * Queries the NearestNeighbours object for the selected detector.
     * @param comp :: pointer to the querying detector
     * @param radius :: distance from detector on which to filter results
     * @return map of DetectorID to distance for the nearest neighbours
     */
    std::map<specid_t, double> MatrixWorkspace::getNeighbours(const IDetector *comp, const double radius) const
    {
      if ( !m_nearestNeighbours )
      {
        buildNearestNeighbours(comp);
      }
      // Find the spectrum number
      std::vector<specid_t> spectra;
      this->getSpectraFromDetectorIDs(std::vector<detid_t>(1, comp->getID()), spectra);
      if(spectra.empty())
      {
        throw Kernel::Exception::NotFoundError("MatrixWorkspace::getNeighbours - Cannot find spectrum number for detector", comp->getID());
      }
      std::map<specid_t, double> neighbours = m_nearestNeighbours->neighbours(spectra[0], radius);
      return neighbours;
    }









    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the Workspace Index
    *    VALUE is the Spectrum #
    */
    index2spec_map * MatrixWorkspace::getWorkspaceIndexToSpectrumMap() const
    {
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToSpectrumMap: axis[1] is not a SpectraAxis, so I cannot generate a map.");
      index2spec_map * map = new index2spec_map();
      try
      {
        ax->getIndexSpectraMap(*map);
      }
      catch (std::runtime_error &)
      {
        delete map;
        throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToSpectrumMap: no elements!");
      }
      return map;
    }

    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the Spectrum #
    *    VALUE is the Workspace Index
    */
    spec2index_map * MatrixWorkspace::getSpectrumToWorkspaceIndexMap() const
    {
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: axis[1] is not a SpectraAxis, so I cannot generate a map.");
      spec2index_map * map = new spec2index_map();
      try
      {
        ax->getSpectraIndexMap(*map);
      }
      catch (std::runtime_error &)
      {
        delete map;
        throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: no elements!");
      }
      return map;
    }

    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the DetectorID (pixel ID)
    *    VALUE is the Workspace Index
    *  @param throwIfMultipleDets :: set to true to make the algorithm throw an error
    *         if there is more than one detector for a specific workspace index.
    *  @throw runtime_error if there is more than one detector per spectrum (if throwIfMultipleDets is true)
    *  @return Index to Index Map object

    */
    detid2index_map * MatrixWorkspace::getDetectorIDToWorkspaceIndexMap( bool throwIfMultipleDets ) const
    {
      if (this->m_axes.size() < 2)
      {
        throw std::runtime_error("MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(): axis[1] does not exist, so I cannot generate a map.");
      }
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(): axis[1] is not a SpectraAxis, so I cannot generate a map.");

      detid2index_map * map = new detid2index_map();
      //Loop through the workspace index
      for (size_t workspaceIndex=0; workspaceIndex < this->getNumberHistograms(); workspaceIndex++)
      {
        //Get the spectrum # from the WS index
        specid_t specNo = ax->spectraNo(workspaceIndex);

        //Now the list of detectors
        std::vector<detid_t> detList = m_spectraMap->getDetectors(specNo);
        if (throwIfMultipleDets)
        {
          if (detList.size() > 1)
          {
            delete map;
            throw std::runtime_error("MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(): more than 1 detector for one histogram! I cannot generate a map of detector ID to workspace index.");
          }

          //Set the KEY to the detector ID and the VALUE to the workspace index.
          if (detList.size() == 1)
            (*map)[ detList[0] ] = workspaceIndex;
        }
        else
        {
          //Allow multiple detectors per workspace index
          for (std::vector<detid_t>::iterator it = detList.begin(); it != detList.end(); it++)
            (*map)[ *it ] = workspaceIndex;
        }

        //Ignore if the detector list is empty.
      }
      return map;
    }


    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the Workspace Index
    *    VALUE is the DetectorID (pixel ID)
    *  @throw runtime_error if there is more than one detector per spectrum, or other incompatibilities.
    *  @return Map of workspace index to detector/pixel id.
    */
    index2detid_map * MatrixWorkspace::getWorkspaceIndexToDetectorIDMap() const
    {
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToDetectorIDMap: axis[1] is not a SpectraAxis, so I cannot generate a map.");

      index2detid_map * map = new index2detid_map();
      //Loop through the workspace index
      for (size_t workspaceIndex=0; workspaceIndex < this->getNumberHistograms(); workspaceIndex++)
      {
        //Get the spectrum # from the WS index
        specid_t specNo = ax->spectraNo(workspaceIndex);

        //Now the list of detectors
        std::vector<detid_t> detList = this->m_spectraMap->getDetectors(specNo);
        if (detList.size() > 1)
        {
          delete map;
          throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToDetectorIDMap(): more than 1 detector for one histogram! I cannot generate a map of workspace index to detector ID.");
        }

        //Set the KEY to the detector ID and the VALUE to the workspace index.
        if (detList.size() == 1)
          (*map)[workspaceIndex] = detList[0];

        //Ignore if the detector list is empty.
      }
      return map;
    }


    //---------------------------------------------------------------------------------------
    /** Converts a list of spectrum numbers to the corresponding workspace indices.
    *  Not a very efficient operation, but unfortunately it's sometimes required.
    *
    *  @param spectraList :: The list of spectrum numbers required
    *  @param indexList ::   Returns a reference to the vector of indices (empty if not a Workspace2D)
    */
    void MatrixWorkspace::getIndicesFromSpectra(const std::vector<specid_t>& spectraList, std::vector<size_t>& indexList) const
    {
      // Clear the output index list
      indexList.clear();
      indexList.reserve(this->getNumberHistograms());

      std::vector<specid_t>::const_iterator iter = spectraList.begin();
      while( iter != spectraList.end() )
      {
        for (size_t i = 0; i < this->getNumberHistograms(); ++i)
        {
          if ( this->getSpectrum(i)->getSpectrumNo() == *iter )
          {
            indexList.push_back(i);
            break;
          }
        }
        ++iter;
      }
    }

    //---------------------------------------------------------------------------------------
    /** Given a spectrum number, find the corresponding workspace index
    *
    * @param specNo :: spectrum number wanted
    * @return the workspace index
    * @throw runtime_error if not found.
    */
    size_t MatrixWorkspace::getIndexFromSpectrumNumber(const specid_t specNo) const
    {
      for (size_t i = 0; i < this->getNumberHistograms(); ++i)
      {
        if ( this->getSpectrum(i)->getSpectrumNo() == specNo )
          return i;
      }
      throw std::runtime_error("Could not find spectrum number in any spectrum.");
    }


    //---------------------------------------------------------------------------------------
    /** Converts a list of detector IDs to the corresponding workspace indices.
     * Might be slow!
     * This is optimized for few detectors in the list vs the number of histograms.
     *
     *  @param detIdList :: The list of detector IDs required
     *  @param indexList :: Returns a reference to the vector of indices
     */
    void MatrixWorkspace::getIndicesFromDetectorIDs(const std::vector<detid_t>& detIdList, std::vector<size_t>& indexList) const
    {
      std::vector<detid_t>::const_iterator it_start = detIdList.begin();
      std::vector<detid_t>::const_iterator it_end = detIdList.end();

      indexList.clear();

      // Try every detector in the list
      std::vector<detid_t>::const_iterator it;
      for (it = it_start; it != it_end; it++)
      {
        bool foundDet = false;
        size_t foundWI = 0;

        // Go through every histogram
        for (size_t i=0; i<this->getNumberHistograms(); i++)
        {
          if (this->getSpectrum(i)->hasDetectorID(*it))
          {
            foundDet = true;
            foundWI = i;
            break;
          }
        }

        if (foundDet)
          indexList.push_back(foundWI);
      } // for each detector ID in the list
    }


    //---------------------------------------------------------------------------------------
    /** Converts a list of detector IDs to the corresponding spectrum numbers. Might be slow!
     *
     * @param detIdList :: The list of detector IDs required
     * @param spectraList :: Returns a reference to the vector of spectrum numbers.
     *                       0 for not-found detectors
     */
    void MatrixWorkspace::getSpectraFromDetectorIDs(const std::vector<detid_t>& detIdList, std::vector<specid_t>& spectraList) const
    {
      std::vector<detid_t>::const_iterator it_start = detIdList.begin();
      std::vector<detid_t>::const_iterator it_end = detIdList.end();

      spectraList.clear();

      // Try every detector in the list
      std::vector<detid_t>::const_iterator it;
      for (it = it_start; it != it_end; it++)
      {
        bool foundDet = false;
        specid_t foundSpecNum = 0;

        // Go through every histogram
        for (size_t i=0; i<this->getNumberHistograms(); i++)
        {
          if (this->getSpectrum(i)->hasDetectorID(*it))
          {
            foundDet = true;
            foundSpecNum = this->getSpectrum(i)->getSpectrumNo();
            break;
          }
        }

        if (foundDet)
          spectraList.push_back(foundSpecNum);
      } // for each detector ID in the list
    }


    //---------------------------------------------------------------------------------------
    /** Integrate all the spectra in the matrix workspace within the range given.
     * Default implementation, can be overridden by base classes if they know something smarter!
     *
     * @param out :: returns the vector where there is one entry per spectrum in the workspace. Same
     *            order as the workspace indices.
     * @param minX :: minimum X bin to use in integrating.
     * @param maxX :: maximum X bin to use in integrating.
     * @param entireRange :: set to true to use the entire range. minX and maxX are then ignored!
     */
    void MatrixWorkspace::getIntegratedSpectra(std::vector<double> & out, const double minX, const double maxX, const bool entireRange) const
    {
      out.resize(this->getNumberHistograms(), 0.0);

      //Run in parallel if the implementation is threadsafe
      PARALLEL_FOR_IF( this->threadSafe() )
      for (int wksp_index = 0; wksp_index < static_cast<int>(this->getNumberHistograms()); wksp_index++)
      {
        // Get Handle to data
        const Mantid::MantidVec& x=this->readX(wksp_index);
        const Mantid::MantidVec& y=this->readY(wksp_index);
        // If it is a 1D workspace, no need to integrate
        if ((x.size()<=2) && (y.size() >= 1))
        {
          out[wksp_index] = y[0];
        }
        else
        {
          // Iterators for limits - whole range by default
          Mantid::MantidVec::const_iterator lowit, highit;
          lowit=x.begin();
          highit=x.end()-1;

          //But maybe we don't want the entire range?
          if (!entireRange)
          {
            // If the first element is lower that the xmin then search for new lowit
            if ((*lowit) < minX)
              lowit = std::lower_bound(x.begin(),x.end(),minX);
            // If the last element is higher that the xmax then search for new lowit
            if ((*highit) > maxX)
              highit = std::upper_bound(lowit,x.end(),maxX);
          }

          // Get the range for the y vector
          Mantid::MantidVec::difference_type distmin = std::distance(x.begin(), lowit);
          Mantid::MantidVec::difference_type distmax = std::distance(x.begin(), highit);
          double sum(0.0);
          if( distmin <= distmax )
          {
            // Integrate
            sum = std::accumulate(y.begin() + distmin,y.begin() + distmax,0.0);
          }
          //Save it in the vector
          out[wksp_index] = sum;
        }
      }
    }

    /** Get the effective detector for the given spectrum
    *  @param  index The workspace index for which the detector is required
    *  @return A single detector object representing the detector(s) contributing
    *          to the given spectrum number. If more than one detector contributes then
    *          the returned object's concrete type will be DetectorGroup.
    *  @throw  std::runtime_error if the SpectraDetectorMap has not been filled
    *  @throw  Kernel::Exception::NotFoundError if the SpectraDetectorMap or the Instrument
    do not contain the requested spectrum number of detector ID
    */
    Geometry::IDetector_const_sptr MatrixWorkspace::getDetector(const size_t workspaceIndex) const
    {
      const ISpectrum * spec = this->getSpectrum(workspaceIndex);
      if (!spec)
        throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): NULL spectrum found at the given workspace index.", "");

      const std::set<detid_t> dets = spec->getDetectorIDs();
      Instrument_const_sptr localInstrument = getInstrument();
      if( !localInstrument )
      {
        g_log.debug() << "No instrument defined.\n";
        throw Kernel::Exception::NotFoundError("Instrument not found", "");
      }

      const size_t ndets = dets.size();
      //std::cout << "MatrixWorkspace::getDetector() has " << ndets << std::endl;

      if ( ndets == 1 )
      {
        // If only 1 detector for the spectrum number, just return it
        return localInstrument->getDetector(*dets.begin());
      }
      else if (ndets==0)
      {
        throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No detectors for this workspace index.", "");
      }
      // Else need to construct a DetectorGroup and return that
      std::vector<Geometry::IDetector_const_sptr> dets_ptr = localInstrument->getDetectors(dets);
      return Geometry::IDetector_const_sptr( new Geometry::DetectorGroup(dets_ptr, false) );
    }

    /** Returns the 2Theta scattering angle for a detector
     *  @param det :: A pointer to the detector object (N.B. might be a DetectorGroup)
     *  @return The scattering angle (0 < theta < pi)
     *  @throws InstrumentDefinitionError if source or sample is missing, or they are in the same place
     */
    double MatrixWorkspace::detectorTwoTheta(Geometry::IDetector_const_sptr det) const
    {
      Geometry::IObjComponent_const_sptr source = getInstrument()->getSource();
      Geometry::IObjComponent_const_sptr sample = getInstrument()->getSample();
      if ( source == NULL || sample == NULL )
      {
        throw Kernel::Exception::InstrumentDefinitionError("Instrument not sufficiently defined: failed to get source and/or sample");
      }

      const Kernel::V3D samplePos = sample->getPos();
      const Kernel::V3D beamLine  = samplePos - source->getPos();

      if ( beamLine.nullVector() )
      {
        throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
      }

      return det->getTwoTheta(samplePos,beamLine);
    }

    /**Calculates the distance a neutron coming from the sample will have deviated from a
    *  straight tragetory before hitting a detector. If calling this function many times
    *  for the same detector you can call this function once, with waveLength=1, and use
    *  the fact drop is proportional to wave length squared .This function has no knowledge
    *  of which axis is vertical for a given instrument
    *  @param det :: the detector that the neutron entered
    *  @param waveLength :: the neutrons wave length in meters
    *  @return the deviation in meters
    */
    double MatrixWorkspace::gravitationalDrop(Geometry::IDetector_const_sptr det, const double waveLength) const
    {
      using namespace PhysicalConstants;
      /// Pre-factor in gravity calculation: gm^2/2h^2
      static const double gm2_OVER_2h2 = g*NeutronMass*NeutronMass/( 2.0*h*h );

      const V3D samplePos = getInstrument()->getSample()->getPos();
      const double pathLength = det->getPos().distance(samplePos);
      // Want L2 (sample-pixel distance) squared, times the prefactor g^2/h^2
      const double L2 = gm2_OVER_2h2*std::pow(pathLength,2);

      return waveLength*waveLength*L2;
    }



    //---------------------------------------------------------------------------------------
    /** Add parameters to the instrument parameter map that are defined in instrument
    *   definition file and for which logfile data are available. Logs must be loaded
    *   before running this method.
    */
    void MatrixWorkspace::populateInstrumentParameters()
    {
      // Get instrument and sample

      boost::shared_ptr<const Instrument> instrument = getBaseInstrument();
      Instrument* inst = const_cast<Instrument*>(instrument.get());

      // Get the data in the logfiles associated with the raw data

      const std::vector<Kernel::Property*>& logfileProp = run().getLogData();


      // Get pointer to parameter map that we may add parameters to and information about
      // the parameters that my be specified in the instrument definition file (IDF)

      Geometry::ParameterMap& paramMap = instrumentParameters();
      std::multimap<std::string, boost::shared_ptr<XMLlogfile> >& paramInfoFromIDF = inst->getLogfileCache();


      // iterator to browse through the multimap: paramInfoFromIDF

      std::multimap<std::string, boost::shared_ptr<XMLlogfile> > :: const_iterator it;
      std::pair<std::multimap<std::string, boost::shared_ptr<XMLlogfile> >::iterator,
        std::multimap<std::string, boost::shared_ptr<XMLlogfile> >::iterator> ret;

      // In order to allow positions to be set with r-position, t-position and p-position parameters
      // The idea is here to simply first check if parameters with names "r-position", "t-position"
      // and "p-position" are encounted then at the end of this method act on this
      std::set<const IComponent*> rtp_positionComp;
      std::multimap<const IComponent*, m_PositionEntry > rtp_positionEntry;

      // loop over all logfiles and see if any of these are associated with parameters in the
      // IDF

      size_t N = logfileProp.size();
      for (size_t i = 0; i < N; i++)
      {
        // Get the name of the timeseries property

        std::string logName = logfileProp[i]->name();

        // See if filenamePart matches any logfile-IDs in IDF. If this add parameter to parameter map

        ret = paramInfoFromIDF.equal_range(logName);
        for (it=ret.first; it!=ret.second; ++it)
        {
          double value = ((*it).second)->createParamValue(static_cast<Kernel::TimeSeriesProperty<double>*>(logfileProp[i]));

          // special cases of parameter names

          std::string paramN = ((*it).second)->m_paramName;
          if ( paramN.compare("x")==0 || paramN.compare("y")==0 || paramN.compare("z")==0 )
            paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
          else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )
          {
            paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
          }
          else if ( paramN.compare("r-position")==0 || paramN.compare("t-position")==0 || paramN.compare("p-position")==0 )
          {
            rtp_positionComp.insert(((*it).second)->m_component);
            rtp_positionEntry.insert(
              std::pair<const IComponent*, m_PositionEntry >(
                ((*it).second)->m_component, m_PositionEntry(paramN, value)));
          }
          else
            paramMap.addDouble(((*it).second)->m_component, paramN, value);
        }
      }

      // Check if parameters have been specified using the 'value' attribute rather than the 'logfile-id' attribute
      // All such parameters have been stored using the key = "".
      ret = paramInfoFromIDF.equal_range("");
      Kernel::TimeSeriesProperty<double>* dummy = NULL;
      for (it = ret.first; it != ret.second; ++it)
      {
        std::string paramN = ((*it).second)->m_paramName;
        std::string category = ((*it).second)->m_type;

        // if category is sting no point in trying to generate a double from parameter
        double value = 0.0;
        if ( category.compare("string") != 0 )
          value = ((*it).second)->createParamValue(dummy);

        if ( category.compare("fitting") == 0 )
        {
          std::ostringstream str;
          str << value << " , " << ((*it).second)->m_fittingFunction << " , " << paramN << " , " << ((*it).second)->m_constraint[0] << " , "
            << ((*it).second)->m_constraint[1] << " , " << ((*it).second)->m_penaltyFactor << " , "
            << ((*it).second)->m_tie << " , " << ((*it).second)->m_formula << " , "
            << ((*it).second)->m_formulaUnit << " , " << ((*it).second)->m_resultUnit << " , " << (*(((*it).second)->m_interpolation));
          paramMap.add("fitting",((*it).second)->m_component, paramN, str.str());
        }
        else if ( category.compare("string") == 0 )
        {
          paramMap.addString(((*it).second)->m_component, paramN, ((*it).second)->m_value);
        }
        else
        {
          if (paramN.compare("x") == 0 || paramN.compare("y") == 0 || paramN.compare("z") == 0)
            paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
          else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )
            paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
          else if ( paramN.compare("r-position")==0 || paramN.compare("t-position")==0 || paramN.compare("p-position")==0 )
          {
            rtp_positionComp.insert(((*it).second)->m_component);
            rtp_positionEntry.insert(
              std::pair<const IComponent*, m_PositionEntry >(
                ((*it).second)->m_component, m_PositionEntry(paramN, value)));
          }
          else
            paramMap.addDouble(((*it).second)->m_component, paramN, value);
        }
      }

      // check if parameters with names "r-position", "t-position"
      // and "p-position" were encounted
      std::pair<std::multimap<const IComponent*, m_PositionEntry >::iterator,
        std::multimap<const IComponent*, m_PositionEntry >::iterator> retComp;
      double deg2rad = (M_PI/180.0);
      std::set<const IComponent*>::iterator itComp;
      std::multimap<const IComponent*, m_PositionEntry > :: const_iterator itRTP;
      for (itComp=rtp_positionComp.begin(); itComp!=rtp_positionComp.end(); itComp++)
      {
        retComp = rtp_positionEntry.equal_range(*itComp);
        bool rSet = false;
        double rVal=0.0;
        double tVal=0.0;
        double pVal=0.0;
        for (itRTP = retComp.first; itRTP!=retComp.second; ++itRTP)
        {
          std::string paramN = ((*itRTP).second).paramName;
          if ( paramN.compare("r-position")==0 )
          {
            rSet = true;
            rVal = ((*itRTP).second).value;
          }
          if ( paramN.compare("t-position")==0 )
          {
            tVal = deg2rad*((*itRTP).second).value;
          }
          if ( paramN.compare("p-position")==0 )
          {
            pVal = deg2rad*((*itRTP).second).value;
          }
        }
        if ( rSet )
        {
          // convert spherical coordinates to cartesian coordinate values
          double x = rVal*sin(tVal)*cos(pVal);
          double y = rVal*sin(tVal)*sin(pVal);
          double z = rVal*cos(tVal);

          paramMap.addPositionCoordinate(*itComp, "x", x);
          paramMap.addPositionCoordinate(*itComp, "y", y);
          paramMap.addPositionCoordinate(*itComp, "z", z);
        }
      }

      // Clear out the nearestNeighbors so that it gets recalculated
      this->m_nearestNeighbours.reset();
    }


    //----------------------------------------------------------------------------------------------------
    /// @return The number of axes which this workspace has
    int MatrixWorkspace::axes() const
    {
      return static_cast<int>(m_axes.size());
    }

    //----------------------------------------------------------------------------------------------------
    /** Get a pointer to a workspace axis
    *  @param axisIndex :: The index of the axis required
    *  @throw IndexError If the argument given is outside the range of axes held by this workspace
    *  @return Pointer to Axis object
    */
    Axis* MatrixWorkspace::getAxis(const size_t& axisIndex) const
    {
      if ( axisIndex >= m_axes.size() )
      {
        g_log.error() << "Argument to getAxis (" << axisIndex << ") is invalid for this (" << m_axes.size() << " axis) workspace" << std::endl;
        throw Kernel::Exception::IndexError(axisIndex, m_axes.size(),"Argument to getAxis is invalid for this workspace");
      }

      return m_axes[axisIndex];
    }

    /** Replaces one of the workspace's axes with the new one provided.
    *  @param axisIndex :: The index of the axis to replace
    *  @param newAxis :: A pointer to the new axis. The class will take ownership.
    *  @throw IndexError If the axisIndex given is outside the range of axes held by this workspace
    *  @throw std::runtime_error If the new axis is not of the correct length (within one of the old one)
    */
    void MatrixWorkspace::replaceAxis(const size_t& axisIndex, Axis* const newAxis)
    {
      // First check that axisIndex is in range
      if ( axisIndex >= m_axes.size() )
      {
        g_log.error() << "Value of axisIndex (" << axisIndex << ") is invalid for this (" << m_axes.size() << " axis) workspace" << std::endl;
        throw Kernel::Exception::IndexError(axisIndex, m_axes.size(),"Value of axisIndex is invalid for this workspace");
      }
      // If we're OK, then delete the old axis and set the pointer to the new one
      delete m_axes[axisIndex];
      m_axes[axisIndex] = newAxis;
    }


    //----------------------------------------------------------------------------------------------------
    /// Returns the units of the data in the workspace
    std::string MatrixWorkspace::YUnit() const
    {
      return m_YUnit;
    }

    /// Sets a new unit for the data (Y axis) in the workspace
    void MatrixWorkspace::setYUnit(const std::string& newUnit)
    {
      m_YUnit = newUnit;
    }

    /// Returns a caption for the units of the data in the workspace
    std::string MatrixWorkspace::YUnitLabel() const
    {
      std::string retVal;
      if ( !m_YUnitLabel.empty() ) retVal = m_YUnitLabel;
      else
      {
        retVal = m_YUnit;
        // If this workspace a distribution & has at least one axis & this axis has its unit set
        // then append that unit to the string to be returned
        if ( !retVal.empty() && this->isDistribution() && this->axes() && this->getAxis(0)->unit() )
        {
          retVal = retVal + " per " + this->getAxis(0)->unit()->label();
        }
      }

      return retVal;
    }

    /// Sets a new caption for the data (Y axis) in the workspace
    void MatrixWorkspace::setYUnitLabel(const std::string& newLabel)
    {
      m_YUnitLabel = newLabel;
    }

    //----------------------------------------------------------------------------------------------------
    /** Are the Y-values in this workspace dimensioned?
    * TODO: For example: ????
    * @return whether workspace is a distribution or not
    */
    const bool& MatrixWorkspace::isDistribution() const
    {
      return m_isDistribution;
    }

    /** Set the flag for whether the Y-values are dimensioned
    *  @return whether workspace is now a distribution
    */
    bool& MatrixWorkspace::isDistribution(bool newValue)
    {
      m_isDistribution = newValue;
      return m_isDistribution;
    }

    /**
    *  Whether the workspace contains histogram data
    *  @return whether the worksapace contains histogram data
    */
    bool MatrixWorkspace::isHistogramData() const
    {
      return ( readX(0).size()==blocksize() ? false : true );
    }


    //----------------------------------------------------------------------------------------------------
    /**
     * Mask a given workspace index, setting the data and error values to zero
     * @param index :: The index within the workspace to mask
     */
    void MatrixWorkspace::maskWorkspaceIndex(const size_t index)
    {
      if( index >= this->getNumberHistograms() )
      {
        throw Kernel::Exception::IndexError(index,this->getNumberHistograms(),
            "MatrixWorkspace::maskWorkspaceIndex,index");
      }

      ISpectrum * spec = this->getSpectrum(index);
      if (!spec) throw std::invalid_argument("MatrixWorkspace::maskWorkspaceIndex() got a null Spectrum.");

      // Virtual method clears the spectrum as appropriate
      spec->clearData();

      const std::set<detid_t> dets = spec->getDetectorIDs();
      for (std::set<detid_t>::const_iterator iter=dets.begin(); iter != dets.end(); ++iter)
      {
        try
        {
          if ( const Geometry::Detector* det = dynamic_cast<const Geometry::Detector*>(sptr_instrument->getDetector(*iter).get()) )
          {
            m_parmap->addBool(det,"masked",true);  // Thread-safe method
          }
        }
        catch(Kernel::Exception::NotFoundError &)
        {
        }
      }
    }

    //----------------------------------------------------------------------------------------------------
    /** Called by the algorithm MaskBins to mask a single bin for the first time, algorithms that later propagate the
    *  the mask from an input to the output should call flagMasked() instead. Here y-values and errors will be scaled
    *  by (1-weight) as well as the mask flags (m_masks) being updated. This function doesn't protect the writes to the
    *  y and e-value arrays and so is not safe if called by multiple threads working on the same spectrum. Writing to
    *  the mask set is marked parrallel critical so different spectra can be analysised in parallel
    *  @param workspaceIndex :: The workspace spectrum index of the bin
    *  @param binIndex ::      The index of the bin in the spectrum
    *  @param weight ::        'How heavily' the bin is to be masked. =1 for full masking (the default).
    */
    void MatrixWorkspace::maskBin(const size_t& workspaceIndex, const size_t& binIndex, const double& weight)
    {
      // First check the workspaceIndex is valid
      if (workspaceIndex >= this->getNumberHistograms() )
        throw Kernel::Exception::IndexError(workspaceIndex,this->getNumberHistograms(),"MatrixWorkspace::maskBin,workspaceIndex");
      // Then check the bin index
      if (binIndex>= this->blocksize() )
        throw Kernel::Exception::IndexError(binIndex,this->blocksize(),"MatrixWorkspace::maskBin,binIndex");

      // this function is marked parallel critical
      flagMasked(workspaceIndex, binIndex, weight);

      //this is the actual result of the masking that most algorithms and plotting implementations will see, the bin mask flags defined above are used by only some algorithms
      this->dataY(workspaceIndex)[binIndex] *= (1-weight);
      this->dataE(workspaceIndex)[binIndex] *= (1-weight);
    }

    /** Writes the masking weight to m_masks (doesn't alter y-values). Contains a parrallel critical section
    *  and so is thread safe
    *  @param spectrumIndex :: The workspace spectrum index of the bin
    *  @param binIndex ::      The index of the bin in the spectrum
    *  @param weight ::        'How heavily' the bin is to be masked. =1 for full masking (the default).
    */
    void MatrixWorkspace::flagMasked(const size_t& spectrumIndex, const size_t& binIndex, const double& weight)
    {
      // Writing to m_masks is not thread-safe, so put in some protection
      PARALLEL_CRITICAL(maskBin)
      {
        // First get a reference to the list for this spectrum (or create a new list)
        MaskList& binList = m_masks[spectrumIndex];
        //see if the bin is already masked. Normally only a handfull of bins are masked, if it's 100s you might want to make this faster
        for(MaskList::const_iterator it = binList.begin(); it != binList.end(); ++it)
        {
          if ( it->first == binIndex )
          {
            //calling erase will invalidate the iterator! So we must call break immediately after
            binList.erase(it);
            break;
          }
        }
        binList.insert( std::make_pair(binIndex,weight) );
      }
    }

    /** Does this spectrum contain any masked bins 
    *  @param workspaceIndex :: The workspace spectrum index to test
    *  @return True if there are masked bins for this spectrum
    */
    bool MatrixWorkspace::hasMaskedBins(const size_t& workspaceIndex) const
    {
      // First check the workspaceIndex is valid. Return false if it isn't (decided against throwing here).
      if ( workspaceIndex >= this->getNumberHistograms() )
        return false;
      return (m_masks.find(workspaceIndex)==m_masks.end()) ? false : true;
    }

    /** Returns the list of masked bins for a spectrum. 
    *  @param  workspaceIndex
    *  @return A const reference to the list of masked bins
    *  @throw  Kernel::Exception::IndexError if there are no bins masked for this spectrum (so call hasMaskedBins first!)
    */
    const MatrixWorkspace::MaskList& MatrixWorkspace::maskedBins(const size_t& workspaceIndex) const
    {
      std::map<int64_t,MaskList>::const_iterator it = m_masks.find(workspaceIndex);
      // Throw if there are no masked bins for this spectrum. The caller should check first using hasMaskedBins!
      if (it==m_masks.end())
      {
        g_log.error() << "There are no masked bins for spectrum index " << workspaceIndex << std::endl;
        throw Kernel::Exception::IndexError(workspaceIndex,0,"MatrixWorkspace::maskedBins");
      }

      return it->second;
    }

    //---------------------------------------------------------------------------------------------
    /** Return memory used by the workspace, in bytes.
     * @return bytes used.
     */
    size_t MatrixWorkspace::getMemorySize() const
    {
      //3 doubles per histogram bin.
      return 3*size()*sizeof(double) + m_run->getMemorySize();
    }

    /** Returns the memory used (in bytes) by the X axes, handling ragged bins.
     * @return bytes used
     */
    size_t MatrixWorkspace::getMemorySizeForXAxes() const
    {
      size_t total = 0;
      MantidVecPtr lastX = this->refX(0);
      for (size_t wi=0; wi < getNumberHistograms(); wi++)
      {
        MantidVecPtr X = this->refX(wi);
        // If the pointers are the same
        if (!(X == lastX) || wi==0)
          total += (*X).size() * sizeof(double);
      }
      return total;
    }



    //-----------------------------------------------------------------------------
    /** Return the time of the first pulse received, by accessing the run's
     * sample logs to find the proton_charge.
     *
     * NOTE, JZ: Pulse times before 1991 (up to 100) are skipped. This is to avoid
     * a DAS bug at SNS around Mar 2011 where the first pulse time is Jan 1, 1990.
     *
     * @return the time of the first pulse
     * @throw runtime_error if the log is not found; or if it is empty.
     */
    Kernel::DateAndTime MatrixWorkspace::getFirstPulseTime() const
    {
      TimeSeriesProperty<double>* log = dynamic_cast<TimeSeriesProperty<double>*> (this->run().getLogData("proton_charge"));
      if (!log)
        throw std::runtime_error("EventWorkspace::getFirstPulseTime: No TimeSeriesProperty called 'proton_charge' found in the workspace.");
      DateAndTime startDate;
      DateAndTime reference("1991-01-01");

      int i=0;
      startDate = log->nthTime(i);

      // Find the first pulse after 1991
      while (startDate < reference && i < 100)
      {
        i++;
        startDate = log->nthTime(i);
      }

      //Return as DateAndTime.
      return startDate;
    }


    //-----------------------------------------------------------------------------
    /** Return the time of the last pulse received, by accessing the run's
     * sample logs to find the proton_charge
     *
     * @return the time of the first pulse
     * @throw runtime_error if the log is not found; or if it is empty.
     */
    Kernel::DateAndTime MatrixWorkspace::getLastPulseTime() const
    {
      TimeSeriesProperty<double>* log = dynamic_cast<TimeSeriesProperty<double>*> (this->run().getLogData("proton_charge"));
      if (!log)
        throw std::runtime_error("EventWorkspace::getFirstPulseTime: No TimeSeriesProperty called 'proton_charge' found in the workspace.");
      DateAndTime stopDate = log->lastTime();
      //Return as DateAndTime.
      return stopDate;
    }


    //----------------------------------------------------------------------------------------------------
    /**
    * Returns the bin index of the given X value
    * @param xValue :: The X value to search for
    * @param index :: The index within the workspace to search within (default = 0)
    * @returns An index that 
    */
    size_t MatrixWorkspace::binIndexOf(const double xValue, const size_t index) const
    {
      if( index >= getNumberHistograms() )
      {
        throw std::out_of_range("MatrixWorkspace::binIndexOf - Index out of range.");
      }
      const MantidVec & xValues = this->dataX(index);
      // Lower bound will test if the value is greater than the last but we need to see if X is valid at the start
      if( xValue < xValues.front() )
      {
        throw std::out_of_range("MatrixWorkspace::binIndexOf - X value lower than lowest in current range.");
      }
      MantidVec::const_iterator lowit = std::lower_bound(xValues.begin(), xValues.end(), xValue);
      if( lowit == xValues.end() )
      {
        throw std::out_of_range("MatrixWorkspace::binIndexOf - X value greater than highest in current range.");
      }
      // If we are pointing at the first value then that means we still want to be in the first bin
      if( lowit == xValues.begin() )
      {
        ++lowit;
      }
      size_t hops = std::distance(xValues.begin(), lowit);
      // The bin index is offset by one from the number of hops between iterators as they start at zero
      return hops - 1;
    }

    uint64_t MatrixWorkspace::getNPoints() const
    {
      return (uint64_t)(this->size());
    }

    //================================= FOR MDGEOMETRY ====================================================

    size_t MatrixWorkspace::getNumDims() const
    {
      return 2;
    }



    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getPoint(size_t index) const
    {
      HistogramIndex histInd = m_indexCalculator.getHistogramIndex(index);
      BinIndex binInd = m_indexCalculator.getBinIndex(index, histInd);
      MatrixMDPointMap::const_iterator iter = m_mdPointMap.find(index);
      //Create the MDPoint if it is not already present.
      if(m_mdPointMap.end() ==  iter)
      {
        m_mdPointMap[static_cast<int64_t>(index)] = createPoint(histInd, binInd);
      }
      return m_mdPointMap[static_cast<int64_t>(index)];
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getPointImp(size_t histogram, size_t bin) const
    {
      Index oneDimIndex = m_indexCalculator.getOneDimIndex(histogram, bin);
      MatrixMDPointMap::const_iterator iter = m_mdPointMap.find(oneDimIndex);
      if(m_mdPointMap.end() ==  iter)
      {
        m_mdPointMap[oneDimIndex] = createPoint(histogram, bin);
      }
      return m_mdPointMap[oneDimIndex];
    }

    Mantid::Geometry::MDPoint  MatrixWorkspace::createPoint(HistogramIndex histogram, BinIndex bin) const
    {
      VecCoordinate verts(4);

      signal_t signal = this->dataY(histogram)[bin];
      signal_t error = this->dataE(histogram)[bin];
      coord_t x = this->dataX(histogram)[bin];
      coord_t histogram_d = static_cast<double>(histogram);

      if(isHistogramData()) //TODO. complete vertex generating cases.
      {
        verts[0] = Coordinate::createCoordinate2D(x, histogram_d);
        verts[1] = Coordinate::createCoordinate2D(this->dataX(histogram)[bin+1], histogram_d);
        verts[2] = Coordinate::createCoordinate2D(x, histogram_d+1.);
        verts[3] = Coordinate::createCoordinate2D(this->dataX(histogram)[bin+1], histogram_d+1.);
      }

      IDetector_const_sptr detector;
      const ISpectrum * spec = this->getSpectrum(histogram);
      if(spec->getDetectorIDs().size() > 0)
      {
        try
        {
          detector = this->getDetector(histogram);
        }
        catch(std::exception&)
        {
          //Swallow exception and continue processing.
        }
      }
      return Mantid::Geometry::MDPoint(signal, error, verts, detector, this->sptr_instrument);
    }


    std::string MatrixWorkspace::getDimensionIdFromAxis(const int& axisIndex) const
    {
      std::string id;
      if(0 == axisIndex)
      {
        id = xDimensionId;
      }
      else if(1 == axisIndex)
      {
        id = yDimensionId;
      }
      else
      {
        throw std::invalid_argument("Cannot have an index for a MatrixWorkspace axis that is not == 0 or == 1");
      }
      return id;
    }

    class MWDimension: public Mantid::Geometry::IMDDimension
    {
    public:

      MWDimension(const Axis* axis, const std::string& dimensionId):
          m_axis(*axis), m_dimensionId(dimensionId)
      {
      }
      /// the name of the dimennlsion as can be displayed along the axis
      virtual std::string getName() const {return m_axis.title();}
      virtual std::string getUnits() const {return m_axis.unit()->label();}
      /// short name which identify the dimension among other dimensin. A dimension can be usually find by its ID and various  
      /// various method exist to manipulate set of dimensions by their names. 
      virtual std::string getDimensionId() const {return m_dimensionId;}

      /// if the dimension is integrated (e.g. have single bin)
      virtual bool getIsIntegrated() const {return m_axis.length() == 1;}
      // it is sometimes convinient to shift image data by some number along specific dimension
      virtual double getDataShift()const {return 0;}

      virtual double getMaximum() const {return m_axis(m_axis.length()-1);}

      virtual double getMinimum() const {return m_axis(0);}
      /// number of bins dimension have (an integrated has one). A axis directed along dimension would have getNBins+1 axis points. 
      virtual size_t getNBins() const {return m_axis.length();}
      /// the change of the location in the multidimensional image array, which occurs if the index of this dimension changes by one. 
      virtual size_t      getStride()const {return 0;}
      /// defines if the dimension is reciprocal or not. The reciprocal dimensions are treated differently from an orthogonal one
      virtual bool isReciprocal() const {return false;}
      /// defines the dimension scale in physical units. 
      virtual double getScale()const {return 0;}
      ///  Get coordinate for index;
      virtual double getX(size_t ind)const {return m_axis(ind);}
      // Mess; TODO: clear
      virtual Kernel::V3D getDirection(void)const {throw std::runtime_error("Not implemented");}
      virtual Kernel::V3D getDirectionCryst(void)const {throw std::runtime_error("Not implemented");}

      /// the function returns the center points of the axis bins; There are nBins of such points 
      /// (when axis has nBins+1 points with point 0 equal rMin and nBins+1 equal rMax)
      virtual void getAxisPoints(std::vector<double>  &)const{throw std::runtime_error("Not implemented");}


      //Dimensions must be xml serializable.
      virtual std::string toXMLString() const {throw std::runtime_error("Not implemented");}

      virtual ~MWDimension(){};
    private:
      const Axis& m_axis;
      const std::string m_dimensionId;
    };


    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::getDimensionNum(size_t index)const
    { 
      if (index == 0)
      {
        Axis* xAxis = this->getAxis(0);
        MWDimension* dimension = new MWDimension(xAxis, xDimensionId);
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dimension);
      }
      else if (index == 1)
      {
        Axis* yAxis = this->getAxis(1);
        MWDimension* dimension = new MWDimension(yAxis, yDimensionId);
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dimension);
      }
      else
        throw std::invalid_argument("MatrixWorkspace only has 2 dimensions.");
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::getDimension(std::string id) const
    { 
      int nAxes = this->axes();
      IMDDimension* dim = NULL;
      for(int i = 0; i < nAxes; i++)
      {
        Axis* xAxis = this->getAxis(i);
        const std::string& knownId = getDimensionIdFromAxis(i);
        if(knownId == id)
        {
          dim = new MWDimension(xAxis, id);
          break;
        }
      }
      if(NULL == dim)
      {
        std::string message = "Cannot find id : " + id;
        throw std::overflow_error(message);
      }
      return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dim);
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(size_t dim1Increment) const
    { 
      if (dim1Increment >= this->dataX(0).size())
      {
        throw std::range_error("MatrixWorkspace::getCell, increment out of range");
      }

      return this->getPoint(dim1Increment);
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(size_t dim1Increment, size_t dim2Increment) const
    { 
      if (dim1Increment >= this->dataX(0).size())
      {
        throw std::range_error("MatrixWorkspace::getCell, increment out of range");
      }
      if (dim2Increment >= this->dataX(0).size())
      {
        throw std::range_error("MatrixWorkspace::getCell, increment out of range");
      }

      return getPointImp(dim1Increment, dim2Increment);
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(size_t, size_t, size_t) const
    { 
      throw std::logic_error("Cannot access higher dimensions");
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(size_t, size_t, size_t, size_t) const
    { 
      throw std::logic_error("Cannot access higher dimensions");
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(...) const
    { 
      throw std::logic_error("Cannot access higher dimensions");
    }

    std::string MatrixWorkspace::getWSLocation() const
    {
      throw std::logic_error("Cannot access the workspace location on a MatrixWS");
    }


  } // namespace API
} // Namespace Mantid


///\cond TEMPLATE
template MANTID_API_DLL class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef,Mantid::API::MatrixWorkspace>;
template MANTID_API_DLL class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::API::MatrixWorkspace>;

namespace Mantid
{
  namespace Kernel
  {

    template<> MANTID_API_DLL
      Mantid::API::MatrixWorkspace_sptr IPropertyManager::getValue<Mantid::API::MatrixWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected MatrixWorkspace.";
        throw std::runtime_error(message);
      }
    }

    template<> MANTID_API_DLL
      Mantid::API::MatrixWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::MatrixWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const MatrixWorkspace.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
