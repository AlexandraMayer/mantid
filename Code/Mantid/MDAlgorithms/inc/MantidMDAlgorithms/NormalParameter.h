#ifndef NORMALPARAMETER_H
#define NORMALPARAMETER_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameter.h"

namespace Mantid
{
namespace MDAlgorithms
{
/**

 Implementation of a parameter expressing normal vector information.

 @author Owen Arnold, Tessella plc
 @date 01/10/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class DLLExport NormalParameter: public Mantid::API::ImplicitFunctionParameter
{

private:

  std::vector<double> m_normal;

public:

  NormalParameter(double n1, double n2, double n3);

  NormalParameter();

  NormalParameter(const NormalParameter& other);

  NormalParameter& operator=(const NormalParameter& other);

  bool operator==(const NormalParameter &other) const;

  bool operator!=(const NormalParameter &other) const;

  std::string getName() const;

  bool isValid() const;

  NormalParameter* reflect();

  NormalParameter* clone() const;

  ~NormalParameter();

  double getX() const;

  double getY() const;

  double getZ() const;

  std::string toXMLString() const;

  static std::string parameterName()
  {
    return "NormalParameter";
  }
};
}
}

#endif
