//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FlatBackground.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <climits>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlatBackground)

using namespace Kernel;
using namespace API;

void FlatBackground::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
    new HistogramValidator<> ),
    "The input workspace must either have constant width bins or is a distribution\n"
    "workspace. It is also assumed that all spectra have the same X bin boundaries");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output ),
    "Name to use for the output workspace.");
  MandatoryValidator<double> *mustHaveValue = new MandatoryValidator<double>;
  declareProperty("StartX", Mantid::EMPTY_DBL(), mustHaveValue,
    "The X value at which to start the background fit");
  declareProperty("EndX", Mantid::EMPTY_DBL(), mustHaveValue->clone(),
    "The X value at which to end the background fit");
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
    "Indices of the spectra that will have their background removed\n"
    "default: modify all spectra");
  std::vector<std::string> modeOptions;
  modeOptions.push_back("Linear Fit");
  modeOptions.push_back("Mean");
  declareProperty("Mode","Linear Fit",new ListValidator(modeOptions),
    "The background count rate is estimated either by taking a mean or doing a\n"
    "linear fit (default: Linear Fit)");
}

void FlatBackground::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  // Copy over all the data
  const int numHists = inputWS->getNumberHistograms();
  const int blocksize = inputWS->blocksize();

  // Get the required X range
  double startX,endX;
  this->checkRange(startX,endX);

  std::vector<int> specInds = getProperty("WorkspaceIndexList");
  // check if the user passed an empty list, if so all of spec will be processed
  this->getSpecInds(specInds, numHists);

  const bool useMean = isModeMean(getProperty("mode"));
 
  // Initialise the progress reporting object
  m_progress = new Progress(this,0.0,0.3,numHists); 

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for the output
  if (outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    PARALLEL_FOR2(inputWS,outputWS)
    for (int i=0; i < numHists; ++i)
    {
		  PARALLEL_START_INTERUPT_REGION
      outputWS->dataX(i) = inputWS->readX(i);
      outputWS->dataY(i) = inputWS->readY(i);
      outputWS->dataE(i) = inputWS->readE(i);
		  m_progress->report();
		  PARALLEL_END_INTERUPT_REGION
    }
	  PARALLEL_CHECK_INTERUPT_REGION
  }
  
  // these are used to report information to the user, one progress update for each percent and a report on the size of the background found
  double prg=0.3, toFitsize=specInds.size();
  const int progStep = static_cast<int>(ceil(toFitsize/70.0));
  double backgroundTotal = 0;

  // Now loop over the required spectra
  std::vector<int>::const_iterator specIt;
  for (specIt = specInds.begin(); specIt != specInds.end(); ++specIt)
  {
    const int currentSpec = *specIt;
    try
    {
      // Only if Mean() is called will variance be changed
      double variance = -1;

      // Now call the function the user selected to calculate the background
      const double background = useMean ?
        this->Mean(outputWS, currentSpec, startX, endX, variance) :
        this->LinearFit(outputWS, currentSpec, startX, endX);
      
      if (background < 0)
      {
        g_log.warning() << "Problem with calculating the background number of counts spectrum with index "
          << currentSpec << ". The spectrum has been left unchanged.\n";
        g_log.debug() << "The background for spectra index " << currentSpec
          << "was calculated to be " << background << std::endl;
        continue;
      }
      else
      {// only used for the logging that gets done at the end
        backgroundTotal += background;
      }

      MantidVec &E = outputWS->dataE(currentSpec);
      // only the Mean() function calculates the variance
      if ( variance > 0 )
      {
        // adjust the errors using the variance (variance = error^2)
        std::transform(E.begin(), E.end(), E.begin(),
          std::bind2nd(VectorHelper::AddVariance<double>(), variance ));
      }
      // Get references to the current spectrum
      MantidVec &Y = outputWS->dataY(currentSpec);
      // Now subtract the background from the data
      for (int j=0; j < blocksize; ++j)
      {
        Y[j] -= background;
        //remove negative values
        if (Y[j] < 0.0)
        {
          Y[j]=0;
          // The error estimate must go up in this nonideal situation and the value of background is a good estimate for it. However, don't reduce the error if it was already more than that
          E[j] = E[j]>background ? E[j] : background;
        }
      }
    }
    catch (std::exception)
    {
      g_log.error() << "Error processing the spectrum with index " << currentSpec << std::endl;
      throw;
    }

    // make regular progress reports and check for cancelling the algorithm
    if ( static_cast<int>( specInds.end()-specInds.begin() ) % progStep == 0 )
    {
      interruption_point();
      prg+=(progStep*0.7/toFitsize);
      progress(prg);
    }
  } // Loop over spectra to be fitted

  g_log.information()<<"Mean of the backgrounds in the specified " << toFitsize
    << " spectra was " << backgroundTotal/toFitsize << " counts per bin\n";

  // Assign the output workspace to its property
  setProperty("OutputWorkspace",outputWS);
}

/** Checks that the range parameters have been set correctly
 *  @param startX The starting point
 *  @param endX   The ending point
 *  @throw std::invalid_argument If XMin or XMax are not set, or XMax is less than XMin
 */
void FlatBackground::checkRange(double& startX, double& endX)
{
  //use the overloaded operator =() to get the X value stored in each property
  startX = getProperty("StartX");
  endX = getProperty("EndX");
      
  if (startX > endX)
  {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    throw std::invalid_argument(failure);
  }
}

/** checks if the array is empty and if so fills it with all the index numbers
*  in the workspace. Non-empty arrays are left untouched
*  @param output the array to be checked
*  @param workspaceTotal required to be the total number of spectra in the workspace
*/
void FlatBackground::getSpecInds(std::vector<int> &output, const int workspaceTotal)
{
  if ( output.size() > 0 )
  {
    return;
  }

  output.resize(workspaceTotal);
  for ( int i = 0; i < workspaceTotal; ++i )
  {
    output[i] = i;
  }
}
/** Checks the user selected mode value
*  @param mode must be one of LinearFit or Mean
*  @return true if the user slected Mean background analysis and false if Linear Fit was selected
*  @throw invalid_argument if the mode is not recognised
*/
bool FlatBackground::isModeMean(const std::string &mode)
{
  if ( mode == "Mean" )
  {
    return true;
  }
  if ( mode == "Linear Fit" )
  {
    return false;
  }
  throw std::invalid_argument("Selected mode: \"" + mode + "\" is not recognised");
}
/** Gets the mean number of counts in each bin the background region and the variance (error^2) of that
*  number
*  @param WS points to the input workspace
*  @param specInd index of the spectrum to process
*  @param startX a X-value in the first bin that will be considered, must not be greater endX
*  @param endX a X-value in the last bin that will be considered, must not less than startX
*  @param variance will be set to the number of counts divided by the number of bins squared (= error^2)
*  @return the mean number of counts in each bin the background region
*  @throw out_of_range if either startX or endX are out of the range of X-values in the specified spectrum
*  @throw invalid_argument if endX has the value of first X-value one of the spectra
*/
double FlatBackground::Mean(const API::MatrixWorkspace_const_sptr WS, const int specInd, const double startX, const double endX, double &variance) const
{
  const MantidVec &XS = WS->readX(specInd), &YS = WS->readY(specInd);
  const MantidVec &ES = WS->readE(specInd);
  // the function checkRange should already have checked that startX <= endX, but we still need to check values weren't out side the ranges
  if ( endX > XS.back() || startX < XS.front() )
  {
    throw std::out_of_range("Either the property startX or endX is outside the range of X-values present in one of the specified spectra");
  }
  // Get the index of the first bin contains the X-value, which means this is an inclusive sum. The minus one is because lower_bound() returns index past the last index pointing to a lower value. For example if startX has a higher X value than the first bin boundary but lower than the second lower_bound returns 1, which is the index of the second bin boundary
  int startInd = std::lower_bound(XS.begin(),XS.end(),startX)-XS.begin() - 1;
  if ( startInd == -1 )
  {// happens if startX is the first X-value, e.g. the first X-value is zero and the user selects zero
    startInd = 0;
  }

  // the -1 matches definition of startIn, see the comment above that statement
  int endInd=std::lower_bound(XS.begin()+startInd,XS.end(),endX)-XS.begin()-1;
  if ( endInd == -1 )
  {// 
    throw std::invalid_argument("EndX was set to the start of one of the spectra, it must greater than the first X-value in any of the specified spectra");
  }
  
  // the +1 is because this is an inclusive sum (includes each bin that contains each X-value). Hence if startInd == endInd we are still analysising one bin
  const int numBins = (1+endInd-startInd);
  // the +1 here is because the accumulate() stops one before the location of the last iterator
  double background =
    std::accumulate( YS.begin()+startInd, YS.begin()+endInd+1, 0.0 )/numBins;
  // The error on the total number of background counts in the background region is taken as the sqrt the total number counts. To get the the error on the counts in each bin just divide this by the number of bins. The variance = error^2 that is the total variance divide by the number of bins _squared_.
  variance = 
    std::accumulate( ES.begin()+startInd, ES.begin()+endInd+1, 0.0, VectorHelper::SumSquares<double>())/(numBins*numBins);
  // return mean number of counts in each bin, the sum of the number of counts in all the bins divided by the number of bins used in that sum
  return background;
}
/// Calls Linear as a sub-algorithm to do the fitting
double FlatBackground::LinearFit(API::MatrixWorkspace_sptr WS, int spectrum, double startX, double endX)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("Linear");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("WorkspaceIndex",spectrum);
  childAlg->setProperty<double>("StartX",startX);
  childAlg->setProperty<double>("EndX",endX);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run Linear fit sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("Unable to successfully run Linear fit sub-algorithm");
    throw std::runtime_error("Unable to successfully run Linear fit sub-algorithm");
  }

  std::string fitStatus = childAlg->getProperty("FitStatus");
  if ( fitStatus != "success" )
  {
    g_log.warning("Unable to successfully fit the data");
    return -1.0;
  }

  // Calculate the value of the flat background by taking the value at the centre point of the fit
  const double c = childAlg->getProperty("FitIntercept");
  const double m = childAlg->getProperty("FitSlope");
  const double centre = (startX+endX)/2.0;
  const double background = m*centre + c;

  return background;
}

} // namespace Algorithms
} // namespace Mantid
