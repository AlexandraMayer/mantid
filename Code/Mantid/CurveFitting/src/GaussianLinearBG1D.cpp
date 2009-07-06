//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GaussianLinearBG1D.h"

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(GaussianLinearBG1D)

using namespace Kernel;

void GaussianLinearBG1D::declareParameters()
{
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  declareProperty("BG0", 0.0,
    "Constant background value (default 0)", Direction::InOut);
  declareProperty("BG1", 0.0,
    "Linear background modelling parameter (default 0)",
    Direction::InOut);
  declareProperty("Height", 0.0, "Height of peak (default 0)",
    Direction::InOut);
  declareProperty("PeakCentre", 0.0, "Centre of peak (default 0)",
    Direction::InOut);
  declareProperty("Sigma", 1.0, positiveDouble, 
    "Standard deviation (default 1)", Direction::InOut);
}

void GaussianLinearBG1D::modifyStartOfRange(double& startX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  startX = peak_val-(6*sigma);
}

void GaussianLinearBG1D::modifyEndOfRange(double& endX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  endX = peak_val+(6*sigma);
}

void GaussianLinearBG1D::modifyInitialFittedParameters(std::vector<double>& fittedParameter)
{
  const double sigma = getProperty("Sigma");

  fittedParameter[4] = 1/(sigma*sigma);  // the fitting is actually done on 1/sigma^2, also referred to as the weight
}

void GaussianLinearBG1D::modifyFinalFittedParameters(std::vector<double>& fittedParameter) 
{
  double weight = fittedParameter[4];

  fittedParameter[4] = sqrt(1/weight); // to convert back to sigma
}


void GaussianLinearBG1D::function(const double* in, double* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData)
{
    const double& bg0 = in[0];
    const double& bg1 = in[1];
    const double& height = in[2];
    const double& peakCentre = in[3];
    const double& weight = in[4];

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        double Yi = height*exp(-0.5*diff*diff*weight)+bg0+bg1*xValues[i];
        out[i] = (Yi - yValues[i])/yErrors[i];
    }
}

void GaussianLinearBG1D::functionDeriv(const double* in, Jacobian* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData)
{
    const double& height = in[2];
    const double& peakCentre = in[3];
    const double& weight = in[4];

    int nParam = m_parameterNames.size();
    for (int i = 0; i < nData; i++) {
        double s = yErrors[i];
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight)/s;
        out->set(i,0, 1/s);
        out->set(i,1, xValues[i]/s);
        out->set(i,2, e);
        out->set(i,3, diff*height*e*weight);
        out->set(i,4, -0.5*diff*diff*height*e);
    }
}

} // namespace CurveFitting
} // namespace Mantid
