#include "MantidAlgorithms/CalMuonDetectorPhases.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalMuonDetectorPhases)

//----------------------------------------------------------------------------------------------
/** Initializes the algorithm's properties.
 */
void CalMuonDetectorPhases::init() {

  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Name of the reference input workspace");

  declareProperty("FirstGoodData", EMPTY_DBL(),
                  "The first good data point in units of "
                  "micro-seconds as measured from time "
                  "zero",
                  Direction::Input);

  declareProperty("LastGoodData", EMPTY_DBL(),
                  "The last good data point in units of "
                  "micro-seconds as measured from time "
                  "zero",
                  Direction::Input);

  declareProperty("Frequency", EMPTY_DBL(), "Starting hing for the frequency",
                  Direction::Input);

  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "DetectorTable", "", Direction::Output),
                  "The name of the TableWorkspace in which to store the list "
                  "of phases and asymmetries for each detector");

  declareProperty(new API::WorkspaceProperty<API::WorkspaceGroup>(
                      "DataFitted", "", Direction::Output),
                  "Name of the output workspace holding fitting results");
}

//----------------------------------------------------------------------------------------------
/** Validates the inputs.
 */
std::map<std::string, std::string> CalMuonDetectorPhases::validateInputs() {

  std::map<std::string, std::string> result;

  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Check units, should be microseconds
  Unit_const_sptr unit = inputWS->getAxis(0)->unit();
  if ((unit->caption() != "Time") || (unit->label().ascii() != "microsecond")) {
    result["InputWorkspace"] = "InputWorkspace units must be microseconds";
  }

  return result;
}
//----------------------------------------------------------------------------------------------
/** Executes the algorithm.
 */
void CalMuonDetectorPhases::exec() {

  // Get the input ws
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Get start and end time
  double startTime = getProperty("FirstGoodData");
  double endTime = getProperty("LastGoodData");

  // Get the frequency
  double freq = getProperty("Frequency");

  // Prepares the workspaces: extracts data from [startTime, endTime] and
  // removes exponential decay
  API::MatrixWorkspace_sptr tempWS = prepareWorkspace(inputWS, startTime, endTime);

  auto tab = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  auto group = API::WorkspaceGroup_sptr(new API::WorkspaceGroup());

  fitWorkspace(tempWS, freq, tab, group);

  // Set the table
  setProperty("DetectorTable", tab);
  // Set the group
  setProperty("DataFitted", group);
}

/** Fits each spectrum in the workspace to f(x) = A * sin( w * x + p)
* @param ws :: [input] The workspace to fit
* @param freq :: [input] Hint for the frequency (w)
* @param resTab :: [output] Table workspace storing the asymmetries and phases
* @param resGroup :: [output] Workspace group storing the fitting results
*/
void CalMuonDetectorPhases::fitWorkspace(const API::MatrixWorkspace_sptr &ws,
                                         double freq,
                                         API::ITableWorkspace_sptr &resTab,
                                         API::WorkspaceGroup_sptr &resGroup) {

  int nspec = static_cast<int>(ws->getNumberHistograms());

  // Create the fitting function f(x) = A * sin ( w * x + p )
  std::string funcStr = createFittingFunction(nspec, freq);
  // Create the function from string
  API::IFunction_sptr func =
      API::FunctionFactory::Instance().createInitialized(funcStr);
  // Create the multi domain function
  boost::shared_ptr<API::MultiDomainFunction> multi =
      boost::dynamic_pointer_cast<API::MultiDomainFunction>(func);
  // Set the domain indices
  for (int i = 0; i < nspec; ++i) {
    multi->setDomainIndex(i, i);
  }

  API::IAlgorithm_sptr fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function",
                   boost::dynamic_pointer_cast<API::IFunction>(multi));
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("WorkspaceIndex", 0);
  for (int s = 1; s < nspec; s++) {
    std::string suffix = boost::lexical_cast<std::string>(s);
    fit->setProperty("InputWorkspace_" + suffix, ws);
    fit->setProperty("WorkspaceIndex_" + suffix, s);
  }
  fit->setProperty("CreateOutput", true);
  fit->execute();

  // Get the parameter table
  API::ITableWorkspace_sptr tab = fit->getProperty("OutputParameters");
  // Now we have our fitting results stored in tab
  // but we need to extract the relevant information, i.e.
  // the detector phases (parameter 'p') and asymmetries ('A')
  resTab = extractDetectorInfo(tab, static_cast<size_t>(nspec));

  // Get the fitting results
  resGroup = fit->getProperty("OutputWorkspace");

}

/** Extracts detector asymmetries and phases from fitting results
* @param paramTable :: [input] Output parameter table resulting from the fit
* @param nspec :: [input] Number of detectors/spectra
* @return :: A new table workspace storing the asymmetries and phases
*/
API::ITableWorkspace_sptr CalMuonDetectorPhases::extractDetectorInfo(
    const API::ITableWorkspace_sptr &paramTab, size_t nspec) {

  // Make sure paramTable is the right size
  // It should contain three parameters per detector/spectrum plus 'const function value'
  if (paramTab->rowCount() != nspec * 3 + 1) {
    throw std::invalid_argument("Can't extract detector parameters from fit results");
  }

  // Create the table to store detector info
  auto tab = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  tab->addColumn("int", "Detector");
  tab->addColumn("double", "Asymmetry");
  tab->addColumn("double", "Phase");

  for (int s = 0; s < nspec; s++) {
    // The following '3' factor corresponds to the number of function params
    size_t specRow = s * 3;
    double asym = paramTab->Double(specRow,1);
    double phase = paramTab->Double(specRow+2,1);
    // Handle phases greather (less) than 2*PI (-2*PI)
    if (phase > 2 * M_PI) {
      phase = phase - 2 * M_PI;
    } else if (phase < -2 * M_PI) {
      phase = phase + 2 * M_PI;
    }
    // If asym<0, take the absolute value and add \pi to phase
    // f(x) = A * sin( w * x + p) = -A * sin( w * x + p + PI)
    if (asym<0) {
      asym = -asym;
      phase = phase + M_PI;
    }
    // Copy parameters to new table
    API::TableRow row = tab->appendRow();
    row << s << asym << phase;
  }

  return tab;
}

/** Creates the fitting function f(x) = A * sin( w*x + p) as string
* @param nspec :: [input] The number of domains (spectra)
* @param freq :: [input] Hint for the frequency (w)
* @returns :: The fitting function as a string
*/
std::string CalMuonDetectorPhases::createFittingFunction(int nspec, double freq) {

  // The fitting function is:
  // f(x) = A * sin ( w * x + p )
  // where w is shared across workspaces
  std::ostringstream ss;
  ss << "composite=MultiDomainFunction,NumDeriv=true;";
  for (int s = 0; s < nspec; s++) {
    ss << "name=UserFunction,";
    ss << "Formula=A*sin(w*x+p),";
    ss << "A=0.5,";
    ss << "w=" << freq << ",";
    ss << "p=0.;";
  }
  ss << "ties=(";
  for (int s = 1; s < nspec - 1; s++) {
    ss << "f";
    ss << boost::lexical_cast<std::string>(s);
    ss << ".w=f0.w,";
  }
  ss << "f";
  ss << boost::lexical_cast<std::string>(nspec - 1);
  ss << ".w=f0.w)";

  return ss.str();
}

/** Extracts relevant data from a workspace and removes exponential decay
* @param ws :: [input] The input workspace
* @param startTime :: [input] First X value to consider
* @param endTime :: [input] Last X value to consider
* @return :: Pre-processed workspace to fit
*/
API::MatrixWorkspace_sptr
CalMuonDetectorPhases::prepareWorkspace(const API::MatrixWorkspace_sptr &ws,
                                        double startTime, double endTime) {

  // If startTime and/or endTime are EMPTY_DBL():
  if (startTime == EMPTY_DBL()) {
    // TODO set to zero for now, it should be read from FirstGoodBin
    startTime = 0.;
  }
  if (endTime == EMPTY_DBL()) {
    // Last available time
    endTime = ws->readX(0).back();
  }

  // Extract counts from startTime to endTime
  API::IAlgorithm_sptr crop = createChildAlgorithm("CropWorkspace");
  crop->setProperty("InputWorkspace",ws);
  crop->setProperty("XMin",startTime);
  crop->setProperty("XMax",endTime);
  crop->executeAsChildAlg();
  boost::shared_ptr<API::MatrixWorkspace> wsCrop =
      crop->getProperty("OutputWorkspace");

  // Remove exponential decay
  API::IAlgorithm_sptr remove = createChildAlgorithm("RemoveExpDecay");
  remove->setProperty("InputWorkspace",wsCrop);
  remove->executeAsChildAlg();
  boost::shared_ptr<API::MatrixWorkspace> wsRem =
      remove->getProperty("OutputWorkspace");

  return wsRem;

}

} // namespace Algorithms
} // namespace Mantid
