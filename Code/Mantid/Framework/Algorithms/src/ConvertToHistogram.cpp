/*WIKI* 

The input workspace must contain point data. 

Once executed, the OutputWorkspace will contain histogram data where the bin width is guessed from the spacing between the input X points.


*WIKI*/
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToHistogram.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
  namespace Algorithms
  {

    DECLARE_ALGORITHM(ConvertToHistogram);
    
    /// Sets documentation strings for this algorithm
    void ConvertToHistogram::initDocs()
    {
      this->setWikiSummary("Converts a workspace containing point data into one containing histograms. ");
      this->setOptionalMessage("Converts a workspace containing point data into one containing histograms.");
    }
    

    using API::MatrixWorkspace_sptr;
    using Mantid::MantidVec;
    using std::size_t;

    //------------------------------------------------------------------------------
    // Private member functions
    //------------------------------------------------------------------------------
    
    /**
     * Returns true if the algorithm needs to be run. 
     * @param inputWS pointer to input workspace
     * @returns True if the input workspace needs to be run through this algorithm
     */
    bool ConvertToHistogram::isProcessingRequired(const MatrixWorkspace_sptr inputWS) const
    {
      if( inputWS->isHistogramData() )
      {
        g_log.information() << "Input workspace already contains histogram data. "
          << "OutputWorkspace set to InputWorkspace value.\n";
        return false;
      }
      return true;
    }
    
    /** 
     * Checks the input workspace's X data structure is logical.
     * @param inputWS pointer to input workspace
     * @returns True if the X structure of the given input is what we expect, i.e. NX=NY+1
     */
    bool ConvertToHistogram::isWorkspaceLogical(const MatrixWorkspace_sptr inputWS) const
    {
      const size_t numYPoints = inputWS->blocksize();
      //Workspace guarantees that each X-vector is the same size
      const size_t numXPoints = inputWS->readX(0).size();
      if( numYPoints != numXPoints )
      {
        g_log.error() << "The number of Y data points must equal the number of X data points on the InputWorkspace. "
          << "Found NY=" << numYPoints << " and NX=" << numXPoints << "\n";
        return false;
      }
      return true;
    }

    /**
     * Returns the size of the new X vector
     * @param inputWS pointer to input workspace
     * @returns An integer giving the size of the new X vector
     */
    size_t ConvertToHistogram::getNewXSize(const MatrixWorkspace_sptr inputWS) const
    {
      return (inputWS->blocksize() + 1);
    }

    /**
     * Calculate the histogram boundaries. For uniform bins this should work correctly
     * and should be convertable back to point data. For non-uniform bins the boundaries 
     * are guessed such that the boundary goes mid-way between each point
     * @param inputX :: A const reference to the input data
     * @param outputX :: A reference to the output data
     */
    void ConvertToHistogram::calculateXPoints(const MantidVec & inputX, MantidVec &outputX) const
    {
      const size_t numPoints = inputX.size();
      const size_t numBoundaries = numPoints + 1;
      assert(outputX.size() == numBoundaries);
      // Handle the front and back points outside
      for( size_t i = 0; i < numPoints - 1; ++i )
      {
        outputX[i+1] = 0.5*(inputX[i+1] + inputX[i]);
      }
      // Now deal with the end points
      outputX[0] = inputX.front() - (outputX[1] - inputX.front());
      outputX[numPoints] = inputX.back() + (inputX.back() - outputX[numBoundaries - 2]);
    }

  }
}
