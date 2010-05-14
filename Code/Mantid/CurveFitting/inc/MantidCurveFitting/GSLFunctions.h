#ifndef MANTID_CURVEFITTING_GSLFUNCTIONS_H_
#define MANTID_CURVEFITTING_GSLFUNCTIONS_H_

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_blas.h>
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {
    // forward declaration
    class Fit;
    class ICostFunction;

    /**
    Various GSL specific functions used GSL specific minimizers

    @author Anders Markvardsen, ISIS, RAL
    @date 14/05/2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

  /// The implementation of Jacobian
    class JacobianImpl1: public API::Jacobian
  {
  public:
    /// The pointer to the GSL's internal jacobian matrix
    gsl_matrix * m_J;
    /// Maps declared indeces to active. For fixed (tied) parameters holds -1
    std::vector<int> m_index;

    /// Set the pointer to the GSL's jacobian
    void setJ(gsl_matrix * J){m_J = J;}

    /// overwrite base method
    ///  @throw runtime_error Thrown if column of Jacobian to add number to does not exist
    void addNumberToColumn(const double& value, const int& iActiveP) 
    {
      if (iActiveP < static_cast<int>(m_J->size2) )
      {
        // add penalty to first and last point and every 10th point in between
        m_J->data[iActiveP] += value;
        m_J->data[(m_J->size1-1)*m_J->size2 + iActiveP] += value;
        for (size_t iY = 9; iY < m_J->size1-1; iY++) 
          m_J->data[iY*m_J->size2 + iActiveP] += value;
      }
      else
      {
        throw std::runtime_error("Try to add number to column of Jacobian matrix which does not exist.");
      }   
    }
    /// overwrite base method
    void set(int iY, int iP, double value)
    {
      int j = m_index[iP];
      if (j >= 0) gsl_matrix_set(m_J,iY,j,value);
    }
  };

  /// Structure to contain least squares data and used by GSL
  struct FitData1 {
    /// Constructor
    FitData1(Fit* f);
    /// number of points to be fitted (size of X, Y and sqrtWeightData arrays)
    size_t n;
    /// number of (active) fit parameters
    size_t p;
    /// the data to be fitted (abscissae)
    double * X;
    /// the data to be fitted (ordinates)
    const double * Y;
    /// the standard deviations of the Y data points
    double * sqrtWeightData;
    /// pointer to instance of Fit
    Fit* fit;
    /// pointer to the cost function
    ICostFunction* costFunc;
    /// Jacobi matrix interface
    JacobianImpl1 J;
    /// To use the none least-squares gsl algorithms within the gsl least-squared framework
    /// include here container for calculuated data and calculated jacobian. 
    double * holdCalculatedData;
    gsl_matrix * holdCalculatedJacobian;
  };

  int gsl_f(const gsl_vector * x, void *params, gsl_vector * f);
  int gsl_df(const gsl_vector * x, void *params, gsl_matrix * J);
  int gsl_fdf(const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
  double gsl_costFunction(const gsl_vector * x, void *params);
  void gsl_costFunction_df(const gsl_vector * x, void *params, gsl_vector *df);
  void gsl_costFunction_fdf(const gsl_vector * x, void *params, double *f, gsl_vector *df);


  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLFUNCTIONS_H_*/
