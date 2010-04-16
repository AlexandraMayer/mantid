//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/XMLlogfile.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <muParser.h>
#include <ctime>
#include <fstream>
#include "MantidAPI/LogParser.h"

namespace Mantid
{
namespace API
{

using namespace Kernel;
using namespace API;

Logger& XMLlogfile::g_log = Logger::get("XMLlogfile");

/** Constructor
 *  @param logfileID The logfile id -- the part of the file name which identifies the log 
 *  @param value Rather then extracting value from logfile, specify a value directly
 *  @param paramName The name of the parameter which will be created based on the log values
 *  @param type The type
 *  @param fixed - specific to fitting parameter is it by default fixed or not?
 *  @param extractSingleValueAs Describes the way to extract a single value from the log file( average, first number, etc)
 *  @param eq muParser equation to calculate the parameter value from the log value
 *  @param comp The pointer to the instrument component
 */
XMLlogfile::XMLlogfile(const std::string& logfileID, const std::string& value, const boost::shared_ptr<Kernel::Interpolation>& interpolation, 
                       const std::string& formula, const std::string& formulaUnit, const std::string& paramName, const std::string& type, const std::string& tie, 
                       const std::string& constraintMin, const std::string& constraintMax, const std::string& fitFunc, const std::string& extractSingleValueAs, 
                       const std::string& eq, const Geometry::IComponent* comp)
  : m_logfileID(logfileID), m_value(value), m_paramName(paramName), m_type(type), m_tie(tie),
    m_constraintMin(constraintMin), m_constraintMax(constraintMax), m_fittingFunction(fitFunc),
    m_formula(formula), m_formulaUnit(formulaUnit), m_interpolation(interpolation),
    m_extractSingleValueAs(extractSingleValueAs), m_eq(eq), m_component(comp)
{
}

/** Returns parameter value as generated using possibly equation expression etc
 *
 *  @param logData Data in logfile
 *  @return parameter value
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument definition file
 */
double XMLlogfile::createParamValue(TimeSeriesProperty<double>* logData)
{
  double extractedValue; 

  // get value either as directly specified by user using the 'value' attribute or through
  // a logfile as specified using the 'logfile-id' attribute. Note if both specified 'logfile-id'
  // takes precedence over the 'value' attribute

  if ( !m_logfileID.empty() )
  {
    // get value from time series

    if ( m_extractSingleValueAs.compare("mean" ) == 0 )
    {
      extractedValue = timeMean(logData);
    }
    // Looking for string: "position n", where n is an integer
    else if ( m_extractSingleValueAs.find("position") == 0 && m_extractSingleValueAs.size() >= 10 )
    {
      std::stringstream extractPosition(m_extractSingleValueAs);
      std::string dummy;
      int position;
      extractPosition >> dummy >> position;

      extractedValue = logData->nthValue(position);
    }
    else
    {
      throw Kernel::Exception::InstrumentDefinitionError(std::string("extract-single-value-as attribute for <parameter>")
          + " element (eq=" + m_eq + ") in instrument definition file is not recognised.");
    }
  }
  {
    std::stringstream extractValue(m_value);
    extractValue >> extractedValue;
  }

  // Check if m_eq is specified if yes evaluate this equation

  if ( m_eq.empty() )
    return extractedValue;
  else
  {
    size_t found;
    std::string equationStr = m_eq;
    found = equationStr.find("value");
    if ( found==std::string::npos )
    {
      throw Kernel::Exception::InstrumentDefinitionError(std::string("Equation attribute for <parameter>")
        + " element (eq=" + m_eq + ") in instrument definition file must contain the string: \"value\"."
        + ". \"value\" is replaced by a value from the logfile.");
    }

    std::stringstream readDouble;
    readDouble << extractedValue;
    std::string extractedValueStr = readDouble.str();
    equationStr.replace(found, 5, extractedValueStr);

    // check if more than one 'value' in m_eq

    while ( equationStr.find("value") != std::string::npos )
    {
      found = equationStr.find("value");
      equationStr.replace(found, 5, extractedValueStr);
    }

    try
    {
      mu::Parser p;
      p.SetExpr(equationStr);
      return p.Eval();
    }
    catch (mu::Parser::exception_type &e)
    {
      throw Kernel::Exception::InstrumentDefinitionError(std::string("Equation attribute for <parameter>")
        + " element (eq=" + m_eq + ") in instrument definition file cannot be parsed."
        + ". Muparser error message is: " + e.GetMsg());
    }
  }


}



} // namespace API
} // namespace Mantid
