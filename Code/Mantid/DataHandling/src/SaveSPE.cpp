//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveSPE.h"
#include "MantidKernel/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <cstdio>
#include <cmath>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveSPE)

using namespace Kernel;
using namespace API;

///@cond
const char NUM_FORM[] = "%10.3E";
const char NUMS_FORM[] = "%10.3E%10.3E%10.3E%10.3E%10.3E%10.3E%10.3E%10.3E\n";
static const char Y_HEADER[] = "### S(Phi,w)\n";
static const char E_HEADER[] = "### Errors\n";
///@endcond

/// set to the number of numbers on each line (the length of lines is hard-coded in other parts of the code too)
static const int NUM_PER_LINE = 8;

const double SaveSPE::MASK_FLAG=-1e30;
const double SaveSPE::MASK_ERROR=0.0;

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveSPE::init()
{
  // Data must be in Energy Transfer and common bins
  API::CompositeValidator<> *wsValidator = new API::CompositeValidator<>;
  wsValidator->add(new API::WorkspaceUnitValidator<>("DeltaE"));
  wsValidator->add(new API::CommonBinsValidator<>);
  wsValidator->add(new API::HistogramValidator<>);
  // the SPE file specification states that values following ### S(Phi,w) are "intensities" (www.mantidproject.org/images/3/3d/Spe_file_format.pdf) so to be accuarate the dependence on bin boundary widths should be removed (i.e. numbers of counts should have been divided by a number that is proporational to the bin width)
  wsValidator->add(new API::RawCountValidator<>(false));
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Direction::Input,wsValidator),
    "The input workspace, which must be in Energy Transfer");
  declareProperty(new FileProperty("Filename","", FileProperty::Save),
    "The filename to use for the saved data");
}

/**
 * Execute the algorithm
 */
void SaveSPE::exec()
{
  using namespace Mantid::API;
  // Retrieve the input workspace
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  
  // Do the full check for common binning
  if ( ! WorkspaceHelpers::commonBoundaries(inputWS) )
  {
    g_log.error("The input workspace must have common binning");
    throw std::invalid_argument("The input workspace must have common binning");
  }
  
  const int nHist = inputWS->getNumberHistograms();
  m_nBins = inputWS->blocksize();

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  FILE * outSPE_File;
  outSPE_File = fopen(filename.c_str(),"w");
  if (!outSPE_File)
  {
    g_log.error("Failed to open file:" + filename);
    throw Kernel::Exception::FileError("Failed to open file:" , filename);
  }

  // Number of Workspaces and Number of Energy Bins
  fprintf(outSPE_File,"%8u%8u\n",nHist, m_nBins);

  // Write the angle grid (dummy if no 'vertical' axis)
  if ( inputWS->axes() > 1 && inputWS->getAxis(1)->unit() )
  {
    const Axis& axis = *(inputWS->getAxis(1));
    const std::string commentLine = "### " + axis.unit()->caption() + " Grid\n";
    fprintf(outSPE_File,commentLine.c_str());
    const int axisLength = axis.length();
    const int phiPoints = (axisLength==nHist) ? axisLength+1 : axisLength;
    for (int i = 0; i < phiPoints; i++)
    {
      const double value = (i < axisLength) ? axis(i) : axis(axisLength-1)+1;
      fprintf(outSPE_File,NUM_FORM,value);
      if ( (i > 0) && ((i + 1) % 8 == 0) )
      {
        fprintf(outSPE_File,"\n");
      }
    }
  }
  else
  {
    fprintf(outSPE_File,"### Phi Grid\n");
    const int phiPoints = nHist + 1; // Pretend this is binned
    for (int i = 0; i < phiPoints; i++)
    {
      const double value = i + 0.5;
      fprintf(outSPE_File,NUM_FORM,value);
      if ( (i > 0) && ((i + 1) % 8 == 0) )
      {
        fprintf(outSPE_File,"\n");
      }
    }
  }

  // If the number of angles isn't a factor of 8 then we need to add an extra CR/LF
  if (nHist % 8 != 0) fprintf(outSPE_File,"\n");

  // Get the Energy Axis (X) of the first spectra (they are all the same - checked above)
  const MantidVec& X = inputWS->readX(0);

  // Write the energy grid
  fprintf(outSPE_File,"### Energy Grid\n");
  const int energyPoints = m_nBins + 1; // Validator enforces binned data
  int i = 7;
  for (; i < energyPoints; i+=8)
  {// output a whole line of numbers at once
    fprintf(outSPE_File,NUMS_FORM,
            X[i-7],X[i-6],X[i-5],X[i-4],X[i-3],X[i-2],X[i-1],X[i]);
  }
  // if the last line is not a full line enter them individually
  if ( i != energyPoints-1 )
  {// the condition above means that the last line has fewer characters than the usual
    for (i-=7; i < energyPoints; ++i)
    {
      fprintf(outSPE_File,NUM_FORM,X[i]);
    }
    fprintf(outSPE_File,"\n");
  }

  writeHists(inputWS, outSPE_File);

  // Close the file
  fclose(outSPE_File);
}
/** Write the bin values and errors for all histograms to the file
*  @param WS the workspace to be saved
*  @param outFile the file object to write to
*/
void SaveSPE::writeHists(const API::MatrixWorkspace_const_sptr WS, FILE * const outFile)
{
  // We write out values NUM_PER_LINE at a time, so will need to do extra work if nBins isn't a factor of NUM_PER_LINE
  m_remainder = m_nBins % NUM_PER_LINE;
  const int nHist = WS->getNumberHistograms();
  // Create a progress reporting object
  Progress progress(this,0,1,100);
  const int progStep = static_cast<int>(ceil(nHist/100.0));

  // there are very often spectra that are missing detectors, as this can be a lot of detectors log it once at the end
  std::vector<int> spuriousSpectra;
  // Loop over the spectra, writing out Y and then E values for each
  for (int i = 0; i < nHist; i++)
  {
    try
    {//need to check if _all_ the detectors for the spectrum are masked, as we don't have output values for those
      if ( ! WS->getDetector(i)->isMasked() )
      {
        // there's no masking, write the data
        writeHist(WS, outFile, i);
      }
      else
      {//all the detectors are masked, write the masking value from the SPE spec http://www.mantidproject.org/images/3/3d/Spe_file_format.pdf
        writeMaskFlags(outFile);
      }
    }
    catch (Exception::NotFoundError &)
    {// WS->getDetector(i) throws this if the detector isn't in the instrument definition file, write mask values and prepare to log what happened
      spuriousSpectra.push_back(i);
      writeMaskFlags(outFile);
    }
    // make regular progress reports and check for cancelling the algorithm
    if ( i % progStep == 0 )
    {
      progress.report();
    }
  }
  logAnyMissing(spuriousSpectra);
}
/** Write the bin values and errors in a single histogram spectra to the file
*  @param WS the workspace to being saved
*  @param outFile the file object to write to
*  @param specIn the index number of the histgram to write
*/
void SaveSPE::writeHist(const API::MatrixWorkspace_const_sptr WS, FILE * const outFile, const int specIn) const
{
  fprintf(outFile, Y_HEADER);
  writeBins(WS->readY(specIn), outFile);

  fprintf(outFile, E_HEADER);
  writeBins(WS->readE(specIn), outFile);
}
/** Write the mask flags for in a histogram entry
*  @param outFile the file object to write to
*/
void SaveSPE::writeMaskFlags(FILE * const outFile) const
{
  fprintf(outFile, Y_HEADER);
  writeValue(MASK_FLAG, outFile);

  fprintf(outFile, E_HEADER);
  writeValue(MASK_ERROR, outFile);
}
/** Write the the values in the array to the file in the correct format
*  @param Vs the array of values to write (must have length given by m_nbins)
*  @param outFile the file object to write to
*/
void SaveSPE::writeBins(const MantidVec &Vs, FILE * const outFile) const
{
  for(int j = NUM_PER_LINE-1; j < m_nBins; j+=NUM_PER_LINE)
  {// output a whole line of numbers at once
    fprintf(outFile,NUMS_FORM,
            Vs[j-7],Vs[j-6],Vs[j-5],Vs[j-4],Vs[j-3],Vs[j-2],Vs[j-1],Vs[j]);
  }
  if (m_remainder)
  {
    for ( int l = m_nBins - m_remainder; l < m_nBins; ++l)
    {
      fprintf(outFile,NUM_FORM,Vs[l]);
    }
    fprintf(outFile,"\n");
  }
}
/** Write the the value the file a number of times given by m_nbins
*  @param value the value that will be writen continuely
*  @param outFile the file object to write to
*/
void SaveSPE::writeValue(const double value, FILE * const outFile) const
{
  for(int j = NUM_PER_LINE-1; j < m_nBins; j+=NUM_PER_LINE)
  {// output a whole line of numbers at once
    fprintf(outFile,NUMS_FORM,
            value,value,value,value,value,value,value,value);
  }
  if (m_remainder)
  {
    for ( int l = m_nBins - m_remainder; l < m_nBins; ++l)
    {
      fprintf(outFile,NUM_FORM,value);
    }
    fprintf(outFile,"\n");
  }
}

void SaveSPE::logAnyMissing(std::vector<int> &inds) const
{
  std::vector<int>::const_iterator index = inds.begin(), end = inds.end();
  if (index != end)
  {
    g_log.information() << "Found " << inds.size() << " spectra without associated detectors, probably the detectors are not present in the instrument definition, this is not unusual. The Y values for those spectra have been set to zero" << std::endl;
    g_log.information() << "The spectrum indices that were set to zero are: " << *index;
    for ( ; index != end; ++index )
    {
      g_log.information() << ", " << *index;
    }
    g_log.information() << std::endl;
  }
}

}
}
