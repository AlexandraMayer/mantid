#ifndef MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_
#define MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_
/*WIKI* 


Calculates the probability of a neutron being transmitted through the sample using detected counts from two monitors, one in front and one behind the sample. A data workspace can be corrected for transmission by [[Divide|dividing]] by the output of this algorithm.

Because the detection efficiency of the monitors can be different the transmission calculation is done using two runs, one run with the sample (represented by <math>S</math> below) and a direct run without it(<math>D</math>). The fraction transmitted through the sample <math>f</math> is calculated from this formula:
<br>
<br>
<math> p = \frac{S_T}{D_T}\frac{D_I}{S_I} </math>
<br>
<br>
where <math>S_I</math> is the number of counts from the monitor in front of the sample (the incident beam monitor), <math>S_T</math> is the transmission monitor after the sample, etc.

The resulting fraction as a function of wavelength is created as the OutputUnfittedData workspace. However, because of statistical variations it is recommended to use the OutputWorkspace, which is the evaluation of a fit to those transmission fractions. The unfitted data is not affected by the RebinParams or Fitmethod properties but these can be used to refine the fitted data. The RebinParams method is useful when the range of wavelengths passed to CalculateTransmission is different from that of the data to be corrected.

=== Subalgorithms used ===

Uses the algorithm [[linear]] to fit to the calculated transmission fraction.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates the transmission correction, as a function of wavelength, for a SANS
    instrument. Currently makes the assumption that the incident beam monitor's
    UDET is 2, while that of the transmission monitor is 3 (as for LOQ). 
   
    Required Properties:
    <UL>
    <LI> SampleRunWorkspace  - The workspace containing the sample transmission run. </LI>
    <LI> DirectRunWorkspace  - The workspace containing the direct beam transmission run. </LI>
    <LI> OutputWorkspace     - The fitted transmission correction. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> IncidentBeamMonitor - The UDET of the incident beam monitor (Default: 2, as for LOQ). </LI>
    <LI> TransmissionMonitor - The UDET of the transmission monitor (Default: 3, as for LOQ). </LI>
    <LI> MinWavelength       - The minimum wavelength for the fit (Default: 2.2 Angstroms). </LI>
    <LI> MaxWavelength       - The maximum wavelength for the fit (Default: 10 Angstroms). </LI>
    <LI> FitMethod           - Whether to fit to the log of the transmission curve (the default) or directly (i.e. linearly). </LI>
    <LI> OutputUnfittedData  - If true (false is the default), will output an additional workspace
                               called [OutputWorkspace]_unfitted containing the unfitted transmission
                               correction. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 22/01/2009

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CalculateTransmission : public API::Algorithm
{
public:
  /// Constructor
  CalculateTransmission();
  /// Virtual destructor
  virtual ~CalculateTransmission();
  /// Algorithm's name
  virtual const std::string name() const { return "CalculateTransmission"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// stores an estimate of the progress so far as a proportion (starts at zero goes to 1.0)
  mutable double m_done;
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// Pull out a single spectrum from a 2D workspace
  API::MatrixWorkspace_sptr extractSpectrum(API::MatrixWorkspace_sptr WS, const int64_t index);
  /// Returns a workspace with the evaulation of the fit to the calculated transmission fraction
  API::MatrixWorkspace_sptr fit(API::MatrixWorkspace_sptr raw, std::vector<double> rebinParams, const std::string fitMethod);
  /// Call the Linear fitting algorithm as a child algorithm
  API::MatrixWorkspace_sptr fitData(API::MatrixWorkspace_sptr WS, double & grad, double & offset);
  /// Calls the rebin algorithm
  API::MatrixWorkspace_sptr rebin(std::vector<double> & binParams, API::MatrixWorkspace_sptr output);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CALCULATETRANSMISSION_H_*/
