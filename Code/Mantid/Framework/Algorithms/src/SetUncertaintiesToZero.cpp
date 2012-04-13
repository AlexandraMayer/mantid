/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SetUncertaintiesToZero.h"
#include "MantidKernel/ListValidator.h"
#include <vector>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetUncertaintiesToZero)

/// Sets documentation strings for this algorithm
void SetUncertaintiesToZero::initDocs()
{
  this->setWikiSummary("This algorithm creates a workspace which is the duplicate of the input, but where the error value for every bin has been set to zero. ");
  this->setOptionalMessage("This algorithm creates a workspace which is the duplicate of the input, but where the error value for every bin has been set to zero.");
}


using namespace Kernel;
using namespace API;

/// (Empty) Constructor
SetUncertaintiesToZero::SetUncertaintiesToZero() : API::Algorithm()
{}

/// Virtual destructor
SetUncertaintiesToZero::~SetUncertaintiesToZero()
{}

/// Algorithm's name
const std::string SetUncertaintiesToZero::name() const
{ return "SetUncertaintiesToZero";}

/// Algorithm's version
int SetUncertaintiesToZero::version() const
{ return (1);}

const std::string ZERO("zero");
const std::string SQRT("sqrt");

void SetUncertaintiesToZero::init()
{
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",
                                                              Direction::Input));
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",
                                                              Direction::Output));
  std::vector<std::string> errorTypes;
  errorTypes.push_back(ZERO);
  errorTypes.push_back(SQRT);
  declareProperty("SetError", ZERO, boost::make_shared<StringListValidator>(errorTypes), "How to reset the uncertainties");
}

void SetUncertaintiesToZero::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  std::string errorType = getProperty("SetError");
  bool zeroError = (errorType.compare(ZERO) == 0);

  // Create the output workspace. This will copy many aspects from the input one.
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);

  // ...but not the data, so do that here.
  const size_t numHists = inputWorkspace->getNumberHistograms();
  Progress prog(this,0.0,1.0,numHists);

  PARALLEL_FOR2(inputWorkspace,outputWorkspace)
  for (int64_t i = 0; i < int64_t(numHists); ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    outputWorkspace->setX(i,inputWorkspace->refX(i));
    outputWorkspace->dataY(i) = inputWorkspace->readY(i);
    outputWorkspace->dataE(i) = std::vector<double>(inputWorkspace->readE(i).size(), 0.);

    if (!zeroError)
    {
      MantidVec& Y = outputWorkspace->dataY(i);
      MantidVec& E = outputWorkspace->dataE(i);
      std::size_t numE = E.size();
      for (std::size_t j = 0; j < numE; j++)
      {
        E[j] = sqrt(Y[j]);
      }
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid

