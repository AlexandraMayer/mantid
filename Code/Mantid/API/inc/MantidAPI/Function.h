#ifndef MANTID_API_FUNCTION_H_
#define MANTID_API_FUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/IFunction.h"
#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

namespace Mantid
{
namespace API
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Jacobian;
class ParameterTie;
class IConstraint;
/** An interface to a function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport Function : public IFunction
{
public:
  /// Default constructor
  Function(){}
  /// Copy contructor
  Function(const Function&);
  /// Assignment operator
  Function& operator=(const Function&);
  /// Virtual destructor
  virtual ~Function();

  /// Set i-th parameter
  virtual void setParameter(int, const double& value, bool explicitlySet = true);
  /// Get i-th parameter
  virtual double getParameter(int i)const;
  /// Set parameter by name.
  virtual void setParameter(const std::string& name, const double& value, bool explicitlySet = true);
  /// Get parameter by name.
  virtual double getParameter(const std::string& name)const;
  /// Total number of parameters
  virtual int nParams()const{return static_cast<int>(m_parameters.size());}
  /// Returns the index of parameter name
  virtual int parameterIndex(const std::string& name)const;
  /// Returns the index of a parameter
  //virtual int parameterIndex(const double* p)const;
  /// Returns the name of parameter i
  virtual std::string parameterName(int i)const;
  /// Checks if a parameter has been set explicitly
  virtual bool isExplicitlySet(int i)const;

  /// Number of active (in terms of fitting) parameters
  virtual int nActive()const{return static_cast<int>(m_indexMap.size());}
  /// Returns "global" index of active parameter i
  virtual int indexOfActive(int i)const;
  /// Returns the name of active parameter i
  virtual std::string nameOfActive(int i)const;

  /// Check if a declared parameter i is active
  virtual bool isActive(int i)const;
  /// Get active index for a declared parameter i
  virtual int activeIndex(int i)const;
  /// Removes a declared parameter i from the list of active
  virtual void removeActive(int i);
  /// Restores a declared parameter i to the active status
  virtual void restoreActive(int i);

  /// Return parameter index from a parameter reference. Usefull for constraints and ties in composite functions
  virtual int getParameterIndex(const ParameterReference& ref)const;
  /// Get the containing function
  IFunction* getContainingFunction(const ParameterReference& ref)const;
  /// Get the containing function
  IFunction* getContainingFunction(const IFunction* fun);

  /// Apply the ties
  virtual void applyTies();
  /// Remove all ties
  virtual void clearTies();
  /// Removes i-th parameter's tie
  virtual bool removeTie(int i);
  /// Get the tie of i-th parameter
  virtual ParameterTie* getTie(int i)const;

  /// Add a constraint to function
  virtual void addConstraint(IConstraint* ic);
  /// Get first constraint
  virtual IConstraint* firstConstraint()const;
  /// Get next constraint
  virtual IConstraint* nextConstraint()const;
  /// Remove a constraint
  virtual void removeConstraint(const std::string& parName);
 
  void setParametersToSatisfyConstraints();

  using IFunction::removeTie;

protected:

  /// Function initialization. Declare function parameters in this method.
  virtual void init(){};
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name,double initValue = 0);

  /// Add a new tie
  virtual void addTie(ParameterTie* tie);
  /// Get the address of the parameter. For use in UserFunction with mu::Parser
  virtual double* getParameterAddress(int i);

  /// Nonvirtual member which removes all declared parameters
  void clearAllParameters();

private:
  /// The index map. m_indexMap[i] gives the total index for active parameter i
  std::vector<int> m_indexMap;
  /// Keeps parameter names
  std::vector<std::string> m_parameterNames;
  /// Keeps parameter values
  std::vector<double> m_parameters;
  /// Holds parameter ties as <parameter index,tie pointer> 
  std::vector<ParameterTie*> m_ties;
  /// Holds the constraints added to function
  std::vector<IConstraint*> m_constraints;
  /// Flags of explicitly set parameters
  std::vector<bool> m_explicitlySet;
  /// Constraint counter for use in nextConstraint()
  mutable int m_iConstraint;
};

} // namespace API
} // namespace Mantid


#endif /*MANTID_API_FUNCTION_H_*/
