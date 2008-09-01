#ifndef MANTID_ALGORITHMS_UNWRAP_H_
#define MANTID_ALGORITHMS_UNWRAP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes an input Workspace2D that contains 'raw' data, unwraps the data according to
    the reference flightpath provided and converts the units to wavelength.
    The output workspace will have common bins in the maximum theoretical wavelength range.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> LRef            - The 'reference' flightpath (in metres). </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 25/07/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport Unwrap : public API::Algorithm
{
public:
  Unwrap();
  virtual ~Unwrap();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "Unwrap"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Units";}

private:
  void init();
  void exec();

  void checkInputWorkspace() const;
  const double getPrimaryFlightpath() const;
  const double calculateFlightpath(const int& spectrum, const double& L1, bool& isMonitor) const;
  const std::vector<int> unwrapX(const API::Workspace_sptr& tempWS, const int& spectrum, const double& Ld);
  std::pair<int,int> handleFrameOverlapped(const std::vector<double>& xdata, const double& Ld, std::vector<double>& tempX);
  void unwrapYandE(const API::Workspace_sptr& tempWS, const int& spectrum, const std::vector<int>& rangeBounds);
  API::Workspace_sptr rebin(const API::Workspace_sptr& workspace, const double& min, const double& max, const int& numBins);

  double m_conversionConstant; ///< The constant used in the conversion from TOF to wavelength
  API::Workspace_const_sptr m_inputWS; ///< Pointer to the input workspace
  double m_LRef; ///< The 'reference' flightpath
  double m_Tmin; ///< The start of the time-of-flight frame
  double m_Tmax; ///< The end of the time-of-flight frame
  unsigned int m_XSize; ///< The size of the X vectors in the input workspace

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_UNWRAP_H_ */
