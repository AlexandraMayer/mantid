//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MaskBins.h"
#include <limits>
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskBins)

using namespace Kernel;
using namespace API;
using namespace Mantid;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::EventWorkspace_const_sptr;

MaskBins::MaskBins() : API::Algorithm(), m_startX(0.0), m_endX(0.0) {}

void MaskBins::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  
  // This validator effectively makes these properties mandatory
  // Would be nice to have an explicit validator for this, but MandatoryValidator is already taken!
  BoundedValidator<double> *required = new BoundedValidator<double>();
  required->setUpper(std::numeric_limits<double>::max()*0.99);
  declareProperty("XMin",std::numeric_limits<double>::max(),required);
  declareProperty("XMax",std::numeric_limits<double>::max(),required->clone());
}

/** Execution code.
 *  @throw std::invalid_argument If XMax is less than XMin
 */
void MaskBins::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Check for valid X limits
  m_startX = getProperty("XMin");
  m_endX = getProperty("XMax");

  if (m_startX > m_endX)
  {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    throw std::invalid_argument(failure);
  }
  
  //---------------------------------------------------------------------------------
  //Now, determine if the input workspace is actually an EventWorkspace
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  if (eventW != NULL)
  {
    //------- EventWorkspace ---------------------------
    this->execEvent();
  }
  else
  {
    //------- MatrixWorkspace of another kind -------------
    MantidVec::difference_type startBin(0),endBin(0);

    // If the binning is the same throughout, we only need to find the index limits once
    const bool commonBins = WorkspaceHelpers::commonBoundaries(inputWS);
    if (commonBins)
    {
      const MantidVec& X = inputWS->readX(0);
      this->findIndices(X,startBin,endBin);
    }

    // Only create the output workspace if it's different to the input one
    MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
    if ( outputWS != inputWS )
    {
      outputWS = WorkspaceFactory::Instance().create(inputWS);
      setProperty("OutputWorkspace",outputWS);
    }
    
    const int numHists = inputWS->getNumberHistograms();
    Progress progress(this,0.0,1.0,numHists);
    //Parallel running has problems with a race condition, leading to occaisional test failures and crashes

    for (int i = 0; i < numHists; ++i)
    {
      // Copy over the data
      outputWS->dataX(i) = inputWS->readX(i);
      const MantidVec& X = outputWS->dataX(i);
      outputWS->dataY(i) = inputWS->readY(i);
      outputWS->dataE(i) = inputWS->readE(i);

      MantidVec::difference_type startBinLoop(startBin),endBinLoop(endBin);
      if (!commonBins) this->findIndices(X,startBinLoop,endBinLoop);

      // Loop over masking each bin in the range
      for (int j = startBinLoop; j < endBinLoop; ++j)
      {
        outputWS->maskBin(i,j);
      }
      progress.report();
    }

  }
 
}

/** Execution code for EventWorkspaces
 */
void MaskBins::execEvent()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputMatrixWS = getProperty("OutputWorkspace");
  if ( outputMatrixWS != inputWS )
  {
    outputMatrixWS = WorkspaceFactory::Instance().create(eventW);
    setProperty("OutputWorkspace",outputMatrixWS);
  }

  //This is the event output workspace
  EventWorkspace_sptr outputWS = boost::dynamic_pointer_cast<EventWorkspace>(outputMatrixWS);

//  std::cout << outputWS->getNumberHistograms() << " histograms.\n";
//  std::cout << outputWS->getEventListAtWorkspaceIndex(0).getNumberEvents() << " events in spectrum 0.\n";

  //Go through all histograms
  const int numHists = inputWS->getNumberHistograms();
  Progress progress(this,0.0,1.0,numHists);

  for (int i = 0; i < numHists; ++i)
  {
    outputWS->getEventListAtWorkspaceIndex(i).maskTof(m_startX, m_endX);
    progress.report();
  }

  //Clear the MRU
  outputWS->clearMRU();


}


/** Finds the indices of the bins at the limits of the range given. 
 *  @param X        The X vector to search
 *  @param startBin Returns the bin index including the starting value
 *  @param endBin   Returns the bin index after the end value
 */
void MaskBins::findIndices(const MantidVec& X, MantidVec::difference_type& startBin, MantidVec::difference_type& endBin)
{
  startBin = std::upper_bound(X.begin(),X.end(),m_startX) - X.begin();
  if (startBin!=0) --startBin;
  MantidVec::const_iterator last = std::lower_bound(X.begin(),X.end(),m_endX);
  if (last==X.end()) --last;
  endBin = last - X.begin();
}

} // namespace Algorithms
} // namespace Mantid

