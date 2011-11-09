/*WIKI* 

This algorithm searches the specified spectra in a workspace for peaks, returning a list of the found and successfully fitted peaks. The search algorithm is described in full in reference [1]. In summary: the second difference of each spectrum is computed and smoothed. This smoothed data is then searched for patterns consistent with the presence of a peak. The list of candidate peaks found is passed to a fitting routine and those that are successfully fitted are kept and returned in the output workspace (and logged at information level).

The output [[TableWorkspace]] contains the following columns, which reflect the fact that the peak has been fitted to a Gaussian atop a linear background: spectrum, centre, width, height, backgroundintercept & backgroundslope.

====Subalgorithms used====
FindPeaks uses the [[SmoothData]] algorithm to, well, smooth the data - a necessary step to identify peaks in statistically fluctuating data. The [[Gaussian]] algorithm is used to fit candidate peaks.

==== References ====
# M.A.Mariscotti, ''A method for automatic identification of peaks in the presence of background and its application to spectrum analysis'', NIM '''50''' (1967) 309.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/algorithm/string.hpp>
#include <numeric>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindPeaks)

/// Sets documentation strings for this algorithm
void FindPeaks::initDocs()
{
  this->setWikiSummary("Searches for peaks in a dataset. ");
  this->setOptionalMessage("Searches for peaks in a dataset.");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Constructor
FindPeaks::FindPeaks() : API::Algorithm(),m_progress(NULL) {}


//=================================================================================================
/** Initialize and declare properties.
 *
 */
void FindPeaks::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "Name of the workspace to search" );

  BoundedValidator<int> *min = new BoundedValidator<int>();
  min->setLower(1);
  // The estimated width of a peak in terms of number of channels
  declareProperty("FWHM",7,min,
    "Estimated number of points covered by the fwhm of a peak (default 7)" );

  // The tolerance allowed in meeting the conditions
  declareProperty("Tolerance", 4, min->clone(),
    "A measure of the strictness desired in meeting the condition on peak candidates,\n"
    "Mariscotti recommends 2 (default 4)");
  
  declareProperty("PeakPositions", "",
    "Optional: enter a comma-separated list of the expected X-position of the centre of the peaks. Only peaks near these positions will be fitted." );

  std::vector<std::string> bkgdtypes;
  bkgdtypes.push_back("Linear");
  bkgdtypes.push_back("Quadratic");
  declareProperty("BackgroundType", "Linear", new ListValidator(bkgdtypes),
      "Type of Background. The choice can be either Linear or Quadratic");

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex",EMPTY_INT(),mustBePositive,
    "If set, only this spectrum will be searched for peaks (otherwise all are)");  
  
  declareProperty("HighBackground", true,
      "Relatively weak peak in high background");

  // The found peaks in a table
  declareProperty(new WorkspaceProperty<API::ITableWorkspace>("PeaksList","",Direction::Output),
    "The name of the TableWorkspace in which to store the list of peaks found" );
  

  // Debug Workspaces
  /*
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("BackgroundWorkspace", "", Direction::Output),
      "Temporary Background Workspace ");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("TheorticBackgroundWorkspace", "", Direction::Output),
      "Temporary Background Workspace ");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("PeakWorkspace", "", Direction::Output),
      "Temporary Background Workspace ");
  */


  // Set up the columns for the TableWorkspace holding the peak information
  m_peaks = WorkspaceFactory::Instance().createTable("TableWorkspace");
  m_peaks->addColumn("int","spectrum");
  m_peaks->addColumn("double","centre");
  m_peaks->addColumn("double","width");
  m_peaks->addColumn("double","height");
  m_peaks->addColumn("double","backgroundintercept");
  m_peaks->addColumn("double","backgroundslope");
  m_peaks->addColumn("double", "A2");
}


//=================================================================================================
/** Execute the findPeaks algorithm.
 *
 */
void FindPeaks::exec()
{
  // Retrieve the input workspace
  inputWS = getProperty("InputWorkspace");
  
  // If WorkspaceIndex has been set it must be valid
  index = getProperty("WorkspaceIndex");
  singleSpectrum = !isEmpty(index);
  if ( singleSpectrum && index >= static_cast<int>(inputWS->getNumberHistograms()) )
  {
    g_log.error() << "The value of WorkspaceIndex provided (" << index << 
      ") is larger than the size of this workspace (" <<
      inputWS->getNumberHistograms() << ")\n";
    throw Kernel::Exception::IndexError(index,inputWS->getNumberHistograms()-1,
      "FindPeaks WorkspaceIndex property");
  }

  //Get some properties
  this->fwhm = getProperty("FWHM");

  //Get the specified peak positions, which is optional
  std::string peakPositions = getProperty("PeakPositions");

  std::string backgroundtype = getProperty("BackgroundType");

  mHighBackground = getProperty("HighBackground");

  if (peakPositions.size() > 0)
  {
    //Split the string and turn it into a vector.
    std::vector<double> centers = Kernel::VectorHelper::splitStringIntoVector<double>(peakPositions);

    //Perform fit with fixed start positions.
    //std::cout << "Number of Centers = " << centers.size() << std::endl;
    this->findPeaksGivenStartingPoints(centers, backgroundtype);
  }
  else
  {
    //Use Mariscotti's method to find the peak centers
    this->findPeaksUsingMariscotti(backgroundtype);
  }

  g_log.information() << "Total of " << m_peaks->rowCount() << " peaks found and successfully fitted." << std::endl;
  setProperty("PeaksList",m_peaks);
}


//=================================================================================================
/** Use the Mariscotti method to find the start positions to fit gaussian peaks
 * @param peakCenters: vector of the center x-positions specified to perform fits.
 */
void FindPeaks::findPeaksGivenStartingPoints(std::vector<double> peakCenters, std::string backgroundtype)
{
  std::vector<double>::iterator it;

  // Loop over the spectra searching for peaks
  const int start = singleSpectrum ? index : 0;
  const int end = singleSpectrum ? index+1 : static_cast<int>(inputWS->getNumberHistograms());
  m_progress = new Progress(this,0.0,1.0,end-start);

  for (int spec = start; spec < end; ++spec)
  {

    const MantidVec& datax = inputWS->dataX(spec);

    for (it = peakCenters.begin(); it != peakCenters.end(); ++it)
    {
      //Try to fit at this center
      double x_center = *it;
      // Check whether it is the in data range
      if (x_center > datax[0] && x_center < datax[datax.size()-1]){
        this->fitPeak(inputWS, spec, x_center, this->fwhm, backgroundtype);
      }

    } // loop through the peaks specified

  m_progress->report();

  } // loop over spectra

}


//=================================================================================================
/** Use the Mariscotti method to find the start positions to fit gaussian peaks
 *
 */
void FindPeaks::findPeaksUsingMariscotti(std::string backgroundtype)
{

  //At this point the data has not been smoothed yet.
  MatrixWorkspace_sptr smoothedData = this->calculateSecondDifference(inputWS);

  // The optimum number of points in the smoothing, according to Mariscotti, is 0.6*fwhm
  int w = static_cast<int>(0.6 * fwhm);
  // w must be odd
  if (!(w%2)) ++w;

  // Carry out the number of smoothing steps given by g_z (should be 5)
  for (int i = 0; i < g_z; ++i)
  {
    this->smoothData(smoothedData,w);
  }
  // Now calculate the errors on the smoothed data
  this->calculateStandardDeviation(inputWS,smoothedData,w);

  // Calculate n1 (Mariscotti eqn. 18)
  const double kz = 1.22; // This kz corresponds to z=5 & w=0.6*fwhm - see Mariscotti Fig. 8
  const int n1 = static_cast<int>(kz * fwhm + 0.5);
  // Can't calculate n2 or n3 yet because they need i0
  const int tolerance = getProperty("Tolerance");

//  // Temporary - to allow me to look at smoothed data
//  setProperty("SmoothedData",smoothedData);

  // Loop over the spectra searching for peaks
  const int start = singleSpectrum ? index : 0;
  const int end = singleSpectrum ? index+1 : static_cast<int>(smoothedData->getNumberHistograms());
  m_progress = new Progress(this,0.0,1.0,end-start);
  const int blocksize = static_cast<int>(smoothedData->blocksize());

  for (int k = start; k < end; ++k)
  {
    const MantidVec &S = smoothedData->readY(k);
    const MantidVec &F = smoothedData->readE(k);

    // This implements the flow chart given on page 320 of Mariscotti
    int i0 = 0, i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
    for ( int i = 1; i < blocksize; ++i)
    {

      int M = 0;
      if ( S[i] > F[i] ) M = 1;
      else { S[i] > 0 ? M = 2 : M = 3; }

      if ( S[i-1] > F[i-1] )
      {
        switch (M)
        {
        case 3:
          i3 = i;
          // intentional fall-through
        case 2:
          i2 = i-1;
          break;
        case 1:
          // do nothing
          break;
        default:
          assert( false ); // should never happen
        }
      }
      else if ( S[i-1] > 0 )
      {
        switch (M)
        {
        case 3:
          i3 = i;
          break;
        case 2:
          // do nothing
          break;
        case 1:
          i1 = i;
          break;
        default:
          assert( false ); // should never happen
        }
      }
      else
      {
        switch (M)
        {
        case 3:
          // do nothing
          break;
        case 2: // fall through (i.e. same action if M = 1 or 2)
        case 1:
          i5 = i-1;
          break;
        default:
          assert( false ); // should never happen
        }
      }

      if ( i5 && i1 && i2 && i3 ) // If i5 has been set then we should have the full set and can check conditions
      {
        i4 = i3; // Starting point for finding i4 - calculated below
        double num = 0.0, denom = 0.0;
        for ( int j = i3; j <= i5; ++j )
        {
          // Calculate i4 - it's at the minimum value of Si between i3 & i5
          if ( S[j] <= S[i4] ) i4 = j;
          // Calculate sums for i0 (Mariscotti eqn. 27)
          num += j * S[j];
          denom += S[j];
        }
        i0 = static_cast<int>(num/denom);

        // Check we have a correctly ordered set of points. If not, reset and continue
        if ( i1>i2 || i2>i3 || i3>i4 || i5<=i4 )
        {
          i5 = 0;
          continue;
        }

        // Check if conditions are fulfilled - if any are not, loop onto the next i in the spectrum
        // Mariscotti eqn. (14)
        if ( std::abs(S[i4]) < 2*F[i4] )
        {
          i5 = 0;
          continue;
        }
        // Mariscotti eqn. (19)
        if ( abs( i5-i3+1-n1 ) > tolerance )
        {
          i5 = 0;
          continue;
        }
        // Calculate n2 (Mariscotti eqn. 20)
        int n2 = abs( static_cast<int>(0.5*(F[i0]/S[i0])*(n1+tolerance)+0.5) );
        const int n2b = abs( static_cast<int>(0.5*(F[i0]/S[i0])*(n1-tolerance)+0.5) );
        if (n2b > n2) n2 = n2b;
        // Mariscotti eqn. (21)
        const int testVal = n2 ? n2 : 1;
        if ( i3-i2-1 > testVal )
        {
          i5 = 0;
          continue;
        }
        // Calculate n3 (Mariscotti eqn. 22)
        int n3 = abs( static_cast<int>((n1+tolerance)*(1-2*(F[i0]/S[i0])) + 0.5) );
        const int n3b = abs( static_cast<int>((n1-tolerance)*(1-2*(F[i0]/S[i0])) + 0.5) );
        if ( n3b < n3 ) n3 = n3b;
        // Mariscotti eqn. (23)
        if ( i2-i1+1 < n3 )
        {
          i5 = 0;
          continue;
        }

        // If we get to here then we've identified a peak
        g_log.debug() << "Spectrum=" << k << " i0=" << inputWS->readX(k)[i0] << " i1=" << i1 << " i2=" << i2 << " i3=" << i3 << " i4=" << i4 << " i5=" << i5 << std::endl;

        this->fitPeak(inputWS,k,i0,i2,i4, backgroundtype);
        
        // reset and go searching for the next peak
        i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
      }

    } // loop through a single spectrum

  m_progress->report();

  } // loop over spectra

}

//=================================================================================================
/** Calculates the second difference of the data (Y values) in a workspace.
 *  Done according to equation (3) in Mariscotti: \f$ S_i = N_{i+1} - 2N_i + N_{i+1} \f$.
 *  In the output workspace, the 2nd difference is in Y, X is unchanged and E is zero.
 *  @param input :: The workspace to calculate the second difference of
 *  @return A workspace containing the second difference
 */
API::MatrixWorkspace_sptr FindPeaks::calculateSecondDifference(const API::MatrixWorkspace_const_sptr &input)
{
  // We need a new workspace the same size as the input ont
  MatrixWorkspace_sptr diffed = WorkspaceFactory::Instance().create(input);

  const size_t numHists = input->getNumberHistograms();
  const size_t blocksize = input->blocksize();

  // Loop over spectra
  for (size_t i = 0; i < size_t(numHists); ++i)
  {
    // Copy over the X values
    diffed->dataX(i) = input->readX(i);
    
    const MantidVec &Y = input->readY(i);
    MantidVec &S = diffed->dataY(i);
    // Go through each spectrum calculating the second difference at each point
    // First and last points in each spectrum left as zero (you'd never be able to find peaks that close to the edge anyway)
    for (size_t j = 1; j < blocksize-1; ++j)
    {
      S[j] = Y[j-1] - 2*Y[j] + Y[j+1];
    }
  }

  return diffed;
}

//=================================================================================================
/** Calls the SmoothData algorithm as a sub-algorithm on a workspace
 *  @param WS :: The workspace containing the data to be smoothed. The smoothed result will be stored in this pointer.
 *  @param w ::  The number of data points which should contribute to each smoothed point
 */
void FindPeaks::smoothData(API::MatrixWorkspace_sptr &WS, const int &w)
{
  g_log.information("Smoothing the input data");
  IAlgorithm_sptr smooth = createSubAlgorithm("SmoothData");
  smooth->setProperty("InputWorkspace", WS);
  // The number of points which contribute to each smoothed point
  smooth->setProperty("NPoints",w);
  smooth->executeAsSubAlg();
  // Get back the result
  WS = smooth->getProperty("OutputWorkspace");
}


//=================================================================================================
/** Calculates the statistical error on the smoothed data.
 *  Uses Mariscotti equation (11), amended to use errors of input data rather than sqrt(Y).
 *  @param input ::    The input data to the algorithm
 *  @param smoothed :: The smoothed dataBackgroud type is not supported in FindPeak.cpp
 *  @param w ::        The value of w (the size of the smoothing 'window')
 *  @throw std::invalid_argument if w is greater than 19
 */
void FindPeaks::calculateStandardDeviation(const API::MatrixWorkspace_const_sptr &input, const API::MatrixWorkspace_sptr &smoothed, const int &w)
{
  // Guard against anyone changing the value of z, which would mean different phi values were needed (see Marriscotti p.312)
  assert( g_z == 5 );
  // Have to adjust for fact that I normalise Si (unlike the paper)
  const int factor = static_cast<int>(std::pow(static_cast<double>(w),g_z));

  const double constant = sqrt(static_cast<double>(this->computePhi(w))) / factor;
  
  const size_t numHists = smoothed->getNumberHistograms();
  const size_t blocksize = smoothed->blocksize();
  for (size_t i = 0; i < size_t(numHists); ++i)
  {
    const MantidVec &E = input->readE(i);
    MantidVec &Fi = smoothed->dataE(i);

    for (size_t j = 0; j < blocksize; ++j)
    {
      Fi[j] = constant * E[j];
    }
  }
}

//=================================================================================================
/** Calculates the coefficient phi which goes into the calculation of the error on the smoothed data
 *  Uses Mariscotti equation (11). Pinched from the GeneralisedSecondDifference code.
 *  Can return a very big number, hence the type.
 *  @param  w The value of w (the size of the smoothing 'window')
 *  @return The value of phi(g_z,w)
 */
long long FindPeaks::computePhi(const int& w) const
{
  const int m = (w-1)/2;
  int zz=0;
  int max_index_prev=1;
  int n_el_prev=3;
  std::vector<long long> previous(n_el_prev);
  previous[0]=1;
  previous[1]=-2;
  previous[2]=1;

  // Can't happen at present
  if (g_z==0) return std::accumulate(previous.begin(),previous.end(),static_cast<long long>(0),VectorHelper::SumSquares<long long>());
  
  std::vector<long long> next;
  // Calculate the Cij iteratively.
  do
  {
    zz++;
    int max_index=zz*m+1;
    int n_el=2*max_index+1;
    next.resize(n_el);
    std::fill(next.begin(),next.end(),0);
    for (int i=0;i<n_el;++i)
    {
      int delta=-max_index+i;
      for (int l=delta-m;l<=delta+m;l++)
      {
        int index=l+max_index_prev;
        if (index>=0 && index<n_el_prev) next[i]+=previous[index];
      }
    }
    previous.resize(n_el);
    std::copy(next.begin(),next.end(),previous.begin());
    max_index_prev=max_index;
    n_el_prev=n_el;
  } while (zz != g_z);

  const long long retval = std::accumulate(previous.begin(),previous.end(),static_cast<long long>(0),VectorHelper::SumSquares<long long>());
  g_log.debug() << "FindPeaks::computePhi - calculated value = " << retval << "\n";
  return retval;
}

//=================================================================================================
/** Attempts to fit a candidate peak
 * 
 *  @param input ::    The input workspace
 *  @param spectrum :: The spectrum index of the peak (is actually the WorkspaceIndex)
 *  @param i0 ::       Channel number of peak candidate i0 - the higher side of the peak (right side)
 *  @param i2 ::       Channel number of peak candidate i2 - the lower side of the peak (left side)
 *  @param i4 ::       Channel number of peak candidate i4 - the center of the peak
 */

void FindPeaks::fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const int i0, const int i2, const int i4,
    std::string backgroundtype)
{
  const MantidVec &X = input->readX(spectrum);
  const MantidVec &Y = input->readY(spectrum);
  
  g_log.debug() << "Fit Peak @ " << X[i4] << "  of Spectrum " << spectrum << "  ";
  g_log.debug() << "Peak In Range " << X[i0] << ", " << X[i2] << "  Peak @ " << X[i4] << std::endl;

  // Get the initial estimate of the width, in # of bins
  const int fitWidth = i0-i2;

  // See Mariscotti eqn. 20. Using l=1 for bg0/bg1 - correspond to p6 & p7 in paper.
  unsigned int i_min = 1;
  if (i0 > static_cast<int>(5*fitWidth)) i_min = i0 - 5*fitWidth;
  unsigned int i_max = i0 + 5*fitWidth;
  // Bounds checks
  if (i_min<1) i_min=1;
  if (i_max>=Y.size()-1) i_max=static_cast<unsigned int>(Y.size()-2); // TODO this is dangerous

  g_log.debug() << "Background + Peak -- Bounds = " << X[i_min] << ", " << X[i_max] << std::endl;

  // Estimate height, boundary, and etc for fitting
  const double bg_lowerSum = Y[i_min-1] + Y[i_min] + Y[i_min+1];
  const double bg_upperSum = Y[i_max-1] + Y[i_max] + Y[i_max+1];
  const double in_bg0 = (bg_lowerSum + bg_upperSum) / 6.0;
  const double in_bg1 = (bg_upperSum - bg_lowerSum) / (3.0*(i_max-i_min+1));
  const double in_bg2 = 0.0;
  const double in_height = Y[i4] - in_bg0;
  const double in_centre = input->isHistogramData() ? 0.5*(X[i0]+X[i0+1]) : X[i0];

  // TODO max guessed width = 10 is good for SNS.  But it may be broken in extreme case
  if (!mHighBackground){

    /** Not high background.  Fit background and peak together
     *  The original Method
     */

    for (unsigned int width = 2; width <= 10; width +=2)
    {

      // a) Set up sub algorithm Fit
      IAlgorithm_sptr fit;
      try
      {
        // Fitting the candidate peaks to a Gaussian
        fit = createSubAlgorithm("Fit", -1, -1, true);
      } catch (Exception::NotFoundError &)
      {
        g_log.error("The StripPeaks algorithm requires the CurveFitting library");
        throw;
      }
      fit->setProperty("InputWorkspace", input);
      fit->setProperty("WorkspaceIndex", spectrum);
      fit->setProperty("MaxIterations", 50);

      // b) Guess sigma
      const double in_sigma = (i0 + width < X.size()) ? X[i0 + width] - X[i0] : 0.;

      // c) Construct Function string
      std::stringstream ss;
      if (backgroundtype.compare("Linear") == 0)
      {
        ss << "name=Gaussian,Height=" << in_height << ",PeakCentre=" << in_centre << ",Sigma="
            << in_sigma << ";name=LinearBackground,A0=" << in_bg0 << ",A1=" << in_bg1;
      }
      else if (backgroundtype.compare("Quadratic") == 0)
      {
        ss << "name=Gaussian,Height=" << in_height << ",PeakCentre=" << in_centre << ",Sigma="
            << in_sigma << ";name=QuadraticBackground,A0=" << in_bg0 << ",A1=" << in_bg1 << ",A2="
            << in_bg2;
      }
      else
      {
        g_log.error() << "Background type " << backgroundtype << " is not supported in FindPeak.cpp!"
            << std::endl;
        throw std::invalid_argument("Background type is not supported in FindPeak.cpp");
      }
      std::string function = ss.str();
      g_log.debug() << "Background Type = " << backgroundtype << "  Function: " << function << std::endl;

      // d) complete fit
      fit->setProperty("StartX", (X[i0] - 5 * (X[i0] - X[i2])));
      fit->setProperty("EndX", (X[i0] + 5 * (X[i0] - X[i2])));
      fit->setProperty("Minimizer", "Levenberg-Marquardt");
      fit->setProperty("CostFunction", "Least squares");
      fit->setProperty("Function", function);

      // e) Fit and get result
      fit->executeAsSubAlg();

      std::string fitStatus = fit->getProperty("OutputStatus");
      std::vector<double> params = fit->getProperty("Parameters");
      std::vector<std::string> paramnames = fit->getProperty("ParameterNames");

      // Check order of names
      if (paramnames[0].compare("f0.Height") != 0)
      {
        g_log.error() << "Parameter 0 should be f0.Height, but is " << paramnames[0] << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be f0.Height");
      }
      if (paramnames[1].compare("f0.PeakCentre") != 0)
      {
        g_log.error() << "Parameter 1 should be f0.PeakCentre, but is " << paramnames[1]
            << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be f0.PeakCentre");
      }
      if (paramnames[2].compare("f0.Sigma") != 0)
      {
        g_log.error() << "Parameter 2 should be f0.Sigma, but is " << paramnames[2] << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be f0.Sigma");
      }
      if (paramnames[3].compare("f1.A0") != 0)
      {
        g_log.error() << "Parameter 3 should be f1.A0, but is " << paramnames[3] << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be f1.A0");
      }
      if (paramnames[4].compare("f1.A1") != 0)
      {
        g_log.error() << "Parameter 4 should be f1.A1, but is " << paramnames[4] << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be f1.A1");
      }
      if (backgroundtype.compare("Quadratic") == 0 && paramnames[5].compare("f1.A2") != 0)
      {
        g_log.error() << "Parameter 5 should be f1.A2, but is " << paramnames[5] << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be f1.A2");
      }

      double height = params[0];
      if (height <= 0)
        fitStatus.clear(); // Height must be strictly positive

      if (!fitStatus.compare("success"))
      {
        const double centre = params[1];
        const double width = params[2];
        const double bgintercept = params[3];
        const double bgslope = params[4];

        if ((centre != centre) || (width != width) || (bgintercept != bgintercept) || (bgslope
            != bgslope))
        {
          g_log.information() << "NaN detected in the results of peak fitting. Peak ignored."
              << std::endl;
        }
        else
        {
          g_log.debug() << "Peak Fitted. Centre=" << centre << ", Sigma=" << width
              << ", Height=" << height << ", Background slope=" << bgslope
              << ", Background intercept=" << bgintercept << std::endl;
          API::TableRow t = m_peaks->appendRow();
          t << spectrum << centre << width << height << bgintercept << bgslope;
        }

        break;
      } // if SUCCESS
      else
      {
        g_log.debug() << "Fit Status = " << fitStatus << std::endl;
      } // if FAILED

    }
  } // // not high background
  else {

    /** High background
     **/

    fitPeakHighBackground(input, spectrum, i0, i2, i4, i_min, i_max, in_bg0, in_bg1, in_bg2, backgroundtype);

  } // if high background

  g_log.debug() << "Fit Peak Over" << std::endl;

  return;

}

//=================================================================================================
/** Attempts to fit a candidate peak given a center and width guess.
 *
 *  @param input ::    The input workspace
 *  @param spectrum :: The spectrum index of the peak (is actually the WorkspaceIndex)
 *  @param center_guess: A guess of the X-value of the center of the peak, in whatever units of the X-axis of the workspace.
 *  @param FWHM_guess: A guess of the full-width-half-max of the peak, in # of bins.
*/
void FindPeaks::fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const double center_guess, const int FWHM_guess,
    std::string backgroundtype)
{
  //The indices
  int i_left, i_right, i_center;

  //The X axis you are looking at
  const MantidVec &X = input->readX(spectrum);

  //find i_center - the index of the center
  i_center = 0;
  //The guess is within the X axis?
  if (X[0] < center_guess)
    for (i_center=0; i_center<static_cast<int>(X.size()-1); i_center++)
    {
      if ((center_guess >= X[i_center]) && (center_guess < X[i_center+1]))
        break;
    }
  //else, bin 0 is closest to it by default;

  i_left = i_center - FWHM_guess / 2;
  i_right = i_left + FWHM_guess;

  this->fitPeak(input, spectrum, i_right, i_left, i_center, backgroundtype);

  return;

}

/*
 * Fit peak with high background
 *
 * @param  X, Y, Z: MantidVec&
 * @param i0: bin index of right end of peak
 * @param i2: bin index of left end of peak
 * @param i4: bin index of center of peak
 * @param i_min: bin index of left bound of fit range
 * @param i_max: bin index of right bound of fit range
 * @param in_bg0: guessed value of a0
 * @param in_bg1: guessed value of a1
 * @param in_bg2: guessed value of a2
 * @param backgroundtype: type of background (linear or quadratic)
 */
void FindPeaks::fitPeakHighBackground(const API::MatrixWorkspace_sptr &input, const int spectrum, const int& i0, const int& i2, const int& i4,
    const unsigned int& i_min, const unsigned int& i_max,
    const double& in_bg0, const double& in_bg1, const double& in_bg2, std::string& backgroundtype){

  const MantidVec &X = input->readX(spectrum);
  const MantidVec &Y = input->readY(spectrum);
  const MantidVec &E = input->readE(spectrum);

  // a) Construct a Workspace to fit for background
  std::vector<double> newX, newY, newE;
  for (size_t i = i_min; i <= i_max; i ++){
    if (i > size_t(i0) || i < size_t(i2)){
      newX.push_back(X[i]);
      newY.push_back(Y[i]);
      newE.push_back(E[i]);
    }
  }

  if (newX.size() < 3){
    g_log.error() << "Size of new Workspace = " << newX.size() << "  Too Small! "<< std::endl;
    throw;
  }

  API::MatrixWorkspace_sptr bkgdWS = API::WorkspaceFactory::Instance().create("Workspace2D", 1, newX.size(), newY.size());
  MantidVec& wsX = bkgdWS->dataX(0);
  MantidVec& wsY = bkgdWS->dataY(0);
  MantidVec& wsE = bkgdWS->dataE(0);
  for (size_t i = 0; i < newY.size(); i ++)
  {
    wsX[i] = newX[i];
    wsY[i] = newY[i];
    wsE[i] = newE[i];
  }

  // b) Fit background
  IAlgorithm_sptr fit;
  try
  {
    // Fitting the candidate peaks to a Gaussian
    fit = createSubAlgorithm("Fit", -1, -1, true);
  } catch (Exception::NotFoundError &)
  {
    g_log.error("The StripPeaks algorithm requires the CurveFitting library");
    throw;
  }
  fit->setProperty("InputWorkspace", bkgdWS);
  fit->setProperty("WorkspaceIndex", 0);
  fit->setProperty("MaxIterations", 50);

  // c) Construct Function string
  std::stringstream ss;
  if (backgroundtype.compare("Linear") == 0)
  {
    ss << "name=LinearBackground,A0=" << in_bg0 << ",A1=" << in_bg1;
  }
  else if (backgroundtype.compare("Quadratic") == 0)
  {
    ss << "name=QuadraticBackground,A0=" << in_bg0 << ",A1=" << in_bg1 << ",A2="<< in_bg2;
  }
  else
  {
    g_log.error() << "Background type " << backgroundtype << " is not supported in FindPeak.cpp!"
        << std::endl;
    throw std::invalid_argument("Background type is not supported in FindPeak.cpp");
  }
  std::string function = ss.str();
  g_log.debug() << "Background Type = " << backgroundtype << "  Function: " << function << std::endl;

  // d) complete fit
  fit->setProperty("StartX", (X[i0] - 5 * (X[i0] - X[i2])));
  fit->setProperty("EndX", (X[i0] + 5 * (X[i0] - X[i2])));
  fit->setProperty("Minimizer", "Levenberg-Marquardt");
  fit->setProperty("CostFunction", "Least squares");
  fit->setProperty("Function", function);

  // e) Fit and get result
  fit->executeAsSubAlg();

  std::string fitStatus = fit->getProperty("OutputStatus");
  std::vector<double> params = fit->getProperty("Parameters");
  std::vector<std::string> paramnames = fit->getProperty("ParameterNames");

  if (fitStatus.compare("success")){
    g_log.error() << "Fit " << backgroundtype << " Fails For Peak @ " << X[i4] << std::endl;
    throw;
  }

  double a0 = params[0];
  double a1 = params[1];
  double a2 = 0;
  if (backgroundtype.compare("Quadratic")==0){
    a2 = params[2];
  }
  g_log.debug() << "Backgound parameters: a0 = " << a0 << "  a1 = " << a1 << "  a2 = " << a2 << std::endl;

  // f) Create theoretic background workspace and thus peak workspace
  // TODO theortic background workspace will be removed when the code is matured
  size_t fitsize = i_max-i_min+1;
  API::MatrixWorkspace_sptr tbkgdWS = API::WorkspaceFactory::Instance().create("Workspace2D", 1, fitsize, fitsize);
  API::MatrixWorkspace_sptr peakWS = API::WorkspaceFactory::Instance().create("Workspace2D", 1, fitsize, fitsize);
  MantidVec& tX = tbkgdWS->dataX(0);
  MantidVec& tY = tbkgdWS->dataY(0);
  MantidVec& tE = tbkgdWS->dataE(0);
  MantidVec& pX = peakWS->dataX(0);
  MantidVec& pY = peakWS->dataY(0);
  MantidVec& pE = peakWS->dataE(0);

  double xPeak = 0;
  double vPeak = 0;
  for (size_t i = 0; i < fitsize; i ++){
    double d = X[i+i_min];
    double bkgd = a0+a1*d+a2*d*d;
    tX[i] = d;
    tY[i] = bkgd;
    tE[i] = sqrt(fabs(bkgd));

    pX[i] = d;
    pY[i] = Y[i+i_min]-bkgd;
    pE[i] = sqrt(fabs(pY[i]));

    if (pY[i] > vPeak){
      vPeak = pY[i];
      xPeak = pX[i];
    }

  }

  /*
  this->setProperty("BackgroundWorkspace", bkgdWS);
  this->setProperty("TheorticBackgroundWorkspace", tbkgdWS);
  this->setProperty("PeakWorkspace", peakWS);
  */

  // g) Looping on peak width for the best fit
  double mincost = 1.0E10;
  double bestsigma = 0;
  double bestcenter = 0;
  double bestheight = 0;

  for (unsigned int iwidth = 2; iwidth < 10; iwidth ++){
    // a) Set up sub algorithm Fit
    IAlgorithm_sptr gfit;
    try
    {
      // Fitting the candidate peaks to a Gaussian
      gfit = createSubAlgorithm("Fit", -1, -1, true);
    } catch (Exception::NotFoundError &)
    {
      g_log.error("The FindPeaks algorithm requires the CurveFitting library");
      throw;
    }
    gfit->setProperty("InputWorkspace", peakWS);
    gfit->setProperty("WorkspaceIndex", 0);
    gfit->setProperty("MaxIterations", 50);

    // b) Guess sigma
    double in_sigma = (i4 + iwidth < X.size()) ? X[i4 + iwidth] - X[i4] : 0.;
    double in_centre = xPeak;
    double in_height = vPeak;

    // c) Construct Function string
    std::stringstream ss;
    ss << "name=Gaussian,Height=" << in_height << ",PeakCentre=" << in_centre << ",Sigma=" << in_sigma;
    std::string function = ss.str();

    // d) complete fit
    gfit->setProperty("StartX", (X[i4] - 5 * (X[i0] - X[i2])));
    gfit->setProperty("EndX",   (X[i4] + 5 * (X[i0] - X[i2])));
    gfit->setProperty("Minimizer", "Levenberg-Marquardt");
    gfit->setProperty("CostFunction", "Least squares");
    gfit->setProperty("Function", function);

    g_log.debug() << "Function: " << function << "  From " << (X[i4] - 5 * (X[i0] - X[i2])) << "  to " << (X[i4] + 5 * (X[i0] - X[i2])) << std::endl;

    // e) Fit and get result
    gfit->executeAsSubAlg();

    std::string fitStatus = gfit->getProperty("OutputStatus");
    std::vector<double> params = gfit->getProperty("Parameters");
    std::vector<std::string> paramnames = gfit->getProperty("ParameterNames");

    // Check order of names
    if (paramnames[0].compare("Height") != 0)
    {
      g_log.error() << "Parameter 0 should be f0.Height, but is " << paramnames[0] << std::endl;
      throw;
    }
    if (paramnames[1].compare("PeakCentre") != 0)
    {
      g_log.error() << "Parameter 1 should be f0.PeakCentre, but is " << paramnames[1] << std::endl;
      throw;
    }
    if (paramnames[2].compare("Sigma") != 0)
    {
      g_log.error() << "Parameter 2 should be f0.Sigma, but is " << paramnames[2] << std::endl;
      throw;
    }

    // f) get value
    double fheight = params[0];
    double fcenter = params[1];
    double fsigma  = params[2];
    double chi2 = gfit->getProperty("OutputChi2overDoF");
    std::string outputstatus = gfit->getProperty("OutputStatus");

    if (fheight <= 0 || fsigma <= 0){
      g_log.debug() << "Wrong Fit!!!" << std::endl;
    } else {
      if (chi2 < mincost){
        bestheight = fheight;
        bestcenter = fcenter;
        bestsigma = fsigma;
      }
    }
    g_log.debug() << "Status: " << outputstatus << " Cost = " << chi2 << "  Height, Center, Sigma = " << fheight << ", " << fcenter << ", " << fsigma << std::endl;

  } // ENDFOR

  // h) Fit again with everything altogether
  IAlgorithm_sptr lastfit;
  try
  {
    // Fitting the candidate peaks to a Gaussian
    lastfit = createSubAlgorithm("Fit", -1, -1, true);
  } catch (Exception::NotFoundError &)
  {
    g_log.error("The StripPeaks algorithm requires the CurveFitting library");
    throw;
  }
  lastfit->setProperty("InputWorkspace", input);
  lastfit->setProperty("WorkspaceIndex", spectrum);
  lastfit->setProperty("MaxIterations", 50);

  // c) Construct Function string
  std::stringstream ss2;
  if (backgroundtype.compare("Linear") == 0)
  {
    ss2 << "name=Gaussian,Height=" << bestheight << ",PeakCentre=" << bestcenter << ",Sigma="
        << bestsigma << ";name=LinearBackground,A0=" << a0 << ",A1=" << a1;
  }
  else if (backgroundtype.compare("Quadratic") == 0)
  {
    ss2 << "name=Gaussian,Height=" << bestheight << ",PeakCentre=" << bestcenter << ",Sigma="
        << bestsigma << ";name=QuadraticBackground,A0=" << a0 << ",A1=" << a1 << ",A2=" << a2;
  }
  else
  {
    g_log.error() << "Background type " << backgroundtype << " is not supported in FindPeak.cpp!"
        << std::endl;
    throw std::invalid_argument("Background type is not supported in FindPeak.cpp");
  }
  function = ss2.str();
  g_log.debug() << "Final Fit Function: " << function << std::endl;

  // d) complete fit
  lastfit->setProperty("StartX", (X[i4] - 2 * (X[i0] - X[i2])));
  lastfit->setProperty("EndX", (X[i4] + 2 * (X[i0] - X[i2])));
  lastfit->setProperty("Minimizer", "Levenberg-Marquardt");
  lastfit->setProperty("CostFunction", "Least squares");
  lastfit->setProperty("Function", function);

  // e) Fit and get result
  lastfit->executeAsSubAlg();

  std::string fitStatus2 = lastfit->getProperty("OutputStatus");
  params = lastfit->getProperty("Parameters");
  paramnames = lastfit->getProperty("ParameterNames");

  // Check order of names
  if (paramnames[0].compare("f0.Height") != 0)
  {
    g_log.error() << "Parameter 0 should be f0.Height, but is " << paramnames[0] << std::endl;
    throw std::invalid_argument("Parameters are out of order @ 0, should be f0.Height");
  }
  if (paramnames[1].compare("f0.PeakCentre") != 0)
  {
    g_log.error() << "Parameter 1 should be f0.PeakCentre, but is " << paramnames[1]
        << std::endl;
    throw std::invalid_argument("Parameters are out of order @ 0, should be f0.PeakCentre");
  }
  if (paramnames[2].compare("f0.Sigma") != 0)
  {
    g_log.error() << "Parameter 2 should be f0.Sigma, but is " << paramnames[2] << std::endl;
    throw std::invalid_argument("Parameters are out of order @ 0, should be f0.Sigma");
  }
  if (paramnames[3].compare("f1.A0") != 0)
  {
    g_log.error() << "Parameter 3 should be f1.A0, but is " << paramnames[3] << std::endl;
    throw std::invalid_argument("Parameters are out of order @ 0, should be f1.A0");
  }
  if (paramnames[4].compare("f1.A1") != 0)
  {
    g_log.error() << "Parameter 4 should be f1.A1, but is " << paramnames[4] << std::endl;
    throw std::invalid_argument("Parameters are out of order @ 0, should be f1.A1");
  }
  if (backgroundtype.compare("Quadratic") == 0 && paramnames[5].compare("f1.A2") != 0)
  {
    g_log.error() << "Parameter 5 should be f1.A2, but is " << paramnames[5] << std::endl;
    throw std::invalid_argument("Parameters are out of order @ 0, should be f1.A2");
  }

  double tempheight = params[0];
  if (!fitStatus2.compare("success") && tempheight > 0)
  {
    bestheight = tempheight;
    bestcenter = params[1];
    bestsigma  = params[2];
    a0 = params[3];
    a1 = params[4];
    if (backgroundtype.compare("Quadratic") == 0){
      a2 = params[5];
    }
  }

  // i) Set return value
  API::TableRow t = m_peaks->appendRow();
  t << spectrum << bestcenter << bestsigma << bestheight << a0 << a1 << a2;

  return;

}


} // namespace Algorithms
} // namespace Mantid
