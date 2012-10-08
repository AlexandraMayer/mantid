#ifndef MANTID_MDEVENTS_SLICEMD_H_
#define MANTID_MDEVENTS_SLICEMD_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Algorithm that can take a slice out of an original MDEventWorkspace
   * while preserving all the events contained wherein.
    
    @author Janik Zikovsky
    @date 2011-09-27

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport SliceMD  : public SlicingAlgorithm
  {
  public:
    SliceMD();
    ~SliceMD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SliceMD";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDAlgorithms";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    /// Helper method
    template<typename MDE, size_t nd>
    void doExec(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

    /// Method to actually do the slice
    template<typename MDE, size_t nd, typename OMDE, size_t ond>
    void slice(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);


  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_SLICEMD_H_ */
