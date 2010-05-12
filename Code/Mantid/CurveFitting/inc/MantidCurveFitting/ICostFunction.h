#ifndef MANTID_CURVEFITTING_ICOSTFUNCTION_H_
#define MANTID_CURVEFITTING_ICOSTFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"

namespace Mantid
{
namespace CurveFitting
{
/** An interface for specifying the cost function to be used with Fit,
    for example, the default being least squares fitting

    @author Anders Markvardsen, ISIS, RAL
    @date 11/05/2010

    Copyright &copy; 2010 STFC Rutherford Appleton Laboratory

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
class DLLExport ICostFunction 
{
public:
  /// Virtual destructor
  virtual ~ICostFunction() {}

  /// Get name of minimizer
  virtual std::string name() const = 0;

  /// Calculate value of cost function from observed
  /// and calculated values
  virtual double val(const double* yData, const double* inverseError, double* yCal, const int& n) = 0;

  /// Calculate the derivatives of the cost function
  virtual void deriv(const double* yData, const double* inverseError, const double* yCal, 
                     const double* jacobian, double* outDerivs, const int& p, const int& n) = 0;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ICOSTFUNCTION_H_*/
