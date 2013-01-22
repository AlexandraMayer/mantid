#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidKernel/DateAndTime.h"
#include <sstream>

namespace Mantid
{
namespace API
{
  /// Does nothing other than make the compiler happy.
  DeprecatedAlgorithm::DeprecatedAlgorithm()
  {
    this->m_replacementAlgorithm = "";
  }

  /// Does nothing other than make the compiler happy.
  DeprecatedAlgorithm::~DeprecatedAlgorithm()
  {}

  /// The algorithm to use instead of this one.
  void DeprecatedAlgorithm::useAlgorithm(const std::string & replacement)
  {
    if (!replacement.empty())
      this->m_replacementAlgorithm = replacement;
    else
      this->m_replacementAlgorithm = "";
  }

  /// The date the algorithm was deprecated on
  void DeprecatedAlgorithm::deprecatedDate(const std::string & date)
  {
    this->m_deprecatdDate = "";
    if (date.empty()) {
      // TODO warn people that it wasn't set
      return;
    }
    if (!Kernel::DateAndTime::stringIsISO8601(date)) {
      // TODO warn people that it wasn't set
      return;
    }
    this->m_deprecatdDate = date;
  }

  /// This merely prints the deprecation error for people to see.
  const std::string DeprecatedAlgorithm::deprecationMsg(const IAlgorithm *algo)
  {
    std::stringstream msg;
    if (algo != NULL)
      msg << algo->name() << " is ";

    msg << "deprecated";

    if (!this->m_deprecatdDate.empty())
      msg << " (on " << this->m_deprecatdDate << ")";

    if (this->m_replacementAlgorithm.empty())
    {
      msg << " and has no replacement.";
    }
    else
    {
      msg << ". Use " << this->m_replacementAlgorithm << " instead.";
    }

    return msg.str();
  }
} // namesapce API
} // namespace Mantid
