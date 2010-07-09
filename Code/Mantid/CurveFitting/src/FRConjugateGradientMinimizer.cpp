//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FRConjugateGradientMinimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{
///@cond nodoc
DECLARE_FUNCMINIMIZER(FRConjugateGradientMinimizer,Conjugate gradient (Fletcher-Reeves imp.))
///@endcond

// Get a reference to the logger
Kernel::Logger& FRConjugateGradientMinimizer::g_log = Kernel::Logger::get("FRConjugateGradientMinimizer");

void FRConjugateGradientMinimizer::initialize(double* X, const double* Y, 
                                              double *sqrtWeight, const int& nData, 
                                              const int& nParam, gsl_vector* startGuess, Fit* fit, 
                                              const std::string& costFunction) 
{
  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(fit);  //,X, Y, sqrtWeight, nData, nParam);
  m_data->p = nParam;
  m_data->n = nData; 
  m_data->X = X;
  m_data->Y = Y;
  m_data->sqrtWeightData = sqrtWeight;
  m_data->holdCalculatedData = new double[nData];
  m_data->holdCalculatedJacobian =  gsl_matrix_alloc (nData, nParam);
  m_data->costFunc = CostFunctionFactory::Instance().createUnwrapped(costFunction);

  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_conjugate_fr;

  // setup GSL container
  m_gslMultiminContainer.n = nParam;  
  m_gslMultiminContainer.f = &gsl_costFunction;
  m_gslMultiminContainer.df = &gsl_costFunction_df;
  m_gslMultiminContainer.fdf = &gsl_costFunction_fdf;
  m_gslMultiminContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, nParam);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &m_gslMultiminContainer, startGuess, 0.01, 1e-4);

  // for covariance matrix
  m_gslLeastSquaresContainer.f = &gsl_f;
  m_gslLeastSquaresContainer.df = &gsl_df;
  m_gslLeastSquaresContainer.fdf = &gsl_fdf;
  m_gslLeastSquaresContainer.n = nData;
  m_gslLeastSquaresContainer.p = nParam;
  m_gslLeastSquaresContainer.params = m_data;
}

FRConjugateGradientMinimizer::~FRConjugateGradientMinimizer()
{
  delete [] m_data->holdCalculatedData;
  delete m_data->costFunc;
  gsl_matrix_free (m_data->holdCalculatedJacobian);
  delete m_data;

  gsl_multimin_fdfminimizer_free(m_gslSolver);
}

std::string FRConjugateGradientMinimizer::name()const
{
  return m_name;
}

int FRConjugateGradientMinimizer::iterate() 
{
  return gsl_multimin_fdfminimizer_iterate(m_gslSolver);
}

int FRConjugateGradientMinimizer::hasConverged()
{
  return gsl_multimin_test_gradient(m_gslSolver->gradient, 1e-3);
}

double FRConjugateGradientMinimizer::costFunctionVal()
{
  return m_gslSolver->f;
}

void FRConjugateGradientMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  gsl_matrix * holdCalculatedJacobian;
  holdCalculatedJacobian =  gsl_matrix_alloc (m_gslLeastSquaresContainer.n, m_gslLeastSquaresContainer.p);

  int dummy = m_gslLeastSquaresContainer.df(m_gslSolver->x, m_gslLeastSquaresContainer.params, holdCalculatedJacobian);
  gsl_multifit_covar (holdCalculatedJacobian, epsrel, covar);

  gsl_matrix_free (holdCalculatedJacobian);
}

} // namespace CurveFitting
} // namespace Mantid
