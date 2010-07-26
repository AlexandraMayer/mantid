#ifndef MANTID_API_MATRIXWORKSPACE_H_
#define MANTID_API_MATRIXWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/Axis.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/cow_ptr.h"
#include <boost/shared_ptr.hpp>
#include <set>

namespace Mantid
{
/// typedef for the data storage used in Mantid matrix workspaces
typedef std::vector<double> MantidVec;
/// typedef for the data storage used in Mantid matrix workspaces
typedef Kernel::cow_ptr<MantidVec> MantidVecPtr;

namespace API
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class SpectraDetectorMap;

/** Base MatrixWorkspace Abstract Class.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport MatrixWorkspace : public Workspace
{
public:
  // The Workspace Factory create-from-parent method needs direct access to the axes.
  friend class WorkspaceFactoryImpl;
  /// Typedef for the workspace_iterator to use with a Workspace
  typedef workspace_iterator<LocatedDataRef, MatrixWorkspace> iterator;
  /// Typedef for the const workspace_iterator to use with a Workspace
  typedef workspace_iterator<const LocatedDataRef, const MatrixWorkspace> const_iterator;

  void initialize(const int &NVectors, const int &XLength, const int &YLength);
  virtual ~MatrixWorkspace();

  void setInstrument(const IInstrument_sptr&);
  IInstrument_sptr getInstrument() const;
  boost::shared_ptr<Instrument> getBaseInstrument()const;

  // SpectraDetectorMap accessors
  const SpectraDetectorMap& spectraMap() const;
  SpectraDetectorMap& mutableSpectraMap();

  /// Sample accessors
  const  Sample& sample() const;
  Sample& mutableSample();

  /// Run details object access
  const Run & run() const;
  /// Writable version of the run object
  Run& mutableRun();

  /// Get a detector object (Detector or DetectorGroup) for the given spectrum index
  Geometry::IDetector_sptr getDetector(const int index) const;
  double detectorTwoTheta(Geometry::IDetector_const_sptr det) const;
  
  /// Get the footprint in memory in KB.
  virtual long int getMemorySize() const;

  /// Returns the set of parameters modifying the base instrument
  Geometry::ParameterMap& instrumentParameters()const;
  /// Const version
  const Geometry::ParameterMap& constInstrumentParameters() const;
  // Add parameters to the instrument parameter map
  void populateInstrumentParameters();

  // Section required for iteration
  /// Returns the number of single indexable items in the workspace
  virtual int size() const = 0;
  /// Returns the size of each block of data returned by the dataY accessors
  virtual int blocksize() const = 0;
  /// Returns the number of histograms in the workspace
  virtual int getNumberHistograms() const = 0;

  /// Returns the bin index for a given X value of a given workspace index
  size_t binIndexOf(const double xValue, const int index = 0) const;

  //----------------------------------------------------------------------
  // DATA ACCESSORS
  //----------------------------------------------------------------------
  // Methods for getting read-only access to the data. Created for the python interface.
  // Just passes through to the virtual dataX/Y/E function (const version)
  /// Returns a read-only (i.e. const) reference to the specified X array
  const MantidVec& readX(int const index) const { return dataX(index); }
  /// Returns a read-only (i.e. const) reference to the specified Y array
  const MantidVec& readY(int const index) const { return dataY(index); }
  /// Returns a read-only (i.e. const) reference to the specified E array
  const MantidVec& readE(int const index) const { return dataE(index); }

  /// Returns the x data
  virtual MantidVec& dataX(int const index) = 0;
  /// Returns the y data
  virtual MantidVec& dataY(int const index) = 0;
  /// Returns the error data
  virtual MantidVec& dataE(int const index) = 0;
  /// Returns the x data const
  virtual const MantidVec& dataX(int const index) const = 0;
  /// Returns the y data const
  virtual const MantidVec& dataY(int const index) const = 0;
  /// Returns the error const
  virtual const MantidVec& dataE(int const index) const = 0;
  
  /// Returns a pointer to the x data
  virtual Kernel::cow_ptr<MantidVec> refX(const int index) const = 0;
  /// Set the specified X array to point to the given existing array
  virtual void setX(const int index, const Kernel::cow_ptr<MantidVec>& X) = 0;
  //----------------------------------------------------------------------

  int axes() const;
  Axis* const getAxis(const int& axisIndex) const;
  void replaceAxis(const int& axisIndex, Axis* const newAxis);

  /// Returns true if the workspace contains data in histogram form (as opposed to point-like)
  virtual bool isHistogramData() const;

  std::string YUnit() const;
  void setYUnit(const std::string& newUnit);
  std::string YUnitLabel() const;
  void setYUnitLabel(const std::string& newLabel);

  /// Are the Y-values dimensioned?
  const bool& isDistribution() const;
  bool& isDistribution(bool newValue);

  // Methods to set and access masked bins
  void maskBin(const int& spectrumIndex, const int& binIndex, const double& weight = 1.0);
  bool hasMaskedBins(const int& spectrumIndex) const;
  /// Masked bins for each spectrum are stored as a set of pairs containing <bin index, weight>
  typedef std::set< std::pair<int,double> > MaskList;
  const MaskList& maskedBins(const int& spectrumIndex) const;
  
protected:
  MatrixWorkspace();

  /// Initialises the workspace. Sets the size and lengths of the arrays. Must be overloaded.
  virtual void init(const int &NVectors, const int &XLength, const int &YLength) = 0;

  /// A vector of pointers to the axes for this workspace
  std::vector<Axis*> m_axes;

private:
  /// Private copy constructor. NO COPY ALLOWED
  MatrixWorkspace(const MatrixWorkspace&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  MatrixWorkspace& operator=(const MatrixWorkspace&);

  /// Has this workspace been initialised?
  bool m_isInitialized;

  /// The instrument used for this experiment
  boost::shared_ptr<Instrument> sptr_instrument;
  /// The SpectraDetector table used for this experiment. Inside a copy-on-write pointer.
  Kernel::cow_ptr<SpectraDetectorMap> m_spectramap;
  /// The information on the sample environment
  Kernel::cow_ptr<Sample> m_sample;
  /// The run information
  Kernel::cow_ptr<Run> m_run;

  /// The unit for the data values (e.g. Counts)
  std::string m_YUnit;
  /// A text label for use when plotting spectra
  std::string m_YUnitLabel;
  /// Flag indicating whether the Y-values are dimensioned. False by default
  bool m_isDistribution;

  /// Parameters modifying the base instrument
  mutable Kernel::cow_ptr<Geometry::ParameterMap> m_parmap;

  /// The set of masked bins in a map keyed on spectrum index
  std::map< int, MaskList > m_masks;
  
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

///shared pointer to the matrix workspace base class
typedef boost::shared_ptr<MatrixWorkspace> MatrixWorkspace_sptr;
///shared pointer to the matrix workspace base class (const version)
typedef boost::shared_ptr<const MatrixWorkspace> MatrixWorkspace_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_MATRIXWORKSPACE_H_*/
