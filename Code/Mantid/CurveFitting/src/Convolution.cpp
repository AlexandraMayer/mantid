//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Convolution.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <algorithm>
#include <functional>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

#include <sstream>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

//DECLARE_FUNCTION(Convolution)

/// Constructor
Convolution::Convolution()
:m_resolution(NULL),m_resolutionSize(0)
{
}

/// Destructor
Convolution::~Convolution()
{
  if (m_resolution) delete[] m_resolution;
}

void Convolution::init()
{
}

/**
 * Calculates convolution of the two member functions. 
 */
void Convolution::function(double* out, const double* xValues, const int& nData)
{
  if (nFunctions() == 0)
  {
    std::fill_n(out,nData,0.0);
    return;
  }

  if (m_resolutionSize != nData)
  {
    refreshResolution();
  }

  gsl_fft_real_workspace * workspace = gsl_fft_real_workspace_alloc(nData);
  gsl_fft_real_wavetable * wavetable = gsl_fft_real_wavetable_alloc(nData);

  int n2 = nData / 2;
  bool odd = n2*2 != nData;
  if (m_resolution == 0)
  {
    m_resolutionSize = nData;
    m_resolution = new double[nData];
    // the resolution must be defined on interval -L < xr < L, L == (xValues[nData-1] - xValues[0]) / 2 
    double* xr = new double[nData];
    double dx = (xValues[nData-1] - xValues[0]) / (nData - 1);
    // make sure that xr[nData/2] == 0.0
    xr[n2] = 0.0;
    for(int i=1;i<n2;i++)
    {
      double x = i*dx;
      xr[n2 + i] =  x;
      xr[n2 - i] = -x;
    }
    
    xr[0] = -n2*dx;
    if (odd) xr[nData-1] = -xr[0];

    getFunction(0)->function(m_resolution,xr,nData);

    // rotate the data to produce the right transform
    if (odd)
    {
      double tmp = m_resolution[nData-1];
      for(int i=n2-1;i>=0;i--)
      {
        m_resolution[n2+i+1] = m_resolution[i];
        m_resolution[i] = m_resolution[n2+i];
      }
      m_resolution[n2] = tmp;
    }
    else
    {
      for(int i=0;i<n2;i++)
      {
        double tmp = m_resolution[i];
        m_resolution[i] = m_resolution[n2+i];
        m_resolution[n2+i] = tmp;
      }
    }
    gsl_fft_real_transform (m_resolution, 1, nData, wavetable, workspace);
    std::transform(m_resolution,m_resolution+nData,m_resolution,std::bind2nd(std::multiplies<double>(),dx));
    delete[] xr;
  }

  // return the resolution transform for testing
  if (nFunctions() == 1)
  {
    double dx = 1.;//nData > 1? xValues[1] - xValues[0]: 1.;
    std::transform(m_resolution,m_resolution+nData,out,std::bind2nd(std::multiplies<double>(),dx));
    gsl_fft_real_wavetable_free (wavetable);
    gsl_fft_real_workspace_free (workspace);
    return;
  }

  getFunction(1)->function(out,xValues,nData);
  gsl_fft_real_transform (out, 1, nData, wavetable, workspace);
  gsl_fft_real_wavetable_free (wavetable);

  double dx = nData > 1? xValues[1] - xValues[0]: 1.;
  std::transform(out,out+nData,out,std::bind2nd(std::multiplies<double>(),dx));

  HalfComplex res(m_resolution,nData);
  HalfComplex fun(out,nData);

  double df = nData > 1? 1./(xValues[nData-1] - xValues[0]): 1.;
  //std::cerr<<"df="<<df<<'\n';
  //std::ofstream ftrans("trans.txt");
  for(int i = 0; i <= res.size(); i++)
  {
    // complex multiplication
    double res_r = res.real(i);
    double res_i = res.imag(i);
    double fun_r = fun.real(i);
    double fun_i = fun.imag(i);
    fun.set(i,res_r*fun_r - res_i*fun_i,res_r*fun_i + res_i*fun_r);
    //ftrans<<df*i<<' '<<fun.real(i)<<' '<<fun.imag(i)<<'\n';
  }

  gsl_fft_halfcomplex_wavetable * wavetable_r = gsl_fft_halfcomplex_wavetable_alloc(nData);
  gsl_fft_halfcomplex_inverse(out, 1, nData, wavetable_r, workspace);
  gsl_fft_halfcomplex_wavetable_free (wavetable_r);

  gsl_fft_real_workspace_free (workspace);

  dx = nData > 1? 1./(xValues[1] - xValues[0]): 1.;
  std::transform(out,out+nData,out,std::bind2nd(std::multiplies<double>(),dx));

}

void Convolution::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    //const double& height = getParameter("Height");
    //const double& peakCentre = getParameter("PeakCentre");
    //const double& weight = pow(1/getParameter("Sigma"),2);

    //for (int i = 0; i < nData; i++) {
    //    double diff = xValues[i]-peakCentre;
    //    double e = exp(-0.5*diff*diff*weight);
    //    out->set(i,0, e);
    //    out->set(i,1, diff*height*e*weight);
    //    out->set(i,2, -0.5*diff*diff*height*e);  // derivative with respect to weight not sigma
    //}
}

/**
 * The first function added must be the resolution. 
 * The second is the convoluted (model) function. If third, fourth and so on
 * functions added they will be combined in a composite function. So the
 * Convolution always has two member functions: the resolution and the model.
 * @param f A pointer to the function to add
 * @return The index of the new function which will be 0 for the resolution and 1 for the model
 */
int Convolution::addFunction(IFunction* f)
{
  if (nFunctions() == 0)
  {
    for(int i=0;i<f->nParams();i++)
    {
      f->removeActive(i);
    }
  }
  int iFun = 0;
  if (nFunctions() < 2)
  {
    iFun = CompositeFunction::addFunction(f);
  }
  else
  {
    IFunction* f1 = getFunction(1);
    CompositeFunction* cf = dynamic_cast<CompositeFunction*>(f1);
    if (cf == 0)
    {
      cf = dynamic_cast<CompositeFunction*>(API::FunctionFactory::Instance().createFunction("CompositeFunction"));
      removeFunction(1,false);// remove but don't delete
      cf->addFunction(f1);
      CompositeFunction::addFunction(cf);
    }
    cf->addFunction(f);
    checkFunction();
    iFun = 1;
  }
  return iFun;
}

/// Deletes and zeroes pointer m_resolution forsing function(...) to recalculate the resolution function
void Convolution::refreshResolution()const
{
  if (m_resolution) delete[] m_resolution;
  m_resolution = NULL;
  m_resolutionSize = 0;
}

/// Writes itself into a string
std::string Convolution::asString()const
{
  if (nFunctions() != 2)
  {
    throw std::runtime_error("Convolution function is incomplete");
  }
  std::ostringstream ostr;
  ostr<<"composite=Convolution;";
  IFunction* res = getFunction(0);
  IFunction* fun = getFunction(1);
  bool isCompRes = dynamic_cast<CompositeFunction*>(res) != 0;
  bool isCompFun = dynamic_cast<CompositeFunction*>(fun) != 0;

  if (isCompRes) ostr << '(';
  ostr << *res ;
  if (isCompRes) ostr << ')';
  ostr << ';';

  if (isCompFun) ostr << '(';
  ostr << *fun ;
  if (isCompFun) ostr << ')';

  return ostr.str();
}


} // namespace CurveFitting
} // namespace Mantid
