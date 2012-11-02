#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/BinMD.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Deprecated -> BinMD
    
    @date 2011-11-22

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport BinToMDHistoWorkspace  : public BinMD, public API::DeprecatedAlgorithm
  {
  public:
    BinToMDHistoWorkspace();
    virtual ~BinToMDHistoWorkspace();
    virtual const std::string name() const;
  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_ */
