#ifndef MANTID_ALGORITHMS_CHECKWORKSPACESMATCH_H_
#define MANTID_ALGORITHMS_CHECKWORKSPACESMATCH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/** Compares two workspaces for equality.
 *
 * This algorithm is deprecated. Please use CompareWorkspaces instead.
 *
 * The data values (X,Y and error) are always checked. The algorithm can also
 * optionally check
 * the axes (this includes the units), the spectra-detector map, the instrument
 * (the name and
 * parameter map) and any bin masking.
 *
 * Required Properties:
 * <UL>
 * <LI> Workspace1 - The name of the first input workspace </LI>
 * <LI> Workspace2 - The name of the second input workspace </LI>
 * </UL>
 *
 * Optional Properties:
 * <UL>
 * <LI> Tolerance       - The maximum amount by which values may differ between
 * the workspaces (default: 0) </LI>
 * <LI> CheckAxes       - Whether to check that the axes match (default: true)
 * </LI>
 * <LI> CheckSpectraMap - Whether to check that the spectra-detector maps match
 * (default: true) </LI>
 * <LI> CheckInstrument - Whether to check that the instruments match (default:
 * true) </LI>
 * <LI> CheckMasking    - Whether to check that the bin masking matches
 * (default: true) </LI>
 * <LI> CheckSample     - Whether to check that the sample object mathces
 * (default: false) </LI>
 * </UL>
 *
 * Output Properties:
 * <UL>
 * <LI> Result - Contains 'success' if the workspaces match, the reason for the
 * failure otherwise </LI>
 * </UL>
 *
 * Copyright &copy; 2009-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
 * Ridge National Laboratory & European Spallation Source
 *
 * This file is part of Mantid.
 *
 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport CheckWorkspacesMatch : public API::Algorithm,
                                       public API::DeprecatedAlgorithm {
public:
  CheckWorkspacesMatch();
  virtual ~CheckWorkspacesMatch();

  /// Algorithm's name
  virtual const std::string name() const { return "CheckWorkspacesMatch"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Compares two workspaces for equality. This algorithm is mainly "
           "intended for use by the Mantid development team as part of the "
           "testing process.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }

  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility\\Workspaces"; }

  /// The string that is returned when comparison is successful.
  static std::string successString() { return "Success!"; }

private:
  /// Initialisation code
  void init();

  /// Called when comparing two individual workspaces
  void exec();

  /// Called when comparing workspace groups
  virtual bool processGroups();

  /// This algorithm is now deprecated and calls CompareWorkspaces instead
  std::string runCompareWorkspaces(bool group_compare = false);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CHECKWORKSPACESMATCH_H_*/