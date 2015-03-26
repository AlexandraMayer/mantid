#include "MantidMDAlgorithms/CutMD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {

// Typedef to simplify function signatures
typedef std::pair<double, double> MinMax;

MinMax getDimensionExtents(IMDEventWorkspace_sptr ws, size_t index) {
  if (!ws)
    throw std::runtime_error(
        "Invalid workspace passed to getDimensionExtents.");
  auto dim = ws->getDimension(index);
  return std::make_pair(dim->getMinimum(), dim->getMaximum());
}

DblMatrix matrixFromProjection(ITableWorkspace_sptr projection) {
  if (!projection) {
    return DblMatrix(3, 3, true /* makeIdentity */);
  }

  const size_t numDims = projection->rowCount();
  DblMatrix ret(3, 3);
  for (size_t i = 0; i < numDims; i++) {
    const std::string name =
        projection->getColumn("name")->cell<std::string>(i);
    const std::string valueStr =
        projection->getColumn("value")->cell<std::string>(i);
    std::vector<std::string> valueStrVec;
    boost::split(valueStrVec, valueStr, boost::is_any_of(","));

    std::vector<double> valueDblVec;
    for (auto it = valueStrVec.begin(); it != valueStrVec.end(); ++it)
      valueDblVec.push_back(boost::lexical_cast<double>(*it));

    if (name == "u")
      ret.setRow(0, valueDblVec);
    else if (name == "v")
      ret.setRow(1, valueDblVec);
    else if (name == "w")
      ret.setRow(2, valueDblVec);
  }
  return ret;
}

std::vector<std::string> unitsFromProjection(ITableWorkspace_sptr projection) {
  std::vector<std::string> ret(3);
  const size_t numDims = projection->rowCount();
  for (size_t i = 0; i < numDims; i++) {
    const std::string name =
        projection->getColumn("name")->cell<std::string>(i);
    const std::string unit =
        projection->getColumn("type")->cell<std::string>(i);

    if (name == "u")
      ret[0] = unit;
    else if (name == "v")
      ret[1] = unit;
    else if (name == "w")
      ret[2] = unit;
  }
  return ret;
}

DblMatrix scaleProjection(const DblMatrix &inMatrix,
                          const std::vector<std::string> &inUnits,
                          const std::vector<std::string> &outUnits,
                          IMDEventWorkspace_sptr inWS) {
  DblMatrix ret(inMatrix);
  // Check if we actually need to do anything
  if (std::equal(inUnits.begin(), inUnits.end(), outUnits.begin()))
    return ret;

  if (inUnits.size() != outUnits.size())
    throw std::runtime_error(
        "scaleProjection given different quantity of input and output units");

  const OrientedLattice &orientedLattice =
      inWS->getExperimentInfo(0)->sample().getOrientedLattice();

  const size_t numDims = inUnits.size();
  for (size_t i = 0; i < numDims; ++i) {
    const double dStar =
        2 * M_PI *
        orientedLattice.dstar(inMatrix[i][0], inMatrix[i][1], inMatrix[i][2]);
    if (inUnits[i] == outUnits[i])
      continue;
    else if (inUnits[i] == "a") {
      // inv angstroms to rlu
      for (size_t j = 0; j < numDims; ++j)
        ret[i][j] *= dStar;
    } else {
      // rlu to inv angstroms
      for (size_t j = 0; j < numDims; ++j)
        ret[i][j] /= dStar;
    }
  }

  return ret;
}

std::vector<MinMax> calculateExtents(const DblMatrix &inMatrix,
                                     const std::vector<MinMax> &limits) {
  DblMatrix invMat(inMatrix);
  invMat.Invert();

  // iterate through min/max of each dimension, calculate dot(vert, inv_mat)
  // and store min/max value for each dimension
  std::vector<double> hRange(2), kRange(2), lRange(2);

  hRange[0] = limits[0].first;
  hRange[1] = limits[0].second;
  kRange[0] = limits[1].first;
  kRange[1] = limits[1].second;
  lRange[0] = limits[2].first;
  lRange[1] = limits[2].second;

  // Calculate the minimums and maximums of transformed coordinates
  // Use maxDbl as a "not-yet-set" placeholder
  const double maxDbl = std::numeric_limits<double>::max();
  std::vector<MinMax> extents(3, std::make_pair(maxDbl, maxDbl));

  for (auto hIt = hRange.begin(); hIt != hRange.end(); ++hIt) {
    for (auto kIt = kRange.begin(); kIt != kRange.end(); ++kIt) {
      for (auto lIt = lRange.begin(); lIt != lRange.end(); ++lIt) {
        V3D origPos(*hIt, *kIt, *lIt);
        for (size_t i = 0; i < 3; ++i) {
          const V3D other(invMat[i][0], invMat[i][1], invMat[i][2]);
          double val = origPos.scalar_prod(other);
          // Check if min needs updating
          if (extents[i].first == maxDbl || extents[i].first > val)
            extents[i].first = val;
          // Check if max needs updating
          if (extents[i].second == maxDbl || extents[i].second < val)
            extents[i].second = val;
        }
      }
    }
  }

  return extents;
}

std::pair<std::vector<MinMax>, std::vector<size_t>>
calculateSteps(std::vector<MinMax> inExtents,
               std::vector<std::vector<double>> binning) {
  std::vector<MinMax> outExtents(inExtents);
  std::vector<size_t> outBins;

  const size_t numBins = binning.size();

  for (size_t i = 0; i < numBins; ++i) {
    const size_t nArgs = binning[i].size();
    if (nArgs == 0) {
      throw std::runtime_error("Binning parameter cannot be empty");
    } else if (nArgs == 1) {
      const double dimRange = inExtents[i].second - inExtents[i].first;
      double stepSize = binning[i][0];
      if (stepSize > dimRange)
        stepSize = dimRange;
      outExtents[i].second =
          inExtents[i].first + static_cast<double>(numBins) * stepSize;
      outBins.push_back(static_cast<size_t>(dimRange / stepSize));
    } else if (nArgs == 2) {
      outExtents[i].first = binning[i][0];
      outExtents[i].second = binning[i][1];
      outBins.push_back(1);
    } else if (nArgs == 3) {
      const double dimMin = binning[i][0];
      const double dimMax = binning[i][2];
      const double dimRange = dimMax - dimMin;
      double stepSize = binning[i][i];
      if (stepSize > dimRange)
        stepSize = dimRange;
      outExtents[i].second = dimMin + static_cast<double>(numBins) * stepSize;
      outExtents[i].first = dimMin;
      outBins.push_back(static_cast<size_t>(dimRange / stepSize));
    } else {
      throw std::runtime_error("Cannot handle " +
                               boost::lexical_cast<std::string>(nArgs) +
                               " bins.");
    }
  }
  return std::make_pair(outExtents, outBins);
}

std::vector<std::vector<std::string>>
labelProjection(const DblMatrix &projection) {
  std::vector<std::vector<std::string>> ret(3, std::vector<std::string>(3));

  const char *replacements[] = {"zeta", "eta", "xi"};

  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      const double in = projection[i][j];
      std::string out;

      if (std::abs(in) == 1)
        if (in > 0)
          out = replacements[i];
        else
          out = std::string("-") + replacements[i];
      else if (in == 0)
        out = "0";
      else
        out = boost::lexical_cast<std::string>(in) + replacements[i];

      ret[i][j] = out;
    }
  }

  return ret;
}
} // anonymous namespace

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CutMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CutMD::CutMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CutMD::~CutMD() {}

//----------------------------------------------------------------------------------------------

void CutMD::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "MDWorkspace to slice");

  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("Projection", "", Direction::Input,
                                             PropertyMode::Optional),
      "Projection");

  declareProperty(new ArrayProperty<double>("P1Bin"), "Projection 1 binning.");
  declareProperty(new ArrayProperty<double>("P2Bin"), "Projection 2 binning.");
  declareProperty(new ArrayProperty<double>("P3Bin"), "Projection 3 binning.");
  declareProperty(new ArrayProperty<double>("P4Bin"), "Projection 4 binning.");
  declareProperty(new ArrayProperty<double>("P5Bin"), "Projection 5 binning.");

  declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace", "",
                                                      Direction::Output),
                  "Output cut workspace");
  declareProperty("NoPix", false, "If False creates a full MDEventWorkspaces "
                                  "as output. True to create an "
                                  "MDHistoWorkspace as output. This is DND "
                                  "only in Horace terminology.");
}
void CutMD::exec() {
  g_log.warning("CutMD is in the beta stage of development. Its properties and "
                "behaviour may change without warning.");

  g_log.warning("point 0");
  // Collect input properties
  const IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");
  g_log.warning("point 0.5");
  const size_t numDims = inWS->getNumDims();
  const ITableWorkspace_sptr projection = getProperty("Projection");
  const bool haveProjection = !(*getProperty("Projection")).isDefault();
  std::vector<std::vector<double>> pbins(5);
  pbins[0] = getProperty("P1Bin");
  pbins[1] = getProperty("P2Bin");
  pbins[2] = getProperty("P3Bin");
  pbins[3] = getProperty("P4Bin");
  pbins[4] = getProperty("P5Bin");
  const bool noPix = getProperty("NoPix");

  // Check Projection format
  if (haveProjection) {
    auto colNames = projection->getColumnNames();
    if (colNames.size() != 4 || colNames[0] != "name" ||
        colNames[1] != "value" || colNames[2] != "type" ||
        colNames[3] != "offset")
      throw std::runtime_error(
          "Invalid Projection supplied. Please check column names.");
    if (projection->rowCount() < 3)
      throw std::runtime_error(
          "Insufficient rows in projection table. Must be 3 or more.");
  }
  g_log.warning("point 1");
  // Check PBin properties
  for (size_t i = 0; i < 5; ++i) {
    if (i < numDims && pbins[i].empty())
      throw std::runtime_error(
          "P" + boost::lexical_cast<std::string>(i + 1) +
          "Bin must be set when processing a workspace with " +
          boost::lexical_cast<std::string>(numDims) + " dimensions.");
    if (i >= numDims && !pbins[i].empty())
      throw std::runtime_error(
          "P" + boost::lexical_cast<std::string>(i + 1) +
          "Bin must NOT be set when processing a workspace with " +
          boost::lexical_cast<std::string>(numDims) + " dimensions.");
  }

  // Get extents in projection
  std::vector<MinMax> extentLimits;
  extentLimits.push_back(getDimensionExtents(inWS, 0));
  extentLimits.push_back(getDimensionExtents(inWS, 1));
  extentLimits.push_back(getDimensionExtents(inWS, 2));

  // Scale projection
  DblMatrix projectionMatrix = matrixFromProjection(projection);
  std::vector<std::string> targetUnits = unitsFromProjection(projection);
  std::vector<std::string> originUnits(3); // TODO. This is a hack!
  originUnits[0] = "r";
  originUnits[1] = "r";
  originUnits[2] = "r";

  DblMatrix scaledProjectionMatrix =
      scaleProjection(projectionMatrix, originUnits, targetUnits, inWS);

  // Calculate extents for the first 3 dimensions
  std::vector<MinMax> scaledExtents =
      calculateExtents(scaledProjectionMatrix, extentLimits);
  auto stepPair = calculateSteps(scaledExtents, pbins);
  std::vector<MinMax> steppedExtents = stepPair.first;
  std::vector<size_t> steppedBins = stepPair.second;

  g_log.warning("point 2");
  // Calculate extents for additional dimensions
  for (size_t i = 3; i < numDims; ++i) {
    const size_t nArgs = pbins[i].size();
    const MinMax extentLimit = getDimensionExtents(inWS, i);
    const double dimRange = extentLimit.second - extentLimit.first;

    if (nArgs == 1) {
      steppedExtents.push_back(extentLimit);
      steppedBins.push_back(static_cast<size_t>(dimRange / pbins[i][0]));
    } else if (nArgs == 2) {
      steppedExtents.push_back(std::make_pair(pbins[i][0], pbins[i][1]));
      steppedBins.push_back(1);
    } else if (nArgs == 3) {
      steppedExtents.push_back(std::make_pair(pbins[i][0], pbins[i][2]));
      double stepSize = pbins[i][1];
      double dimRange = extentLimit.second - extentLimit.first;
      if (stepSize > dimRange)
        stepSize = dimRange;
      steppedBins.push_back(static_cast<size_t>(dimRange / stepSize));
    }

    // and double targetUnits' length by appending itself to itself
    size_t preSize = targetUnits.size();
    targetUnits.resize(preSize * 2);
    for (size_t i = 0; i < preSize; ++i)
      targetUnits[preSize + i] = targetUnits[i];
  }

  // Make labels
  /* Matrix<std::string> labels = labelProjection(projectionMatrix); */
  std::vector<std::vector<std::string>> labels =
      labelProjection(projectionMatrix);

  g_log.warning("point 3");
  // Either run RebinMD or SliceMD
  const std::string cutAlgName = noPix ? "BinMD" : "SliceMD";
  IAlgorithm_sptr cutAlg = createChildAlgorithm(cutAlgName, 0.0, 1.0);
  cutAlg->initialize();
  g_log.warning("point 4");
  cutAlg->setProperty("InputWorkspace", inWS);
  g_log.warning("point 5");
  cutAlg->setProperty("OutputWorkspace", "sliced");
  cutAlg->setProperty("NormalizeBasisVectors", false);
  cutAlg->setProperty("AxisAligned", false);

  for (size_t i = 0; i < numDims; ++i) {
    std::string label;
    std::string unit;
    std::string vecStr;

    if (i < 3) {
      // Slicing algorithms accept name as [x,y,z]
      label = "['" + labels[i][0] + "','" + labels[i][1] + "','" +
              labels[i][2] + "']";
      unit = targetUnits[i];

      std::vector<std::string> vec(numDims, "0");
      for (size_t j = 0; j < 3; ++j)
        vec[j] = boost::lexical_cast<std::string>(projectionMatrix[i][j]);
      vecStr = boost::algorithm::join(vec, ", ");
    } else {
      // Always orthogonal
      auto dim = inWS->getDimension(i);
      label = dim->getName();
      unit = dim->getUnits();
      std::vector<std::string> vec(numDims, "0");
      vec[i] = "1";
      vecStr = boost::algorithm::join(vec, ", ");
    }

    const std::string value = label + ", " + unit + ", " + vecStr;
    cutAlg->setProperty("BasisVector" + boost::lexical_cast<std::string>(i),
                        value);
  }

  // Translate extents into a single vector
  std::vector<double> outExtents(steppedExtents.size() * 2);
  for (size_t i = 0; i < steppedExtents.size(); ++i) {
    outExtents[2 * i] = steppedExtents[i].first;
    outExtents[2 * i + 1] = steppedExtents[i].second;
  }

  g_log.warning("point 6");
  cutAlg->setProperty("OutputExtents", outExtents);
  cutAlg->setProperty("OutputBins", steppedBins);

  g_log.warning("point 7");
  cutAlg->execute();
  g_log.warning("point 8");
  Workspace_sptr sliceWS = cutAlg->getProperty("OutputWorkspace");
  IMDEventWorkspace_sptr slice =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(sliceWS);

  g_log.warning("point 9");
  if (!slice)
    throw std::runtime_error(
        "Child algorithm did not produce IMDEventWorkspace");

  // Attach projection matrix to output
  if (slice->getNumExperimentInfo() > 0) {
    ExperimentInfo_sptr info = slice->getExperimentInfo(0);
    info->mutableRun().addProperty("W_MATRIX", projectionMatrix.getVector(),
                                   true);
  }

  // Done!
  setProperty("OutputWorkspace", slice);
}

} // namespace Mantid
} // namespace MDAlgorithms
