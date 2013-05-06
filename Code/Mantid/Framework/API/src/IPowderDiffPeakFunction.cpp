//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <boost/lexical_cast.hpp>
#include <cmath>

const double IGNOREDCHANGE = 1.0E-9;

namespace Mantid
{
namespace API
{

  int IPowderDiffPeakFunction::s_peakRadius = 5;

  //----------------------------------------------------------------------------------------------
  /** Constructor. Sets peak radius to the value of curvefitting.peakRadius property
    */
  IPowderDiffPeakFunction::IPowderDiffPeakFunction()
  {
    // Set peak's radius from configuration
    int peakRadius;
    if ( Kernel::ConfigService::Instance().getValue("curvefitting.peakRadius",peakRadius) )
    {
      if ( peakRadius != s_peakRadius )
      {
        setPeakRadius(peakRadius);
      }
    }
  }  

  //----------------------------------------------------------------------------------------------
  /** Desctructor
    */
  IPowderDiffPeakFunction::~IPowderDiffPeakFunction()
  {

  }

  //----------------------------------------------------------------------------------------------
  /** Override setting parameter by parameter index
    * @param i :: parameter index in function;
    * @param value :: parameter name
    * @param explicitlySet ::
    */
  void IPowderDiffPeakFunction::setParameter(size_t i, const double& value, bool explicitlySet)
  {
    double origparamvalue = getParameter(i);
    if (fabs(origparamvalue - value) > IGNOREDCHANGE)
    {
      m_hasNewParameterValue = true;
    }
    ParamFunction::setParameter(i, value, explicitlySet);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Overriding setting parameter by parameter name
    * @param name :: name of the parameter to set
    * @param value :: parameter name
    * @param explicitlySet ::
    */
  void IPowderDiffPeakFunction::setParameter(const std::string& name, const double& value, bool explicitlySet)
  {
    double origparamvalue = getParameter(name);
    if (fabs(origparamvalue - value) > IGNOREDCHANGE)
    {
      m_hasNewParameterValue = true;
    }
    ParamFunction::setParameter(name, value, explicitlySet);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Get peak centre
    */
  double IPowderDiffPeakFunction::centre() const
  {
    // Re-calcualte peak parameters if required
    if (m_hasNewParameterValue)
      calculateParameters(false);

    return m_centre;
  }

  //----------------------------------------------------------------------------------------------
  /** Get peak height
    */
  double IPowderDiffPeakFunction::height() const
  {
    return m_intensity;
  }

  //----------------------------------------------------------------------------------------------
  /**  Set peak height (intensity indeed)
    */
  void IPowderDiffPeakFunction::setHeight(const double h)
  {
    m_intensity = h;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Get peak's FWHM
    */
  double IPowderDiffPeakFunction::fwhm() const
  {
    if (m_hasNewParameterValue)
      calculateParameters(false);

    return m_fwhm;
  }


  //----------------------------------------------------------------------------------------------
  /** General implementation of the method for all peaks. Limits the peak evaluation to
   * a certain number of FWHMs around the peak centre. The outside points are set to 0.
   * Calls functionLocal() to compute the actual values
   * @param out :: Output function values
   * @param xValues :: X values for data points
   * @param nData :: Number of data points
   */
  void IPowderDiffPeakFunction::function1D(double* out, const double* xValues, const size_t nData)const
  {
    double c = this->centre();
    double dx = fabs(s_peakRadius*this->fwhm());
    int i0 = -1;
    int n = 0;
    for(size_t i = 0; i < nData; ++i)
    {
      if (fabs(xValues[i] - c) < dx)
      {
        if (i0 < 0) i0 = static_cast<int>(i);
        ++n;
      }
      else
      {
        out[i] = 0.0;
      }
    }

    if (i0 < 0 || n == 0)
      return;
    this->functionLocal(out+i0, xValues+i0, n);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** General implementation of the method for all peaks. Calculates derivatives only
   * for a range of x values limited to a certain number of FWHM around the peak centre.
   * For the points outside the range all derivatives are set to 0.
   * Calls functionDerivLocal() to compute the actual values
   * @param out :: Derivatives
   * @param xValues :: X values for data points
   * @param nData :: Number of data points

  void IPowderDiffPeakFunction::functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData) const
  {
    double c = this->centre();
    double dx = fabs(s_peakRadius*this->fwhm());
    int i0 = -1;
    int n = 0;
    for(size_t i = 0; i < nData; ++i)
    {
      if (fabs(xValues[i] - c) < dx)
      {
        if (i0 < 0) i0 = static_cast<int>(i);
        ++n;
      }
      else
      {
        for(size_t ip = 0; ip < this->nParams(); ++ip)
        {
          out->set(i,ip, 0.0);
        }
      }
    }
    if (i0 < 0 || n == 0) return;
#if 0
    PartialJacobian1 J(out,i0);
    this->functionDerivLocal(&J,xValues+i0,n);
#else
    throw runtime_error("Need to think how to implement! Message 1026.");
#endif

    return;
  }
   */

  //----------------------------------------------------------------------------------------------
  /** Set peak radius
    * @param r :: radius
    */
  void IPowderDiffPeakFunction::setPeakRadius(const int& r)
  {
    if (r > 0)
    {
      s_peakRadius = r;
      std::string setting = boost::lexical_cast<std::string>(r);
      Kernel::ConfigService::Instance().setString("curvefitting.peakRadius",setting);
    }
  }


} // namespace API
} // namespace Mantid
