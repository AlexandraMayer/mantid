//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SmoothData.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SmoothData)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void SmoothData::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "Name of the input workspace");
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm" );

  BoundedValidator<int> *min = new BoundedValidator<int>();
  min->setLower(3);
  // The number of points to use in the smoothing.
  declareProperty("NPoints", 3, min,
    "The number of points to average over (minimum 3). If an even number is\n"
    "given, it will be incremented by 1 to make it odd (default value 3)" );
}

void SmoothData::exec()
{
  // Get the input properties
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  int npts = getProperty("NPoints");
  // Number of smoothing points must always be an odd number, so add 1 if it isn't.
  if (!(npts%2))
  {
    g_log.information("Adding 1 to number of smoothing points, since it must always be odd");
    ++npts;
  }

  // Check that the number of points in the smoothing isn't larger than the spectrum length
  const int vecSize = inputWorkspace->blocksize();
  if ( npts >= vecSize )
  {
    g_log.error("The number of averaging points requested is larger than the spectrum length");
    throw std::out_of_range("The number of averaging points requested is larger than the spectrum length");
  }
  const int halfWidth = (npts-1)/2;

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);
  Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(outputWorkspace);

  // Next lines enable sharing of the X vector to be carried over to the output workspace if in the input one
  const MantidVec * const XFirst = &(inputWorkspace->readX(0));
  Histogram1D::RCtype newX;
  newX.access() = inputWorkspace->readX(0);

  Progress progress(this,0.0,1.0,inputWorkspace->getNumberHistograms());
  PARALLEL_FOR2(inputWorkspace,outputWorkspace)
  // Loop over all the spectra in the workspace
  for (int i = 0; i < inputWorkspace->getNumberHistograms(); ++i)
  {
		PARALLEL_START_INTERUPT_REGION
    // Copy the X data over. Preserve data sharing if present in input workspace.
    const MantidVec &X = inputWorkspace->readX(i);
    if (output2D && XFirst==&X)
    {
      output2D->setX(i,newX);
    }
    else
    {
      outputWorkspace->dataX(i) = X;
    }

    // Now get references to the Y & E vectors in the input and output workspaces
    const MantidVec &Y = inputWorkspace->readY(i);
    const MantidVec &E = inputWorkspace->readE(i);
    MantidVec &newY = outputWorkspace->dataY(i);
    MantidVec &newE = outputWorkspace->dataE(i);

    // Use total to help hold our moving average
    double total = 0.0, totalE = 0.0;
    // First push the values ahead of the current point onto total
    for (int i = 0; i < halfWidth; ++i)
    {
      total += Y[i];
      totalE += E[i]*E[i];
    }
    // Now calculate the smoothed values for the 'end' points, where the number contributing
    // to the smoothing will be less than NPoints
    for (int j = 0; j <= halfWidth; ++j)
    {
      const int index = j+halfWidth;
      total += Y[index];
      newY[j] = total/(index+1);
      totalE += E[index]*E[index];
      newE[j] = sqrt(totalE)/(index+1);
    }
    // This is the main part, where each data point is the average of NPoints points centred on the
    // current point. Note that the statistical error will be reduced by sqrt(npts) because more
    // data is now contributing to each point.
    for (int k = halfWidth+1; k < vecSize-halfWidth; ++k)
    {
      const int kp = k+halfWidth;
      const int km = k-halfWidth-1;
      total += Y[kp] - Y[km];
      newY[k] = total/npts;
      totalE += E[kp]*E[kp] - E[km]*E[km];
      // Use of a moving average can lead to rounding error where what should be 
      // zero actually comes out as a tiny negative number - bad news for sqrt so protect
      newE[k] = std::sqrt(std::abs(totalE))/npts;
    }
    // This deals with the 'end' at the tail of each spectrum
    for (int l = vecSize-halfWidth; l < vecSize; ++l)
    {
      const int index = l-halfWidth;
      total -= Y[index-1];
      newY[l] = total/(vecSize-index);
      totalE -= E[index-1]*E[index-1];
      newE[l] = std::sqrt(std::abs(totalE))/(vecSize-index);
    }
		progress.report();
		PARALLEL_END_INTERUPT_REGION
  } // Loop over spectra
	PARALLEL_CHECK_INTERUPT_REGION

  // Set the output workspace to its property
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
