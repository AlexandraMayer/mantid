#ifndef MANTID_MDALGORITHMS_FINDPEAKSMD_H_
#define MANTID_MDALGORITHMS_FINDPEAKSMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace MDAlgorithms {

/** FindPeaksMD : TODO: DESCRIPTION
 *
 * @author
 * @date 2011-06-02
 */
class DLLExport FindPeaksMD : public API::Algorithm {
public:
  FindPeaksMD();
  /// Algorithm's name for identification
  const std::string name() const override { return "FindPeaksMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find peaks in reciprocal space in a MDEventWorkspace or a "
           "MDHistoWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Optimization\\PeakFinding;MDAlgorithms\\Peaks";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Read member variables from experiment info
  void readExperimentInfo(const Mantid::API::ExperimentInfo_sptr &ei,
                          const Mantid::API::IMDWorkspace_sptr &ws);

  /// Adds a peak based on Q, bin count & a set of detector IDs
  void addPeak(const Mantid::Kernel::V3D &Q, const double binCount);

  /// Adds a peak based on Q, bin count
  boost::shared_ptr<DataObjects::Peak> createPeak(const Mantid::Kernel::V3D &Q,
                                                  const double binCount);

  /// Run find peaks on an MDEventWorkspace
  template <typename MDE, size_t nd>
  void findPeaks(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
  /// Run find peaks on a histo workspace
  void findPeaksHisto(Mantid::DataObjects::MDHistoWorkspace_sptr ws);

  /// Output PeaksWorkspace
  Mantid::DataObjects::PeaksWorkspace_sptr peakWS;

  /// Estimated radius of peaks. Boxes closer than this are rejected
  coord_t peakRadiusSquared;

  /// Thresholding factor
  double DensityThresholdFactor;

  /// Max # of peaks
  int64_t m_maxPeaks;

  /// Flag to include the detectors within the peak
  bool m_addDetectors;

  /// Arbitrary scaling factor for density to make more manageable numbers,
  /// especially for older file formats.
  signal_t m_densityScaleFactor;

  /// Progress reporter.
  Mantid::API::Progress *prog;

  /** Enum describing which type of dimensions in the MDEventWorkspace */
  enum eDimensionType { HKL, QLAB, QSAMPLE };

  /// Instrument
  Mantid::Geometry::Instrument_const_sptr inst;
  /// Run number of the peaks
  int m_runNumber;
  /// Dimension type
  eDimensionType dimType;
  /// Goniometer matrix
  Mantid::Kernel::Matrix<double> m_goniometer;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_FINDPEAKSMD_H_ */
