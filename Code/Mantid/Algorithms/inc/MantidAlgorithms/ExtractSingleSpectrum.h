#ifndef MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_
#define MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Copies a single spectrum from a 2D Workspace into a new workspace.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    <LI> SpectrumIndex - The workspace index number of the spectrum to extract. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/06/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport ExtractSingleSpectrum : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ExtractSingleSpectrum() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ExtractSingleSpectrum() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ExtractSingleSpectrum"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_*/
