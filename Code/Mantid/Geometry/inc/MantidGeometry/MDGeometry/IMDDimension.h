#ifndef I_MD_DIMENSION_H
#define I_MD_DIMENSION_H


/*! The class discribes one dimension of multidimensional dataset representing an ortogonal dimension and linear axis. 
*
*   Abstract type for a multi dimensional dimension. Gives a read-only layer to the concrete implementation.

    @author Owen Arnold, RAL ISIS
    @date 12/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#include "MantidKernel/System.h"

namespace Mantid
{
  namespace Geometry
  {

    class DLLExport IMDDimension
    {
    public:
    /// the name of the dimennlsion as can be displayed along the axis
      virtual std::string getName() const = 0;
    /// short name which identify the dimension among other dimensin. A dimension can be usually find by its ID and various  
    /// various method exist to manipulate set of dimensions by their names. 
      virtual std::string getDimensionId() const = 0;

    /// if the dimension is integrated (e.g. have single bin)
      virtual bool getIsIntegrated() const = 0;

      virtual double getMaximum() const = 0;

      virtual double getMinimum() const = 0;
   /// number of bins dimension have (an integrated has one). A axis directed along dimension would have getNBins+1 axis points. 
      virtual unsigned int getNBins() const = 0;
      
	  virtual ~IMDDimension(){};
    };

  }
}
#endif
