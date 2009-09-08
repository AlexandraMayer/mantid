//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian1D.h"

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Gaussian1D)

using namespace Kernel;

void Gaussian1D::declareParameters()
{
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  declareProperty("BG0", 0.0,
    "Constant background value (default 0)", Direction::InOut);
  declareProperty("Height", 0.0, "Height of peak (default 0)",
    Direction::InOut);
  declareProperty("PeakCentre",0.0, "Centre of peak (default 0)",
    Direction::InOut);
  declareProperty("Sigma", 1.0, positiveDouble,
    "Standard deviation (default 1)", Direction::InOut);
}

void Gaussian1D::modifyStartOfRange(double& startX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  startX = peak_val-(6*sigma);
}

void Gaussian1D::modifyEndOfRange(double& endX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  endX = peak_val+(6*sigma);
}

void Gaussian1D::modifyInitialFittedParameters(std::vector<double>& fittedParameter)
{
  const double sigma = getProperty("Sigma");

  fittedParameter[3] = 1/(sigma*sigma);  // the fitting is actually done on 1/sigma^2, also referred to as the weight
}

void Gaussian1D::modifyFinalFittedParameters(std::vector<double>& fittedParameter) 
{
  double weight = fittedParameter[3];

  fittedParameter[3] = sqrt(1/weight); // to convert back to sigma
}

double Gaussian1D::function(const double* in, const double& x)
{
    const double& bg0 = in[0];
    const double& height = in[1];
    const double& peakCentre = in[2];
    const double& weight = in[3];

    double diff=x-peakCentre;
    return height*exp(-0.5*diff*diff*weight)+bg0;
}

void Gaussian1D::function(const double* in, double* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData)
{
    const double& bg0 = in[0];
    const double& height = in[1];
    const double& peakCentre = in[2];
    const double& weight = in[3];

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        double Yi = height*exp(-0.5*diff*diff*weight)+bg0;
        out[i] = (Yi - yValues[i])/yErrors[i];
    }
}

void Gaussian1D::functionDeriv(const double* in, Jacobian* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData)
{
    const double& height = in[1];
    const double& peakCentre = in[2];
    const double& weight = in[3];

    for (int i = 0; i < nData; i++) {
        double s = yErrors[i];
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight)/s;
        out->set(i,0, 1/s);
        out->set(i,1, e);
        out->set(i,2, diff*height*e*weight);
        out->set(i,3, -0.5*diff*diff*height*e);
    }
}

} // namespace CurveFitting
} // namespace Mantid
