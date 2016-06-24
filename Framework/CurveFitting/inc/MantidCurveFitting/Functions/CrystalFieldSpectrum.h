#ifndef MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_

#include "MantidCurveFitting/Functions/FunctionGenerator.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Calculates crystal field spectrum.

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CrystalFieldSpectrum : public FunctionGenerator {
public:
  CrystalFieldSpectrum();
  std::string name() const override { return "CrystalFieldSpectrum"; }
  const std::string category() const override { return "General"; }
  void buildTargetFunction() const override;

protected:
  void updateTargetFunction() const override;

//  /** @name Overrides implementing composition of two functions:
//   * CrystalFieldPeaks and CompositeFunction.
//   * These can be factored out into a separate class later.
//   */
//  //@{
//  /// Set i-th parameter
//  void setParameter(size_t, const double &value,
//                    bool explicitlySet = true) override;
//  /// Set i-th parameter description
//  void setParameterDescription(size_t, const std::string &description) override;
//  /// Get i-th parameter
//  double getParameter(size_t i) const override;
//  /// Set parameter by name.
//  void setParameter(const std::string &name, const double &value,
//                    bool explicitlySet = true) override;
//  /// Set description of parameter by name.
//  void setParameterDescription(const std::string &name,
//                               const std::string &description) override;
//  /// Get parameter by name.
//  double getParameter(const std::string &name) const override;
//  /// Total number of parameters
//  size_t nParams() const override;
//  /// Returns the index of parameter name
//  size_t parameterIndex(const std::string &name) const override;
//  /// Returns the name of parameter i
//  std::string parameterName(size_t i) const override;
//  /// Returns the description of parameter i
//  std::string parameterDescription(size_t i) const override;
//  /// Checks if a parameter has been set explicitly
//  bool isExplicitlySet(size_t i) const override;
//  /// Get the fitting error for a parameter
//  double getError(size_t i) const override;
//  /// Set the fitting error for a parameter
//  void setError(size_t i, double err) override;
//
//  /// Check if a declared parameter i is fixed
//  bool isFixed(size_t i) const override;
//  /// Removes a declared parameter i from the list of active
//  void fix(size_t i) override;
//  /// Restores a declared parameter i to the active status
//  void unfix(size_t i) override;
//
//  /// Return parameter index from a parameter reference.
//  size_t getParameterIndex(const API::ParameterReference &ref) const override;
//  /// Tie a parameter to other parameters (or a constant)
//  API::ParameterTie *tie(const std::string &parName, const std::string &expr,
//                         bool isDefault = false) override;
//  /// Apply the ties
//  void applyTies() override;
//  /// Remove all ties
//  void clearTies() override;
//  // Unhide base class function: removeTie(string).
//  using IFunction::removeTie;
//  /// Removes i-th parameter's tie
//  bool removeTie(size_t i) override;
//  /// Get the tie of i-th parameter
//  API::ParameterTie *getTie(size_t i) const override;
//
//  /// Add a constraint to function
//  void addConstraint(API::IConstraint *ic) override;
//  /// Get constraint of i-th parameter
//  API::IConstraint *getConstraint(size_t i) const override;
//  /// Remove a constraint
//  void removeConstraint(const std::string &parName) override;
//
//  /// Set up the function for a fit.
//  void setUpForFit() override;
//
//  /// Build m_spectrum function.
//  void buildSpectrumFunction() const;
//
//protected:
//  /// Declare a new parameter
//  void declareParameter(const std::string &name, double initValue = 0,
//                        const std::string &description = "") override;
//
//  /// Add a new tie. Derived classes must provide storage for ties
//  void addTie(API::ParameterTie *tie) override;
//  //@}
//
//public:
//  /** @name Attributes */
//  //@{
//  /// Returns the number of attributes associated with the function
//  size_t nAttributes() const override;
//  /// Returns a list of attribute names
//  std::vector<std::string> getAttributeNames() const override;
//  /// Return a value of attribute attName
//  Attribute getAttribute(const std::string &name) const override;
//  /// Set a value to attribute attName
//  void setAttribute(const std::string &name, const Attribute &) override;
//  /// Check if attribute attName exists
//  bool hasAttribute(const std::string &name) const override;
//  //@}
//
//  /// Evaluate the function
//  void function(const API::FunctionDomain &domain,
//                API::FunctionValues &values) const override;
//
//protected:
//  /// overwrite IFunction base class method, which declare function parameters
//  void init() override;
//
//private:
//  /// Test if a name (parameter's or attribute's) belongs to m_crystalFiled
//  bool isOwnName(const std::string &aName) const;
//  /// Update spectrum function.
//  void updateSpectrumFunction() const;
//  /// Update spectrum function if necessary.
//  void checkSpectrumFunction() const;
//  /// Function that calculates peak centres and intensities.
//  CrystalFieldPeaks m_crystalField;
//  /// Function that actually callculates the spectrum.
//  mutable API::CompositeFunction m_spectrum;
//  /// Cached number of parameters in m_crystalField.
//  size_t m_nOwnParams;
//  /// Flag indicating that updateSpectrumFunction() is required.
//  mutable bool m_dirty;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_*/
