#ifndef MANTID_CURVEFITTING_CRYSTALFIELDSUSCEPTIBILITY_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDSUSCEPTIBILITY_H_

#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidAPI/IFunction1D.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldSusceptibility is a function that calculates the molar magnetic
  susceptibility (in cm^3/mol or m^3/mol) due to the crystalline electric field.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

class MANTID_CURVEFITTING_DLL CrystalFieldSusceptibility
    : public CrystalFieldPeaksBase,
      public API::IFunction1D {
public:
  CrystalFieldSusceptibility();
  std::string name() const override { return "CrystalFieldSusceptibility"; }
  const std::string category() const override { return "General"; }
  void setEigensystem(const DoubleFortranVector &en,
                      const ComplexFortranMatrix &wf, const int nre);
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

private:
  DoubleFortranVector m_en;
  ComplexFortranMatrix m_wf;
  int m_nre;
  bool m_setDirect;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDSUSCEPTIBILITY_H_ */
