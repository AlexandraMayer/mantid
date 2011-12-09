#ifndef MANTID_ALGORITHMS_FILTERBYLOGVALUE_H_
#define MANTID_ALGORITHMS_FILTERBYLOGVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

namespace Algorithms
{


/** Filters events in an EventWorkspace using values in a SampleLog.

    @author Janik Zikovsky, SNS
    @date September 15th, 2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FilterByLogValue : public API::Algorithm
{
public:
  FilterByLogValue();
  virtual ~FilterByLogValue();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FilterByLogValue";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Events\\EventFiltering";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();

  /// Pointer for an event workspace
  EventWorkspace_const_sptr eventW;
};



} // namespace Algorithms
} // namespace Mantid


#endif /* MANTID_ALGORITHMS_FILTERBYLOGVALUE_H_ */


