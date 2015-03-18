#ifndef MANTID_CURVEFITTING_PAWLEYFIT_H_
#define MANTID_CURVEFITTING_PAWLEYFIT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/PawleyFunction.h"

namespace Mantid {
namespace CurveFitting {

/** PawleyFit

  This algorithm uses the Pawley-method to refine lattice parameters using a
  powder diffractogram and a list of unique Miller indices. From the initial
  lattice parameters, theoretical reflection positions are calculated. Each
  reflection is described by the peak profile function supplied by the user and
  all parameters except the one for location of the reflection are freely
  refined. Available lattice parameters depend on the selected crystal system.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/03/2015

  Copyright © 2015 PSI-NXMM

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport PawleyFit : public API::Algorithm {
public:
  virtual ~PawleyFit() {}

  const std::string name() const { return "PawleyFit"; }
  int version() const { return 1; }
  const std::string summary() const;

protected:
  std::vector<Kernel::V3D> hklsFromString(const std::string &hklString) const;
  void addHKLsToFunction(PawleyFunction_sptr &pawleyFn,
                         const API::ITableWorkspace_sptr &tableWs) const;

  Kernel::V3D getHkl(const std::string &hklString) const;

  void init();
  void exec();
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PAWLEYFIT_H_ */
