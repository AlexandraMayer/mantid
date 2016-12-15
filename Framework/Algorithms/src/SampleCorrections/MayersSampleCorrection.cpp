#include "MantidAlgorithms/SampleCorrections/MayersSampleCorrection.h"
#include "MantidAlgorithms/SampleCorrections/MayersSampleCorrectionStrategy.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Material.h"

namespace Mantid {
namespace Algorithms {

using API::MatrixWorkspace_sptr;
namespace Exception = Kernel::Exception;
using Geometry::IDetector_const_sptr;
using Kernel::Direction;
using Kernel::V3D;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MayersSampleCorrection)

/**
 * Constructor
 */
MayersSampleCorrection::MayersSampleCorrection() : API::Algorithm() {}

/// Algorithms name for identification. @see Algorithm::name
const std::string MayersSampleCorrection::name() const {
  return "MayersSampleCorrection";
}

/// Algorithm's version for identification. @see Algorithm::version
int MayersSampleCorrection::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MayersSampleCorrection::category() const {
  return "CorrectionFunctions\\AbsorptionCorrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MayersSampleCorrection::summary() const {
  return "Corrects the input data for the effects of attenuation & multiple "
         "scattering";
}

/** Initialize the algorithm's properties.
 */
void MayersSampleCorrection::init() {
  using API::WorkspaceProperty;
  // Inputs
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<>>(
          "InputWorkspace", "", Direction::Input, createInputWSValidator()),
      "Input workspace with X units in TOF. The workspace must "
      "also have a sample with a cylindrical shape and an "
      "instrument with a defined source and sample position.");
  declareProperty(
      "MultipleScattering", false,
      "If True then also correct for the effects of multiple scattering."
      "Please note that the MS correction assumes the scattering is elastic.",
      Direction::Input);
  // Outputs
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/**
 */
void MayersSampleCorrection::exec() {
  using API::Progress;
  using API::WorkspaceFactory;
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  bool mscatOn = getProperty("MultipleScattering");
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS);

  // Instrument constants
  auto instrument = inputWS->getInstrument();
  const auto source = instrument->getSource();
  const auto sample = instrument->getSample();
  const auto frame = instrument->getReferenceFrame();

  // Sample
  const auto &sampleShape = inputWS->sample().getShape();
  // Current Object code computes quite an inaccurate bounding box so we do
  // something better for the time being
  const double big(100.); // seems to be a sweet spot...
  double minX(-big), maxX(big), minY(-big), maxY(big), minZ(-big), maxZ(big);
  sampleShape.getBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
  V3D boxWidth(maxX - minX, maxY - minY, maxZ - minZ);
  const double radius(0.5 * boxWidth[frame->pointingHorizontal()]),
      height(boxWidth[frame->pointingUp()]);
  const auto &sampleMaterial = sampleShape.material();

  const size_t nhist(inputWS->getNumberHistograms());
  Progress prog(this, 0., 1., nhist);
  prog.setNotifyStep(0.01);

  const auto &spectrumInfo = inputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < static_cast<int64_t>(nhist); ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) ||
        spectrumInfo.isMasked(i)) {
      continue;
    }

    const auto &det = spectrumInfo.detector(i);

    MayersSampleCorrectionStrategy::Parameters params;
    params.mscat = mscatOn;
    params.l1 = spectrumInfo.l1();
    params.l2 = spectrumInfo.l2(i);
    params.twoTheta = spectrumInfo.twoTheta(i);
    params.phi = det.getPhi();
    params.rho = sampleMaterial.numberDensity();
    params.sigmaAbs = sampleMaterial.absorbXSection();
    params.sigmaSc = sampleMaterial.totalScatterXSection();
    params.cylRadius = radius;
    params.cylHeight = height;

    MayersSampleCorrectionStrategy correction(params, inputWS->histogram(i));
    outputWS->setHistogram(i, correction.getCorrectedHisto());
    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outputWS);
}

//------------------------------------------------------------------------------
// Private members
//------------------------------------------------------------------------------
/**
 * @return The validator required for the input workspace
 */
Kernel::IValidator_sptr MayersSampleCorrection::createInputWSValidator() const {
  using API::InstrumentValidator;
  using API::SampleValidator;
  using Kernel::CompositeValidator;
  auto validator = boost::make_shared<CompositeValidator>();

  unsigned int requires = (InstrumentValidator::SamplePosition |
                           InstrumentValidator::SourcePosition);
  validator->add<InstrumentValidator, unsigned int>(requires);

  requires = (SampleValidator::Shape | SampleValidator::Material);
  validator->add<SampleValidator, unsigned int>(requires);

  return validator;
}

} // namespace Algorithms
} // namespace Mantid
