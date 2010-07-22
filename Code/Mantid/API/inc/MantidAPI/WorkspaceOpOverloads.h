#ifndef MANTID_API_WORKOPOVERLOADS_H_
#define MANTID_API_WORKOPOVERLOADS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace API
{

// Workspace operator overloads
MatrixWorkspace_sptr DLLExport operator+(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator-(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator*(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator/(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);

MatrixWorkspace_sptr DLLExport operator+(const MatrixWorkspace_sptr lhs, const double& rhsValue);
MatrixWorkspace_sptr DLLExport operator-(const MatrixWorkspace_sptr lhs, const double& rhsValue);
MatrixWorkspace_sptr DLLExport operator-(const double& lhsValue, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator*(const MatrixWorkspace_sptr lhs, const double& rhsValue);
MatrixWorkspace_sptr DLLExport operator*(const double& lhsValue, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator/(const MatrixWorkspace_sptr lhs, const double& rhsValue);
MatrixWorkspace_sptr DLLExport operator/(const double& lhsValue, const MatrixWorkspace_sptr rhs);

MatrixWorkspace_sptr DLLExport operator+=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator-=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator*=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr DLLExport operator/=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);

MatrixWorkspace_sptr DLLExport operator+=(const MatrixWorkspace_sptr lhs, const double& rhsValue);
MatrixWorkspace_sptr DLLExport operator-=(const MatrixWorkspace_sptr lhs, const double& rhsValue);
MatrixWorkspace_sptr DLLExport operator*=(const MatrixWorkspace_sptr lhs, const double& rhsValue);
MatrixWorkspace_sptr DLLExport operator/=(const MatrixWorkspace_sptr lhs, const double& rhsValue);

/** A collection of static functions for use with workspaces

    @author Russell Taylor, Tessella Support Services plc
    @date 19/09/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
struct DLLExport WorkspaceHelpers
{
  // Checks whether a workspace has common X bins/values
  static bool commonBoundaries(const MatrixWorkspace_const_sptr WS);
  // Checks whether the binning is the same in two histograms
  static bool matchingBins(const MatrixWorkspace_const_sptr ws1,
                           const MatrixWorkspace_const_sptr ws2, const bool firstOnly = false);
  // Checks whether a the X vectors in a workspace are actually the same vector
  static bool sharedXData(const MatrixWorkspace_const_sptr WS);
  // Divides the data in a workspace by the bin width to make it a distribution (or the reverse)
  static void makeDistribution(MatrixWorkspace_sptr workspace, const bool forwards = true);
  // Convert a list of spectrum numbers to the corresponding workspace indices
  static void getIndicesFromSpectra(const MatrixWorkspace_const_sptr WS, const std::vector<int>& spectraList,
                                    std::vector<int>& indexList);
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKOPOVERLOADS_H_ */
