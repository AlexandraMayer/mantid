#ifndef MANTID_API_IFunctionMWMW_H_
#define MANTID_API_IFunctionMWMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/variant.hpp>
#include <string>
#include <vector>

namespace Mantid
{

namespace CurveFitting
{
  class Fit;
}

namespace API
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;
class Jacobian;
class ParameterTie;
class IConstraint;
class ParameterReference;
class FunctionHandler;
/** This is an interface to a fitting function - a semi-abstarct class.
    Functions derived from IFunctionMW can be used with the Fit algorithm.
    IFunctionMW defines the structure of a fitting funtion.

    A function has a number of named parameters (not arguments), type double, on which it depends.
    Parameters must be declared either in the constructor or in the init() method
    of a derived class with method declareParameter(...). Method nParams() returns 
    the number of declared parameters. A parameter can be accessed either by its name
    or the index. For example in case of Gaussian the parameters can be "Height",
    "PeakCentre" and "Sigma".

    To fit a function to a set of data its parameters must be adjusted so that the difference
    between the data and the corresponding function values were minimized. This is the aim
    of the Fit algorithm. But Fit does not work with the declared parameters directly.
    Instead it uses other - active - parameters. The active parameters can be a subset of the
    declared parameters or completely different ones. The rationale for this is following.
    The fitting parameters sometimes need to be fixed during the fit or "tied" (expressed
    in terms of other parameters). In this case the active parameters will be those
    declared parameters which are not tied in any sence. Also some of the declared parameters
    can be unsuitable for the use in a fitting algorithm. In this case different active parameters
    can be used in place of the inefficient declared parameters. An example is Gaussian where
    "Sigma" makes the fit unstable. So in the fit it can be replaced with variable Weight = 1 / Sigma
    which is more efficient. The number of active parameters (returned by nActive()) cannot be
    greater than nParams(). The function which connects the active parameters with the declared ones
    must be monotonic so that the forward and backward transformations between the two sets are
    single-valued (this is my understanding). At the moment only simple one to one transformations
    of Weight - Sigma type are allowed. More complecated cases of simultaneous transformations of
    several parameters are not supported.

    The active parameters can be accessed by their index. The implementations of the access method
    for both active and declared parameters must ensure that any changes to one of them 
    immediately reflected on the other so that the two are consistent at any moment.

    IFunctionMW declares method nameOfActive(size_t i) which returns the name of the declared parameter
    corresponding to the i-th active parameter. I am not completely sure in the usefulness of it.

    IFunctionMW provides methods for tying and untying parameters. Only the declared parameters can be 
    tied. A tied parameter cannot be active. When a parameter is tied it becomes inactive.
    This implies that the number of active parameters is variable and can change at runtime.

    Method addConstraint adds constraints on possible values of a declared parameter. Constraints
    and ties are used only in fitting.

    The main method of IFunctionMW is called function(out,xValues,nData). It calculates nData output values
    out[i] at arguments xValues[i]. Implement functionDeriv method for the function to be used with
    fitting algorithms using derivatives. functionDeriv calculates patrial derivatives of the
    function with respect to the fitting parameters.

    Any non-fitting parameters can be implemented as attributes (class IFunctionMW::Attribute). 
    An attribute can have one of three types: std::string, int, or double. The type is set at construction
    and cannot be changed later. To read or write the attributes there are two ways. If the type
    is known the type specific accessors can be used, e.g. asString(), asInt(). Otherwise the
    IFunctionMW::AttributeVisitor can be used. It provides alternative virtual methods to access 
    attributes of each type. When creating a function from a string (using FunctionFactory::creaeInitialized(...))
    the attributes must be set first, before any fitting parameter, as the number and names of the parameters
    can depend on the attributes.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class MANTID_API_DLL IFunctionMW: public virtual IFitFunction
{
public:

  /// Constructor
  IFunctionMW():IFitFunction(),m_dataSize(0),m_data(NULL){}

  /* Overidden methods */

  using IFitFunction::setWorkspace;
  virtual void setWorkspace(boost::shared_ptr<const Workspace>){}
  virtual void setWorkspace(boost::shared_ptr<const Workspace> ws,bool copyData);
  virtual void setSlicing(const std::string& slicing);
  virtual boost::shared_ptr<const API::Workspace> getWorkspace()const;
  virtual void function(FunctionDomain& )const{}

  /// Returns the size of the fitted data (number of double values returned by the function)
  virtual size_t dataSize()const;
  /// Returns a reference to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  virtual const double* getData()const;
  virtual const double* getWeights()const;
  /// Function you want to fit to. 
  /// @param out :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  virtual void function(double* out)const;
  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv(Jacobian* out);

  /* MatrixWorkspace specific methods */

  /// Set the workspace
  virtual void setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX = 0.0, double endX = 0.0);
  /// Get the workspace
  virtual boost::shared_ptr<const API::MatrixWorkspace> getMatrixWorkspace()const{return m_workspace;}
  /// Get workspace index
  virtual size_t getWorkspaceIndex()const{return m_workspaceIndex;}

  /// Function you want to fit to.
  virtual void functionMW(double* out, const double* xValues, const size_t nData)const = 0;
  /// Derivatives of function with respect to active parameters
  virtual void functionDerivMW(Jacobian* out, const double* xValues, const size_t nData);
  /// Derivatives to be used in covariance matrix calculation. Override this method some of the fitted parameters
  /// are different form the declared ones.
  virtual void calJacobianForCovariance(Jacobian* out, const double* xValues, const size_t nData);

  /// To be used temporarily with the old Fit algorithm. 
  //virtual void setUpNewStuff(boost::shared_array<double> xs = boost::shared_array<double>(),
  //                           boost::shared_array<double> weights = boost::shared_array<double>());

  boost::shared_ptr<API::MatrixWorkspace> createCalculatedWorkspace(
    boost::shared_ptr<const API::MatrixWorkspace> inWS, 
    size_t wi,
    const std::vector<double>& sd = std::vector<double>()
    );

protected:

  /// Convert a value from one unit (inUnit) to unit defined in workspace (ws) 
  double convertValue(double value, Kernel::Unit_sptr& inUnit, 
                      boost::shared_ptr<const MatrixWorkspace> ws,
                      size_t wsIndex)const;

  void convertValue(std::vector<double>& values, Kernel::Unit_sptr& outUnit, 
    boost::shared_ptr<const MatrixWorkspace> ws,
    size_t wsIndex) const;

  /// Calculate numerical derivatives
  void calNumericalDeriv(Jacobian* out, const double* xValues, const size_t& nData);

  /// Shared pointer to the workspace
  boost::shared_ptr<const API::MatrixWorkspace> m_workspace;
  /// Spectrum index
  size_t m_workspaceIndex;
  /// Lower bin index
  size_t m_xMinIndex;
  /// Upper bin index
  size_t m_xMaxIndex;
  /// Size of the fitted data
  size_t m_dataSize;
  /// Pointer to the fitted data
  const double* m_data;
  /// Pointer to the fitting weights
  boost::shared_array<double> m_weights;
  /// Pointer to the x (function argument as in f(x)) data
  boost::shared_array<double> m_xValues;

  mutable boost::shared_array<double> m_tmpFunctionOutputMinusStep;
  mutable boost::shared_array<double> m_tmpFunctionOutputPlusStep;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;

  /// Making a friend
  friend class CurveFitting::Fit;

};

} // namespace API
} // namespace Mantid

/**
 * Macro for declaring a new type of function to be used with the FunctionFactory
 */
#define DECLARE_FUNCTION(classname) \
        namespace { \
        Mantid::Kernel::RegistrationHelper register_function_##classname( \
  ((Mantid::API::FunctionFactory::Instance().subscribe<classname>(#classname)) \
        , 0)); \
        }

#endif /*MANTID_API_IFunctionMW_H_*/
