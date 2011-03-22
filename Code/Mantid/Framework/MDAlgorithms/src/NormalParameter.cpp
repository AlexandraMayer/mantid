﻿#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "MantidMDAlgorithms/NormalParameter.h"
#include <cmath>

namespace Mantid
{
namespace MDAlgorithms
{

NormalParameter::NormalParameter(double n1, double n2, double n3): m_normal(3)
{
  m_normal[0] = n1;
  m_normal[1] = n2;
  m_normal[2] = n3;
  m_isValid = true;
}

NormalParameter::NormalParameter()
{
  this->m_isValid = false;
}

NormalParameter::NormalParameter(const NormalParameter& other)
{
  this->m_isValid = other.m_isValid;
  if(m_isValid)
  {
    this->m_normal = std::vector<double>(3);
    std::copy(other.m_normal.begin(), other.m_normal.end(), this->m_normal.begin());
  }
}

NormalParameter& NormalParameter::operator=(const NormalParameter& other)
{
  if (&other != this)
  {
    this->m_isValid = other.m_isValid;
    this->m_normal.clear();
    this->m_normal = std::vector<double>(3);
    std::copy(other.m_normal.begin(), other.m_normal.end(), this->m_normal.begin());
  }
  return *this;
}

NormalParameter NormalParameter::reflect() const
{
  return NormalParameter(-m_normal.at(0), -m_normal.at(1), -m_normal.at(2));
}

std::string NormalParameter::getName() const
{
  return parameterName();
}

bool NormalParameter::isValid() const
{
  return this->m_isValid;
}

NormalParameter* NormalParameter::clone() const
{
  return new NormalParameter(m_normal.at(0), m_normal.at(1), m_normal.at(2));
}

NormalParameter::~NormalParameter()
{
}

double NormalParameter::getX() const
{
  return m_normal.at(0);
}

double NormalParameter::getY() const
{
  return m_normal.at(1);
}

double NormalParameter::getZ() const
{
  return m_normal.at(2);
}

bool NormalParameter::operator==(const NormalParameter &other) const
{
  return this->m_normal.at(0) == other.m_normal.at(0) && this->m_normal.at(1) == other.m_normal.at(1)
      && this->m_normal.at(2) == other.m_normal.at(2);
}

bool NormalParameter::operator!=(const NormalParameter &other) const
{
  return !(*this == other);
}

std::string NormalParameter::toXMLString() const
{
  std::string valueXMLtext = boost::str(boost::format("%.4f, %.4f, %.4f") % m_normal.at(0)
      % m_normal.at(1) % m_normal.at(2));

  return this->parameterXMLTemplate(valueXMLtext);
}

NormalParameter NormalParameter::asUnitVector() const
{
  double mag = magnitude();
  return NormalParameter(m_normal[0]/mag, m_normal[1]/mag, m_normal[2]/mag);
}

bool NormalParameter::isUnitVector() const
{
  return 1 == magnitude();
}

double NormalParameter::magnitude() const
{
  return std::sqrt(m_normal[0]*m_normal[0] + m_normal[1]*m_normal[1] + m_normal[2]*m_normal[2]);
}

}

}
