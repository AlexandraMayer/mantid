#ifndef MANTID_PYTHONINTERFACE_NUMPYCONVERTERS_H_
#define MANTID_PYTHONINTERFACE_NUMPYCONVERTERS_H_
/*
  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#include <boost/python/object.hpp> //Safer way to include Python.h
#include <vector>
#include "MantidKernel/Matrix.h"

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Numpy
    {
      /// Create a numpy array wrapper around existing vector. This is only possible for contiguous data
      DLLExport PyObject *wrapWithNumpy(const std::vector<double> & data);
      /// Create a read-only numpy array wrapper around existing data.
      DLLExport PyObject *wrapWithReadOnlyNumpy(const std::vector<double> & data);

      /// Create a read-only array wrapper around a double Matrix
      DLLExport PyObject *wrapWithNumpy(const Kernel::DblMatrix & data);
      /// Create a read-only array wrapper around a double Matrix
      DLLExport PyObject *wrapWithReadOnlyNumpy(const Kernel::DblMatrix & data);
    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_NUMPYCONVERTERS_H_ */
