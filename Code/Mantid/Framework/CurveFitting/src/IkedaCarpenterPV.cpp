//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IkedaCarpenterPV.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>
#include <limits>
#include "MantidGeometry/Instrument/DetectorGroup.h"


namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace SpecialFunctionSupport;

DECLARE_FUNCTION(IkedaCarpenterPV)

// Get a reference to the logger
Kernel::Logger& IkedaCarpenterPV::g_log = Kernel::Logger::get("IkedaCarpenterPV");

double IkedaCarpenterPV::centre()const 
{
  return getParameter("X0");
}


void IkedaCarpenterPV::setHeight(const double h) 
{
  // calculate height of peakshape function corresponding to intensity = 1
  setParameter("I",1);
  double h0 = height();

  // to avoid devide by zero and to insane value for I to be set
  double minCutOff = 100.0*std::numeric_limits<double>::min();
  if ( h0 > 0 && h0 < minCutOff )
    h0 = minCutOff;
  if ( h0 < 0 && h0 > -minCutOff )
    h0 = -minCutOff;

  // The intensity is then estimated to be h/h0
  setParameter("I", h/h0);
};


double IkedaCarpenterPV::height()const 
{
  // return the function value at centre()
  double h0;
  double toCentre = centre();
  constFunction(&h0, &toCentre, 1);
  return h0;
};

double IkedaCarpenterPV::width()const 
{
  return sqrt(8.0*M_LN2*getParameter("SigmaSquared"))+getParameter("Gamma");
};

void IkedaCarpenterPV::setWidth(const double w) 
{
  setParameter("SigmaSquared", w*w/(32.0*M_LN2));  // used 4.0 * 8.0 = 32.0
  setParameter("Gamma", w/2.0);
};

void IkedaCarpenterPV::setCentre(const double c) 
{
  setParameter("X0",c);
};


void IkedaCarpenterPV::init()
{
  declareParameter("I", 0.0);
  declareParameter("Alpha0",1.6);
  declareParameter("Alpha1",1.5);
  declareParameter("Beta0",31.9);
  declareParameter("Kappa",46.0);
  declareParameter("SigmaSquared",1.0);
  declareParameter("Gamma",1.0);
  declareParameter("X0",0.0);
}


/** Method for updating m_waveLength, although don't do this if m_waveLengthFixed = true.
 *  Also if size of m_waveLength is equal to number of data (for a new instance of this 
 *  class this vector is empty initially) then don't recalculate it.
 *
 *  @param xValues x values
 *  @param nData length of xValues
 */
void IkedaCarpenterPV::calWavelengthAtEachDataPoint(const double* xValues, const int& nData) const
{
  if (!m_waveLengthFixed)
  { 
    // if wavelength vector already have the right size no need for resizing it
    // further we make the assumption that no need to recalculate this vector if
    // it already has the right size

    /*
    if (static_cast<int>(m_waveLength.size()) != nData)
    {
      // This peak shape requires the fit to be done in TOF units

      if ( m_workspace->getAxis(0)->unit()->unitID().compare("TOF") != 0 )
      {
        g_log.information() << "IkedaCarpenterPV function is perhaps best used when working with x-axis unit = TOF\n";
      }

      m_waveLength.resize(nData);

      // Get the geometric information for this detector
      
      Geometry::IInstrument_const_sptr instrument = m_workspace->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      const double l1 = instrument->getSource()->getDistance(*sample);
      Geometry::IDetector_sptr det = m_workspace->getDetector(m_workspaceIndex);  // i is the workspace index
      const double l2 = det->getDistance(*sample);
      const double twoTheta = m_workspace->detectorTwoTheta(det);
     
      Mantid::Kernel::Unit_const_sptr wavelength = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
      for (int i = 0; i < nData; i++)
      {
        m_waveLength[i] = xValues[i];
      }
      std::vector<double> y; // Create an empty vector, it's not used in fromTOF
      wavelength->fromTOF(m_waveLength,y,l1,l2,twoTheta,0,0.0,0.0);
    }*/

    if (static_cast<int>(m_waveLength.size()) != nData)
    {
      // This peak shape requires the fit to be done in TOF units

      if ( m_workspace->getAxis(0)->unit()->unitID().compare("TOF") != 0 )
      {
        g_log.information() << "IkedaCarpenterPV function is perhaps best used when working with x-axis unit = TOF\n";
      }

      m_waveLength.resize(nData);


      Mantid::Kernel::Unit_sptr wavelength = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
      for (int i = 0; i < nData; i++)
      {
        m_waveLength[i] = xValues[i];
      }
	    convertValue(m_waveLength, wavelength, m_workspace, m_workspaceIndex);
    }
  }
}


/** Convert values from unit defined in workspace (ws) to outUnit
 *
 *  @param values   As input: assumed to be in unit of workspace. 
 *                  As output: in unit of outUnit
 *  @param outUnit  unit to convert to
 *  @param ws      workspace
 *  @param wsIndex workspace index
 */
void IkedaCarpenterPV::convertValue(std::vector<double>& values, Kernel::Unit_sptr& outUnit, 
                               boost::shared_ptr<const API::MatrixWorkspace> ws,
                               int wsIndex) const
{
  Kernel::Unit_sptr wsUnit = ws->getAxis(0)->unit();

  // if unit required by formula or look-up-table different from ws-unit then 
  if ( outUnit->unitID().compare(wsUnit->unitID()) != 0 )
  {
    // first check if it is possible to do a quick convertion convert
    double factor,power;
    if (wsUnit->quickConversion(*outUnit,factor,power) )
    {
      for (unsigned int i = 0; i < values.size(); i++)
        values[i] = factor * std::pow(values[i],power);
    }
    else
    {
      double l1,l2,twoTheta;

      // Get l1, l2 and theta  (see also RemoveBins.calculateDetectorPosition())
      Geometry::IInstrument_const_sptr instrument = ws->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      l1 = instrument->getSource()->getDistance(*sample);
      Geometry::IDetector_sptr det = ws->getDetector(wsIndex);
      if ( boost::dynamic_pointer_cast<Geometry::DetectorGroup>(det) )
      {
        det = instrument->getDetector(det->getID());
      }
      if ( ! det->isMonitor() )
      {
        l2 = det->getDistance(*sample);
        twoTheta = ws->detectorTwoTheta(det);
      }
      else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
      {
        l2 = det->getDistance(*(instrument->getSource()));
        l2 = l2 - l1;
        twoTheta = 0.0;
      }

      std::vector<double> emptyVec;
      wsUnit->toTOF(values,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
      outUnit->fromTOF(values,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
    }
  }  
}


/** convert voigt params to pseudo voigt params
 *
 *  @param voigtSigmaSq voigt param
 *  @param voigtGamma voigt param
 *  @param H pseudo voigt param
 *  @param eta pseudo voigt param
 */
void IkedaCarpenterPV::convertVoigtToPseudo(const double& voigtSigmaSq, const double& voigtGamma, 
  double& H, double& eta) const
{
  double fwhmGsq = 8.0 * M_LN2 * voigtSigmaSq;
  double fwhmG = sqrt(fwhmGsq);
  double fwhmG4 = fwhmGsq*fwhmGsq;
  double fwhmL = voigtGamma;
  double fwhmLsq = voigtGamma*voigtGamma;
  double fwhmL4 = fwhmLsq*fwhmLsq;

  H = pow(fwhmG4*fwhmG+2.69269*fwhmG4*fwhmL+2.42843*fwhmGsq*fwhmG*fwhmLsq
    +4.47163*fwhmGsq*fwhmLsq*fwhmL+0.07842*fwhmG*fwhmL4+fwhmL4*fwhmL, 0.2);

  double tmp = fwhmL/H;

  eta = 1.36603*tmp - 0.47719*tmp*tmp + 0.11116*tmp*tmp*tmp;
}

void IkedaCarpenterPV::constFunction(double* out, const double* xValues, const int& nData) const
{
    const double& I = getParameter("I");
    const double& alpha0 =getParameter("Alpha0");
    const double& alpha1 = getParameter("Alpha1");
    const double& beta0 = getParameter("Beta0");
    const double& kappa = getParameter("Kappa");
    const double& voigtsigmaSquared = getParameter("SigmaSquared");
    const double& voigtgamma = getParameter("Gamma");
    const double& X0 = getParameter("X0");

    // cal pseudo voigt sigmaSq and gamma and eta
    double gamma = 1.0; // dummy initialization
    double eta = 0.5;   // dummy initialization
    convertVoigtToPseudo(voigtsigmaSquared, voigtgamma, gamma, eta);
    double sigmaSquared = gamma*gamma/(8.0*M_LN2); 

    const double beta = 1/beta0;

    // equations taken from Fullprof manual

    const double k = 0.05;   

    double u,v,s,r;
    double yu, yv, ys, yr;

    const double someConst = 1/sqrt(2.0*sigmaSquared);

    double R, Nu, Nv, Ns, Nr, N;

    std::complex<double> zs, zu, zv, zr;

    double alpha, a_minus, a_plus, x, y, z;

    // update wavelength vector
    calWavelengthAtEachDataPoint(xValues, nData);

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-X0;

        if (m_waveLengthFixed)
        {
          // this is to allow unit testing when a workspace is not available
          R = exp(-81.799/(m_waveLength[0]*m_waveLength[0]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[0]*alpha1);
        }
        else
        {
          R = exp(-81.799/(m_waveLength[i]*m_waveLength[i]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[i]*alpha1);
        }

        a_minus = alpha*(1-k);
        a_plus = alpha*(1+k);
        x=a_minus-beta;
        y=alpha-beta;
        z=a_plus-beta; 

        Nu=1-R*a_minus/x;
        Nv=1-R*a_plus/z;
        Ns=-2*(1-R*alpha/y);
        Nr=2*R*alpha*alpha*beta*k*k/(x*y*z);

        u=a_minus*(a_minus*sigmaSquared-2*diff)/2.0;
        v=a_plus*(a_plus*sigmaSquared-2*diff)/2.0;
        s=alpha*(alpha*sigmaSquared-2*diff)/2.0;
        r=beta*(beta*sigmaSquared-2*diff)/2.0;

        yu = (a_minus*sigmaSquared-diff)*someConst;
        yv = (a_plus*sigmaSquared-diff)*someConst;
        ys = (alpha*sigmaSquared-diff)*someConst;
        yr = (beta*sigmaSquared-diff)*someConst;

        zs = std::complex<double>(-alpha*diff, 0.5*alpha*gamma);
        zu = (1-k)*zs;
        zv = (1-k)*zs;
        zr = std::complex<double>(-beta*diff, 0.5*beta*gamma);

        N = 0.25*alpha*(1-k*k)/(k*k);

        out[i] = I*N*( (1-eta)*(Nu*exp(u+gsl_sf_log_erfc(yu))+Nv*exp(v+gsl_sf_log_erfc(yv)) + 
                        Ns*exp(s+gsl_sf_log_erfc(ys))+Nr*exp(r+gsl_sf_log_erfc(yr))) -
                 eta*2.0/M_PI*(Nu*exponentialIntegral(zu).imag()+Nv*exponentialIntegral(zv).imag()
                              +Ns*exponentialIntegral(zs).imag()+Nr*exponentialIntegral(zr).imag()) );
    }

}

void IkedaCarpenterPV::functionLocal(double* out, const double* xValues, const int& nData)const
{
    const double& I = getParameter("I");
    const double& alpha0 =getParameter("Alpha0");
    const double& alpha1 = getParameter("Alpha1");
    const double& beta0 = getParameter("Beta0");
    const double& kappa = getParameter("Kappa");
    const double& voigtsigmaSquared = getParameter("SigmaSquared");
    const double& voigtgamma = getParameter("Gamma");
    const double& X0 = getParameter("X0");

    // cal pseudo voigt sigmaSq and gamma and eta
    double gamma = 1.0; // dummy initialization
    double eta = 0.5;   // dummy initialization
    convertVoigtToPseudo(voigtsigmaSquared, voigtgamma, gamma, eta);
    double sigmaSquared = gamma*gamma/(8.0*M_LN2); 

    const double beta = 1/beta0;

    // equations taken from Fullprof manual

    const double k = 0.05;   

    double u,v,s,r;
    double yu, yv, ys, yr;

    const double someConst = 1/sqrt(2.0*sigmaSquared);

    double R, Nu, Nv, Ns, Nr, N;

    std::complex<double> zs, zu, zv, zr;

    double alpha, a_minus, a_plus, x, y, z;

    // update wavelength vector
    calWavelengthAtEachDataPoint(xValues, nData);

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-X0;

        if (m_waveLengthFixed)
        {
          // this is to allow unit testing when a workspace is not available
          R = exp(-81.799/(m_waveLength[0]*m_waveLength[0]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[0]*alpha1);
        }
        else
        {
          R = exp(-81.799/(m_waveLength[i]*m_waveLength[i]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[i]*alpha1);
        }

        a_minus = alpha*(1-k);
        a_plus = alpha*(1+k);
        x=a_minus-beta;
        y=alpha-beta;
        z=a_plus-beta; 

        Nu=1-R*a_minus/x;
        Nv=1-R*a_plus/z;
        Ns=-2*(1-R*alpha/y);
        Nr=2*R*alpha*alpha*beta*k*k/(x*y*z);

        u=a_minus*(a_minus*sigmaSquared-2*diff)/2.0;
        v=a_plus*(a_plus*sigmaSquared-2*diff)/2.0;
        s=alpha*(alpha*sigmaSquared-2*diff)/2.0;
        r=beta*(beta*sigmaSquared-2*diff)/2.0;

        yu = (a_minus*sigmaSquared-diff)*someConst;
        yv = (a_plus*sigmaSquared-diff)*someConst;
        ys = (alpha*sigmaSquared-diff)*someConst;
        yr = (beta*sigmaSquared-diff)*someConst;

        zs = std::complex<double>(-alpha*diff, 0.5*alpha*gamma);
        zu = (1-k)*zs;
        zv = (1-k)*zs;
        zr = std::complex<double>(-beta*diff, 0.5*beta*gamma);

        N = 0.25*alpha*(1-k*k)/(k*k);

        out[i] = I*N*( (1-eta)*(Nu*exp(u+gsl_sf_log_erfc(yu))+Nv*exp(v+gsl_sf_log_erfc(yv)) + 
                        Ns*exp(s+gsl_sf_log_erfc(ys))+Nr*exp(r+gsl_sf_log_erfc(yr))) -
                 eta*2.0/M_PI*(Nu*exponentialIntegral(zu).imag()+Nv*exponentialIntegral(zv).imag()
                              +Ns*exponentialIntegral(zs).imag()+Nr*exponentialIntegral(zr).imag()) );
    }
}

void IkedaCarpenterPV::functionDerivLocal(API::Jacobian* out, const double* xValues, const int& nData)
{
  calNumericalDeriv(out, xValues, nData);
}


} // namespace CurveFitting
} // namespace Mantid
