#include "MantidMDAlgorithms/HeightParameter.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace Mantid
{
  namespace MDAlgorithms
  {

    HeightParameter::HeightParameter(double height) : m_height(height)
    {
      if(height >= 0)
      {
         m_isValid = true;
      }
      else
      {
        m_isValid = false;
      }
    }

    HeightParameter::HeightParameter()
    { 
      m_isValid = false;
    }

    HeightParameter::HeightParameter(const HeightParameter& other)
    {
      this->m_isValid = other.m_isValid;
      this->m_height = other.m_height;
    }

    HeightParameter& HeightParameter::operator=(const HeightParameter& other)
    {
      if (&other != this)
      {
        this->m_isValid = other.m_isValid;
        this->m_height = other.m_height;
      }
      return *this;
    }

    std::string HeightParameter::getName() const
    {
      return parameterName();
    }

    bool HeightParameter::isValid() const
    {
      return this->m_isValid;
    }

    HeightParameter* HeightParameter::clone() const
    {
      return new HeightParameter(m_height);
    }

    HeightParameter::~HeightParameter()
    {
    }

    double HeightParameter::getValue() const
    {
      return m_height;
    }


    bool HeightParameter::operator==(const HeightParameter &other) const
    {
      return this->m_height == other.m_height;
    }

    bool HeightParameter::operator!=(const HeightParameter &other) const
    {
      return !(*this == other);
    }

    std::string HeightParameter::toXMLString() const
    {
      std::string valueXMLtext = boost::str(boost::format("%.4f") % m_height);

      return this->parameterXMLTemplate(valueXMLtext);
    }

  }

}
