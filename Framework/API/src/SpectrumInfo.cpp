#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidBeamline/SpectrumInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/make_shared.hpp>
#include <algorithm>

namespace Mantid {
namespace API {

SpectrumInfo::SpectrumInfo(const Beamline::SpectrumInfo &spectrumInfo,
                           const ExperimentInfo &experimentInfo)
    : m_experimentInfo(experimentInfo),
      m_detectorInfo(experimentInfo.detectorInfo()),
      m_spectrumInfo(spectrumInfo), m_lastDetector(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

SpectrumInfo::SpectrumInfo(const Beamline::SpectrumInfo &spectrumInfo,
                           ExperimentInfo &experimentInfo)
    : m_experimentInfo(experimentInfo),
      m_mutableDetectorInfo(&experimentInfo.mutableDetectorInfo()),
      m_detectorInfo(*m_mutableDetectorInfo), m_spectrumInfo(spectrumInfo),
      m_lastDetector(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

// Defined as default in source for forward declaration with std::unique_ptr.
SpectrumInfo::~SpectrumInfo() = default;

/// Returns the size of the SpectrumInfo, i.e., the number of spectra.
size_t SpectrumInfo::size() const { return m_spectrumInfo.size(); }

/// Returns a const reference to the SpectrumDefinition of the spectrum.
const SpectrumDefinition &
SpectrumInfo::spectrumDefinition(const size_t index) const {
  m_experimentInfo.updateSpectrumDefinitionIfNecessary(index);
  return m_spectrumInfo.spectrumDefinition(index);
}

const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &
SpectrumInfo::sharedSpectrumDefinitions() const {
  for (size_t i = 0; i < size(); ++i)
    m_experimentInfo.updateSpectrumDefinitionIfNecessary(i);
  return m_spectrumInfo.sharedSpectrumDefinitions();
}

/// Returns true if the detector(s) associated with the spectrum are monitors.
bool SpectrumInfo::isMonitor(const size_t index) const {
  for (const auto detIndex : getDetectorIndices(index))
    if (!m_detectorInfo.isMonitor(detIndex))
      return false;
  return true;
}

/// Returns true if the detector(s) associated with the spectrum are masked.
bool SpectrumInfo::isMasked(const size_t index) const {
  bool masked = true;
  for (const auto detIndex : getDetectorIndices(index))
    masked &= m_detectorInfo.isMasked(detIndex);
  return masked;
}

/** Returns L2 (distance from sample to spectrum).
 *
 * For monitors this is defined such that L1+L2 = source-detector distance,
 * i.e., for a monitor in the beamline between source and sample L2 is negative.
 */
double SpectrumInfo::l2(const size_t index) const {
  double l2{0.0};
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    l2 += m_detectorInfo.l2(detIndex);
  }
  return l2 / static_cast<double>(dets.size());
}

/** Returns the scattering angle 2 theta in radians (angle w.r.t. to beam
 *direction).
 *
 * Throws an exception if the spectrum is a monitor.
 */
double SpectrumInfo::twoTheta(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  double twoTheta{0.0};
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    twoTheta += m_detectorInfo.twoTheta(detIndex);
  }
  return twoTheta / static_cast<double>(dets.size());
}

/** Returns the signed scattering angle 2 theta in radians (angle w.r.t. to beam
 * direction).
 *
 * Throws an exception if the spectrum is a monitor.
 */
double SpectrumInfo::signedTwoTheta(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  double signedTwoTheta{0.0};
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    signedTwoTheta += m_detectorInfo.signedTwoTheta(detIndex);
  }
  return signedTwoTheta / static_cast<double>(dets.size());
}

/// Returns the position of the spectrum with given index.
Kernel::V3D SpectrumInfo::position(const size_t index) const {
  Kernel::V3D newPos;
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    newPos += m_detectorInfo.position(detIndex);
  }
  return newPos / static_cast<double>(dets.size());
}

/// Returns true if the spectrum is associated with detectors in the instrument.
bool SpectrumInfo::hasDetectors(const size_t index) const {
  // Workspaces can contain invalid detector IDs. Those IDs will be silently
  // ignored here until this is fixed.
  return spectrumDefinition(index).size() > 0;
}

/// Returns true if the spectrum is associated with exactly one detector.
bool SpectrumInfo::hasUniqueDetector(const size_t index) const {
  // Workspaces can contain invalid detector IDs. Those IDs will be silently
  // ignored here until this is fixed.
  return spectrumDefinition(index).size() == 1;
}

/** Set the mask flag of the spectrum with given index. Not thread safe.
 *
 * Currently this simply sets the mask flags for the underlying detectors. */
void SpectrumInfo::setMasked(const size_t index, bool masked) {
  for (const auto detIndex : getDetectorIndices(index))
    m_mutableDetectorInfo->setMasked(detIndex, masked);
}

/// Return a const reference to the detector or detector group of the spectrum
/// with given index.
const Geometry::IDetector &SpectrumInfo::detector(const size_t index) const {
  return getDetector(index);
}

/// Returns the source position.
Kernel::V3D SpectrumInfo::sourcePosition() const {
  return m_detectorInfo.sourcePosition();
}

/// Returns the sample position.
Kernel::V3D SpectrumInfo::samplePosition() const {
  return m_detectorInfo.samplePosition();
}

/// Returns L1 (distance from source to sample).
double SpectrumInfo::l1() const { return m_detectorInfo.l1(); }

const Geometry::IDetector &SpectrumInfo::getDetector(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] == index)
    return *m_lastDetector[thread];

  // Note: This function body has big overlap with the method
  // MatrixWorkspace::getDetector(). The plan is to eventually remove the
  // latter, once SpectrumInfo is in widespread use.
  const auto &specDef = spectrumDefinition(index);
  const size_t ndets = specDef.size();
  if (ndets == 1) {
    // If only 1 detector for the spectrum number, just return it
    const auto detIndex = specDef[0].first;
    m_lastDetector[thread] = m_detectorInfo.getDetectorPtr(detIndex);
  } else if (ndets == 0) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No "
                                           "detectors for this workspace "
                                           "index.",
                                           "");
  } else {
    // Else need to construct a DetectorGroup and use that
    std::vector<boost::shared_ptr<const Geometry::IDetector>> det_ptrs;
    for (const auto &index : specDef) {
      const auto detIndex = index.first;
      det_ptrs.push_back(m_detectorInfo.getDetectorPtr(detIndex));
    }
    m_lastDetector[thread] =
        boost::make_shared<Geometry::DetectorGroup>(det_ptrs);
  }
  m_lastIndex[thread] = index;
  return *m_lastDetector[thread];
}

std::vector<Geometry::IDetector_const_sptr>
SpectrumInfo::getDetectorVector(const size_t index) const {
  const auto &det = getDetector(index);
  const auto &ndet = det.nDets();
  if (ndet > 1) {
    const auto group = dynamic_cast<const Geometry::DetectorGroup *>(&det);
    return group->getDetectors();
  } else {
    size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
    return {m_lastDetector[thread]};
  }
}

std::vector<size_t> SpectrumInfo::getDetectorIndices(const size_t index) const {
  std::vector<size_t> detIndices;
  for (const auto &def : spectrumDefinition(index))
    detIndices.push_back(def.first);
  if (detIndices.empty())
    throw Kernel::Exception::NotFoundError(
        "SpectrumInfo: No detectors for this workspace index.", "");
  return detIndices;
}

} // namespace API
} // namespace Mantid
