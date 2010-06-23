#ifndef MANTID_GEOMETRY_FITPARAMETER_H_
#define MANTID_GEOMETRY_FITPARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
//#include <fstream>
#include "MantidKernel/System.h"
#include "MantidKernel/Interpolation.h"

namespace Mantid
{
  namespace Geometry
  {
    /**
    Store information about a fitting parameter such as its value
    if it is constrained or tied. Main purpose is to use this 
    class as a type for storing information about a fitting parameter
    in the parameter map of the workspace.

    @author Anders Markvardsen, ISIS, RAL
    @date 26/2/2010

    Copyright &copy; 2007-10 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport FitParameter 
    {
    public:
      /// Constructor
      FitParameter() : m_value(0.0), m_tie(""), m_function("") {};

      /// get paramter value
      double getValue(const double& at) const;
      /// get paramter value
      double getValue() const;
      /// set parameter value
      double& setValue() {return m_value;}
      /// get tie
      std::string getTie() const { return m_tie; }
      /// set tie
      std::string& setTie() { return m_tie; }
      /// get constraint and penalty factor
      std::string getConstraint() const;
      std::string getConstraintPenaltyFactor() const { return m_constraintPenaltyFactor; }
      /// set constraint and penalty factors
      std::string& setConstraintMin() { return m_constraintMin; }
      std::string& setConstraintMax() { return m_constraintMax; }
      std::string& setConstraintPenaltyFactor() { return m_constraintPenaltyFactor; }
      /// get formula
      std::string getFormula() const { return m_formula; }
      /// set formula
      std::string& setFormula() { return m_formula; }
      /// get formula unit
      std::string getFormulaUnit() const { return m_formulaUnit; }
      /// set formula unit
      std::string& setFormulaUnit() { return m_formulaUnit; }
      /// get result formula unit
      std::string getResultUnit() const { return m_resultUnit; }
      /// set result formula unit
      std::string& setResultUnit() { return m_resultUnit; }
      /// get function
      std::string getFunction() const { return m_function; }
      /// set function
      std::string& setFunction() { return m_function; }
      /// get name
      std::string getName() const { return m_name; }
      /// set name
      std::string& setName() { return m_name; }
      /// get look up table
      const Kernel::Interpolation& getLookUpTable() const { return m_lookUpTable; }
      /// set look up table
      Kernel::Interpolation& setLookUpTable() { return m_lookUpTable; }

      /// Prints object to stream
      void printSelf(std::ostream& os) const;

    private:
      /// value of parameter
      mutable double m_value;
      /// tie of parameter
      std::string m_tie;
      /// constraint of parameter
      std::string m_constraintMin;
      std::string m_constraintMax;
      std::string m_constraintPenaltyFactor;
      /// name of fitting function
      std::string m_function;
      /// name of parameter
      std::string m_name;
      /// look up table
      Kernel::Interpolation m_lookUpTable;
      /// formula
      std::string m_formula;
      std::string m_formulaUnit;
      std::string m_resultUnit;

	    /// Static reference to the logger class
	    static Kernel::Logger& g_log;
    };

    // defining operator << and >>
    DLLExport std::ostream& operator<<(std::ostream&, const FitParameter& );
    DLLExport std::istream& operator>>(std::istream&,FitParameter&);

  } // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_FITPARAMETER_H_*/
