#ifndef MANTID_VATES_VTK_MDEW_HEXAHEDRON_FACTORY_H_
#define MANTID_VATES_VTK_MDEW_HEXAHEDRON_FACTORY_H_

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace VATES
{

/** Class is used to generate vtkUnstructuredGrids from IMDEventWorkspaces. Utilises the non-uniform nature of the underlying workspace grid/box structure
as the basis for generating visualisation cells. The recursion depth through the box structure is configurable.

 @author Owen Arnold, Tessella plc
 @date 27/July/2011

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

class DLLExport vtkMDEWHexahedronFactory : public vtkDataSetFactory
{

public:

  /// Constructor
  vtkMDEWHexahedronFactory(ThresholdRange_scptr thresholdRange, const std::string& scalarName, const size_t maxDepth = 1000);

  /// Destructor
  virtual ~vtkMDEWHexahedronFactory();

  /// Factory Method. Should also handle delegation to successors.
  virtual vtkDataSet* create() const;
  
  /// Create as a mesh only.
  virtual vtkDataSet* createMeshOnly() const;

  /// Create the scalar array only.
  virtual vtkFloatArray* createScalarArray() const;

  /// Initalize with a target workspace.
  virtual void initialize(Mantid::API::Workspace_sptr);

  /// Get the name of the type.
  virtual std::string getFactoryTypeName() const
  {
    return "vtkMDEWHexahedronFactory";
  }

  virtual void setRecursionDepth(size_t depth);
  
  /// Typedef sptr to 3d event workspace.
  typedef boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace3> MDEventWorkspace3_sptr;

private:

  /// Template Method pattern to validate the factory before use.
  virtual void validate() const;

  /// Threshold range strategy.
  ThresholdRange_scptr m_thresholdRange;

  /// Scalar name to provide on dataset.
  const std::string m_scalarName;

  /// Member workspace to generate vtkdataset from.
  MDEventWorkspace3_sptr m_workspace;

  /// Maximum recursion depth to use.
  size_t m_maxDepth;

};


}
}


#endif