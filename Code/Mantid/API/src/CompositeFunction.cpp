//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterTie.h"

#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <sstream>
#include <iostream>

namespace Mantid
{
namespace API
{

DECLARE_FUNCTION(CompositeFunction)

/// Copy contructor
CompositeFunction::CompositeFunction(const CompositeFunction& f)
:m_nActive(f.m_nParams),m_nParams(f.m_nParams),m_iConstraintFunction(0)
{
  m_functions.assign(f.m_functions.begin(),f.m_functions.end());
  m_activeOffsets.assign(f.m_activeOffsets.begin(),f.m_activeOffsets.end());
  m_paramOffsets.assign(f.m_paramOffsets.begin(),f.m_paramOffsets.end());
}

///Assignment operator
CompositeFunction& CompositeFunction::operator=(const CompositeFunction& f)
{
  m_nActive = f.m_nActive;
  m_nParams = f.m_nParams;
  m_functions.assign(f.m_functions.begin(),f.m_functions.end());
  m_activeOffsets.assign(f.m_activeOffsets.begin(),f.m_activeOffsets.end());
  m_paramOffsets.assign(f.m_paramOffsets.begin(),f.m_paramOffsets.end());
  m_iConstraintFunction = f.m_iConstraintFunction;
  return *this;
}

///Destructor
CompositeFunction::~CompositeFunction()
{
  for(int i=0;i<nFunctions();i++)
    if (m_functions[i]) delete m_functions[i];
}


/// Function initialization. Declare function parameters in this method.
void CompositeFunction::init()
{
}

/// Writes itself into a string
std::string CompositeFunction::asString()const
{
  std::ostringstream ostr;
  for(int i=0;i<nFunctions();i++)
  {
    IFunction* fun = getFunction(i);
    bool isComp = dynamic_cast<CompositeFunction*>(fun) != 0;
    if (isComp) ostr << '(';
    ostr << fun->asString() << ';';
    if (isComp) ostr << ')';
  }
  return ostr.str();
}

/// Function you want to fit to.
void CompositeFunction::function(double* out, const double* xValues, const int& nData)
{
  if (nData <= 0) return;
  boost::shared_array<double> tmpOut(new double[nData]);
  for(int i=0;i<nFunctions();i++)
  {
    if (i == 0)
      m_functions[i]->function(out,xValues,nData);
    else
    {
      m_functions[i]->function(tmpOut.get(),xValues,nData);
      std::transform(out,out+nData,tmpOut.get(),out,std::plus<double>());
    }
  }
}

/** A Jacobian for individual functions
 */
class PartialJacobian: public Jacobian
{
  Jacobian* m_J;  ///< pointer to the overall Jacobian
  int m_iP0;      ///< offset in the overall Jacobian for a particular function
  int m_iaP0;      ///< offset in the active Jacobian for a particular function
public:
  /** Constructor
   * @param J A pointer to the overall Jacobian
   * @param iP0 The parameter index (declared) offset for a particular function
   * @param iap0 The active parameter index (declared) offset for a particular function
   */
  PartialJacobian(Jacobian* J,int iP0, int iap0):m_J(J),m_iP0(iP0),m_iaP0(iap0)
  {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY The index of the data point
   * @param iP The parameter index of an individual function.
   * @param value The derivative value
   */
  void set(int iY, int iP, double value)
  {
      m_J->set(iY,m_iP0 + iP,value);
  }
 /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
  *   @param value Value to add
  *   @param iActiveP The index of an active parameter.
  */
  virtual void addNumberToColumn(const double& value, const int& iActiveP) 
  {
    m_J->addNumberToColumn(value,m_iaP0+iActiveP);
  }
};

/// Derivatives of function with respect to active parameters
void CompositeFunction::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  for(int i=0;i<nFunctions();i++)
  {
    PartialJacobian J(out,m_paramOffsets[i],m_activeOffsets[i]);
    m_functions[i]->functionDeriv(&J,xValues,nData);
  }
}

/// Derivatives to be used in covariance matrix calculation. 
void CompositeFunction::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  if (nData <= 0) return;
  for(int i=0;i<nFunctions();i++)
  {
    PartialJacobian J(out,m_paramOffsets[i],m_activeOffsets[i]);
    m_functions[i]->calJacobianForCovariance(&J,xValues,nData);
  }
}


/** Sets a new value to the i-th parameter.
 *  @param i The parameter index
 *  @param value The new value
 *  @param explicitlySet A boolean falgging the parameter as explicitly set (by user)
 */
void CompositeFunction::setParameter(int i, const double& value, bool explicitlySet)
{
  int iFun = functionIndex(i);
  m_functions[ iFun ]->setParameter(i - m_paramOffsets[iFun],value,explicitlySet);
}

/** Get the i-th parameter.
 *  @param i The parameter index
 */
double CompositeFunction::getParameter(int i)const
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->getParameter(i - m_paramOffsets[iFun]);
}

/**
 * Sets a new value to a parameter by name.
 * @param name The name of the parameter.
 * @param value The new value
 * @param explicitlySet A boolean falgging the parameter as explicitly set (by user)
 */
void CompositeFunction::setParameter(const std::string& name, const double& value, bool explicitlySet)
{
  std::string pname;
  int index;
  parseName(name,index,pname);
  if (index < 0)
    throw std::invalid_argument("CompositeFunction::getParameter: parameter name must contain function index");
  else
  {   
    getFunction(index)->setParameter(pname,value,explicitlySet);
  }
}

/**
 * Parameters by name.
 * @param name The name of the parameter.
 */
double CompositeFunction::getParameter(const std::string& name)const
{
  std::string pname;
  int index;
  parseName(name,index,pname);
  if (index < 0)
    throw std::invalid_argument("CompositeFunction::getParameter: parameter name must contain function index");
  else
  {   
    return getFunction(index)->getParameter(pname);
  }
}

/// Total number of parameters
int CompositeFunction::nParams()const
{
  return m_nParams;
}

/**
 * 
 * @param name The name of a parameter
 */
int CompositeFunction::parameterIndex(const std::string& name)const
{
  std::string pname;
  int index;
  parseName(name,index,pname);
  if (index < 0)
    throw std::invalid_argument("CompositeFunction::getParameter: parameter name must contain function index");

  return m_paramOffsets[index] + getFunction(index)->parameterIndex(pname);
}

/**
 * Checks that a pointer points to a parameter of this function and returns its index.
 * @param p A pointer to a double variable.
 * @return The index of the parameter or -1 if p is not a pointer to any of the function's parameters.
 */
//int CompositeFunction::parameterIndex(const double* p)const
//{
//  for(int iFun=0;iFun<nFunctions();iFun++)
//  {
//    int i = m_functions[iFun]->parameterIndex(p);
//    if (i >= 0)
//    {
//      return m_paramOffsets[iFun] + i;
//    }
//  }
//  return -1;
//}

/// Returns the name of parameter i
std::string CompositeFunction::parameterName(int i)const
{
  int iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.' << m_functions[ iFun ]->parameterName(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/// Number of active (in terms of fitting) parameters
int CompositeFunction::nActive()const
{
  return m_nActive;
}

/// Value of i-th active parameter. Override this method to make fitted parameters different from the declared
double CompositeFunction::activeParameter(int i)const
{
  int iFun = functionIndexActive(i);
  return m_functions[ iFun ]->activeParameter(i - m_activeOffsets[iFun]);
}

/// Set new value of i-th active parameter. Override this method to make fitted parameters different from the declared
void CompositeFunction::setActiveParameter(int i, double value)
{
  int iFun = functionIndexActive(i);
  return m_functions[ iFun ]->setActiveParameter(i - m_activeOffsets[iFun],value);
}

/// Update parameters after a fitting iteration
void CompositeFunction::updateActive(const double* in)
{
  for(int iFun = 0; iFun < int(m_functions.size()); iFun++)
  {
    m_functions[ iFun ]->updateActive(in + m_activeOffsets[ iFun ]);
  }
  applyTies();
}

/// Returns "global" index of active parameter i
int CompositeFunction::indexOfActive(int i)const
{
  int iFun = functionIndexActive(i);
  return m_paramOffsets[ iFun ] + m_functions[ iFun ]->indexOfActive(i - m_activeOffsets[iFun]);
}

/// Returns the name of active parameter i
std::string CompositeFunction::nameOfActive(int i)const
{
  int iFun = functionIndexActive(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.' << m_functions[ iFun ]->nameOfActive(i - m_activeOffsets[iFun]);
  return ostr.str();
}

/**
 * Returns true if parameter i is active
 * @param i The index of a declared parameter
 */
bool CompositeFunction::isActive(int i)const
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->isActive(i - m_paramOffsets[iFun]);
}

/**
 * @param i A declared parameter index to be removed from active
 */
void CompositeFunction::removeActive(int i)
{
  int iFun = functionIndex(i);
  int ia = m_activeOffsets[iFun] + m_functions[iFun]->activeIndex(i - m_paramOffsets[iFun]);
  m_iFunctionActive.erase(m_iFunctionActive.begin()+ia);
  m_functions[ iFun ]->removeActive(i - m_paramOffsets[iFun]);

  m_nActive--;
  for(int j=iFun+1;j<nFunctions();j++)
    m_activeOffsets[j] -= 1;
}

/** Makes a parameter active again. It doesn't change the parameter's tie.
 * @param i A declared parameter index to be restored to active
 */
void CompositeFunction::restoreActive(int i)
{
  int iFun = functionIndex(i);
  int ia = m_activeOffsets[iFun] + m_functions[iFun]->activeIndex(i - m_paramOffsets[iFun]);

  std::vector<int>::iterator itFun = 
    std::find_if(m_iFunctionActive.begin(),m_iFunctionActive.end(),std::bind2nd(std::greater<int>(),i));

  m_iFunctionActive.insert(itFun,1,ia);
  m_functions[ iFun ]->restoreActive(i - m_paramOffsets[iFun]);

  m_nActive++;
  for(int j=iFun+1;j<nFunctions();j++)
    m_activeOffsets[j] += 1;
}

/**
 * @param i The index of a declared parameter
 * @return The index of declared parameter i in the list of active parameters or -1
 *         if the parameter is tied.
 */
int CompositeFunction::activeIndex(int i)const
{
  int iFun = functionIndex(i);
  int j = m_functions[iFun]->activeIndex(i - m_paramOffsets[iFun]);

  if (j == -1) 
  {
    return -1;
  }

  return m_activeOffsets[iFun] + j;
}

/** Makes sure that the function is consistent. 
 */
void CompositeFunction::checkFunction()
{
  m_nParams = 0;
  m_nActive = 0;
  m_paramOffsets.clear();
  m_activeOffsets.clear();
  m_iFunction.clear();
  m_iFunctionActive.clear();

  std::vector<IFunction*> functions(m_functions.begin(),m_functions.end());
  m_functions.clear();

  for(int i=0;i<functions.size();i++)
  {
    IFunction* f = functions[i];
    addFunction(f);
  }
}

/** Add a function
 * @param f A pointer to the added function
 * @return The function index
 */
int CompositeFunction::addFunction(IFunction* f)
{
  m_iFunction.insert(m_iFunction.end(),f->nParams(),m_functions.size());
  m_iFunctionActive.insert(m_iFunctionActive.end(),f->nActive(),m_functions.size());
  m_functions.push_back(f);
  //?f->init();
  if (m_paramOffsets.size() == 0)
  {
    m_paramOffsets.push_back(0);
    m_activeOffsets.push_back(0);
    m_nParams = f->nParams();
    m_nActive = f->nActive();
  }
  else
  {
    m_paramOffsets.push_back(m_nParams);
    m_activeOffsets.push_back(m_nActive);
    m_nParams += f->nParams();
    m_nActive += f->nActive();
  }
  return m_functions.size()-1;
}

/** Remove a function
 * @param i The index of the function to remove
 * @param del The deletion flag. If true the function will be deleted otherwise - simply detached
 */
void CompositeFunction::removeFunction(int i, bool del)
{
  if ( i >= nFunctions() )
    throw std::out_of_range("Function index out of range.");

  IFunction* fun = getFunction(i);

  int dna = fun->nActive();
  int dnp = fun->nParams();

  for(int j=0;j<nParams();)
  {
    ParameterTie* tie = getTie(j);
    if (tie && tie->findParametersOf(fun))
    {
      removeTie(j);
    }
    else
    {
      j++;
    }
  }

  // Shift down the function indeces for parameters
  for(std::vector<int>::iterator it=m_iFunction.begin();it!=m_iFunction.end();)
  {

    if (*it == i)
    {
      it = m_iFunction.erase(it);
    }
    else
    {
      if (*it > i)
      {
        *it -= 1;
      }
      it++;
    }
  }

  // Shift down the function indeces for active parameters
  for(std::vector<int>::iterator it=m_iFunctionActive.begin();it!=m_iFunctionActive.end();)
  {
    if (*it == i)
    {
      it = m_iFunctionActive.erase(it);
    }
    else
    {
      if (*it > i)
      {
        *it -= 1;
      }
      it++;
    }
  }

  m_nActive -= dna;
  // Shift the active offsets down by the number of i-th function's active params
  for(int j=i+1;j<nFunctions();j++)
  {
    m_activeOffsets[j] -= dna;
  }
  m_activeOffsets.erase(m_activeOffsets.begin()+i);

  m_nParams -= dnp;
  // Shift the parameter offsets down by the total number of i-th function's params
  for(int j=i+1;j<nFunctions();j++)
  {
    m_paramOffsets[j] -= dnp;
  }
  m_paramOffsets.erase(m_paramOffsets.begin()+i);

  m_functions.erase(m_functions.begin()+i);
  if (del)
  {
    delete fun;
  }
}

/** Replace a function with a new one. The old function is deleted.
 * @param i The index of the function to replace
 * @param f A pointer to the new function
 */
void CompositeFunction::replaceFunction(int i,IFunction* f)
{
  if ( i >= nFunctions() )
    throw std::out_of_range("Function index out of range.");

  IFunction* fun = getFunction(i);
  int na_old = fun->nActive();
  int np_old = fun->nParams();

  int na_new = f->nActive();
  int np_new = f->nParams();

  // Modify function indeces: The new function may have different number of parameters
  {
    std::vector<int>::iterator itFun = std::find(m_iFunction.begin(),m_iFunction.end(),i);
    if(itFun != m_iFunction.end()) // functions must have at least 1 parameter
    {
      if (np_old > np_new)
      {
        m_iFunction.erase(itFun,itFun + np_old - np_new);
      }
      else if (np_old < np_new) 
      {
        m_iFunction.insert(itFun,np_new - np_old,i);
      }
    }
  }

  // Modify function indeces: The new function may have different number of active parameters
  {
    std::vector<int>::iterator itFun = std::find(m_iFunctionActive.begin(),m_iFunctionActive.end(),i);
    if (itFun != m_iFunctionActive.end())
    {
      if (na_old > na_new)
      {
        m_iFunctionActive.erase(itFun,itFun + na_old - na_new);
      }
      else if (na_old < na_new) 
      {
        m_iFunctionActive.insert(itFun,na_new - na_old,i);
      }
    }
    else if (na_new > 0)
    {
      itFun = std::find_if(m_iFunctionActive.begin(),m_iFunctionActive.end(),std::bind2nd(std::greater<int>(),i));
      m_iFunctionActive.insert(itFun,na_new,i);
    }
  }

  int dna = na_new - na_old;
  m_nActive += dna;
  // Recalc the active offsets 
  for(int j=i+1;j<nFunctions();j++)
  {
    m_activeOffsets[j] += dna;
  }

  int dnp = np_new - np_old;
  m_nParams += dnp;
  // Shift the parameter offsets down by the total number of i-th function's params
  for(int j=i+1;j<nFunctions();j++)
  {
    m_paramOffsets[j] += dnp;
  }

  m_functions[i] = f;
  delete fun;
}

/**
 * @param i The index of the function
 */
IFunction* CompositeFunction::getFunction(int i)const
{
  if ( i >= nFunctions() )
    throw std::out_of_range("Function index out of range.");

  return m_functions[i];
}

/**
 * Get the index of the function to which parameter i belongs
 * @param i The parameter index
 */
int CompositeFunction::functionIndex(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_iFunction[i];
}

/**
 * Get the index of the function to which parameter i belongs
 * @param i The active parameter index
 */
int CompositeFunction::functionIndexActive(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_iFunctionActive[i];
}

/**
* @param varName The variable name which may contain function index ( [f<index.>]name )
* @param index Receives function index or -1 
* @param name Receives the parameter name
*/
void CompositeFunction::parseName(const std::string& varName,int& index, std::string& name)
{
  size_t i = varName.find('.');
  if (i == std::string::npos)
  {
    name = varName;
    index = -1;
    return;
  }
  else
  {
    if (varName[0] != 'f')
      throw std::invalid_argument("External function parameter name must start with 'f'");

    std::string sindex = varName.substr(1,i-1);
    index = boost::lexical_cast<int>(sindex);

    if (i == varName.size() - 1)
      throw std::invalid_argument("Name cannot be empty");

    name = varName.substr(i+1);
  }
}

/** Returns the name of parameter i as it declared in its function
 * @param i The parameter index
 * @return The pure parameter name (without the function identifier f#.)
 */
std::string CompositeFunction::parameterLocalName(int i)const
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->parameterName(i - m_paramOffsets[iFun]);
}

/** Initialize the function providing it the workspace
 * @param workspace The shared pointer to a workspace to which the function will be fitted
 * @param spec The number of a spectrum for fitting
 * @param xMin The minimum bin index of spectrum spec that will be used in fitting
 * @param xMax The maximum bin index of spectrum spec that will be used in fitting
 */
void CompositeFunction::setWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int spec,int xMin,int xMax)
{
  IFunction::setWorkspace(workspace,spec,xMin,xMax);
  for(int i=0;i<nFunctions();i++)
    getFunction(i)->setWorkspace(workspace,spec,xMin,xMax);
}

/**
 * Apply the ties. First the ties of the individual functions are applied, then the common ties (inter-function)
 */
void CompositeFunction::applyTies()
{
  for(int i=0;i<nFunctions();i++)
  {
    getFunction(i)->applyTies();
  }
}

/**
 * Clear the ties. 
 */
void CompositeFunction::clearTies()
{
  for(int i=0;i<nFunctions();i++)
  {
    getFunction(i)->clearTies();
  }
}

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i The index of the tied parameter.
 * @return True if successfull
 */
bool CompositeFunction::removeTie(int i)
{
  int iFun = functionIndex(i);
  bool res = m_functions[ iFun ]->removeTie(i - m_paramOffsets[iFun]);
  if (res)
  {
    m_nActive++;
  }
  return res;
}

/** Get the tie of i-th parameter
 * @param i The parameter index
 * @return A pointer to the tie.
 */
ParameterTie* CompositeFunction::getTie(int i)const
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->getTie(i - m_paramOffsets[iFun]);
}

/**
 * Attaches a tie to this function. The attached tie is owned by the function.
 * @param tie A pointer to a new tie
 */
void CompositeFunction::addTie(ParameterTie* tie)
{
  int i = getParameterIndex(*tie);
  if (i < 0)
  {
    throw std::logic_error("Trying to use a tie on a parameter not belonging to this function");
  }
  int iFun = functionIndex(i);
  m_functions[iFun]->addTie(tie);
}

/**
 * Declare a new parameter. To used in the implementation'c constructor.
 * @param name The parameter name.
 * @param initValue The initial value for the parameter
 */
void CompositeFunction::declareParameter(const std::string& name,double initValue )
{
  throw Kernel::Exception::NotImplementedError("CompositeFunction cannot not have its own parameters.");
}

/** Add a constraint
 *  @param ic Pointer to a constraint.
 */
void CompositeFunction::addConstraint(IConstraint* ic)
{
  throw Kernel::Exception::NotImplementedError("addConstraint is not implemented for CompositeFunction yet.");
}

void CompositeFunction::setParametersToSatisfyConstraints()
{
  for(int i=0;i<nFunctions();i++)
  {
    getFunction(i)->setParametersToSatisfyConstraints();
  }
}

/// Get first constraint
IConstraint* CompositeFunction::firstConstraint()
{
  m_iConstraintFunction = 0;
  if (nFunctions() == 0) 
  {
    return 0;
  }
  IConstraint* c = 0;
  while(m_iConstraintFunction < nFunctions()
    && !(c = getFunction(m_iConstraintFunction)->firstConstraint()) )
  {
    m_iConstraintFunction++;
  }
  return c;
}

/// Get next constraint
IConstraint* CompositeFunction::nextConstraint()
{
  if (nFunctions() == 0) 
  {
    return 0;
  }
  if (m_iConstraintFunction >= nFunctions()-1)
  {
    return getFunction(m_iConstraintFunction)->nextConstraint();
  }
  IConstraint* c = getFunction(m_iConstraintFunction)->nextConstraint();
  if (c)
  {
    return c;
  }
  ++m_iConstraintFunction;
  return getFunction(m_iConstraintFunction)->firstConstraint();
}

/** 
 *  @param i The parameter index
 */
bool CompositeFunction::isExplicitlySet(int i)const
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->isExplicitlySet(i - m_paramOffsets[iFun]);
}

/**
 * Returns the index of parameter if the ref points to one of the member function or -1
 * @param ref A reference to a parameter
 * @return Parameter index or -1
 */
int CompositeFunction::getParameterIndex(const ParameterReference& ref)const
{
  if (ref.getFunction() == this && ref.getIndex() < nParams())
  {
    return ref.getIndex();
  }
  for(int iFun=0;iFun<nFunctions();iFun++)
  {
    int iLocalIndex = getFunction(iFun)->getParameterIndex(ref);
    if (iLocalIndex >= 0)
    {
      return m_paramOffsets[iFun] + iLocalIndex;
    }
  }
  return -1;
}



} // namespace API
} // namespace Mantid
