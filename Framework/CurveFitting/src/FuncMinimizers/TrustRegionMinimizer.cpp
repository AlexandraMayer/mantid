//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/TrustRegionMinimizer.h"
#include "MantidCurveFitting/RalNlls/TrustRegion.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_version.h>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

using namespace NLLS;

TrustRegionMinimizer::TrustRegionMinimizer()
    : m_function(){
  //declareProperty("AbsError", m_absError, "Absolute error allowed for "
  //                                        "parameters - a stopping parameter "
  //                                        "in success.");
}

void TrustRegionMinimizer::initialize(
    API::ICostFunction_sptr costFunction, size_t) {
  m_leastSquares =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
          costFunction);
  if (!m_leastSquares) {
    throw std::runtime_error("Trust-region minimizer can only be used with Least "
                             "squares cost function.");
  }

  m_function = m_leastSquares->getFittingFunction();
  auto &values = *m_leastSquares->getValues();
  int n = static_cast<int>(m_leastSquares->nParams());
  int m = static_cast<int>(values.size());
  if (n > m) {
    throw std::runtime_error("More parameters than data.");
  }
  m_workspace.initialize(n, m, m_options, m_inform);
  m_x.allocate(n);
  m_leastSquares->getParameters(m_x);
  int j = 0;
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (m_function->isActive(i)) {
      m_J.m_index.push_back(j);
      j++;
    } else
      m_J.m_index.push_back(-1);
  }
}

/// Evaluate the fitting function and calculate the residuals.
/// @param x :: The fitting parameters as a fortran 1d array.
/// @param f :: The output fortran vector with the weighted residuals. 
void TrustRegionMinimizer::eval_F(const DoubleFortranVector &x, DoubleFortranVector &f) const {
  m_leastSquares->setParameters(x);
  auto &domain = *m_leastSquares->getDomain();
  auto &values = *m_leastSquares->getValues();
  m_function->function(domain, values);
  int m = static_cast<int>(values.size());
  if (f.len() != m) {
    f.allocate(m);
  }
  for(size_t i = 0; i < values.size(); ++i) {
    f.set(i, (values.getCalculated(i) - values.getFitData(i)) * values.getFitWeight(i));
  }
}

/// Evaluate the Jacobian
/// @param x :: The fitting parameters as a fortran 1d array.
/// @param J :: The output fortran matrix with the weighted Jacobian.
void TrustRegionMinimizer::eval_J(const DoubleFortranVector &x, DoubleFortranMatrix &J) const {
  m_leastSquares->setParameters(x);
  auto &domain = *m_leastSquares->getDomain();
  auto &values = *m_leastSquares->getValues();
  int n = static_cast<int>(m_leastSquares->nParams());
  int m = static_cast<int>(values.size());
  if (J.len1() != m || J.len2() != n) {
    J.allocate(m, n);
  }
  m_J.setJ(J.gsl());
  m_function->functionDeriv(domain, m_J);
  for(int i = 1; i <= m; ++i) {
    double w = values.getFitWeight(i - 1);
    for(int j = 1; j <= n; ++j) {
      J(i, j) *= w;
    }
  }
}

/// Evaluate the Hessian
/// @param x :: The fitting parameters as a fortran 1d array.
/// @param f :: The fortran vector with the weighted residuals. 
/// @param h :: The fortran matrix with the Hessian.
void TrustRegionMinimizer::eval_HF(const DoubleFortranVector &x, const DoubleFortranVector &f, DoubleFortranMatrix &h) const {
  int n = static_cast<int>(m_leastSquares->nParams());
  if (h.len1() != n || h.len2() != n) {
    h.allocate(n, n);
  }
  // Mantid fit functions don't calculate second derivatives.
  // For now the Hessian will not be used.
  h.zero();
}


bool TrustRegionMinimizer::iterate(size_t iter) {
  int max_tr_decrease = 100;
  double rho, normFnew, md, Jmax, JtJdiag;
  auto &w = m_workspace;
  auto &options = m_options;
  auto &inform = m_inform;
  auto &X = m_x;
  int n = m_x.len();
  int m = static_cast<int>(m_leastSquares->getValues()->size());

  std::cerr << "Iteration " << iter << std::endl;

  if (w.first_call == 0) {

    w.first_call = 1; // ?
    std::cerr << "First call" << std::endl;

    // evaluate the residual
    eval_F(X, w.f);
    inform.f_eval = inform.f_eval + 1;

    // and evaluate the jacobian
    eval_J(X, w.J);
    inform.g_eval = inform.g_eval + 1;

    if (options.relative_tr_radius == 1) {
      std::cerr << "Relative radius is 1" << std::endl;
      // first, let's get diag(J^TJ)
      Jmax = 0.0;
      for (int i = 1; i <= n; ++i) {
        // note:: assumes column-storage of J
        // JtJdiag = norm2( w.J( (i-1)*m + 1 : i*m ) );
        JtJdiag = 0.0;
        for (int j = 1; j <= m; ++j) { // for_do(j, 1, m)
          JtJdiag += pow(w.J(j, i), 2);
        }
        JtJdiag = sqrt(JtJdiag);
        if (JtJdiag > Jmax)
          Jmax = JtJdiag;
      }
      w.Delta = options.initial_radius_scale * (pow(Jmax, 2));
    } else {
      w.Delta = options.initial_radius;
    }

    if (options.calculate_svd_J) {
      std::cerr << "Calculate SVD" << std::endl;
      // calculate the svd of J (if needed)
      get_svd_J(w.J, w.smallest_sv(1), w.largest_sv(1));
    }

    w.normF = norm2(w.f);
    w.normF0 = w.normF;

    // g = -J^Tf
    mult_Jt(w.J, w.f, w.g);
    w.g *= -1.0;
    w.normJF = norm2(w.g);
    w.normJF0 = w.normJF;
    w.normJFold = w.normJF;

    // save some data
    inform.obj = 0.5 * (pow(w.normF, 2));
    inform.norm_g = w.normJF;
    inform.scaled_g = w.normJF / w.normF;

    // if we need to output vectors of the history of the residual
    // and gradient, the set the initial values
    if (options.output_progress_vectors) {
      w.resvec(1) = inform.obj;
      w.gradvec(1) = inform.norm_g;
    }

    // Select the order of the model to be used..
    switch (options.model) {
    case 1: // first-order
    {
      std::cerr << "First order" << std::endl;
      w.hf.zero();
      w.use_second_derivatives = false;
      break;
    }
    case 2: // second order
    {
      if (options.exact_second_derivatives) {
        std::cerr << "Exact second order derivatives" << std::endl;
        eval_HF(X, w.f, w.hf);
        inform.h_eval = inform.h_eval + 1;
      } else {
        // S_0 = 0 (see Dennis, Gay and Welsch)
        w.hf.zero();
        std::cerr << "non-Exact second order derivatives" << std::endl;
      }
      w.use_second_derivatives = true;
      break;
    }
    case 3: // hybrid (MNT)
    {
      std::cerr << "Hybrid" << std::endl;
      // set the tolerance :: make this relative
      w.hybrid_tol =
          options.hybrid_tol * (w.normJF / (0.5 * (pow(w.normF, 2))));
      // use first-order method initially
      w.hf.zero();
      w.use_second_derivatives = false;
      if (!options.exact_second_derivatives) {
        // initialize hf_temp too
        w.hf_temp.zero();
      }
      break;
    }
    default:
      throw std::logic_error("Unsupported model.");
    }
  }

  w.iter = w.iter + 1;
  inform.iter = w.iter;

  rho = -one; // intialize rho as a negative value
  bool success = false;
  int no_reductions = 0;

  while (!success) { // loop until successful
    no_reductions = no_reductions + 1;
    if (no_reductions > max_tr_decrease + 1) {
      inform.status = NLLS_ERROR::MAX_TR_REDUCTIONS;
      return true;
    }
    std::cerr << "w.Delta=" << w.Delta << std::endl;
    // Calculate the step d that the model thinks we should take next
    calculate_step(w.J, w.f, w.hf, w.g, n, m, w.Delta, w.d, w.normd, options,
                   inform, w.calculate_step_ws);

    std::cerr << "Corrections: " << w.d << std::endl;

    // Accept the step?
    w.Xnew = X;
    w.Xnew += w.d;
    eval_F(w.Xnew, w.fnew);
    inform.f_eval = inform.f_eval + 1;
    normFnew = norm2(w.fnew);

    // Get the value of the model
    //      md :=   m_k(d)       
    // evaluated at the new step 
    evaluate_model(w.f, w.J, w.hf, w.d, md, m, n, options, w.evaluate_model_ws);

    // Calculate the quantity                                  
    //   rho = 0.5||f||^2 - 0.5||fnew||^2 =   actual_reduction 
    //         --------------------------   -------------------
    //             m_k(0)  - m_k(d)         predicted_reduction
    //                                                         
    // if model is good, rho should be close to one
    calculate_rho(w.normF, normFnew, md, rho, options);
    if (rho > options.eta_successful) {
      success = true;
    }
    std::cerr << "rho: " << w.normF << ' ' << normFnew << ' ' << md << ' ' << rho << std::endl;

    // Update the TR radius
    update_trust_region_radius(rho, options, inform, w);

    if (!success) {
      // finally, check d makes progress
      if (norm2(w.d) < std::numeric_limits<double>::epsilon() * norm2(w.Xnew)) {
        inform.status = NLLS_ERROR::X_NO_PROGRESS;
        return false;
      }
    }
  }
  // if we reach here, a successful step has been found

  // update X and f
  X = w.Xnew;
  w.f = w.fnew;

  if (!options.exact_second_derivatives) {
    // first, let's save some old values...
    // g_old = -J_k^T r_k
    w.g_old = w.g;
    // g_mixed = -J_k^T r_{k+1}
    mult_Jt(w.J, w.fnew, w.g_mixed);
    w.g_mixed *= -1.0;
  }

  // evaluate J and hf at the new point
  eval_J(X, w.J);
  inform.g_eval = inform.g_eval + 1;

  if (options.calculate_svd_J) {
    get_svd_J(w.J, w.smallest_sv(w.iter + 1), w.largest_sv(w.iter + 1));
  }

  // g = -J^Tf
  mult_Jt(w.J, w.f, w.g);
  w.g *= -1.0;

  w.normJFold = w.normJF;
  w.normF = normFnew;
  w.normJF = norm2(w.g);

  // setup the vectors needed if second derivatives are not available
  if (!options.exact_second_derivatives) {
    w.y = w.g_old;
    w.y -= w.g;
    w.y_sharp = w.g_mixed;
    w.y_sharp -= w.g;
  }

  if (options.model == 3) {
    // hybrid method -- check if we need second derivatives

    if (w.use_second_derivatives) {
      if (w.normJF > w.normJFold) {
        // switch to Gauss-Newton
        w.use_second_derivatives = false;
        // save hf as hf_temp
        w.hf_temp = w.hf;
        w.hf.zero();
      }
    } else {
      auto FunctionValue = 0.5 * (pow(w.normF, 2));
      if (w.normJF / FunctionValue < w.hybrid_tol) {
        w.hybrid_count = w.hybrid_count + 1;
        if (w.hybrid_count == options.hybrid_switch_its) {
          // use (Quasi-)Newton
          w.use_second_derivatives = true;
          w.hybrid_count = 0;
          // copy hf from hf_temp
          if (!options.exact_second_derivatives) {
            w.hf = w.hf_temp;
          }
        }
      } else {
        w.hybrid_count = 0;
      }
    }

    if (!w.use_second_derivatives) {
      // call apply_second_order_info anyway, so that we update the
      // second order approximation
      if (!options.exact_second_derivatives) {
        rank_one_update(w.hf_temp, w, n);
      }
    }
  }

  if (w.use_second_derivatives) {
    //apply_second_order_info(n, m, X, w, eval_HF, params, options, inform,
    //                        weights);
    if (options.exact_second_derivatives) {
      eval_HF(X, w.f, w.hf);
      inform.h_eval = inform.h_eval + 1;
    } else {
      // use the rank-one approximation...
      rank_one_update(w.hf, w, n);
    }
  }

  // update the stats
  inform.obj = 0.5 * (pow(w.normF, 2));
  inform.norm_g = w.normJF;
  inform.scaled_g = w.normJF / w.normF;
  if (options.output_progress_vectors) {
    w.resvec(w.iter + 1) = inform.obj;
    w.gradvec(w.iter + 1) = inform.norm_g;
  }

  // Test convergence
  test_convergence(w.normF, w.normJF, w.normF0, w.normJF0, options, inform);

  if (inform.convergence_normf == 1 || inform.convergence_normg == 1) {
    return false;
  }

  inform.iter = w.iter;
  inform.resvec = w.resvec;
  inform.gradvec = w.gradvec;

  return true;
}

double TrustRegionMinimizer::costFunctionVal() {
  return 0.0;
}

/* Calculates covariance matrix
 *
 * @param epsrel :: Is used to remove linear-dependent columns
 * @param covar :: Returned covariance matrix, here as
 */
void TrustRegionMinimizer::calCovarianceMatrix(double epsrel,
                                                      gsl_matrix *covar) {
//#if GSL_MAJOR_VERSION < 2
//  gsl_multifit_covar(m_gslSolver->J, epsrel, covar);
//#else
//  gsl_matrix *J = gsl_matrix_alloc(gslContainer.n, gslContainer.p);
//  gsl_multifit_fdfsolver_jac(m_gslSolver, J);
//  gsl_multifit_covar(J, epsrel, covar);
//  gsl_matrix_free(J);
//#endif
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
